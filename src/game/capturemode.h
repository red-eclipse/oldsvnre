// server-side capture manager
struct captureservmode : capturestate, servmode
{
    bool hasflaginfo;

    captureservmode() : hasflaginfo(false) {}

    void reset(bool empty)
    {
        capturestate::reset();
        hasflaginfo = false;
    }

    void dropaffinity(clientinfo *ci, const vec &o, const vec &inertia = vec(0, 0, 0), int target = -1)
    {
        if(!hasflaginfo || ci->state.aitype >= AI_START) return;
        int numflags = 0, iterflags = 0;
        loopv(flags) if(flags[i].owner == ci->clientnum) numflags++;
        vec dir = inertia, olddir = dir, v = o; v.z += 1;
        if(numflags > 1 && dir.iszero())
        {
            dir.x = -sinf(RAD*ci->state.yaw);
            dir.y = cosf(RAD*ci->state.yaw);
            dir.z = 0;
            olddir = dir.normalize().mul(max(dir.magnitude(), 1.f));
        }
        loopv(flags) if(flags[i].owner == ci->clientnum)
        {
            if(numflags > 1)
            {
                float yaw = -45.f+(90/float(numflags+1)*(iterflags+1));
                dir = vec(olddir).rotate_around_z(yaw*RAD);
                iterflags++;
            }
            ivec p(vec(v).mul(DMF)), q(vec(dir).mul(DMF));
            sendf(-1, 1, "ri3i7", N_DROPAFFIN, ci->clientnum, -1, i, p.x, p.y, p.z, q.x, q.y, q.z);
            capturestate::dropaffinity(i, v, dir, gamemillis);
        }
    }

    void leavegame(clientinfo *ci, bool disconnecting = false)
    {
        if(!hasflaginfo) return;
        dropaffinity(ci, ci->state.o, vec(ci->state.vel).add(ci->state.falling));
    }

    void dodamage(clientinfo *target, clientinfo *actor, int &damage, int &hurt, int &weap, int &flags, const ivec &hitpush)
    {
        //if(weaptype[weap].melee || flags&HIT_CRIT) dropaffinity(target, target->state.o, vec(ci->state.vel).add(ci->state.falling));
    }

    void died(clientinfo *ci, clientinfo *actor)
    {
        if(!hasflaginfo) return;
        dropaffinity(ci, ci->state.o, vec(ci->state.vel).add(ci->state.falling));
    }

    int addscore(int team, int points = 1)
    {
        score &cs = teamscore(team);
        cs.total += points;
        return cs.total;
    }

    void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
    {
        if(!hasflaginfo || ci->state.aitype >= AI_START) return;
        if(GAME(capturethreshold) > 0 && oldpos.dist(newpos) >= GAME(capturethreshold))
            dropaffinity(ci, oldpos, vec(ci->state.vel).add(ci->state.falling));
        loopv(flags) if(flags[i].owner == ci->clientnum)
        {
            loopvk(flags)
            {
                flag &f = flags[k];
                if(f.team == ci->team && (f.owner < 0 || (m_gsp1(gamemode, mutators) && f.owner == ci->clientnum && i == k)) && !f.droptime && newpos.dist(f.spawnloc) <= enttype[AFFINITY].radius*2/3)
                {
                    capturestate::returnaffinity(i, gamemillis);
                    givepoints(ci, GAME(capturepoints));
                    if(flags[i].team != ci->team)
                    {
                        ci->state.gscore++;
                        int score = addscore(ci->team);
                        sendf(-1, 1, "ri5", N_SCOREAFFIN, ci->clientnum, i, k, score);
                        if(GAME(capturelimit) && score >= GAME(capturelimit))
                        {
                            ancmsgft(-1, S_V_NOTIFY, CON_EVENT, "\fyscore limit has been reached");
                            startintermission();
                        }
                    }
                    else sendf(-1, 1, "ri3", N_RETURNAFFIN, ci->clientnum, i);
                }
            }
        }
    }

    void takeaffinity(clientinfo *ci, int i)
    {
        if(!hasflaginfo || !flags.inrange(i) || ci->state.state!=CS_ALIVE || !ci->team || ci->state.aitype >= AI_START) return;
        flag &f = flags[i];
        if(f.owner >= 0 || (f.team == ci->team && !m_gsp3(gamemode, mutators) && (m_gsp2(gamemode, mutators) || !f.droptime))) return;
        if(f.lastowner == ci->clientnum && f.droptime && (GAME(capturepickupdelay) < 0 || lastmillis-f.droptime <= GAME(capturepickupdelay))) return;
        if(!m_gsp(gamemode, mutators) && f.team == ci->team)
        {
            capturestate::returnaffinity(i, gamemillis);
            givepoints(ci, GAME(capturepoints));
            sendf(-1, 1, "ri3", N_RETURNAFFIN, ci->clientnum, i);
        }
        else
        {
            capturestate::takeaffinity(i, ci->clientnum, gamemillis);
            if(f.team != ci->team && (!f.droptime || f.lastowner != ci->clientnum)) givepoints(ci, GAME(capturepickuppoints));
            sendf(-1, 1, "ri3", N_TAKEAFFIN, ci->clientnum, i);
        }
    }

