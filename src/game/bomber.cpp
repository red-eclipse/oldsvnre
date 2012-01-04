#include "game.h"
namespace bomber
{
    bomberstate st;

    bool carryaffinity(gameent *d)
    {
        loopv(st.flags) if(st.flags[i].owner == d) return true;
        return false;
    }

    int findtarget(gameent *d)
    {
        float bestdist = 1e16f;
        gameent *e = NULL;
        int best = -1;
        vec targ;
        int numdyns = game::numdynents();
        loopk(d->aitype != AI_NONE ? 4 : 2)
        {
            loopi(numdyns) if((e = (gameent *)game::iterdynents(i)) && e->team == d->team && e->state == CS_ALIVE && (k%2 ? d->aitype == AI_BOT : d->aitype == AI_NONE))
            {
                float fx = k >= 2 ? 360 : (d->ai ? d->ai->views[0] : curfov), fy = k >= 2 ? 360 : (d->ai ? d->ai->views[1] : fovy);
                if(getsight(d->o, d->yaw, d->pitch, e->o, targ, bestdist, fx, fy))
                {
                    vec dir = vec(e->o).sub(d->o).normalize();
                    float yaw, pitch; vectoyawpitch(dir, yaw, pitch);
                    while(yaw < 0) yaw += 360; while(yaw >= 360) yaw -= 360;
                    float dist = fabs(d->yaw-yaw);
                    if(dist < bestdist)
                    {
                        best = e->clientnum;
                        bestdist = dist;
                    }
                }
            }
            if(best >= 0) break;
        }
        return best;
    }

    bool dropaffinity(gameent *d)
    {
        if(carryaffinity(d) && (d->action[AC_AFFINITY] || d->actiontime[AC_AFFINITY] > 0))
        {
            if(d->action[AC_AFFINITY]) return true;
            vec inertia;
            vecfromyawpitch(d->yaw, d->pitch, 1, 0, inertia);
            inertia.normalize().mul(bomberspeed).add(vec(d->vel).add(d->falling).mul(bomberrelativity));
            bool guided = m_team(game::gamemode, game::mutators) && bomberlockondelay && lastmillis-d->actiontime[AC_AFFINITY] >= bomberlockondelay;
            client::addmsg(N_DROPAFFIN, "ri8", d->clientnum, guided ? findtarget(d) : -1, int(d->o.x*DMF), int(d->o.y*DMF), int(d->o.z*DMF), int(inertia.x*DMF), int(inertia.y*DMF), int(inertia.z*DMF));
            d->action[AC_AFFINITY] = false;
            d->actiontime[AC_AFFINITY] = 0;
            return true;
        }
        return false;
    }

    void preload()
    {
        loadmodel("ball", -1, true);
    }

    vec pulsecolour()
    {
        uint n = lastmillis/100;
        return vec::hexcolor(pulsecols[2][n%PULSECOLOURS]).lerp(vec::hexcolor(pulsecols[2][(n+1)%PULSECOLOURS]), (lastmillis%100)/100.0f);
    }

    void drawblips(int w, int h, float blend)
    {
        static vector<int> hasbombs; hasbombs.setsize(0);
        loopv(st.flags) if(st.flags[i].owner == game::focus) hasbombs.add(i);
        loopv(st.flags)
        {
            bomberstate::flag &f = st.flags[i];
            if(!entities::ents.inrange(f.ent) || hasbombs.find(i) >= 0 || !f.enabled) continue;
            vec pos = f.pos(false), dir = vec(pos).sub(camera1->o), colour = isbomberaffinity(f) ? pulsecolour() : vec::hexcolor(TEAM(f.team, colour));
            float area = 3, size = hud::radaraffinitysize;
            if(isbomberaffinity(f))
            {
                area = 2;
                if(!f.owner && !f.droptime)
                {
                    int millis = lastmillis-f.interptime;
                    if(millis < 1000) size *= 1.f+(1-clamp(float(millis)/1000.f, 0.f, 1.f));
                }
            }
            else if(!m_gsp2(game::gamemode, game::mutators))
            {
                area = 3;
                if(isbombertarg(f, game::focus->team) && !hasbombs.empty()) size *= 1.25f;
            }
            hud::drawblip(isbomberaffinity(f) ? hud::bombtex : (isbombertarg(f, game::focus->team) ? hud::arrowtex : hud::flagtex), area, w, h, size, blend*hud::radaraffinityblend, (isbombertarg(f, game::focus->team) ? -1-hud::radarstyle : hud::radarstyle), (isbombertarg(f, game::focus->team) ? dir : pos), colour);
        }
    }

