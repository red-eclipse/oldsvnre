enum { AI_NONE = 0, AI_BOT, AI_TURRET, AI_GRUNT, AI_MAX, AI_START = AI_TURRET, AI_TOTAL = AI_MAX-AI_START };

struct aistyles
{
    int type,           weap,           health;
    float   xradius,    yradius,    height,     weight,     speed,      scale;
    bool    canmove,    canstrafe,  canjump,    cancrouch,  useweap,    living,     hitbox;
    const char  *name,      *playermodel[2];
};
#ifdef GAMESERVER
aistyles aistyle[] = {
    {
        AI_NONE,         -1,             0,
            3,          3,          14,         200,        50,         1,
            true,       true,       true,       true,       true,       true,       true,
                "player",   { "actors/player/male",     "actors/player/male/hwep" }
    },
    {
        AI_BOT,         -1,             0,
            3,          3,          14,         200,        50,         1,
            true,       true,       true,       true,       true,       true,       true,
                "bot",      { "actors/player/male",     "actors/player/male/hwep" }
    },
    {
        AI_TURRET,      WEAP_SMG,       100,
            4.75,       4.75,       8.75,       150,        1,          1,
            false,      false,      false,      false,      false,      false,      false,
                "turret",   { "actors/turret",          "actors/player/hwep" }
    },
    {
        AI_GRUNT,       WEAP_MELEE,     50,
            3,          3,          14,         150,        40,         1,
            true,       false,      true,       true,       true,       true,       true,
                "grunt",    { "actors/grunt",           "actors/player/hwep" }
    },
};
#else
struct gameent;
extern aistyles aistyle[];

namespace ai
{
    const int MAXWAYPOINTS      = USHRT_MAX - 2;
    const int MAXWAYPOINTLINKS  = 6;
    const int WAYPOINTRADIUS    = 16;

    const float MINWPDIST       = 4.f;     // is on top of
    const float CLOSEDIST       = 32.f;    // is close
    const float RETRYDIST       = 64.f;    // is close when retrying
    const float JETDIST         = 128.f;   // close when jetting
    const float FARDIST         = 256.f;   // too far to remap close
    const float JUMPMIN         = 2.f;     // decides to jump
    const float JUMPMAX         = 32.f;    // max jump
    const float SIGHTMIN        = 128.f;   // minimum line of sight
    const float SIGHTMAX        = 1024.f;  // maximum line of sight
    const float ALERTMIN        = 64.f;    // minimum alert distance
    const float ALERTMAX        = 512.f;   // maximum alert distance
    const float VIEWMIN         = 90.f;    // minimum field of view
    const float VIEWMAX         = 180.f;   // maximum field of view

    struct waypoint
    {
        vec o;
        float curscore, estscore;
		int weight;
        ushort route, prev;
        ushort links[MAXWAYPOINTLINKS];

        waypoint() {}
        waypoint(const vec &o, int weight = 0) : o(o), weight(weight), route(0) { memset(links, 0, sizeof(links)); }

        int score() const { return int(curscore) + int(estscore); }

        int find(int wp)
		{
			loopi(MAXWAYPOINTLINKS) if(links[i] == wp) return i;
			return -1;
		}

		bool haslinks() { return links[0]!=0; }
    };
    extern vector<waypoint> waypoints;

    static inline bool iswaypoint(int n)
    {
        return n > 0 && n < waypoints.length();
    }

    struct oldwaypoint
    {
        vec o;
        int ent;
        linkvector links;
    };
    extern vector<oldwaypoint> oldwaypoints;

    extern int showwaypoints, dropwaypoints;
    extern int closestwaypoint(const vec &pos, float mindist, bool links);
    extern void findwaypointswithin(const vec &pos, float mindist, float maxdist, vector<int> &results);
	extern void inferwaypoints(gameent *d, const vec &o, const vec &v, float mindist = ai::CLOSEDIST);

    struct avoidset
    {
        struct obstacle
        {
            void *owner;
            int numwaypoints;
            float above;

            obstacle(void *owner, float above = -1) : owner(owner), numwaypoints(0), above(above) {}
        };

