#include "game.h"

namespace hud
{
    static vector<score> scores;

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

    void resetscores()
    {
        scores.shrink(0);
    }

    struct scoregroup : score
    {
        vector<gameent *> players;
    };
    vector<scoregroup *> groups;
    vector<gameent *> spectators;

    VAR(IDF_PERSIST, autoshowscores, 0, 2, 3); // 1 = when dead, 2 = also in spectv, 3 = and in waittv too
    VAR(IDF_PERSIST, showscoresdelay, 0, 0, VAR_MAX); // otherwise use respawn delay
    VAR(IDF_PERSIST, showscoresinfo, 0, 1, 1);
    VAR(IDF_PERSIST, scorehighlight, 0, 1, 1);

    VAR(IDF_PERSIST, showpj, 0, 0, 1);
    VAR(IDF_PERSIST, showping, 0, 1, 1);
    VAR(IDF_PERSIST, showpoints, 0, 1, 1);
    VAR(IDF_PERSIST, showscore, 0, 2, 2);
    VAR(IDF_PERSIST, showclientnum, 0, 1, 1);
    VAR(IDF_PERSIST, showbotinfo, 0, 0, 1);
    VAR(IDF_PERSIST, showspectators, 0, 1, 1);
    VAR(IDF_PERSIST, showconnecting, 0, 0, 1);
    VAR(IDF_PERSIST, showhostname, 0, 0, 1);

    static bool scoreson = false, scoresoff = false, shownscores = false;
    static int menustart = 0, menulastpress = 0;

    bool canshowscores()
    {
        if(!scoresoff && !scoreson && !shownscores && autoshowscores)
        {
            if(game::player1->state == CS_DEAD)
            {
                int delay = showscoresdelay ? showscoresdelay : m_delay(game::gamemode, game::mutators);
                if(!delay || lastmillis-game::player1->lastdeath > delay) return true;
            }
            else return game::tvmode() && autoshowscores >= (game::player1->state == CS_SPECTATOR ? 2 : 3);
        }
        return false;
    }

    static inline bool playersort(const gameent *a, const gameent *b)
    {
        if(a->state == CS_SPECTATOR)
        {
            if(b->state == CS_SPECTATOR) return strcmp(a->name, b->name) < 0;
            else return false;
        }
        else if(b->state == CS_SPECTATOR) return true;
        if(m_trial(game::gamemode))
        {
            if((a->cptime && !b->cptime) || (a->cptime && b->cptime && a->cptime < b->cptime)) return true;
            if((b->cptime && !a->cptime) || (a->cptime && b->cptime && b->cptime < a->cptime)) return false;
        }
        if(a->points > b->points) return true;
        if(a->points < b->points) return false;
        if(a->frags > b->frags) return true;
        if(a->frags < b->frags) return false;
        return strcmp(a->name, b->name) < 0;
    }

    static inline bool scoregroupcmp(const scoregroup *x, const scoregroup *y)
    {
        if(!x->team)
        {
            if(y->team) return false;
        }
        else if(!y->team) return true;
        if(x->total > y->total) return true;
        if(x->total < y->total) return false;
        if(x->players.length() > y->players.length()) return true;
        if(x->players.length() < y->players.length()) return false;
        return x->team && y->team && x->team < y->team;
    }

    int groupplayers()
    {
        int numgroups = 0;
        spectators.shrink(0);
        int numdyns = game::numdynents();
        loopi(numdyns)
        {
            gameent *o = (gameent *)game::iterdynents(i);
            if(!o || o->type != ENT_PLAYER || (!showconnecting && !o->name[0])) continue;
            if(o->state == CS_SPECTATOR)
            {
                if(o != game::player1 || !client::demoplayback) spectators.add(o);
                continue;
            }
            int team = m_fight(game::gamemode) && m_isteam(game::gamemode, game::mutators) ? o->team : TEAM_NEUTRAL;
            bool found = false;
            loopj(numgroups)
            {
                scoregroup &g = *groups[j];
                if(team != g.team) continue;
                if(team) g.total = teamscore(team).total;
                g.players.add(o);
                found = true;
                break;
            }
            if(found) continue;
            if(numgroups >= groups.length()) groups.add(new scoregroup);
            scoregroup &g = *groups[numgroups++];
            g.team = team;
            if(!team) g.total = 0;
            else if(m_isteam(game::gamemode, game::mutators)) g.total = teamscore(o->team).total;
            else g.total = o->points;

            g.players.shrink(0);
            g.players.add(o);
        }
        loopi(numgroups) groups[i]->players.sort(playersort);
        spectators.sort(playersort);
        groups.sort(scoregroupcmp, 0, numgroups);
        return numgroups;
    }

