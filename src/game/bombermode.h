// server-side bomber manager
struct bomberservmode : bomberstate, servmode
{
    bool hasflaginfo;
    int bombertime, scoresec;

    bomberservmode() : hasflaginfo(false), bombertime(-1) {}

    void reset(bool empty)
    {
        bomberstate::reset();
        hasflaginfo = false;
        bombertime = -1;
    }

    void dropaffinity(clientinfo *ci, const vec &o, const vec &inertia = vec(0, 0, 0), int target = -1)
    {
        if(!hasflaginfo || ci->state.aitype >= AI_START) return;
        vec n = inertia.iszero() ? vec(0, 0, GAME(bomberspeed)/10.f) : inertia, v = o; v.z += 1;
        loopv(flags) if(flags[i].owner == ci->clientnum)
        {
            ivec p(vec(v).mul(DMF)), q(vec(n).mul(DMF));
            sendf(-1, 1, "ri3i7", N_DROPAFFIN, ci->clientnum, target, i, p.x, p.y, p.z, q.x, q.y, q.z);
            bomberstate::dropaffinity(i, v, n, gamemillis);
        }
    }

    void leavegame(clientinfo *ci, bool disconnecting = false)
    {
        if(!hasflaginfo) return;
        dropaffinity(ci, ci->state.o, vec(ci->state.vel).add(ci->state.falling));
    }

    void dodamage(clientinfo *target, clientinfo *actor, int &damage, int &hurt, int &weap, int &flags, const ivec &hitpush)
    {
        //if(weaptype[weap].melee || flags&HIT_CRIT) dropaffinity(target, target->state.o);
    }

    void spawned(clientinfo *ci)
    {
        if(bombertime >= 0) return;
        if(m_isteam(gamemode, mutators))
        {
            int alive[TEAM_MAX] = {0}, numt = numteams(gamemode, mutators);
            loopv(clients) if(clients[i]->state.state == CS_ALIVE) alive[clients[i]->team]++;
            loopk(numt) if(!alive[k+1]) return;
        }
        else
        {
            int alive = 0;
            loopv(clients) if(clients[i]->state.state == CS_ALIVE) alive++;
            if(alive <= 1) return;
        }
        bombertime = gamemillis+GAME(bomberdelay);
        loopvj(sents) if(enttype[sents[j].type].usetype == EU_ITEM) setspawn(j, hasitem(j), true, true);
    }

    void died(clientinfo *ci, clientinfo *actor)
    {
        if(!hasflaginfo) return;
        dropaffinity(ci, ci->state.o, vec(ci->state.vel).add(ci->state.falling));
    }

    int addscore(int team, int points)
    {
        score &cs = teamscore(team);
        cs.total += points;
        return cs.total;
    }

    void scorebomb(clientinfo *ci, int relay, int goal)
    {
        flag g = flags[goal];
        if(!g.enabled) return;
        int score = 0;
        if(g.team != ci->team)
        {
            givepoints(ci, GAME(bomberpoints));
            ci->state.gscore++;
            score = addscore(ci->team, 1);
        }
        else
        {
            givepoints(ci, -GAME(bomberpenalty));
            ci->state.gscore--;
            score = addscore(ci->team, -1);
        }
        bomberstate::returnaffinity(relay, gamemillis, false);
        sendf(-1, 1, "ri5", N_SCOREAFFIN, ci->clientnum, relay, goal, score);
        if(GAME(bomberreset))
        {
            loopvj(clients) if(clients[j]->state.state != CS_SPECTATOR && clients[j]->state.aitype < AI_START)
            {
                if((GAME(bomberreset) >= 2 || clients[j]->team == ci->team) && clients[j]->state.state == CS_ALIVE)
                    waiting(clients[j], 0, DROP_EXPIRE);
            }
            bombertime = -1;
        }
        else bombertime = gamemillis+GAME(bomberdelay);
        loopvj(flags) if(flags[j].enabled)
        {
            bomberstate::returnaffinity(j, gamemillis, false);
            sendf(-1, 1, "ri3", N_RESETAFFIN, j, 0);
        }
        if(GAME(bomberlimit) && score >= GAME(bomberlimit))
        {
            ancmsgft(-1, S_V_NOTIFY, CON_EVENT, "\fyscore limit has been reached");
            startintermission();
        }
    }

