#define GAMESERVER 1
#include "game.h"

namespace server
{
    struct srventity
    {
        vec o;
        int type;
        bool spawned;
        int millis, last;
        attrvector attrs, kin;

        srventity() :  o(0, 0, 0), type(NOTUSED), spawned(false), millis(0), last(0) { reset(); }
        ~srventity() { reset(); }

        void reset()
        {
            o = vec(0, 0, 0);
            attrs.shrink(0);
            kin.shrink(0);
        }
    };

    static const int DEATHMILLIS = 300;

    struct clientinfo;

    struct gameevent
    {
        virtual ~gameevent() {}
        virtual bool flush(clientinfo *ci, int fmillis);
        virtual void process(clientinfo *ci) {}
        virtual bool keepable() const { return false; }
    };

    struct timedevent : gameevent
    {
        int millis;
        bool flush(clientinfo *ci, int fmillis);
    };

    struct shotevent : timedevent
    {
        int id, weap, flags, scale, num;
        ivec from;
        vector<shotmsg> shots;
        void process(clientinfo *ci);
    };

    struct switchevent : timedevent
    {
        int id, weap;
        void process(clientinfo *ci);
    };

    struct dropevent : timedevent
    {
        int id, weap;
        void process(clientinfo *ci);
    };

    struct reloadevent : timedevent
    {
        int id, weap;
        void process(clientinfo *ci);
    };

    struct hitset
    {
        int flags, proj, target;
        union
        {
            int rays;
            int dist;
        };
        ivec dir;
    };

    struct destroyevent : timedevent
    {
        int id, weap, flags, radial, scale;
        vector<hitset> hits;
        bool keepable() const { return true; }
        void process(clientinfo *ci);
    };

    struct suicideevent : gameevent
    {
        int flags;
        void process(clientinfo *ci);
    };

    struct useevent : timedevent
    {
        int id, ent;
        void process(clientinfo *ci);
    };

    struct projectile
    {
        int id, value;
        projectile(int n, int v) : id(n), value(v) {}
        ~projectile() {}
    };
    struct projectilestate
    {
        vector<projectile> projs;
        projectilestate() { reset(); }
        void reset() { projs.shrink(0); }
        void add(int id, int value = -1)
        {
            projs.add(projectile(id, value));
        }
        bool remove(int id)
        {
            loopv(projs) if(projs[i].id==id)
            {
                projs.remove(i);
                return true;
            }
            return false;
        }
        int removeall(int id)
        {
            int count = 0;
            loopvrev(projs) if(projs[i].id==id)
            {
                projs.remove(i);
                count++;
            }
            return count;
        }
        bool find(int id)
        {
            loopv(projs) if(projs[i].id==id) return true;
            return false;
        }
        int value(int id)
        {
            loopv(projs) if(projs[i].id==id) return projs[i].value;
            return -1;
        }
    };

    struct dmghist
    {
        int clientnum, millis;
        dmghist() {}
        dmghist(int c, int m) : clientnum(c), millis(m) {}
        ~dmghist() {}
    };
    extern int gamemode, mutators;
    struct servstate : gamestate
    {
        vec o, vel, falling;
        float yaw, pitch, roll;
        int state;
        projectilestate dropped, weapshots[WEAP_MAX][2];
        int score, spree, crits, rewards, gscore, teamkills, shotdamage, damage, weapjams[WEAP_MAX];
        int lasttimeplayed, timeplayed, aireinit, lastburnowner, lastbleedowner, lastboost;
        vector<int> fraglog, fragmillis, cpnodes;
        vector<dmghist> damagelog;

        servstate() : state(CS_SPECTATOR), aireinit(0), lastburnowner(-1), lastbleedowner(-1) {}

        bool isalive(int millis)
        {
            return state == CS_ALIVE || ((state == CS_DEAD || state == CS_WAITING) && millis-lastdeath <= DEATHMILLIS);
        }

        void reset(bool change = false)
        {
            if(state != CS_SPECTATOR) state = CS_DEAD;
            dropped.reset();
            loopi(WEAP_MAX)
            {
                loopj(2) weapshots[i][j].reset();
                weapjams[i] = 0;
            }
            if(!change) score = timeplayed = 0;
            else gamestate::mapchange();
            frags = spree = crits = rewards = gscore = deaths = teamkills = shotdamage = damage = 0;
            fraglog.shrink(0); fragmillis.shrink(0); cpnodes.shrink(0); damagelog.shrink(0);
            respawn();
        }

        void respawn(int millis = 0, int heal = 0)
        {
            lastboost = 0;
            lastburnowner = lastbleedowner = -1;
            loopi(WEAP_MAX) weapjams[i] = 0;
            gamestate::respawn(millis, heal);
            o = vec(-1e10f, -1e10f, -1e10f);
            vel = falling = vec(0, 0, 0);
            yaw = pitch = roll = 0;
        }
    };

    struct savedscore
    {
        uint ip;
        string name;
        int points, score, frags, spree, crits, rewards, gscore, timeplayed, deaths, teamkills, shotdamage, damage;

        void save(servstate &gs)
        {
            points = gs.points;
            score = gs.score;
            frags = gs.frags;
            spree = gs.spree;
            crits = gs.crits;
            rewards = gs.rewards;
            gscore = gs.gscore;
            deaths = gs.deaths;
            teamkills = gs.teamkills;
            shotdamage = gs.shotdamage;
            damage = gs.damage;
            timeplayed = gs.timeplayed;
        }

        void restore(servstate &gs)
        {
            gs.points = points;
            gs.score = score;
            gs.frags = frags;
            gs.spree = spree;
            gs.crits = crits;
            gs.rewards = rewards;
            gs.gscore = gscore;
            gs.deaths = deaths;
            gs.teamkills = teamkills;
            gs.shotdamage = shotdamage;
            gs.damage = damage;
            gs.timeplayed = timeplayed;
        }

        void mapchange()
        {
            points = frags = spree = crits = rewards = gscore = deaths = teamkills = shotdamage = damage = 0;
        }
    };

    struct votecount
    {
        char *map;
        int mode, muts, count;
        votecount() {}
        votecount(char *s, int n, int m) : map(s), mode(n), muts(m), count(0) {}
    };

    struct clientinfo
    {
        int clientnum, connectmillis, sessionid, overflow, ping, team, lastteam;
        string name, mapvote;
        int modevote, mutsvote, lastvote;
        int privilege;
        bool connected, local, timesync, online, wantsmap, failedmap, connectauth;
        int gameoffset, lastevent;
        servstate state;
        vector<gameevent *> events;
        vector<uchar> position, messages;
        int posoff, poslen, msgoff, msglen;
        uint authreq;
        string authname;
        string clientmap;
        int mapcrc;
        bool warned;
        ENetPacket *clipboard;
        int lastclipboard, needclipboard;

        clientinfo() : clipboard(NULL) { reset(); }
        ~clientinfo() { events.deletecontents(); cleanclipboard(); }

        void addevent(gameevent *e)
        {
            if(state.state==CS_SPECTATOR || events.length()>250) delete e;
            else events.add(e);
        }

        void mapchange(bool change = true)
        {
            mapvote[0] = 0;
            state.reset(change);
            events.deletecontents();
            overflow = 0;
            timesync = wantsmap = failedmap = false;
            lastevent = gameoffset = lastvote = 0;
            team = lastteam = TEAM_NEUTRAL;
            clientmap[0] = '\0';
            mapcrc = 0;
            warned = false;
        }

        void cleanclipboard(bool fullclean = true)
        {
            if(clipboard) { if(--clipboard->referenceCount <= 0) enet_packet_destroy(clipboard); clipboard = NULL; }
            if(fullclean) lastclipboard = 0;
        }

        void reset()
        {
            ping = 0;
            name[0] = 0;
            privilege = PRIV_NONE;
            connected = local = online = wantsmap = failedmap = connectauth = false;
            authreq = 0;
            position.setsize(0);
            messages.setsize(0);
            needclipboard = 0;
            cleanclipboard();
            mapchange(false);
        }

        int getmillis(int millis, int id)
        {
            if(!timesync)
            {
                timesync = true;
                gameoffset = millis-id;
                return millis;
            }
            return gameoffset+id;
        }
    };

    struct worldstate
    {
        int uses;
        vector<uchar> positions, messages;
    };

    namespace aiman {
        int dorefresh = 0;
        extern int findaiclient(int exclude = -1);
        extern bool addai(int type, int ent, int skill);
        extern void deleteai(clientinfo *ci);
        extern bool delai(int type);
        extern void removeai(clientinfo *ci, bool complete = false);
        extern bool reassignai(int exclude = -1);
        extern void checkskills();
        extern void clearai(int type = 0);
        extern void checkai();
    }

    bool hasgameinfo = false;
    int gamemode = G_EDITMODE, mutators = 0, gamemillis = 0, gamelimit = 0;
    string smapname;
    int interm = 0, timeremaining = -1, oldtimelimit = -1;
    bool maprequest = false, inovertime = false;
    enet_uint32 lastsend = 0;
    int mastermode = MM_OPEN;
    bool masterupdate = false, mapsending = false, shouldcheckvotes = false;
    stream *mapdata[3] = { NULL, NULL, NULL };
    vector<clientinfo *> clients, connects;
    vector<worldstate *> worldstates;
    bool reliablemessages = false;

    struct demofile
    {
        string info;
        uchar *data;
        int len;
    };

    vector<demofile> demos;

    VAR(0, maxdemos, 0, 5, 25);
    VAR(0, maxdemosize, 0, 16, 64);
    VAR(0, restrictdemos, 0, 1, 1);
 
    bool demonextmatch = false;
    stream *demotmp = NULL, *demorecord = NULL, *demoplayback = NULL;
    int nextplayback = 0, triggerid = 0;
    struct triggergrp
    {
        int id;
        vector<int> ents;
        triggergrp() { reset(); }
        void reset(int n = 0) { id = n; ents.shrink(0); }
    } triggers[TRIGGERIDS+1];

    struct servmode
    {
        servmode() {}
        virtual ~servmode() {}

        virtual void entergame(clientinfo *ci) {}
        virtual void leavegame(clientinfo *ci, bool disconnecting = false) {}

        virtual void moved(clientinfo *ci, const vec &oldpos, const vec &newpos) {}
        virtual bool canspawn(clientinfo *ci, bool tryspawn = false) { return true; }
        virtual void spawned(clientinfo *ci) {}
        virtual int points(clientinfo *victim, clientinfo *actor)
        {
            if(victim==actor || victim->team == actor->team) return -1;
            return 1;
        }
        virtual void died(clientinfo *victim, clientinfo *actor = NULL) {}
        virtual void changeteam(clientinfo *ci, int oldteam, int newteam) {}
        virtual void initclient(clientinfo *ci, packetbuf &p, bool connecting) {}
        virtual void update() {}
        virtual void reset(bool empty) {}
        virtual void layout() {}
        virtual void intermission() {}
        virtual bool damage(clientinfo *target, clientinfo *actor, int damage, int weap, int flags, const ivec &hitpush = ivec(0, 0, 0)) { return true; }
        virtual void dodamage(clientinfo *target, clientinfo *actor, int &damage, int &hurt, int &weap, int &flags, const ivec &hitpush = ivec(0, 0, 0)) { }
        virtual void regen(clientinfo *ci, int &total, int &amt, int &delay) {}
        virtual void checkclient(clientinfo *ci) {}
    };

    vector<srventity> sents;
    vector<savedscore> savedscores;
    servmode *smode;
    vector<servmode *> smuts;
    #define mutate(a,b) loopvk(a) { servmode *mut = a[k]; { b; } }
    int nplayers, totalspawns;

    vector<score> scores;
    score &teamscore(int team)
    {
        loopv(scores)
        {
            score &cs = scores[i];
            if(cs.team == team) return cs;
        }
        score &cs = scores.add();
        cs.team = team;
        cs.total = 0;
        return cs;
    }

    extern void waiting(clientinfo *ci, int doteam = 0, int drop = 2, bool exclude = false);
    bool chkloadweap(clientinfo *ci, bool request = true)
    {
        loopj(2)
        {
            int aweap = ci->state.loadweap[j];
            if(ci->state.loadweap[j] >= WEAP_ITEM) ci->state.loadweap[j] = WEAP_MELEE;
            else if(ci->state.loadweap[j] >= WEAP_OFFSET) switch(WEAP(ci->state.loadweap[j], allowed))
            {
                case 0: ci->state.loadweap[j] = -1; break;
                case 1: if(m_duke(gamemode, mutators)) { ci->state.loadweap[j] = -1; break; } // fall through
                case 2: case 3: default: break;
            }
            if(!isweap(ci->state.loadweap[j]) && ci->state.aitype == AI_NONE)
            {
                if(request)
                {
                    if(isweap(aweap)) srvmsgft(ci->clientnum, CON_EVENT, "sorry, the \fs\f[%d]%s\fS is not available, please select a different weapon", WEAP(aweap, colour), WEAP(aweap, name));
                    sendf(ci->clientnum, 1, "ri", N_LOADWEAP);
                }
                return false;
            }
        }
        return true;
    }

    struct vampireservmode : servmode
    {
        vampireservmode() {}
        void dodamage(clientinfo *target, clientinfo *actor, int &damage, int &hurt, int &weap, int &flags, const ivec &hitpush = ivec(0, 0, 0))
        {
            if(actor != target && (!m_team(gamemode, mutators) || actor->team != target->team) && actor->state.state == CS_ALIVE && hurt > 0)
            {
                int rgn = actor->state.health, heal = min(actor->state.health+hurt, int(m_health(gamemode, mutators)*GAME(maxhealthvampire))), eff = heal-rgn;
                if(eff)
                {
                    actor->state.health = heal;
                    actor->state.lastregen = gamemillis;
                    sendf(-1, 1, "ri4", N_REGEN, actor->clientnum, actor->state.health, eff);
                }
            }
        }
    } vampiremutator;

    struct spawnservmode : servmode // pseudo-mutator to regulate spawning clients
    {
        vector<clientinfo *> spawnq, playing;

        spawnservmode() {}

        bool spawnqueue(bool all = false, bool needinfo = true)
        {
            return m_fight(gamemode) && !m_duke(gamemode, mutators) && GAME(maxalive) > 0 && (!needinfo || hasgameinfo) && (!all || GAME(maxalivequeue)) && numclients() > 1;
        }

        void queue(clientinfo *ci, bool msg = true, bool wait = true, bool top = false)
        {
            if(spawnqueue(true) && ci->online && ci->state.state != CS_SPECTATOR && ci->state.state != CS_EDITING && ci->state.aitype < AI_START)
            {
                int n = spawnq.find(ci);
                playing.removeobj(ci);
                if(top)
                {
                    if(n >= 0) spawnq.remove(n);
                    spawnq.insert(0, ci);
                }
                else if(n < 0) spawnq.add(ci);
                if(wait && ci->state.state != CS_WAITING) waiting(ci, 0, 1);
                if(msg && allowbroadcast(ci->clientnum) && !top)
                {
                    int maxplayers = max(int(GAME(maxalive)*nplayers), max(int(numclients()*GAME(maxalivethreshold)), GAME(maxaliveminimum)));
                    if(m_team(gamemode, mutators))
                    {
                        if(maxplayers%2) maxplayers++;
                        maxplayers = maxplayers/2;
                    }
                    int alive = 0;
                    loopv(playing) if(playing[i] && ci->team == playing[i]->team) alive++;
                    if(maxplayers-alive == 0)
                    {
                        int wait = 0;
                        loopv(spawnq) if(spawnq[i] && spawnq[i]->team == ci->team && spawnq[i]->state.aitype == AI_NONE)
                        {
                            wait++;
                            if(spawnq[i] == ci)
                            {
                                if(wait > 1) srvmsgft(spawnq[i]->clientnum, CON_EVENT, "\fyyou are \fs\fzcg#%d\fS in the \fs\fgrespawn queue\fS", wait);
                                else srvmsgft(spawnq[i]->clientnum, CON_EVENT, "\fyyou are \fs\fzcrNEXT\fS in the \fs\fgrespawn queue\fS");
                                break;
                            }
                        }
                    }
                }
            }
        }

        void entergame(clientinfo *ci)
        {
            spawnq.removeobj(ci);
            playing.removeobj(ci);
            queue(ci);
        }

        void leavegame(clientinfo *ci, bool disconnecting = false)
        {
            spawnq.removeobj(ci);
            playing.removeobj(ci);
        }

        bool canspawn(clientinfo *ci, bool tryspawn = false)
        {
            if(ci->state.aitype >= AI_START) return true;
            else if(tryspawn)
            {
                if(m_arena(gamemode, mutators) && !chkloadweap(ci, false)) return false;
                if(spawnqueue(true) && spawnq.find(ci) < 0 && playing.find(ci) < 0) queue(ci);
                return true;
            }
            else
            {
                if(m_arena(gamemode, mutators) && !chkloadweap(ci, false)) return false;
                if(m_trial(gamemode) && ci->state.cpmillis < 0) return false;
                int delay = ci->state.aitype >= AI_START && ci->state.lastdeath ? GAME(enemyspawntime) : m_delay(gamemode, mutators);
                if(delay && ci->state.respawnwait(gamemillis, delay)) return false;
                if(spawnqueue() && playing.find(ci) < 0)
                {
                    if(!hasgameinfo) return false;
                    if(GAME(maxalivequeue) && spawnq.find(ci) < 0) queue(ci);
                    int maxplayers = max(int(GAME(maxalive)*nplayers), max(int(numclients()*GAME(maxalivethreshold)), GAME(maxaliveminimum)));
                    if(m_team(gamemode, mutators))
                    {
                        if(maxplayers%2) maxplayers++;
                        maxplayers = maxplayers/2;
                    }
                    int alive = 0;
                    loopv(playing) if(playing[i])
                    {
                        if(playing[i]->state.state != CS_DEAD && playing[i]->state.state != CS_ALIVE)
                        {
                            if(playing[i]->state.state != CS_WAITING || !GAME(maxalivequeue))
                            {
                                playing.removeobj(playing[i--]);
                                continue;
                            }
                        }
                        if(spawnq.find(playing[i]) >= 0) spawnq.removeobj(playing[i]);
                        if(ci->team == playing[i]->team) alive++;
                    }
                    if(alive >= maxplayers) return false;
                    if(GAME(maxalivequeue))
                    {
                        loopv(spawnq) if(spawnq[i] && spawnq[i]->team == ci->team)
                        {
                            if(spawnq[i] != ci && (ci->state.state >= 0 || spawnq[i]->state.aitype == AI_NONE)) return false;
                            break;
                        }
                        // at this point is where it decides this player is spawning, so tell everyone else their position
                        if(maxplayers-alive == 1)
                        {
                            int wait = 0;
                            loopv(spawnq) if(spawnq[i] && spawnq[i] != ci && spawnq[i]->team == ci->team && spawnq[i]->state.aitype == AI_NONE)
                            {
                                wait++;
                                if(allowbroadcast(spawnq[i]->clientnum))
                                {
                                    if(wait > 1) srvmsgft(spawnq[i]->clientnum, CON_EVENT, "\fyyou are \fs\fzcg#%d\fS in the \fs\fgrespawn queue\fS", wait);
                                    else srvmsgft(spawnq[i]->clientnum, CON_EVENT, "\fyyou are \fs\fzcrNEXT\fS in the \fs\fgrespawn queue\fS");
                                }
                            }
                        }
                    }
                    spawnq.removeobj(ci);
                    if(playing.find(ci) < 0) playing.add(ci);
                }
                return true;
            }
            return false;
        }

        void spawned(clientinfo *ci)
        {
            spawnq.removeobj(ci);
            if(playing.find(ci) < 0) queue(ci);
        }

        void died(clientinfo *ci, clientinfo *at)
        {
            spawnq.removeobj(ci);
            if(GAME(maxalivequeue)) playing.removeobj(ci);
        }

        void reset(bool empty)
        {
            spawnq.shrink(0);
            playing.shrink(0);
        }
    } spawnmutator;

    SVAR(0, serverpass, "");
    SVAR(0, adminpass, "");

    int version[2] = {0};
    ICOMMAND(0, setversion, "ii", (int *a, int *b), version[0] = *a; version[1] = *b);

    int mastermask()
    {
        switch(GAME(serveropen))
        {
            case 0: default: return MM_FREESERV; break;
            case 1: return MM_OPENSERV; break;
            case 2: return MM_COOPSERV; break;
            case 3: return MM_VETOSERV; break;
        }
        return 0;
    }

    #define setmod(a,b) { if(a != b) { setvar(#a, b, true);  sendf(-1, 1, "ri2ss", N_COMMAND, -1, &((const char *)#a)[3], #b); } }
    #define setmodf(a,b) { if(a != b) { setfvar(#a, b, true);  sendf(-1, 1, "ri2ss", N_COMMAND, -1, &((const char *)#a)[3], #b); } }

    //void eastereggs()
    //{
    //    if(GAME(alloweastereggs))
    //    {
    //        time_t ct = time(NULL); // current time
    //        struct tm *lt = localtime(&ct);
    //        int month = lt->tm_mon+1, mday = lt->tm_mday; //, day = lt->tm_wday+1
    //        if(GAME(alloweastereggs) >= 2 && month == 4 && mday == 1) setmod(sv_returningfire, 1);
    //    }
    //}

    int numgamevars = 0, numgamemods = 0;
    void resetgamevars(bool flush)
    {
        string val;
        numgamevars = numgamemods = 0;
        enumerate(idents, ident, id, {
            if(id.flags&IDF_SERVER) // reset vars
            {
                val[0] = 0;
                numgamevars++;
                switch(id.type)
                {
                    case ID_VAR:
                    {
                        setvar(id.name, id.def.i, true);
                        if(flush) formatstring(val)(id.flags&IDF_HEX && *id.storage.i >= 0 ? (id.maxval==0xFFFFFF ? "0x%.6X" : "0x%X") : "%d", *id.storage.i);
                        break;
                    }
                    case ID_FVAR:
                    {
                        setfvar(id.name, id.def.f, true);
                        if(flush) formatstring(val)("%f", *id.storage.f);
                        break;
                    }
                    case ID_SVAR:
                    {
                        setsvar(id.name, id.def.s && *id.def.s ? id.def.s : "", true);
                        if(flush) formatstring(val)("%s", *id.storage.s);
                        break;
                    }
                    default: break;
                }
                if(flush) sendf(-1, 1, "ri2ss", N_COMMAND, -1, &id.name[3], val);
            }
        });
#ifndef STANDALONE
        execfile("localexec.cfg", false);
#else
        execfile("servexec.cfg", false);
#endif
        //eastereggs();
    }

    const char *pickmap(const char *suggest, int mode, int muts)
    {
        const char *map = GAME(defaultmap);
        if(!map || !*map) map = choosemap(suggest, mode, muts, m_campaign(gamemode) ? 1 : GAME(maprotate));
        return map && *map ? map : "maps/untitled";
    }

    void setpause(bool on = false)
    {
        if(sv_gamepaused != (on ? 1 : 0))
        {
            setvar("sv_gamepaused", on ? 1 : 0, true);
            sendf(-1, 1, "ri2ss", N_COMMAND, -1, "gamepaused", on ? "1" : "0");
        }
    }

    void resetbans()
    {
        loopvrev(bans) if(bans[i].type == ipinfo::TEMPORARY) bans.remove(i);
    }

    void resetallows()
    {
        loopvrev(allows) if(allows[i].type == ipinfo::TEMPORARY) allows.remove(i);
    }