    void drawnotices(int w, int h, int &tx, int &ty, float blend)
    {
        if(game::player1->state == CS_ALIVE && hud::shownotices >= 3)
        {
            loopv(st.flags)
            {
                bomberstate::flag &f = st.flags[i];
                if(f.owner == game::player1)
                {
                    pushfont("emphasis");
                    ty += draw_textx("You have: \fs\f[%d]\f(%s)bomb\fS", tx, ty, 255, 255, 255, int(255*blend), TEXT_CENTERED, -1, -1, pulsecols[2][clamp((lastmillis/100)%PULSECOLOURS, 0, PULSECOLOURS-1)], hud::bombtex)*hud::noticescale;
                    popfont();
                    if(bombercarrytime)
                    {
                        int delay = bombercarrytime-(lastmillis-f.taketime);
                        pushfont("default");
                        ty += draw_textx("Explodes in \fs\fzgy%s\fS", tx, ty, 255, 255, 255, int(255*blend), TEXT_CENTERED, -1, -1, hud::timetostr(delay, -1))*hud::noticescale;
                        popfont();
                    }
                    SEARCHBINDCACHE(altkey)("action 9", 0);
                    pushfont("reduced");
                    ty += draw_textx("Press \fs\fc%s\fS to throw", tx, ty, 255, 255, 255, int(255*blend), TEXT_CENTERED, -1, -1, altkey)*hud::noticescale;
                    popfont();
                    break;
                }
            }
        }
    }

    int drawinventory(int x, int y, int s, int m, float blend)
    {
        int sy = 0;
        loopv(st.flags) if(isbomberaffinity(st.flags[i]))
        {
            if(y-sy-s < m) break;
            bomberstate::flag &f = st.flags[i];
            if(!entities::ents.inrange(f.ent) || !f.enabled) continue;
            int millis = lastmillis-f.interptime;
            vec colour = pulsecolour();
            float skew = hud::inventoryskew;
            if(f.owner || f.droptime)
            {
                if(f.owner == game::focus)
                {
                    if(millis <= 1000) skew += (1.f-skew)*clamp(float(millis)/1000.f, 0.f, 1.f);
                    else skew = 1; // override it
                }
                else if(millis <= 1000) skew += ((1.f-skew)*clamp(float(millis)/1000.f, 0.f, 1.f));
                else skew = 1;
            }
            else if(millis <= 1000) skew += ((1.f-skew)-(clamp(float(millis)/1000.f, 0.f, 1.f)*(1.f-skew)));
            int oldy = y-sy;
            sy += hud::drawitem(hud::bombtex, x, oldy, s, true, false, colour.x, colour.y, colour.z, blend, skew);
            if(f.owner)
            {
               int qy = hud::drawitemsubtext(x, oldy, s, false, skew, "reduced", blend, "%s", f.owner->team == f.team ? "\fgsecured" : "\fytaken");
               hud::drawitemsubtext(x, oldy-qy, s, false, skew, "reduced", blend, "%s", game::colorname(f.owner));
            }
            else hud::drawitemsubtext(x, oldy, s, false, skew, "reduced", blend, "%s", f.droptime ? "\fcdropped" : "");
            if(f.droptime || (f.owner && bombercarrytime))
            {
                int sx = x-int(s*skew);
                float wait = f.droptime ? clamp((lastmillis-f.droptime)/float(bomberresetdelay), 0.f, 1.f) : clamp((lastmillis-f.taketime)/float(bombercarrytime), 0.f, 1.f);
                if(wait > 0.5f)
                {
                    int delay = wait > 0.7f ? (wait > 0.85f ? 150 : 300) : 600, millis = lastmillis%(delay*2);
                    float amt = (millis <= delay ? millis/float(delay) : 1.f-((millis-delay)/float(delay)));
                    flashcolour(colour.r, colour.g, colour.b, 1.f, 0.f, 0.f, amt);
                }
                if(wait < 1) hud::drawprogress(sx, oldy, wait, 1-wait, s, false, colour.r, colour.g, colour.b, blend*0.25f, skew);
                hud::drawprogress(sx, oldy, 0, wait, s, false, colour.r, colour.g, colour.b, blend, skew, "super", "%d%%", int(wait*100.f));
            }
            if(f.owner)
            {
                if(f.owner == game::focus && m_team(game::gamemode, game::mutators) && bomberlockondelay && f.owner->action[AC_AFFINITY] && lastmillis-f.owner->actiontime[AC_AFFINITY] >= bomberlockondelay)
                {
                    gameent *e = game::getclient(findtarget(f.owner));
                    float cx = 0.5f, cy = 0.5f, cz = 1;
                    if(e && vectocursor(e->headpos(), cx, cy, cz))
                    {
                        int interval = lastmillis%500;
                        float rp = 1, gp = 1, bp = 1,
                              sp = interval >= 250 ? (500-interval)/250.f : interval/250.f,
                              sq = max(sp, 0.5f);
                        hud::colourskew(rp, gp, bp, sp);
                        int sx = int(cx*hud::hudwidth-s*sq), sy = int(cy*hud::hudsize-s*sq), ss = int(s*2*sq);
                        Texture *t = textureload(hud::indicatortex, 3);
                        if(t && t != notexture)
                        {
                            glBindTexture(GL_TEXTURE_2D, t->id);
                            glColor4f(rp, gp, bp, sq);
                            hud::drawsized(sx, sy, ss);
                        }
                        t = textureload(hud::crosshairtex, 3);
                        if(t && t != notexture)
                        {
                            glBindTexture(GL_TEXTURE_2D, t->id);
                            glColor4f(rp, gp, bp, sq*0.5f);
                            hud::drawsized(sx+ss/4, sy+ss/4, ss/2);
                        }
                    }
                }
            }
        }
        return sy;
    }

