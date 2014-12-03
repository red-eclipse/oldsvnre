#ifdef GAMESERVER
struct duelservmode : servmode
{
    int duelround, dueltime, duelcheck, dueldeath, duelwinner, duelwins;
    bool waitforhumans;
    vector<clientinfo *> duelqueue, allowed, playing, respawns;

    duelservmode() {}

    void shrink()
    {
        allowed.shrink(0);
        playing.shrink(0);
        respawns.shrink(0);
    }

    void doreset()
    {
        dueltime = G(duelcooloff);
        duelcheck = dueldeath = -1;
    }

    void reset()
    {
        waitforhumans = true;
        shrink();
        duelqueue.shrink(0);
        doreset();
        duelround = duelwins = 0;
        duelwinner = -1;
    }

    void position(clientinfo *ci, int n)
    {
        if(m_survivor(gamemode, mutators))
            srvmsgft(ci->clientnum, CON_EVENT, "\fyyou are now \fs\fzgyqueued\fS for the \fs\fgnext match\fS");
        else
        {
            if(n) srvmsgft(ci->clientnum, CON_EVENT, "\fyyou are \fs\fzcy#%d\fS in the \fs\fgduel queue\fS", n+1);
            else srvmsgft(ci->clientnum, CON_EVENT, "\fyyou are \fs\fzcrNEXT\fS in the \fs\fgduel queue\fS");
        }
    }

    void queue(clientinfo *ci, bool pos = true, bool top = false, bool wait = true)
    {
        if(ci->state.actortype >= A_ENEMY || ci->state.state == CS_SPECTATOR) return;
        if(ci->state.actortype == A_PLAYER && waitforhumans) waitforhumans = false;
        int n = duelqueue.find(ci);
        if(top)
        {
            if(n >= 0) duelqueue.remove(n);
            duelqueue.insert(0, ci);
            n = 0;
        }
        else if(n < 0)
        {
            n = duelqueue.length();
            duelqueue.add(ci);
        }
        if(allowed.find(ci) >= 0) allowed.removeobj(ci);
        if(respawns.find(ci) >= 0) respawns.removeobj(ci);
        if(wait && ci->state.state != CS_WAITING) waiting(ci, DROP_RESET);
        if(pos) position(ci, n);
    }

    void entergame(clientinfo *ci)
    {
        queue(ci);
        if(dueltime < 0 && dueldeath < 0 && m_affinity(gamemode)) switch(G(duelaffinity))
        {
            case 2: allowed.add(ci); // fall through because they need to be in respawns too
            case 1: respawns.add(ci); break;
            case 0: default: break;
        }
    }
    void leavegame(clientinfo *ci, bool disconnecting = false)
    {
        if(duelwinner == ci->clientnum)
        {
            duelwinner = -1;
            duelwins = 0;
        }
        duelqueue.removeobj(ci);
        allowed.removeobj(ci);
        playing.removeobj(ci);
        respawns.removeobj(ci);
    }

    bool damage(clientinfo *m, clientinfo *v, int damage, int weap, int flags, int material, const ivec &hitpush, const ivec &hitvel, float dist)
    {
        if(dueltime >= 0 && m->state.actortype < A_ENEMY) return false;
        return true;
    }

    bool canspawn(clientinfo *ci, bool tryspawn = false)
    {
        if(allowed.find(ci) >= 0 || ci->state.actortype >= A_ENEMY) return true;
        else if(tryspawn)
        {
            if(!m_affinity(gamemode)) queue(ci);
            return true;
        }
        if(m_affinity(gamemode) && respawns.find(ci) >= 0)
        {
            int delay = m_xdelay(gamemode, mutators, ci->team);
            if(delay && ci->state.respawnwait(gamemillis, delay)) return false;
            if(m_survivor(gamemode, mutators))
            {
                int alive = 0;
                vector<clientinfo *> mates;
                loopv(clients) if(clients[i]->state.actortype < A_ENEMY && clients[i]->team == ci->team)
                { // includes ci
                    mates.add(clients[i]);
                    if(clients[i]->state.state == CS_ALIVE && (!G(duelbotcheck) || clients[i]->state.actortype != A_BOT)) alive++;
                }
                if(!alive)
                {
                    loopv(mates) if(allowed.find(mates[i]) < 0) allowed.add(mates[i]);
                    duelcheck = gamemillis+1000;
                    return true;
                }
                return false; // only respawn when all dead
            }
            if(allowed.find(ci) < 0) allowed.add(ci);
            duelcheck = gamemillis+1000;
            return true; // overrides it
        }
        return false; // you spawn when we want you to buddy
    }

    void spawned(clientinfo *ci)
    {
        allowed.removeobj(ci);
    }

