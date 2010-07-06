// server side stf manager
struct defendservmode : defendstate, servmode
{
    int scoresec;
    bool hasflaginfo;

    defendservmode() : scoresec(0), hasflaginfo(false) {}

    void reset(bool empty)
    {
        defendstate::reset();
        scoresec = 0;
        hasflaginfo = false;
    }

    void stealaffinity(int n, int team)
    {
        flag &b = flags[n];
        loopv(clients) if(clients[i]->state.aitype < AI_START)
        {
            server::clientinfo *ci = clients[i];
            if(ci->state.state==CS_ALIVE && ci->team && ci->team == team && insideaffinity(b, ci->state.o))
                b.enter(ci->team);
        }
        sendaffinity(n);
    }

    void moveaffinity(int team, const vec &oldpos, const vec &newpos)
    {
        if(!team) return;
        loopv(flags)
        {
            flag &b = flags[i];
            bool leave = insideaffinity(b, oldpos),
                 enter = insideaffinity(b, newpos);
            if(leave && !enter && b.leave(team)) sendaffinity(i);
            else if(enter && !leave && b.enter(team)) sendaffinity(i);
            else if(leave && enter && b.steal(team)) stealaffinity(i, team);
        }
    }

    void leaveaffinity(int team, const vec &o)
    {
        moveaffinity(team, o, vec(-1e10f, -1e10f, -1e10f));
    }

    void enteraffinity(int team, const vec &o)
    {
        moveaffinity(team, vec(-1e10f, -1e10f, -1e10f), o);
    }

    void addscore(int i, int team, int n)
    {
        if(!n) return;
        flag &b = flags[i];
        loopvk(clients) if(clients[k]->state.aitype < AI_START && team == clients[k]->team && insideaffinity(b, clients[k]->state.o)) givepoints(clients[k], n);
        score &cs = findscore(team);
        cs.total += n;
        sendf(-1, 1, "ri3", N_SCORE, team, cs.total);
    }

    void update()
    {
        endcheck();
        int t = (gamemillis/SCORESECS)-((gamemillis-(curtime+scoresec))/SCORESECS);
        if(t < 1) { scoresec += curtime; return; }
        else scoresec = 0;
        loopv(flags)
        {
            flag &b = flags[i];
            if(b.enemy)
            {
                if(!b.owners || !b.enemies)
                {
                    int pts = b.occupy(b.enemy, GAME(defendpoints)*(b.enemies ? b.enemies : -(1+b.owners))*t, GAME(defendoccupy), m_gsp1(gamemode, mutators));
                    if(pts > 0) loopvk(clients) if(clients[k]->state.aitype < AI_START && b.owner == clients[k]->team && insideaffinity(b, clients[k]->state.o)) givepoints(clients[k], 3);
                }
                sendaffinity(i);
            }
            else if(b.owner)
            {
                b.securetime += t;
                int score = b.securetime/SCORESECS - (b.securetime-t)/SCORESECS;
                if(score) addscore(i, b.owner, score);
                sendaffinity(i);
            }
        }
    }

    void sendaffinity(int i)
    {
        flag &b = flags[i];
        sendf(-1, 1, "ri5", N_INFOAFFIN, i, b.enemy ? b.converted : 0, b.owner, b.enemy);
    }

    void sendaffinity()
    {
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        initclient(NULL, p, false);
        sendpacket(-1, 1, p.finalize());
    }

    void initclient(clientinfo *ci, packetbuf &p, bool connecting)
    {
        if(connecting)
        {
            loopv(scores)
            {
                score &cs = scores[i];
                putint(p, N_SCORE);
                putint(p, cs.team);
                putint(p, cs.total);
            }
        }
        putint(p, N_AFFIN);
        putint(p, flags.length());
        loopv(flags)
        {
            flag &b = flags[i];
            putint(p, b.kinship);
            putint(p, b.converted);
            putint(p, b.owner);
            putint(p, b.enemy);
        }
    }