        vector<obstacle> obstacles;
        vector<int> waypoints;

        void clear()
        {
            obstacles.setsize(0);
            waypoints.setsize(0);
        }

        void add(void *owner, float above)
        {
            obstacles.add(obstacle(owner, above));
        }

        void add(void *owner, float above, int wp)
        {
            if(obstacles.empty() || owner != obstacles.last().owner) add(owner, above);
            obstacles.last().numwaypoints++;
            waypoints.add(wp);
        }

		void add(avoidset &avoid)
		{
			waypoints.put(avoid.waypoints.getbuf(), avoid.waypoints.length());
			loopv(avoid.obstacles)
			{
				obstacle &o = avoid.obstacles[i];
				if(obstacles.empty() || o.owner != obstacles.last().owner) add(o.owner, o.above);
				obstacles.last().numwaypoints += o.numwaypoints;
			}
		}

        void avoidnear(void *owner, float above, const vec &pos, float limit);

        #define loopavoid(v, d, body) \
            if(!(v).obstacles.empty()) \
            { \
                int cur = 0; \
                loopv((v).obstacles) \
                { \
                    const ai::avoidset::obstacle &ob = (v).obstacles[i]; \
                    int next = cur + ob.numwaypoints; \
                    if(ob.owner != d) \
                    { \
                        for(; cur < next; cur++) \
                        { \
                            int wp = (v).waypoints[cur]; \
                            body; \
                        } \
                    } \
                    cur = next; \
                } \
            }

        bool find(int n, gameent *d) const
        {
            loopavoid(*this, d, { if(wp == n) return true; });
            return false;
        }

        int remap(gameent *d, int n, vec &pos, bool retry = false);
    };

    extern bool route(gameent *d, int node, int goal, vector<int> &route, const avoidset &obstacles, int retries = 0);
    extern void navigate();
    extern void clearwaypoints(bool full = false);
    extern bool loadwaypoints(bool force = false, const char *mname = NULL);
    extern void savewaypoints(bool force = false, const char *mname = NULL);
    extern bool importwaypoints();
    extern bool getwaypoints(bool force = false, const char *mname = NULL, bool check = true);

    // ai state information for the owner client
    enum
    {
        AI_S_WAIT = 0,      // waiting for next command
        AI_S_DEFEND,        // defend goal target
        AI_S_PURSUE,        // pursue goal target
        AI_S_INTEREST,      // interest in goal entity
        AI_S_MAX
    };

    enum
    {
        AI_T_NODE,
        AI_T_ACTOR,
        AI_T_AFFINITY,
        AI_T_ENTITY,
        AI_T_DROP,
        AI_T_MAX
    };

    struct interest
    {
        int state, node, target, targtype;
        float score, tolerance;
        bool team;
        interest() : state(-1), node(-1), target(-1), targtype(-1), score(0.f), tolerance(1.f), team(false) {}
        ~interest() {}
    };

    struct aistate
    {
        int type, millis, targtype, target, idle;
        bool override;

        aistate(int m, int t, int r = -1, int v = -1) : type(t), millis(m), targtype(r), target(v)
        {
            reset();
        }
        ~aistate() {}

        void reset()
        {
            idle = 0;
            override = false;
        }
    };

    const int NUMPREVNODES = 8;

    struct aiinfo
    {
        vector<aistate> state;
        vector<int> route;
        vec target, spot;
        int weappref, enemy, enemyseen, enemymillis, prevnodes[NUMPREVNODES], targnode, targlast, targtime, targseq,
            lastrun, lastaction, lastcheck, jumpseed, jumprand, blocktime, blockseq, lastaimrnd, lastmelee, lastturn;
        float targyaw, targpitch, views[3], aimrnd[3];
        bool dontmove, tryreset;

        aiinfo()
        {
            clean();
            reset();
            loopk(3) views[k] = aimrnd[k] = 0.f;
        }
        ~aiinfo() {}