    void scoreaffinity(clientinfo *ci, int relay, int goal)
    {
        if(!m_isteam(gamemode, mutators) || !GAME(bomberbasket) || !flags.inrange(relay) || !flags.inrange(goal) || flags[relay].lastowner != ci->clientnum || !flags[relay].droptime) return;
        scorebomb(ci, relay, goal);
    }

    void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
    {
        if(!hasflaginfo || ci->state.aitype >= AI_START) return;
        if(GAME(bomberthreshold) > 0 && oldpos.dist(newpos) >= GAME(bomberthreshold))
            dropaffinity(ci, oldpos, vec(ci->state.vel).add(ci->state.falling));
        if(!m_isteam(gamemode, mutators) || m_gsp1(gamemode, mutators)) return;
        loopv(flags) if(isbomberaffinity(flags[i]) && flags[i].owner == ci->clientnum)
        {
            loopvk(flags)
            {
                flag &f = flags[k];
                if(isbombertarg(f, ci->team) && newpos.dist(f.spawnloc) <= enttype[AFFINITY].radius/2) scorebomb(ci, i, k);
            }
        }
    }

    void takeaffinity(clientinfo *ci, int i)
    {
        if(!hasflaginfo || !flags.inrange(i) || ci->state.state!=CS_ALIVE || ci->state.aitype >= AI_START) return;
        flag &f = flags[i];
        if(!isbomberaffinity(f) || f.owner >= 0 || !f.enabled) return;
        if(f.lastowner == ci->clientnum && f.droptime && (GAME(bomberpickupdelay) < 0 || lastmillis-f.droptime <= GAME(bomberpickupdelay))) return;
        bomberstate::takeaffinity(i, ci->clientnum, gamemillis);
        if(!m_gsp1(gamemode, mutators) && (!f.droptime || f.lastowner != ci->clientnum)) givepoints(ci, GAME(bomberpickuppoints));
        sendf(-1, 1, "ri3", N_TAKEAFFIN, ci->clientnum, i);
    }

    void returnaffinity(int i, int v) // 0 = disable, 1 = return and wait, 2 = return instantly
    {
        bomberstate::returnaffinity(i, gamemillis, v == 2);
        sendf(-1, 1, "ri3", N_RESETAFFIN, i, flags[i].enabled ? v : 0);
        if(v && !flags[i].enabled)
        {
            loopvj(flags) if(flags[j].enabled) returnaffinity(j, 0);
            if(bombertime >= 0) bombertime = gamemillis+GAME(bomberdelay);
        }
    }

    void resetaffinity(clientinfo *ci, int i, bool force = false)
    {
        if(!hasflaginfo || !flags.inrange(i) || ci->state.ownernum >= 0) return;
        flag &f = flags[i];
        if(!isbomberaffinity(f) || f.owner >= 0 || !f.droptime || f.votes.find(ci->clientnum) >= 0 || !f.enabled) return;
        f.votes.add(ci->clientnum);
        if(f.votes.length() >= numclients()/2) returnaffinity(i, 2);
    }

    void layout()
    {
        if(!hasflaginfo) return;
        bombertime = -1;
        loopv(flags) if(flags[i].owner >= 0 || flags[i].droptime) returnaffinity(i, 0);
        bombertime = gamemillis+GAME(bomberdelay);
    }

