namespace hud
{
    struct scoreboard : guicb
    {
        bool scoreson, scoresoff, shownscores;
        int menustart, menulastpress;

        struct sline { string s; };
        struct teamscore
        {
            int team, score;
            teamscore() {}
            teamscore(int s, int n) : team(s), score(n) {}
        };
        struct scoregroup : teamscore
        {
            vector<gameent *> players;
        };
        vector<scoregroup *> groups;
        vector<gameent *> spectators;

        IVAR(IDF_PERSIST, autoshowscores, 0, 2, 3); // 1 = when dead, 2 = also in spectv, 3 = and in waittv too
        IVAR(IDF_PERSIST, showscoresdelay, 0, 0, INT_MAX-1); // otherwise use a static timespan
        IVAR(IDF_PERSIST, showscoresinfo, 0, 1, 1);
        IVAR(IDF_PERSIST, highlightscore, 0, 1, 1);

        IVAR(IDF_PERSIST, showpj, 0, 0, 1);
        IVAR(IDF_PERSIST, showping, 0, 1, 1);
        IVAR(IDF_PERSIST, showpoints, 0, 1, 1);
        IVAR(IDF_PERSIST, showscore, 0, 2, 2);
        IVAR(IDF_PERSIST, showclientnum, 0, 1, 1);
        IVAR(IDF_PERSIST, showskills, 0, 1, 1);
        IVAR(IDF_PERSIST, showownernum, 0, 0, 1);
        IVAR(IDF_PERSIST, showspectators, 0, 1, 1);
        IVAR(IDF_PERSIST, showconnecting, 0, 0, 1);

        scoreboard() : scoreson(false), scoresoff(false), shownscores(false), menulastpress(0)
        {
            CCOMMAND(0, showscores, "D", (scoreboard *self, int *down), self->showscores(*down!=0, false, false, true));
        }

        bool canshowscores()
        {
            if(!scoresoff && !scoreson && !shownscores && autoshowscores() && (game::player1->state == CS_DEAD || (autoshowscores() >= (game::player1->state == CS_SPECTATOR ? 2 : 3) && game::tvmode())))
            {
                if(game::tvmode()) return true;
                else if(game::player1->state == CS_DEAD)
                {
                    int delay = !showscoresdelay() ? m_delay(game::gamemode, game::mutators) : showscoresdelay();
                    if(!delay || lastmillis-game::player1->lastdeath > delay) return true;
                }
            }
            return false;
        }

        void showscores(bool on, bool interm = false, bool onauto = true, bool ispress = false)
        {
            if(client::ready())
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
                if(!scoreson && on) menustart = starttime();
                scoresoff = !onauto;
                scoreson = on;
                if(m_play(game::gamemode) && interm)
                {
                    if(m_campaign(game::gamemode)) game::announce(S_V_MCOMPLETE, CON_MESG, game::player1, "\fwmission completed");
                    else if(m_fight(game::gamemode) && !m_trial(game::gamemode))
                    {
                        if(!groupplayers()) return;
                        scoregroup &sg = *groups[0];
                        if(m_fight(game::gamemode) && m_team(game::gamemode, game::mutators))
                        {
                            int anc = sg.players.find(game::player1) >= 0 ? S_V_YOUWIN : (game::player1->state != CS_SPECTATOR ? S_V_YOULOSE : -1);
                            if(m_stf(game::gamemode) && sg.score==INT_MAX)
                                game::announce(anc, CON_MESG, game::player1, "\fw\fs%s%s\fS team secured all flags", teamtype[sg.team].chat, teamtype[sg.team].name);
                            else if(m_trial(game::gamemode)) game::announce(anc, CON_MESG, game::player1, "\fw\fs%s%s\fS team won the match with the fastest lap: \fs\fc%s\fS", teamtype[sg.team].chat, teamtype[sg.team].name, sg.score ? timetostr(sg.score) : "dnf");
                            else game::announce(anc, CON_MESG, game::player1, "\fw\fs%s%s\fS team won the match with a total score of: \fs\fc%d\fS", teamtype[sg.team].chat, teamtype[sg.team].name, sg.score);
                        }
                        else
                        {
                            int anc = sg.players[0] == game::player1 ? S_V_YOUWIN : (game::player1->state != CS_SPECTATOR ? S_V_YOULOSE : -1);
                            if(m_trial(game::gamemode)) game::announce(anc, CON_MESG, game::player1, "\fw%s won the match with the fastest lap: \fs\fc%s\fS", game::colorname(sg.players[0]), sg.players[0]->cptime ? timetostr(sg.players[0]->cptime) : "dnf");
                            else game::announce(anc, CON_MESG, game::player1, "\fw%s won the match with a total score of: \fs\fc%d\fS", game::colorname(sg.players[0]), sg.players[0]->points);
                        }
                    }
                }
            }
            else scoresoff = scoreson = false;
        }

