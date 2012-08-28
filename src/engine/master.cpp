#include "engine.h"
#include <enet/time.h>

#define MASTER_LIMIT 4096
#define CLIENT_TIME (60*1000)
#define SERVER_TIME (35*60*1000)
#define AUTH_TIME (30*1000)
#define DUP_LIMIT 16
#define PING_TIME 3000
#define PING_RETRY 5

VAR(0, masterserver, 0, 0, 1);
VAR(0, masterport, 1, RE_MASTER_PORT, VAR_MAX);
SVAR(0, masterip, "");
SVAR(0, masterscriptclient, "");
SVAR(0, masterscriptserver, "");

struct authuser
{
    char *name, *flags;
    void *pubkey;
};

struct authreq
{
    enet_uint32 reqtime;
    uint id;
    void *answer;
    authuser *user;
};

struct masterclient
{
    ENetAddress address;
    ENetSocket socket;
    string name;
    char input[4096];
    vector<char> output;
    int inputpos, outputpos, port, numpings;
    enet_uint32 lastping, lastpong, lastactivity, lastpush;
    vector<authreq> authreqs;
    bool isserver, ishttp, listserver, shouldping, shouldpurge;

    masterclient() : inputpos(0), outputpos(0), port(RE_SERVER_PORT), numpings(0), lastping(0), lastpong(0), lastactivity(0), lastpush(0), isserver(false), ishttp(false), listserver(false), shouldping(false), shouldpurge(false) {}
};

static vector<masterclient *> masterclients;
static ENetSocket mastersocket = ENET_SOCKET_NULL, pingsocket = ENET_SOCKET_NULL;
static time_t starttime;

bool setuppingsocket(ENetAddress *address)
{
    if(pingsocket != ENET_SOCKET_NULL) return true;
    pingsocket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    if(pingsocket == ENET_SOCKET_NULL) return false;
    if(address && enet_socket_bind(pingsocket, address) < 0) return false;
    enet_socket_set_option(pingsocket, ENET_SOCKOPT_NONBLOCK, 1);
    return true;
}

void setupmaster()
{
    if(masterserver)
    {
        conoutf("init: master (%s:%d)", *masterip ? masterip : "*", masterport);
        ENetAddress address = { ENET_HOST_ANY, enet_uint16(masterport) };
        if(*masterip && enet_address_set_host(&address, masterip) < 0) fatal("failed to resolve master address: %s", masterip);
        if((mastersocket = enet_socket_create(ENET_SOCKET_TYPE_STREAM)) == ENET_SOCKET_NULL) fatal("failed to create master server socket");
        if(enet_socket_set_option(mastersocket, ENET_SOCKOPT_REUSEADDR, 1) < 0) fatal("failed to set master server socket option");
        if(enet_socket_bind(mastersocket, &address) < 0) fatal("failed to bind master server socket");
        if(enet_socket_listen(mastersocket, -1) < 0) fatal("failed to listen on master server socket");
        if(enet_socket_set_option(mastersocket, ENET_SOCKOPT_NONBLOCK, 1) < 0) fatal("failed to make master server socket non-blocking");
        if(!setuppingsocket(&address)) fatal("failed to create ping socket");
        starttime = time(NULL);
        char *ct = ctime(&starttime);
        if(strchr(ct, '\n')) *strchr(ct, '\n') = '\0';
        conoutf("master server started on %s:[%d]", *masterip ? masterip : "localhost", masterport);
    }
}

void masterout(masterclient &c, const char *msg, int len = 0)
{
    if(!len) len = strlen(msg);
    c.output.put(msg, len);
}

void masteroutf(masterclient &c, const char *fmt, ...)
{
    string msg;
    va_list args;
    va_start(args, fmt);
    vformatstring(msg, fmt, args);
    va_end(args);
    masterout(c, msg);
}

static hashtable<char *, authuser> authusers;

void addauth(char *name, char *flags, char *pubkey)
{
    name = newstring(name);
    authuser &u = authusers[name];
    u.name = name;
    u.flags = newstring(flags);
    u.pubkey = parsepubkey(pubkey);
}
COMMAND(0, addauth, "sss");

void clearauth()
{
    enumerate(authusers, authuser, u, { delete[] u.name; delete[] u.flags; freepubkey(u.pubkey); });
    authusers.clear();
}
COMMAND(0, clearauth, "");