    void layout()
    {
        loopvj(clients) if(clients[j]->state.actortype < A_ENEMY)
        {
            vector<int> shots;
            loop(a, W_MAX) loop(b, 2)
            {
                loopv(clients[j]->state.weapshots[a][b].projs)
                    shots.add(clients[j]->state.weapshots[a][b].projs[i].id);
                clients[j]->state.weapshots[a][b].projs.shrink(0);
            }
            if(!shots.empty()) sendf(-1, 1, "ri2iv", N_DESTROY, clients[j]->clientnum, shots.length(), shots.length(), shots.getbuf());

        }
        if(m_survivor(gamemode, mutators) || G(duelclear))
            loopv(sents) if(enttype[sents[i].type].usetype == EU_ITEM) setspawn(i, hasitem(i), true, true);
    }

    void scoreaffinity(clientinfo *ci, bool win)
    {
        if(!m_affinity(gamemode) || dueltime >= 0 || duelround <= 0) return;
        respawns.shrink(0);
        #define scoredteam(x,y) (win ? x != y : x == y)
        loopv(clients) if(clients[i]->state.actortype < A_ENEMY) switch(clients[i]->state.state)
        {
            case CS_ALIVE:
                if(playing.find(clients[i]) < 0 || scoredteam(clients[i]->team, ci->team)) queue(clients[i], false);
                break;
            case CS_DEAD:
                if(playing.find(clients[i]) < 0 || scoredteam(clients[i]->team, ci->team))
                {
                    queue(clients[i], false);
                    break;
                }
                if(allowed.find(clients[i]) < 0) allowed.add(clients[i]);
                waiting(clients[i], DROP_RESET);
                duelcheck = gamemillis+1000;
                break;
            case CS_WAITING:
                if(playing.find(clients[i]) < 0 || scoredteam(clients[i]->team, ci->team))
                {
                    queue(clients[i], false);
                    break;
                }
                if(allowed.find(clients[i]) < 0) allowed.add(clients[i]);
                duelcheck = gamemillis+1000;
                break;
            default: break;
        }
    }

    void clear()
    {
        doreset();
        bool reset = false;
        if(m_duel(gamemode, mutators) && G(duelcycle)&(m_team(gamemode, mutators) ? 2 : 1) && duelwinner >= 0 && duelwins > 0)
        {
            clientinfo *ci = (clientinfo *)getinfo(duelwinner);
            if(ci)
            {
                int numwins = G(duelcycles), numplrs = 0;
                loopv(clients)
                    if(clients[i]->state.actortype < A_ENEMY && clients[i]->state.state != CS_SPECTATOR && clients[i]->team == ci->team)
                        numplrs++;
                if(numplrs > (m_team(gamemode, mutators) ? 1 : 2))
                {
                    if(!numwins) numwins = numplrs;
                    if(duelwins >= numwins) reset = true;
                }
            }
            else
            {
                duelwinner = -1;
                duelwins = 0;
            }
        }
        loopv(clients) queue(clients[i], false, !reset && clients[i]->state.state == CS_ALIVE, reset || G(duelreset) || clients[i]->state.state != CS_ALIVE);
        shrink();
    }