        static int teamscorecmp(const teamscore *x, const teamscore *y)
        {
            if(m_trial(game::gamemode))
            {
                if((x->score && !y->score) || (x->score && y->score && x->score < y->score)) return -1;
                if((y->score && !x->score) || (x->score && y->score && y->score < x->score)) return 1;
            }
            else
            {
                if(x->score > y->score) return -1;
                if(x->score < y->score) return 1;
            }
            return x->team-y->team;
        }

        static int playersort(const gameent **a, const gameent **b)
        {
            if((*a)->state==CS_SPECTATOR)
            {
                if((*b)->state==CS_SPECTATOR) return strcmp((*a)->name, (*b)->name);
                else return 1;
            }
            else if((*b)->state==CS_SPECTATOR) return -1;
            if(m_trial(game::gamemode))
            {
                if(((*a)->cptime && !(*b)->cptime) || ((*a)->cptime && (*b)->cptime && (*a)->cptime < (*b)->cptime)) return -1;
                if(((*b)->cptime && !(*a)->cptime) || ((*a)->cptime && (*b)->cptime && (*b)->cptime < (*a)->cptime)) return 1;
            }
            if((*a)->points > (*b)->points) return -1;
            if((*a)->points < (*b)->points) return 1;
            if((*a)->frags > (*b)->frags) return -1;
            if((*a)->frags < (*b)->frags) return 1;
            return strcmp((*a)->name, (*b)->name);
        }

        void bestplayers(vector<gameent *> &best)
        {
            loopi(game::numdynents())
            {
                gameent *o = (gameent *)game::iterdynents(i);
                if(o && o->type==ENT_PLAYER && o->state!=CS_SPECTATOR) best.add(o);
            }
            best.sort(playersort);
            while(best.length()>1 && best.last()->points < best[0]->points) best.drop();
        }

        void sortteams(vector<teamscore> &teamscores)
        {
            if(m_stf(game::gamemode))
            {
                loopv(stf::st.scores) teamscores.add(teamscore(stf::st.scores[i].team, stf::st.scores[i].total));
            }
            else if(m_ctf(game::gamemode))
            {
                loopv(ctf::st.scores) teamscores.add(teamscore(ctf::st.scores[i].team, ctf::st.scores[i].total));
            }
            loopi(game::numdynents())
            {
                gameent *o = (gameent *)game::iterdynents(i);
                if(o && o->type==ENT_PLAYER)
                {
                    teamscore *ts = NULL;
                    loopv(teamscores) if(teamscores[i].team == o->team) { ts = &teamscores[i]; break; }
                    if(!ts) teamscores.add(teamscore(o->team, m_stf(game::gamemode) || m_ctf(game::gamemode) ? 0 : (m_trial(game::gamemode) ? o->cptime : o->points)));
                    else if(m_trial(game::gamemode)) { if(o->cptime && (!ts->score || o->cptime < ts->score)) ts->score = o->cptime; }
                    else if(!m_stf(game::gamemode) && !m_ctf(game::gamemode)) ts->score += o->points;
                }
            }
            teamscores.sort(teamscorecmp);
        }