    void cleanup(bool init = false)
    {
        setpause(false);
        if(sv_botoffset != 0)
        {
            setvar("sv_botoffset", 0, true);
            sendf(-1, 1, "ri2ss", N_COMMAND, -1, "botoffset", "0");
        }
        if(GAME(resetmmonend)) { mastermode = MM_OPEN; resetallows(); }
        if(GAME(resetbansonend)) resetbans();
        if(GAME(resetvarsonend) || init) resetgamevars(true);
        changemap();
    }

    void start() { cleanup(true); }
    void shutdown()
    {
        srvmsgft(-1, CON_EVENT, "\fyserver shutdown in progress..");
        aiman::clearai();
        loopv(clients) if(getinfo(i)) disconnect_client(i, DISC_SHUTDOWN);
    }

    void *newinfo() { return new clientinfo; }
    void deleteinfo(void *ci) { delete (clientinfo *)ci; }

    int numchannels() { return 3; }
    int reserveclients() { return GAME(serverclients)+4; }

    bool hasclient(clientinfo *ci, clientinfo *cp = NULL)
    {
        if(!ci || (ci != cp && ci->clientnum != cp->clientnum && ci->state.ownernum != cp->clientnum)) return false;
        return true;
    }

    int peerowner(int n)
    {
        clientinfo *ci = (clientinfo *)getinfo(n);
        if(ci && ci->state.aitype > AI_NONE) return ci->state.ownernum;
        return n;
    }

    bool allowbroadcast(int n)
    {
        clientinfo *ci = (clientinfo *)getinfo(n);
        return ci && ci->connected && ci->state.aitype == AI_NONE;
    }

    const char *mastermodename(int type)
    {
        switch(type)
        {
            case MM_OPEN: return "open";
            case MM_VETO: return "veto";
            case MM_LOCKED: return "locked";
            case MM_PRIVATE: return "private";
            case MM_PASSWORD: return "password";
            default: return "unknown";
        }
    }

    const char *privname(int type)
    {
        switch(type)
        {
            case PRIV_ADMIN: return "admin";
            case PRIV_MASTER: case PRIV_AUTH: return "master";
            case PRIV_USER: return "user";
            case PRIV_MAX: return "local";
            default: return "alone";
        }
    }

    int numclients(int exclude, bool nospec, int aitype)
    {
        int n = 0;
        loopv(clients)
        {
            if(clients[i]->clientnum >= 0 && clients[i]->name[0] && clients[i]->clientnum != exclude &&
                (!nospec || clients[i]->state.state != CS_SPECTATOR) &&
                    (clients[i]->state.aitype == AI_NONE || (aitype > AI_NONE && clients[i]->state.aitype <= aitype && clients[i]->state.ownernum >= 0)))
                        n++;
        }
        return n;
    }

    bool duplicatename(clientinfo *ci, char *name)
    {
        if(!name) name = ci->name;
        loopv(clients) if(clients[i]!=ci && !strcmp(name, clients[i]->name)) return true;
        return false;
    }

    const char *colorname(clientinfo *ci, char *name = NULL, bool team = true, bool dupname = true)
    {
        if(!name) name = ci->name;
        static string cname;
        formatstring(cname)("\fs\f[%d]%s", TEAM(ci->team, colour), name);
        if(!name[0] || ci->state.aitype == AI_BOT || (ci->state.aitype < AI_START && dupname && duplicatename(ci, name)))
        {
            defformatstring(s)(" [%d]", ci->clientnum);
            concatstring(cname, s);
        }
        concatstring(cname, "\fS");
        return cname;
    }

    bool haspriv(clientinfo *ci, int flag, const char *msg = NULL)
    {
        if(ci->local || ci->privilege >= flag) return true;
        else if(mastermask()&MM_AUTOAPPROVE && flag <= PRIV_MASTER && !numclients(ci->clientnum)) return true;
        else if(msg && *msg)
            srvmsgft(ci->clientnum, CON_EVENT, "\fraccess denied, you need to be %s to %s", privname(flag), msg);
        return false;
    }

    bool cmppriv(clientinfo *ci, clientinfo *cp, const char *msg = NULL)
    {
        mkstring(str); if(msg && *msg) formatstring(str)("%s %s", msg, colorname(cp));
        if(haspriv(ci, cp->local ? PRIV_MAX : cp->privilege, str)) return true;
        return false;
    }

    const char *gameid() { return GAMEID; }
    ICOMMAND(0, gameid, "", (), result(gameid()));
    int getver(int n)
    {
        switch(n)
        {
            case 0: return RE_VERSION;
            case 1: return GAMEVERSION;
            case 2: case 3: return version[n%2];
        }
        return 0;
    }
    ICOMMAND(0, getversion, "i", (int *a), intret(getver(*a)));
    const char *gamename(int mode, int muts, int compact)
    {
        if(!m_game(mode))
        {
            mode = G_DEATHMATCH;
            muts = m_implied(mode, 0);
        }
        static string gname;
        gname[0] = 0;
        if(gametype[mode].mutators[0] && muts) loopi(G_M_NUM)
        {
            int implied = m_implied(mode, muts);
            if((gametype[mode].mutators[0]&mutstype[i].type) && (muts&mutstype[i].type) && (!implied || !(implied&mutstype[i].type)))
            {
                const char *mut = i < G_M_GSP ? mutstype[i].name : gametype[mode].gsp[i-G_M_GSP];
                if(mut && *mut)
                {
                    string name;
                    switch(compact)
                    {
                        case 2: formatstring(name)("%s%c", *gname ? gname : "", mut[0]); break;
                        case 1: formatstring(name)("%s%s%c", *gname ? gname : "", *gname ? "-" : "", mut[0]); break;
                        case 0: default: formatstring(name)("%s%s%s", *gname ? gname : "", *gname ? "-" : "", mut); break;
                    }
                    copystring(gname, name);
                }
            }
        }
        defformatstring(mname)("%s%s%s", *gname ? gname : "", *gname ? " " : "", gametype[mode].name);
        copystring(gname, mname);
        return gname;
    }
    ICOMMAND(0, gamename, "iii", (int *g, int *m, int *c), result(gamename(*g, *m, *c)));

    const char *modedesc(int mode, int muts, int type)
    {
        if(!m_game(mode))
        {
            mode = G_DEATHMATCH;
            muts = m_implied(mode, 0);
        }
        static string mdname; mdname[0] = 0;
        if(type == 1 || type == 3 || type == 4) concatstring(mdname, gametype[mode].name);
        if(type == 3 || type == 4) concatstring(mdname, ": ");
        if(type == 2 || type == 3 || type == 4 || type == 5)
        {
            if((type == 4 || type == 5) && m_capture(mode) && m_gsp3(mode, muts)) concatstring(mdname, gametype[mode].gsd[2]);
            else if((type == 4 || type == 5) && m_bomber(mode) && m_gsp2(mode, muts)) concatstring(mdname, gametype[mode].gsd[1]);
            else concatstring(mdname, gametype[mode].desc);
        }
        return mdname;
    }
    ICOMMAND(0, modedesc, "iii", (int *g, int *m, int *c), result(modedesc(*g, *m, *c)));
    VAR(0, maxmodes, G_MAX-1, G_MAX-1, -G_MAX-1);

    const char *mutsdesc(int mode, int muts, int type)
    {
        if(!m_game(mode))
        {
            mode = G_DEATHMATCH;
            muts = m_implied(mode, 0);
        }
        static string mtname; mtname[0] = 0;
        loopi(G_M_NUM) if(muts&mutstype[i].type)
        {
            if(type == 4 || type == 5)
            {
                if(m_capture(mode) && m_gsp3(mode, muts)) break;
                else if(m_bomber(mode) && m_gsp2(mode, muts)) break;
            }
            if(type == 1 || type == 3 || type == 4) concatstring(mtname, i >= G_M_GSP ? gametype[mode].gsp[i-G_M_GSP] : mutstype[i].name);
            if(type == 3 || type == 4) concatstring(mtname, ": ");
            if(type == 2 || type == 3 || type == 4 || type == 5) concatstring(mtname, i >= G_M_GSP ? gametype[mode].gsd[i-G_M_GSP] : mutstype[i].desc);
            break;
        }
        return mtname;
    }
    ICOMMAND(0, mutsdesc, "iii", (int *g, int *m, int *c), result(mutsdesc(*g, *m, *c)));
    VAR(0, maxmuts, G_M_NUM, G_M_NUM, -G_M_NUM);

    void changemode(int &mode, int &muts)
    {
        if(mode < 0)
        {
            mode = GAME(defaultmode);
            if(GAME(rotatemode))
            {
                int num = 0;
                loopi(G_MAX) if((1<<i)&GAME(rotatemodefilter)) num++;
                if(!num) mode = rnd(G_RAND)+G_FIGHT;
                else
                {
                    int r = num > 1 ? rnd(num) : 0, n = 0;
                    loopi(G_MAX) if((1<<i)&GAME(rotatemodefilter))
                    {
                        if(n != r) n++;
                        else { mode = i; break; }
                    }
                }
                if(!mode || !((1<<mode)&GAME(rotatemodefilter))) mode = rnd(G_RAND)+G_FIGHT;
            }
        }
        if(muts < 0)
        {
            muts = GAME(defaultmuts);
            if(GAME(rotatemuts))
            {
                int num = rnd(G_M_NUM+1);
                if(num) loopi(num) if(GAME(rotatemuts) == 1 || !rnd(GAME(rotatemuts)))
                {
                    int rmut = 1<<rnd(G_M_NUM);
                    if(GAME(rotatemutsfilter) && !(GAME(rotatemutsfilter)&rmut)) continue;
                    muts |= rmut;
                    modecheck(mode, muts, rmut);
                }
            }
        }
        modecheck(mode, muts);
    }

    const char *choosemap(const char *suggest, int mode, int muts, int force)
    {
        static string chosen;
        if(suggest && *suggest)
        {
            if(!strncasecmp(suggest, "maps/", 5) || !strncasecmp(suggest, "maps\\", 5))
                copystring(chosen, suggest+5);
            else copystring(chosen, suggest);
        }
        else *chosen = 0;
        int rotate = force ? force : GAME(maprotate);
        if(rotate)
        {
            char *list = NULL;
            maplist(list, mode, muts, numclients());
            if(list)
            {
                int n = listlen(list), p = -1, c = -1;
                if(*chosen)
                {
                    p = listincludes(list, chosen, strlen(chosen));
                    if(p >= 0 && rotate == 1) c = p >= 0 && p < n-1 ? p+1 : 0;
                }
                if(c < 0)
                {
                    c = n ? rnd(n) : 0;
                    if(c == p) c = p >= 0 && p < n-1 ? p+1 : 0;
                }
                if(c >= 0)
                {
                    int len = 0;
                    const char *elem = indexlist(list, c, len);
                    if(len > 0) copystring(chosen, elem, len+1);
                }
                DELETEA(list);
            }
        }
        return *chosen ? chosen : pickmap(suggest, mode, muts);
    }

    bool canload(const char *type)
    {
        if(!strcmp(type, gameid()) || !strcmp(type, "bfa") || !strcmp(type, "bfg"))
            return true;
        return false;
    }

    void startintermission()
    {
        setpause(false);
        timeremaining = 0;
        gamelimit = min(gamelimit, gamemillis);
        inovertime = false;
        if(smode) smode->intermission();
        mutate(smuts, mut->intermission());
        maprequest = false;
        interm = totalmillis+GAME(intermlimit);
        if(!interm) interm = 1;
        sendf(-1, 1, "ri2", N_TICK, 0);
    }

    void checklimits()
    {
        if(m_fight(gamemode))
        {
            if(m_trial(gamemode))
            {
                loopv(clients) if(clients[i]->state.cpmillis < 0 && gamemillis+clients[i]->state.cpmillis >= GAME(triallimit))
                {
                    ancmsgft(-1, S_V_NOTIFY, CON_EVENT, "\fytime trial wait period has timed out");
                    startintermission();
                    return;
                }
            }
            int limit = inovertime ? GAME(overtimelimit) : GAME(timelimit);
            if(limit != oldtimelimit || (gamemillis-curtime>0 && gamemillis/1000!=(gamemillis-curtime)/1000))
            {
                if(limit != oldtimelimit)
                {
                    if(limit) gamelimit += (limit-oldtimelimit)*60000;
                    oldtimelimit = limit;
                }
                if(timeremaining)
                {
                    if(limit)
                    {
                        if(gamemillis >= gamelimit) timeremaining = 0;
                        else timeremaining = (gamelimit-gamemillis+1000-1)/1000;
                    }
                    else timeremaining = -1;
                    bool wantsoneminute = true;
                    if(!timeremaining)
                    {
                        bool wantsovertime = false;
                        if(!inovertime && GAME(overtimeallow))
                        {
                            if(m_team(gamemode, mutators))
                            {
                                int best = -1;
                                loopi(numteams(gamemode, mutators))
                                {
                                    score &cs = teamscore(i+TEAM_FIRST);
                                    if(best < 0 || cs.total > teamscore(best).total)
                                    {
                                        best = i+TEAM_FIRST;
                                        wantsovertime = false;
                                    }
                                    else if(best >= 0 && cs.total == teamscore(best).total)
                                        wantsovertime = true;
                                }
                            }
                            else
                            {
                                int best = -1;
                                loopv(clients) if(clients[i]->state.aitype < AI_START)
                                {
                                    if(best < 0 || clients[i]->state.points > clients[best]->state.points)
                                    {
                                        best = i;
                                        wantsovertime = false;
                                    }
                                    else if(best >= 0 && clients[i]->state.points == clients[best]->state.points)
                                        wantsovertime = true;
                                }
                            }
                        }
                        if(wantsovertime)
                        {
                            limit = oldtimelimit = GAME(overtimelimit);
                            if(limit)
                            {
                                timeremaining = limit*60;
                                gamelimit += timeremaining*1000;
                                ancmsgft(-1, S_V_OVERTIME, CON_EVENT, "\fyovertime, match extended by \fs\fc%d\fS %s", limit, limit > 1 ? "minutes" : "minute");
                            }
                            else
                            {
                                timeremaining = -1;
                                gamelimit = 0;
                                ancmsgft(-1, S_V_OVERTIME, CON_EVENT, "\fyovertime, match extended until someone wins");
                            }
                            inovertime = true;
                            wantsoneminute = false;
                        }
                        else
                        {
                            ancmsgft(-1, S_V_NOTIFY, CON_EVENT, "\fytime limit has been reached");
                            startintermission();
                            return; // bail
                        }
                    }
                    if(timeremaining != 0)
                    {
                        sendf(-1, 1, "ri2", N_TICK, timeremaining);
                        if(wantsoneminute && timeremaining == 60) ancmsgft(-1, S_V_ONEMINUTE, CON_EVENT, "\fzygone minute remains");
                    }
                }
            }
            if(inovertime)
            {
                bool wantsovertime = false;
                if(m_team(gamemode, mutators))
                {
                    int best = -1;
                    loopi(numteams(gamemode, mutators))
                    {
                        score &cs = teamscore(i+TEAM_FIRST);
                        if(best < 0 || cs.total > teamscore(best).total)
                        {
                            best = i+TEAM_FIRST;
                            wantsovertime = false;
                        }
                        else if(best >= 0 && cs.total == teamscore(best).total)
                            wantsovertime = true;
                    }
                }
                else
                {
                    int best = -1;
                    loopv(clients) if(clients[i]->state.aitype < AI_START)
                    {
                        if(best < 0 || clients[i]->state.points > clients[best]->state.points)
                        {
                            best = i;
                            wantsovertime = false;
                        }
                        else if(best >= 0 && clients[i]->state.points == clients[best]->state.points)
                            wantsovertime = true;
                    }
                }
                if(!wantsovertime)
                {
                    ancmsgft(-1, S_V_NOTIFY, CON_EVENT, "\fyovertime has ended, a winner has been chosen");
                    startintermission();
                    return; // bail
                }
            }
            if(GAME(pointlimit) && m_scores(gamemode))
            {
                if(m_team(gamemode, mutators))
                {
                    int best = -1;
                    loopi(numteams(gamemode, mutators)) if(best < 0 || teamscore(i+TEAM_FIRST).total > teamscore(best).total)
                        best = i+TEAM_FIRST;
                    if(best >= 0 && teamscore(best).total >= GAME(pointlimit))
                    {
                        ancmsgft(-1, S_V_NOTIFY, CON_EVENT, "\fyscore limit has been reached");
                        startintermission();
                        return; // bail
                    }
                }
                else
                {
                    int best = -1;
                    loopv(clients) if(clients[i]->state.aitype < AI_START && (best < 0 || clients[i]->state.points > clients[best]->state.points))
                        best = i;
                    if(best >= 0 && clients[best]->state.points >= GAME(pointlimit))
                    {
                        ancmsgft(-1, S_V_NOTIFY, CON_EVENT, "\fyscore limit has been reached");
                        startintermission();
                        return; // bail
                    }
                }
            }
        }
    }

    bool hasitem(int i)
    {
        if(!sents.inrange(i) || m_noitems(gamemode, mutators)) return false;
        switch(sents[i].type)
        {
            case WEAPON:
            {
                int attr = w_attr(gamemode, sents[i].attrs[0], m_weapon(gamemode, mutators));
                if(!isweap(attr)) return false;
                if(m_arena(gamemode, mutators) && attr < WEAP_ITEM && GAME(maxcarry) <= 2) return false;
                switch(WEAP(attr, allowed))
                {
                    case 0: return false;
                    case 1: if(m_duke(gamemode, mutators)) return false; // fall through
                    case 2: if(m_limited(gamemode, mutators)) return false;
                    case 3: default: break;
                }
                if((sents[i].attrs[4] && sents[i].attrs[4] != triggerid) || !m_check(sents[i].attrs[2], sents[i].attrs[3], gamemode, mutators)) return false;
                break;
            }
            default: break;
        }
        return true;
    }

    bool finditem(int i, bool spawned = false, bool carry = false)
    {
        if(sents[i].spawned) return true;
        if(sents[i].type == WEAPON && !(sents[i].attrs[1]&WEAP_F_FORCED)) loopvk(clients)
        {
            clientinfo *ci = clients[k];
            if(ci->state.dropped.find(i) && (!spawned || gamemillis < sents[i].millis)) return true;
            else if(carry) loopj(WEAP_MAX)
                if(ci->online && ci->state.state == CS_ALIVE && ci->state.entid[j] == i && ci->state.hasweap(j, m_weapon(gamemode, mutators)))
                    return spawned;
        }
        if(spawned && gamemillis < sents[i].millis) return true;
        return false;
    }

    template<class T>
    void sortrandomly(vector<T> &src)
    {
        vector<T> dst;
        dst.reserve(src.length());
        while(src.length()) dst.add(src.removeunordered(rnd(src.length())));
        src.move(dst);
    }

    void setupitems(bool update)
    {
        static vector<int> items, actors;
        items.setsize(0); actors.setsize(0);
        int sweap = m_weapon(gamemode, mutators);
        loopv(sents)
        {
            if(sents[i].type == ACTOR && sents[i].attrs[0] >= 0 && sents[i].attrs[0] < AI_TOTAL && (sents[i].attrs[5] == triggerid || !sents[i].attrs[5]) && m_check(sents[i].attrs[3], sents[i].attrs[4], gamemode, mutators))
            {
                sents[i].millis += m_campaign(gamemode) ? 50 : GAME(enemyspawndelay);
                switch(GAME(enemyspawnstyle) == 3 ? rnd(2)+1 : GAME(enemyspawnstyle))
                {
                    case 1: actors.add(i); break;
                    case 2: sents[i].millis += (GAME(enemyspawntime)+rnd(GAME(enemyspawntime)))/2; break;
                    default: break;
                }
            }
            else if(m_fight(gamemode) && enttype[sents[i].type].usetype == EU_ITEM && hasitem(i))
            {
                sents[i].millis += GAME(itemspawndelay);
                switch(GAME(itemspawnstyle) == 3 ? rnd(2)+1 : GAME(itemspawnstyle))
                {
                    case 1: items.add(i); break;
                    case 2:
                    {
                        int delay = sents[i].type == WEAPON ? w_spawn(w_attr(gamemode, sents[i].attrs[0], sweap)) : GAME(itemspawntime);
                        if(delay > 1) sents[i].millis += (delay+rnd(delay))/2;
                        break;
                    }
                    default: break;
                }
            }
        }
        if(!items.empty())
        {
            sortrandomly(items);
            loopv(items) sents[items[i]].millis += GAME(itemspawndelay)*i;
        }
        if(!actors.empty())
        {
            sortrandomly(actors);
            loopv(actors) sents[actors[i]].millis += (m_campaign(gamemode) ? 50 : GAME(enemyspawndelay))*i;
        }
    }

    void setuptriggers(bool update)
    {
        loopi(TRIGGERIDS+1) triggers[i].reset(i);
        if(update)
        {
            loopv(sents) if(sents[i].type == TRIGGER && sents[i].attrs[4] >= 2 && sents[i].attrs[0] >= 0 && sents[i].attrs[0] <= TRIGGERIDS+1 && m_check(sents[i].attrs[5], sents[i].attrs[6], gamemode, mutators))
                triggers[sents[i].attrs[0]].ents.add(i);
        }
        else triggerid = 0;

        if(triggerid <= 0)
        {
            static vector<int> valid; valid.setsize(0);
            loopi(TRIGGERIDS) if(!triggers[i+1].ents.empty()) valid.add(triggers[i+1].id);
            if(!valid.empty()) triggerid = valid[rnd(valid.length())];
        }

        if(triggerid > 0) loopi(TRIGGERIDS) if(triggers[i+1].id != triggerid) loopvk(triggers[i+1].ents)
        {
            bool spawn = sents[triggers[i+1].ents[k]].attrs[4]%2;
            if(spawn != sents[triggers[i+1].ents[k]].spawned)
            {
                sents[triggers[i+1].ents[k]].spawned = spawn;
                sents[triggers[i+1].ents[k]].millis = gamemillis;
            }
            sendf(-1, 1, "ri3", N_TRIGGER, triggers[i+1].ents[k], 1+(spawn ? 2 : 1));
            loopvj(sents[triggers[i+1].ents[k]].kin) if(sents.inrange(sents[triggers[i+1].ents[k]].kin[j]))
            {
                sents[sents[triggers[i+1].ents[k]].kin[j]].spawned = sents[triggers[i+1].ents[k]].spawned;
                sents[sents[triggers[i+1].ents[k]].kin[j]].millis = sents[triggers[i+1].ents[k]].millis;
            }
        }
    }

    struct spawn
    {
        int spawncycle;
        vector<int> ents;
        vector<int> cycle;

        spawn() { reset(); }
        ~spawn() {}

        void reset()
        {
            ents.shrink(0);
            cycle.shrink(0);
            spawncycle = 0;
        }
        void add(int n)
        {
            ents.add(n);
            cycle.add(0);
        }
    } spawns[TEAM_ALL];