    void update()
    {
        if(!hasflaginfo || bombertime < 0) return;
        if(bombertime)
        {
            if(gamemillis < bombertime) return;
            int hasaffinity = 0;
            vector<int> candidates[TEAM_MAX];
            loopv(flags) candidates[flags[i].team].add(i);
            int wants = m_gsp1(gamemode, mutators) ? 1 : teamcount(gamemode, mutators);
            loopi(wants)
            {
                int c = candidates[i].length(), r = c > 1 ? rnd(c) : 0;
                if(candidates[i].inrange(r) && flags.inrange(candidates[i][r]) && isteam(gamemode, mutators, flags[candidates[i][r]].team, TEAM_NEUTRAL))
                {
                    bomberstate::returnaffinity(candidates[i][r], gamemillis, true);
                    sendf(-1, 1, "ri3", N_RESETAFFIN, candidates[i][r], 1);
                    hasaffinity++;
                }
            }
            if(hasaffinity < wants)
            {
                if(!candidates[TEAM_NEUTRAL].empty() && !m_gsp1(gamemode, mutators))
                {
                    srvmsgf(-1, "\fzoythis map does have enough goals, switching on hold mutator");
                    sendf(-1, 1, "risi3", N_MAPCHANGE, smapname, 0, gamemode, mutators|(1<<G_M_GSP1));
                    changemap(smapname, gamemode, mutators|(1<<G_M_GSP1));
                    return;
                }
                hasflaginfo = false;
                loopv(flags) sendf(-1, 1, "ri3", N_RESETAFFIN, i, 0);
                srvmsgf(-1, "\fs\fzoythis map is not playable in:\fS %s", gamename(gamemode, mutators));
            }
            else ancmsgft(-1, S_V_BOMBSTART, CON_INFO, "\fathe \fs\fwbomb\fS has been spawned");
            bombertime = 0;
        }
        int t = (gamemillis/GAME(bomberholdinterval))-((gamemillis-(curtime+scoresec))/GAME(bomberholdinterval));
        if(t < 1) scoresec += curtime;
        else scoresec = 0;
        loopv(flags) if(isbomberaffinity(flags[i]))
        {
            flag &f = flags[i];
            if(f.owner >= 0)
            {
                clientinfo *ci = (clientinfo *)getinfo(f.owner);
                if((!m_isteam(gamemode, mutators) || m_gsp1(gamemode, mutators)) && t > 0)
                {
                    int score = GAME(bomberholdpoints)*t;
                    if(score)
                    {
                        int total = 0;
                        givepoints(ci, score);
                        if(m_isteam(gamemode, mutators))
                        {
                            total = addscore(ci->team, score);
                            sendf(-1, 1, "ri3", N_SCORE, ci->team, total);
                        }
                        else total = ci->state.points;
                        if(GAME(bomberholdlimit) && total >= GAME(bomberholdlimit))
                        {
                            ancmsgft(-1, S_V_NOTIFY, CON_EVENT, "\fyscore limit has been reached");
                            startintermission();
                        }
                    }
                }
                if(ci && GAME(bombercarrytime) && gamemillis-f.taketime >= GAME(bombercarrytime))
                {
                    ci->state.weapshots[WEAP_GRENADE][0].add(1);
                    sendf(-1, 1, "ri8", N_DROP, ci->clientnum, -1, 1, WEAP_GRENADE, -1, -1, -1);
                    dropaffinity(ci, ci->state.o, vec(ci->state.vel).add(ci->state.falling));
                    if((!m_isteam(gamemode, mutators) || m_gsp1(gamemode, mutators)) && GAME(bomberholdpenalty))
                    {
                        givepoints(ci, -GAME(bomberholdpenalty));
                        if(m_isteam(gamemode, mutators) && m_gsp1(gamemode, mutators))
                        {
                            int total = addscore(ci->team, -GAME(bomberholdpenalty));
                            sendf(-1, 1, "ri3", N_SCORE, ci->team, total);
                        }
                    }
                }
                continue;
            }
            if(f.droptime && gamemillis-f.droptime >= GAME(bomberresetdelay)) returnaffinity(i, 2);
        }
    }

    void sendaffinity()
    {
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        initclient(NULL, p, false);
        sendpacket(-1, 1, p.finalize());
    }

