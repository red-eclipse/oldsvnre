#include "game.h"
namespace ai
{
    entities::avoidset obs, wps;
    int updatemillis = 0, iteration = 0, itermillis = 0;
    float oldenemyspeed = -1, oldenemyscale = -1, oldbotspeed = -1, oldbotscale = -1;
    vec aitarget(0, 0, 0);

    VAR(0, aidebug, 0, 0, 6);
    VAR(0, aiforcegun, -1, -1, WEAP_MAX-1);
    VAR(0, aicampaign, 0, 0, 1);
    VAR(IDF_PERSIST, aideadfade, 0, 10000, INT_MAX-1);
    VAR(IDF_PERSIST, showaiinfo, 0, 0, 2); // 0/1 = shows/hides bot join/parts, 2 = show more verbose info

    float viewdist(int x) { return x <= 100 ? clamp((SIGHTMIN+(SIGHTMAX-SIGHTMIN))/100.f*float(x), float(SIGHTMIN), max(float(fog), ai::SIGHTMIN)) : max(float(fog), ai::SIGHTMIN); }
    float viewfieldx(int x) { return x <= 100 ? clamp((VIEWMIN+(VIEWMAX-VIEWMIN))/100.f*float(x), float(VIEWMIN), float(VIEWMAX)) : float(VIEWMAX); }
    float viewfieldy(int x) { return viewfieldx(x)*3.f/4.f; }

    int owner(gameent *d)
    {
        if(d->aitype >= AI_START)
        {
            if(entities::ents.inrange(d->aientity))
            {
                if(m_capture(game::gamemode)) return capture::aiowner(d);
                else if(m_defend(game::gamemode)) return defend::aiowner(d);
                else if(m_bomber(game::gamemode)) return bomber::aiowner(d);
            }
        }
        return d->team;
    }

    float weapmindist(int weap, bool alt)
    {
        return WEAPEX(weap, alt, game::gamemode, game::mutators, 1.f) && WEAP2(weap, collide, alt)&COLLIDE_OWNER ? WEAPEX(weap, alt, game::gamemode, game::mutators, 1.f) : 2;
    }

    float weapmaxdist(int weap, bool alt)
    {
        return WEAP2(weap, aidist, alt) ? WEAP2(weap, aidist, alt) : hdr.worldsize;
    }

    bool weaprange(gameent *d, int weap, bool alt, float dist)
    {
        if(!isweap(weap) || (WEAP2(weap, extinguish, alt) && d->inliquid)) return false;
        float mindist = weapmindist(weap, alt), maxdist = weapmaxdist(weap, alt);
        return dist >= mindist*mindist && dist <= maxdist*maxdist;
    }

    bool targetable(gameent *d, gameent *e)
    {
        if(e && d != e && !m_edit(game::gamemode) && e->state == CS_ALIVE && physics::issolid(e, d))
        {
            int dt = owner(d), et = owner(e);
            if(dt == TEAM_ENEMY && et == TEAM_ENEMY) return false;
            if(!m_team(game::gamemode, game::mutators) || dt != et) return true;
        }
        return false;
    }

    bool cansee(gameent *d, vec &x, vec &y, bool force, vec &targ)
    {
        if(force) return raycubelos(x, y, targ);
        return getsight(x, d->yaw, d->pitch, y, targ, d->ai->views[2], d->ai->views[0], d->ai->views[1]);
    }

    bool badhealth(gameent *d)
    {
        if(d->skill <= 100) return d->health <= (111-d->skill)/4;
        return false;
    }

    bool canshoot(gameent *d, gameent *e, bool alt = true)
    {
        if(isweap(d->weapselect) && weaprange(d, d->weapselect, alt, e->o.squaredist(d->o)))
        {
            int prot = m_protect(game::gamemode, game::mutators);
            if((d->aitype >= AI_START || !d->protect(lastmillis, prot)) && targetable(d, e))
                return d->canshoot(d->weapselect, alt ? HIT_ALT : 0, m_weapon(game::gamemode, game::mutators), lastmillis, (1<<WEAP_S_RELOAD));
        }
        return false;
    }

    bool altfire(gameent *d, gameent *e)
    {
        if(e && !WEAP(d->weapselect, zooms) && canshoot(d, e, true))
        {
            if(d->weapstate[d->weapselect] == WEAP_S_POWER)
            {
                if(d->action[AC_ALTERNATE] && (!d->action[AC_ATTACK] || d->actiontime[AC_ALTERNATE] > d->actiontime[AC_ATTACK]))
                    return true;
            }
            switch(d->weapselect)
            {
                case WEAP_PISTOL: return true; break;
                case WEAP_MELEE: case WEAP_ROCKET: default: return false; break;
                case WEAP_SWORD: case WEAP_SHOTGUN: case WEAP_SMG: case WEAP_PLASMA: case WEAP_GRENADE: if(rnd(d->skill*3) <= d->skill) return false; break;
                case WEAP_RIFLE: if(weaprange(d, d->weapselect, false, e->o.squaredist(d->o))) return false; break;
            }
            return true;
        }
        return false;
    }

    bool hastarget(gameent *d, aistate &b, gameent *e, bool alt, float yaw, float pitch, float dist)
    { // add margins of error
        if(weaprange(d, d->weapselect, alt, dist) || (d->skill <= 100 && !rnd(d->skill)))
        {
            if(weaptype[d->weapselect].melee) return true;
            float skew = clamp(float(lastmillis-d->ai->enemymillis)/float((d->skill*WEAP(d->weapselect, rdelay)/5000.f)+(d->skill*WEAP2(d->weapselect, adelay, alt)/500.f)), 0.f, d->weapselect >= WEAP_ITEM ? 0.25f : 1e16f),
                offy = yaw-d->yaw, offp = pitch-d->pitch;
            if(offy > 180) offy -= 360;
            else if(offy < -180) offy += 360;
            if(fabs(offy) <= d->ai->views[0]*skew && fabs(offp) <= d->ai->views[1]*skew) return true;
        }
        return false;
    }

    vec getaimpos(gameent *d, gameent *e, bool alt)
    {
        vec o = e->headpos();
        if(WEAP2(d->weapselect, radial, alt)) o.z -= e->height;
        if(d->skill <= 100)
        {
            if(lastmillis >= d->ai->lastaimrnd)
            {
                #define rndaioffset(r) ((rnd(int(r*WEAP2(d->weapselect, aiskew, alt)*2)+1)-(r*WEAP2(d->weapselect, aiskew, alt)))*(1.f/float(max(d->skill, 1))))
                loopk(3) d->ai->aimrnd[k] = rndaioffset(e->radius);
                int dur = (d->skill+10)*10;
                d->ai->lastaimrnd = lastmillis+dur+rnd(dur);
            }
            loopk(3) o[k] += d->ai->aimrnd[k];
        }
        return o;
    }

    bool hasweap(gameent *d, int weap)
    {
        if(!isweap(weap)) return false;
        if(w_carry(weap, m_weapon(game::gamemode, game::mutators)))
            return d->hasweap(weap, m_weapon(game::gamemode, game::mutators));
        return d->ammo[weap] >= WEAP(weap, max);
    }

    bool wantsweap(gameent *d, int weap, bool all = false)
    {
        if(!isweap(weap) || hasweap(d, weap)) return false;
        if(d->carry(m_weapon(game::gamemode, game::mutators)) >= maxcarry && (hasweap(d, d->ai->weappref) || weap != d->ai->weappref))
            return false;
        return true;
    }

    void create(gameent *d)
    {
        if(!d->ai && !(d->ai = new aiinfo()))
        {
            fatal("could not create ai");
            return;
        }
    }

    void destroy(gameent *d)
    {
        if(d->ai) DELETEP(d->ai);
    }

    void setparams(gameent *d)
    {
        if(!d->ai) return;
        bool hasent = d->aitype >= AI_START && entities::ents.inrange(d->aientity) && entities::ents[d->aientity]->type == ACTOR;
        if(hasent && entities::ents[d->aientity]->attrs[7] > 0) d->bspeed = d->speed = entities::ents[d->aientity]->attrs[7]*enemyspeed;
        else d->bspeed = d->speed = aistyle[clamp(d->aitype, int(AI_BOT), int(AI_MAX-1))].speed*(d->aitype >= AI_START ? enemyspeed : botspeed);
        if(hasent && entities::ents[d->aientity]->attrs[8] > 0) d->ai->scale = d->curscale = (entities::ents[d->aientity]->attrs[8]/100.f)*enemyscale;
        else d->ai->scale = d->curscale = aistyle[clamp(d->aitype, int(AI_BOT), int(AI_MAX-1))].scale*(d->aitype >= AI_START ? enemyscale : botscale);
        d->setparams();
    }