    void setupspawns(bool update, int players = 0)
    {
        nplayers = totalspawns = 0;
        loopi(TEAM_ALL) spawns[i].reset();
        if(update)
        {
            int numt = numteams(gamemode, mutators), cplayers = 0;
            if(m_fight(gamemode) && m_team(gamemode, mutators))
            {
                loopk(3)
                {
                    loopv(sents) if(sents[i].type == PLAYERSTART && (sents[i].attrs[5] == triggerid || !sents[i].attrs[5]) && m_check(sents[i].attrs[3], sents[i].attrs[4], gamemode, mutators))
                    {
                        if(!k && !isteam(gamemode, mutators, sents[i].attrs[0], TEAM_FIRST)) continue;
                        else if(k == 1 && sents[i].attrs[0] == TEAM_NEUTRAL) continue;
                        else if(k == 2 && sents[i].attrs[0] != TEAM_NEUTRAL) continue;
                        spawns[!k && m_team(gamemode, mutators) ? sents[i].attrs[0] : TEAM_NEUTRAL].add(i);
                        totalspawns++;
                    }
                    if(!k && m_team(gamemode, mutators))
                    {
                        loopi(numt) if(spawns[i+TEAM_FIRST].ents.empty())
                        {
                            loopj(TEAM_ALL) spawns[j].reset();
                            totalspawns = 0;
                            break;
                        }
                    }
                    if(totalspawns) break;
                }
            }
            else
            { // use all neutral spawns
                loopv(sents) if(sents[i].type == PLAYERSTART && sents[i].attrs[0] == TEAM_NEUTRAL && (sents[i].attrs[5] == triggerid || !sents[i].attrs[5]) && m_check(sents[i].attrs[3], sents[i].attrs[4], gamemode, mutators))
                {
                    spawns[TEAM_NEUTRAL].add(i);
                    totalspawns++;
                }
            }
            if(!totalspawns)
            { // use all spawns
                loopv(sents) if(sents[i].type == PLAYERSTART && (sents[i].attrs[5] == triggerid || !sents[i].attrs[5]) && m_check(sents[i].attrs[3], sents[i].attrs[4], gamemode, mutators))
                {
                    spawns[TEAM_NEUTRAL].add(i);
                    totalspawns++;
                }
            }

            if(totalspawns) cplayers = totalspawns/2;
            else
            { // we can cheat and use weapons for spawns
                loopv(sents) if(sents[i].type == WEAPON)
                {
                    spawns[TEAM_NEUTRAL].add(i);
                    totalspawns++;
                }
                cplayers = totalspawns/3;
            }
            nplayers = players > 0 ? players : cplayers;
            if(m_fight(gamemode) && m_team(gamemode, mutators))
            {
                int offt = nplayers%numt;
                if(offt) nplayers += numt-offt;
            }
        }
    }

    int pickspawn(clientinfo *ci)
    {
        if(ci->state.aitype >= AI_START) return ci->state.aientity;
        else
        {
            if(m_checkpoint(gamemode) && !ci->state.cpnodes.empty())
            {
                int checkpoint = ci->state.cpnodes.last();
                if(sents.inrange(checkpoint)) return checkpoint;
            }
            if(totalspawns && GAME(spawnrotate))
            {
                int cycle = -1, team = m_fight(gamemode) && m_team(gamemode, mutators) && !spawns[ci->team].ents.empty() ? ci->team : TEAM_NEUTRAL;
                if(!spawns[team].ents.empty()) switch(GAME(spawnrotate))
                {
                    case 2:
                    {
                        static vector<int> lowest; lowest.setsize(0);
                        loopv(spawns[team].cycle) if(lowest.empty() || spawns[team].cycle[i] <= spawns[team].cycle[lowest[0]])
                        {
                            if(!lowest.empty() && spawns[team].cycle[i] < spawns[team].cycle[lowest[0]]) lowest.setsize(0);
                            lowest.add(i);
                        }
                        if(!lowest.empty())
                        {
                            int r = lowest.length() > 1 ? rnd(lowest.length()) : 0, n = lowest[r];
                            spawns[team].cycle[n]++;
                            spawns[team].spawncycle = cycle = n;
                            break;
                        }
                        // fall through if this fails..
                    }
                    case 1: default:
                    {
                        if(++spawns[team].spawncycle >= spawns[team].ents.length()) spawns[team].spawncycle = 0;
                        cycle = spawns[team].spawncycle;
                        break;
                    }
                }
                if(spawns[team].ents.inrange(cycle)) return spawns[team].ents[cycle];
            }
        }
        return -1;
    }

    void setupgameinfo(int np)
    {
        setuptriggers(true);
        setupitems(true);
        setupspawns(true, m_trial(gamemode) ? 0 : np);
        hasgameinfo = true;
        aiman::dorefresh = GAME(airefresh);
    }

    void sendspawn(clientinfo *ci)
    {
        servstate &gs = ci->state;
        int weap = -1, health = 0;
        if(ci->state.aitype >= AI_START)
        {
            bool hasent = sents.inrange(ci->state.aientity) && sents[ci->state.aientity].type == ACTOR;
            weap = hasent && sents[ci->state.aientity].attrs[6] > 0 ? sents[ci->state.aientity].attrs[6]-1 : aistyle[ci->state.aitype].weap;
            if(!m_insta(gamemode, mutators))
            {
                int heal = hasent && sents[ci->state.aientity].attrs[7] > 0 ? sents[ci->state.aientity].attrs[7] : aistyle[ci->state.aitype].health,
                    amt = heal/2+rnd(heal);
                health = GAME(enemystrength) != 1 ? max(int(amt*GAME(enemystrength)), 1) : amt;
            }
            if(!isweap(weap)) weap = rnd(WEAP_MAX-1)+1;
        }
        gs.spawnstate(gamemode, mutators, weap, health);
        int spawn = pickspawn(ci);
        sendf(ci->clientnum, 1, "ri9v", N_SPAWNSTATE, ci->clientnum, spawn, gs.state, gs.points, gs.frags, gs.health, gs.cptime, gs.weapselect, WEAP_MAX, &gs.ammo[0]);
        gs.lastrespawn = gs.lastspawn = gamemillis;
    }

    template<class T>
    void sendstate(servstate &gs, T &p)
    {
        putint(p, gs.state);
        putint(p, gs.points);
        putint(p, gs.frags);
        putint(p, gs.health);
        putint(p, gs.cptime);
        putint(p, gs.weapselect);
        loopi(WEAP_MAX) putint(p, gs.ammo[i]);
    }

    void relayf(int r, const char *s, ...)
    {
        defvformatstring(str, s, s);
#ifdef IRC
        ircoutf(r, "%s", str);
#endif
#ifdef STANDALONE
        string ft;
        filtertext(ft, str);
        logoutf("%s", ft);
#endif
    }

    void ancmsgft(int cn, int snd, int conlevel, const char *s, ...)
    {
        if(cn < 0 || allowbroadcast(cn))
        {
            defvformatstring(str, s, s);
            sendf(cn, 1, "ri3s", N_ANNOUNCE, snd, conlevel, str);
        }
    }

    void srvmsgft(int cn, int conlevel, const char *s, ...)
    {
        if(cn < 0 || allowbroadcast(cn))
        {
            defvformatstring(str, s, s);
            sendf(cn, 1, "ri2s", N_SERVMSG, conlevel, str);
        }
    }

    void srvmsgf(int cn, const char *s, ...)
    {
        if(cn < 0 || allowbroadcast(cn))
        {
            defvformatstring(str, s, s);
            int conlevel = CON_MESG;
            switch(cn)
            {
                case -3: conlevel = CON_CHAT; cn = -1; break;
                case -2: conlevel = CON_EVENT; cn = -1; break;
                default: break;
            }
            sendf(cn, 1, "ri2s", N_SERVMSG, conlevel, str);
        }
    }

    void srvoutf(int r, const char *s, ...)
    {
        defvformatstring(str, s, s);
        srvmsgf(r >= 0 ? -1 : -2, "%s", str);
        relayf(abs(r), "%s", str);
    }

    void listdemos(int cn)
    {
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        putint(p, N_SENDDEMOLIST);
        putint(p, demos.length());
        loopv(demos) sendstring(demos[i].info, p);
        sendpacket(cn, 1, p.finalize());
    }

    void cleardemos(int n)
    {
        if(!n)
        {
            loopv(demos) delete[] demos[i].data;
            demos.shrink(0);
            srvoutf(4, "cleared all demos");
        }
        else if(demos.inrange(n-1))
        {
            delete[] demos[n-1].data;
            demos.remove(n-1);
            srvoutf(4, "cleared demo %d", n);
        }
    }

    void senddemo(int cn, int num)
    {
        if(!num) num = demos.length();
        if(!demos.inrange(num-1)) return;
        demofile &d = demos[num-1];
        sendf(cn, 2, "rim", N_SENDDEMO, d.len, d.data);
    }

    void sendwelcome(clientinfo *ci);
    int welcomepacket(packetbuf &p, clientinfo *ci);

    void enddemoplayback()
    {
        if(!demoplayback) return;
        DELETEP(demoplayback);

        loopv(clients) sendf(clients[i]->clientnum, 1, "ri3", N_DEMOPLAYBACK, 0, clients[i]->clientnum);

        srvoutf(4, "demo playback finished");

        loopv(clients) sendwelcome(clients[i]);
    }

    void setupdemoplayback()
    {
        demoheader hdr;
        string msg;
        msg[0] = '\0';
        defformatstring(file)("%s.dmo", smapname);
        demoplayback = opengzfile(file, "rb");
        if(!demoplayback) formatstring(msg)("could not read demo \"%s\"", file);
        else if(demoplayback->read(&hdr, sizeof(demoheader))!=sizeof(demoheader) || memcmp(hdr.magic, DEMO_MAGIC, sizeof(hdr.magic)))
            formatstring(msg)("\"%s\" is not a demo file", file);
        else
        {
            lilswap(&hdr.version, 2);
            if(hdr.version!=DEMO_VERSION) formatstring(msg)("demo \"%s\" requires %s version of Red Eclipse", file, hdr.version<DEMO_VERSION ? "an older" : "a newer");
            else if(hdr.gamever!=GAMEVERSION) formatstring(msg)("demo \"%s\" requires %s version of Red Eclipse", file, hdr.gamever<GAMEVERSION ? "an older" : "a newer");
        }
        if(msg[0])
        {
            DELETEP(demoplayback);
            srvoutf(4, "%s", msg);
            return;
        }

        srvoutf(4, "playing demo \"%s\"", file);

        sendf(-1, 1, "ri3", N_DEMOPLAYBACK, 1, -1);

        if(demoplayback->read(&nextplayback, sizeof(nextplayback))!=sizeof(nextplayback))
        {
            enddemoplayback();
            return;
        }
        lilswap(&nextplayback, 1);
    }

    void readdemo()
    {
        if(!demoplayback || paused) return;
        while(gamemillis>=nextplayback)
        {
            int chan, len;
            if(demoplayback->read(&chan, sizeof(chan))!=sizeof(chan) ||
                demoplayback->read(&len, sizeof(len))!=sizeof(len))
            {
                enddemoplayback();
                return;
            }
            lilswap(&chan, 1);
            lilswap(&len, 1);
            ENetPacket *packet = enet_packet_create(NULL, len, 0);
            if(!packet || demoplayback->read(packet->data, len)!=len)
            {
                if(packet) enet_packet_destroy(packet);
                enddemoplayback();
                return;
            }
            sendpacket(-1, chan, packet);
            if(!packet->referenceCount) enet_packet_destroy(packet);
            if(!demoplayback) break;
            if(demoplayback->read(&nextplayback, sizeof(nextplayback))!=sizeof(nextplayback))
            {
                enddemoplayback();
                return;
            }
            lilswap(&nextplayback, 1);
        }
    }

    void prunedemos(int extra = 0)
    {
        int n = clamp(demos.length() + extra - maxdemos, 0, demos.length());
        if(n <= 0) return;
        loopi(n) delete[] demos[n].data;
        demos.remove(0, n);
    }

    void adddemo()
    {
        if(!demotmp) return;
        int len = (int)min(demotmp->size(), stream::offset((maxdemosize<<20) + 0x10000));
        demofile &d = demos.add();
        time_t t = time(NULL);
        char *timestr = ctime(&t), *trim = timestr + strlen(timestr);
        while(trim>timestr && iscubespace(*--trim)) *trim = '\0';
        formatstring(d.info)("%s: %s, %s, %.2f%s", timestr, gamename(gamemode, mutators), smapname, len > 1024*1024 ? len/(1024*1024.f) : len/1024.0f, len > 1024*1024 ? "MB" : "kB");
        srvoutf(4, "demo \"%s\" recorded", d.info);
        d.data = new uchar[len];
        d.len = len;
        demotmp->seek(0, SEEK_SET);
        demotmp->read(d.data, len);
        DELETEP(demotmp);
    }

    void enddemorecord()
    {
        if(!demorecord) return;

        DELETEP(demorecord);

        if(!demotmp) return;
        if(!maxdemos || !maxdemosize) { DELETEP(demotmp); return; }

        prunedemos(1);
        adddemo();
    }

    void writedemo(int chan, void *data, int len)
    {
        if(!demorecord) return;
        int stamp[3] = { gamemillis, chan, len };
        lilswap(stamp, 3);
        demorecord->write(stamp, sizeof(stamp));
        demorecord->write(data, len);
        if(demorecord->rawtell() >= (maxdemosize<<20)) enddemorecord();
    }

    void recordpacket(int chan, void *data, int len)
    {
        writedemo(chan, data, len);
    }

    void setupdemorecord()
    {
        if(m_demo(gamemode) || m_edit(gamemode)) return;

        demotmp = opentempfile("demorecord", "w+b");
        stream *f = opengzfile(NULL, "wb", demotmp);
        if(!f) { DELETEP(demotmp); return; }

        srvoutf(4, "recording demo");

        demorecord = f;

        demoheader hdr;
        memcpy(hdr.magic, DEMO_MAGIC, sizeof(hdr.magic));
        hdr.version = DEMO_VERSION;
        hdr.gamever = GAMEVERSION;
        lilswap(&hdr.version, 2);
        demorecord->write(&hdr, sizeof(demoheader));

        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        welcomepacket(p, NULL);
        writedemo(1, p.buf, p.len);
    }

    void endmatch()
    {
        setpause(false);
        if(demorecord) enddemorecord();
        if(GAME(resetmmonend) >= 2) { mastermode = MM_OPEN; resetallows(); }
        if(GAME(resetvarsonend) >= 2) resetgamevars(true);
        if(GAME(resetbansonend) >= 2) resetbans();
    }

    bool checkvotes(bool force = false)
    {
        shouldcheckvotes = false;

        vector<votecount> votes;
        int maxvotes = 0;
        loopv(clients)
        {
            clientinfo *oi = clients[i];
            if(oi->state.aitype > AI_NONE) continue;
            maxvotes++;
            if(!oi->mapvote[0]) continue;
            votecount *vc = NULL;
            loopvj(votes) if(!strcmp(oi->mapvote, votes[j].map) && oi->modevote == votes[j].mode && oi->mutsvote == votes[j].muts)
            {
                vc = &votes[j];
                break;
            }
            if(!vc) vc = &votes.add(votecount(oi->mapvote, oi->modevote, oi->mutsvote));
            vc->count++;
        }

        votecount *best = NULL;
        int morethanone = 0;
        loopv(votes) if(!best || votes[i].count >= best->count)
        {
            if(best && votes[i].count == best->count) morethanone++;
            else morethanone = 0;
            best = &votes[i];
        }
        bool gotvotes = best && best->count >= min(max(int(maxvotes*GAME(votethreshold)), force ? 1 : 2), force ? 1 : maxvotes);
        if(force && gotvotes && morethanone)
        {
            int r = rnd(morethanone+1), n = 0;
            loopv(votes) if(votes[i].count == best->count)
            {
                if(n != r) n++;
                else { best = &votes[i]; break; }
            }
        }
        if(force || gotvotes)
        {
            endmatch();
            if(gotvotes)
            {
                srvoutf(3, "vote passed: \fs\fy%s\fS on map \fs\fo%s\fS", gamename(best->mode, best->muts), best->map);
                sendf(-1, 1, "risi3", N_MAPCHANGE, best->map, 0, best->mode, best->muts);
                changemap(best->map, best->mode, best->muts);
            }
            else
            {
                int mode = GAME(rotatemode) ? -1 : gamemode, muts = GAME(rotatemuts) ? -1 : mutators;
                changemode(mode, muts);
                const char *map = choosemap(smapname, mode, muts);
                srvoutf(3, "server chooses: \fs\fy%s\fS on map \fs\fo%s\fS", gamename(mode, muts), map);
                sendf(-1, 1, "risi3", N_MAPCHANGE, map, 0, mode, muts);
                changemap(map, mode, muts);
            }
            return true;
        }
        return false;
    }

    bool mutscmp(int req, int limit)
    {
        if(req)
        {
            if(!limit) return false;
            loopi(G_M_NUM) if(req&(1<<i) && !(limit&(1<<i))) return false;
        }
        return true;
    }

    void vote(const char *reqmap, int &reqmode, int &reqmuts, int sender)
    {
        clientinfo *ci = (clientinfo *)getinfo(sender); modecheck(reqmode, reqmuts);
        if(!ci || !m_game(reqmode) || !reqmap || !*reqmap) return;
        if(GAME(modelock) == 5 && GAME(mapslock) == 5 && !haspriv(ci, PRIV_MAX, "vote for a new game")) return;
        else switch(GAME(votelock))
        {
            case 1: case 2: if(!m_edit(reqmode) && !strcmp(reqmap, smapname) && !haspriv(ci, GAME(votelock) == 1 ? PRIV_MASTER : PRIV_ADMIN, "vote for the same map again")) return; break;
            case 3: case 4: if(!haspriv(ci, GAME(votelock) == 3 ? PRIV_MASTER : PRIV_ADMIN, "vote for a new game")) return; break;
            case 5: if(!haspriv(ci, PRIV_MAX, "vote for a new game")) return; break;
        }
        bool hasveto = haspriv(ci, PRIV_MASTER) && (mastermode >= MM_VETO || !numclients(ci->clientnum));
        if(!hasveto)
        {
            if(ci->lastvote && totalmillis-ci->lastvote <= GAME(votewait)) return;
            if(ci->modevote == reqmode && ci->mutsvote == reqmuts && !strcmp(ci->mapvote, reqmap)) return;
        }
        if(m_local(reqmode) && !ci->local)
        {
            srvmsgft(ci->clientnum, CON_EVENT, "\fraccess denied, you must be a local client to start a %s game", gametype[reqmode].name);
            return;
        }
        switch(GAME(modelock))
        {
            case 1: case 2: if(!haspriv(ci, GAME(modelock) == 1 ? PRIV_MASTER : PRIV_ADMIN, "change game modes")) return; break;
            case 3: case 4: if((!((1<<reqmode)&GAME(modelockfilter)) || !mutscmp(reqmuts, GAME(mutslockfilter))) && !haspriv(ci, GAME(modelock) == 3 ? PRIV_MASTER : PRIV_ADMIN, "change to a locked game mode")) return; break;
            case 5: if(!haspriv(ci, PRIV_MAX, "change game modes")) return; break;
            case 0: default: break;
        }
        if(reqmode != G_EDITMODE && GAME(mapslock))
        {
            char *list = NULL;
            switch(GAME(mapslock))
            {
                case 1: case 2:
                {
                    list = newstring(GAME(allowmaps));
                    mapcull(list, reqmode, reqmuts, numclients());
                    break;
                }
                case 3: case 4:
                {
                    maplist(list, reqmode, reqmuts, numclients());
                    break;
                }
                case 5: if(!haspriv(ci, PRIV_MAX, "select a map to play")) return; break;
                case 0: default: break;
            }
            if(list)
            {
                if(listincludes(list, reqmap, strlen(reqmap)) < 0 && !haspriv(ci, GAME(mapslock)%2 ? PRIV_MASTER : PRIV_ADMIN, "select maps not in the rotation"))
                {
                    DELETEA(list);
                    return;
                }
                DELETEA(list);
            }
        }
        copystring(ci->mapvote, reqmap);
        ci->modevote = reqmode;
        ci->mutsvote = reqmuts;
        ci->lastvote = totalmillis;
        if(hasveto)
        {
            endmatch();
            srvoutf(3, "%s forced: \fs\fy%s\fS on map \fs\fo%s\fS", colorname(ci), gamename(ci->modevote, ci->mutsvote), ci->mapvote);
            sendf(-1, 1, "risi3", N_MAPCHANGE, ci->mapvote, 0, ci->modevote, ci->mutsvote);
            changemap(ci->mapvote, ci->modevote, ci->mutsvote);
            return;
        }
        sendf(-1, 1, "ri2si2", N_MAPVOTE, ci->clientnum, ci->mapvote, ci->modevote, ci->mutsvote);
        relayf(3, "%s suggests: \fs\fy%s\fS on map \fs\fo%s\fS", colorname(ci), gamename(ci->modevote, ci->mutsvote), ci->mapvote);
        checkvotes();
    }

    savedscore *findscore(clientinfo *ci, bool insert)
    {
        uint ip = getclientip(ci->clientnum);
        if(!ip) return 0;
        if(!insert)
        {
            loopv(clients)
            {
                clientinfo *oi = clients[i];
                if(oi->clientnum != ci->clientnum && getclientip(oi->clientnum) == ip && !strcmp(oi->name, ci->name))
                {
                    oi->state.timeplayed += lastmillis-oi->state.lasttimeplayed;
                    oi->state.lasttimeplayed = lastmillis;
                    static savedscore curscore;
                    curscore.save(oi->state);
                    return &curscore;
                }
            }
        }
        loopv(savedscores)
        {
            savedscore &sc = savedscores[i];
            if(sc.ip == ip && !strcmp(sc.name, ci->name)) return &sc;
        }
        if(!insert) return 0;
        savedscore &sc = savedscores.add();
        sc.ip = ip;
        copystring(sc.name, ci->name);
        return &sc;
    }

    void givepoints(clientinfo *ci, int points, bool team = true)
    {
        ci->state.score += points;
        ci->state.points += points;
        sendf(-1, 1, "ri4", N_POINTS, ci->clientnum, points, ci->state.points);
        if(team && m_scores(gamemode) && m_team(gamemode, mutators))
        {
            score &ts = teamscore(ci->team);
            ts.total += points;
            sendf(-1, 1, "ri3", N_SCORE, ts.team, ts.total);
        }
    }

    void savescore(clientinfo *ci)
    {
        savedscore *sc = findscore(ci, true);
        if(sc) sc->save(ci->state);
    }

    void setteam(clientinfo *ci, int team, bool reset = true, bool info = false)
    {
        if(ci->team != team)
        {
            bool sm = false;
            if(reset) waiting(ci, 0, 1);
            else if(ci->state.state == CS_ALIVE)
            {
                if(smode) smode->leavegame(ci);
                mutate(smuts, mut->leavegame(ci));
                sm = true;
            }
            ci->lastteam = ci->team;
            ci->team = team;
            if(sm)
            {
                if(smode) smode->entergame(ci);
                mutate(smuts, mut->entergame(ci));
            }
            if(ci->state.aitype == AI_NONE) aiman::dorefresh = GAME(airefresh); // get the ai to reorganise
        }
        if(info) sendf(-1, 1, "ri3", N_SETTEAM, ci->clientnum, ci->team);
    }

    struct teamcheck
    {
        int team;
        float score;
        int clients;

        teamcheck() : team(TEAM_NEUTRAL), score(0.f), clients(0) {}
        teamcheck(int n) : team(n), score(0.f), clients(0) {}
        teamcheck(int n, float r) : team(n), score(r), clients(0) {}
        teamcheck(int n, int s) : team(n), score(s), clients(0) {}

        ~teamcheck() {}
    };

