bool allowbroadcast(int n)
{
    clientinfo *ci = (clientinfo *)getinfo(n);
    return ci && ci->connected && ci->state.aitype < 0;
}

bool hasclient(clientinfo *ci, clientinfo *cp = NULL)
{
    if(!ci || (ci != cp && ci->clientnum != cp->clientnum && ci->state.ownernum != cp->clientnum)) return false;
    return true;
}

int peerowner(int n)
{
    clientinfo *ci = (clientinfo *)getinfo(n);
    if(ci && ci->state.aitype >= 0) return ci->state.ownernum;
    return n;
}

int reserveclients() { return GAME(serverclients)+4; }

void hashpassword(int cn, int sessionid, const char *pwd, char *result, int maxlen)
{
    char buf[2*sizeof(string)];
    formatstring(buf)("%d %d ", cn, sessionid);
    copystring(&buf[strlen(buf)], pwd);
    if(!hashstring(buf, result, maxlen)) *result = '\0';
}

bool checkpassword(clientinfo *ci, const char *wanted, const char *given)
{
    string hash;
    hashpassword(ci->clientnum, ci->sessionid, wanted, hash, sizeof(string));
    return !strcmp(hash, given);
}

namespace auth
{
    ENetSocket socket = ENET_SOCKET_NULL;
    char input[4096];
    vector<char> output;
    int inputpos = 0, outputpos = 0;
    int lastconnect = 0, lastactivity = 0;
    uint nextauthreq = 1;

    extern void connect();
    extern void disconnect();

    bool isconnected() { return socket != ENET_SOCKET_NULL; }

    void addoutput(const char *str)
    {
        int len = strlen(str);
        if(len + output.length() > 64*1024) return;
        output.put(str, len);
    }

    clientinfo *findauth(uint id)
    {
        loopv(clients) if(clients[i]->authreq == id) return clients[i];
        return NULL;
    }

    void reqauth(clientinfo *ci)
    {
        if(!nextauthreq) nextauthreq = 1;
        ci->authreq = nextauthreq++;
        ci->authlevel = -1;
        defformatstring(buf)("reqauth %u %s\n", ci->authreq, ci->authname);
        addoutput(buf);
        sendf(ci->clientnum, 1, "ri2s", N_SERVMSG, CON_MESG, "please wait, requesting credential match");
    }

    bool tryauth(clientinfo *ci, const char *user)
    {
        if(!ci) return false;
        else if(!isconnected())
        {
            sendf(ci->clientnum, 1, "ri2s", N_SERVMSG, CON_MESG, "not connected to authentication server");
            return false;
        }
        else if(ci->authreq)
        {
            sendf(ci->clientnum, 1, "ri2s", N_SERVMSG, CON_MESG, "waiting for previous attempt..");
            return true;
        }
        filtertext(ci->authname, user, true, true, false, 100);
        reqauth(ci);
        return true;
    }

    void setmaster(clientinfo *ci, bool val, int flags = 0)
    {
        int privilege = ci->privilege;
        if(val)
        {
            if(ci->privilege >= flags) return;
            privilege = ci->privilege = flags;
        }
        else
        {
            if(!ci->privilege) return;
            ci->privilege = PRIV_NONE;
            int others = 0;
            loopv(clients) if(clients[i]->privilege >= PRIV_MASTER || clients[i]->local) others++;
            if(!others) mastermode = MM_OPEN;
        }
        srvoutf(2, "\fy%s %s \fs\fc%s\fS", colorname(ci), val ? "claimed" : "relinquished", privname(privilege));
        masterupdate = true;
        if(paused)
        {
            int others = 0;
            loopv(clients) if(clients[i]->privilege >= (GAME(varslock) ? PRIV_ADMIN : PRIV_MASTER) || clients[i]->local) others++;
            if(!others) setpause(false);
        }
    }