    void checkcams(vector<cament *> &cameras)
    {
        loopv(st.flags) // flags/bases
        {
            bomberstate::flag &f = st.flags[i];
            if(!entities::ents.inrange(f.ent)) continue;
            cament *c = cameras.add(new cament);
            c->o = f.pos(false);
            c->o.z += enttype[AFFINITY].radius/2;
            c->type = cament::AFFINITY;
            c->id = i;
        }
    }

    void updatecam(cament *c)
    {
        switch(c->type)
        {
            case cament::AFFINITY:
            {
                if(st.flags.inrange(c->id))
                {
                    bomberstate::flag &f = st.flags[c->id];
                    c->o = f.pos(false);
                    c->o.z += enttype[AFFINITY].radius/2;
                    if(f.owner) c->player = f.owner;
                }
                break;
            }
        }
    }

    void render()
    {
        loopv(st.flags) // flags/bases
        {
            bomberstate::flag &f = st.flags[i];
            if(!entities::ents.inrange(f.ent) || !f.enabled || (f.owner == game::focus && !game::thirdpersonview(true))) continue;
            vec above(f.pos());
            float trans = isbomberaffinity(f) ? 1.f : 0.5f;
            if(!isbomberaffinity(f) || (!f.droptime && !f.owner))
            {
                int millis = lastmillis-f.interptime;
                if(millis <= 1000) trans *= float(millis)/1000.f;
            }
            if(trans > 0)
            {
                if(isbomberaffinity(f))
                {
                    if(!f.owner && !f.droptime) above.z += enttype[AFFINITY].radius/4*trans;
                    entitylight *light = &entities::ents[f.ent]->light;
                    float yaw = !f.owner && f.proj ? f.proj->yaw : (lastmillis/4)%360, pitch = !f.owner && f.proj ? f.proj->pitch : 0, roll = !f.owner && f.proj ? f.proj->roll : 0,
                          wait = f.droptime ? clamp((lastmillis-f.droptime)/float(bomberresetdelay), 0.f, 1.f) : ((f.owner && bombercarrytime) ? clamp((lastmillis-f.taketime)/float(bombercarrytime), 0.f, 1.f) : 0.f);
                    int interval = lastmillis%1000;
                    light->effect = pulsecolour();
                    if(wait > 0.5f)
                    {
                        int delay = wait > 0.7f ? (wait > 0.85f ? 150 : 300) : 600, millis = lastmillis%(delay*2);
                        float amt = (millis <= delay ? millis/float(delay) : 1.f-((millis-delay)/float(delay)));
                        flashcolour(light->effect.r, light->effect.g, light->effect.b, 1.f, 0.f, 0.f, amt);
                    }
                    light->material[0] = bvec::fromcolor(light->effect);
                    rendermodel(light, "ball", ANIM_MAPMODEL|ANIM_LOOP, above, yaw, pitch, roll, MDL_DYNSHADOW|MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_LIGHTFX, NULL, NULL, 0, 0, trans, trans);
                    float fluc = interval >= 500 ? (1500-interval)/1000.f : (500+interval)/1000.f;
                    int pcolour = (int(light->material[0].x)<<16)|(int(light->material[0].y)<<8)|int(light->material[0].z);
                    part_create(PART_HINT_SOFT, 1, above, pcolour, enttype[AFFINITY].radius/4*trans+(2*fluc), fluc*trans);
                    if(f.droptime)
                    {
                        above.z += enttype[AFFINITY].radius/4*trans+2.5f;
                        part_icon(above, textureload(hud::progresstex, 3), 3*trans, max(trans, 0.5f), 0, 0, 1, pcolour, (lastmillis%1000)/1000.f, 0.1f);
                        part_icon(above, textureload(hud::progresstex, 3), 2*trans, max(trans, 0.5f)*0.25f, 0, 0, 1, pcolour);
                        part_icon(above, textureload(hud::progresstex, 3), 2*trans, max(trans, 0.5f), 0, 0, 1, pcolour, 0, wait);
                        above.z += 1.f;
                        defformatstring(str)("<huge>%d%%", int(wait*100.f)); part_textcopy(above, str, PART_TEXT, 1, pcolour, 2, max(trans, 0.5f));
                    }
                }
                else if(!m_gsp2(game::gamemode, game::mutators))
                {
                    part_explosion(above, enttype[AFFINITY].radius*trans, PART_SHOCKWAVE, 1, TEAM(f.team, colour), 1.f, trans*0.25f);
                    part_explosion(above, enttype[AFFINITY].radius/2*trans, PART_SHOCKBALL, 1, TEAM(f.team, colour), 1.f, trans*0.65f);
                    above.z += enttype[AFFINITY].radius*trans+2.5f;
                    defformatstring(info)("<super>%s goal", TEAM(f.team, name));
                    part_textcopy(above, info, PART_TEXT, 1, TEAM(f.team, colour), 2, max(trans, 0.5f));
                }
            }
        }
    }