    int chooseteam(clientinfo *ci, int suggest = -1)
    {
        if(ci->state.aitype >= AI_START) return TEAM_ENEMY;
        else if(m_fight(gamemode) && m_team(gamemode, mutators) && ci->state.state != CS_SPECTATOR && ci->state.state != CS_EDITING)
        {
            int team = -1, balance = GAME(teambalance), teams[3][3] = {
                { suggest, ci->team, -1 },
                { suggest, ci->team, ci->lastteam },
                { suggest, ci->lastteam, ci->team }
            };
            loopi(3) if(isteam(gamemode, mutators, teams[GAME(teampersist)][i], TEAM_FIRST))
            {
                team = teams[GAME(teampersist)][i];
                if(GAME(teampersist) == 2) return team;
                break;
            }
            if(balance < 3 && ci->state.aitype > AI_NONE) balance = 1;
            if(balance || team < 0)
            {
                teamcheck teamchecks[TEAM_TOTAL];
                loopk(TEAM_TOTAL) teamchecks[k].team = k+1;
                loopv(clients)
                {
                    clientinfo *cp = clients[i];
                    if(!cp->team || cp == ci || cp->state.state == CS_SPECTATOR || cp->state.state == CS_EDITING) continue;
                    if((cp->state.aitype > AI_NONE && cp->state.ownernum < 0) || cp->state.aitype >= AI_START) continue;
                    if(ci->state.aitype > AI_NONE || (ci->state.aitype == AI_NONE && cp->state.aitype == AI_NONE))
                    { // remember: ai just balance teams
                        cp->state.timeplayed += lastmillis-cp->state.lasttimeplayed;
                        cp->state.lasttimeplayed = lastmillis;
                        teamcheck &ts = teamchecks[cp->team-TEAM_FIRST];
                        ts.score += cp->state.score/float(max(cp->state.timeplayed, 1));
                        ts.clients++;
                    }
                }
                teamcheck *worst = &teamchecks[0];
                if(balance != 3 || ci->state.aitype > AI_NONE)
                {
                    loopi(numteams(gamemode, mutators))
                    {
                        teamcheck &ts = teamchecks[i];
                        switch(balance)
                        {
                            case 2:
                            {
                                if(ts.score < worst->score || (ts.score == worst->score && ts.clients < worst->clients))
                                    worst = &ts;
                                break;
                            }
                            case 3:
                            {
                                if(!i)
                                {
                                    worst = &teamchecks[1];
                                    break; // don't use team alpha for bots in this case
                                }
                            } // fall through
                            case 1: default:
                            {
                                if(ts.clients < worst->clients || (ts.clients == worst->clients && ts.score < worst->score))
                                    worst = &ts;
                                break;
                            }
                        }
                    }
                }
                team = worst->team;
            }
            return team;
        }
        return TEAM_NEUTRAL;
    }

    void stopdemo()
    {
        if(m_demo(gamemode)) enddemoplayback();
        else enddemorecord();
    }

    void setspawn(int ent, bool spawned, bool clear = false, bool msg = false)
    {
        if(sents.inrange(ent))
        {
            if(clear) loopvk(clients) clients[k]->state.dropped.removeall(ent);
            sents[ent].spawned = spawned;
            sents[ent].millis = sents[ent].last = gamemillis;
            if(sents[ent].type == WEAPON && !(sents[ent].attrs[1]&WEAP_F_FORCED))
                sents[ent].millis += w_spawn(w_attr(gamemode, sents[ent].attrs[0], m_weapon(gamemode, mutators)));
            else sents[ent].millis += GAME(itemspawntime);
            if(msg) sendf(-1, 1, "ri3", N_ITEMSPAWN, ent, sents[ent].spawned ? 1 : 0);
        }
    }

    void takeammo(clientinfo *ci, int weap, int amt = 1) { ci->state.ammo[weap] = max(ci->state.ammo[weap]-amt, 0); }

    struct droplist { int weap, ent, value; };
    void dropitems(clientinfo *ci, int level = 2, int weap = -1)
    {
        servstate &ts = ci->state;
        vector<droplist> drop;
        int sweap = m_weapon(gamemode, mutators);
        if(level == 3 || (level == 2 && GAME(kamikaze) && (GAME(kamikaze) > 2 || (ts.hasweap(WEAP_GRENADE, sweap) && (GAME(kamikaze) > 1 || ts.weapselect == WEAP_GRENADE)))))
        {
            ci->state.weapshots[WEAP_GRENADE][0].add(1);
            droplist &d = drop.add();
            d.weap = WEAP_GRENADE;
            d.ent = d.value = -1;
            takeammo(ci, WEAP_GRENADE, WEAP2(WEAP_GRENADE, sub, false));
        }
        if(level)
        {
            loopi(WEAP_MAX) if(i != sweap && ts.hasweap(i, sweap) && sents.inrange(ts.entid[i]))
            {
                if(isweap(weap) && i != weap) continue;
                setspawn(ts.entid[i], false);
                droplist &d = drop.add();
                d.weap = i;
                d.ent = ts.entid[i];
                d.value = ts.ammo[i];
                ts.dropped.add(d.ent, d.value);
                ts.entid[i] = ts.ammo[i] = -1;
                ts.setweapstate(i, WEAP_S_JAM, GAME(weaponjamtime), gamemillis);
            }
        }
        if(level && !drop.empty())
            sendf(-1, 1, "ri3iv", N_DROP, ci->clientnum, -1, drop.length(), drop.length()*sizeof(droplist)/sizeof(int), drop.getbuf());
    }

    void connected(clientinfo *ci);

    #include "auth.h"

    void spectator(clientinfo *ci, int sender = -1)
    {
        if(!ci || ci->state.aitype > AI_NONE) return;
        ci->state.state = CS_SPECTATOR;
        sendf(sender, 1, "ri3", N_SPECTATOR, ci->clientnum, 1);
        setteam(ci, TEAM_NEUTRAL, false, true);
    }

    bool allowstate(clientinfo *ci, int n)
    {
        if(!ci) return false;
        bool isai = ci->state.aitype >= AI_BOT;
        switch(n)
        {
            case 0: if(ci->state.state == CS_SPECTATOR || gamemode >= G_EDITMODE) return false; // first spawn, falls through
            case 1: // try spawn
            {
                if(!isai && ((ci->wantsmap && !ci->failedmap) || (mastermode >= MM_LOCKED && ci->state.state == CS_SPECTATOR))) return false;
                if(ci->state.state == CS_ALIVE || ci->state.state == CS_WAITING) return false;
                if(ci->state.lastdeath && gamemillis-ci->state.lastdeath <= DEATHMILLIS) return false;
                break;
            }
            case 2: // spawn
            {
                if(ci->state.state != CS_DEAD && ci->state.state != CS_WAITING) return false;
                if(ci->state.lastdeath && gamemillis-ci->state.lastdeath <= DEATHMILLIS) return false;
                break;
            }
            case 3: return !isai; // spec
            case 5: if(ci->state.state != CS_EDITING) return false;
            case 4: // edit on/off
            {
                if(isai || !m_edit(gamemode) || (mastermode >= MM_LOCKED && ci->state.state == CS_SPECTATOR)) return false;
                break;
            }
            default: break;
        }
        return true;
    }

    #include "capturemode.h"
    #include "defendmode.h"
    #include "bombermode.h"
    #include "duelmut.h"
    #include "aiman.h"

    bool firstblood = false;
    void changemap(const char *name, int mode, int muts)
    {
        hasgameinfo = maprequest = mapsending = shouldcheckvotes = firstblood = false;
        stopdemo();
        changemode(gamemode = mode, mutators = muts);
        nplayers = gamemillis = interm = 0;
        oldtimelimit = GAME(timelimit);
        timeremaining = GAME(timelimit) ? GAME(timelimit)*60 : -1;
        gamelimit = GAME(timelimit) ? timeremaining*1000 : 0;
        inovertime = false;
        sents.shrink(0);
        scores.shrink(0);
        loopv(savedscores) savedscores[i].mapchange();
        setuptriggers(false);
        setupspawns(false);
        if(smode) smode->reset(false);
        mutate(smuts, mut->reset(false));
        aiman::clearai();
        aiman::dorefresh = GAME(airefresh);

        const char *reqmap = name && *name ? name : pickmap(smapname, gamemode, mutators);
#ifdef STANDALONE // interferes with savemap on clients, in which case we can just use the auto-request
        loopi(3)
        {
            if(mapdata[i]) DELETEP(mapdata[i]);
            const char *reqext = "xxx";
            switch(i)
            {
                case 2: reqext = "cfg"; break;
                case 1: reqext = "png"; break;
                default: case 0: reqext = "mpz"; break;
            }
            defformatstring(reqfile)(strstr(reqmap, "maps/")==reqmap || strstr(reqmap, "maps\\")==reqmap ? "%s" : "maps/%s", reqmap);
            defformatstring(reqfext)("%s.%s", reqfile, reqext);
            if(!(mapdata[i] = openfile(reqfext, "rb")) && !i)
            {
                loopk(3) if(mapdata[k]) DELETEP(mapdata[k]);
                break;
            }
        }
#else
        loopi(3) if(mapdata[i]) DELETEP(mapdata[i]);
#endif
        copystring(smapname, reqmap);

        // server modes
        if(m_capture(gamemode)) smode = &capturemode;
        else if(m_defend(gamemode)) smode = &defendmode;
        else if(m_bomber(gamemode)) smode = &bombermode;
        else smode = NULL;
        smuts.shrink(0);
        smuts.add(&spawnmutator);
        if(m_duke(gamemode, mutators)) smuts.add(&duelmutator);
        if(m_vampire(gamemode, mutators)) smuts.add(&vampiremutator);
        if(smode) smode->reset(false);
        mutate(smuts, mut->reset(false));

        if(m_local(gamemode)) kicknonlocalclients(DISC_PRIVATE);

        loopv(clients) clients[i]->mapchange(true);
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            if(allowstate(ci, 0))
            {
                ci->state.state = CS_DEAD;
                waiting(ci, 2, 1);
            }
            else spectator(ci);
        }