    void showscores(bool on, bool interm, bool onauto, bool ispress)
    {
        if(!client::waiting())
        {
            if(ispress)
            {
                bool within = menulastpress && totalmillis-menulastpress < PHYSMILLIS;
                if(on)
                {
                    if(within) onauto = true;
                    menulastpress = totalmillis;
                }
                else if(within && !scoresoff) { menulastpress = 0; return; }
            }
            if(!scoreson && on) menustart = guicb::starttime();
            scoresoff = !onauto;
            scoreson = on;
            if(m_play(game::gamemode) && m_fight(game::gamemode) && interm)
            {
                int numgroups = groupplayers();
                if(!numgroups) return;
                scoregroup &sg = *groups[0];
                if(!m_trial(game::gamemode) && m_isteam(game::gamemode, game::mutators))
                {
                    int anc = sg.players.find(game::player1) >= 0 ? S_V_YOUWIN : (game::player1->state != CS_SPECTATOR ? S_V_YOULOSE : -1);
                    if(m_defend(game::gamemode) && sg.total == INT_MAX)
                        game::announcef(anc, CON_MESG, NULL, true, "\fwteam \fs\f[%d]%s\fS secured all flags", TEAM(sg.team, colour), TEAM(sg.team, name));
                    else
                    {
                        if(numgroups > 1 && sg.total == groups[1]->total)
                        {
                            mkstring(winner);
                            loopi(numgroups) if(i)
                            {
                                if(sg.total == groups[i]->total)
                                {
                                    defformatstring(tw)("\fw\fs\f[%d]%s\fS, ", TEAM(groups[i]->team, colour), TEAM(groups[i]->team, name));
                                    concatstring(winner, tw);
                                }
                                else break;
                            }
                            game::announcef(S_V_DRAW, CON_MESG, NULL, true, "\fw\fs\f[%d]%s\fS tied %swith a total score of: \fs\fc%d\fS", TEAM(sg.team, colour), TEAM(sg.team, name), winner, sg.total);
                        }
                        else game::announcef(anc, CON_MESG, NULL, true, "\fwteam \fs\f[%d]%s\fS won the match with a total score of: \fs\fc%d\fS", TEAM(sg.team, colour), TEAM(sg.team, name), sg.total);
                    }
                }
                else
                {
                    int anc = sg.players[0] == game::player1 ? S_V_YOUWIN : (game::player1->state != CS_SPECTATOR ? S_V_YOULOSE : -1);
                    if(m_trial(game::gamemode))
                    {
                        if(sg.players.length() > 1 && sg.players[0]->cptime == sg.players[1]->cptime)
                        {
                            mkstring(winner);
                            loopv(sg.players) if(i)
                            {
                                if(sg.players[0]->cptime == sg.players[i]->cptime)
                                {
                                    concatstring(winner, game::colorname(sg.players[i]));
                                    concatstring(winner, ", ");
                                }
                                else break;
                            }
                            game::announcef(S_V_DRAW, CON_MESG, NULL, true, "\fw%s tied %swith the fastest lap: \fs\fc%s\fS", game::colorname(sg.players[0]), winner, sg.players[0]->cptime ? timetostr(sg.players[0]->cptime) : "dnf");
                        }
                        else game::announcef(anc, CON_MESG, NULL, true, "\fw%s won the match with the fastest lap: \fs\fc%s\fS", game::colorname(sg.players[0]), sg.players[0]->cptime ? timetostr(sg.players[0]->cptime) : "dnf");
                    }
                    else
                    {
                        if(sg.players.length() > 1 && sg.players[0]->points == sg.players[1]->points)
                        {
                            mkstring(winner);
                            loopv(sg.players) if(i)
                            {
                                if(sg.players[0]->points == sg.players[i]->points)
                                {
                                    concatstring(winner, game::colorname(sg.players[i]));
                                    concatstring(winner, ", ");
                                }
                                else break;
                            }
                            game::announcef(S_V_DRAW, CON_MESG, NULL, true, "\fw%s tied %swith a total score of: \fs\fc%d\fS", game::colorname(sg.players[0]), winner, sg.players[0]->points);
                        }
                        else game::announcef(anc, CON_MESG, NULL, true, "\fw%s won the match with a total score of: \fs\fc%d\fS", game::colorname(sg.players[0]), sg.players[0]->points);
                    }
                }
            }
        }
        else scoresoff = scoreson = false;
    }