    void resetaffinity(clientinfo *ci, int i)
    {
        if(!hasflaginfo || !flags.inrange(i) || ci->state.ownernum >= 0) return;
        flag &f = flags[i];
        if(f.owner >= 0 || !f.droptime || f.votes.find(ci->clientnum) >= 0) return;
        f.votes.add(ci->clientnum);
        if(f.votes.length() >= numclients()/2)
        {
            capturestate::returnaffinity(i, gamemillis);
            sendf(-1, 1, "ri3", N_RESETAFFIN, i, 2);
        }
    }

    void layout()
    {
        if(!hasflaginfo) return;
        loopv(flags) if(flags[i].owner >= 0 || flags[i].droptime)
        {
            capturestate::returnaffinity(i, gamemillis);
            sendf(-1, 1, "ri3", N_RESETAFFIN, i, 1);
        }
    }

    void update()
    {
        if(!hasflaginfo) return;
        loopv(flags)
        {
            flag &f = flags[i];
            if(m_gsp3(gamemode, mutators) && f.owner >= 0 && f.taketime && gamemillis-f.taketime >= GAME(captureprotectdelay))
            {
                clientinfo *ci = (clientinfo *)getinfo(f.owner);
                if(f.team != ci->team)
                {
                    capturestate::returnaffinity(i, gamemillis);
                    givepoints(ci, GAME(capturepoints));
                    ci->state.gscore++;
                    int score = addscore(ci->team);
                    sendf(-1, 1, "ri5", N_SCOREAFFIN, ci->clientnum, i, -1, score);
                    if(GAME(capturelimit) && score >= GAME(capturelimit))
                    {
                        ancmsgft(-1, S_V_NOTIFY, CON_EVENT, "\fyscore limit has been reached");
                        startintermission();
                    }
                }
            }
            else if(f.owner < 0 && f.droptime && gamemillis-f.droptime >= capturedelay)
            {
                capturestate::returnaffinity(i, gamemillis);
                sendf(-1, 1, "ri3", N_RESETAFFIN, i, 2);
            }
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
            if(!oi || !oi->connected || (ci && oi->clientnum == ci->clientnum) || !oi->state.lastbuff) continue;
            putint(p, N_SPHY);
            putint(p, oi->clientnum);
            putint(p, SPHY_BUFF);
            putint(p, 1);
        }
    }

    void regen(clientinfo *ci, int &total, int &amt, int &delay)
    {
        if(!hasflaginfo || !GAME(captureregenbuff) || !ci->state.lastbuff) return;
        if(GAME(maxhealth)) total = max(int(m_health(gamemode, mutators)*GAME(maxhealth)), total);
        if(ci->state.lastregen && GAME(captureregendelay)) delay = GAME(captureregendelay);
        if(GAME(captureregenextra)) amt += GAME(captureregenextra);
    }

    void checkclient(clientinfo *ci)
    {
        if(!hasflaginfo || ci->state.state != CS_ALIVE || m_insta(gamemode, mutators)) return;
        #define capturebuffx(a) (GAME(capturebuffing)&a && owner && owner->team == ci->team && ci->state.o.dist(owner->state.o) <= enttype[AFFINITY].radius*GAME(capturebuffarea))
        #define capturebuff1    (GAME(capturebuffing)&1 && f.team == ci->team && (owner ? (owner->clientnum == ci->clientnum || capturebuffx(4)): (!f.droptime || m_gsp2(gamemode, mutators)) && ci->state.o.dist(f.droptime ? f.droploc : f.spawnloc) <= enttype[AFFINITY].radius*GAME(capturebuffarea)))
        #define capturebuff2    (GAME(capturebuffing)&2 && f.team != ci->team && owner && (owner->clientnum == ci->clientnum || capturebuffx(8)))
        if(GAME(capturebuffing)) loopv(flags)
        {
            flag &f = flags[i];
            clientinfo *owner = f.owner >= 0 ? (clientinfo *)getinfo(f.owner) : NULL;
            if(capturebuff1 || capturebuff2)
            {
                if(!ci->state.lastbuff) sendf(-1, 1, "ri4", N_SPHY, ci->clientnum, SPHY_BUFF, 1);
                ci->state.lastbuff = gamemillis;
                return;
            }
        }
        if(ci->state.lastbuff && (!GAME(capturebuffing) || gamemillis-ci->state.lastbuff > GAME(capturebuffdelay)))
        {
            ci->state.lastbuff = 0;
            sendf(-1, 1, "ri4", N_SPHY, ci->clientnum, SPHY_BUFF, 0);
        }
    }

    void moveaffinity(clientinfo *ci, int cn, int id, const vec &o, const vec &inertia = vec(0, 0, 0))
    {
        if(!flags.inrange(id)) return;
        flag &f = flags[id];
        if(!f.droptime || f.owner >= 0) return;
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
        bool isteam = victim==actor || victim->team == actor->team;
        int p = isteam ? -1 : 1, v = p;
        loopv(flags) if(flags[i].owner == victim->clientnum) p += v;
        return p;
    }
} capturemode;