    void adddynlights()
    {
        loopv(st.flags)
        {
            bomberstate::flag &f = st.flags[i];
            if(!entities::ents.inrange(f.ent) || !f.enabled) continue;
            float trans = 1.f;
            int millis = lastmillis-f.interptime;
            if(millis <= 1000) trans = float(millis)/1000.f;
            vec colour = isbomberaffinity(f) ? pulsecolour() : vec::hexcolor(TEAM(f.team, colour));
            adddynlight(f.pos(), enttype[AFFINITY].radius*trans, colour, 0, 0, DL_KEEP);
        }
    }

    void reset()
    {
        st.reset();
    }

    void setup()
    {
        loopv(entities::ents) if(entities::ents[i]->type == AFFINITY)
        {
            gameentity &e = *(gameentity *)entities::ents[i];
            if(!m_check(e.attrs[3], e.attrs[4], game::gamemode, game::mutators) || !isteam(game::gamemode, game::mutators, e.attrs[0], TEAM_NEUTRAL))
                continue;
            st.addaffinity(e.o, e.attrs[0], i);
        }
    }

    void sendaffinity(packetbuf &p)
    {
        putint(p, N_INITAFFIN);
        putint(p, st.flags.length());
        loopv(st.flags)
        {
            bomberstate::flag &f = st.flags[i];
            putint(p, f.team);
            loopk(3) putint(p, int(f.spawnloc[k]*DMF));
        }
    }

    void setscore(int team, int total)
    {
        hud::teamscore(team).total = total;
    }

    void parseaffinity(ucharbuf &p)
    {
        int numflags = getint(p);
        loopi(numflags)
        {
            int team = getint(p), owner = getint(p), dropped = 0;
            vec droploc(0, 0, 0), inertia(0, 0, 0);
            if(owner < 0)
            {
                dropped = getint(p);
                if(dropped)
                {
                    loopk(3) droploc[k] = getint(p)/DMF;
                    loopk(3) inertia[k] = getint(p)/DMF;
                }
            }
            if(st.flags.inrange(i))
            {
                bomberstate::flag &f = st.flags[i];
                f.team = team;
                if(owner >= 0) st.takeaffinity(i, game::getclient(owner), lastmillis);
                else if(dropped) st.dropaffinity(i, droploc, inertia, lastmillis);
            }
        }
    }