void purgeauths(masterclient &c)
{
    int expired = 0;
    loopv(c.authreqs)
    {
        if(ENET_TIME_DIFFERENCE(totalmillis, c.authreqs[i].reqtime) >= AUTH_TIME)
        {
            masteroutf(c, "failauth %u\n", c.authreqs[i].id);
            freechallenge(c.authreqs[i].answer);
            expired = i + 1;
        }
        else break;
    }
    if(expired > 0) c.authreqs.remove(0, expired);
}

void reqauth(masterclient &c, uint id, char *name)
{
    purgeauths(c);

    time_t t = time(NULL);
    char *ct = ctime(&t);
    if(ct)
    {
        char *newline = strchr(ct, '\n');
        if(newline) *newline = '\0';
    }
    string ip;
    if(enet_address_get_host_ip(&c.address, ip, sizeof(ip)) < 0) copystring(ip, "-");
    conoutf("%s: attempting \"%s\" as %u from %s\n", ct ? ct : "-", name, id, ip);

    authuser *u = authusers.access(name);
    if(!u)
    {
        masteroutf(c, "failauth %u\n", id);
        return;
    }

    authreq &a = c.authreqs.add();
    a.user = u;
    a.reqtime = totalmillis;
    a.id = id;
    uint seed[3] = { uint(starttime), uint(totalmillis), randomMT() };
    static vector<char> buf;
    buf.setsize(0);
    a.answer = genchallenge(u->pubkey, seed, sizeof(seed), buf);

    masteroutf(c, "chalauth %u %s\n", id, buf.getbuf());
}

void confauth(masterclient &c, uint id, const char *val)
{
    purgeauths(c);

    loopv(c.authreqs) if(c.authreqs[i].id == id)
    {
        string ip;
        if(enet_address_get_host_ip(&c.address, ip, sizeof(ip)) < 0) copystring(ip, "-");
        if(checkchallenge(val, c.authreqs[i].answer))
        {
            masteroutf(c, "succauth %u \"%s\" \"%s\"\n", id, c.authreqs[i].user->name, c.authreqs[i].user->flags);
            conoutf("succeeded %u (%s [%s]) from %s\n", id, c.authreqs[i].user->name, c.authreqs[i].user->flags, ip);
        }
        else
        {
            masteroutf(c, "failauth %u\n", id);
            conoutf("failed %u (%s) from %s\n", id, c.authreqs[i].user->name, ip);
        }
        freechallenge(c.authreqs[i].answer);
        c.authreqs.remove(i--);
        return;
    }
    masteroutf(c, "failauth %u\n", id);
}

void purgemasterclient(int n)
{
    masterclient &c = *masterclients[n];
    enet_socket_destroy(c.socket);
    conoutf("master peer %s disconnected", c.name);
    delete masterclients[n];
    masterclients.remove(n);
}

void checkmasterpongs()
{
    ENetBuffer buf;
    ENetAddress addr;
    static uchar pong[MAXTRANS];
    for(;;)
    {
        buf.data = pong;
        buf.dataLength = sizeof(pong);
        int len = enet_socket_receive(pingsocket, &addr, &buf, 1);
        if(len <= 0) break;
        loopv(masterclients)
        {
            masterclient &c = *masterclients[i];
            if(c.address.host == addr.host && c.port+1 == addr.port)
            {
                c.lastpong = totalmillis ? totalmillis : 1;
                c.listserver = true;
                c.shouldping = false;
                break;
            }
        }
    }
}