        if(m_fight(gamemode) && numclients()) sendf(-1, 1, "ri2", N_TICK, timeremaining);
        if(m_demo(gamemode))
        {
            if(clients.length()) setupdemoplayback();
        }
        else if(demonextmatch)
        {
            demonextmatch = false;
            setupdemorecord();
        }
    }

    struct crcinfo
    {
        int crc, matches;

        crcinfo() {}
        crcinfo(int crc, int matches) : crc(crc), matches(matches) {}

        static bool compare(const crcinfo &x, const crcinfo &y)
        {
            return x.matches > y.matches;
        }
    };

    void checkmaps(int req = -1)
    {
        if(m_edit(gamemode)) return;
        vector<crcinfo> crcs;
        int total = 0, unsent = 0, invalid = 0;
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            if(ci->state.state==CS_SPECTATOR || ci->state.aitype > AI_NONE) continue;
            total++;
            if(!ci->clientmap[0])
            {
                if(ci->mapcrc < 0) invalid++;
                else if(!ci->mapcrc) unsent++;
            }
            else
            {
                crcinfo *match = NULL;
                loopvj(crcs) if(crcs[j].crc == ci->mapcrc) { match = &crcs[j]; break; }
                if(!match) crcs.add(crcinfo(ci->mapcrc, 1));
                else match->matches++;
            }
        }
        if(total - unsent < min(total, 4)) return;
        crcs.sort(crcinfo::compare);
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            if(ci->state.state==CS_SPECTATOR || ci->state.aitype > AI_NONE || ci->clientmap[0] || ci->mapcrc >= 0 || (req < 0 && ci->warned)) continue;
            srvmsgf(req, "\fy\fs%s\fS has modified map \"%s\"", colorname(ci), smapname);
            if(req < 0) ci->warned = true;
        }
        if(crcs.empty() || crcs.length() < 2) return;
        loopv(crcs)
        {
            crcinfo &info = crcs[i];
            if(i || info.matches <= crcs[i+1].matches) loopvj(clients)
            {
                clientinfo *ci = clients[j];
                if(ci->state.state==CS_SPECTATOR || ci->state.aitype > AI_NONE || !ci->clientmap[0] || ci->mapcrc != info.crc || (req < 0 && ci->warned)) continue;
                srvmsgf(req, "\fy\fs%s\fS has modified map \"%s\"", colorname(ci), smapname);
                if(req < 0) ci->warned = true;
            }
        }
    }

    void checkvar(ident *id, const char *arg)
    {
        if(id && id->flags&IDF_SERVER && !(id->flags&IDF_ADMIN)) switch(id->type)
        {
            case ID_VAR:
            {
                int ret = parseint(arg);
                if(*id->storage.i == id->def.i) { if(ret != id->def.i) numgamemods++; }
                else if(ret == id->def.i) numgamemods--;
                break;
            }
            case ID_FVAR:
            {
                int ret = parsefloat(arg);
                if(*id->storage.f == id->def.f) { if(ret != id->def.f) numgamemods++; }
                else if(ret == id->def.f) numgamemods--;
                break;
            }
            case ID_SVAR:
            {
                if(!strcmp(*id->storage.s, id->def.s)) { if(strcmp(arg, id->def.s)) numgamemods++; }
                else if(!strcmp(arg, id->def.s)) numgamemods--;
                break;
            }
            default: break;
        }
    }

    bool servcmd(int nargs, const char *cmd, const char *arg)
    { // incoming command from scripts
#ifndef STANDALONE
        if(::connected(false, false)) return false;
#endif
        ident *id = idents.access(cmd);
        if(id && id->flags&IDF_SERVER)
        {
            static string scmdval; scmdval[0] = 0;
            switch(id->type)
            {
                case ID_COMMAND:
                {
                    string s;
                    if(nargs <= 1 || !arg) formatstring(s)("%s", cmd);
                    else formatstring(s)("%s %s", cmd, arg);
                    char *ret = executestr(s);
                    if(ret)
                    {
                        if(*ret) conoutft(CON_MESG, "\fc%s returned %s", cmd, ret);
                        delete[] ret;
                    }
                    return true;
                }
                case ID_VAR:
                {
                    if(nargs <= 1 || !arg)
                    {
                        conoutft(CON_MESG, id->flags&IDF_HEX && *id->storage.i >= 0 ? (id->maxval==0xFFFFFF ? "\fc%s = 0x%.6X" : "\fc%s = 0x%X") : "\fc%s = %d", cmd, *id->storage.i);
                        return true;
                    }
                    if(id->maxval < id->minval)
                    {
                        conoutft(CON_MESG, "\frcannot override variable: %s", cmd);
                        return true;
                    }
                    int ret = parseint(arg);
                    if(ret < id->minval || ret > id->maxval)
                    {
                        conoutft(CON_MESG,
                            id->flags&IDF_HEX ?
                                    (id->minval <= 255 ? "\frvalid range for %s is %d..0x%X" : "\frvalid range for %s is 0x%X..0x%X") :
                                    "\frvalid range for %s is %d..%d", cmd, id->minval, id->maxval);
                        return true;
                    }
                    checkvar(id, arg);
                    *id->storage.i = ret;
                    id->changed();
                    formatstring(scmdval)(id->flags&IDF_HEX && *id->storage.i >= 0 ? (id->maxval==0xFFFFFF ? "0x%.6X" : "0x%X") : "%d", *id->storage.i);
                    break;
                }
                case ID_FVAR:
                {
                    if(nargs <= 1 || !arg)
                    {
                        conoutft(CON_MESG, "\fc%s = %s", cmd, floatstr(*id->storage.f));
                        return true;
                    }
                    float ret = parsefloat(arg);
                    if(ret < id->minvalf || ret > id->maxvalf)
                    {
                        conoutft(CON_MESG, "\frvalid range for %s is %s..%s", cmd, floatstr(id->minvalf), floatstr(id->maxvalf));
                        return true;
                    }
                    checkvar(id, arg);
                    *id->storage.f = ret;
                    id->changed();
                    formatstring(scmdval)("%s", floatstr(*id->storage.f));
                    break;
                }
                case ID_SVAR:
                {
                    if(nargs <= 1 || !arg)
                    {
                        conoutft(CON_MESG, strchr(*id->storage.s, '"') ? "\fc%s = [%s]" : "\fc%s = \"%s\"", cmd, *id->storage.s);
                        return true;
                    }
                    checkvar(id, arg);
                    delete[] *id->storage.s;
                    *id->storage.s = newstring(arg);
                    id->changed();
                    formatstring(scmdval)("%s", *id->storage.s);
                    break;
                }
                default: return false;
            }
            sendf(-1, 1, "ri2ss", N_COMMAND, -1, &id->name[3], scmdval);
            arg = scmdval;
            return true;
        }
        return false; // parse will spit out "unknown command" in this case
    }

    void parsecommand(clientinfo *ci, int nargs, const char *cmd, const char *arg)
    { // incoming commands from clients
        defformatstring(cmdname)("sv_%s", cmd);
        ident *id = idents.access(cmdname);
        if(id && id->flags&IDF_SERVER)
        {
            mkstring(val);
            int locked = max(id->flags&IDF_ADMIN ? 2 : 0, GAME(varslock));
            switch(id->type)
            {
                case ID_COMMAND:
                {
                    if(locked && !haspriv(ci, locked >= 3 ? PRIV_MAX : (locked >= 2 ? PRIV_ADMIN : PRIV_MASTER), "execute commands")) return;
                    string s;
                    if(nargs <= 1 || !arg) formatstring(s)("sv_%s", cmd);
                    else formatstring(s)("sv_%s %s", cmd, arg);
                    char *ret = executestr(s);
                    if(ret && *ret) srvoutf(-3, "\fc%s executed %s (returned: %s)", colorname(ci), cmd, ret);
                    else srvoutf(-3, "\fc%s executed %s", colorname(ci), cmd);
                    if(ret) delete[] ret;
                    return;
                }
                case ID_VAR:
                {
                    if(nargs <= 1 || !arg)
                    {
                        srvmsgf(ci->clientnum, id->flags&IDF_HEX && *id->storage.i >= 0 ? (id->maxval==0xFFFFFF ? "\fc%s = 0x%.6X" : "\fc%s = 0x%X") : "\fc%s = %d", cmd, *id->storage.i);
                        return;
                    }
                    else if(locked && !haspriv(ci, locked >= 3 ? PRIV_MAX : (locked >= 2 ? PRIV_ADMIN : PRIV_MASTER), "change variables"))
                    {
                        formatstring(val)(id->flags&IDF_HEX && *id->storage.i >= 0 ? (id->maxval==0xFFFFFF ? "0x%.6X" : "0x%X") : "%d", *id->storage.i);
                        sendf(ci->clientnum, 1, "ri2ss", N_COMMAND, -1, &id->name[3], val);
                        return;
                    }
                    if(id->maxval < id->minval)
                    {
                        srvmsgf(ci->clientnum, "\frcannot override variable: %s", cmd);
                        return;
                    }
                    int ret = parseint(arg);
                    if(ret < id->minval || ret > id->maxval)
                    {
                        srvmsgf(ci->clientnum,
                            id->flags&IDF_HEX ?
                                (id->minval <= 255 ? "\frvalid range for %s is %d..0x%X" : "\frvalid range for %s is 0x%X..0x%X") :
                                "\frvalid range for %s is %d..%d", cmd, id->minval, id->maxval);
                        return;
                    }
                    checkvar(id, arg);
                    *id->storage.i = ret;
                    id->changed();
                    formatstring(val)(id->flags&IDF_HEX && *id->storage.i >= 0 ? (id->maxval==0xFFFFFF ? "0x%.6X" : "0x%X") : "%d", *id->storage.i);
                    break;
                }
                case ID_FVAR:
                {
                    if(nargs <= 1 || !arg)
                    {
                        srvmsgf(ci->clientnum, "\fc%s = %s", cmd, floatstr(*id->storage.f));
                        return;
                    }
                    else if(locked && !haspriv(ci, locked >= 3 ? PRIV_MAX : (locked >= 2 ? PRIV_ADMIN : PRIV_MASTER), "change variables"))
                    {
                        formatstring(val)("%s", floatstr(*id->storage.f));
                        sendf(ci->clientnum, 1, "ri2ss", N_COMMAND, -1, &id->name[3], val);
                        return;
                    }
                    float ret = parsefloat(arg);
                    if(ret < id->minvalf || ret > id->maxvalf)
                    {
                        srvmsgf(ci->clientnum, "\frvalid range for %s is %s..%s", cmd, floatstr(id->minvalf), floatstr(id->maxvalf));
                        return;
                    }
                    checkvar(id, arg);
                    *id->storage.f = ret;
                    id->changed();
                    formatstring(val)("%s", floatstr(*id->storage.f));
                    break;
                }
                case ID_SVAR:
                {
                    if(nargs <= 1 || !arg)
                    {
                        srvmsgf(ci->clientnum, strchr(*id->storage.s, '"') ? "\fc%s = [%s]" : "\fc%s = \"%s\"", cmd, *id->storage.s);
                        return;
                    }
                    else if(locked && !haspriv(ci, locked >= 3 ? PRIV_MAX : (locked >= 2 ? PRIV_ADMIN : PRIV_MASTER), "change variables"))
                    {
                        formatstring(val)("%s", *id->storage.s);
                        sendf(ci->clientnum, 1, "ri2ss", N_COMMAND, -1, &id->name[3], val);
                        return;
                    }
                    checkvar(id, arg);
                    delete[] *id->storage.s;
                    *id->storage.s = newstring(arg);
                    id->changed();
                    formatstring(val)("%s", *id->storage.s);
                    break;
                }
                default: return;
            }
            sendf(-1, 1, "ri2ss", N_COMMAND, ci->clientnum, &id->name[3], val);
            relayf(3, "\fc%s set %s to %s", colorname(ci), &id->name[3], val);
        }
        else srvmsgf(ci->clientnum, "\frunknown command: %s", cmd);
    }

    bool rewritecommand(ident *id, tagval *args, int numargs)
    {
        bool found = false;
        const char *argstr = numargs > 2 ? conc(&args[1], numargs-1, true) : (numargs > 1 ? args[1].getstr() : "");
        if(id && id->flags&IDF_SERVER && id->type!=ID_COMMAND) found = servcmd(numargs, args[0].s, argstr);
#ifndef STANDALONE
        else if(!id || id->flags&IDF_CLIENT) found = client::sendcmd(numargs, args[0].s, argstr);
#endif
        if(numargs > 2) delete[] (char *)argstr;
        return found;
    }

    clientinfo *choosebestclient()
    {
        clientinfo *best = NULL;
        loopv(clients)
        {
            clientinfo *cs = clients[i];
            if(cs->state.aitype > AI_NONE || !cs->name[0] || !cs->online || cs->wantsmap) continue;
            if(!best || cs->state.timeplayed > best->state.timeplayed) best = cs;
        }
        return best;
    }

    void sendservinit(clientinfo *ci)
    {
        sendf(ci->clientnum, 1, "ri5", N_SERVERINIT, ci->clientnum, GAMEVERSION, ci->sessionid, serverpass[0] ? 1 : 0);
    }

    bool restorescore(clientinfo *ci)
    {
        savedscore *sc = findscore(ci, false);
        if(sc)
        {
            sc->restore(ci->state);
            return true;
        }
        return false;
    }

    void sendresume(clientinfo *ci)
    {
        servstate &gs = ci->state;
        sendf(-1, 1, "ri8vi", N_RESUME, ci->clientnum, gs.state, gs.points, gs.frags, gs.health, gs.cptime, gs.weapselect, WEAP_MAX, &gs.ammo[0], -1);
    }

    void putinitclient(clientinfo *ci, packetbuf &p)
    {
        if(ci->state.aitype > AI_NONE)
        {
            if(ci->state.ownernum >= 0)
            {
                putint(p, N_INITAI);
                putint(p, ci->clientnum);
                putint(p, ci->state.ownernum);
                putint(p, ci->state.aitype);
                putint(p, ci->state.aientity);
                putint(p, ci->state.skill);
                sendstring(ci->name, p);
                putint(p, ci->team);
                putint(p, ci->state.colour);
                putint(p, ci->state.model);
            }
        }
        else
        {
            putint(p, N_CLIENTINIT);
            putint(p, ci->clientnum);
            sendstring(ci->name, p);
            putint(p, ci->state.colour);
            putint(p, ci->state.model);
            putint(p, ci->team);
        }
    }

    void sendinitclient(clientinfo *ci)
    {
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        putinitclient(ci, p);
        sendpacket(-1, 1, p.finalize(), ci->clientnum);
    }

    void welcomeinitclient(packetbuf &p, int exclude = -1)
    {
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            if(!ci->connected || ci->clientnum == exclude) continue;

            putinitclient(ci, p);
        }
    }

    void sendwelcome(clientinfo *ci)
    {
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        int chan = welcomepacket(p, ci);
        sendpacket(ci->clientnum, chan, p.finalize());
    }

    int welcomepacket(packetbuf &p, clientinfo *ci)
    {
        putint(p, N_WELCOME);
        putint(p, N_MAPCHANGE);
        sendstring(smapname, p);
        if(!ci) putint(p, 0);
        else if(!ci->online && m_edit(gamemode) && numclients(ci->clientnum))
        {
            loopi(3) if(mapdata[i]) DELETEP(mapdata[i]);
            ci->wantsmap = true;
            if(!mapsending)
            {
                clientinfo *best = choosebestclient();
                if(best)
                {
                    srvmsgft(ci->clientnum, CON_EVENT, "map is being requested, please wait..");
                    sendf(best->clientnum, 1, "ri", N_GETMAP);
                    mapsending = true;
                }
                else
                {
                    sendf(-1, 1, "ri", N_FAILMAP);
                    loopv(clients)
                    {
                        clientinfo *ci = clients[i];
                        ci->failedmap = true;
                    }
                }
            }
            putint(p, 1); // already in progress
        }
        else
        {
            ci->wantsmap = false;
            if(ci->online) putint(p, 2); // we got a temp map eh?
            else putint(p, ci->local ? -1 : 0);
        }
        putint(p, gamemode);
        putint(p, mutators);

        enumerate(idents, ident, id, {
            if(id.flags&IDF_SERVER) // reset vars
            {
                mkstring(val);
                switch(id.type)
                {
                    case ID_VAR:
                    {
                        formatstring(val)(id.flags&IDF_HEX && *id.storage.i >= 0 ? (id.maxval==0xFFFFFF ? "0x%.6X" : "0x%X") : "%d", *id.storage.i);
                        break;
                    }
                    case ID_FVAR:
                    {
                        formatstring(val)("%s", floatstr(*id.storage.f));
                        break;
                    }
                    case ID_SVAR:
                    {
                        formatstring(val)("%s", *id.storage.s);
                        break;
                    }
                    default: break;
                }
                putint(p, N_COMMAND);
                putint(p, -1);
                sendstring(&id.name[3], p);
                sendstring(val, p);
            }
        });

        if(!ci || (m_fight(gamemode) && numclients()))
        {
            putint(p, N_TICK);
            putint(p, timeremaining);
        }

        if(hasgameinfo)
        {
            putint(p, N_GAMEINFO);
            loopv(sents) if(enttype[sents[i].type].resyncs)
            {
                putint(p, i);
                if(enttype[sents[i].type].usetype == EU_ITEM) putint(p, finditem(i) ? 1 : 0);
                else putint(p, sents[i].spawned ? 1 : 0);
            }
            putint(p, -1);
        }

        if(ci)
        {
            ci->state.state = CS_SPECTATOR;
            ci->team = TEAM_NEUTRAL;
            putint(p, N_SPECTATOR);
            putint(p, ci->clientnum);
            putint(p, 1);
            sendf(-1, 1, "ri3x", N_SPECTATOR, ci->clientnum, 1, ci->clientnum);
            putint(p, N_SETTEAM);
            putint(p, ci->clientnum);
            putint(p, ci->team);
        }
        if(!ci || clients.length() > 1)
        {
            putint(p, N_RESUME);
            loopv(clients)
            {
                clientinfo *oi = clients[i];
                if(ci && oi->clientnum == ci->clientnum) continue;
                putint(p, oi->clientnum);
                sendstate(oi->state, p);
            }
            putint(p, -1);
            welcomeinitclient(p, ci ? ci->clientnum : -1);
            loopv(clients)
            {
                clientinfo *oi = clients[i];
                if(oi->state.aitype > AI_NONE || (ci && oi->clientnum == ci->clientnum)) continue;
                if(oi->mapvote[0])
                {
                    putint(p, N_MAPVOTE);
                    putint(p, oi->clientnum);
                    sendstring(oi->mapvote, p);
                    putint(p, oi->modevote);
                    putint(p, oi->mutsvote);
                }
            }
        }

        if(*GAME(servermotd))
        {
            putint(p, N_ANNOUNCE);
            putint(p, S_GUIACT);
            putint(p, CON_CHAT);
            sendstring(GAME(servermotd), p);
        }

        if(m_team(gamemode, mutators)) loopv(scores)
        {
            score &cs = scores[i];
            putint(p, N_SCORE);
            putint(p, cs.team);
            putint(p, cs.total);
        }

        if(smode) smode->initclient(ci, p, true);
        mutate(smuts, mut->initclient(ci, p, true));
        if(ci) ci->online = true;
        aiman::dorefresh = GAME(airefresh);
        return 1;
    }

    void clearevent(clientinfo *ci) { delete ci->events.remove(0); }

    void addhistory(clientinfo *target, clientinfo *actor, int millis)
    {
        bool found = false;
        loopv(target->state.damagelog) if (target->state.damagelog[i].clientnum == actor->clientnum)
        {
            target->state.damagelog[i].millis = millis;
            found = true;
            break;
        }
        if(!found) target->state.damagelog.add(dmghist(actor->clientnum, millis));
    }

    void gethistory(clientinfo *target, clientinfo *actor, int millis, vector<int> &log, bool clear = false, int points = 0)
    {
        loopv(target->state.damagelog) if(target->state.damagelog[i].clientnum != actor->clientnum && millis-target->state.damagelog[i].millis <= GAME(assistkilldelay))
        {
            clientinfo *assist = (clientinfo *)getinfo(target->state.damagelog[i].clientnum);
            if(assist)
            {
                log.add(assist->clientnum);
                if(points && !m_duke(gamemode, mutators)) givepoints(assist, points);
            }
        }
        if(clear) target->state.damagelog.shrink(0);
    }

    void dodamage(clientinfo *target, clientinfo *actor, int damage, int weap, int flags, const ivec &hitpush = ivec(0, 0, 0))
    {
        int realdamage = damage, realflags = flags, nodamage = 0, hurt = 0; realflags &= ~HIT_SFLAGS;
        if(smode && !smode->damage(target, actor, realdamage, weap, realflags, hitpush)) { nodamage++; }
        mutate(smuts, if(!mut->damage(target, actor, realdamage, weap, realflags, hitpush)) { nodamage++; });
        if(actor->state.aitype < AI_START)
        {
            if((actor == target && !GAME(selfdamage)) || (m_trial(gamemode) && !GAME(trialdamage))) nodamage++;
            else if(m_play(gamemode) && m_team(gamemode, mutators) && actor->team == target->team && actor != target)
            {
                switch(GAME(teamdamage))
                {
                    case 2: default: break;
                    case 1: if(actor->state.aitype == AI_NONE || target->state.aitype > AI_NONE) break;
                    case 0: nodamage++; break;
                }
            }
            if(m_expert(gamemode, mutators) && !hithead(flags)) nodamage++;
        }
        if(nodamage || !hithurts(realflags))
        {
            realflags &= ~HIT_CLEAR;
            realflags |= HIT_WAVE;
        }
        else
        {
            if(isweap(weap) && GAME(criticalchance) > 0 && WEAP(weap, critmult) > 0)
            {
                bool crdash = WEAP2(weap, critdash, flags&HIT_ALT) && actor->state.lastboost && gamemillis-actor->state.lastboost <= WEAP2(weap, critdash, flags&HIT_ALT);
                actor->state.crits++;
                int offset = GAME(criticalchance)-actor->state.crits;
                if(target != actor)
                {
                    if(crdash && WEAP(weap, critboost) > 0) offset = int(offset*(1.f/WEAP(weap, critboost)));
                    if(WEAP(weap, critdist) != 0)
                    {
                        float dist = actor->state.o.dist(target->state.o);
                        if(WEAP(weap, critdist) < 0 && dist < 0-WEAP(weap, critdist))
                            offset = int(offset*clamp(dist, 1.f, 0-WEAP(weap, critdist))/(0-WEAP(weap, critdist)));
                        else if(WEAP(weap, critdist) > 0 && dist > WEAP(weap, critdist))
                            offset = int(offset*WEAP(weap, critdist)/clamp(dist, WEAP(weap, critdist), 1e16f));
                    }
                }
                if(offset <= 0 || !rnd(offset))
                {
                    realflags |= HIT_CRIT;
                    realdamage = int(realdamage*WEAP(weap, critmult));
                    actor->state.crits = 0;
                }
            }
            hurt = min(target->state.health, realdamage);
            target->state.health -= realdamage;
            if(target->state.health <= m_health(gamemode, mutators)) target->state.lastregen = 0;
            target->state.lastpain = gamemillis;
            actor->state.damage += realdamage;
            if(target->state.health <= 0) realflags |= HIT_KILL;
            if(GAME(burntime) && doesburn(weap, flags))
            {
                target->state.lastburn = target->state.lastburntime = gamemillis;
                target->state.lastburnowner = actor->clientnum;
            }
            else if(GAME(bleedtime) && doesbleed(weap, flags))
            {
                target->state.lastbleed = target->state.lastbleedtime = gamemillis;
                target->state.lastbleedowner = actor->clientnum;
            }
        }
        if(smode) smode->dodamage(target, actor, realdamage, hurt, weap, realflags, hitpush);
        mutate(smuts, mut->dodamage(target, actor, realdamage, hurt, weap, realflags, hitpush));
        if(target != actor && (!m_team(gamemode, mutators) || target->team != actor->team))
            addhistory(target, actor, gamemillis);
        sendf(-1, 1, "ri7i3", N_DAMAGE, target->clientnum, actor->clientnum, weap, realflags, realdamage, target->state.health, hitpush.x, hitpush.y, hitpush.z);
        if(realflags&HIT_KILL)
        {
            bool isai = target->state.aitype >= AI_START && !m_campaign(gamemode);
            int fragvalue = 1;
            if(target != actor && (!m_team(gamemode, mutators) || target->team != actor->team)) actor->state.frags++;
            else fragvalue = -fragvalue;
            int pointvalue = (smode && !isai ? smode->points(target, actor) : fragvalue), style = FRAG_NONE;
            pointvalue *= isai ? GAME(enemybonus) : GAME(fragbonus);
            if(!m_insta(gamemode, mutators) && (realdamage >= (realflags&HIT_EXPLODE ? m_health(gamemode, mutators) : m_health(gamemode, mutators)*3/2)))
                style = FRAG_OBLITERATE;
            target->state.spree = 0;
            if(m_team(gamemode, mutators) && actor->team == target->team)
            {
                actor->state.spree = 0;
                if(actor->state.aitype < AI_START)
                {
                    if(actor != target) actor->state.teamkills++;
                    pointvalue *= GAME(teamkillpenalty);
                }
            }
            else if(actor != target && actor->state.aitype < AI_START)
            {
                if(!firstblood && actor->state.aitype == AI_NONE && (m_campaign(gamemode) ? target->state.aitype >= AI_START : target->state.aitype < AI_START))
                {
                    firstblood = true;
                    style |= FRAG_FIRSTBLOOD;
                    pointvalue += GAME(firstbloodpoints);
                }
                if(flags&HIT_HEAD) // NOT HZONE
                {
                    style |= FRAG_HEADSHOT;
                    pointvalue += GAME(headshotpoints);
                }
                if(m_fight(gamemode) && target->state.aitype < AI_START)
                {
                    int logs = 0;
                    actor->state.spree++;
                    actor->state.fraglog.add(target->clientnum);
                    if(GAME(multikilldelay))
                    {
                        logs = 0;
                        loopv(actor->state.fragmillis)
                        {
                            if(lastmillis-actor->state.fragmillis[i] > GAME(multikilldelay)) actor->state.fragmillis.remove(i--);
                            else logs++;
                        }
                        if(!logs) actor->state.rewards &= ~FRAG_MULTI;
                        actor->state.fragmillis.add(lastmillis); logs++;
                        if(logs >= 2)
                        {
                            int offset = clamp(logs-2, 0, 2), type = 1<<(FRAG_MKILL+offset); // double, triple, multi..
                            if(!(actor->state.rewards&type))
                            {
                                style |= type;
                                actor->state.rewards |= type;
                                pointvalue *= (GAME(multikillpoints) ? offset+1 : 1)*GAME(multikillbonus);
                                //loopv(actor->state.fragmillis) actor->state.fragmillis[i] = lastmillis;
                            }
                        }
                    }
                    if(actor->state.spree <= GAME(spreecount)*FRAG_SPREES && !(actor->state.spree%GAME(spreecount)))
                    {
                        int offset = clamp((actor->state.spree/GAME(spreecount)), 1, int(FRAG_SPREES))-1, type = 1<<(FRAG_SPREE+offset);
                        if(!(actor->state.rewards&type))
                        {
                            style |= type;
                            actor->state.rewards |= type;
                            pointvalue *= (GAME(spreepoints) ? offset+1 : 1)*GAME(spreebonus);
                        }
                    }
                    logs = 0;
                    loopv(target->state.fraglog) if(target->state.fraglog[i] == actor->clientnum) { logs++; target->state.fraglog.remove(i--); }
                    if(logs >= GAME(dominatecount))
                    {
                        style |= FRAG_REVENGE;
                        pointvalue += GAME(revengepoints);
                    }
                    logs = 0;
                    loopv(actor->state.fraglog) if(actor->state.fraglog[i] == target->clientnum) logs++;
                    if(logs == GAME(dominatecount))
                    {
                        style |= FRAG_DOMINATE;
                        pointvalue += GAME(dominatepoints);
                    }
                }
            }
            if(pointvalue && !m_duke(gamemode, mutators))
            {
                if(actor != target && actor->state.aitype >= AI_START && target->state.aitype < AI_START)
                    givepoints(target, -pointvalue);
                else if(actor->state.aitype < AI_START) givepoints(actor, pointvalue);
            }
            target->state.deaths++;
            dropitems(target, aistyle[target->state.aitype].living ? 2 : 3);
            static vector<int> dmglog; dmglog.setsize(0);
            gethistory(target, actor, gamemillis, dmglog, true, 1);
            sendf(-1, 1, "ri9iv", N_DIED, target->clientnum, actor->clientnum, actor->state.frags, actor->state.spree, style, weap, realflags, realdamage, dmglog.length(), dmglog.length(), dmglog.getbuf());
            target->position.setsize(0);
            if(smode) smode->died(target, actor);
            mutate(smuts, mut->died(target, actor));
            target->state.state = CS_DEAD; // don't issue respawn yet until DEATHMILLIS has elapsed
            target->state.lastdeath = gamemillis;
        }
    }

    void suicideevent::process(clientinfo *ci)
    {
        servstate &gs = ci->state;
        if(gs.state != CS_ALIVE) return;
        if(!(flags&HIT_DEATH) && !(flags&HIT_LOST))
        {
            if(smode && !smode->damage(ci, ci, ci->state.health, -1, flags)) { return; }
            mutate(smuts, if(!mut->damage(ci, ci, ci->state.health, -1, flags)) { return; });
        }
        ci->state.spree = 0;
        if(m_trial(gamemode))
        {
            if(!flags || ci->state.cpnodes.length() == 1) // reset if suicided or hasn't reached another checkpoint yet
            {
                ci->state.cpmillis = 0;
                ci->state.cpnodes.shrink(0);
                sendf(-1, 1, "ri4", N_CHECKPOINT, ci->clientnum, -1, 0);
            }
        }
        else if(!m_duke(gamemode, mutators)) givepoints(ci, smode ? smode->points(ci, ci) : -1);
        ci->state.deaths++;
        dropitems(ci, aistyle[ci->state.aitype].living ? 2 : 3);
        if(GAME(burntime) && (flags&HIT_MELT || flags&HIT_BURN))
        {
            ci->state.lastburn = ci->state.lastburntime = gamemillis;
            ci->state.lastburnowner = ci->clientnum;
        }
        static vector<int> dmglog; dmglog.setsize(0);
        gethistory(ci, ci, gamemillis, dmglog, true, 1);
        sendf(-1, 1, "ri9iv", N_DIED, ci->clientnum, ci->clientnum, ci->state.frags, 0, 0, -1, flags, ci->state.health, dmglog.length(), dmglog.length(), dmglog.getbuf());
        ci->position.setsize(0);
        if(smode) smode->died(ci, NULL);
        mutate(smuts, mut->died(ci, NULL));
        gs.state = CS_DEAD;
        gs.lastdeath = gamemillis;
    }

    int calcdamage(clientinfo *actor, clientinfo *target, int weap, int &flags, int radial, float size, float dist, float scale, bool self)
    {
        flags &= ~HIT_SFLAGS;
        if(!hithurts(flags)) flags = HIT_WAVE|(flags&HIT_ALT ? HIT_ALT : 0); // so it impacts, but not hurts

        float skew = 1;
        if(!m_insta(gamemode, mutators))
        {
            skew *= clamp(scale, 0.f, 1.f)*GAME(damagescale);
            if(radial) skew *= clamp(1.f-dist/size, 1e-6f, 1.f);
            else if(WEAP2(weap, taperin, flags&HIT_ALT) > 0 || WEAP2(weap, taperout, flags&HIT_ALT) > 0) skew *= clamp(dist, 0.f, 1.f);
            if(m_capture(gamemode) && GAME(capturebuffdelay))
            {
                if(actor->state.lastbuff) skew *= GAME(capturebuffdamage);
                if(target->state.lastbuff) skew /= GAME(capturebuffshield);
            }
            else if(m_defend(gamemode) && GAME(defendbuffdelay))
            {
                if(actor->state.lastbuff) skew *= GAME(defendbuffdamage);
                if(target->state.lastbuff) skew /= GAME(defendbuffshield);
            }
            else if(m_bomber(gamemode) && GAME(bomberbuffdelay))
            {
                if(actor->state.lastbuff) skew *= GAME(bomberbuffdamage);
                if(target->state.lastbuff) skew /= GAME(bomberbuffshield);
            }
        }
        if(!(flags&HIT_HEAD))
        {
            if(flags&HIT_WHIPLASH) skew *= WEAP2(weap, whipdmg, flags&HIT_ALT);
            else if(flags&HIT_TORSO) skew *= WEAP2(weap, torsodmg, flags&HIT_ALT);
            else if(flags&HIT_LEGS) skew *= WEAP2(weap, legsdmg, flags&HIT_ALT);
            else skew = 0;
        }
        if(self) skew *= WEAP2(weap, selfdmg, flags&HIT_ALT);

        return int(ceilf((flags&HIT_FLAK ? WEAP2(weap, flakdmg, flags&HIT_ALT) : WEAP2(weap, damage, flags&HIT_ALT))*skew));
    }

    void destroyevent::process(clientinfo *ci)
    {
        servstate &gs = ci->state;
        if(weap == -1)
        {
            gs.dropped.remove(id);
            if(sents.inrange(id)) sents[id].millis = gamemillis;
        }
        else if(isweap(weap))
        {
            if(!gs.weapshots[weap][flags&HIT_ALT ? 1 : 0].find(id))
            {
                if(GAME(serverdebug) >= 2) srvmsgf(ci->clientnum, "sync error: destroy [%d (%d)] failed - not found", weap, id);
                return;
            }
            if(hits.empty())
            {
                gs.weapshots[weap][flags&HIT_ALT ? 1 : 0].remove(id);
                if(id >= 0 && !m_insta(gamemode, mutators))
                {
                    int f = WEAP2(weap, flakweap, flags&HIT_ALT);
                    if(f >= 0)
                    {
                        int w = f%WEAP_MAX, r = WEAP2(weap, flakrays, flags&HIT_ALT);
                        loopi(r) gs.weapshots[w][f >= WEAP_MAX ? 1 : 0].add(-id);
                    }
                }
                sendf(-1, 1, "ri4x", N_DESTROY, ci->clientnum, 1, id, ci->clientnum);
            }
            else loopv(hits)
            {
                hitset &h = hits[i];
                clientinfo *target = (clientinfo *)getinfo(h.target);
                if(!target)
                {
                    if(GAME(serverdebug) >= 2) srvmsgf(ci->clientnum, "sync error: destroy [%d (%d)] failed - hit %d [%d] not found", weap, id, i, h.target);
                    continue;
                }
                if(h.proj)
                {
                    servstate &ts = target->state;
                    loopj(WEAP_MAX) loopk(2) if(ts.weapshots[j][k].find(h.proj))
                    {
                        ts.weapshots[j][k].remove(h.proj);
                        sendf(target->clientnum, 1, "ri4", N_DESTROY, target->clientnum, 1, h.proj);
                        break;
                    }
                }
                else
                {
                    int hflags = flags|h.flags;
                    float skew = float(scale)/DNF;
                    if(radial) radial = clamp(radial, 1, WEAPEX(weap, flags&HIT_ALT, gamemode, mutators, skew));
                    float size = radial ? (hflags&HIT_WAVE ? radial*WEAP(weap, pusharea) : radial) : 0.f, dist = float(h.dist)/DNF;
                    if(target->state.state == CS_ALIVE && !target->state.protect(gamemillis, m_protect(gamemode, mutators)))
                    {
                        int damage = calcdamage(ci, target, weap, hflags, radial, size, dist, skew, ci == target);
                        if(damage) dodamage(target, ci, damage, weap, hflags, h.dir);
                        else if(GAME(serverdebug) >= 2)
                            srvmsgf(ci->clientnum, "sync error: destroy [%d (%d)] failed - hit %d [%d] determined zero damage", weap, id, i, h.target);
                    }
                    else if(GAME(serverdebug) >= 2)
                        srvmsgf(ci->clientnum, "sync error: destroy [%d (%d)] failed - hit %d [%d] state disallows it", weap, id, i, h.target);
                }
            }
        }
    }

    bool weaponjam(clientinfo *ci, int millis, int weap, int load)
    {
        if(isweap(weap) && GAME(weaponjamming) && WEAP(weap, jamchance))
        {
            int sweap = m_weapon(gamemode, mutators);
            if(sweap >= 0)
            {
                ci->state.weapjams[weap]++;
                int offset = WEAP(weap, jamchance)-ci->state.weapjams[weap];
                if(offset <= 0 || !rnd(offset))
                {
                    ci->state.weapjams[weap] = ci->state.weapload[weap] = 0;
                    dropitems(ci, GAME(weaponjamming) >= 3 ? 3 : 1, weap == sweap || GAME(weaponjamming)%2 == 0 ? -1 : weap);
                    return true;
                }
            }
        }
        return false;
    }

    void shotevent::process(clientinfo *ci)
    {
        servstate &gs = ci->state;
        if(!gs.isalive(gamemillis) || !isweap(weap))
        {
            if(GAME(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: shoot [%d] failed - unexpected message", weap);
            return;
        }
        int sub = WEAP2(weap, sub, flags&HIT_ALT);
        if(sub > 1 && WEAP2(weap, power, flags&HIT_ALT))
        {
            if(ci->state.ammo[weap] < sub)
            {
                int maxscale = int(ci->state.ammo[weap]/float(sub)*WEAP2(weap, power, flags&HIT_ALT));
                if(scale > maxscale) scale = maxscale;
            }
            sub = int(ceilf(sub*scale/float(WEAP2(weap, power, flags&HIT_ALT))));
        }
        if(!gs.canshoot(weap, flags, m_weapon(gamemode, mutators), millis))
        {
            if(!gs.canshoot(weap, flags, m_weapon(gamemode, mutators), millis, (1<<WEAP_S_RELOAD)))
            {
                if(sub && WEAP(weap, max)) ci->state.ammo[weap] = max(ci->state.ammo[weap]-sub, 0);
                if(!gs.hasweap(weap, m_weapon(gamemode, mutators))) gs.entid[weap] = -1; // its gone..
                if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: shoot [%d] failed - current state disallows it", weap);
                return;
            }
            else if(gs.weapload[gs.weapselect] > 0)
            {
                takeammo(ci, gs.weapselect, gs.weapload[gs.weapselect]);
                gs.weapload[gs.weapselect] = -gs.weapload[gs.weapselect];
                if(weaponjam(ci, millis, gs.weapselect, gs.weapload[gs.weapselect]) && gs.weapselect != m_weapon(gamemode, mutators)) return;
                sendf(-1, 1, "ri5", N_RELOAD, ci->clientnum, gs.weapselect, gs.weapload[gs.weapselect], gs.ammo[gs.weapselect]);
            }
            else return;
        }
        takeammo(ci, weap, sub);
        gs.setweapstate(weap, flags&HIT_ALT ? WEAP_S_SECONDARY : WEAP_S_PRIMARY, WEAP2(weap, adelay, flags&HIT_ALT), millis);
        sendf(-1, 1, "ri8ivx", N_SHOTFX, ci->clientnum, weap, flags, scale, from.x, from.y, from.z, shots.length(), shots.length()*sizeof(shotmsg)/sizeof(int), shots.getbuf(), ci->clientnum);
        gs.weapshot[weap] = sub;
        gs.shotdamage += WEAP2(weap, damage, flags&HIT_ALT)*shots.length();
        loopv(shots) gs.weapshots[weap][flags&HIT_ALT ? 1 : 0].add(shots[i].id);
        if(!gs.hasweap(weap, m_weapon(gamemode, mutators)))
        {
            //if(sents.inrange(gs.entid[weap])) setspawn(gs.entid[weap], false);
            gs.entid[weap] = -1; // its gone..
        }
    }

    void switchevent::process(clientinfo *ci)
    {
        servstate &gs = ci->state;
        if(!gs.isalive(gamemillis) || !isweap(weap))
        {
            if(GAME(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: switch [%d] failed - unexpected message", weap);
            sendf(ci->clientnum, 1, "ri3", N_WEAPSELECT, ci->clientnum, gs.weapselect);
            return;
        }
        if(!gs.canswitch(weap, m_weapon(gamemode, mutators), millis, (1<<WEAP_S_SWITCH)))
        {
            if(!gs.canswitch(weap, m_weapon(gamemode, mutators), millis, (1<<WEAP_S_RELOAD)))
            {
                if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: switch [%d] failed - current state disallows it", weap);
                sendf(ci->clientnum, 1, "ri3", N_WEAPSELECT, ci->clientnum, gs.weapselect);
                return;
            }
            else if(gs.weapload[gs.weapselect] > 0)
            {
                takeammo(ci, gs.weapselect, gs.weapload[gs.weapselect]);
                gs.weapload[gs.weapselect] = -gs.weapload[gs.weapselect];
                if(weaponjam(ci, millis, gs.weapselect, gs.weapload[gs.weapselect]) && gs.weapselect != m_weapon(gamemode, mutators))
                {
                    sendf(ci->clientnum, 1, "ri3", N_WEAPSELECT, ci->clientnum, gs.weapselect);
                    return;
                }
                sendf(-1, 1, "ri5", N_RELOAD, ci->clientnum, gs.weapselect, gs.weapload[gs.weapselect], gs.ammo[gs.weapselect]);
            }
            else return;
        }
        gs.weapswitch(weap, millis, GAME(weaponswitchdelay));
        sendf(-1, 1, "ri3x", N_WEAPSELECT, ci->clientnum, weap, ci->clientnum);
    }

    void dropevent::process(clientinfo *ci)
    {
        servstate &gs = ci->state;
        if(!gs.isalive(gamemillis) || !isweap(weap))
        {
            if(GAME(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: drop [%d] failed - unexpected message", weap);
            return;
        }
        int sweap = m_weapon(gamemode, mutators);
        if(!gs.hasweap(weap, sweap))
        {
            if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: drop [%d] failed - current state disallows it", weap);
            return;
        }
        if(!gs.weapwaited(weap, millis, (1<<WEAP_S_SWITCH)))
        {
            if(!gs.weapwaited(weap, millis, (1<<WEAP_S_RELOAD)))
            {
                if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: drop [%d] failed - current state disallows it", weap);
                return;
            }
            else if(gs.weapload[weap] > 0)
            {
                takeammo(ci, weap, gs.weapload[weap]);
                gs.weapload[weap] = -gs.weapload[weap];
            }
            else return;
        }
        int dropped = -1, value = -1, nweap = gs.bestweap(sweap, true); // switch to best weapon
        if(sents.inrange(gs.entid[weap]))
        {
            dropped = gs.entid[weap];
            value = gs.ammo[weap] ? gs.ammo[weap] : WEAP(weap, max);
            setspawn(dropped, false);
            gs.dropped.add(dropped, value);
        }
        gs.ammo[weap] = gs.entid[weap] = -1;
        gs.weapswitch(nweap, millis, GAME(weaponswitchdelay));
        sendf(-1, 1, "ri7", N_DROP, ci->clientnum, nweap, 1, weap, dropped, value);
    }

    void reloadevent::process(clientinfo *ci)
    {
        servstate &gs = ci->state;
        if(!gs.isalive(gamemillis) || !isweap(weap))
        {
            if(GAME(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: reload [%d] failed - unexpected message", weap);
            sendf(ci->clientnum, 1, "ri5", N_RELOAD, ci->clientnum, weap, gs.weapload[weap], gs.ammo[weap]);
            return;
        }
        if(!gs.canreload(weap, m_weapon(gamemode, mutators), millis))
        {
            if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: reload [%d] failed - current state disallows it", weap);
            sendf(ci->clientnum, 1, "ri5", N_RELOAD, ci->clientnum, weap, gs.weapload[weap], gs.ammo[weap]);
            return;
        }
        gs.setweapstate(weap, WEAP_S_RELOAD, WEAP(weap, rdelay), millis);
        int oldammo = gs.ammo[weap];
        gs.ammo[weap] = min(max(gs.ammo[weap], 0) + WEAP(weap, add), WEAP(weap, max));
        gs.weapload[weap] = gs.ammo[weap]-oldammo;
        sendf(-1, 1, "ri5x", N_RELOAD, ci->clientnum, weap, gs.weapload[weap], gs.ammo[weap], ci->clientnum);
    }

    void useevent::process(clientinfo *ci)
    {
        servstate &gs = ci->state;
        if(gs.state != CS_ALIVE || !sents.inrange(ent) || enttype[sents[ent].type].usetype != EU_ITEM)
        {
            if(GAME(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: use [%d] failed - unexpected message", ent);
            return;
        }
        int sweap = m_weapon(gamemode, mutators), attr = sents[ent].type == WEAPON ? w_attr(gamemode, sents[ent].attrs[0], sweap) : sents[ent].attrs[0];
        if(!finditem(ent))
        {
            if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: use [%d] failed - doesn't seem to be spawned anywhere", ent);
            return;
        }
        if(!gs.canuse(sents[ent].type, attr, sents[ent].attrs, sweap, millis, (1<<WEAP_S_SWITCH)))
        {
            if(!gs.canuse(sents[ent].type, attr, sents[ent].attrs, sweap, millis, (1<<WEAP_S_RELOAD)))
            {
                if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: use [%d] failed - current state disallows it", ent);
                return;
            }
            else if(gs.weapload[gs.weapselect] > 0)
            {
                takeammo(ci, gs.weapselect, gs.weapload[gs.weapselect]);
                gs.weapload[gs.weapselect] = -gs.weapload[gs.weapselect];
                if(weaponjam(ci, millis, gs.weapselect, gs.weapload[gs.weapselect]) && gs.weapselect != sweap) return;
                sendf(-1, 1, "ri5", N_RELOAD, ci->clientnum, gs.weapselect, gs.weapload[gs.weapselect], gs.ammo[gs.weapselect]);
            }
            else return;
        }
        int weap = -1, amt = -1, dropped = -1, value = -1;
        if(sents[ent].type == WEAPON)
        {
            if(!gs.hasweap(attr, sweap) && w_carry(attr, sweap) && gs.carry(sweap) >= GAME(maxcarry))
                weap = gs.drop(sweap);
            loopvk(clients) if(clients[k]->state.dropped.find(ent))
            {
                amt = clients[k]->state.dropped.value(ent);
                break;
            }
            if(isweap(weap))
            {
                if(sents.inrange(gs.entid[weap]))
                {
                    dropped = gs.entid[weap];
                    value = gs.ammo[weap];
                    setspawn(dropped, false);
                    gs.setweapstate(weap, WEAP_S_SWITCH, GAME(weaponswitchdelay), millis);
                    gs.dropped.add(dropped, value);
                }
                gs.ammo[weap] = gs.entid[weap] = -1;
            }
        }
        setspawn(ent, false, true);
        gs.useitem(ent, sents[ent].type, attr, amt, sweap, millis, GAME(weaponswitchdelay));
        if(sents[ent].type == WEAPON && isweap(attr)) gs.weapjams[attr] = 0;
        sendf(-1, 1, "ri8", N_ITEMACC, ci->clientnum, ent, amt, sents[ent].spawned ? 1 : 0, weap, dropped, value);
    }

    bool gameevent::flush(clientinfo *ci, int fmillis)
    {
        process(ci);
        return true;
    }

    bool timedevent::flush(clientinfo *ci, int fmillis)
    {
        if(millis > fmillis) return false;
        else if(millis >= ci->lastevent)
        {
            ci->lastevent = millis;
            process(ci);
        }
        return true;
    }

    void flushevents(clientinfo *ci, int millis)
    {
        while(ci->events.length())
        {
            gameevent *ev = ci->events[0];
            if(ev->flush(ci, millis)) clearevent(ci);
            else break;
        }
    }

    void processevents()
    {
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            flushevents(ci, gamemillis);
        }
    }

    void cleartimedevents(clientinfo *ci)
    {
        int keep = 0;
        loopv(ci->events)
        {
            if(ci->events[i]->keepable())
            {
                if(keep < i)
                {
                    for(int j = keep; j < i; j++) delete ci->events[j];
                    ci->events.remove(keep, i - keep);
                    i = keep;
                }
                keep = i+1;
                continue;
            }
        }
        while(ci->events.length() > keep) delete ci->events.pop();
    }

    void waiting(clientinfo *ci, int doteam, int drop, bool exclude)
    {
        if(m_campaign(gamemode) && ci->state.cpnodes.empty())
        {
            int maxnodes = -1;
            loopv(clients)
            {
                clientinfo *oi = clients[i];
                if(oi->clientnum >= 0 && oi->name[0] && oi->state.aitype < AI_START && (!clients.inrange(maxnodes) || oi->state.cpnodes.length() > clients[maxnodes]->state.cpnodes.length()))
                    maxnodes = i;
            }
            if(clients.inrange(maxnodes)) loopv(clients[maxnodes]->state.cpnodes) ci->state.cpnodes.add(clients[maxnodes]->state.cpnodes[i]);
        }
        if(ci->state.state == CS_ALIVE)
        {
            dropitems(ci, drop);
            if(smode) smode->died(ci);
            mutate(smuts, mut->died(ci));
            ci->state.lastdeath = gamemillis;
        }
        else if(!doteam && drop%2 == 1) ci->state.lastdeath = gamemillis;
        if(exclude) sendf(-1, 1, "ri2x", N_WAITING, ci->clientnum, ci->clientnum);
        else sendf(-1, 1, "ri2", N_WAITING, ci->clientnum);
        ci->state.state = CS_WAITING;
        ci->state.weapreset(false);
        if(m_arena(gamemode, mutators)) chkloadweap(ci);
        if(doteam && (doteam == 2 || !isteam(gamemode, mutators, ci->team, TEAM_FIRST)))
            setteam(ci, chooseteam(ci), false, true);
    }

    int triggertime(int i)
    {
        if(sents.inrange(i)) switch(sents[i].type)
        {
            case TRIGGER: case MAPMODEL: case PARTICLES: case MAPSOUND: case TELEPORT: case PUSHER: return 1000; break;
            default: break;
        }
        return 0;
    }

    void checkents()
    {
        bool thresh = m_fight(gamemode) && !m_noitems(gamemode, mutators) && !m_special(gamemode, mutators) && GAME(itemthreshold) > 0;
        int items[MAXENTTYPES], lowest[MAXENTTYPES], sweap = m_weapon(gamemode, mutators), players = 0;
        memset(items, 0, sizeof(items)); memset(lowest, -1, sizeof(lowest));
        if(thresh)
        {
            loopv(clients) if(clients[i]->clientnum >= 0 && clients[i]->online && clients[i]->state.state == CS_ALIVE && clients[i]->state.aitype < AI_START)
                players++;
            loopv(sents) if(enttype[sents[i].type].usetype == EU_ITEM && hasitem(i))
            {
                if(sents[i].type == WEAPON)
                {
                    int attr = w_attr(gamemode, sents[i].attrs[0], sweap);
                    if(attr < WEAP_OFFSET || attr >= WEAP_ITEM) continue;
                }
                if(finditem(i, true, true)) items[sents[i].type]++;
                else if(!sents.inrange(lowest[sents[i].type]) || sents[i].millis < sents[lowest[sents[i].type]].millis)
                    lowest[sents[i].type] = i;
            }
        }
        loopv(sents) switch(sents[i].type)
        {
            case TRIGGER:
            {
                if(sents[i].attrs[1] == TR_LINK && sents[i].spawned && gamemillis >= sents[i].millis && (sents[i].attrs[4] == triggerid || !sents[i].attrs[4]))
                {
                    sents[i].spawned = false;
                    sents[i].millis = gamemillis+(triggertime(i)*2);
                    sendf(-1, 1, "ri3", N_TRIGGER, i, 0);
                    loopvj(sents[i].kin) if(sents.inrange(sents[i].kin[j]))
                    {
                        sents[sents[i].kin[j]].spawned = sents[i].spawned;
                        sents[sents[i].kin[j]].millis = sents[i].millis;
                    }
                }
                break;
            }
            default:
            {
                bool allowed = hasitem(i);
                if(enttype[sents[i].type].usetype == EU_ITEM && (allowed || sents[i].spawned))
                {
                    bool found = finditem(i, true, true);
                    if(allowed && thresh && i == lowest[sents[i].type] && (gamemillis-sents[lowest[sents[i].type]].last > GAME(itemspawndelay)))
                    {
                        float dist = items[sents[i].type]/float(players*GAME(maxcarry));
                        if(dist < GAME(itemthreshold)) found = false;
                    }
                    if((!found && !sents[i].spawned) || (!allowed && sents[i].spawned))
                    {
                        setspawn(i, allowed, true, true);
                        items[sents[i].type]++;
                    }
                }
                break;
            }
        }
    }

    void spectate(clientinfo *ci, bool val)
    {
        if(ci->state.state != CS_SPECTATOR && val)
        {
            if(ci->state.state == CS_ALIVE)
            {
                suicideevent ev;
                ev.flags = HIT_SPEC;
                ev.process(ci);
            }
            if(smode) smode->leavegame(ci);
            mutate(smuts, mut->leavegame(ci));
            sendf(-1, 1, "ri3", N_SPECTATOR, ci->clientnum, val);
            ci->state.cpnodes.shrink(0);
            ci->state.cpmillis = 0;
            ci->state.state = CS_SPECTATOR;
            ci->state.timeplayed += lastmillis-ci->state.lasttimeplayed;
            setteam(ci, TEAM_NEUTRAL, false, true);
            aiman::dorefresh = GAME(airefresh);
        }
        else if(ci->state.state == CS_SPECTATOR && !val)
        {
            ci->state.cpnodes.shrink(0);
            ci->state.cpmillis = 0;
            ci->state.state = CS_DEAD;
            ci->state.lasttimeplayed = lastmillis;
            waiting(ci, 2, 1);
            if(smode) smode->entergame(ci);
            mutate(smuts, mut->entergame(ci));
            aiman::dorefresh = GAME(airefresh);
            if(ci->clientmap[0] || ci->mapcrc) checkmaps();
        }
    }

    void checkclients()
    {
        loopv(clients) if(clients[i]->name[0] && clients[i]->online)
        {
            clientinfo *ci = clients[i];
            if(smode) smode->checkclient(ci);
            mutate(smuts, mut->checkclient(ci));
            if(ci->state.state == CS_ALIVE)
            {
                if(ci->state.burning(gamemillis, GAME(burntime)))
                {
                    if(gamemillis-ci->state.lastburntime >= GAME(burndelay))
                    {
                        clientinfo *co = (clientinfo *)getinfo(ci->state.lastburnowner);
                        dodamage(ci, co ? co : ci, GAME(burndamage), -1, HIT_BURN);
                        ci->state.lastburntime += GAME(burndelay);
                    }
                }
                else if(ci->state.lastburn) ci->state.lastburn = ci->state.lastburntime = 0;
                if(ci->state.bleeding(gamemillis, GAME(bleedtime)))
                {
                    if(gamemillis-ci->state.lastbleedtime >= GAME(bleeddelay))
                    {
                        clientinfo *co = (clientinfo *)getinfo(ci->state.lastbleedowner);
                        dodamage(ci, co ? co : ci, GAME(bleeddamage), -1, HIT_BLEED);
                        ci->state.lastbleedtime += GAME(bleeddelay);
                    }
                }
                else if(ci->state.lastbleed) ci->state.lastbleed = ci->state.lastbleedtime = 0;
                if(m_regen(gamemode, mutators) && ci->state.aitype < AI_START)
                {
                    int total = m_health(gamemode, mutators), amt = GAME(regenhealth),
                        delay = ci->state.lastregen ? GAME(regentime) : GAME(regendelay);
                    if(smode) smode->regen(ci, total, amt, delay);
                    if(delay && ci->state.health != total)
                    {
                        int millis = gamemillis-(ci->state.lastregen ? ci->state.lastregen : ci->state.lastpain);
                        if(millis >= delay)
                        {
                            int low = 0;
                            if(ci->state.health > total)
                            {
                                amt = -GAME(regendecay);
                                total = ci->state.health;
                                low = m_health(gamemode, mutators);
                            }
                            int rgn = ci->state.health, heal = clamp(ci->state.health+amt, low, total), eff = heal-rgn;
                            if(eff)
                            {
                                ci->state.health = heal;
                                ci->state.lastregen = gamemillis;
                                sendf(-1, 1, "ri4", N_REGEN, ci->clientnum, ci->state.health, eff);
                            }
                        }
                    }
                }
            }
            else if(ci->state.state == CS_WAITING)
            {
                int nospawn = 0;
                if(smode && !smode->canspawn(ci, false)) { nospawn++; }
                mutate(smuts, if(!mut->canspawn(ci, false)) { nospawn++; });
                if(!nospawn)
                {
                    if(ci->state.lastdeath) flushevents(ci, ci->state.lastdeath + DEATHMILLIS);
                    cleartimedevents(ci);
                    ci->state.state = CS_DEAD; // safety
                    ci->state.respawn(gamemillis, m_health(gamemode, mutators));
                    sendspawn(ci);
                }
            }
            if(GAME(autospectate) && ci->state.state == CS_DEAD && ci->state.lastdeath && gamemillis-ci->state.lastdeath >= GAME(autospecdelay))
                spectate(ci, true);
        }
    }

    void serverupdate()
    {
        loopv(connects) if(totalmillis-connects[i]->connectmillis > 15000) disconnect_client(connects[i]->clientnum, DISC_TIMEOUT);
        loopvrev(bans) if(bans[i].type == ipinfo::TEMPORARY && totalmillis-bans[i].time > 4*60*60000) bans.remove(i);

        if(numclients())
        {
            if(!paused) gamemillis += curtime;
            if(m_demo(gamemode)) readdemo();
            else if(!paused && !interm)
            {
                processevents();
                checkents();
                checklimits();
                checkclients();
                if(smode) smode->update();
                mutate(smuts, mut->update());
            }

            if(masterupdate)
            {
                loopv(clients) sendf(-1, 1, "ri3", N_CURRENTMASTER, clients[i]->clientnum, clients[i]->privilege);
                masterupdate = false;
            }

            if(interm && totalmillis - interm >= 0) // wait then call for next map
            {
                if(GAME(votelimit) && !maprequest && GAME(votelock) != 5 && (GAME(modelock) != 5 || GAME(mapslock) != 5))
                { // if they can't vote, no point in waiting for them to do so
                    if(demorecord) enddemorecord();
                    sendf(-1, 1, "ri", N_NEWGAME);
                    maprequest = true;
                    interm = totalmillis+GAME(votelimit);
                    if(!interm) interm = 1;
                }
                else
                {
                    interm = 0;
                    checkvotes(true);
                }
            }
            if(shouldcheckvotes) checkvotes();
        }
        aiman::checkai();
        auth::update();
    }

    int clientconnect(int n, uint ip, bool local)
    {
        clientinfo *ci = (clientinfo *)getinfo(n);
        ci->clientnum = n;
        ci->connectmillis = totalmillis;
        ci->sessionid = (rnd(0x1000000)*((totalmillis%10000)+1))&0xFFFFFF;
        ci->local = local;
        connects.add(ci);
        if(!local)
        {
            if(m_local(gamemode) || servertype <= 0) return DISC_PRIVATE;
#ifndef STANDALONE
            bool haslocal = false;
            loopv(clients) if(clients[i]->local) { haslocal = true; break; }
            if(!haslocal) return DISC_PRIVATE;
#endif
        }
        sendservinit(ci);
        return DISC_NONE;
    }

    void clientdisconnect(int n, bool local, int reason)
    {
        clientinfo *ci = (clientinfo *)getinfo(n);
        bool complete = !numclients(n);
        if(local)
        {
            if(m_demo(gamemode)) enddemoplayback();
        }
        if(ci->connected)
        {
            if(reason != DISC_SHUTDOWN)
            {
                loopv(clients) if(clients[i] != ci)
                {
                    loopvk(clients[i]->state.fraglog) if(clients[i]->state.fraglog[k] == ci->clientnum)
                        clients[i]->state.fraglog.remove(k--);
                }
                if(ci->state.state == CS_ALIVE) dropitems(ci, 0);
                if(ci->privilege) auth::setmaster(ci, false);
                if(smode) smode->leavegame(ci, true);
                mutate(smuts, mut->leavegame(ci, true));
                ci->state.timeplayed += lastmillis-ci->state.lasttimeplayed;
                savescore(ci);
                aiman::removeai(ci, complete);
                if(!complete) aiman::dorefresh = GAME(airefresh);
            }
            sendf(-1, 1, "ri3", N_DISCONNECT, n, reason);
            ci->connected = false;
            if(ci->name[0])
            {
                int amt = numclients(ci->clientnum);
                relayf(2, "\fo%s (%s) has left the game (%s, %d %s)", colorname(ci), gethostname(n), reason >= 0 ? disc_reasons[reason] : "normal", amt, amt != 1 ? "players" : "player");
            }
            clients.removeobj(ci);
        }
        else connects.removeobj(ci);
        if(complete) cleanup();
        else shouldcheckvotes = true;
    }

    #include "extinfo.h"
    void queryreply(ucharbuf &req, ucharbuf &p)
    {
        if(!getint(req))
        {
            extqueryreply(req, p);
            return;
        }
        putint(p, numclients());
        putint(p, 8);                   // number of attrs following
        putint(p, GAMEVERSION);         // 1
        putint(p, gamemode);            // 2
        putint(p, mutators);            // 3
        putint(p, timeremaining);       // 4
        putint(p, GAME(serverclients)); // 5
        bool priv = m_local(gamemode);
#ifndef STANDALONE
        if(!priv)
        {
            bool haslocal = false;
            loopv(clients) if(clients[i]->local) { haslocal = true; break; }
            if(!haslocal) priv = true;
        }
#endif
        putint(p, serverpass[0] ? MM_PASSWORD : (priv ? MM_PRIVATE : mastermode)); // 6
        putint(p, numgamevars); // 7
        putint(p, numgamemods); // 8
        sendstring(smapname, p);
        if(*GAME(serverdesc)) sendstring(GAME(serverdesc), p);
        else
        {
            #ifdef STANDALONE
            sendstring("unnamed", p);
            #else
            const char *cname = client::getname();
            if(!cname || !cname[0]) cname = "unnamed";
            sendstring(cname, p);
            #endif
        }
        loopv(clients) if(clients[i]->clientnum >= 0 && clients[i]->name[0] && clients[i]->state.aitype == AI_NONE)
            sendstring(colorname(clients[i]), p);
        sendqueryreply(p);
    }

    const char *tempmapfile[3] = { "mapdata", "mapshot", "mapconf" };
    bool receivefile(int sender, uchar *data, int len)
    {
        clientinfo *ci = (clientinfo *)getinfo(sender);
        ucharbuf p(data, len);
        int type = getint(p), n = 0;
        data += p.length();
        len -= p.length();
        switch(type)
        {
            case N_SENDMAPFILE: case N_SENDMAPSHOT: case N_SENDMAPCONFIG:
                n = type-N_SENDMAPFILE;
                break;
            default: srvmsgf(sender, "bad map file type %d"); return false;
        }
        if(mapdata[n])
        {
            if(ci != choosebestclient())
            {
                srvmsgf(sender, "sorry, the map isn't needed from you");
                return false;
            }
            DELETEP(mapdata[n]);
        }
        if(!len)
        {
            srvmsgf(sender, "you sent a zero length packet for map data");
            return false;
        }
        mapdata[n] = opentempfile(tempmapfile[n], "w+b");
        if(!mapdata[n])
        {
            srvmsgf(sender, "failed to open temporary file for map");
            return false;
        }
        mapdata[n]->write(data, len);
        return n == 2;
    }

    int checktype(int type, clientinfo *ci)
    {
        if(ci)
        {
            if(!ci->connected) return type == (ci->connectauth ? N_AUTHANS : N_CONNECT) || type == N_PING ? type : -1;
            if(ci->local) return type;
        }
        // only allow edit messages in coop-edit mode
        if(type >= N_EDITENT && type <= N_NEWMAP && (!m_edit(gamemode) || !ci || ci->state.state == CS_SPECTATOR)) return -1;
        // server only messages
        static const int servtypes[] = { N_SERVERINIT, N_CLIENTINIT, N_WELCOME, N_NEWGAME, N_MAPCHANGE, N_SERVMSG, N_DAMAGE, N_SHOTFX, N_DIED, N_POINTS, N_SPAWNSTATE, N_ITEMACC, N_ITEMSPAWN, N_TICK, N_DISCONNECT, N_CURRENTMASTER, N_PONG, N_RESUME, N_SCORE, N_ANNOUNCE, N_SENDDEMOLIST, N_SENDDEMO, N_DEMOPLAYBACK, N_REGEN, N_CLIENT, N_AUTHCHAL };
        if(ci)
        {
            loopi(sizeof(servtypes)/sizeof(int)) if(type == servtypes[i]) return -1;
            if(type < N_EDITENT || type > N_NEWMAP || !m_edit(gamemode) || !ci || ci->state.state != CS_EDITING)
            {
                static const int exempt[] = { N_POS, N_SPAWN, N_DESTROY };
                loopi(sizeof(exempt)/sizeof(int)) if(type == exempt[i]) return type;
                if(++ci->overflow >= 250) return -2;
            }
        }
        return type;
    }

    static void freecallback(ENetPacket *packet)
    {
        extern void cleanworldstate(ENetPacket *packet);
        cleanworldstate(packet);
    }

    void cleanworldstate(ENetPacket *packet)
    {
        loopv(worldstates)
        {
            worldstate *ws = worldstates[i];
            if(ws->positions.inbuf(packet->data) || ws->messages.inbuf(packet->data)) ws->uses--;
            else continue;
            if(!ws->uses)
            {
                delete ws;
                worldstates.remove(i);
            }
            break;
        }
    }

    bool buildworldstate()
    {
        worldstate &ws = *new worldstate;
        loopv(clients)
        {
            clientinfo &ci = *clients[i];
            ci.overflow = 0;
            if(ci.position.empty()) ci.posoff = -1;
            else
            {
                ci.posoff = ws.positions.length();
                ws.positions.put(ci.position.getbuf(), ci.position.length());
                ci.poslen = ws.positions.length() - ci.posoff;
                ci.position.setsize(0);
            }
            if(ci.messages.empty()) ci.msgoff = -1;
            else
            {
                ci.msgoff = ws.messages.length();
                putint(ws.messages, N_CLIENT);
                putint(ws.messages, ci.clientnum);
                putuint(ws.messages, ci.messages.length());
                ws.messages.put(ci.messages.getbuf(), ci.messages.length());
                ci.msglen = ws.messages.length() - ci.msgoff;
                ci.messages.setsize(0);
            }
        }
        int psize = ws.positions.length(), msize = ws.messages.length();
        if(psize)
        {
            recordpacket(0, ws.positions.getbuf(), psize);
            ucharbuf p = ws.positions.reserve(psize);
            p.put(ws.positions.getbuf(), psize);
            ws.positions.addbuf(p);
        }
        if(msize)
        {
            recordpacket(1, ws.messages.getbuf(), msize);
            ucharbuf p = ws.messages.reserve(msize);
            p.put(ws.messages.getbuf(), msize);
            ws.messages.addbuf(p);
        }
        ws.uses = 0;
        loopv(clients)
        {
            clientinfo &ci = *clients[i];
            ENetPacket *packet;
            if(ci.state.aitype == AI_NONE && psize && (ci.posoff<0 || psize-ci.poslen>0))
            {
                packet = enet_packet_create(&ws.positions[ci.posoff<0 ? 0 : ci.posoff+ci.poslen],
                                            ci.posoff<0 ? psize : psize-ci.poslen,
                                            ENET_PACKET_FLAG_NO_ALLOCATE);
                sendpacket(ci.clientnum, 0, packet);
                if(!packet->referenceCount) enet_packet_destroy(packet);
                else { ++ws.uses; packet->freeCallback = freecallback; }
            }

            if(ci.state.aitype == AI_NONE && msize && (ci.msgoff<0 || msize-ci.msglen>0))
            {
                packet = enet_packet_create(&ws.messages[ci.msgoff<0 ? 0 : ci.msgoff+ci.msglen],
                                            ci.msgoff<0 ? msize : msize-ci.msglen,
                                            (reliablemessages ? ENET_PACKET_FLAG_RELIABLE : 0) | ENET_PACKET_FLAG_NO_ALLOCATE);
                sendpacket(ci.clientnum, 1, packet);
                if(!packet->referenceCount) enet_packet_destroy(packet);
                else { ++ws.uses; packet->freeCallback = freecallback; }
            }
        }
        reliablemessages = false;
        if(!ws.uses)
        {
            delete &ws;
            return false;
        }
        else
        {
            worldstates.add(&ws);
            return true;
        }
    }

    bool sendpackets(bool force)
    {
        if(clients.empty()) return false;
        enet_uint32 millis = enet_time_get()-lastsend;
        if(millis<40 && !force) return false;
        bool flush = buildworldstate();
        lastsend += millis - (millis%40);
        return flush;
    }

    void sendclipboard(clientinfo *ci)
    {
        if(!ci->lastclipboard || !ci->clipboard) return;
        bool flushed = false;
        loopv(clients)
        {
            clientinfo &e = *clients[i];
            if(e.clientnum != ci->clientnum && e.needclipboard - ci->lastclipboard >= 0)
            {
                if(!flushed) { flushserver(true); flushed = true; }
                sendpacket(e.clientnum, 1, ci->clipboard);
            }
        }
    }

    void connected(clientinfo *ci)
    {
        connects.removeobj(ci);
        clients.add(ci);

        ci->connected = true;
        ci->needclipboard = totalmillis ? totalmillis : 1;
        masterupdate = true;
        ci->state.lasttimeplayed = lastmillis;

        sendwelcome(ci);
        if(restorescore(ci)) sendresume(ci);
        sendinitclient(ci);
        int amt = numclients();
        relayf(2, "\fg%s (%s) has joined the game (%d %s)", colorname(ci), gethostname(ci->clientnum), amt, amt != 1 ? "players" : "player");

        if(m_demo(gamemode)) setupdemoplayback();
    }

    void parsepacket(int sender, int chan, packetbuf &p)     // has to parse exactly each byte of the packet
    {
        if(sender<0 || p.packet->flags&ENET_PACKET_FLAG_UNSEQUENCED) return;
        char text[MAXTRANS];
        int type = -1, prevtype = -1;
        clientinfo *ci = sender>=0 ? (clientinfo *)getinfo(sender) : NULL;
        if(ci && !ci->connected)
        {
            if(chan==0) return;
            else if(chan!=1) { disconnect_client(sender, DISC_TAGT); return; }
            else while(p.length() < p.maxlen) switch(checktype(getint(p), ci))
            {
                case N_CONNECT:
                {
                    getstring(text, p);
                    if(!text[0]) copystring(text, "unnamed");
                    filtertext(text, text, true, true, true, MAXNAMELEN);
                    copystring(ci->name, text, MAXNAMELEN+1);
                    ci->state.colour = max(getint(p), 0);
                    ci->state.model = max(getint(p), 0);

                    string password = "", authname = "";
                    getstring(text, p); copystring(password, text);
                    getstring(text, p); copystring(authname, text);
                    int disc = auth::allowconnect(ci, true, password, authname);
                    if(disc)
                    {
                        disconnect_client(sender, disc);
                        return;
                    }

                    if(!ci->connectauth) connected(ci);

                    break;
                }

                case N_AUTHANS:
                {
                    uint id = (uint)getint(p);
                    getstring(text, p);
                    auth::answerchallenge(ci, id, text);
                    break;
                }

                case N_PING:
                    getint(p);
                    break;

                default:
                    disconnect_client(sender, DISC_TAGT);
                    return;
            }
            return;
        }
        else if(chan==2)
        {
            if(receivefile(sender, p.buf, p.maxlen))
            {
                mapsending = false;
                sendf(-1, 1, "ri", N_SENDMAP);
            }
            return;
        }
        if(p.packet->flags&ENET_PACKET_FLAG_RELIABLE) reliablemessages = true;
        #define QUEUE_MSG { while(curmsg<p.length()) ci->messages.add(p.buf[curmsg++]); }
        #define QUEUE_BUF(body) { curmsg = p.length(); body; }
        #define QUEUE_INT(n) QUEUE_BUF(putint(ci->messages, n))
        #define QUEUE_UINT(n) QUEUE_BUF(putuint(ci->messages, n))
        #define QUEUE_FLT(n) QUEUE_BUF(putfloat(ci->messages, n))
        #define QUEUE_STR(text) QUEUE_BUF(sendstring(text, ci->messages))

        int curmsg;
        while((curmsg = p.length()) < p.maxlen)
        {
            int curtype = getint(p);
            prevtype = type;
            switch(type = checktype(curtype, ci))
            {
                case N_POS:
                {
                    int lcn = getuint(p);
                    if(lcn<0)
                    {
                        disconnect_client(sender, DISC_CN);
                        return;
                    }

                    bool havecn = true;
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!cp || (cp->clientnum != sender && cp->state.ownernum != sender))
                        havecn = false;

                    p.get();
                    getuint(p);
                    uint flags = getuint(p);
                    vec pos, vel, falling;
                    float yaw, pitch, roll;
                    loopk(3)
                    {
                        int n = p.get(); n |= p.get()<<8; if(flags&(1<<k)) { n |= p.get()<<16; if(n&0x800000) n |= -1<<24; }
                        pos[k] = n/DMF;
                    }
                    int dir = p.get(); dir |= p.get()<<8;
                    yaw = dir%360;
                    pitch = clamp(dir/360, 0, 180)-90;
                    roll = clamp(int(p.get()), 0, 180)-90;
                    int mag = p.get(); if(flags&(1<<3)) mag |= p.get()<<8;
                    dir = p.get(); dir |= p.get()<<8;
                    vecfromyawpitch(dir%360, clamp(dir/360, 0, 180)-90, 1, 0, vel);
                    vel.mul(mag/DVELF);
                    if(flags&(1<<4))
                    {
                        mag = p.get(); if(flags&(1<<5)) mag |= p.get()<<8;
                        if(flags&(1<<6))
                        {
                            dir = p.get(); dir |= p.get()<<8;
                            vecfromyawpitch(dir%360, clamp(dir/360, 0, 180)-90, 1, 0, falling);
                        }
                        else falling = vec(0, 0, -1);
                        falling.mul(mag/DVELF);
                    }
                    else falling = vec(0, 0, 0);
                    if(flags&(1<<7)) loopk(2) p.get();
                    if(havecn)
                    {
                        vec oldpos = cp->state.o;
                        cp->state.o = pos;
                        cp->state.vel = vel;
                        cp->state.falling = falling;
                        cp->state.yaw = yaw;
                        cp->state.pitch = pitch;
                        cp->state.roll = roll;
                        if(cp->state.state==CS_ALIVE || cp->state.state==CS_EDITING)
                        {
                            cp->position.setsize(0);
                            while(curmsg<p.length()) cp->position.add(p.buf[curmsg++]);
                        }
                        if(cp->state.state==CS_ALIVE)
                        {
                            if(smode) smode->moved(cp, oldpos, cp->state.o);
                            mutate(smuts, mut->moved(cp, oldpos, cp->state.o));
                        }
                    }
                    break;
                }

                case N_SPHY:
                {
                    int lcn = getint(p), idx = getint(p);
                    if(idx >= SPHY_SERVER) break; // clients can't send this
                    if(idx == SPHY_POWER) getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci)) break;
                    if(idx == SPHY_EXTINGUISH)
                    {
                        if(cp->state.burning(gamemillis, GAME(burntime))) cp->state.lastburn = cp->state.lastburntime = 0;
                        else break; // don't propogate
                    }
                    else if((idx == SPHY_BOOST || idx == SPHY_DASH) && (!cp->state.lastboost || gamemillis-cp->state.lastboost > GAME(impulsedelay)))
                        cp->state.lastboost = gamemillis;
                    QUEUE_MSG;
                    break;
                }

                case N_EDITMODE:
                {
                    int val = getint(p);
                    if(!ci || ci->state.aitype > AI_NONE) break;
                    if(!allowstate(ci, val ? 4 : 5) && !haspriv(ci, PRIV_MASTER, "unspectate and edit")) { spectator(ci); break; }
                    ci->state.editspawn(gamemode, mutators);
                    if(val)
                    {
                        if(smode) smode->leavegame(ci);
                        mutate(smuts, mut->leavegame(ci));
                        ci->state.state = CS_EDITING;
                        ci->events.deletecontents();
                    }
                    else
                    {
                        ci->state.state = CS_ALIVE;
                        if(smode) smode->entergame(ci);
                        mutate(smuts, mut->entergame(ci));
                    }
                    QUEUE_MSG;
                    break;
                }

                case N_MAPCRC:
                {
                    getstring(text, p);
                    int crc = getint(p);
                    if(!ci) break;
                    if(strcmp(text, smapname))
                    {
                        if(ci->clientmap[0])
                        {
                            ci->clientmap[0] = '\0';
                            ci->mapcrc = 0;
                        }
                        else if(ci->mapcrc > 0) ci->mapcrc = 0;
                        break;
                    }
                    copystring(ci->clientmap, text);
                    ci->mapcrc = text[0] ? crc : 1;
                    checkmaps();
                    break;
                }

                case N_CHECKMAPS:
                    checkmaps(sender);
                    break;

                case N_TRYSPAWN:
                {
                    int lcn = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci) || !allowstate(cp, 1)) break;
                    if(!ci->clientmap[0] && !ci->mapcrc)
                    {
                        ci->mapcrc = -1;
                        checkmaps();
                    }
                    if(smode) smode->canspawn(cp, true);
                    mutate(smuts, mut->canspawn(cp, true));
                    cp->state.state = CS_DEAD;
                    waiting(cp, 1, 1);
                    break;
                }

                case N_LOADWEAP:
                {
                    int lcn = getint(p), aweap = getint(p), bweap = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci) || !isweap(aweap) || !m_arena(gamemode, mutators)) break;
                    cp->state.loadweap[0] = aweap;
                    cp->state.loadweap[1] = bweap;
                    chkloadweap(cp);
                    if(cp->state.state == CS_ALIVE)
                    {
                        cp->state.state = CS_DEAD;
                        waiting(cp, 1, 1);
                    }
                    break;
                }

                case N_WEAPSELECT:
                {
                    int lcn = getint(p), id = getint(p), weap = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci) || !isweap(weap)) break;
                    switchevent *ev = new switchevent;
                    ev->id = id;
                    ev->weap = weap;
                    ev->millis = cp->getmillis(gamemillis, ev->id);
                    cp->addevent(ev);
                    break;
                }

                case N_SPAWN:
                {
                    int lcn = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci) || !allowstate(cp, 2)) break;
                    cp->state.lastrespawn = -1;
                    cp->state.state = CS_ALIVE;
                    if(smode) smode->spawned(cp);
                    mutate(smuts, mut->spawned(cp););
                    QUEUE_BUF({
                        putint(ci->messages, N_SPAWN);
                        putint(ci->messages, cp->clientnum);
                        sendstate(cp->state, ci->messages);
                    });
                    break;
                }

                case N_SUICIDE:
                {
                    int lcn = getint(p), flags = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci)) break;
                    suicideevent *ev = new suicideevent;
                    ev->flags = flags;
                    cp->addevent(ev);
                    break;
                }

                case N_SHOOT:
                {
                    int lcn = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    bool havecn = (cp && (cp->clientnum == ci->clientnum || cp->state.ownernum == ci->clientnum));
                    shotevent *ev = new shotevent;
                    ev->id = getint(p);
                    ev->weap = getint(p);
                    ev->flags = getint(p);
                    ev->scale = getint(p);
                    if(!isweap(ev->weap)) havecn = false;
                    else
                    {
                        ev->scale = clamp(ev->scale, 0, WEAP2(ev->weap, power, ev->flags&HIT_ALT));
                        if(havecn) ev->millis = cp->getmillis(gamemillis, ev->id);
                    }
                    loopk(3) ev->from[k] = getint(p);
                    ev->num = getint(p);
                    loopj(ev->num)
                    {
                        if(p.overread()) break;
                        if(j >= 100) { loopk(3) getint(p); continue; }
                        shotmsg &s = ev->shots.add();
                        s.id = getint(p);
                        loopk(3) s.pos[k] = getint(p);
                    }
                    if(havecn)
                    {
                        int rays = WEAP2(ev->weap, rays, ev->flags&HIT_ALT);
                        if(rays > 1 && WEAP2(ev->weap, power, ev->flags&HIT_ALT)) rays = int(ceilf(rays*ev->scale/float(WEAP2(ev->weap, power, ev->flags&HIT_ALT))));
                        while(ev->shots.length() > rays) ev->shots.remove(rnd(ev->shots.length()));
                        cp->addevent(ev);
                    }
                    else delete ev;
                    break;
                }

                case N_DROP:
                { // gee this looks familiar
                    int lcn = getint(p), id = getint(p), weap = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!cp || (cp->clientnum != ci->clientnum && cp->state.ownernum != ci->clientnum) || !isweap(weap))
                        break;
                    dropevent *ev = new dropevent;
                    ev->id = id;
                    ev->weap = weap;
                    ev->millis = cp->getmillis(gamemillis, ev->id);
                    cp->events.add(ev);
                    break;
                }

                case N_RELOAD:
                {
                    int lcn = getint(p), id = getint(p), weap = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!cp || (cp->clientnum != ci->clientnum && cp->state.ownernum != ci->clientnum) || !isweap(weap))
                        break;
                    reloadevent *ev = new reloadevent;
                    ev->id = id;
                    ev->weap = weap;
                    ev->millis = cp->getmillis(gamemillis, ev->id);
                    cp->events.add(ev);
                    break;
                }

                case N_DESTROY: // cn millis weap id radial hits
                {
                    int lcn = getint(p), millis = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    bool havecn = (cp && (cp->clientnum == ci->clientnum || cp->state.ownernum == ci->clientnum));
                    destroyevent *ev = new destroyevent;
                    ev->weap = getint(p);
                    ev->flags = getint(p);
                    if(havecn) ev->millis = cp->getmillis(gamemillis, millis);
                    ev->id = getint(p);
                    ev->radial = getint(p);
                    ev->scale = getint(p);
                    int hits = getint(p);
                    loopj(hits)
                    {
                        if(p.overread()) break;
                        static hitset dummy;
                        hitset &hit = havecn && j < 100 ? ev->hits.add() : dummy;
                        hit.flags = getint(p);
                        hit.proj = getint(p);
                        hit.target = getint(p);
                        hit.dist = getint(p);
                        loopk(3) hit.dir[k] = getint(p);
                    }
                    if(havecn) cp->events.add(ev);
                    else delete ev;
                    break;
                }

                case N_ITEMUSE:
                {
                    int lcn = getint(p), id = getint(p), ent = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci)) break;
                    useevent *ev = new useevent;
                    ev->id = id;
                    ev->ent = ent;
                    ev->millis = cp->getmillis(gamemillis, ev->id);
                    cp->events.add(ev);
                    break;
                }

                case N_TRIGGER:
                {
                    int lcn = getint(p), ent = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci) || cp->state.state != CS_ALIVE) break;
                    if(sents.inrange(ent))
                    {
                        if(sents[ent].type == CHECKPOINT)
                        {
                            if(cp->state.cpnodes.find(ent) < 0 && (sents[ent].attrs[5] == triggerid || !sents[ent].attrs[5]) && m_check(sents[ent].attrs[3], sents[ent].attrs[4], gamemode, mutators))
                            {
                                if(m_trial(gamemode)) switch(sents[ent].attrs[6])
                                {
                                    case CP_LAST: case CP_FINISH:
                                    {
                                        int laptime = gamemillis-cp->state.cpmillis;
                                        if(cp->state.cptime <= 0 || laptime < cp->state.cptime)
                                        {
                                            cp->state.cptime = laptime;
                                            if(sents[ent].attrs[6] == CP_FINISH) { cp->state.cpmillis = -gamemillis; waiting(cp, 0, 0); }
                                        }
                                        sendf(-1, 1, "ri4", N_CHECKPOINT, cp->clientnum, laptime, cp->state.cptime);
                                        if(m_team(gamemode, mutators))
                                        {
                                            score &ts = teamscore(cp->team);
                                            if(!ts.total || ts.total > cp->state.cptime)
                                            {
                                                ts.total = cp->state.cptime;
                                                sendf(-1, 1, "ri3", N_SCORE, ts.team, ts.total);
                                            }
                                        }
                                    }
                                    case CP_RESPAWN: if(sents[ent].attrs[6] == CP_RESPAWN && cp->state.cpmillis) break;
                                    case CP_START:
                                    {
                                        cp->state.cpmillis = gamemillis;
                                        cp->state.cpnodes.shrink(0);
                                    }
                                    default: break;
                                }
                                cp->state.cpnodes.add(ent);
                            }
                        }
                        else if(sents[ent].type == TRIGGER)
                        {
                            bool commit = false, kin = false;
                            if(sents[ent].attrs[4] == triggerid || !sents[ent].attrs[4]) switch(sents[ent].attrs[1])
                            {
                                case TR_TOGGLE:
                                {
                                    if(!sents[ent].spawned || sents[ent].attrs[2] != TA_AUTO)
                                    {
                                        sents[ent].millis = gamemillis+(triggertime(ent)*2);
                                        sents[ent].spawned = !sents[ent].spawned;
                                        commit = kin = true;
                                    }
                                    //else sendf(cp->clientnum, 1, "ri3", N_TRIGGER, ent, sents[ent].spawned ? 1 : 0);
                                    break;
                                }
                                case TR_ONCE: if(sents[ent].spawned) break;
                                case TR_LINK:
                                {
                                    sents[ent].millis = gamemillis+(triggertime(ent)*2); kin = true;
                                    if(!sents[ent].spawned)
                                    {
                                        sents[ent].spawned = true;
                                        commit = true;
                                    }
                                    //else sendf(cp->clientnum, 1, "ri3", N_TRIGGER, ent, sents[ent].spawned ? 1 : 0);
                                    break;
                                }
                                case TR_EXIT:
                                {
                                    if(sents[ent].spawned) break;
                                    if(m_campaign(gamemode))
                                    {
                                        sents[ent].spawned = true;
                                        startintermission();
                                    }
                                }
                            }
                            if(commit) sendf(-1, 1, "ri3", N_TRIGGER, ent, sents[ent].spawned ? 1 : 0);
                            if(kin) loopvj(sents[ent].kin) if(sents.inrange(sents[ent].kin[j]))
                            {
                                sents[sents[ent].kin[j]].spawned = sents[ent].spawned;
                                sents[sents[ent].kin[j]].millis = sents[ent].millis;
                            }
                        }
                    }
                    else if(GAME(serverdebug)) srvmsgf(cp->clientnum, "sync error: cannot trigger %d - not a trigger", ent);
                    break;
                }

                case N_TEXT:
                {
                    int lcn = getint(p), flags = getint(p);
                    getstring(text, p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci)) break;
                    if(flags&SAY_TEAM && !m_team(gamemode, mutators)) flags &= ~SAY_TEAM;
                    loopv(clients)
                    {
                        clientinfo *t = clients[i];
                        if(t == cp || !allowbroadcast(t->clientnum) || (flags&SAY_TEAM && cp->team != t->team)) continue;
                        sendf(t->clientnum, 1, "ri3s", N_TEXT, cp->clientnum, flags, text);
                    }
                    defformatstring(m)("%s", colorname(cp));
                    if(flags&SAY_TEAM)
                    {
                        defformatstring(t)(" (\fs\f[%d]%s\fS)", TEAM(cp->team, colour), TEAM(cp->team, name));
                        concatstring(m, t);
                    }
                    if(flags&SAY_ACTION) relayf(0, "\fv* \fs%s\fS \fs\fv%s\fS", m, text);
                    else relayf(0, "\fa<\fs\fw%s\fS> \fs\fw%s\fS", m, text);
                    break;
                }

                case N_COMMAND:
                {
                    int lcn = getint(p), nargs = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    string cmd;
                    getstring(cmd, p);
                    getstring(text, p);
                    if(!hasclient(cp, ci)) break;
                    parsecommand(cp, nargs, cmd, text);
                    break;
                }

                case N_SETPLAYERINFO:
                {
                    QUEUE_MSG;
                    defformatstring(oldname)("%s", colorname(ci));
                    getstring(text, p);
                    ci->state.colour = max(getint(p), 0);
                    ci->state.model = max(getint(p), 0);
                    if(!text[0]) copystring(text, "unnamed");
                    filtertext(text, text, true, true, true, MAXNAMELEN);
                    copystring(ci->name, text, MAXNAMELEN+1);
                    relayf(2, "\fm* %s is now known as %s", oldname, colorname(ci));
                    QUEUE_STR(ci->name);
                    QUEUE_INT(ci->state.colour);
                    QUEUE_INT(ci->state.model);
                    break;
                }

                case N_SWITCHTEAM:
                {
                    int team = getint(p);
                    if(((ci->state.state == CS_SPECTATOR || ci->state.state == CS_EDITING) && team != TEAM_NEUTRAL) || !isteam(gamemode, mutators, team, TEAM_FIRST) || ci->state.aitype >= AI_START)
                        team = chooseteam(ci, team);
                    if(ci->team != team)
                    {
                        setteam(ci, team);
                        sendf(-1, 1, "ri3", N_SETTEAM, sender, team);
                    }
                    break;
                }

                case N_MAPVOTE:
                {
                    getstring(text, p);
                    filtertext(text, text);
                    const char *s = text;
                    if(!strncasecmp(s, "maps/", 5) || !strncasecmp(s, "maps\\", 5)) s += 5;
                    int reqmode = getint(p), reqmuts = getint(p);
                    vote(s, reqmode, reqmuts, sender);
                    break;
                }

                case N_CLEARVOTE:
                {
                    if(ci->mapvote[0])
                    {
                        ci->mapvote[0] = 0;
                        ci->modevote = ci->mutsvote = -1;
                        sendf(-1, 1, "ri2", N_CLEARVOTE, ci->clientnum);
                    }
                    break;
                }

                case N_GAMEINFO:
                {
                    int n, np = getint(p);
                    while((n = getint(p)) != -1)
                    {
                        int type = getint(p), numattr = getint(p);
                        if(p.overread() || type < 0 || type >= MAXENTTYPES || n < 0 || n >= MAXENTS) break;
                        if(!hasgameinfo && enttype[type].syncs)
                        {
                            while(sents.length() <= n) sents.add();
                            sents[n].reset();
                            sents[n].type = type;
                            sents[n].spawned = false; // wait a bit then load 'em up
                            sents[n].millis = gamemillis;
                            sents[n].attrs.add(0, clamp(numattr, 5, MAXENTATTRS));
                            loopk(numattr) { if(p.overread()) break; int attr = getint(p); if(sents[n].attrs.inrange(k)) sents[n].attrs[k] = attr; }
                            if(enttype[type].syncpos) loopj(3) { if(p.overread()) break; sents[n].o[j] = getint(p)/DMF; }
                            if(enttype[type].synckin)
                            {
                                int numkin = getint(p);
                                sents[n].kin.add(0, clamp(numkin, 0, MAXENTKIN));
                                loopk(numkin) { if(p.overread()) break; int kin = getint(p); if(sents[n].kin.inrange(k)) sents[n].kin[k] = kin; }
                            }
                        }
                        else
                        {
                            loopk(numattr) { if(p.overread()) break; getint(p); }
                            if(enttype[type].syncpos) loopj(3) { if(p.overread()) break; getint(p); }
                            if(enttype[type].synckin)
                            {
                                int numkin = getint(p);
                                loopk(numkin) { if(p.overread()) break; getint(p); }
                            }
                        }
                    }
                    if(!hasgameinfo) setupgameinfo(np);
                    break;
                }

                case N_SCORE:
                    getint(p);
                    getint(p);
                    QUEUE_MSG;
                    break;

                case N_INFOAFFIN:
                    getint(p);
                    getint(p);
                    getint(p);
                    getint(p);
                    QUEUE_MSG;
                    break;

                case N_SETUPAFFIN:
                    if(smode==&defendmode) defendmode.parseaffinity(p);
                    break;

                case N_MOVEAFFIN:
                {
                    int cn = getint(p), id = getint(p);
                    vec o, inertia;
                    loopi(3) o[i] = getint(p)/DMF;
                    loopi(3) inertia[i] = getint(p)/DMF;
                    clientinfo *cp = (clientinfo *)getinfo(cn);
                    if(!cp || !hasclient(cp, ci)) break;
                    if(smode==&capturemode) capturemode.moveaffinity(cp, cn, id, o, inertia);
                    else if(smode==&bombermode) bombermode.moveaffinity(cp, cn, id, o, inertia);
                    break;
                }

                case N_TAKEAFFIN:
                {
                    int lcn = getint(p), flag = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci) || cp->state.state == CS_SPECTATOR) break;
                    if(smode==&capturemode) capturemode.takeaffinity(cp, flag);
                    else if(smode==&bombermode) bombermode.takeaffinity(cp, flag);
                    break;
                }

                case N_RESETAFFIN:
                {
                    int flag = getint(p);
                    if(!ci) break;
                    if(smode==&capturemode) capturemode.resetaffinity(ci, flag);
                    else if(smode==&bombermode) bombermode.resetaffinity(ci, flag);
                    break;
                }

                case N_SCOREAFFIN:
                {
                    int lcn = getint(p), relay = getint(p), goal = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci) || cp->state.state == CS_SPECTATOR) break;
                    if(smode==&bombermode) bombermode.scoreaffinity(cp, relay, goal);
                    break;
                }

                case N_DROPAFFIN:
                {
                    int lcn = getint(p), tcn = getint(p);
                    vec droploc, inertia;
                    loopk(3) droploc[k] = getint(p)/DMF;
                    loopk(3) inertia[k] = getint(p)/DMF;
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci) || cp->state.state == CS_SPECTATOR) break;
                    if(smode==&capturemode) capturemode.dropaffinity(cp, droploc, inertia, tcn);
                    else if(smode==&bombermode) bombermode.dropaffinity(cp, droploc, inertia, tcn);
                    break;
                }

                case N_INITAFFIN:
                {
                    if(smode==&capturemode) capturemode.parseaffinity(p);
                    else if(smode==&bombermode) bombermode.parseaffinity(p);
                    break;
                }

                case N_PING:
                    sendf(sender, 1, "i2", N_PONG, getint(p));
                    break;

                case N_CLIENTPING:
                {
                    int ping = getint(p);
                    if(ci)
                    {
                        ci->ping = ping;
                        loopv(clients) if(clients[i]->state.ownernum == ci->clientnum) clients[i]->ping = ping;
                    }
                    QUEUE_MSG;
                    break;
                }

                case N_MASTERMODE:
                {
                    int mm = getint(p);
                    if(haspriv(ci, PRIV_MASTER, "change mastermode") && mm >= MM_OPEN && mm <= MM_PRIVATE)
                    {
                        if(haspriv(ci, PRIV_ADMIN) || (mastermask()&(1<<mm)))
                        {
                            mastermode = mm;
                            resetallows();
                            if(mastermode >= MM_PRIVATE)
                            {
                                loopv(clients)
                                {
                                    ipinfo &allow = allows.add();
                                    allow.time = totalmillis;
                                    allow.ip = getclientip(clients[i]->clientnum);
                                    allow.mask = 0xFFFFFFFF;
                                }
                            }
                            srvoutf(-3, "\fymastermode is now \fs\fc%d\fS (\fs\fc%s\fS)", mastermode, mastermodename(mastermode));
                        }
                        else srvmsgft(sender, CON_EVENT, "\frmastermode %d (%s) is disabled on this server", mm, mastermodename(mm));
                    }
                    break;
                }

                case N_CLEARBANS:
                {
                    if(haspriv(ci, PRIV_MASTER, "clear bans"))
                    {
                        resetbans();
                        srvoutf(3, "cleared existing bans");
                    }
                    break;
                }

                case N_KICKBAN:
                {
                    int victim = getint(p);
                    bool ban = getint(p) != 0;
                    if(haspriv(ci, PRIV_MASTER, "kick/ban people") && victim >= 0 && ci->clientnum != victim)
                    {
                        uint ip = getclientip(victim);
                        if(!ip) break;
                        clientinfo *cp = (clientinfo *)getinfo(victim);
                        if(!cp || cp->state.ownernum >= 0 || !cmppriv(ci, cp, "kick/ban")) break;
                        if(checkipinfo(allows, ip))
                        {
                            if(!haspriv(ci, PRIV_ADMIN, "kick/ban protected people")) break;
                            else if(ban) loopvrev(allows) if((ip & allows[i].mask) == allows[i].ip) allows.remove(i);
                        }
                        if(ban)
                        {
                            ipinfo &ban = bans.add();
                            ban.time = totalmillis;
                            ban.ip = ip;
                            ban.mask = 0xFFFFFFFF;
                            disconnect_client(victim, DISC_IPBAN);
                        }
                        else disconnect_client(victim, DISC_KICK);
                    }
                    break;
                }

                case N_SPECTATOR:
                {
                    int sn = getint(p), val = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(sn);
                    if(!cp || cp->state.aitype > AI_NONE) break;
                    if((sn != sender || !allowstate(cp, val ? 3 : 1)) && !haspriv(ci, PRIV_MASTER, sn != sender ? "spectate others" : "unspectate")) break;
                    spectate(cp, val);
                    break;
                }

                case N_SETTEAM:
                {
                    int who = getint(p), team = getint(p);
                    if(who<0 || who>=getnumclients() || !haspriv(ci, PRIV_MASTER, "change the team of others")) break;
                    clientinfo *cp = (clientinfo *)getinfo(who);
                    if(!cp || !m_team(gamemode, mutators) || !m_fight(gamemode) || cp->state.aitype >= AI_START) break;
                    if(cp->state.state == CS_SPECTATOR || cp->state.state == CS_EDITING || !isteam(gamemode, mutators, team, TEAM_FIRST)) break;
                    setteam(cp, team, true, true);
                    break;
                }

                case N_RECORDDEMO:
                {
                    int val = getint(p);
                    if(!haspriv(ci, restrictdemos ? PRIV_ADMIN : PRIV_MASTER, "record demos")) break;
                    if(!maxdemos || !maxdemosize) 
                    {
                        srvmsgft(ci->clientnum, CON_EVENT, "\frthe server has disabled demo recording"); 
                        break;
                    }
                    demonextmatch = val!=0;
                    srvoutf(4, "\fodemo recording is \fs\fc%s\fS for next match", demonextmatch ? "enabled" : "disabled");
                    break;
                }

                case N_STOPDEMO:
                {
                    if(!haspriv(ci, restrictdemos ? PRIV_ADMIN : PRIV_MASTER, "stop demos")) break;
                    if(m_demo(gamemode)) enddemoplayback();
                    else enddemorecord();
                    break;
                }

                case N_CLEARDEMOS:
                {
                    int demo = getint(p);
                    if(!haspriv(ci, restrictdemos ? PRIV_ADMIN : PRIV_MASTER, "clear demos")) break;
                    cleardemos(demo);
                    break;
                }

                case N_LISTDEMOS:
                    if(ci->state.state==CS_SPECTATOR) break;
                    listdemos(sender);
                    break;

                case N_GETDEMO:
                {
                    int n = getint(p);
                    if(ci->state.state==CS_SPECTATOR) break;
                    senddemo(sender, n);
                    break;
                }

                case N_EDITENT:
                {
                    int n = getint(p), oldtype = NOTUSED;
                    bool tweaked = false;
                    loopk(3) getint(p);
                    if(sents.inrange(n)) oldtype = sents[n].type;
                    else while(sents.length() <= n) sents.add();
                    if((sents[n].type = getint(p)) != oldtype) tweaked = true;
                    int numattrs = getint(p);
                    while(sents[n].attrs.length() < max(5, numattrs)) sents[n].attrs.add(0);
                    loopk(numattrs) sents[n].attrs[k] = getint(p);
                    if(oldtype == PLAYERSTART || sents[n].type == PLAYERSTART) setupspawns(true);
                    hasgameinfo = true;
                    QUEUE_MSG;
                    if(tweaked && enttype[sents[n].type].usetype != EU_NONE)
                    {
                        if(enttype[sents[n].type].usetype == EU_ITEM) setspawn(n, true, true, true);
                        if(sents[n].type == TRIGGER) setuptriggers(true);
                    }
                    break;
                }

                case N_EDITVAR:
                {
                    int t = getint(p);
                    getstring(text, p);
                    if(!ci || ci->state.state != CS_EDITING)
                    {
                        switch(t)
                        {
                            case ID_VAR: getint(p); break;
                            case ID_FVAR: getfloat(p); break;
                            case ID_SVAR: case ID_ALIAS: getstring(text, p); break;
                            default: break;
                        }
                        break;
                    }
                    QUEUE_INT(N_EDITVAR);
                    QUEUE_INT(t);
                    QUEUE_STR(text);
                    switch(t)
                    {
                        case ID_VAR:
                        {
                            int val = getint(p);
                            relayf(3, "\fc%s set worldvar %s to %d", colorname(ci), text, val);
                            QUEUE_INT(val);
                            break;
                        }
                        case ID_FVAR:
                        {
                            float val = getfloat(p);
                            relayf(3, "\fc%s set worldvar %s to %s", colorname(ci), text, floatstr(val));
                            QUEUE_FLT(val);
                            break;
                        }
                        case ID_SVAR:
                        case ID_ALIAS:
                        {
                            string val;
                            getstring(val, p);
                            relayf(3, "\fc%s set world%s %s to %s", colorname(ci), t == ID_ALIAS ? "alias" : "var", text, val);
                            QUEUE_STR(val);
                            break;
                        }
                        default: break;
                    }
                    break;
                }

                case N_GETMAP:
                {
                    clientinfo *best = choosebestclient();
                    ci->wantsmap = true;
                    if(ci == best) // we asked them for the map and they asked us, oops
                    {
                        mapsending = false;
                        best = choosebestclient();
                    }
                    if(!mapsending)
                    {
                        if(mapdata[0] && mapdata[1] && mapdata[2])
                        {
                            srvmsgft(ci->clientnum, CON_EVENT, "sending map, please wait..");
                            loopk(3) if(mapdata[k]) sendfile(sender, 2, mapdata[k], "ri", N_SENDMAPFILE+k);
                            sendwelcome(ci);
                            ci->needclipboard = totalmillis ? totalmillis : 1;
                        }
                        else if(best)
                        {
                            loopk(3) if(mapdata[k]) DELETEP(mapdata[k]);
                            srvmsgft(ci->clientnum, CON_EVENT, "map is being requested, please wait..");
                            sendf(best->clientnum, 1, "ri", N_GETMAP);
                            mapsending = true;
                        }
                        else
                        {
                            sendf(-1, 1, "ri", N_FAILMAP);
                            loopv(clients)
                            {
                                clientinfo *ci = clients[i];
                                ci->failedmap = false;
                            }
                        }
                    }
                    else srvmsgft(ci->clientnum, CON_EVENT, "map is being uploaded, please be patient..");
                    break;
                }

                case N_NEWMAP:
                {
                    int size = getint(p);
                    if(ci->state.state == CS_SPECTATOR) break;
                    if(size >= 0)
                    {
                        copystring(smapname, "maps/untitled");
                        sents.shrink(0);
                        hasgameinfo = true;
                        if(smode) smode->reset(true);
                        mutate(smuts, mut->reset(true));
                    }
                    QUEUE_MSG;
                    break;
                }

                case N_SETMASTER:
                {
                    int val = getint(p);
                    getstring(text, p);
                    if(val != 0)
                    {
                        if(adminpass[0] && (ci->local || (text[0] && checkpassword(ci, adminpass, text))))
                            auth::setmaster(ci, true, PRIV_ADMIN);
                        else if(ci->privilege <= PRIV_USER)
                        {
                            bool fail = false;
                            if(!(mastermask()&MM_AUTOAPPROVE) && !ci->privilege)
                            {
                                srvmsgft(ci->clientnum, CON_EVENT, "\fraccess denied, you need auth/admin access to gain master");
                                fail = true;
                            }
                            else loopv(clients) if(ci != clients[i] && clients[i]->privilege >= PRIV_MASTER)
                            {
                                srvmsgft(ci->clientnum, CON_EVENT, "\fraccess denied, there is already another master");
                                fail = true;
                                break;
                            }
                            if(!fail) auth::setmaster(ci, true, PRIV_MASTER);
                        }
                    }
                    else auth::setmaster(ci, false);
                    break; // don't broadcast the master password
                }

                case N_ADDBOT: getint(p); break;
                case N_DELBOT: break;

                case N_AUTHTRY:
                {
                    getstring(text, p);
                    auth::tryauth(ci, text);
                    break;
                }

                case N_AUTHANS:
                {
                    uint id = (uint)getint(p);
                    getstring(text, p);
                    auth::answerchallenge(ci, id, text);
                    break;
                }

                case N_COPY:
                    ci->cleanclipboard();
                    ci->lastclipboard = totalmillis ? totalmillis : 1;
                    goto genericmsg;

                case N_PASTE:
                    if(ci->state.state!=CS_SPECTATOR) sendclipboard(ci);
                    goto genericmsg;

                case N_CLIPBOARD:
                {
                    int unpacklen = getint(p), packlen = getint(p);
                    ci->cleanclipboard(false);
                    if(ci->state.state==CS_SPECTATOR)
                    {
                        if(packlen > 0) p.subbuf(packlen);
                        break;
                    }
                    if(packlen <= 0 || packlen > (1<<16) || unpacklen <= 0)
                    {
                        if(packlen > 0) p.subbuf(packlen);
                        packlen = unpacklen = 0;
                    }
                    packetbuf q(32 + packlen, ENET_PACKET_FLAG_RELIABLE);
                    putint(q, N_CLIPBOARD);
                    putint(q, ci->clientnum);
                    putint(q, unpacklen);
                    putint(q, packlen);
                    if(packlen > 0) p.get(q.subbuf(packlen).buf, packlen);
                    ci->clipboard = q.finalize();
                    ci->clipboard->referenceCount++;
                    break;
                }

                case -1:
                    conoutf("\fy[tag error] from: %d, cur: %d, msg: %d, prev: %d", sender, curtype, type, prevtype);
                    disconnect_client(sender, DISC_TAGT);
                    return;

                case -2:
                    disconnect_client(sender, DISC_OVERFLOW);
                    return;

                default: genericmsg:
                {
                    int size = msgsizelookup(type);
                    if(size<=0)
                    {
                        conoutf("\fy[tag error] from: %d, cur: %d, msg: %d, prev: %d", sender, curtype, type, prevtype);
                        disconnect_client(sender, DISC_TAGT);
                        return;
                    }
                    loopi(size-1) getint(p);
                    if(ci) QUEUE_MSG;
                    break;
                }
            }
            if(verbose > 5) conoutf("\fy[server] from: %d, cur: %d, msg: %d, prev: %d", sender, curtype, type, prevtype);
        }
    }

    bool serveroption(char *arg)
    {
        if(arg[0]=='-' && arg[1]=='s') switch(arg[2])
        {
            case 'P': setsvar("adminpass", &arg[3]); return true;
            case 'k': setsvar("serverpass", &arg[3]); return true;
            default: break;
        }
        return false;
    }
};
#undef GAMESERVER