    void dropaffinity(gameent *d, int i, const vec &droploc, const vec &inertia, int target)
    {
        if(!st.flags.inrange(i)) return;
        st.dropaffinity(i, droploc, inertia, lastmillis, target);
    }

    void removeplayer(gameent *d)
    {
        loopv(st.flags) if(st.flags[i].owner == d)
        {
            bomberstate::flag &f = st.flags[i];
            st.dropaffinity(i, f.owner->o, f.owner->vel, lastmillis);
        }
    }

    void affinityeffect(int i, int team, const vec &from, const vec &to, int effect, const char *str)
    {
        if(from.x >= 0)
        {
            if(effect&1)
            {
                defformatstring(text)("<super>\fzZe%s", str);
                part_textcopy(vec(from).add(vec(0, 0, enttype[AFFINITY].radius)), text, PART_TEXT, game::eventiconfade, TEAM(team, colour), 3, 1, -10);
            }
            if(game::dynlighteffects) adddynlight(vec(from).add(vec(0, 0, enttype[AFFINITY].radius)), enttype[AFFINITY].radius*2, vec::hexcolor(TEAM(team, colour)).mul(2.f), 500, 250);
        }
        if(to.x >= 0)
        {
            if(effect&2)
            {
                defformatstring(text)("<super>\fzZe%s", str);
                part_textcopy(vec(to).add(vec(0, 0, enttype[AFFINITY].radius)), text, PART_TEXT, game::eventiconfade, TEAM(team, colour), 3, 1, -10);
            }
            if(game::dynlighteffects) adddynlight(vec(to).add(vec(0, 0, enttype[AFFINITY].radius)), enttype[AFFINITY].radius*2, vec::hexcolor(TEAM(team, colour)).mul(2.f), 500, 250);
        }
        if(from.x >= 0 && to.x >= 0) part_trail(PART_SPARK, 500, from, to, TEAM(team, colour), 1, 1, -10);
    }

    void destroyaffinity(const vec &o)
    {
        float radius = max(WEAPEX(WEAP_GRENADE, false, game::gamemode, game::mutators, 1), enttype[AFFINITY].radius);
        part_create(PART_PLASMA_SOFT, 250, o, 0xAA4400, radius*0.5f, 0.5f);
        part_explosion(o, radius, PART_EXPLOSION, 500, 0xAA4400, 1.f, 0.5f);
        part_explosion(o, radius*2, PART_SHOCKWAVE, 250, 0xAA4400, 1.f, 0.1f);
        part_create(PART_SMOKE_LERP_SOFT, 500, o, 0x333333, radius*0.75f, 0.5f, -15);
        int debris = rnd(5)+5, amt = int((rnd(debris)+debris+1)*game::debrisscale);
        loopi(amt) projs::create(o, o, true, NULL, PRJ_DEBRIS, rnd(game::debrisfade)+game::debrisfade, 0, rnd(501), rnd(101)+50);
        playsound(WEAPSND2(WEAP_GRENADE, false, S_W_EXPLODE), o, NULL, 0, 255);
    }

    void resetaffinity(int i, int value)
    {
        if(!st.flags.inrange(i)) return;
        bomberstate::flag &f = st.flags[i];
        if(f.enabled && value)
        {
            destroyaffinity(f.pos());
            if(value == 2 && isbomberaffinity(f))
            {
                affinityeffect(i, TEAM_NEUTRAL, f.pos(), f.spawnloc, 3, "RESET");
                game::announcef(S_V_BOMBRESET, CON_INFO, NULL, "\fathe \fs\fwbomb\fS has been reset");
            }
        }
        st.returnaffinity(i, lastmillis, value!=0);
    }

