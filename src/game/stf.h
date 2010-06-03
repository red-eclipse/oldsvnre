#ifdef GAMESERVER
#define stfstate stfservstate
#endif
struct stfstate
{
    static const int SCORESECS = 100;

    struct flag
    {
        vec o;
        int kinship, owner, enemy;
#ifndef GAMESERVER
        string name, info;
        bool hasflag;
        int ent, lasthad;
#endif
        int owners, enemies, converted, securetime;

        flag()
        {
            kinship = TEAM_NEUTRAL;
#ifndef GAMESERVER
            ent = -1;
#endif
            reset();
        }

        void noenemy()
        {
            enemy = TEAM_NEUTRAL;
            enemies = 0;
            converted = 0;
        }

        void reset()
        {
            noenemy();
            owner = kinship;
            securetime = -1;
            owners = 0;
#ifndef GAMESERVER
            hasflag = false;
            lasthad = 0;
#endif
        }

        bool enter(int team)
        {
            if(owner == team)
            {
                owners++;
                return false;
            }
            if(!enemies)
            {
                if(enemy != team)
                {
                    converted = 0;
                    enemy = team;
                }
                enemies++;
                return true;
            }
            else if(enemy != team) return false;
            else enemies++;
            return false;
        }

        bool steal(int team)
        {
            return !enemies && owner != team;
        }

        bool leave(int team)
        {
            if(owner == team)
            {
                owners--;
                return false;
            }
            if(enemy != team) return false;
            enemies--;
            return !enemies;
        }

        int occupy(int team, int units, int occupy, bool instant)
        {
            if(enemy != team) return -1;
            converted += units;
            if(units<0)
            {
                if(converted<=0) noenemy();
                return -1;
            }
            else if(converted<(!instant && owner ? 2 : 1)*occupy) return -1;
            if(!instant && owner) { owner = kinship; converted = 0; enemy = team; return 0; }
            else { owner = team; securetime = 0; owners = enemies; noenemy(); return 1; }
        }
    };

    vector<flag> flags;

    struct score
    {
        int team, total;
    };

    vector<score> scores;

    int secured;

    stfstate() : secured(0) {}

    void reset()
    {
        flags.shrink(0);
        scores.shrink(0);
        secured = 0;
    }

    score &findscore(int team)
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

    void addflag(const vec &o, int team)
    {
        flag &b = flags.add();
        b.o = o;
        b.kinship = team;
        b.reset();
    }

    void initflag(int i, int kin, int owner, int enemy, int converted)
    {
        if(!flags.inrange(i)) return;
        flag &b = flags[i];
        b.kinship = kin;
        b.reset();
        b.owner = owner;
        b.enemy = enemy;
        b.converted = converted;
    }

    bool hasflags(int team)
    {
        loopv(flags)
        {
            flag &b = flags[i];
            if(b.owner && b.owner == team) return true;
        }
        return false;
    }

    float disttoenemy(flag &b)
    {
        float dist = 1e10f;
        loopv(flags)
        {
            flag &e = flags[i];
            if(e.owner && b.owner != e.owner)
                dist = min(dist, b.o.dist(e.o));
        }
        return dist;
    }

    bool insideflag(const flag &b, const vec &o, float scale = 1.f)
    {
        float dx = (b.o.x-o.x), dy = (b.o.y-o.y), dz = (b.o.z-o.z);
        return dx*dx + dy*dy <= (enttype[FLAG].radius*enttype[FLAG].radius)*scale && fabs(dz) <= enttype[FLAG].radius*scale;
    }
};

#ifndef GAMESERVER
namespace stf
{
    extern stfstate st;
    extern void sendflags(packetbuf &p);
    extern void updateflag(int i, int owner, int enemy, int converted);
    extern void setscore(int team, int total);
    extern void setupflags();
    extern void drawlast(int w, int h, int &tx, int &ty, float blend);
    extern void drawblips(int w, int h, float blend);
    extern int drawinventory(int x, int y, int s, int m, float blend);
    extern void preload();
    extern void render();
    extern void adddynlights();
    extern int aiowner(gameent *d);
    extern void aifind(gameent *d, ai::aistate &b, vector<ai::interest> &interests);
    extern bool aicheck(gameent *d, ai::aistate &b);
    extern bool aidefend(gameent *d, ai::aistate &b);
    extern bool aipursue(gameent *d, ai::aistate &b);
    extern void removeplayer(gameent *d);
}
#endif