        void bestteams(vector<int> &best)
        {
            vector<teamscore> teamscores;
            sortteams(teamscores);
            while(teamscores.length()>1 && teamscores.last().score < teamscores[0].score) teamscores.drop();
            loopv(teamscores) best.add(teamscores[i].team);
        }

        static int scoregroupcmp(const scoregroup **x, const scoregroup **y)
        {
            if(!(*x)->team)
            {
                if((*y)->team) return 1;
            }
            else if(!(*y)->team) return -1;
            if((*x)->score > (*y)->score) return -1;
            if((*x)->score < (*y)->score) return 1;
            if((*x)->players.length() > (*y)->players.length()) return -1;
            if((*x)->players.length() < (*y)->players.length()) return 1;
            return (*x)->team && (*y)->team ? (*x)->team-(*y)->team : 0;
        }

        int groupplayers()
        {
            int numgroups = 0;
            spectators.shrink(0);
            loopi(game::numdynents())
            {
                gameent *o = (gameent *)game::iterdynents(i);
                if(!o || o->type!=ENT_PLAYER || (!showconnecting() && !o->name[0])) continue;
                if(o->state==CS_SPECTATOR) { spectators.add(o); continue; }
                int team = m_fight(game::gamemode) && m_team(game::gamemode, game::mutators) ? o->team : TEAM_NEUTRAL;
                bool found = false;
                loopj(numgroups)
                {
                    scoregroup &g = *groups[j];
                    if(team != g.team) continue;
                    if(team && !m_stf(game::gamemode) && !m_ctf(game::gamemode)) g.score += o->points;
                    g.players.add(o);
                    found = true;
                }
                if(found) continue;
                if(numgroups>=groups.length()) groups.add(new scoregroup);
                scoregroup &g = *groups[numgroups++];
                g.team = team;
                if(!team) g.score = 0;
                else if(m_stf(game::gamemode)) g.score = stf::st.findscore(o->team).total;
                else if(m_ctf(game::gamemode)) g.score = ctf::st.findscore(o->team).total;
                else g.score = o->points;

                g.players.shrink(0);
                g.players.add(o);
            }
            loopi(numgroups) groups[i]->players.sort(playersort);
            spectators.sort(playersort);
            groups.sort(scoregroupcmp, 0, numgroups);
            return numgroups;
        }