    void scoreaffinity(gameent *d, int relay, int goal, int score)
    {
        if(!st.flags.inrange(relay) || !st.flags.inrange(goal)) return;
        bomberstate::flag &f = st.flags[relay], &g = st.flags[goal];
        affinityeffect(goal, d->team, g.spawnloc, f.spawnloc, 3, "DESTROYED");
        destroyaffinity(g.spawnloc);
        hud::teamscore(d->team).total = score;
        game::announcef(S_V_BOMBSCORE, d == game::focus ? CON_SELF : CON_INFO, d, "\fa%s destroyed the \fs\f[%d]%s\fS base for team \fs\f[%d]%s\fS (score: \fs\fc%d\fS, time taken: \fs\fc%s\fS)", game::colorname(d), TEAM(g.team, colour), TEAM(g.team, name), TEAM(d->team, colour), TEAM(d->team, name), score, hud::timetostr(lastmillis-f.inittime));
        st.returnaffinity(relay, lastmillis, false);
    }

    void takeaffinity(gameent *d, int i)
    {
        if(!st.flags.inrange(i)) return;
        bomberstate::flag &f = st.flags[i];
        d->action[AC_AFFINITY] = false;
        d->actiontime[AC_AFFINITY] = 0;
        playsound(S_CATCH, d->o, d);
        if(!f.droptime)
        {
            affinityeffect(i, d->team, d->feetpos(), f.pos(), 1, "TAKEN");
            game::announcef(S_V_BOMBPICKUP, d == game::focus ? CON_SELF : CON_INFO, d, "\fa%s picked up the \fs\fwbomb\fS", game::colorname(d));
        }
        st.takeaffinity(i, d, lastmillis);
    }

    void checkaffinity(gameent *d)
    {
        vec o = d->feetpos();
        loopv(st.flags)
        {
            bomberstate::flag &f = st.flags[i];
            if(!entities::ents.inrange(f.ent) || !f.enabled || !isbomberaffinity(f)) continue;
            if(f.owner)
            {
                int takemillis = lastmillis-f.taketime;
                if(bombercarrytime && d->ai && f.owner == d && takemillis >= bombercarrytime-550-bomberlockondelay)
                {
                    if(d->action[AC_AFFINITY])
                    {
                        if(takemillis >= bombercarrytime-500 || lastmillis-d->actiontime[AC_AFFINITY] >= bomberlockondelay)
                            d->action[AC_AFFINITY] = false;
                    }
                    else
                    {
                        d->action[AC_AFFINITY] = true;
                        d->actiontime[AC_AFFINITY] = lastmillis;
                    }
                }
                continue;
            }
            else if(f.droptime)
            {
                f.droploc = f.pos();
                if(f.lastowner && (f.lastowner == game::player1 || f.lastowner->ai) && f.proj && (!f.movetime || totalmillis-f.movetime >= 40))
                {
                    f.inertia = f.proj->vel;
                    f.movetime = totalmillis;
                    client::addmsg(N_MOVEAFFIN, "ri8", f.lastowner->clientnum, i, int(f.droploc.x*DMF), int(f.droploc.y*DMF), int(f.droploc.z*DMF), int(f.inertia.x*DMF), int(f.inertia.y*DMF), int(f.inertia.z*DMF));
                }
            }
            if(f.pickuptime && lastmillis-f.pickuptime <= 1000) continue;
            if(f.lastowner == d && f.droptime && (bomberpickupdelay < 0 || lastmillis-f.droptime <= bomberpickupdelay)) continue;
            if(o.dist(f.pos()) <= enttype[AFFINITY].radius/2)
            {
                client::addmsg(N_TAKEAFFIN, "ri2", d->clientnum, i);
                f.pickuptime = lastmillis;
            }
        }
        dropaffinity(d);
    }

    bool aihomerun(gameent *d, ai::aistate &b)
    {
        vec pos = d->feetpos();
        if(m_team(game::gamemode, game::mutators) && !m_gsp2(game::gamemode, game::mutators))
        {
            int goal = -1;
            loopv(st.flags)
            {
                bomberstate::flag &g = st.flags[i];
                if(isbombertarg(g, ai::owner(d)) && (!st.flags.inrange(goal) || g.pos().squaredist(pos) < st.flags[goal].pos().squaredist(pos)))
                    goal = i;
            }
            if(st.flags.inrange(goal) && ai::makeroute(d, b, st.flags[goal].pos()))
            {
                d->ai->switchstate(b, ai::AI_S_PURSUE, ai::AI_T_AFFINITY, goal);
                return true;
            }
        }
	    if(b.type == ai::AI_S_PURSUE && b.targtype == ai::AI_T_NODE) return true; // we already did this..
		if(ai::randomnode(d, b, ai::ALERTMIN, 1e16f))
		{
            d->ai->switchstate(b, ai::AI_S_PURSUE, ai::AI_T_NODE, d->ai->route[0]);
            return true;
		}
        return false;
    }