    void renderscoreboard(guient &g, bool firstpass)
    {
        g.start(menustart, menuscale, NULL, false, false);
        int numgroups = groupplayers();
        uilist(g, {
            g.image(NULL, 6, true);
            g.space(2);
            uicenter(g, {
                uicenterlist(g, {
                    g.space(0.25f);
                    uicenterlist(g, uifont(g, "emphasis", g.textf("%s", 0xFFFFFF, NULL, 0, *maptitle ? maptitle : mapname)));
                    if(*mapauthor) uicenterlist(g, uifont(g, "default", g.textf("by %s", 0xFFFFFF, NULL, 0, mapauthor)));
                    uicenterlist(g, uifont(g, "reduced", {
                        defformatstring(gname)("%s", server::gamename(game::gamemode, game::mutators));
                        if(strlen(gname) > 32) formatstring(gname)("%s", server::gamename(game::gamemode, game::mutators, 1));
                        g.textf("%s", 0xFFFFFF, NULL, 0, gname);
                        if((m_play(game::gamemode) || client::demoplayback) && game::timeremaining >= 0)
                        {
                            if(game::intermission) g.textf(", \fs\fyintermission\fS", 0xFFFFFF, NULL, 0);
                            else if(paused) g.textf(", \fs\fopaused\fS", 0xFFFFFF, NULL, 0);
                            else if(game::timeremaining) g.textf(", \fs\fg%s\fS remain", 0xFFFFFF, NULL, 0, timetostr(game::timeremaining, 2));
                        }
                    }));
                    if(*connectname)
                    {
                        uicenterlist(g, {
                            uifont(g, "little", g.textf("\fdon ", 0xFFFFFF, NULL, 0));
                            if(*serverdesc) uifont(g, "reduced", g.textf("%s ", 0xFFFFFF, NULL, 0, serverdesc));
                            uifont(g, "little", g.textf("\fd(\fa%s:[%d]\fd)", 0xFFFFFF, NULL, 0, connectname, connectport));
                        });
                    }
                    if(client::demoplayback)
                    {
                        uicenterlist(g, {
                            uifont(g, "reduced", g.textf("Demo Playback in Progress", 0xFFFFFF, NULL, 0));
                        });
                    }
                    else if(!game::intermission)
                    {
                        if(game::player1->state == CS_DEAD || game::player1->state == CS_WAITING)
                        {
                            int sdelay = m_delay(game::gamemode, game::mutators);
                            int delay = game::player1->lastdeath ? game::player1->respawnwait(lastmillis, sdelay) : 0;
                            g.pushlist();
                            g.spring();
                            uifont(g, "reduced", g.textf("%s", 0xFFFFFF, NULL, 0, game::player1->state == CS_WAITING ? "Please Wait" : "Fragged"));
                            if(shownotices >= 2)
                            {
                                g.space(1);
                                SEARCHBINDCACHE(attackkey)("action 0", 0);
                                uifont(g, "little", {
                                    if(delay || (m_trial(game::gamemode) && !game::player1->lastdeath) || m_duke(game::gamemode, game::mutators) || (m_fight(game::gamemode) && maxalive > 0))
                                    {
                                        if(m_duke(game::gamemode, game::mutators)) g.textf("Queued for new round", 0xFFFFFF, NULL, 0);
                                        else if(delay) g.textf("Down for \fs\fy%s\fS", 0xFFFFFF, NULL, 0, timetostr(delay, -1));
                                        else if(game::player1->state == CS_WAITING && m_fight(game::gamemode) && maxalive > 0 && maxalivequeue)
                                        {
                                            int n = game::numwaiting();
                                            if(n) g.textf("Waiting for \fs\fy%d\fS %s", 0xFFFFFF, NULL, 0, n, n != 1 ? "players" : "player");
                                            else g.textf("You are \fs\fgnext\fS in the queue", 0xFFFFFF, NULL, 0);
                                        }
                                        g.spring();
                                        g.poplist();
                                        if(game::player1->state != CS_WAITING && lastmillis-game::player1->lastdeath > 500)
                                            uicenterlist(g, g.textf("Press \fs\fc%s\fS to enter respawn queue", 0xFFFFFF, NULL, 0, attackkey));
                                    }
                                    else
                                    {
                                        g.textf("Ready to respawn", 0xFFFFFF, NULL, 0);
                                        g.spring();
                                        g.poplist();
                                        if(game::player1->state != CS_WAITING) uicenterlist(g, g.textf("Press \fs\fc%s\fS to respawn now", 0xFFFFFF, NULL, 0, attackkey));
                                    }
                                    if(game::player1->state == CS_WAITING && lastmillis-game::player1->lastdeath >= 500)
                                    {
                                        SEARCHBINDCACHE(waitmodekey)("waitmodeswitch", 3);
                                        uicenterlist(g, g.textf("Press \fs\fc%s\fS to %s", 0xFFFFFF, NULL, 0, waitmodekey, game::tvmode() ? "interact" : "switch to TV"));
                                    }
                                    if(m_arena(game::gamemode, game::mutators))
                                    {
                                        SEARCHBINDCACHE(loadkey)("showgui loadout", 0);
                                        uicenterlist(g, g.textf("Press \fs\fc%s\fS to \fs%s\fS loadout", 0xFFFFFF, NULL, 0, loadkey, game::player1->loadweap.empty() ? "\fzoyselect" : "change"));
                                    }
                                    if(m_fight(game::gamemode) && m_isteam(game::gamemode, game::mutators))
                                    {
                                        SEARCHBINDCACHE(teamkey)("showgui team", 0);
                                        uicenterlist(g, g.textf("Press \fs\fc%s\fS to change teams", 0xFFFFFF, NULL, 0, teamkey));
                                    }
                                });
                            }
                            else
                            {
                                g.spring();
                                g.poplist();
                            }
                        }
                        else if(game::player1->state == CS_ALIVE)
                        {
                            uifont(g, "little", {
                                uicenterlist(g, {
                                    // In two cases, the main mode-description is not applicable
                                    if(m_bomber(game::gamemode) && m_gsp2(game::gamemode, game::mutators)) // bomber-ball hold
                                        g.textf("%s", 0xFFFFFF, NULL, 0, gametype[game::gamemode].gsd[1]);
                                    else if(m_capture(game::gamemode) && m_gsp3(game::gamemode, game::mutators)) // ctf protect
                                        g.textf("%s", 0xFFFFFF, NULL, 0, gametype[game::gamemode].gsd[2]);
                                    else g.textf("%s", 0xFFFFFF, NULL, 0, gametype[game::gamemode].desc);
                                });
                                if(m_isteam(game::gamemode, game::mutators))
                                uicenterlist(g, g.textf("Playing for team \fs\f[%d]\f(%s)%s\fS", 0xFFFFFF, NULL, 0, TEAM(game::player1->team, colour), teamtexname(game::player1->team), TEAM(game::player1->team, name)));
                            });
                        }
                        else if(game::player1->state == CS_SPECTATOR)
                        {
                            uicenterlist(g, uifont(g, "reduced", g.textf("%s", 0xFFFFFF, NULL, 0, game::tvmode() ? "SpecTV" : "Spectating")));
                            SEARCHBINDCACHE(speconkey)("spectator 0", 1);
                            uifont(g, "little", {
                                uicenterlist(g, g.textf("Press \fs\fc%s\fS to join the game", 0xFFFFFF, NULL, 0, speconkey));
                                if(!m_edit(game::gamemode) && shownotices >= 2)
                                {
                                    SEARCHBINDCACHE(specmodekey)("specmodeswitch", 1);
                                    uicenterlist(g, g.textf("Press \fs\fc%s\fS to %s", 0xFFFFFF, NULL, 0, specmodekey, game::tvmode() ? "interact" : "switch to TV"));
                                }
                            });
                        }

                        if(m_edit(game::gamemode) && (game::focus->state != CS_EDITING || shownotices >= 4))
                        {
                            SEARCHBINDCACHE(editkey)("edittoggle", 1);
                            uicenterlist(g, uifont(g, "little", g.textf("Press \fs\fc%s\fS to %s editmode", 0xFFFFFF, NULL, 0, editkey, game::focus->state != CS_EDITING ? "enter" : "exit")));
                        }
                    }

                    SEARCHBINDCACHE(scoreboardkey)("showscores", 1);
                    uicenterlist(g, uifont(g, "little", g.textf("%s \fs\fc%s\fS to close this window", 0xFFFFFF, NULL, 0, scoresoff ? "Release" : "Press", scoreboardkey)));
                    uicenterlist(g, uifont(g, "tiny", g.textf("Double-tap to keep the window open", 0xFFFFFF, NULL, 0)));
                });
            });
        });
        g.space(0.5f);
        #define loopscoregroup(b) \
            loopv(sg.players) \
            { \
                gameent *o = sg.players[i]; \
                b; \
            }
        uifont(g, numgroups > 1 ? "little" : "reduced", {
            float namepad = 0;
            loopk(numgroups)
            {
                scoregroup &sg = *groups[k];
                loopscoregroup(namepad = max(namepad, (float)text_width(game::colorname(o, NULL, "", false))));
            }
            namepad = (namepad-(text_width("name")-guibound[0]))*0.5f/guibound[0];
            loopk(numgroups)
            {
                if((k%2)==0)
                {
                    if(k) g.space(0.5f);
                    g.pushlist();
                }
                uicenter(g, {
                    scoregroup &sg = *groups[k];
                    int bgcolor = sg.team && m_fight(game::gamemode) && m_isteam(game::gamemode, game::mutators) ? TEAM(sg.team, colour) : 0x333333;
                    int fgcolor = 0xFFFFFF;
                    uilist(g, {
                        if(sg.team && m_fight(game::gamemode) && m_isteam(game::gamemode, game::mutators))
                        {
                            uicenterlist(g, {
                                g.background(bgcolor);
                                g.text("", 0, teamtexname(sg.team), TEAM(sg.team, colour));
                            });
                        }
                        g.space(1);
                        loopscoregroup({
                            const char *status = playertex;
                            if(o->state == CS_DEAD || o->state == CS_WAITING) status = deadtex;
                            else if(o->state == CS_ALIVE && (!m_isteam(game::gamemode, game::mutators) || o->team != game::focus->team))
                            {
                                if(o->dominating.find(game::focus) >= 0) status = dominatingtex;
                                else if(o->dominated.find(game::focus) >= 0) status = dominatedtex;
                            }
                            uicenterlist(g, g.text("", 0, status, game::getcolour(o, game::playerdisplaytone)));
                        });
                    });

                    if(sg.team && m_fight(game::gamemode) && m_isteam(game::gamemode, game::mutators))
                    {
                        g.pushlist();
                        uilist(g, {
                            g.background(bgcolor);
                            if(m_defend(game::gamemode) && ((defendlimit && sg.total >= defendlimit) || sg.total == INT_MAX))
                                g.textf("%s: WIN", fgcolor, NULL, 0, TEAM(sg.team, name));
                            else g.textf("%s: %d", fgcolor, NULL, 0, TEAM(sg.team, name), sg.total);
                            g.spring();
                        });
                        g.pushlist();
                    }

                    uilist(g, {
                        uicenterlist(g, uipad(g, namepad, uicenterlist(g, g.text("name", fgcolor))));
                        loopscoregroup(uicenterlist(g, uipad(g, 0.5f, uicenterlist(g, g.textf("%s", 0xFFFFFF, NULL, 0, game::colorname(o, NULL, "", false))))));
                    });

                    if(showpoints)
                    {
                        uilist(g, {
                            uicenterlist(g, uipad(g, 1, g.text("points", fgcolor)));
                            loopscoregroup(uicenterlist(g, uipad(g, 0.5f, g.textf("%d", 0xFFFFFF, NULL, 0, o->points))));
                        });
                    }

                    if(showscore && (showscore >= 2 || !m_affinity(game::gamemode)))
                    {
                        uilist(g, {
                            if(m_trial(game::gamemode))
                            {
                                uicenterlist(g, uipad(g, 4, g.text("best", fgcolor)));
                                loopscoregroup(uicenterlist(g, uipad(g, 0.5f, g.textf("%s", 0xFFFFFF, NULL, 0, o->cptime ? timetostr(o->cptime) : "\fadnf"))));
                            }
                            else
                            {
                                uicenterlist(g, uipad(g, 1, g.text("frags", fgcolor)));
                                loopscoregroup(uicenterlist(g, uipad(g, 0.5f, g.textf("%d", 0xFFFFFF, NULL, 0, o->frags))));
                            }
                        });
                    }

                    if(showpj)
                    {
                        uilist(g, {
                            uicenterlist(g, uipad(g, 2, g.text("pj", fgcolor)));
                            loopscoregroup(uicenterlist(g, uipad(g, 0.5f, g.textf("%d", 0xFFFFFF, NULL, 0, o->plag))));
                        });
                    }

                    if(showping)
                    {
                        uilist(g, {
                            uicenterlist(g, uipad(g, 2, g.text("ping", fgcolor)));
                            loopscoregroup(uicenterlist(g, uipad(g, 0.5f, g.textf("%d", 0xFFFFFF, NULL, 0, o->ping))));
                        });
                    }

                    if(showclientnum || game::player1->privilege>=PRIV_HELPER)
                    {
                        uilist(g, {
                            uicenterlist(g, uipad(g, 1, g.text("cn", fgcolor)));
                            loopscoregroup(uicenterlist(g, uipad(g, 0.5f, g.textf("%d", 0xFFFFFF, NULL, 0, o->clientnum))));
                        });
                    }

                    if(showbotinfo)
                    {
                        bool hasbots = false;
                        loopscoregroup(if(o->aitype > AI_NONE) { hasbots = true; break; });
                        if(hasbots)
                        {
                            uilist(g, {
                                uicenterlist(g, uipad(g, 1, g.text("sk", fgcolor)));
                                loopscoregroup({
                                    uicenterlist(g, uipad(g, 0.5f, {
                                        if(o->aitype > AI_NONE) g.textf("%d", 0xFFFFFF, NULL, 0, o->skill);
                                        else g.strut(1);
                                    }));
                                });
                            });
                            uilist(g, {
                                uicenterlist(g, uipad(g, 1, g.text("on", fgcolor)));
                                loopscoregroup({
                                    uicenterlist(g, uipad(g, 0.5f, {
                                        if(o->aitype > AI_NONE) g.textf("%d", 0xFFFFFF, NULL, 0, o->ownernum);
                                        else g.strut(1);
                                    }));
                                });
                            });
                        }
                    }

                    if(showhostname)
                    {
                        uilist(g, {
                            uicenterlist(g, uipad(g, 4, g.text("host", fgcolor)));
                            loopscoregroup({
                                uicenterlist(g, uipad(g, 0.5f, {
                                    if(o->aitype == AI_NONE) g.textf("%s", 0xFFFFFF, NULL, 0, o->hostname);
                                    else g.strut(1);
                                }));
                            });
                        });
                    }
                    if(scorehighlight)
                    {
                        uilist(g, {
                            uicenterlist(g, uipad(g, 0.5f, g.strut(1)));
                            loopscoregroup({
                                int bgcol = o == game::player1 ? 0x888888 : 0x111111;
                                switch(o->privilege)
                                {
                                    case PRIV_ADMINISTRATOR: bgcol |= 0x004444; break;
                                    case PRIV_MODERATOR: bgcol |= 0x440044; break;
                                    case PRIV_HELPER: bgcol |= 0x000044; break;
                                    default: break;
                                }
                                uicenterlist(g, uipad(g, 0.5f, g.text("", 0, insigniatex, bgcol)));
                            });
                        });
                    }

                    if(sg.team && m_fight(game::gamemode) && m_isteam(game::gamemode, game::mutators))
                    {
                        g.poplist(); // horizontal
                        g.poplist(); // vertical
                    }
                });
                if(k+1<numgroups && (k+1)%2) g.space(1);
                else g.poplist();
            }
        });
        if(showspectators && spectators.length())
        {
            g.space(0.5f);
            uifont(g, "little", {
                int count = numgroups > 1 ? 5 : 3;
                bool pushed = false;
                loopv(spectators)
                {
                    gameent *o = spectators[i];
                    if((i%count)==0)
                    {
                        g.pushlist();
                        pushed = true;
                    }
                    uicenter(g, uipad(g, 0.5f, {
                        g.space(0.5f);
                        if(showclientnum || game::player1->privilege>=PRIV_HELPER)
                            g.textf("%s (%d)", 0x666666, spectex, game::getcolour(o, game::playerdisplaytone), game::colorname(o, NULL, "", false), o->clientnum);
                        else g.textf("%s ", 0x666666, spectex, game::getcolour(o, game::playerdisplaytone), game::colorname(o, NULL, "", false));
                        if(scorehighlight)
                        {
                            int bgcol = o == game::player1 ? 0x888888 : 0x111111;
                            switch(o->privilege)
                            {
                                case PRIV_ADMINISTRATOR: bgcol |= 0x004444; break;
                                case PRIV_MODERATOR: bgcol |= 0x440044; break;
                                case PRIV_HELPER: bgcol |= 0x000044; break;
                                default: break;
                            }
                            g.space(0.25f);
                            g.text("", 0, insigniatex, bgcol);
                        }
                    }));
                    if(!((i+1)%count) && pushed)
                    {
                        g.poplist();
                        pushed = false;
                    }
                }
                if(pushed) g.poplist();
            });
        }
        if(m_play(game::gamemode) && game::player1->state != CS_SPECTATOR && (game::intermission || showscoresinfo))
        {
            float ratio = game::player1->frags >= game::player1->deaths ? (game::player1->frags/float(max(game::player1->deaths, 1))) : -(game::player1->deaths/float(max(game::player1->frags, 1)));
            g.space(0.5f);
            uicenterlist(g, uifont(g, "reduced", {
                g.textf("\fs\fg%d\fS %s, \fs\fg%d\fS %s, \fs\fy%.1f\fS:\fs\fy%.1f\fS ratio, \fs\fg%d\fS damage", 0xFFFFFF, NULL, 0,
                    game::player1->frags, game::player1->frags != 1 ? "frags" : "frag",
                    game::player1->deaths, game::player1->deaths != 1 ? "deaths" : "death", ratio >= 0 ? ratio : 1.f, ratio >= 0 ? 1.f : -ratio,
                    game::player1->totaldamage);
            }));
        }
        g.end();
    }