        void clean()
        {
            spot = target = vec(0, 0, 0);
            lastaction = lastcheck = enemyseen = enemymillis = blocktime = blockseq = targtime = targseq = lastaimrnd = lastmelee = lastturn = 0;
            lastrun = jumpseed = lastmillis;
            jumprand = lastmillis+5000;
            weappref = targnode = targlast = enemy = -1;
            targyaw = targpitch = 0;
        }

        void clear(bool tryit = false)
        {
            if(tryit) memset(prevnodes, -1, sizeof(prevnodes));
            route.setsize(0);
            lastcheck = 0;
        }

        void reset(bool tryit = false)
        {
            clear(tryit);
            state.setsize(0);
            addstate(AI_S_WAIT);
            tryreset = false;
        }

        bool hasprevnode(int n) const
        {
            loopi(NUMPREVNODES) if(prevnodes[i] == n) return true;
            return false;
        }

        void addprevnode(int n)
        {
            if(!hasprevnode(n))
            {
                memmove(&prevnodes[1], prevnodes, sizeof(prevnodes) - sizeof(prevnodes[0]));
                prevnodes[0] = n;
                lastcheck = 0;
            }
        }

        aistate &addstate(int t, int r = -1, int v = -1)
        {
            return state.add(aistate(lastmillis, t, r, v));
        }

        void removestate(int index = -1)
        {
            if(index < 0) state.pop();
            else if(state.inrange(index)) state.remove(index);
            if(!state.length()) addstate(AI_S_WAIT);
        }

        aistate &getstate(int idx = -1)
        {
            if(state.inrange(idx)) return state[idx];
            return state.last();
        }

        aistate &switchstate(aistate &b, int t, int r = -1, int v = -1)
        {
            if((b.type == t && b.targtype == r) || (b.type == AI_S_INTEREST && b.targtype == AI_T_NODE))
            {
                b.millis = lastmillis;
                b.target = v;
                b.reset();
                return b;
            }
            return addstate(t, r, v);
        }
    };

    extern avoidset obstacles, wpavoid;
    extern vec aitarget;
    extern int aidebug, aideadfade, showaiinfo;

    extern void startmap(const char *name, const char *reqname, bool empty);

    extern float viewdist(int x = 101);
    extern float viewfieldx(int x = 101);
    extern float viewfieldy(int x = 101);

    extern bool targetable(gameent *d, gameent *e, bool solid = false);
    extern bool cansee(gameent *d, vec &x, vec &y, bool force = false, vec &targ = aitarget);
    extern bool altfire(gameent *d, gameent *e);
    extern int owner(gameent *d);

    extern void init(gameent *d, int at, int et, int on, int sk, int bn, char *name, int tm, int cl, int md);

    extern bool badhealth(gameent *d);
    extern int checkothers(vector<int> &targets, gameent *d = NULL, int state = -1, int targtype = -1, int target = -1, bool teams = false, int *members = NULL);
    extern bool makeroute(gameent *d, aistate &b, int node, bool changed = true, int retries = 0);
    extern bool makeroute(gameent *d, aistate &b, const vec &pos, bool changed = true, int retries = 0);
    extern bool randomnode(gameent *d, aistate &b, const vec &pos, float guard = ALERTMIN, float wander = ALERTMAX);
    extern bool randomnode(gameent *d, aistate &b, float guard = ALERTMIN, float wander = ALERTMAX);
    extern bool violence(gameent *d, aistate &b, gameent *e, int pursue = 0);
    extern bool patrol(gameent *d, aistate &b, const vec &pos, float guard = ALERTMIN, float wander = ALERTMAX, int walk = 1, bool retry = false);
    extern bool defense(gameent *d, aistate &b, const vec &pos, float guard = ALERTMIN, float wander = ALERTMAX, int walk = 0);

    extern void spawned(gameent *d, int ent);
    extern void damaged(gameent *d, gameent *e, int weap, int flags, int damage);
    extern void killed(gameent *d, gameent *e);
    extern void itemspawned(int ent, int spawned);
    extern void update();
    extern void avoid();
    extern void think(gameent *d, bool run);
    extern void render();
    extern void preload();
};
#endif