    int aiowner(gameent *d)
    {
        loopv(st.flags) if(entities::ents.inrange(st.flags[i].ent) && entities::ents[d->aientity]->links.find(st.flags[i].ent) >= 0)
            return st.flags[i].team;
        return d->team;
    }

    bool aicheck(gameent *d, ai::aistate &b)
    {
        if(d->aitype == AI_BOT)
        {
            static vector<int> taken; taken.setsize(0);
            loopv(st.flags)
            {
                bomberstate::flag &g = st.flags[i];
                if(g.owner == d) return aihomerun(d, b);
                else if((g.owner && ai::owner(g.owner) != ai::owner(d)) || g.droptime) taken.add(i);
            }
            if(!ai::badhealth(d)) while(!taken.empty())
            {
                int flag = taken.length() > 2 ? rnd(taken.length()) : 0;
                if(ai::makeroute(d, b, st.flags[taken[flag]].pos()))
                {
                    d->ai->switchstate(b, ai::AI_S_PURSUE, ai::AI_T_AFFINITY, taken[flag]);
                    return true;
                }
                else taken.remove(flag);
            }
        }
        return false;
    }

    void aifind(gameent *d, ai::aistate &b, vector<ai::interest> &interests)
    {
        vec pos = d->feetpos();
        loopvj(st.flags)
        {
            bomberstate::flag &f = st.flags[j];
            if(!entities::ents.inrange(f.ent) || !f.enabled) continue;
            bool home = isbomberhome(f, ai::owner(d));
            static vector<int> targets; // build a list of others who are interested in this
            targets.setsize(0);
            bool regen = d->aitype != AI_BOT || f.team == TEAM_NEUTRAL || !m_regen(game::gamemode, game::mutators) || d->health >= m_health(game::gamemode, game::mutators);
            ai::checkothers(targets, d, home || d->aitype != AI_BOT ? ai::AI_S_DEFEND : ai::AI_S_PURSUE, ai::AI_T_AFFINITY, j, true);
            if(d->aitype == AI_BOT)
            {
                gameent *e = NULL;
                int numdyns = game::numdynents();
                float mindist = enttype[AFFINITY].radius*4; mindist *= mindist;
                loopi(numdyns) if((e = (gameent *)game::iterdynents(i)) && !e->ai && e->state == CS_ALIVE && ai::owner(d) == ai::owner(e))
                {
                    if(targets.find(e->clientnum) < 0 && (f.owner == e || e->feetpos().squaredist(f.pos()) <= mindist))
                        targets.add(e->clientnum);
                }
            }
            if(home)
            {
                bool guard = false;
                if(f.owner || f.droptime || targets.empty()) guard = true;
                else if(d->hasweap(d->ai->weappref, m_weapon(game::gamemode, game::mutators)))
                { // see if we can relieve someone who only has a piece of crap
                    gameent *t;
                    loopvk(targets) if((t = game::getclient(targets[k])))
                    {
                        if((t->ai && !t->hasweap(t->ai->weappref, m_weapon(game::gamemode, game::mutators))) || (!t->ai && t->weapselect < WEAP_OFFSET))
                        {
                            guard = true;
                            break;
                        }
                    }
                }
                if(guard)
                { // defend the flag
                    ai::interest &n = interests.add();
                    n.state = ai::AI_S_DEFEND;
                    n.node = ai::closestwaypoint(f.pos(), ai::CLOSEDIST, true);
                    n.target = j;
                    n.targtype = ai::AI_T_AFFINITY;
                    n.score = pos.squaredist(f.pos())/(!regen ? 100.f : 1.f);
                    n.tolerance = 0.25f;
                    n.team = true;
                }
            }
            else if(isbomberaffinity(f))
            {
                if(targets.empty())
                { // attack the flag
                    ai::interest &n = interests.add();
                    n.state = d->aitype == AI_BOT ? ai::AI_S_PURSUE : ai::AI_S_DEFEND;
                    n.node = ai::closestwaypoint(f.pos(), ai::CLOSEDIST, true);
                    n.target = j;
                    n.targtype = ai::AI_T_AFFINITY;
                    n.score = pos.squaredist(f.pos());
                    n.tolerance = 0.25f;
                    n.team = true;
                }
                else
                { // help by defending the attacker
                    gameent *t;
                    loopvk(targets) if((t = game::getclient(targets[k])))
                    {
                        ai::interest &n = interests.add();
                        bool team = ai::owner(d) == ai::owner(t);
                        n.state = team ? ai::AI_S_DEFEND : ai::AI_S_PURSUE;
                        n.node = t->lastnode;
                        n.target = t->clientnum;
                        n.targtype = ai::AI_T_ACTOR;
                        n.score = d->o.squaredist(t->o);
                        n.tolerance = 0.25f;
                        n.team = team;
                    }
                }
            }
        }
    }