    void init(gameent *d, int at, int et, int on, int sk, int bn, char *name, int tm)
    {
        gameent *o = game::newclient(on);
        string m;
        if(o) copystring(m, game::colorname(o));
        else formatstring(m)("\fs\faunknown [\fs\fr%d\fS]\fS", on);
        bool resetthisguy = false;
        if(!d->name[0])
        {
            if(showaiinfo && game::showplayerinfo)
            {
                if(showaiinfo > 1) conoutft(CON_EVENT, "\fg%s assigned to %s at skill %d", game::colorname(d, name), m, sk);
                else conoutft(CON_EVENT, "\fg%s joined the game", game::colorname(d, name), m, sk);
            }
            game::cameras.shrink(0);
            resetthisguy = true;
        }
        else
        {
            if(d->ownernum != on)
            {
                if(showaiinfo && game::showplayerinfo)
                    conoutft(CON_EVENT, "\fg%s reassigned to %s", game::colorname(d, name), m);
                resetthisguy = true;
            }
            if(d->skill != sk && showaiinfo > 1 && game::showplayerinfo)
                conoutft(CON_EVENT, "\fg%s changed skill to %d", game::colorname(d, name), sk);
        }

        copystring(d->name, name, MAXNAMELEN);
        if((d->aitype = at) >= AI_START) d->type = ENT_AI;
        d->aientity = et;
        d->ownernum = on;
        d->skill = sk;
        d->team = tm;
        setparams(d);

        if(resetthisguy) projs::remove(d);
        if(d->ownernum >= 0 && game::player1->clientnum == d->ownernum)
        {
            create(d);
            if(d->ai)
            {
                d->ai->views[0] = viewfieldx(d->skill);
                d->ai->views[1] = viewfieldy(d->skill);
                d->ai->views[2] = viewdist(d->skill);
            }
        }
        else if(d->ai) destroy(d);
    }

    void update()
    {
        if(game::intermission) { loopv(game::players) if(game::players[i] && game::players[i]->ai) game::players[i]->stopmoving(true); }
        else // fixed rate logic done out-of-sequence at 1 frame per second for each ai
        {
            if(totalmillis-updatemillis > 1000)
            {
                avoid();
                if(multiplayer(false)) { aiforcegun = -1; aicampaign = 0; }
                updatemillis = totalmillis;
            }
            if(!iteration && totalmillis-itermillis > 1000)
            {
                iteration = 1;
                itermillis = totalmillis;
            }
            bool a = false, b = false;
            int c = 0;
            if(oldenemyspeed != enemyspeed || oldenemyscale != enemyscale)
            {
                oldenemyspeed = enemyspeed;
                oldenemyscale = enemyscale;
                a = true;
            }
            if(oldbotspeed != botspeed || oldbotscale != botscale)
            {
                oldbotspeed = botspeed;
                oldbotscale = botscale;
                b = true;
            }
            loopv(game::players) if(game::players[i] && game::players[i]->ai)
            {
                if(game::players[i]->aitype >= AI_START ? a : b) setparams(game::players[i]);
                think(game::players[i], ++c == iteration ? true : false);
            }
            if(++iteration > c) iteration = 0;
        }
    }