bool checkmasterclientinput(masterclient &c)
{
    if(c.inputpos < 0) return false;
    const int MAXWORDS = 8;
    char *w[MAXWORDS];
    int numargs = MAXWORDS;
    const char *p = c.input;
    for(char *end;; p = end)
    {
        end = (char *)memchr(p, '\n', &c.input[c.inputpos] - p);
        if(!end) end = (char *)memchr(p, '\0', &c.input[c.inputpos] - p);
        if(!end) break;
        *end++ = '\0';
        if(c.ishttp) continue; // eat up the rest of the bytes, we've done our bit

        //conoutf("{%s} %s", c.name, p);
        loopi(MAXWORDS)
        {
            w[i] = (char *)"";
            if(i > numargs) continue;
            char *s = parsetext(p);
            if(s) w[i] = s;
            else numargs = i;
        }

        p += strcspn(p, ";\n\0"); p++;

        if(!strcmp(w[0], "GET") && w[1] && *w[1] == '/') // cheap server-to-http hack
        {
            loopi(numargs)
            {
                if(i)
                {
                    if(i == 1)
                    {
                        char *q = newstring(&w[i][1]);
                        delete[] w[i];
                        w[i] = q;
                    }
                    w[i-1] = w[i];
                }
                else delete[] w[i];
            }
            w[numargs-1] = (char *)"";
            numargs--;
            c.ishttp = true;
        }
        bool found = false;
        if(!strcmp(w[0], "server") && !c.ishttp)
        {
            c.port = RE_SERVER_PORT;
            if(w[1]) c.port = clamp(atoi(w[1]), 1, VAR_MAX);
            c.shouldping = true;
            c.numpings = 0;
            c.lastactivity = c.lastpush = totalmillis ? totalmillis : 1;
            if(c.isserver)
            {
                masteroutf(c, "echo \"server updated, sending ping request\"\n");
                conoutf("master peer %s updated server info",  c.name);
            }
            else
            {
                if(*masterscriptserver) masteroutf(c, "%s\n", masterscriptserver);
                masteroutf(c, "echo \"server registered, sending ping request\"\n");
                conoutf("master peer %s registered as a server",  c.name);
                c.isserver = true;
            }
            loopv(control) if(control[i].flag == ipinfo::LOCAL)
                masteroutf(c, "%s %u %u\n", ipinfotypes[control[i].type], control[i].ip, control[i].mask);
            found = true;
        }
        if(!strcmp(w[0], "version") || !strcmp(w[0], "update"))
        {
            masteroutf(c, "setversion %d %d\n", server::getver(0), server::getver(1));
            if(*masterscriptclient) masteroutf(c, "%s\n", masterscriptclient);
            conoutf("master peer %s was sent the version",  c.name);
            found = true;
        }
        if(!strcmp(w[0], "list") || !strcmp(w[0], "update"))
        {
            int servs = 0;
            loopvj(masterclients)
            {
                masterclient &s = *masterclients[j];
                if(!s.listserver) continue;
                masteroutf(c, "addserver %s %d\n", s.name, s.port);
                servs++;
            }
            conoutf("master peer %s was sent %d server(s)", c.name, servs);
            c.shouldpurge = true;
            found = true;
        }
        if(c.isserver)
        {
            if(!strcmp(w[0], "reqauth")) { reqauth(c, uint(atoi(w[1])), w[2]); found = true; }
            if(!strcmp(w[0], "confauth")) { confauth(c, uint(atoi(w[1])), w[2]); found = true; }
        }
        if(w[0] && !found)
        {
            masteroutf(c, "error \"unknown command %s\"\n", w[0]);
            conoutf("master peer %s (client) sent unknown command: %s",  c.name, w[0]);
        }
        loopj(numargs) if(w[j]) delete[] w[j];
    }
    c.inputpos = &c.input[c.inputpos] - p;
    memmove(c.input, p, c.inputpos);
    return c.inputpos < (int)sizeof(c.input);
}