        void gui(guient &g, bool firstpass)
        {
            g.start(menustart, menuscale, NULL, false);
            int numgroups = groupplayers();
            g.pushlist();
            g.image(NULL, 7, true);
            g.space(2);
            g.pushlist();
            g.space(1);
            g.pushfont("super");
            if(*maptitle) g.textf("%s", 0xFFFFFF, NULL, maptitle);
            else g.textf("(%s)", 0xFFFFFF, NULL, mapname);
            g.popfont();
            if(*mapauthor)
            {
                g.pushlist();
                g.space(3);
                g.pushfont("emphasis");
                g.textf("by %s", 0xFFFFFF, NULL, mapauthor);
                g.popfont();
                g.poplist();
            }
            g.pushlist();
            g.pushfont("default");
            g.textf("%s", 0xFFFFFF, NULL, server::gamename(game::gamemode, game::mutators));
            if((m_play(game::gamemode) || client::demoplayback) && game::timeremaining >= 0)
            {
                if(!game::timeremaining) g.textf(", \fs\fyintermission\fS", 0xFFFFFF, NULL);
                else if(paused) g.textf(", \fs\fopaused\fS", 0xFFFFFF, NULL);
                else g.textf(", \fs\fg%s\fS remain", 0xFFFFFF, NULL, hud::timetostr(game::timeremaining, 2));
            }
            g.popfont();
            g.poplist();
            if(*serveraddress)
            {
                g.pushlist();
                g.space(2);
                g.pushfont("radar");
                g.textf("\fdon ", 0xFFFFFF, NULL);
                g.popfont();
                if(*serverdesc)
                {
                    g.pushfont("sub");
                    g.textf("%s ", 0xFFFFFF, NULL, serverdesc);
                    g.popfont();
                }
                g.pushfont("radar");
                g.textf("\fd(\fa%s:[%d]\fd)", 0xFFFFFF, NULL, serveraddress, serverconport);
                g.popfont();
                g.poplist();
            }

            if(game::player1->state == CS_DEAD || game::player1->state == CS_WAITING)
            {
                int sdelay = m_delay(game::gamemode, game::mutators), delay = game::player1->lastdeath ? game::player1->respawnwait(lastmillis, sdelay) : 0;
                const char *msg = game::player1->state != CS_WAITING && game::player1->lastdeath ? "Fragged" : "Please Wait";
                g.space(1);
                g.pushlist();
                g.pushfont("default"); g.textf("%s", 0xFFFFFF, NULL, msg); g.popfont();
                g.space(2);
                SEARCHBINDCACHE(attackkey)("action 0", 0);
                g.pushfont("sub");
                if(delay || m_campaign(game::gamemode) || (m_trial(game::gamemode) && !game::player1->lastdeath) || m_duke(game::gamemode, game::mutators))
                {
                    if(m_duke(game::gamemode, game::mutators)) g.textf("Queued for new round", 0xFFFFFF, NULL);
                    else if(delay) g.textf("Down for \fs\fy%s\fS", 0xFFFFFF, NULL, hud::timetostr(delay, -1));
                    g.poplist();
                    if(game::player1->state != CS_WAITING && lastmillis-game::player1->lastdeath > 500)
                        g.textf("Press \fs\fc%s\fS to enter respawn queue", 0xFFFFFF, NULL, attackkey);
                }
                else
                {
                    g.textf("Ready to respawn", 0xFFFFFF, NULL);
                    g.poplist();
                    if(game::player1->state != CS_WAITING) g.textf("Press \fs\fc%s\fS to respawn now", 0xFFFFFF, NULL, attackkey);
                }
                if(game::player1->state == CS_WAITING && lastmillis-game::player1->lastdeath >= 500)
                {
                    SEARCHBINDCACHE(waitmodekey)("waitmodeswitch", 3);
                    g.textf("Press \fs\fc%s\fS to enter respawn queue", 0xFFFFFF, NULL, waitmodekey);
                }
                if(m_arena(game::gamemode, game::mutators))
                {
                    SEARCHBINDCACHE(loadkey)("showgui loadout", 0);
                    g.textf("Press \fs\fc%s\fS to \fs%s\fS loadouts", 0xFFFFFF, NULL, loadkey, game::player1->loadweap < 0 ? "\fzoyselect" : "change");
                }
                if(m_fight(game::gamemode) && m_team(game::gamemode, game::mutators))
                {
                    SEARCHBINDCACHE(teamkey)("showgui team", 0);
                    g.textf("Press \fs\fc%s\fS to change teams", 0xFFFFFF, NULL, teamkey);
                }
                g.popfont();
            }
            else if(game::player1->state == CS_ALIVE)
            {
                g.space(1);
                g.pushfont("default");
                if(m_edit(game::gamemode)) g.textf("Map Editing", 0xFFFFFF, NULL);
                else if(m_campaign(game::gamemode)) g.textf("Campaign", 0xFFFFFF, NULL);
                else if(m_team(game::gamemode, game::mutators))
                    g.textf("Team \fs%s%s\fS", 0xFFFFFF, NULL, teamtype[game::player1->team].chat, teamtype[game::player1->team].name);
                else g.textf("Free for All", 0xFFFFFF, NULL);
                g.popfont();
            }
            else if(game::player1->state == CS_SPECTATOR)
            {
                g.space(1);
                g.pushfont("default"); g.textf("%s", 0xFFFFFF, NULL, game::tvmode() ? "SpecTV" : "Spectating"); g.popfont();
                SEARCHBINDCACHE(speconkey)("spectator 0", 1);
                g.pushfont("sub");
                g.textf("Press \fs\fc%s\fS to join the game", 0xFFFFFF, NULL, speconkey);
                SEARCHBINDCACHE(specmodekey)("specmodeswitch", 1);
                g.textf("Press \fs\fc%s\fS to %s", 0xFFFFFF, NULL, specmodekey, game::tvmode() ? "interact" : "switch to TV");
                g.popfont();
            }

            if((game::player1->state == CS_WAITING || game::player1->state == CS_SPECTATOR) && !game::tvmode())
            {
                g.pushfont("radar");
                SEARCHBINDCACHE(uvf1key)("universaldelta 1", game::player1->state == CS_WAITING ? 3 : 1);
                SEARCHBINDCACHE(uvf2key)("universaldelta -1", game::player1->state == CS_WAITING ? 3 : 1);
                g.textf("Press \fs\fc%s\fS and \fs\fc%s\fS to change views", 0xFFFFFF, NULL, uvf1key, uvf2key);
                g.popfont();
            }

            SEARCHBINDCACHE(scoreboardkey)("showscores", 1);
            g.pushfont("sub");
            g.textf("%s \fs\fc%s\fS to close this window", 0xFFFFFF, NULL, scoresoff ? "Release" : "Press", scoreboardkey);
            g.popfont();
            g.pushlist();
            g.space(2);
            g.pushfont("radar");
            g.textf("Double-tap to keep the window open", 0xFFFFFF, NULL);
            g.popfont();
            g.poplist();
            g.poplist();
            g.poplist();
            g.space(1);
            loopk(numgroups)
            {
                if((k%2)==0) g.pushlist(); // horizontal

                scoregroup &sg = *groups[k];
                int bgcolor = sg.team && m_fight(game::gamemode) && m_team(game::gamemode, game::mutators) ? teamtype[sg.team].colour : 0,
                    fgcolor = 0xFFFFFF;

                g.pushlist(); // vertical
                g.pushlist(); // horizontal

                #define loopscoregroup(b) \
                    loopv(sg.players) \
                    { \
                        gameent *o = sg.players[i]; \
                        b; \
                    }

                g.pushlist();
                if(sg.team && m_fight(game::gamemode) && m_team(game::gamemode, game::mutators))
                {
                    g.pushlist();
                    g.background(bgcolor, numgroups>1 ? 3 : 5);
                    g.text("", 0, hud::teamtex(sg.team));
                    g.poplist();
                }
                g.pushlist();
                g.strut(1);
                g.poplist();
                loopscoregroup({
                    const char *status = hud::playertex;
                    if(o->state == CS_DEAD || o->state == CS_WAITING) status = hud::deadtex;
                    else if(o->state == CS_ALIVE)
                    {
                        if(o->dominating.find(game::focus) >= 0) status = hud::dominatingtex;
                        else if(o->dominated.find(game::focus) >= 0) status = hud::dominatedtex;
                    }
                    int bgcol = o==game::player1 && highlightscore() ? 0x888888 : 0;
                    if(o->privilege) bgcol |= o->privilege >= PRIV_ADMIN ? 0x226622 : 0x666622;
                    g.pushlist();
                    if(bgcol) g.background(bgcol, 3);
                    g.text("", 0, status);
                    g.poplist();
                });
                g.poplist();

                if(sg.team && m_fight(game::gamemode) && m_team(game::gamemode, game::mutators))
                {
                    g.pushlist(); // vertical
                    if(m_stf(game::gamemode) && stflimit && sg.score >= stflimit) g.textf("%s: WIN", fgcolor, NULL, teamtype[sg.team].name);
                    else g.textf("%s: %d", fgcolor, NULL, teamtype[sg.team].name, sg.score);
                    g.pushlist(); // horizontal
                }

                g.pushlist();
                g.pushlist();
                g.text("name ", fgcolor);
                g.poplist();
                loopscoregroup(g.textf("%s ", 0xFFFFFF, NULL, game::colorname(o, NULL, "", false)));
                g.poplist();

                if(showpoints())
                {
                    g.pushlist();
                    g.strut(7);
                    g.text("points", fgcolor);
                    loopscoregroup(g.textf("%d", 0xFFFFFF, NULL, o->points));
                    g.poplist();
                }

                if(showscore() && (showscore() >= 2 || !m_flag(game::gamemode)))
                {
                    g.pushlist();
                    if(m_trial(game::gamemode))
                    {
                        g.strut(10);
                        g.text("best lap", fgcolor);
                        loopscoregroup(g.textf("%s", 0xFFFFFF, NULL, o->cptime ? timetostr(o->cptime) : "\fadnf"));
                    }
                    else
                    {
                        g.strut(6);
                        g.text("frags", fgcolor);
                        loopscoregroup(g.textf("%d", 0xFFFFFF, NULL, o->frags));
                    }
                    g.poplist();
                }

                if(showpj())
                {
                    g.pushlist();
                    g.strut(5);
                    g.text("pj", fgcolor);
                    loopscoregroup({
                        g.textf("%d", 0xFFFFFF, NULL, o->plag);
                    });
                    g.poplist();
                }

                if(showping())
                {
                    g.pushlist();
                    g.strut(5);
                    g.text("ping", fgcolor);
                    loopscoregroup({
                        if(o->aitype >= 0)
                        {
                            gameent *od = game::getclient(o->ownernum);
                            g.textf("%d", 0xFFFFFF, NULL, od ? od->ping : -1);
                        }
                        else g.textf("%d", 0xFFFFFF, NULL, o->ping);
                    });
                    g.poplist();
                }

                if(showclientnum() || game::player1->privilege>=PRIV_MASTER)
                {
                    g.pushlist();
                    g.strut(4);
                    g.text("cn", fgcolor);
                    loopscoregroup({
                        g.textf("%d", 0xFFFFFF, NULL, o->clientnum);
                    });
                    g.poplist();
                }

                if(showskills())
                {
                    g.pushlist();
                    g.strut(3);
                    g.text("sk", fgcolor);
                    loopscoregroup({
                        if(o->aitype >= 0) g.textf("%d", 0xFFFFFF, NULL, o->skill);
                        else g.space(1);
                    });
                    g.poplist();
                }

                if(showownernum())
                {
                    g.pushlist();
                    g.strut(3);
                    g.text("on", fgcolor);
                    loopscoregroup({
                        if(o->aitype >= 0) g.textf("%d", 0xFFFFFF, NULL, o->ownernum);
                        else g.space(1);
                    });
                    g.poplist();
                }

                if(sg.team && m_fight(game::gamemode) && m_team(game::gamemode, game::mutators))
                {
                    g.poplist(); // horizontal
                    g.poplist(); // vertical
                }

                g.poplist(); // horizontal
                g.poplist(); // vertical

                if(k+1<numgroups && (k+1)%2) g.space(2);
                else g.poplist(); // horizontal
            }

            if(showspectators() && spectators.length())
            {
                g.space(1);
                loopv(spectators)
                {
                    gameent *o = spectators[i];
                    int bgcol = o==game::player1 && highlightscore() ? 0x888888 : 0;
                    if(o->privilege) bgcol |= o->privilege >= PRIV_ADMIN ? 0x226622 : 0x666622;
                    if((i%3)==0) g.pushlist();
                    if(bgcol) g.background(bgcol, -1);
                    if(showclientnum() || game::player1->privilege>=PRIV_MASTER)
                        g.textf("%s (%d)", 0xFFFFFF, hud::conopentex, game::colorname(o, NULL, "", false), o->clientnum);
                    else g.textf("%s", 0xFFFFFF, hud::conopentex, game::colorname(o, NULL, "", false));
                    if(i+1<spectators.length() && (i+1)%3) g.space(1);
                    else g.poplist();
                }
            }
            if(m_play(game::gamemode) && game::player1->state != CS_SPECTATOR && (game::intermission || showscoresinfo()))
            {
                float ratio = game::player1->frags >= game::player1->deaths ? (game::player1->frags/float(max(game::player1->deaths, 1))) : -(game::player1->deaths/float(max(game::player1->frags, 1)));
                g.space(1);
                g.pushfont("sub");
                g.textf("\fs\fg%d\fS %s, \fs\fg%d\fS %s, \fs\fy%.1f\fS:\fs\fy%.1f\fS ratio, \fs\fg%d\fS damage", 0xFFFFFF, NULL,
                    game::player1->frags, game::player1->frags != 1 ? "frags" : "frag",
                    game::player1->deaths, game::player1->deaths != 1 ? "deaths" : "death", ratio >= 0 ? ratio : 1.f, ratio >= 0 ? 1.f : -ratio,
                    game::player1->totaldamage);
                g.popfont();
            }
            g.end();
        }