    int drawscoreitem(const char *icon, int colour, int x, int y, int s, float skew, float fade, int pos, int score, const char *name)
    {
        const char *col = "\fa";
        switch(pos)
        {
            case 0: col = "\fg"; break;
            case 1: col = "\fc"; break;
            case 2: col = "\fy"; break;
        }
        vec c = vec::hexcolor(colour);
        int size = int(s*skew); size += int(size*inventoryglow);
        if(m_defend(game::gamemode) && score == INT_MAX)
            drawitem(icon, x, y+size, s, inventoryscorebg!=0, 0, false, c.r, c.g, c.b, fade, skew, "huge", "%sWIN", col);
        else drawitem(icon, x, y+size, s, inventoryscorebg!=0, 0, false, c.r, c.g, c.b, fade, skew, "huge", "%s%d", col, score);
        drawitemtext(x, y+size, 0, false, skew, "default", fade, "\f[%d]%s", colour, name);
        return size;
    }

    int drawscore(int x, int y, int s, int m, float blend)
    {
        if(!m_fight(game::gamemode) || m_trial(game::gamemode) || (inventoryscore == 1 && game::player1->state == CS_SPECTATOR && game::focus == game::player1)) return 0;
        int sy = 0, numgroups = groupplayers(), numout = 0;
        loopi(2) loopk(numgroups)
        {
            if(sy > m) break;
            scoregroup &sg = *groups[k];
            if(m_fight(game::gamemode) && m_isteam(game::gamemode, game::mutators))
            {
                if(!sg.team || ((sg.team != game::focus->team) == !i)) continue;
                float sk = numout && inventoryscoreshrink > 0 ? 1.f-min(numout*inventoryscoreshrink, inventoryscoreshrinkmax) : 1;
                sy += drawscoreitem(teamtexname(sg.team), TEAM(sg.team, colour), x, y+sy, s, sk*inventoryscoresize, blend*inventoryblend, k, sg.total, TEAM(sg.team, name));
                if(++numout >= inventoryscore) return sy;
            }
            else
            {
                if(sg.team) continue;
                loopvj(sg.players)
                {
                    gameent *d = sg.players[j];
                    if((d != game::focus) == !i) continue;
                    float sk = numout && inventoryscoreshrink > 0 ? 1.f-min(numout*inventoryscoreshrink, inventoryscoreshrinkmax) : 1;
                    sy += drawscoreitem(playertex, game::getcolour(d, game::playerdisplaytone), x, y+sy, s, sk*inventoryscoresize, blend*inventoryblend, j, d->points, game::colorname(d, NULL, "", false));
                    if(++numout >= inventoryscore) return sy;
                }
            }
        }
        return sy;
    }

    int trialinventory(int x, int y, int s, float blend)
    {
        int sy = 0;
        if(groupplayers())
        {
            pushfont("reduced");
            scoregroup &sg = *groups[0];
            if(m_isteam(game::gamemode, game::mutators))
            {
                if(sg.total) sy += draw_textx("\fg%s", x, y, 255, 255, 255, int(blend*255), TEXT_LEFT_UP, -1, -1, timetostr(sg.total));
            }
            else if(!sg.players.empty())
            {
                if(sg.players[0]->cptime) sy += draw_textx("\fg%s", x, y, 255, 255, 255, int(blend*255), TEXT_LEFT_UP, -1, -1, timetostr(sg.players[0]->cptime));
            }
            popfont();
        }
        return sy;
    }

    struct scoreboard : guicb
    {
        void gui(guient &g, bool firstpass)
        {
            renderscoreboard(g, firstpass);
        }
    } sb;

    ICOMMAND(0, showscores, "D", (int *down), showscores(*down!=0, false, false, true));

    void gamemenus()
    {
        if(scoreson) UI::addcb(&sb);
        if(game::player1->state == CS_DEAD) { if(scoreson) shownscores = true; }
        else shownscores = false;
    }
}