    bool aidefense(gameent *d, ai::aistate &b)
    {
        if(d->aitype == AI_BOT)
            loopv(st.flags) if(st.flags[i].owner == d) return aihomerun(d, b);
        if(st.flags.inrange(b.target))
        {
            bomberstate::flag &f = st.flags[b.target];
            if(isbomberaffinity(f) && f.owner && ai::owner(d) != ai::owner(f.owner))
                return ai::violence(d, b, f.owner, 4);
            int walk = f.owner && ai::owner(f.owner) != ai::owner(d) ? 1 : 0;
            if(d->aitype == AI_BOT)
            {
                bool regen = !m_regen(game::gamemode, game::mutators) || d->health >= m_health(game::gamemode, game::mutators);
                if(regen && lastmillis-b.millis >= (201-d->skill)*33)
                {
                    static vector<int> targets; // build a list of others who are interested in this
                    targets.setsize(0);
                    ai::checkothers(targets, d, ai::AI_S_DEFEND, ai::AI_T_AFFINITY, b.target, true);
                    gameent *e = NULL;
                    int numdyns = game::numdynents();
                    float mindist = enttype[AFFINITY].radius*4; mindist *= mindist;
                    loopi(numdyns) if((e = (gameent *)game::iterdynents(i)) && !e->ai && e->state == CS_ALIVE && ai::owner(d) == ai::owner(e))
                    {
                        if(targets.find(e->clientnum) < 0 && (f.owner == e || e->feetpos().squaredist(f.pos()) <= mindist))
                            targets.add(e->clientnum);
                    }
                    if(!targets.empty())
                    {
                        d->ai->trywipe = true; // re-evaluate so as not to herd
                        return true;
                    }
                    else
                    {
                        walk = 2;
                        b.millis = lastmillis;
                    }
                }
                vec pos = d->feetpos();
                float mindist = enttype[AFFINITY].radius*8; mindist *= mindist;
                loopv(st.flags)
                { // get out of the way of the returnee!
                    bomberstate::flag &g = st.flags[i];
                    if(pos.squaredist(g.pos()) <= mindist)
                    {
                        if(g.owner && ai::owner(g.owner) == ai::owner(d) && !walk) walk = 1;
                        if(g.droptime && ai::makeroute(d, b, g.pos())) return true;
                    }
                }
            }
            return ai::defense(d, b, f.pos(), f.owner ? ai::CLOSEDIST : float(enttype[AFFINITY].radius), f.owner ? ai::ALERTMIN : float(enttype[AFFINITY].radius*walk*16), walk);
        }
        return false;
    }

    bool aipursue(gameent *d, ai::aistate &b)
    {
        if(st.flags.inrange(b.target) && d->aitype == AI_BOT)
        {
            bomberstate::flag &f = st.flags[b.target];
            if(!entities::ents.inrange(f.ent) || !f.enabled) return false;
            b.idle = -1;
            if(isbomberaffinity(f))
            {
                if(f.owner)
                {
                    if(d == f.owner) return aihomerun(d, b);
                    else if(ai::owner(d) != ai::owner(f.owner)) return ai::violence(d, b, f.owner, 4);
                    else return ai::defense(d, b, f.pos());
                }
                return ai::makeroute(d, b, f.pos());
            }
            else if(isbombertarg(f, ai::owner(d)))
                loopv(st.flags) if(st.flags[i].owner == d) return ai::makeroute(d, b, f.pos());
        }
        return false;
    }
}