    int allowconnect(clientinfo *ci, const char *pwd = "", const char *authname = "")
    {
        if(ci->local) return DISC_NONE;
        if(m_demo(gamemode)) return DISC_PRIVATE;
        if(ci->privilege >= PRIV_ADMIN) return DISC_NONE;
        if(*authname)
        {
            if(ci->connectauth) return DISC_NONE;
            if(tryauth(ci, authname))
            {
                ci->connectauth = true;
                return DISC_NONE;
            }
        }
        if(*pwd)
        {
            if(adminpass[0] && checkpassword(ci, adminpass, pwd))
            {
                if(GAME(automaster)) setmaster(ci, true, PRIV_ADMIN);
                return DISC_NONE;
            }
            if(serverpass[0] && checkpassword(ci, serverpass, pwd)) return DISC_NONE;
        }
        if(numclients() >= GAME(serverclients)) return DISC_MAXCLIENTS;
        uint ip = getclientip(ci->clientnum);
        if(!ci->privilege && !checkipinfo(allows, ip))
        {
            if(mastermode >= MM_PRIVATE || serverpass[0]) return DISC_PRIVATE;
            if(checkipinfo(bans, ip)) return DISC_IPBAN;
        }
        return DISC_NONE;
    }

    void authfailed(uint id)
    {
        clientinfo *ci = findauth(id);
        if(!ci) return;
        ci->authreq = ci->authname[0] = 0;
        ci->authlevel = -1;
        sendf(ci->clientnum, 1, "ri2s", N_SERVMSG, CON_MESG, "authority request failed, please check your credentials");
        if(ci->connectauth)
        {
            ci->connectauth = false;
            int disc = allowconnect(ci);
            if(disc) { disconnect_client(id, disc); return; }
        }
    }

    void authsucceeded(uint id, const char *name, const char *flags)
    {
        clientinfo *ci = findauth(id);
        if(!ci) return;
        ci->authreq = 0;
        int n = -1;
        for(const char *c = flags; *c; *c++) switch(*c)
        {
            case 'a': n = PRIV_ADMIN; break;
            case 'm': n = PRIV_MASTER; break;
            case 'u': n = PRIV_NONE; break;
        }
        ci->authlevel = n;
        srvoutf(2, "\fy%s identified as '\fs\fc%s\fS'", colorname(ci), name);
        if(ci->authlevel > PRIV_NONE && GAME(automaster)) setmaster(ci, true, ci->authlevel);
        if(ci->connectauth)
        {
            ci->connectauth = false;
            if(ci->authlevel < 0)
            {
                int disc = allowconnect(ci);
                if(disc) { disconnect_client(id, disc); return; }
            }
        }
    }

    void authchallenged(uint id, const char *val)
    {
        clientinfo *ci = findauth(id);
        if(!ci) return;
        sendf(ci->clientnum, 1, "riis", N_AUTHCHAL, id, val);
    }

    void answerchallenge(clientinfo *ci, uint id, char *val)
    {
        if(ci->authreq != id) return;
        for(char *s = val; *s; s++)
        {
            if(!isxdigit(*s)) { *s = '\0'; break; }
        }
        defformatstring(buf)("confauth %u %s\n", id, val);
        addoutput(buf);
    }

    void processinput()
    {
        if(inputpos < 0) return;
        const int MAXWORDS = 25;
        char *w[MAXWORDS];
        int numargs = MAXWORDS;
        const char *p = input;
        for(char *end;; p = end)
        {
            end = (char *)memchr(p, '\n', &input[inputpos] - p);
            if(!end) end = (char *)memchr(p, '\0', &input[inputpos] - p);
            if(!end) break;
            *end++ = '\0';
            loopi(MAXWORDS)
            {
                w[i] = (char *)"";
                if(i > numargs) continue;
                char *s = parsetext(p);
                if(s) w[i] = s;
                else numargs = i;
            }
            p += strcspn(p, ";\n\0"); p++;
            if(!strcmp(w[0], "error")) conoutf("authserv error: %s", w[1]);
            else if(!strcmp(w[0], "echo")) conoutf("authserv reply: %s", w[1]);
            else if(!strcmp(w[0], "failauth")) authfailed((uint)(atoi(w[1])));
            else if(!strcmp(w[0], "succauth")) authsucceeded((uint)(atoi(w[1])), w[2], w[3]);
            else if(!strcmp(w[0], "chalauth")) authchallenged((uint)(atoi(w[1])), w[2]);
            else if(!strcmp(w[0], "ban") || !strcmp(w[0], "allow"))
            {
                ipinfo &p = (strcmp(w[0], "ban") ? allows : bans).add();
                p.ip = (uint)(atoi(w[1]));
                p.mask = (uint)(atoi(w[2]));
                p.time = -2; // master info
            }
            //else if(w[0]) conoutf("authserv sent invalid command: %s", w[0]);
            loopj(numargs) if(w[j]) delete[] w[j];
        }
        inputpos = &input[inputpos] - p;
        memmove(input, p, inputpos);
        if(inputpos >= (int)sizeof(input)) disconnect();
    }