void checkmaster()
{
    if(mastersocket == ENET_SOCKET_NULL || pingsocket == ENET_SOCKET_NULL) return;

    ENetSocketSet readset, writeset;
    ENetSocket maxsock = max(mastersocket, pingsocket);
    ENET_SOCKETSET_EMPTY(readset);
    ENET_SOCKETSET_EMPTY(writeset);
    ENET_SOCKETSET_ADD(readset, mastersocket);
    ENET_SOCKETSET_ADD(readset, pingsocket);
    loopv(masterclients)
    {
        masterclient &c = *masterclients[i];
        if(c.authreqs.length()) purgeauths(c);
        if(c.shouldping && (!c.lastping || ((!c.lastpong || ENET_TIME_GREATER(c.lastping, c.lastpong)) && ENET_TIME_DIFFERENCE(totalmillis, c.lastping) > PING_TIME)))
        {
            if(c.numpings < PING_RETRY)
            {
                static const uchar ping[] = { 1 };
                ENetBuffer buf;
                buf.data = (void *)ping;
                buf.dataLength = sizeof(ping);
                ENetAddress addr = { c.address.host, enet_uint16(c.port+1) };
                c.numpings++;
                c.lastping = totalmillis ? totalmillis : 1;
                enet_socket_send(pingsocket, &addr, &buf, 1);
            }
            else
            {
                c.listserver = false;
                c.shouldping = false;
                masteroutf(c, "echo \"ping attempts failed, your server will not be listed\n");
            }
        }
        if(c.isserver)
        {
            loopv(control) if(control[i].flag == ipinfo::LOCAL && ENET_TIME_GREATER(control[i].time, c.lastpush))
                masteroutf(c, "%s %u %u\n", ipinfotypes[control[i].type], control[i].ip, control[i].mask);
            c.lastpush = totalmillis ? totalmillis : 1;
        }
        if(c.outputpos < c.output.length()) ENET_SOCKETSET_ADD(writeset, c.socket);
        else ENET_SOCKETSET_ADD(readset, c.socket);
        maxsock = max(maxsock, c.socket);
    }
    if(enet_socketset_select(maxsock, &readset, &writeset, 0)<=0) return;

    if(ENET_SOCKETSET_CHECK(readset, pingsocket)) checkmasterpongs();

    if(ENET_SOCKETSET_CHECK(readset, mastersocket))
    {
        ENetAddress address;
        ENetSocket masterclientsocket = enet_socket_accept(mastersocket, &address);
        if(masterclients.length() >= MASTER_LIMIT || (checkipinfo(control, ipinfo::BAN, address.host) && !checkipinfo(control, ipinfo::ALLOW, address.host)))
            enet_socket_destroy(masterclientsocket);
        else if(masterclientsocket!=ENET_SOCKET_NULL)
        {
            int dups = 0, oldest = -1;
            loopv(masterclients) if(masterclients[i]->address.host == address.host)
            {
                dups++;
                if(oldest<0 || ENET_TIME_LESS(masterclients[i]->lastactivity, masterclients[oldest]->lastactivity)) oldest = i;
            }
            if(dups >= DUP_LIMIT) purgemasterclient(oldest);
            masterclient *c = new masterclient;
            c->address = address;
            c->socket = masterclientsocket;
            c->lastactivity = totalmillis ? totalmillis : 1;
            masterclients.add(c);
            enet_address_get_host_ip(&c->address, c->name, sizeof(c->name));
            conoutf("master peer %s connected", c->name);
        }
    }

    loopv(masterclients)
    {
        masterclient &c = *masterclients[i];
        if(c.outputpos < c.output.length() && ENET_SOCKETSET_CHECK(writeset, c.socket))
        {
            ENetBuffer buf;
            buf.data = (void *)&c.output[c.outputpos];
            buf.dataLength = c.output.length()-c.outputpos;
            int res = enet_socket_send(c.socket, NULL, &buf, 1);
            if(res>=0)
            {
                c.outputpos += res;
                if(c.outputpos>=c.output.length())
                {
                    c.output.setsize(0);
                    c.outputpos = 0;
                    if(c.shouldpurge) { purgemasterclient(i--); continue; }
                }
            }
            else { purgemasterclient(i--); continue; }
        }
        if(ENET_SOCKETSET_CHECK(readset, c.socket))
        {
            ENetBuffer buf;
            buf.data = &c.input[c.inputpos];
            buf.dataLength = sizeof(c.input) - c.inputpos;
            int res = enet_socket_receive(c.socket, NULL, &buf, 1);
            if(res>0)
            {
                c.inputpos += res;
                c.input[min(c.inputpos, (int)sizeof(c.input)-1)] = '\0';
                if(!checkmasterclientinput(c)) { purgemasterclient(i--); continue; }
            }
            else { purgemasterclient(i--); continue; }
        }
        /* if(c.output.length() > OUTPUT_LIMIT) { purgemasterclient(i--); continue; } */
        if(ENET_TIME_DIFFERENCE(totalmillis, c.lastactivity) >= (c.isserver ? SERVER_TIME : CLIENT_TIME))
        {
            purgemasterclient(i--);
            continue;
        }
    }
}

void cleanupmaster()
{
    if(mastersocket != ENET_SOCKET_NULL) enet_socket_destroy(mastersocket);
}

void reloadmaster()
{
    clearauth();
}