    void initclient(clientinfo *ci, packetbuf &p, bool connecting)
    {
        if(!hasflaginfo) return;
        putint(p, N_INITAFFIN);
        putint(p, flags.length());
        loopv(flags)
        {
            flag &f = flags[i];
            putint(p, f.team);
            putint(p, f.ent);
            putint(p, f.enabled ? 1 : 0);
            putint(p, f.owner);
            loopj(3) putint(p, int(f.spawnloc[j]*DMF));
            if(f.owner<0)
            {
                putint(p, f.droptime);
                if(f.droptime)
                {
                    loopj(3) putint(p, int(f.droploc[j]*DMF));
                    loopj(3) putint(p, int(f.inertia[j]*DMF));
                }
            }
        }
        loopv(clients)
        {
            clientinfo *oi = clients[i];
            if(!oi->connected || (ci && oi->clientnum == ci->clientnum) || !oi->state.lastbuff) continue;
            putint(p, N_SPHY);
            putint(p, oi->clientnum);
            putint(p, SPHY_BUFF);
            putint(p, 1);
        }
    }

    void regen(clientinfo *ci, int &total, int &amt, int &delay)
    {
        if(!hasflaginfo || !GAME(bomberregenbuff) || !ci->state.lastbuff) return;
        if(GAME(maxhealth)) total = max(int(m_health(gamemode, mutators)*GAME(maxhealth)), total);
        if(ci->state.lastregen && GAME(bomberregendelay)) delay = GAME(bomberregendelay);
        if(GAME(bomberregenextra)) amt += GAME(bomberregenextra);
    }

    void checkclient(clientinfo *ci)
    {
        if(!hasflaginfo || ci->state.state != CS_ALIVE || m_insta(gamemode, mutators)) return;
        #define bomberbuff1 (GAME(bomberbuffing)&1 && isbomberhome(f, ci->team) && ci->state.o.dist(f.spawnloc) <= enttype[AFFINITY].radius*GAME(bomberbuffarea))
        #define bomberbuff2 (GAME(bomberbuffing)&2 && isbomberaffinity(f) && f.owner == ci->clientnum)
        if(GAME(bomberbuffing)) loopv(flags)
        {
            flag &f = flags[i];
            if(f.enabled && (bomberbuff1 || bomberbuff2))
            {
                if(!ci->state.lastbuff) sendf(-1, 1, "ri4", N_SPHY, ci->clientnum, SPHY_BUFF, 1);
                ci->state.lastbuff = gamemillis;
                return;
            }
        }
        if(ci->state.lastbuff && (!GAME(bomberbuffing) || gamemillis-ci->state.lastbuff > GAME(bomberbuffdelay)))
        {
            ci->state.lastbuff = 0;
            sendf(-1, 1, "ri4", N_SPHY, ci->clientnum, SPHY_BUFF, 0);
        }
    }

    void moveaffinity(clientinfo *ci, int cn, int id, const vec &o, const vec &inertia = vec(0, 0, 0))
    {
        if(!flags.inrange(id)) return;
        flag &f = flags[id];
        if(!f.droptime || f.owner >= 0 || !isbomberaffinity(f)) return;
        clientinfo *co = f.lastowner >= 0 ? (clientinfo *)getinfo(f.lastowner) : NULL;
        if(!co || co->clientnum != ci->clientnum) return;
        f.droploc = o;
        f.inertia = inertia;
        //sendf(-1, 1, "ri9", N_MOVEAFFIN, ci->clientnum, id, int(f.droploc.x*DMF), int(f.droploc.y*DMF), int(f.droploc.z*DMF), int(f.inertia.x*DMF), int(f.inertia.y*DMF), int(f.inertia.z*DMF));
    }

    void parseaffinity(ucharbuf &p)
    {
        int numflags = getint(p);
        if(numflags)
        {
            loopi(numflags)
            {
                int team = getint(p), ent = getint(p);
                vec o;
                loopj(3) o[j] = getint(p)/DMF;
                if(!hasflaginfo) addaffinity(o, team, ent);
            }
            if(!hasflaginfo)
            {
                hasflaginfo = true;
                sendaffinity();
                loopv(clients) if(clients[i]->state.state == CS_ALIVE) entergame(clients[i]);
            }
        }
    }

    int points(clientinfo *victim, clientinfo *actor)
    {
        bool isteam = victim==actor || (m_isteam(gamemode, mutators) && victim->team == actor->team);
        int p = isteam ? -1 : (m_isteam(gamemode, mutators) ? 1 : 0), v = p;
        if(p) { loopv(flags) if(flags[i].owner == victim->clientnum) p += v; }
        return p;
    }
} bombermode;