    void regserver()
    {
        loopv(bans) if(bans[i].time == -2) bans.remove(i--);
        loopv(allows) if(allows[i].time == -2) allows.remove(i--);
        conoutf("updating authentication server");
        defformatstring(msg)("server %d %d\n", serverport, serverport+1);
        addoutput(msg);
        lastactivity = totalmillis;
    }

    void connect()
    {
        if(socket != ENET_SOCKET_NULL) return;

        lastconnect = totalmillis;
        conoutf("connecting to authentication server %s:[%d]...", servermaster, servermasterport);
        ENetAddress authserver = { ENET_HOST_ANY, servermasterport };
        if(enet_address_set_host(&authserver, servermaster) < 0) conoutf("could not set authentication host");
        else
        {
            socket = enet_socket_create(ENET_SOCKET_TYPE_STREAM);
            if(socket != ENET_SOCKET_NULL)
            {
                if(enet_socket_connect(socket, &authserver) < 0)
                {
                    enet_socket_destroy(socket);
                    socket = ENET_SOCKET_NULL;
                }
                else enet_socket_set_option(socket, ENET_SOCKOPT_NONBLOCK, 1);
            }
            if(socket == ENET_SOCKET_NULL) conoutf("couldn't connect to authentication server");
            else
            {
                output.setsize(0);
                outputpos = inputpos = 0;
                regserver();
                loopv(clients) if(clients[i]->authreq) reqauth(clients[i]);
            }
        }
    }

    void disconnect()
    {
        if(socket == ENET_SOCKET_NULL) return;

        enet_socket_destroy(socket);
        socket = ENET_SOCKET_NULL;

        output.setsize(0);
        outputpos = inputpos = 0;
        conoutf("disconnected from authentication server");
    }

    void flushoutput()
    {
        if(output.empty()) return;

        ENetBuffer buf;
        buf.data = &output[outputpos];
        buf.dataLength = output.length() - outputpos;
        int sent = enet_socket_send(socket, NULL, &buf, 1);
        if(sent > 0)
        {
            lastactivity = totalmillis;
            outputpos += sent;
            if(outputpos >= output.length())
            {
                output.setsize(0);
                outputpos = 0;
            }
        }
        else if(sent < 0) disconnect();
    }

    void flushinput()
    {
        enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
        if(enet_socket_wait(socket, &events, 0) < 0 || !events) return;

        ENetBuffer buf;
        buf.data = &input[inputpos];
        buf.dataLength = sizeof(input) - inputpos;
        int recv = enet_socket_receive(socket, NULL, &buf, 1);

        if(recv > 0)
        {
            inputpos += recv;
            processinput();
        }
        else if(recv < 0) disconnect();
    }

    void update()
    {
        if(!isconnected())
        {
            if(servertype >= 2 && (!lastconnect || totalmillis-lastconnect > 60*1000)) connect();
            if(!isconnected()) return;
        }
        else if(servertype < 2)
        {
            disconnect();
            return;
        }

        if(totalmillis-lastactivity > 10*60*1000) regserver();
        flushoutput();
        flushinput();
    }
}