    void winner(int team, int score)
    {
        sendf(-1, 1, "ri3", N_SCORE, team, score);
        startintermission();
    }

    void endcheck()
    {
        int maxscore = GAME(defendlimit) ? GAME(defendlimit) : INT_MAX-1;
        loopi(numteams(gamemode, mutators))
        {
            int steam = i+TEAM_FIRST;
            if(findscore(steam).total >= maxscore)
            {
                findscore(steam).total = maxscore;
                sendf(-1, 1, "ri3s", N_ANNOUNCE, S_GUIBACK, CON_MESG, "\fyscore limit has been reached");
                winner(steam, maxscore);
                return;
            }
        }
        if(m_gsp2(gamemode, mutators))
        {
            int steam = TEAM_NEUTRAL;
            loopv(flags)
            {
                flag &b = flags[i];
                if(b.owner)
                {
                    if(!steam) steam = b.owner;
                    else if(steam != b.owner)
                    {
                        steam = TEAM_NEUTRAL;
                        break;
                    }
                }
                else
                {
                    steam = TEAM_NEUTRAL;
                    break;
                }
            }
            if(steam)
            {
                findscore(steam).total = INT_MAX-1;
                sendf(-1, 1, "ri3s", N_ANNOUNCE, S_GUIBACK, CON_MESG, "\fyall flags have been secured");
                winner(steam, INT_MAX-1);
                return;
            }
        }
    }

    void entergame(clientinfo *ci)
    {
        if(!hasflaginfo || ci->state.state!=CS_ALIVE || ci->state.aitype >= AI_START) return;
        enteraffinity(ci->team, ci->state.o);
    }

    void spawned(clientinfo *ci)
    {
        if(!hasflaginfo || ci->state.aitype >= AI_START) return;
        enteraffinity(ci->team, ci->state.o);
    }

    void leavegame(clientinfo *ci, bool disconnecting = false)
    {
        if(!hasflaginfo || ci->state.state!=CS_ALIVE || ci->state.aitype >= AI_START) return;
        leaveaffinity(ci->team, ci->state.o);
    }

    void died(clientinfo *ci, clientinfo *actor)
    {
        if(!hasflaginfo || ci->state.aitype >= AI_START) return;
        leaveaffinity(ci->team, ci->state.o);
    }

    void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
    {
        if(!hasflaginfo || ci->state.aitype >= AI_START) return;
        moveaffinity(ci->team, oldpos, newpos);
    }

    void regen(clientinfo *ci, int &total, int &amt, int &delay)
    {
        if(hasflaginfo && GAME(regenflag)) loopv(flags)
        {
            flag &b = flags[i];
            if(b.owner == ci->team && !b.enemy && insideaffinity(b, ci->state.o, 2.f))
            {
                if(GAME(extrahealth)) total = max(GAME(extrahealth), total);
                if(ci->state.lastregen && GAME(regenguard)) delay = GAME(regenguard);
                if(GAME(regenextra)) amt = GAME(regenextra);
                return;
            }
        }
    }

    void parseaffinity(ucharbuf &p)
    {
        int numflags = getint(p);
        if(numflags)
        {
            loopi(numflags)
            {
                int kin = getint(p);
                vec o;
                o.x = getint(p)/DMF;
                o.y = getint(p)/DMF;
                o.z = getint(p)/DMF;
                if(!hasflaginfo) addaffinity(o, kin);
            }
            if(!hasflaginfo)
            {
                hasflaginfo = true;
                sendaffinity();
                loopv(clients) if(clients[i]->state.state==CS_ALIVE) entergame(clients[i]);
            }
        }
    }

    int points(clientinfo *victim, clientinfo *actor)
    {
        bool isteam = victim==actor || victim->team == actor->team;
        int p = isteam ? -1 : 1, v = p;
        loopv(flags) if(insideaffinity(flags[i], victim->state.o)) p += v;
        return p;
    }
} defendmode;