    bool checkothers(vector<int> &targets, gameent *d, int state, int targtype, int target, bool teams)
    { // checks the states of other ai for a match
        targets.shrink(0);
        gameent *e = NULL;
        loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && e != d && e->ai && e->state == CS_ALIVE && e->aitype == d->aitype)
        {
            if(targets.find(e->clientnum) >= 0) continue;
            if(teams)
            {
                int dt = owner(d), et = owner(e);
                if(dt != TEAM_ENEMY || et != TEAM_ENEMY)
                {
                    if(m_team(game::gamemode, game::mutators) && dt != et) continue;
                }
            }
            aistate &b = e->ai->getstate();
            if(state >= 0 && b.type != state) continue;
            if(target >= 0 && b.target != target) continue;
            if(targtype >=0 && b.targtype != targtype) continue;
            targets.add(e->clientnum);
        }
        return !targets.empty();
    }

    bool makeroute(gameent *d, aistate &b, int node, bool changed, bool retry)
    {
        if(d->lastnode < 0) return false;
        if(changed && !d->ai->route.empty() && d->ai->route[0] == node) return true;
        if(entities::route(d->lastnode, node, d->ai->route, obs, d, retry) > 0)
        {
            b.override = false;
            return true;
        }
        d->ai->clear(true);
        if(!retry) return makeroute(d, b, node, false, true);
        return false;
    }

    bool makeroute(gameent *d, aistate &b, const vec &pos, bool changed, bool retry)
    {
        int node = entities::closestent(WAYPOINT, pos, CLOSEDIST, true);
        return makeroute(d, b, node, changed, retry);
    }

    bool randomnode(gameent *d, aistate &b, const vec &pos, float guard, float wander)
    {
        static vector<int> candidates;
        candidates.setsize(0);
        entities::findentswithin(WAYPOINT, pos, guard, wander, candidates);

        while(!candidates.empty())
        {
            int w = rnd(candidates.length()), n = candidates.removeunordered(w);
            if(n != d->lastnode && !d->ai->hasprevnode(n) && makeroute(d, b, n)) return true;
        }
        return false;
    }

    bool randomnode(gameent *d, aistate &b, float guard, float wander)
    {
        return randomnode(d, b, d->feetpos(), guard, wander);
    }

    bool enemy(gameent *d, aistate &b, const vec &pos, float guard, bool pursue, bool force)
    {
        gameent *t = NULL, *e = NULL;
        vec dp = d->headpos();
        float mindist = guard*guard, bestdist = 1e16f;
        loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && e != d && targetable(d, e))
        {
            vec ep = getaimpos(d, e, altfire(d, e));
            float dist = ep.squaredist(dp);
            if(dist < bestdist && (cansee(d, dp, ep, d->aitype >= AI_START) || dist <= mindist))
            {
                t = e;
                bestdist = dist;
            }
        }
        if(t && violence(d, b, t, pursue)) return true;
        return false;
    }

    bool patrol(gameent *d, aistate &b, const vec &pos, float guard, float wander, int walk, bool retry)
    {
        if(aistyle[d->aitype].canmove)
        {
            vec feet = d->feetpos();
            float dist = feet.squaredist(pos), fardist = guard*4;
            if(walk == 2 || b.override || (walk && dist <= guard*guard) || !makeroute(d, b, pos))
            { // run away and back to keep ourselves busy
                if(!b.override && randomnode(d, b, pos, guard, wander))
                {
                    b.override = true;
                    return true;
                }
                else if(d->ai->route.empty())
                {
                    if(!retry)
                    {
                        b.override = false;
                        return patrol(d, b, pos, guard, wander, walk, true);
                    }
                    b.override = false;
                    return false;
                }
            }
            if(walk && dist >= fardist*fardist) b.idle = -1;
        }
        b.override = false;
        return true;
    }

    bool defense(gameent *d, aistate &b, const vec &pos, float guard, float wander, int walk)
    {
        bool hasenemy = enemy(d, b, pos, wander, weaptype[d->weapselect].melee, false);
        if(!aistyle[d->aitype].canmove) { b.idle = hasenemy ? 2 : 1; return true; }
        else if(!walk)
        {
            if(pos.squaredist(d->feetpos()) <= guard*guard)
            {
                b.idle = hasenemy ? 2 : 1;
                return true;
            }
            walk++;
        }
        return patrol(d, b, pos, guard, wander, walk);
    }

    bool violence(gameent *d, aistate &b, gameent *e, bool pursue)
    {
        if(e && targetable(d, e))
        {
            if(pursue && entities::ents.inrange(d->lastnode)) d->ai->switchstate(b, AI_S_PURSUE, AI_T_ACTOR, e->clientnum);
            if(d->ai->enemy != e->clientnum)
            {
                d->ai->enemyseen = d->ai->enemymillis = lastmillis;
                d->ai->enemy = e->clientnum;
            }
            return true;
        }
        return false;
    }

    bool target(gameent *d, aistate &b, bool pursue = false, bool force = false, float mindist = 0.f)
    {
        gameent *t = NULL, *e = NULL;
        vec dp = d->headpos(), tp(0, 0, 0);
        loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && e != d && targetable(d, e))
        {
            vec ep = getaimpos(d, e, altfire(d, e));
            float dist = ep.squaredist(dp);
            if((!t || dist < tp.squaredist(dp)) && ((mindist > 0 && dist <= mindist) || force || cansee(d, dp, ep, d->aitype >= AI_START)))
            {
                t = e;
                tp = ep;
            }
        }
        if(t) return violence(d, b, t, pursue);
        return false;
    }

    void assist(gameent *d, aistate &b, vector<interest> &interests, bool all = false, bool force = false)
    {
        gameent *e = NULL;
        loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && e != d && (all || e->aitype < 0) && owner(d) == owner(e))
        {
            interest &n = interests.add();
            n.state = AI_S_DEFEND;
            n.node = e->lastnode;
            n.target = e->clientnum;
            n.targtype = AI_T_ACTOR;
            n.score = e->o.squaredist(d->o)/(force ? 1e8f : (hasweap(d, d->ai->weappref) ? 1e4f : 1e2f));
        }
    }

    void items(gameent *d, aistate &b, vector<interest> &interests, bool force = false)
    {
        vec pos = d->feetpos();
        int sweap = m_weapon(game::gamemode, game::mutators);
        loopj(entities::lastusetype[EU_ITEM])
        {
            gameentity &e = *(gameentity *)entities::ents[j];
            if(enttype[e.type].usetype != EU_ITEM) continue;
            switch(e.type)
            {
                case WEAPON:
                {
                    int attr = w_attr(game::gamemode, e.attrs[0], sweap);
                    if(e.spawned && isweap(attr) && wantsweap(d, attr, true))
                    { // go get a weapon upgrade
                        interest &n = interests.add();
                        n.state = AI_S_INTEREST;
                        n.node = entities::closestent(WAYPOINT, e.o, CLOSEDIST, true);
                        n.target = j;
                        n.targtype = AI_T_ENTITY;
                        n.score =  pos.squaredist(e.o)/(attr == d->ai->weappref ? 1e8f : (force ? 1e4f : 1e2f));
                    }
                    break;
                }
                default: break;
            }
        }

        loopvj(projs::projs) if(projs::projs[j]->projtype == PRJ_ENT && projs::projs[j]->ready())
        {
            projent &proj = *projs::projs[j];
            if(!entities::ents.inrange(proj.id) || enttype[entities::ents[proj.id]->type].usetype != EU_ITEM) continue;
            gameentity &e = *(gameentity *)entities::ents[proj.id];
            switch(e.type)
            {
                case WEAPON:
                {
                    int attr = w_attr(game::gamemode, e.attrs[0], sweap);
                    if(isweap(attr) && wantsweap(d, attr, true))
                    { // go get a weapon upgrade
                        if(proj.owner == d) break;
                        interest &n = interests.add();
                        n.state = AI_S_INTEREST;
                        n.node = entities::closestent(WAYPOINT, proj.o, CLOSEDIST, true);
                        n.target = proj.id;
                        n.targtype = AI_T_DROP;
                        n.score = pos.squaredist(proj.o)/(attr == d->ai->weappref ? 1e8f : (force ? 1e4f : 1e2f));
                    }
                    break;
                }
                default: break;
            }
        }
    }

    bool find(gameent *d, aistate &b)
    {
        static vector<interest> interests; interests.setsize(0);
        if(d->aitype == AI_BOT)
        {
            int sweap = m_weapon(game::gamemode, game::mutators);
            if(!hasweap(d, d->ai->weappref) || d->carry(sweap) == 0) items(d, b, interests, d->carry(sweap) == 0);
            if(m_fight(game::gamemode))
            {
                if(m_capture(game::gamemode)) capture::aifind(d, b, interests);
                else if(m_defend(game::gamemode)) defend::aifind(d, b, interests);
                else if(m_bomber(game::gamemode)) bomber::aifind(d, b, interests);
            }
            if(m_team(game::gamemode, game::mutators)) assist(d, b, interests, false, m_campaign(game::gamemode));
            if(m_campaign(game::gamemode) && aicampaign)
            {
                loopi(entities::lastent(TRIGGER)) if(entities::ents[i]->type == TRIGGER && entities::ents[i]->attrs[1] == TR_EXIT)
                {
                    interest &n = interests.add();
                    n.state = AI_S_PURSUE;
                    n.target = i;
                    n.node = entities::closestent(WAYPOINT, entities::ents[i]->o, CLOSEDIST, true);
                    n.targtype = AI_T_AFFINITY;
                    n.score = -1;
                }
            }
        }
        else if(entities::ents.inrange(d->aientity)) loopv(entities::ents[d->aientity]->links)
        {
            interest &n = interests.add();
            n.state = AI_S_DEFEND;
            n.target = n.node = entities::ents[d->aientity]->links[i];
            n.targtype = AI_T_NODE;
            n.score = -1;
        }
        while(!interests.empty())
        {
            int q = interests.length()-1;
            loopi(interests.length()-1) if(interests[i].score < interests[q].score) q = i;
            interest n = interests.removeunordered(q);
            bool proceed = true;
            static vector<int> targets;
            targets.setsize(0);
            if(m_fight(game::gamemode) && d->aitype == AI_BOT) switch(n.state)
            {
                case AI_S_DEFEND: // don't get into herds
                    proceed = !checkothers(targets, d, n.state, n.targtype, n.target, true);
                    break;
                default: break;
            }
            if(proceed && (!aistyle[d->aitype].canmove || makeroute(d, b, n.node, false)))
            {
                d->ai->switchstate(b, n.state, n.targtype, n.target);
                return true;
            }
        }
        return false;
    }

    void damaged(gameent *d, gameent *e, int weap, int flags, int damage)
    {
        if(d != e)
        {
            if(d->ai && (d->aitype >= AI_START || hithurts(flags) || !game::getclient(d->ai->enemy))) // see if this ai is interested in a grudge
            {
                d->ai->unsuspend();
                aistate &b = d->ai->getstate();
                violence(d, b, e, d->aitype != AI_BOT || weaptype[d->weapselect].melee);
            }
            if(d->aitype >= AI_START && e->aitype < AI_START) // alert the horde
            {
                gameent *t = NULL;
                vec dp = d->headpos();
                float maxdist = ALERTMAX*ALERTMAX;
                loopi(game::numdynents()) if((t = (gameent *)game::iterdynents(i)) && t != d && t->ai && t->state == CS_ALIVE && t->aitype >= AI_START && t->ai->suspended && targetable(t, e))
                {
                    vec tp = t->headpos();
                    if(cansee(t, tp, dp, d->aitype >= AI_START) || tp.squaredist(dp) <= maxdist)
                    {
                        aistate &c = t->ai->getstate();
                        if(violence(t, c, e, true)) return;
                    }
                }
            }
            else
            {
                static vector<int> targets; // check if one of our ai is defending them
                targets.setsize(0);
                if(checkothers(targets, d, AI_S_DEFEND, AI_T_ACTOR, d->clientnum, true))
                {
                    gameent *t;
                    loopv(targets) if((t = game::getclient(targets[i])) && t->ai && t->aitype == AI_BOT && (hithurts(flags) || !game::getclient(t->ai->enemy)) && !t->ai->suspended)
                    {
                        aistate &c = t->ai->getstate();
                        violence(t, c, e, weaptype[d->weapselect].melee);
                    }
                }
            }
        }
    }

    void setup(gameent *d, bool tryreset = false, int ent = -1)
    {
        d->aientity = ent;
        if(d->ai)
        {
            d->ai->clearsetup();
            d->ai->reset(tryreset);
            d->ai->lastrun = lastmillis;
            if(d->aitype >= AI_START)
            {
                if(entities::ents.inrange(d->aientity) && entities::ents[d->aientity]->type == ACTOR && entities::ents[d->aientity]->attrs[5] > 0)
                    d->ai->weappref = entities::ents[d->aientity]->attrs[5]-1;
                else d->ai->weappref = aistyle[d->aitype].weap;
                if(!isweap(d->ai->weappref)) d->ai->weappref = rnd(WEAP_MAX-1)+1;
            }
            else
            {
                if(m_limited(game::gamemode, game::mutators) && !m_arena(game::gamemode, game::mutators))
                    d->ai->weappref = m_weapon(game::gamemode, game::mutators);
                else if(aiforcegun >= 0 && aiforcegun < WEAP_MAX) d->ai->weappref = aiforcegun;
                else d->ai->weappref = rnd(WEAP_MAX-WEAP_OFFSET)+WEAP_OFFSET;
            }
            d->ai->suspended = true;
            vec dp = d->headpos();
            findorientation(dp, d->yaw, d->pitch, d->ai->target);
        }
    }

    void spawned(gameent *d, int ent)
    {
        if(d->ai) setup(d, false, ent);
    }

    void killed(gameent *d, gameent *e)
    {
        if(d->ai)
        {
            d->ai->reset();
            if(d->aitype >= AI_START) d->ai->suspended = true;
        }
    }

    void itemspawned(int ent, int spawned)
    {
        if(m_fight(game::gamemode) && entities::ents.inrange(ent) && entities::ents[ent]->type == WEAPON && spawned)
        {
            int sweap = m_weapon(game::gamemode, game::mutators), attr = w_attr(game::gamemode, entities::ents[ent]->attrs[0], sweap);
            loopv(game::players) if(game::players[i] && game::players[i]->ai && game::players[i]->aitype == AI_BOT)
            {
                gameent *d = game::players[i];
                if(!hasweap(d, attr) && (!hasweap(d, d->ai->weappref) || d->carry(sweap) == 0))
                {
                    aistate &b = d->ai->getstate();
                    if(b.type == AI_S_PURSUE && b.targtype == AI_T_AFFINITY) continue;
                    if(b.type == AI_S_INTEREST && (b.targtype == AI_T_ENTITY || b.targtype == AI_T_DROP))
                    {
                        if(entities::ents.inrange(b.target))
                        {
                            int weap = w_attr(game::gamemode, entities::ents[b.target]->attrs[0], sweap);
                            if((attr == d->ai->weappref && weap != d->ai->weappref) || d->o.squaredist(entities::ents[ent]->o) < d->o.squaredist(entities::ents[b.target]->o))
                                d->ai->switchstate(b, AI_S_INTEREST, AI_T_ENTITY, ent);
                        }
                        continue;
                    }
                    d->ai->switchstate(b, AI_S_INTEREST, AI_T_ENTITY, ent);
                }
            }
        }
    }

    bool check(gameent *d, aistate &b)
    {
        if(d->aitype == AI_BOT)
        {
            if(m_capture(game::gamemode) && capture::aicheck(d, b)) return true;
            else if(m_defend(game::gamemode) && defend::aicheck(d, b)) return true;
            else if(m_bomber(game::gamemode) && bomber::aicheck(d, b)) return true;
        }
        return false;
    }

    int dowait(gameent *d, aistate &b)
    {
        float maxdist = ALERTMAX*ALERTMAX;
        if(d->ai->suspended)
        { // bots idle until a human is around
            if(!m_campaign(game::gamemode) && d->aitype == AI_BOT) d->ai->unsuspend();
            else
            {
                gameent *t = NULL;
                loopi(game::numdynents()) if((t = (gameent *)game::iterdynents(i)) && t != d && t->aitype < AI_START)
                {
                    if(d->aitype == AI_BOT ? (t->state != CS_SPECTATOR && t->aitype < 0) : (t->state == CS_ALIVE && d->o.squaredist(t->o) <= maxdist))
                    {
                        d->ai->unsuspend();
                        break;
                    }
                }
            }
        }
        if(check(d, b) || find(d, b)) return 1;
        if(target(d, b, true, false, d->ai->suspended && d->aitype >= AI_START ? maxdist : 0.f)) return 1;

        if(!d->ai->suspended)
        {
            if(target(d, b, true, true)) return 1;
            if(aistyle[d->aitype].canmove && randomnode(d, b, CLOSEDIST, 1e16f))
            {
                d->ai->switchstate(b, AI_S_INTEREST, AI_T_NODE, d->ai->route[0]);
                return 1;
            }
            d->ai->suspended = true; // fine then..
            d->ai->clean(true);
        }
        return 0; // but don't pop the state
    }

    int dodefense(gameent *d, aistate &b)
    {
        if(!d->ai->suspended && d->state == CS_ALIVE)
        {
            switch(b.targtype)
            {
                case AI_T_NODE:
                case AI_T_ENTITY:
                {
                    if(check(d, b)) return 1;
                    if(entities::ents.inrange(b.target))
                    {
                        gameentity &e = *(gameentity *)entities::ents[b.target];
                        return defense(d, b, e.o) ? 1 : 0;
                    }
                    break;
                }
                case AI_T_AFFINITY:
                {
                    if(m_campaign(game::gamemode))
                    {
                        if(aicampaign && entities::ents.inrange(b.target)) return defense(d, b, entities::ents[b.target]->o) ? 1 : 0;
                    }
                    else if(m_capture(game::gamemode)) return capture::aidefense(d, b) ? 1 : 0;
                    else if(m_defend(game::gamemode)) return defend::aidefense(d, b) ? 1 : 0;
                    else if(m_bomber(game::gamemode)) return bomber::aidefense(d, b) ? 1 : 0;
                    break;
                }
                case AI_T_ACTOR:
                {
                    if(check(d, b)) return 1;
                    gameent *e = game::getclient(b.target);
                    if(e && e->state == CS_ALIVE) return defense(d, b, e->feetpos()) ? 1 : 0;
                    break;
                }
                default: break;
            }
        }
        return 0;
    }

    int dointerest(gameent *d, aistate &b)
    {
        if(!d->ai->suspended && d->state == CS_ALIVE && aistyle[d->aitype].canmove)
        {
            int sweap = m_weapon(game::gamemode, game::mutators);
            switch(b.targtype)
            {
                case AI_T_NODE:
                {
                    if(check(d, b)) return 1;
                    if(entities::ents.inrange(b.target))
                    {
                        gameentity &e = *(gameentity *)entities::ents[b.target];
                        if(vec(e.o).sub(d->feetpos()).magnitude() > CLOSEDIST) return makeroute(d, b, e.o) ? 1 : 0;
                    }
                    break;
                }
                case AI_T_ENTITY:
                {
                    if(entities::ents.inrange(b.target))
                    {
                        gameentity &e = *(gameentity *)entities::ents[b.target];
                        if(enttype[e.type].usetype != EU_ITEM) return 0;
                        int attr = w_attr(game::gamemode, e.attrs[0], sweap);
                        switch(e.type)
                        {
                            case WEAPON:
                            {
                                if(!e.spawned || !wantsweap(d, attr)) return 0;
                                float guard = enttype[e.type].radius;
                                if(d->feetpos().squaredist(e.o) <= guard*guard)
                                    b.idle = enemy(d, b, e.o, guard*4, weaptype[d->weapselect].melee, false) ? 2 : 1;
                                else b.idle = -1;
                                break;
                            }
                            default: break;
                        }
                        return makeroute(d, b, e.o) ? 1 : 0;
                    }
                    break;
                }
                case AI_T_DROP:
                {
                    loopvj(projs::projs) if(projs::projs[j]->projtype == PRJ_ENT && projs::projs[j]->ready() && projs::projs[j]->id == b.target)
                    {
                        projent &proj = *projs::projs[j];
                        if(!entities::ents.inrange(proj.id) || enttype[entities::ents[proj.id]->type].usetype != EU_ITEM) return 0;
                        gameentity &e = *(gameentity *)entities::ents[proj.id];
                        int attr = w_attr(game::gamemode, e.attrs[0], sweap);
                        switch(e.type)
                        {
                            case WEAPON:
                            {
                                if(!wantsweap(d, attr) || proj.owner == d) return 0;
                                float guard = enttype[e.type].radius;
                                if(d->feetpos().squaredist(e.o) <= guard*guard) b.idle = enemy(d, b, e.o, guard*4, weaptype[d->weapselect].melee, false) ? 2 : 1;
                                else b.idle = -1;
                                break;
                            }
                            default: break;
                        }
                        return makeroute(d, b, proj.o) ? 1 : 0;
                        break;
                    }
                    break;
                }
                default: break;
            }
        }
        return 0;
    }

    int dopursue(gameent *d, aistate &b)
    {
        if(!d->ai->suspended && d->state == CS_ALIVE)
        {
            switch(b.targtype)
            {
                case AI_T_AFFINITY:
                {
                    if(m_campaign(game::gamemode))
                    {
                        if(aicampaign && entities::ents.inrange(b.target)) return defense(d, b, entities::ents[b.target]->o) ? 1 : 0;
                    }
                    else if(m_capture(game::gamemode)) return capture::aipursue(d, b) ? 1 : 0;
                    else if(m_defend(game::gamemode)) return defend::aipursue(d, b) ? 1 : 0;
                    else if(m_bomber(game::gamemode)) return bomber::aipursue(d, b) ? 1 : 0;
                    break;
                }

                case AI_T_ACTOR:
                {
                    //if(check(d, b)) return 1;
                    gameent *e = game::getclient(b.target);
                    if(e && e->state == CS_ALIVE)
                    {
                        bool alt = altfire(d, e);
                        if(aistyle[d->aitype].canmove)
                        {
                            float mindist = weapmindist(d->weapselect, alt);
                            return patrol(d, b, e->feetpos(), mindist, ALERTMAX) ? 1 : 0;
                        }
                        else
                        {
                            vec dp = d->headpos(), ep = getaimpos(d, e, alt);
                            if(cansee(d, dp, ep, d->aitype >= AI_START) || (e->clientnum == d->ai->enemy && d->ai->enemyseen && lastmillis-d->ai->enemyseen <= (d->skill*10)+1000))
                            {
                                b.idle = -1;
                                return 1;
                            }
                            return 0;
                        }
                    }
                    break;
                }
                default: break;
            }
        }
        return 0;
    }

    int closenode(gameent *d, bool retry = false)
    {
        vec pos = d->feetpos();
        int node = -1;
        float mindist = ALERTMAX*ALERTMAX;
        loopvrev(d->ai->route) if(entities::ents.inrange(d->ai->route[i]))
        {
            gameentity &e = *(gameentity *)entities::ents[d->ai->route[i]];
            vec epos = e.o;
            int entid = obs.remap(d, d->ai->route[i], epos, retry);
            if(entities::ents.inrange(entid) && (retry || entid == d->ai->route[i] || !d->ai->hasprevnode(entid)))
            {
                float dist = epos.squaredist(pos);
                if(dist < mindist)
                {
                    node = entid;
                    mindist = dist;
                }
            }
        }
        return node;
    }

    bool wpspot(gameent *d, int n, bool retry = false)
    {
        if(entities::ents.inrange(n))
        {
            gameentity &e = *(gameentity *)entities::ents[n];
            vec epos = e.o;
            int entid = obs.remap(d, n, epos, retry);
            if(entities::ents.inrange(entid) && (retry || entid == n || !d->ai->hasprevnode(entid)))
            {
                if(entities::ents[entid]->attrs[0]&WP_F_JETPACK && !physics::canjetpack(d)) return false;
                if(!aistyle[d->aitype].canjump && epos.z-d->feetpos().z >= JUMPMIN) epos.z = d->feetpos().z;
                d->ai->spot = epos;
                d->ai->targnode = entid;
                if(aistyle[d->aitype].cancrouch && ((e.attrs[0] & WP_F_CROUCH && !d->action[AC_CROUCH]) || d->action[AC_CROUCH]) && (lastmillis-d->actiontime[AC_CROUCH] >= PHYSMILLIS*3))
                {
                    d->action[AC_CROUCH] = !d->action[AC_CROUCH];
                    d->actiontime[AC_CROUCH] = lastmillis;
                }
                return true;
            }
        }
        return false;
    }

    bool anynode(gameent *d, aistate &b, bool retry = false)
    {
        if(entities::ents.inrange(d->lastnode))
        {
            gameentity &e = *(gameentity *)entities::ents[d->lastnode];
            static vector<int> anyremap; anyremap.setsize(0);
            if(!e.links.empty())
            {
                loopv(e.links) if(entities::ents.inrange(e.links[i]) && (retry || !d->ai->hasprevnode(e.links[i])))
                    anyremap.add(e.links[i]);
            }
            while(!anyremap.empty())
            {
                int r = rnd(anyremap.length()), t = anyremap[r];
                if(wpspot(d, t, retry))
                {
                    d->ai->route.add(t);
                    d->ai->route.add(d->lastnode);
                    return true;
                }
                anyremap.remove(r);
            }
            if(!retry) return anynode(d, b, true);
        }
        return false;
    }

    bool hunt(gameent *d, aistate &b, int retries = 0)
    {
        if(!d->ai->route.empty() && d->lastnode >= 0)
        {
            int n = retries%2 ? d->ai->route.find(d->lastnode) : closenode(d, retries >= 2);
            if(retries%2 && d->ai->route.inrange(n))
            {
                while(d->ai->route.length() > n+1) d->ai->route.pop(); // waka-waka-waka-waka
                if(!n)
                {
                    if(wpspot(d, d->ai->route[n], retries >= 2))
                    {
                        d->ai->clear(false);
                        return true;
                    }
                    else if(retries <= 2) return hunt(d, b, retries+1); // try again
                }
                else n--; // otherwise, we want the next in line
            }
            if(d->ai->route.inrange(n) && wpspot(d, d->ai->route[n], retries >= 2)) return true;
            if(retries <= 2) return hunt(d, b, retries+1); // try again
        }
        b.override = false;
        d->ai->clear(false);
        return anynode(d, b);
    }

    void jumpto(gameent *d, aistate &b, const vec &pos, bool locked)
    {
        vec off = vec(pos).sub(d->feetpos());
        if(d->blocked) off.z += JUMPMIN; // it could help..
        bool offground = d->physstate == PHYS_FALL && !physics::liquidcheck(d) && !d->onladder, air = d->timeinair > 100 && !d->turnside,
            impulse = air && physics::canimpulse(d, 0, 1) && (d->timeinair > 500 || d->vel.z < 1), jet = air && physics::canjetpack(d),
            jumper = (locked || off.z >= JUMPMIN || impulse || jet || (d->aitype == AI_BOT && lastmillis >= d->ai->jumprand)) && (!offground || impulse || jet),
            jump = jumper && (impulse || jet || lastmillis >= d->ai->jumpseed);
        if(jump)
        {
            vec old = d->o;
            d->o = vec(pos).add(vec(0, 0, d->height));
            if(!collide(d, vec(0, 0, 1))) jump = false;
            d->o = old;
            if(jump)
            {
                loopi(entities::lastenttype[PUSHER]) if(entities::ents[i]->type == PUSHER)
                {
                    gameentity &e = *(gameentity *)entities::ents[i];
                    float radius = (e.attrs[3] ? e.attrs[3] : enttype[e.type].radius)*1.5f; radius *= radius;
                    if(e.o.squaredist(pos) <= radius) { jump = false; break; }
                }
            }
        }
        if(jump)
        {
            if((d->action[AC_JUMP] = jump) != false) d->actiontime[AC_JUMP] = lastmillis;
            int seed = (111-d->skill)*(locked || impulse || jet ? 1 : (d->onladder || d->inliquid ? 3 : 5));
            d->ai->jumpseed = lastmillis+seed+rnd(seed*2);
            seed *= 50; if(b.idle) seed *= 2;
            d->ai->jumprand = lastmillis+seed+rnd(seed*2);
        }
        if(!jump && air && physics::canimpulse(d, -1, 3)) d->action[AC_SPECIAL] = true;
    }

    bool lockon(gameent *d, gameent *e, float maxdist, bool check)
    {
        if(check && !d->blocked)
        {
            vec dir = vec(e->o).sub(d->o);
            float xydist = dir.x*dir.x+dir.y*dir.y, zdist = dir.z*dir.z, mdist = maxdist*maxdist, ddist = d->radius*d->radius+e->radius*e->radius;
            if(zdist <= ddist && xydist >= ddist+4 && xydist <= mdist+ddist) return true;
        }
        return false;
    }

    int process(gameent *d, aistate &b)
    {
        int result = 0, stupify = d->skill <= 10+rnd(15) ? rnd(d->skill*1000) : 0, skmod = 101-d->skill;
        float frame = d->skill <= 100 ? float(lastmillis-d->ai->lastrun)/float(max(skmod,1)*10) : 1;
        if(!aistyle[d->aitype].canstrafe && d->skill <= 100) frame *= 2;
        vec dp = d->headpos();

        bool wasdontmove = d->ai->dontmove, idle = b.idle == 1 || (stupify && stupify <= skmod) || d->ai->suspended;
        d->action[AC_SPECIAL] = d->ai->dontmove = false;
        if(idle || !aistyle[d->aitype].canmove)
        {
            d->ai->lasthunt = lastmillis;
            if(aistyle[d->aitype].canmove) d->ai->dontmove = true;
        }
        else if(hunt(d, b))
        {
            game::getyawpitch(dp, vec(d->ai->spot).add(vec(0, 0, d->height)), d->ai->targyaw, d->ai->targpitch);
            d->ai->lasthunt = lastmillis;
        }
        else idle = d->ai->dontmove = true;

        gameent *e = game::getclient(d->ai->enemy);
        bool enemyok = e && targetable(d, e), locked = false,
             melee = weaptype[d->weapselect].melee || d->aitype == AI_BOT;
        if(!enemyok || d->skill >= 50)
        {
            gameent *f = game::intersectclosest(dp, d->ai->target, d);
            if(f)
            {
                if(targetable(d, f))
                {
                    if(!enemyok) violence(d, b, f, weaptype[d->weapselect].melee);
                    enemyok = true;
                    e = f;
                }
                else enemyok = false;
            }
            else if(!enemyok && target(d, b, weaptype[d->weapselect].melee, false, ALERTMIN))
                enemyok = (e = game::getclient(d->ai->enemy)) != NULL;
        }
        if(enemyok)
        {
            bool alt = altfire(d, e);
            vec ep = getaimpos(d, e, alt);
            float yaw, pitch;
            game::getyawpitch(dp, ep, yaw, pitch);
            game::fixrange(yaw, pitch);
            bool insight = cansee(d, dp, ep), hasseen = d->ai->enemyseen && lastmillis-d->ai->enemyseen <= (d->skill*10)+3000,
                quick = d->ai->enemyseen && lastmillis-d->ai->enemyseen <= (WEAP2(d->weapselect, fullauto, alt) ? WEAP2(d->weapselect, adelay, alt)*3 : skmod)+30;
            if(insight) d->ai->enemyseen = lastmillis;
            if(idle || insight || hasseen || quick)
            {
                float sskew = insight || d->skill > 100 ? 1.5f : (hasseen ? 1.f : 0.5f);
                if(insight && lockon(d, e, aistyle[d->aitype].canstrafe ? 32 : 16, melee))
                {
                    d->ai->targyaw = yaw;
                    d->ai->targpitch = pitch;
                    frame *= 2;
                    if(d->aitype == AI_BOT)
                    {
                        locked = true;
                        if(!weaptype[d->weapselect].melee) d->action[AC_SPECIAL] = true;
                    }
                }
                game::scaleyawpitch(d->yaw, d->pitch, yaw, pitch, frame, sskew);
                if(insight || quick)
                {
                    bool shoot = canshoot(d, e, alt);
                    if(d->action[alt ? AC_ALTERNATE : AC_ATTACK] && WEAP2(d->weapselect, power, alt)) switch(WEAP2(d->weapselect, cooked, alt))
                    {
                        case 2: case 3: shoot = false; break; // shorter
                        case 1: case 4: case 5: default: break;
                    }
                    if(shoot && hastarget(d, b, e, alt, yaw, pitch, dp.squaredist(ep)))
                    {
                        d->action[alt ? AC_ALTERNATE : AC_ATTACK] = true;
                        d->actiontime[alt ? AC_ALTERNATE : AC_ATTACK] = lastmillis;
                        result = 3;
                    }
                    else result = 2;
                }
                else result = 1;
            }
            else
            {
                if(!d->ai->enemyseen || lastmillis-d->ai->enemyseen > (d->skill*50)+3000)
                {
                    d->ai->enemy = -1;
                    d->ai->enemyseen = d->ai->enemymillis = 0;
                }
                enemyok = false;
                result = 0;
            }
        }
        else
        {
            if(!enemyok)
            {
                d->ai->enemy = -1;
                d->ai->enemyseen = d->ai->enemymillis = 0;
            }
            enemyok = false;
            result = 0;
        }
        if(result < 3) d->action[AC_ATTACK] = d->action[AC_ALTERNATE] = false;

        game::fixrange(d->ai->targyaw, d->ai->targpitch);
        d->aimyaw = d->ai->targyaw;
        d->aimpitch = d->ai->targpitch;
        if(!result) game::scaleyawpitch(d->yaw, d->pitch, d->ai->targyaw, d->ai->targpitch, frame*0.25f, 1.f);

        if(aistyle[d->aitype].canjump && (!d->ai->dontmove || b.idle)) jumpto(d, b, d->ai->spot, locked);
        if(d->aitype == AI_BOT)
        {
            if(idle)
            {
                bool wascrouching = lastmillis-d->actiontime[AC_CROUCH] <= PHYSMILLIS*2, wantscrouch = d->ai->dontmove && !wasdontmove && !d->action[AC_CROUCH];
                if(wascrouching || wantscrouch)
                {
                    d->action[AC_CROUCH] = true;
                    if(wantscrouch) d->actiontime[AC_CROUCH] = lastmillis;
                }
            }

            bool wantsimpulse = false;
            if(physics::allowimpulse(m_jetpack(game::gamemode, game::mutators) ? 0 : 2))
            {
                if(!impulsemeter || impulsesprint == 0 || impulseregensprint > 0) wantsimpulse = true;
                else if(b.idle == -1 && !d->ai->dontmove)
                    wantsimpulse = (d->action[AC_SPRINT] || !d->actiontime[AC_SPRINT] || lastmillis-d->actiontime[AC_SPRINT] > PHYSMILLIS*2);
            }

            if(d->ai->lastpusher >= 0 && d->physstate == PHYS_FALL && lastmillis-d->ai->lastpushtime < (skmod/10)*250)
            {
                float offyaw, offpitch;
                vec v = vec(d->vel).normalize();
                vectoyawpitch(v, offyaw, offpitch);
                offyaw -= d->aimyaw;
                if(offyaw > 180) offyaw -= 360;
                else if(offyaw < -180) offyaw += 360;
                offpitch -= d->aimpitch;
                if(fabs(offyaw)+fabs(offpitch) >= 135)
                {
                    wantsimpulse = false;
                    d->ai->lastpusher = -1;
                    d->ai->lastpushtime = 0;
                }
                else
                {
                    d->ai->dontmove = true;
                    wantsimpulse = false;
                }
            }
            else
            {
                d->ai->lastpusher = -1;
                d->ai->lastpushtime = 0;
            }

            if(d->action[AC_SPRINT] != wantsimpulse)
                if((d->action[AC_SPRINT] = !d->action[AC_SPRINT]) == true) d->actiontime[AC_SPRINT] = lastmillis;
        }

        if(d->ai->dontmove || (d->aitype >= AI_START && lastmillis-d->lastpain <= PHYSMILLIS/3)) d->move = d->strafe = 0;
        else if(!aistyle[d->aitype].canmove || !aistyle[d->aitype].canstrafe)
        {
            d->aimyaw = d->yaw;
            d->aimpitch = d->pitch;
            d->move = aistyle[d->aitype].canmove ? 1 : 0;
            d->strafe = 0;
        }
        else
        { // our guys move one way.. but turn another?! :)
            const struct aimdir { int move, strafe, offset; } aimdirs[8] =
            {
                {  1,  0,   0 },
                {  1,  -1,  45 },
                {  0,  -1,  90 },
                { -1,  -1,  135 },
                { -1,  0,   180 },
                { -1,  1,   225 },
                {  0,  1,   270 },
                {  1,  1,   315 }
            };
            float yaw = d->aimyaw-d->yaw;
            while(yaw < 0.0f) yaw += 360.0f;
            while(yaw >= 360.0f) yaw -= 360.0f;
            const aimdir &ad = aimdirs[clamp(((int)floor((yaw+22.5f)/45.0f))&7, 0, 7)];
            d->move = ad.move;
            d->strafe = ad.strafe;
            d->aimyaw -= ad.offset;
        }
        if(!aistyle[d->aitype].canstrafe && d->move && enemyok && lockon(d, e, 8, melee)) d->move = 0;
        game::fixrange(d->aimyaw, d->aimpitch);
        findorientation(dp, d->yaw, d->pitch, d->ai->target);
        return result;
    }

    bool hasrange(gameent *d, gameent *e, int weap)
    {
        if(!e) return true;
        if(targetable(d, e))
        {
            loopk(2)
            {
                vec ep = getaimpos(d, e, k!=0);
                float dist = ep.squaredist(d->headpos());
                if(weaprange(d, weap, k!=0, dist)) return true;
            }
        }
        return false;
    }

    bool request(gameent *d, aistate &b)
    {
        int busy = process(d, b), sweap = m_weapon(game::gamemode, game::mutators);
        bool haswaited = d->weapwaited(d->weapselect, lastmillis, d->skipwait(d->weapselect, 0, lastmillis, (1<<WEAP_S_RELOAD), true));
        if(d->aitype == AI_BOT)
        {
            if(b.idle && busy <= 1 && d->carry(sweap, 1) > 1 && d->weapstate[d->weapselect] != WEAP_S_WAIT)
            {
                loopirev(WEAP_ITEM) if(i >= WEAP_OFFSET && i != d->ai->weappref && i != d->weapselect && entities::ents.inrange(d->entid[i]))
                {
                    client::addmsg(N_DROP, "ri3", d->clientnum, lastmillis-game::maptime, i);
                    d->setweapstate(d->weapselect, WEAP_S_WAIT, WEAPSWITCHDELAY, lastmillis);
                    d->ai->lastaction = lastmillis;
                    return true;
                }
            }
            if(busy <= 2 && !d->action[AC_USE] && haswaited)
            {
                static vector<actitem> actitems;
                actitems.setsize(0);
                if(entities::collateitems(d, actitems))
                {
                    while(!actitems.empty())
                    {
                        actitem &t = actitems.last();
                        int ent = -1;
                        switch(t.type)
                        {
                            case ITEM_ENT:
                            {
                                if(!entities::ents.inrange(t.target)) break;
                                extentity &e = *entities::ents[t.target];
                                if(enttype[e.type].usetype != EU_ITEM) break;
                                ent = t.target;
                                break;
                            }
                            case ITEM_PROJ:
                            {
                                if(!projs::projs.inrange(t.target)) break;
                                projent &proj = *projs::projs[t.target];
                                if(!entities::ents.inrange(proj.id)) break;
                                extentity &e = *entities::ents[proj.id];
                                if(enttype[e.type].usetype != EU_ITEM || proj.owner == d) break;
                                ent = proj.id;
                                break;
                            }
                            default: break;
                        }
                        if(entities::ents.inrange(ent))
                        {
                            extentity &e = *entities::ents[ent];
                            int attr = e.type == WEAPON ? w_attr(game::gamemode, e.attrs[0], sweap) : e.attrs[0];
                            if(d->canuse(e.type, attr, e.attrs, sweap, lastmillis, (1<<WEAP_S_RELOAD)|(1<<WEAP_S_SWITCH))) switch(e.type)
                            {
                                case WEAPON:
                                {
                                    if(!wantsweap(d, attr)) break;
                                    d->action[AC_USE] = true;
                                    d->ai->lastaction = d->actiontime[AC_USE] = lastmillis;
                                    return true;
                                }
                                default: break;
                            }
                        }
                        actitems.pop();
                    }
                }
            }
        }

        bool timepassed = d->weapstate[d->weapselect] == WEAP_S_IDLE && (!d->ammo[d->weapselect] || lastmillis-d->weaplast[d->weapselect] >= 6000-(d->skill*50));
        if(busy <= 2 && haswaited && timepassed)
        {
            int weap = d->ai->weappref;
            gameent *e = game::getclient(d->ai->enemy);
            if(!isweap(weap) || !d->hasweap(weap, sweap) || !hasrange(d, e, weap))
            {
                loopirev(WEAP_MAX) if(i >= WEAP_MELEE && d->hasweap(i, sweap) && hasrange(d, e, i)) { weap = i; break; }
            }
            if(isweap(weap) && weap != d->weapselect && weapons::weapselect(d, weap))
            {
                d->ai->lastaction = lastmillis;
                return true;
            }
        }

        if(d->hasweap(d->weapselect, sweap) && busy <= (!d->ammo[d->weapselect] ? 2 : 0) && timepassed)
        {
            if(weapons::weapreload(d, d->weapselect))
            {
                d->ai->lastaction = lastmillis;
                return true;
            }
        }

        return busy >= 1;
    }

    void timeouts(gameent *d, aistate &b)
    {
        if(d->blocked)
        {
            d->ai->blocktime += lastmillis-d->ai->lastrun;
            if(d->ai->blocktime > (d->ai->blockseq+1)*500)
            {
                switch(d->ai->blockseq)
                {
                    case 1: case 2: case 3:
                        if(entities::ents.inrange(d->ai->targnode)) d->ai->addprevnode(d->ai->targnode);
                        d->ai->clear(false);
                        break;
                    case 4: d->ai->reset(true); break;
                    case 5: d->ai->reset(false); break;
                    case 6: game::suicide(d, HIT_LOST); return; break; // this is our last resort..
                    case 0: default: break;
                }
                d->ai->blockseq++;
            }
        }
        else d->ai->blocktime = d->ai->blockseq = 0;

        if(d->ai->targnode == d->ai->targlast)
        {
            d->ai->targtime += lastmillis-d->ai->lastrun;
            if(d->ai->targtime > (d->ai->targseq+1)*1000)
            {
                switch(d->ai->targseq)
                {
                    case 1: case 2: case 3:
                        if(entities::ents.inrange(d->ai->targnode)) d->ai->addprevnode(d->ai->targnode);
                        d->ai->clear(false);
                        break;
                    case 4: d->ai->reset(true); break;
                    case 5: d->ai->reset(false); break;
                    case 6: game::suicide(d, HIT_LOST); return; break; // this is our last resort..
                    case 0: default: break;
                }
                d->ai->targseq++;
            }
        }
        else
        {
            d->ai->targtime = d->ai->targseq = 0;
            d->ai->targlast = d->ai->targnode;
        }

        if(d->ai->lasthunt)
        {
            int millis = lastmillis-d->ai->lasthunt;
            if(millis <= 3000) { d->ai->tryreset = false; d->ai->huntseq = 0; }
            else if(millis > (d->ai->huntseq+1)*3000)
            {
                switch(d->ai->huntseq)
                {
                    case 0: d->ai->reset(true); break;
                    case 1: d->ai->reset(false); break;
                    case 2: game::suicide(d, HIT_LOST); return; break; // this is our last resort..
                    default: break;
                }
                d->ai->huntseq++;
            }
        }
    }

    #define issuspended(a) (a->ai->suspended || (a->aitype >= AI_START ? enemyspeed == 0 : botspeed == 0))

    void logic(gameent *d, aistate &b, bool run)
    {
        if(!issuspended(d))
        {
            vec dp = d->headpos();
            if(d->state != CS_ALIVE || !game::allowmove(d)) d->stopmoving(true);
            else
            {
                if(!request(d, b)) target(d, b, weaptype[d->weapselect].melee, false, ALERTMIN);
                weapons::shoot(d, d->ai->target);
            }
        }
        if(d->state == CS_DEAD || d->state == CS_WAITING)
        {
            if(d->ragdoll) moveragdoll(d, true);
            else if(lastmillis-d->lastpain < 5000)
                physics::move(d, 1, false);
        }
        else
        {
            if(d->ragdoll) cleanragdoll(d);
            if(d->state == CS_ALIVE && !game::intermission)
            {
                if(!issuspended(d))
                {
                    bool ladder = d->onladder;
                    physics::move(d, 1, true);
                    if(aistyle[d->aitype].canmove && !b.idle) timeouts(d, b);
                    if(!ladder && d->onladder) d->ai->jumpseed = lastmillis;
                    entities::checkitems(d);
                }
                else
                {
                    d->move = d->strafe = 0;
                    physics::move(d, 1, true);
                }
            }
        }
    }

    void avoid()
    {
        obs.clear();
        loopi(game::numdynents())
        {
            gameent *d = (gameent *)game::iterdynents(i);
            if(!d) continue; // || d->aitype >= AI_START) continue;
            if(!d->ai && !d->airnodes.empty()) loopvj(d->airnodes)
                if(entities::ents.inrange(d->airnodes[j]) && entities::ents[d->airnodes[j]]->type == WAYPOINT)
                    obs.add(d, d->airnodes[j]);
            if(d->state != CS_ALIVE || !physics::issolid(d)) continue;
            obs.avoidnear(d, d->feetpos(), d->radius+2);
        }
        obs.add(wps);
        loopv(projs::projs)
        {
            projent *p = projs::projs[i];
            if(p && p->state == CS_ALIVE && p->projtype == PRJ_SHOT && WEAPEX(p->weap, p->flags&HIT_ALT, game::gamemode, game::mutators, p->curscale))
                obs.avoidnear(p, p->o, (WEAPEX(p->weap, p->flags&HIT_ALT, game::gamemode, game::mutators, p->curscale)*p->lifesize)+2);
        }
        loopi(entities::lastenttype[MAPMODEL]) if(entities::ents[i]->type == MAPMODEL && !entities::ents[i]->links.empty() && !entities::ents[i]->spawned)
        {
            mapmodelinfo &mmi = getmminfo(entities::ents[i]->attrs[0]);
            if(!&mmi) continue;
            vec center, radius;
            mmi.m->collisionbox(0, center, radius);
            if(entities::ents[i]->attrs[4]) { center.mul(entities::ents[i]->attrs[4]/100.f); radius.mul(entities::ents[i]->attrs[4]/100.f); }
            if(!mmi.m->ellipsecollide) rotatebb(center, radius, int(entities::ents[i]->attrs[1]));
            float limit = enttype[WAYPOINT].radius+(max(radius.x, max(radius.y, radius.z))*mmi.m->height);
            vec pos = entities::ents[i]->o; pos.z += limit*0.5f;
            obs.avoidnear(NULL, pos, limit);
        }
    }

    void think(gameent *d, bool run)
    {
        // the state stack works like a chain of commands, certain commands simply replace each other
        // others spawn new commands to the stack the ai reads the top command from the stack and executes
        // it or pops the stack and goes back along the history until it finds a suitable command to execute
        if(d->ai->state.empty()) setup(d, false, d->aientity);
        bool cleannext = false, parse = run && entities::ents.inrange(d->lastnode);
        loopvrev(d->ai->state)
        {
            aistate &c = d->ai->state[i];
            if(cleannext)
            {
                c.millis = lastmillis;
                c.override = false;
                cleannext = false;
            }
            if(d->state == CS_DEAD)
            {
                if(d->respawned < 0 && (!d->lastdeath || lastmillis-d->lastdeath > (d->aitype == AI_BOT ? 500 : enemyspawntime)))
                {
                    if(d->aitype == AI_BOT && m_arena(game::gamemode, game::mutators))
                    {
                        d->loadweap[0] = d->ai->weappref; d->loadweap[1] = WEAP_PISTOL;
                        client::addmsg(N_LOADWEAP, "ri3", d->clientnum, d->loadweap[0], d->loadweap[1]);
                    }
                    client::addmsg(N_TRYSPAWN, "ri", d->clientnum);
                    d->respawned = lastmillis;
                }
            }
            else if(d->state == CS_ALIVE && parse)
            {
                int result = 0;
                c.idle = 0;
                switch(c.type)
                {
                    case AI_S_WAIT: result = dowait(d, c); break;
                    case AI_S_DEFEND: result = dodefense(d, c); break;
                    case AI_S_PURSUE: result = dopursue(d, c); break;
                    case AI_S_INTEREST: result = dointerest(d, c); break;
                    default: result = 0; break;
                }
                if(result <= 0)
                {
                    d->ai->clear(true);
                    if(c.type != AI_S_WAIT)
                    {
                        switch(result)
                        {
                            case 0: default: d->ai->removestate(i); cleannext = true; break;
                            case -1: i = d->ai->state.length()-1; break;
                        }
                        continue; // shouldn't interfere
                    }
                }
                else if(d->ai->suspended) d->ai->unsuspend();
            }
            logic(d, c, parse);
            break;
        }
        if(d->ai->trywipe) d->ai->wipe();
        d->ai->lastrun = lastmillis;
    }

    void drawstate(gameent *d, aistate &b, bool top, int above)
    {
        const char *bnames[AI_S_MAX] = {
            "wait", "defend", "pursue", "interest"
        }, *btypes[AI_T_MAX+1] = {
            "none", "node", "actor", "affinity", "entity", "drop"
        };
        mkstring(s);
        if(top)
        {
            formatstring(s)("<default>\fg%s (%s) %s:%d (%d[%d])",
                bnames[b.type],
                hud::timetostr(lastmillis-b.millis),
                btypes[b.targtype+1], b.target,
                !d->ai->route.empty() ? d->ai->route[0] : -1,
                d->ai->route.length()
            );
        }
        else
        {
            formatstring(s)("<sub>\fy%s (%d) %s:%d",
                bnames[b.type],
                hud::timetostr(lastmillis-b.millis),
                btypes[b.targtype+1], b.target
            );
        }
        if(s[0]) part_textcopy(vec(d->abovehead()).add(vec(0, 0, above)), s);
     }

    void drawroute(gameent *d, aistate &b, float amt)
    {
        int colour = d->colour(), last = -1;
        loopvrev(d->ai->route)
        {
            if(d->ai->route.inrange(last))
            {
                int index = d->ai->route[i], prev = d->ai->route[last];
                if(entities::ents.inrange(index) && entities::ents.inrange(prev))
                {
                    gameentity &e = *(gameentity *)entities::ents[index], &f = *(gameentity *)entities::ents[prev];
                    vec fr = f.o, dr = e.o;
                    fr.z += amt; dr.z += amt;
                    part_trace(fr, dr, 1, 1, 1, colour);
                }
            }
            last = i;
        }
        if(aidebug > 4)
        {
            vec fr = vec(d->feetpos()).add(vec(0, 0, amt));
            if(d->ai->spot != vec(0, 0, 0)) part_trace(fr, vec(d->ai->spot).add(vec(0, 0, 0.1f)), 1, 1, 1, 0x008888);
            if(entities::ents.inrange(d->lastnode))
            {
                vec dr = vec(entities::ents[d->lastnode]->o).add(vec(0, 0, amt));
                part_trace(fr, dr, 1, 1, 1, 0x884400);
                fr = dr;
            }
            loopi(NUMPREVNODES)
            {
                if(entities::ents.inrange(d->ai->prevnodes[i]))
                {
                    vec dr = vec(entities::ents[d->ai->prevnodes[i]]->o).add(vec(0, 0, amt));
                    part_trace(dr, fr, 1, 1, 1, 0x442200);
                    fr = dr;
                }
            }
        }
    }

    void render()
    {
        if(aidebug > 1)
        {
            int amt[2] = { 0, 0 };
            loopv(game::players) if(game::players[i] && game::players[i]->ai) amt[0]++;
            loopv(game::players) if(game::players[i] && game::players[i]->state == CS_ALIVE && game::players[i]->ai)
            {
                gameent *d = game::players[i];
                bool top = true;
                int above = 0;
                amt[1]++;
                loopvrev(d->ai->state)
                {
                    aistate &b = d->ai->state[i];
                    drawstate(d, b, top, above += 2);
                    if(aidebug > 3 && top && rendernormally && b.type != AI_S_WAIT && aistyle[d->aitype].canmove)
                        drawroute(d, b, 4.f*(float(amt[1])/float(amt[0])));
                    if(top)
                    {
                        if(aidebug > 2) top = false;
                        else break;
                    }
                }
                if(aidebug > 2)
                {
                    if(isweap(d->ai->weappref))
                        part_textcopy(vec(d->abovehead()).add(vec(0, 0, above += 2)), weaptype[d->ai->weappref].name);
                    gameent *e = game::getclient(d->ai->enemy);
                    if(e) part_textcopy(vec(d->abovehead()).add(vec(0, 0, above += 2)), game::colorname(e, NULL, "<default>"));
                }
            }
            if(aidebug >= 4 && !m_edit(game::gamemode))
            {
                int cur = 0;
                loopv(obs.obstacles)
                {
                    const entities::avoidset::obstacle &ob = obs.obstacles[i];
                    int next = cur + ob.numentities;
                    for(; cur < next; cur++)
                    {
                        int ent = obs.entities[cur];
                        gameentity &e = *(gameentity *)entities::ents[ent];
                        part_create(PART_EDIT, 1, e.o, 0xFF6600, 1.5f);
                    }
                    cur = next;
                }
            }
        }
    }

    void preload()
    {
        loopi(AI_TOTAL)
        {
            loadmodel(aistyle[i+AI_START].tpmdl, -1, true);
            loadmodel(aistyle[i+AI_START].fpmdl, -1, true);
        }
    }
}