        void show()
        {
            if(scoreson) UI::addcb(this);
            if(game::player1->state == CS_DEAD) { if(scoreson) shownscores = true; }
            else shownscores = false;
        }

        int drawinventoryitem(int x, int y, int s, float skew, float fade, int pos, int team, int score, const char *name)
        {
            const char *colour = "\fa";
            switch(pos)
            {
                case 0: colour = "\fg"; break;
                case 1: colour = "\fy"; break;
                case 2: colour = "\fo"; break;
                case 3: colour = "\fr"; break;
                default: break;
            }
            int sy = hud::drawitem(hud::inventoryhinttex, x, y, s-s/4, false, 1.f, 1.f, 1.f, fade, skew, "default", "\fs%s[\fS%d\fs%s]\fS", colour, score, colour);
            hud::drawitemsubtext(x, y, s, TEXT_RIGHT_UP, skew, "sub", fade, "%s%s", teamtype[team].chat, name);
            return sy;
        }

        int drawinventory(int x, int y, int s, int m, float blend)
        {
            if(!m_fight(game::gamemode) || m_trial(game::gamemode)) return 0;
            int sy = 0, numgroups = groupplayers(), numout = 0;
            loopi(2) loopk(numgroups)
            {
                if(y-sy-s < m) break;
                scoregroup &sg = *groups[k];
                if(m_fight(game::gamemode) && m_team(game::gamemode, game::mutators))
                {
                    if(!sg.team || ((sg.team != game::focus->team) == !i)) continue;
                    if(!sy) sy += s/8;
                    sy += drawinventoryitem(x, y-sy, s-s/4, 1.25f-clamp(numout,1,3)*0.25f*inventoryskew, blend*inventoryblend, k, sg.team, sg.score, teamtype[sg.team].name);
                    if((numout += 1) > 3) return sy;
                }
                else
                {
                    if(sg.team) continue;
                    loopvj(sg.players)
                    {
                        gameent *d = sg.players[j];
                        if((d != game::focus) == !i) continue;
                        if(!sy) sy += s/8;
                        sy += drawinventoryitem(x, y-sy, s-s/4, 1.25f-clamp(numout,1,3)*0.25f*inventoryskew, blend*inventoryblend, j, sg.team, d->points, game::colorname(d, NULL, "", false));
                        if((numout += 1) > 3) return sy;
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
                pushfont("sub");
                scoregroup &sg = *groups[0];
                if(m_team(game::gamemode, game::mutators))
                {
                    if(sg.score) sy += draw_textx("\fg%s", x, y, 255, 255, 255, int(blend*255), TEXT_LEFT_UP, -1, -1, timetostr(sg.score));
                }
                else if(!sg.players.empty())
                {
                    if(sg.players[0]->cptime) sy += draw_textx("\fg%s", x, y, 255, 255, 255, int(blend*255), TEXT_LEFT_UP, -1, -1, timetostr(sg.players[0]->cptime));
                }
                popfont();
            }
            return sy;
        }
    };
    extern scoreboard sb;
}