    void update()
    {
        if(!canplay() || waitforhumans) return;
        if(dueltime >= 0)
        {
            if(dueltime && ((dueltime -= curtime) <= 0)) dueltime = 0;
            if(!dueltime)
            {
                shrink();
                int wants = max(numteams(gamemode, mutators), 2);
                loopv(clients) if(clients[i]->state.state != CS_SPECTATOR && clients[i]->state.state != CS_ALIVE) queue(clients[i], false);
                loopv(duelqueue)
                {
                    if(m_duel(gamemode, mutators) && playing.length() >= wants) break;
                    if(duelqueue[i]->state.state != CS_ALIVE)
                    {
                        if(duelqueue[i]->state.state != CS_WAITING) waiting(duelqueue[i], DROP_RESET);
                        if(m_duel(gamemode, mutators) && m_team(gamemode, mutators))
                        {
                            bool skip = false;
                            loopvj(playing) if(duelqueue[i]->team == playing[j]->team) { skip = true; break; }
                            if(skip) continue;
                        }
                    }
                    playing.add(duelqueue[i]);
                }
                if(playing.length() >= wants)
                {
                    if(smode) smode->layout();
                    mutate(smuts, mut->layout());
                    duelround++;
                    string fight = "";
                    if(m_duel(gamemode, mutators))
                    {
                        string names = "";
                        loopv(playing)
                        {
                            concatstring(names, colourname(playing[i]));
                            if(i == wants-1) break;
                            else if(i == wants-2) concatstring(names, " and ");
                            else concatstring(names, ", ");
                        }
                        formatstring(fight)("\fyduel between %s, round \fs\fr#%d\fS", names, duelround);
                    }
                    else if(m_survivor(gamemode, mutators)) formatstring(fight)("\fysurvivor, round \fs\fr#%d\fS", duelround);
                    loopv(playing)
                    {
                        if(playing[i]->state.state == CS_ALIVE)
                        {
                            playing[i]->state.lastregen = gamemillis;
                            playing[i]->state.resetresidual();
                            playing[i]->state.health = m_health(gamemode, mutators, playing[i]->state.model);
                            sendf(-1, 1, "ri4", N_REGEN, playing[i]->clientnum, playing[i]->state.health, 0); // amt = 0 regens impulse
                        }
                        else if(allowed.find(playing[i]) < 0) allowed.add(playing[i]);
                        duelqueue.removeobj(playing[i]);
                        if(m_affinity(gamemode)) respawns.add(playing[i]);
                    }
                    loopv(duelqueue) position(duelqueue[i], i);
                    ancmsgft(-1, S_V_FIGHT, CON_INFO, fight);
                    dueltime = dueldeath = -1;
                    duelcheck = gamemillis+5000;
                }
                else shrink();
            }
        }
        else if(duelround > 0)
        {
            bool cleanup = false;
            vector<clientinfo *> alive;
            loopv(clients) if(clients[i]->state.actortype < A_ENEMY && clients[i]->state.state == CS_ALIVE)
            {
                if(playing.find(clients[i]) < 0) queue(clients[i]);
                else alive.add(clients[i]);
            }
            if(!allowed.empty() && duelcheck >= 0 && gamemillis >= duelcheck) loopvrev(allowed)
            {
                if(alive.find(allowed[i]) < 0) spectator(allowed[i]);
                allowed.remove(i);
                cleanup = true;
            }
            if(allowed.empty() && (!m_affinity(gamemode) || respawns.empty()))
            {
                if(m_survivor(gamemode, mutators) && m_team(gamemode, mutators) && !alive.empty())
                {
                    bool found = false;
                    loopv(alive) if(i && alive[i]->team != alive[i-1]->team) { found = true; break; }
                    if(!found)
                    {
                        if(dueldeath < 0) dueldeath = gamemillis+G(dueldelay);
                        else if(gamemillis >= dueldeath)
                        {
                            if(!cleanup)
                            {
                                defformatstring(end)("\fyteam %s are the winners", colourteam(alive[0]->team));
                                bool teampoints = true;
                                loopv(clients) if(playing.find(clients[i]) >= 0)
                                {
                                    if(clients[i]->team == alive[0]->team)
                                    {
                                        ancmsgft(clients[i]->clientnum, S_V_YOUWIN, CON_INFO, end);
                                        if(!m_affinity(gamemode) && alive.find(clients[i]) >= 0)
                                        {
                                            givepoints(clients[i], 1, teampoints);
                                            teampoints = false;
                                        }
                                    }
                                    else ancmsgft(clients[i]->clientnum, S_V_YOULOSE, CON_INFO, end);
                                }
                                else ancmsgft(clients[i]->clientnum, S_V_BOMBSCORE, CON_INFO, end);
                            }
                            clear();
                        }
                    }
                }
                else switch(alive.length())
                {
                    case 0:
                    {
                        if(m_affinity(gamemode) && !playing.empty()) break; // this should not happen
                        if(!cleanup)
                        {
                            ancmsgft(-1, S_V_DRAW, CON_INFO, "\fyeveryone died, \fzoyepic fail");
                            duelwinner = -1;
                            duelwins = 0;
                        }
                        clear();
                        break;
                    }
                    case 1:
                    {
                        if(!m_affinity(gamemode))
                        {
                            if(dueldeath < 0)
                            {
                                dueldeath = gamemillis+G(dueldelay);
                                break;
                            }
                            else if(gamemillis < dueldeath) break;
                        }
                        if(!cleanup)
                        {
                            string end, hp; hp[0] = 0;
                            if(!m_insta(gamemode, mutators) && !m_affinity(gamemode))
                            {
                                if(alive[0]->state.health >= m_health(gamemode, mutators, alive[0]->state.model))
                                    formatstring(hp)(" with a \fs\fcflawless victory\fS");
                                else formatstring(hp)(" with \fs\fc%d\fS health left", alive[0]->state.health);
                            }
                            if(duelwinner != alive[0]->clientnum)
                            {
                                duelwinner = alive[0]->clientnum;
                                duelwins = 1;
                                formatstring(end)("\fy%s was the winner%s", colourname(alive[0]), hp);
                            }
                            else
                            {
                                duelwins++;
                                formatstring(end)("\fy%s was the winner%s (\fs\fc%d\fS in a row)", colourname(alive[0]), hp, duelwins);
                            }
                            loopv(clients)
                            {
                                if(playing.find(clients[i]) >= 0)
                                {
                                    if(clients[i] == alive[0])
                                    {
                                        ancmsgft(clients[i]->clientnum, S_V_YOUWIN, CON_INFO, end);
                                        if(!m_affinity(gamemode)) givepoints(clients[i], 1);
                                    }
                                    else ancmsgft(clients[i]->clientnum, S_V_YOULOSE, CON_INFO, end);
                                }
                                else ancmsgft(clients[i]->clientnum, S_V_BOMBSCORE, CON_INFO, end);
                            }
                        }
                        clear();
                        break;
                    }
                    default: break;
                }
            }
        }
    }

    bool wantsovertime()
    {
        if(dueltime < 0 && duelround > 0) return true;
        return false;
    }

    bool canbalance()
    {
        if(dueltime < 0 && duelround > 0) return false;
        return true;
    }

    void balance(int oldbalance)
    {
        doreset();
    }
} duelmutator;
#endif
