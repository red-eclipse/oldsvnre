// server-side ai manager
namespace aiman
{
    int oldteambalance = -1, oldskillmin = -1, oldskillmax = -1, oldbotbalance = -2, oldbotlimit = -1;

    int findaiclient(int exclude)
    {
        vector<int> siblings;
        while(siblings.length() < clients.length()) siblings.add(-1);
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            if(ci->clientnum < 0 || ci->state.aitype >= 0 || !ci->name[0] || !ci->connected || ci->clientnum == exclude)
                siblings[i] = -1;
            else
            {
                siblings[i] = 0;
                loopvj(clients) if(clients[j]->state.aitype >= 0 && clients[j]->state.ownernum == ci->clientnum)
                    siblings[i]++;
            }
        }
        while(!siblings.empty())
        {
            int q = -1;
            loopv(siblings)
                if(siblings[i] >= 0 && (!siblings.inrange(q) || siblings[i] < siblings[q]))
                    q = i;
            if(siblings.inrange(q))
            {
                if(clients.inrange(q)) return clients[q]->clientnum;
                else siblings.removeunordered(q);
            }
            else break;
        }
        return -1;
    }

    bool addai(int type, int ent, int skill)
    {
        int numbots = 0;
        loopv(clients)
        {
            if(type == AI_BOT && numbots >= GAME(botlimit)) return false;
            if(clients[i]->state.aitype == type)
            {
                clientinfo *ci = clients[i];
                if(ci->state.ownernum < 0)
                { // reuse a slot that was going to removed
                    ci->state.ownernum = findaiclient();
                    ci->state.aireinit = 1;
                    ci->state.aitype = type;
                    ci->state.aientity = ent;
                    return true;
                }
                if(type == AI_BOT) numbots++;
            }
        }
        if(type == AI_BOT && numbots >= GAME(botlimit)) return false;
        int cn = addclient(ST_REMOTE);
        if(cn >= 0)
        {
            clientinfo *ci = (clientinfo *)getinfo(cn);
            if(ci)
            {
                int s = skill, m = max(GAME(skillmax), GAME(skillmin)), n = min(GAME(skillmin), m);
                if(skill > m || skill < n) s = (m != n ? rnd(m-n) + n + 1 : m);
                ci->clientnum = cn;
                ci->state.ownernum = findaiclient();
                ci->state.aireinit = 2;
                ci->state.aitype = type;
                ci->state.aientity = ent;
                ci->state.skill = clamp(s, 1, 101);
                clients.add(ci);
                ci->state.lasttimeplayed = lastmillis;
                copystring(ci->name, aistyle[ci->state.aitype].name, MAXNAMELEN);
                ci->state.state = CS_DEAD;
                ci->team = type == AI_BOT ? TEAM_NEUTRAL : TEAM_ENEMY;
                ci->online = ci->connected = true;
                return true;
            }
            delclient(cn);
        }
        return false;
    }

    void deleteai(clientinfo *ci)
    {
        if(ci->state.aitype < 0) return;
        int cn = ci->clientnum;
        loopv(clients) if(clients[i] != ci)
        {
            loopvk(clients[i]->state.fraglog) if(clients[i]->state.fraglog[k] == ci->clientnum)
                clients[i]->state.fraglog.remove(k--);
        }
        if(ci->state.state == CS_ALIVE) dropitems(ci, 0);
        if(smode) smode->leavegame(ci, true);
        mutate(smuts, mut->leavegame(ci, true));
        ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
        distpoints(ci, true); savescore(ci);
        sendf(-1, 1, "ri3", N_DISCONNECT, cn, DISC_NONE);
        clients.removeobj(ci);
        delclient(cn);
        dorefresh = true;
    }

    bool delai(int type)
    {
        loopvrev(clients) if(clients[i]->state.aitype == type && clients[i]->state.ownernum >= 0)
        {
            deleteai(clients[i]);
            return true;
        }
        return false;
    }

    void reinitai(clientinfo *ci)
    {
        if(ci->state.aitype < 0) return;
        if(ci->state.ownernum < 0) deleteai(ci);
        else if(ci->state.aireinit >= 1)
        {
            if(ci->state.aireinit == 2)
            {
                ci->state.dropped.reset();
                loopk(WEAP_MAX) loopj(2) ci->state.weapshots[k][j].reset();
            }
            sendf(-1, 1, "ri6si", N_INITAI, ci->clientnum, ci->state.ownernum, ci->state.aitype, ci->state.aientity, ci->state.skill, ci->name, ci->team);
            if(ci->state.aireinit == 2)
            {
                waiting(ci, 1, 2);
                if(smode) smode->entergame(ci);
                mutate(smuts, mut->entergame(ci));
            }
            ci->state.aireinit = 0;
        }
    }

    void shiftai(clientinfo *ci, int cn = -1)
    {
        if(cn < 0) { ci->state.aireinit = 0; ci->state.ownernum = -1; }
        else if(ci->state.ownernum != cn) { ci->state.aireinit = 1; ci->state.ownernum = cn; }
    }

    void removeai(clientinfo *ci, bool complete)
    { // either schedules a removal, or someone else to assign to
        loopv(clients) if(clients[i]->state.aitype >= 0 && clients[i]->state.ownernum == ci->clientnum)
            shiftai(clients[i], complete ? -1 : findaiclient(ci->clientnum));
    }

    bool reassignai(int exclude)
    {
        vector<int> siblings;
        while(siblings.length() < clients.length()) siblings.add(-1);
        int hi = -1, lo = -1;
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            if(ci->clientnum < 0 || ci->state.aitype >= 0 || !ci->name[0] || !ci->connected || ci->clientnum == exclude)
                siblings[i] = -1;
            else
            {
                siblings[i] = 0;
                loopvj(clients) if(clients[j]->state.aitype >= 0 && clients[j]->state.ownernum == ci->clientnum)
                    siblings[i]++;
                if(!siblings.inrange(hi) || siblings[i] > siblings[hi]) hi = i;
                if(!siblings.inrange(lo) || siblings[i] < siblings[lo]) lo = i;
            }
        }
        if(siblings.inrange(hi) && siblings.inrange(lo) && (siblings[hi]-siblings[lo]) > 1)
        {
            clientinfo *ci = clients[hi];
            loopv(clients) if(clients[i]->state.aitype >= 0 && clients[i]->state.ownernum == ci->clientnum)
            {
                shiftai(clients[i], clients[lo]->clientnum);
                return true;
            }
        }
        return false;
    }

    void checksetup()
    {
        int m = max(GAME(skillmax), GAME(skillmin)), n = min(GAME(skillmin), m), numbots = 0;
        loopv(clients) if(clients[i]->state.aitype >= 0 && clients[i]->state.ownernum >= 0)
        {
            clientinfo *ci = clients[i];
            int o = clamp(m, 1, 101), p = clamp(n, 1, 101);
            if(ci->state.skill > o || ci->state.skill < p)
            { // needs re-skilling
                ci->state.skill = (o != p ? rnd(o-p) + p + 1 : o);
                if(!ci->state.aireinit) ci->state.aireinit = 1;
            }
            if(ci->state.aitype == AI_BOT && ++numbots >= GAME(botlimit)) shiftai(ci, -1);
        }

        int balance = 0;
        if(m_campaign(gamemode)) balance = GAME(campaignplayers); // campaigns strictly obeys nplayers
        else if(m_fight(gamemode) && !m_trial(gamemode) && GAME(botlimit) > 0)
        {
            int numt = numteams(gamemode, mutators), people = numclients(-1, true, -1);
            switch(GAME(botbalance))
            {
                case -1: balance = max(people, m_duel(gamemode, mutators) ? 2 : nplayers); break; // use distributed numplayers
                case  0: balance = 0; break; // no bots
                default: balance = max(people, m_duel(gamemode, mutators) ? 2 : GAME(botbalance)); break; // balance to at least this
            }
            if(m_team(gamemode, mutators) && (balance > 0 || GAME(teambalance) == 3))
            { // skew this if teams are unbalanced
                if(GAME(teambalance) != 3)
                {
                    int teamscores[TEAM_NUM] = {0}, highest = -1;
                    loopv(clients) if(clients[i]->state.aitype < 0 && clients[i]->team >= TEAM_FIRST && isteam(gamemode, mutators, clients[i]->team, TEAM_FIRST))
                    {
                        int team = clients[i]->team-TEAM_FIRST;
                        teamscores[team]++;
                        if(highest < 0 || teamscores[team] > teamscores[highest]) highest = team;
                    }
                    if(highest >= 0)
                    {
                        int bots = balance-people;
                        loopi(numt) if(i != highest && teamscores[i] < teamscores[highest]) loopj(teamscores[highest]-teamscores[i])
                        {
                            if(bots > 0) bots--;
                            else balance++;
                        }
                    }
                }
                else balance = max(people*numt, numt); // humans vs. bots, just directly balance
                loopvrev(clients)
                {
                    clientinfo *ci = clients[i];
                    if(ci->state.aitype == AI_BOT && ci->state.ownernum >= 0)
                    {
                        if(numclients(-1, true, AI_BOT) > balance) shiftai(ci, -1);
                        else
                        {
                            int teamb = chooseteam(ci, ci->team);
                            if(ci->team != teamb) setteam(ci, teamb, true, true);
                        }
                    }
                }
            }
            int bots = balance-people;
            if(bots > GAME(botlimit))
            {
                balance -= bots-GAME(botlimit);
                if(m_team(gamemode, mutators)) balance -= balance%numt;
            }
        }
        if(balance > 0)
        {
            while(numclients(-1, true, AI_BOT) < balance) if(!addai(AI_BOT, -1, -1)) break;
            while(numclients(-1, true, AI_BOT) > balance) if(!delai(AI_BOT)) break;
        }
        else clearai(1);
    }

    void checkenemies()
    {
        if(GAME(enemybalance) && GAME(enemyallowed) >= (m_campaign(gamemode) ? 0 : (m_insta(gamemode, mutators) ? 2 : 1)))
        {
            loopvj(sents) if(sents[j].type == ACTOR && sents[j].attrs[0] >= AI_START && sents[j].attrs[0] < AI_MAX && gamemillis >= sents[j].millis && (sents[j].attrs[4] == triggerid || !sents[j].attrs[4]) && m_check(sents[j].attrs[3], gamemode))
            {
                int count = 0;
                loopvrev(clients) if(clients[i]->state.aientity == j)
                {
                    count++;
                    if(count > GAME(enemybalance)) deleteai(clients[i]);
                }
                if(count < GAME(enemybalance))
                {
                    int amt = GAME(enemybalance)-count;
                    loopk(amt) addai(sents[j].attrs[0], j, -1);
                }
            }
        }
        else clearai(2);
    }

    void clearai(int type)
    { // clear and remove all ai immediately
        loopvrev(clients) if(type ? (type == 2 ? clients[i]->state.aitype >= AI_START : clients[i]->state.aitype == AI_BOT) : true)
            deleteai(clients[i]);
        dorefresh = false;
    }

    void checkai()
    {
        if(m_play(gamemode) && numclients())
        {
            if(hasgameinfo && !interm)
            {
                #define checkold(n) if(old##n != GAME(n)) { dorefresh = true; old##n = GAME(n); }
                checkold(teambalance);
                checkold(skillmin);
                checkold(skillmax);
                checkold(botbalance);
                checkold(botlimit);
                if(dorefresh) { checksetup(); dorefresh = false; }
                checkenemies();
                loopvrev(clients) if(clients[i]->state.aitype >= 0) reinitai(clients[i]);
                while(true) if(!reassignai()) break;
            }
        }
        else clearai();
    }
}
