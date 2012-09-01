GVAR(IDF_ADMIN, serverdebug, 0, 0, 3);
GVAR(IDF_ADMIN, serverclients, 1, 16, MAXCLIENTS);
GVAR(IDF_ADMIN, serveropen, 0, 3, 3);
GSVAR(IDF_ADMIN, serverdesc, "");
GSVAR(IDF_ADMIN, servermotd, "");

GVAR(IDF_ADMIN, autoadmin, 0, 0, 1);

GVAR(IDF_ADMIN, demolock, 0, 2, 3); // 0 = helper, 1 = moderator, 2 = administrator, 3 = nobody
GVAR(IDF_ADMIN, speclock, 0, 1, 3); // 0 = helper, 1 = moderator, 2 = administrator, 3 = nobody
GVAR(IDF_ADMIN, kicklock, 0, 1, 3); // 0 = helper, 1 = moderator, 2 = administrator, 3 = nobody
GVAR(IDF_ADMIN, allowlock, 0, 1, 3); // 0 = helper, 1 = moderator, 2 = administrator, 3 = nobody
GVAR(IDF_ADMIN, banlock, 0, 1, 3); // 0 = helper, 1 = moderator, 2 = administrator, 3 = nobody
GVAR(IDF_ADMIN, mutelock, 0, 1, 3); // 0 = helper, 1 = moderator, 2 = administrator, 3 = nobody
GVAR(IDF_ADMIN, limitlock, 0, 1, 3); // 0 = helper, 1 = moderator, 2 = administrator, 3 = nobody

GVAR(IDF_ADMIN, floodlock, 0, 3, 4); // 0 = no flood lock, 1 = lock below helper, 2 = lock below moderator, 3 = lock below administrator, 4 = lock everyone
GVAR(IDF_ADMIN, floodmute, 0, 3, VAR_MAX); // automatically mute player when warned this many times
GVAR(IDF_ADMIN, floodtime, 250, 10000, VAR_MAX); // time span to check for floody messages
GVAR(IDF_ADMIN, floodlines, 1, 5, VAR_MAX); // number of lines in aforementioned span before too many
//GVAR(IDF_ADMIN, mapcrclock, 0, 4, 4); // 0 = no mapcrc lock, 1 = lock below helper, 2 = lock below moderator, 3 = lock below administrator, 4 = lock everyone

GVAR(IDF_ADMIN, autospectate, 0, 1, 1); // auto spectate if idle, 1 = auto spectate when remaining dead for autospecdelay
GVAR(IDF_ADMIN, autospecdelay, 0, 60000, VAR_MAX);

GVAR(IDF_ADMIN, resetallowsonend, 0, 1, 2); // reset allows on end (1: just when empty, 2: when matches end)
GVAR(IDF_ADMIN, resetbansonend, 0, 1, 2); // reset bans on end (1: just when empty, 2: when matches end)
GVAR(IDF_ADMIN, resetmutesonend, 0, 1, 2); // reset mutes on end (1: just when empty, 2: when matches end)
GVAR(IDF_ADMIN, resetlimitsonend, 0, 1, 2); // reset limits on end (1: just when empty, 2: when matches end)
GVAR(IDF_ADMIN, resetvarsonend, 0, 1, 2); // reset variables on end (1: just when empty, 2: when matches end)
GVAR(IDF_ADMIN, resetmmonend, 0, 2, 2); // reset mastermode on end (1: just when empty, 2: when matches end)

GVARF(0, gamespeed, 1, 100, 10000, timescale = sv_gamespeed, timescale = gamespeed);
GVAR(IDF_ADMIN, gamespeedlock, 0, 3, 4); // 0 = off, 1 = helper, 2 = moderator, 3 = administrator, 4 = nobody
GVARF(IDF_ADMIN, gamepaused, 0, 0, 1, paused = sv_gamepaused, paused = gamepaused);

GSVAR(IDF_ADMIN, defaultmap, "");
GVAR(IDF_ADMIN, defaultmode, G_START, G_DEATHMATCH, G_MAX-1);
GVAR(IDF_ADMIN, defaultmuts, 0, 0, G_M_ALL);
GVAR(IDF_ADMIN, rotatemode, 0, 1, 1);
GVARF(IDF_ADMIN, rotatemodefilter, 0, G_LIMIT, G_ALL, sv_rotatemodefilter &= ~G_NEVER, rotatemodefilter &= ~G_NEVER); // modes not in this array are filtered out
GVAR(IDF_ADMIN, rotatemuts, 0, 3, VAR_MAX); // any more than one decreases the chances of it picking
GVAR(IDF_ADMIN, rotatemutsfilter, 0, G_M_FILTER, G_M_ALL); // mutators not in this array are filtered out

GSVAR(IDF_ADMIN, allowmaps, "ares bath biolytic blink canals cargo center colony conflict darkness dawn deadsimple deathtrap deli depot dropzone dutility echo error facility forge foundation fourplex futuresport ghost hawk hinder industrial institute isolation keystone2k lab linear longestyard mist neodrive nova oneiroi panic processing pumpstation purge spacetech starlibido stone testchamber tower tranquility tribal ubik venus warp wet");

GSVAR(IDF_ADMIN, mainmaps, "ares bath biolytic cargo center colony conflict darkness deadsimple deathtrap deli depot dropzone dutility echo error foundation fourplex futuresport ghost industrial institute isolation keystone2k linear longestyard mist nova oneiroi panic processing pumpstation spacetech starlibido stone tower tribal ubik venus warp wet");
GSVAR(IDF_ADMIN, capturemaps, "ares bath biolytic cargo center colony conflict darkness deadsimple deli depot dropzone dutility echo foundation fourplex futuresport ghost industrial institute isolation keystone2k linear mist nova panic pumpstation stone tribal venus warp wet");
GSVAR(IDF_ADMIN, defendmaps, "ares bath biolytic cargo center colony conflict darkness deadsimple deli depot dropzone dutility echo foundation fourplex futuresport ghost industrial institute isolation keystone2k linear mist nova panic processing pumpstation stone tower tribal ubik venus warp wet");
GSVAR(IDF_ADMIN, kingmaps, "ares bath biolytic cargo center colony conflict darkness deadsimple depot dropzone dutility echo fourplex futuresport industrial keystone2k linear processing stone tower tribal ubik venus");
GSVAR(IDF_ADMIN, bombermaps, "ares bath biolytic cargo center colony conflict darkness deadsimple deli depot dropzone dutility echo foundation futuresport fourplex ghost industrial isolation linear mist nova pumpstation stone tower tribal venus warp wet");
GSVAR(IDF_ADMIN, holdmaps, "ares bath biolytic cargo center colony conflict darkness deadsimple deli depot dropzone dutility echo foundation fourplex futuresport ghost industrial isolation keystone2k linear mist nova panic processing pumpstation stone tower tribal ubik venus warp wet");
GSVAR(IDF_ADMIN, trialmaps, "hawk hinder neodrive purge testchamber");

GSVAR(IDF_ADMIN, multimaps, "deadsimple depot keystone2k warp isolation fourplex"); // applies to modes which *require* multi spawns (ctf/bb)
GSVAR(IDF_ADMIN, duelmaps, "bath darkness deadsimple dutility echo fourplex ghost longestyard starlibido stone panic wet");
GSVAR(IDF_ADMIN, jetpackmaps, "ares biolytic cargo center colony conflict darkness dawn deadsimple deathtrap deli depot dropzone dutility echo error forge fourplex futuresport ghost isolation linear longestyard mist nova oneiroi pumpstation spacetech starlibido testchamber tower tranquility tribal ubik venus warp");

GSVAR(IDF_ADMIN, smallmaps, "bath darkness deadsimple dutility echo fourplex ghost longestyard starlibido stone panic wet");
GSVAR(IDF_ADMIN, mediummaps, "ares biolytic blink cargo center colony conflict darkness deadsimple deathtrap deli dropzone echo error facility forge foundation fourplex futuresport ghost industrial institute isolation keystone2k lab linear mist nova oneiroi panic processing pumpstation spacetech starlibido stone tower tranquility tribal ubik venus warp wet");
GSVAR(IDF_ADMIN, largemaps, "ares biolytic blink cargo center colony dawn deadsimple deathtrap deli depot error facility forge foundation futuresport ghost industrial isolation lab linear mist nova processing pumpstation spacetech tower tranquility tribal ubik venus warp");


GVAR(IDF_ADMIN, modelock, 0, 5, 7); // 0 = off, 1-3 = helper/moderator/administrator only, 4-6 = helper/moderator/administrator can only set limited mode and higher, 7 = no mode selection
GVAR(IDF_ADMIN, modelockfilter, 0, G_LIMIT, G_ALL);
GVAR(IDF_ADMIN, mutslockfilter, 0, G_M_ALL, G_M_ALL);
GVARF(IDF_ADMIN, instagibfilter, 0, mutstype[G_M_INSTA].mutators&~(1<<G_M_ARENA), mutstype[G_M_INSTA].mutators, sv_instagibfilter &= ~(1<<G_M_VAMPIRE); sv_instagibfilter |= (1<<G_M_INSTA), instagibfilter &= ~(1<<G_M_VAMPIRE); instagibfilter |= (1<<G_M_INSTA));

GVAR(IDF_ADMIN, maprotate, 0, 2, 2); // 0 = off, 1 = sequence, 2 = random
GVAR(IDF_ADMIN, mapsfilter, 0, 1, 2); // 0 = off, 1 = filter based on mutators, 2 = also filter based on players
GVAR(IDF_ADMIN, mapslock, 0, 5, 7); // 0 = off, 1-3 = helper/moderator/administrator can select non-allow maps, 4-6 = helper/moderator/administrator can select non-rotation maps, 7 = no map selection
GVAR(IDF_ADMIN, varslock, 0, 2, 4); // 0 = off, 1 = helper, 2 = moderator, 3 = administrator, 4 = nobody
GVAR(IDF_ADMIN, votelock, 0, 2, 7); // 0 = off, 1-3 = helper/moderator/administrator can select same game, 4 = helper/moderator/administrator only can vote, 7 = no voting
GVAR(IDF_ADMIN, votewait, 0, 2500, VAR_MAX);
GVAR(IDF_ADMIN, votestyle, 0, 2, 2); // 0 = votes don't pass mid-match, 1 = passes if votethreshold is met, 2 = passes if unanimous
GVAR(IDF_ADMIN, voteinterm, 0, 2, 2); // 0 = must wait entire time, 1 = passes if votethreshold is met, 2 = passes if unanimous
GFVAR(IDF_ADMIN, votethreshold, 0, 0.5f, 1); // auto-pass votes when this many agree

GVAR(IDF_ADMIN, smallmapmax, 0, 6, VAR_MAX); // maximum number of players for a small map
GVAR(IDF_ADMIN, mediummapmax, 0, 12, VAR_MAX); // maximum number of players for a medium map

namespace server { extern void resetgamevars(bool flush); }
GICOMMAND(0, resetvars, "", (), server::resetgamevars(true), return);
GICOMMAND(IDF_ADMIN, resetconfig, "", (), rehash(true), );

GFVAR(0, maxalive, 0, 0, FVAR_MAX); // only allow this*numplayers to be alive at once
GVAR(0, maxalivequeue, 0, 1, 1); // if number of players exceeds this amount, use a queue system
GVAR(0, maxaliveminimum, 2, 8, VAR_MAX); // kicks in if numplayers >= this
GFVAR(0, maxalivethreshold, 0, 0.5f, FVAR_MAX); // .. or this percentage of players

GVAR(0, maxcarry, 1, 2, WEAP_MAX-1);
GVAR(0, spawnrotate, 0, 2, 2); // 0 = let client decide, 1 = sequence, 2 = random
GVAR(0, spawnweapon, 0, WEAP_PISTOL, WEAP_MAX-1);
GVAR(0, instaweapon, 0, WEAP_RIFLE, WEAP_MAX-1);
GVAR(0, trialweapon, 0, WEAP_MELEE, WEAP_MAX-1);
GVAR(0, spawngrenades, 0, 0, 2); // 0 = never, 1 = all but insta/trial, 2 = always
GVAR(0, spawndelay, 0, 5000, VAR_MAX); // delay before spawning in most modes
GVAR(0, instadelay, 0, 3000, VAR_MAX); // .. in instagib matches
GVAR(0, trialdelay, 0, 500, VAR_MAX); // .. in time trial matches
GVAR(0, bomberdelay, 0, 3000, VAR_MAX); // delay before spawning in bomber
GVAR(0, spawnprotect, 0, 3000, VAR_MAX); // delay before damage can be dealt to spawning player
GVAR(0, duelprotect, 0, 5000, VAR_MAX); // .. in duel/survivor matches
GVAR(0, instaprotect, 0, 3000, VAR_MAX); // .. in instagib matches

GVAR(0, spawnhealth, 0, 100, VAR_MAX);
GFVAR(0, maxhealth, 0, 1.5f, FVAR_MAX);
GFVAR(0, maxhealthvampire, 0, 2.0f, FVAR_MAX);

GFVAR(0, actorscale, FVAR_NONZERO, 1, FVAR_MAX);
GFVAR(0, maxresizescale, 1, 2, FVAR_MAX);
GFVAR(0, minresizescale, FVAR_NONZERO, 0.5f, 1);
GFVAR(0, instaresizeamt, FVAR_NONZERO, 0.1f, 1); // each kill adds this much size in insta-resize

GVAR(0, burntime, 0, 5500, VAR_MAX);
GVAR(0, burndelay, 0, 1000, VAR_MAX);
GVAR(0, burndamage, 0, 3, VAR_MAX);
GVAR(0, bleedtime, 0, 5500, VAR_MAX);
GVAR(0, bleeddelay, 0, 1000, VAR_MAX);
GVAR(0, bleeddamage, 0, 3, VAR_MAX);

GVAR(0, regendelay, 0, 3000, VAR_MAX); // regen after no damage for this long
GVAR(0, regentime, 0, 1000, VAR_MAX); // regen this often when regenerating normally
GVAR(0, regenhealth, 0, 5, VAR_MAX); // regen this amount each regen
GVAR(0, regendecay, 0, 3, VAR_MAX); // if over maxhealth, decay this amount each regen

GVAR(0, kamikaze, 0, 1, 3); // 0 = never, 1 = holding grenade, 2 = have grenade, 3 = always
GVAR(0, itemsallowed, 0, 2, 2); // 0 = never, 1 = all but limited, 2 = always
GVAR(0, itemspawntime, 1, 15000, VAR_MAX); // when items respawn
GVAR(0, itemspawndelay, 0, 1000, VAR_MAX); // after map start items first spawn
GVAR(0, itemspawnstyle, 0, 1, 3); // 0 = all at once, 1 = staggered, 2 = random, 3 = randomise between both
GFVAR(0, itemthreshold, 0, 2, FVAR_MAX); // if numitems/(players*maxcarry) is less than this, spawn one of this type
GVAR(0, itemcollide, 0, BOUNCE_GEOM, VAR_MAX);
GVAR(0, itemextinguish, 0, 6, 7);
GFVAR(0, itemelasticity, FVAR_MIN, 0.4f, FVAR_MAX);
GFVAR(0, itemrelativity, FVAR_MIN, 1, FVAR_MAX);
GFVAR(0, itemwaterfric, 0, 1.75f, FVAR_MAX);
GFVAR(0, itemweight, FVAR_MIN, 150, FVAR_MAX);
GFVAR(0, itemminspeed, 0, 0, FVAR_MAX);
GFVAR(0, itemrepulsion, 0, 8, FVAR_MAX);
GFVAR(0, itemrepelspeed, 0, 25, FVAR_MAX);

GVAR(0, timelimit, 0, 10, VAR_MAX); // maximum time a match may last, 0 = forever
GVAR(0, overtimeallow, 0, 1, 1); // if scores are equal, go into overtime
GVAR(0, overtimelimit, 0, 5, VAR_MAX); // maximum time overtime may last, 0 = forever
GVAR(0, triallimit, 0, 60000, VAR_MAX);
GVAR(0, intermlimit, 0, 15000, VAR_MAX); // .. before vote menu comes up
GVAR(0, votelimit, 0, 45000, VAR_MAX); // .. before vote passes by default
GVAR(0, duelreset, 0, 1, 1); // reset winner in duel
GVAR(0, duelclear, 0, 1, 1); // clear items in duel
GVAR(0, duellimit, 0, 5000, VAR_MAX); // .. before duel goes to next round
GVAR(0, duelcycle, 0, 2, 3); // determines if players are force-cycled after a certain number of wins (bit: 0 = off, 1 = non-team games, 2 = team games)
GVAR(0, duelcycles, 0, 3, VAR_MAX); // maximum wins in a row before force-cycling (0 = num team/total players)

GVAR(0, selfdamage, 0, 1, 1); // 0 = off, 1 = either hurt self or use teamdamage rules
GVAR(0, teamdamage, 0, 1, 2); // 0 = off, 1 = non-bots damage team, 2 = all players damage team
GVAR(0, teambalance, 0, 1, 2); // 0 = off, 1 = by number then rank, 2 = by rank then number
GVAR(0, teampersist, 0, 1, 2); // 0 = off, 1 = only attempt, 2 = forced
GVAR(0, pointlimit, 0, 0, VAR_MAX); // finish when score is this or more

GVAR(0, capturelimit, 0, 0, VAR_MAX); // finish when score is this or more
GVAR(0, captureresetdelay, 0, 30000, VAR_MAX);
GVAR(0, capturedefenddelay, 0, 15000, VAR_MAX);
GVAR(0, captureprotectdelay, 0, 15000, VAR_MAX);
GVAR(0, capturepickupdelay, -1, 5000, VAR_MAX);
GFVAR(0, capturecarryspeed, 0, 0.9f, FVAR_MAX);
GVAR(0, capturepoints, 0, 5, VAR_MAX); // points added to score
GVAR(0, capturepickuppoints, 0, 3, VAR_MAX); // points added to score
GVAR(0, capturecollide, 0, BOUNCE_GEOM, VAR_MAX);
GVAR(0, captureextinguish, 0, 6, 7);
GFVAR(0, capturerelativity, 0, 0.25f, FVAR_MAX);
GFVAR(0, captureelasticity, FVAR_MIN, 0.35f, FVAR_MAX);
GFVAR(0, capturewaterfric, FVAR_MIN, 1.75f, FVAR_MAX);
GFVAR(0, captureweight, FVAR_MIN, 100, FVAR_MAX);
GFVAR(0, captureminspeed, 0, 0, FVAR_MAX);
GFVAR(0, capturerepulsion, 0, 16, FVAR_MAX);
GFVAR(0, capturerepelspeed, 0, 25, FVAR_MAX);
GFVAR(0, capturethreshold, 0, 0, FVAR_MAX); // if someone 'warps' more than this distance, auto-drop
GVAR(0, capturebuffing, 0, 5, 15); // buffed; 0 = off, &1 = when guarding/secured, &2 = when carrying enemy, &4 = also defending secured, &8 = also defending enemy carrier
GVAR(0, capturebuffdelay, 0, 1000, VAR_MAX); // buffed for this long after leaving
GFVAR(0, capturebuffarea, FVAR_NONZERO, 3, FVAR_MAX); // multiply affinity radius by this much for buff
GFVAR(0, capturebuffdamage, 1, 1.5f, FVAR_MAX); // multiply outgoing damage by this much when buffed
GFVAR(0, capturebuffshield, 1, 1.5f, FVAR_MAX); // divide incoming damage by this much when buffed
GVAR(0, captureregenbuff, 0, 1, 1); // 0 = off, 1 = modify regeneration when buffed
GVAR(0, captureregendelay, 0, 1000, VAR_MAX); // regen this often when buffed
GVAR(0, captureregenextra, 0, 2, VAR_MAX); // add this to regen when buffed

GVAR(0, defendlimit, 0, 0, VAR_MAX); // finish when score is this or more
GVAR(0, defendpoints, 0, 1, VAR_MAX); // points added to score
GVAR(0, defendinterval, 0, 50, VAR_MAX);
GVAR(0, defendoccupy, 1, 100, VAR_MAX); // points needed to occupy in regular games
GVAR(0, defendking, 1, 25, VAR_MAX); // points needed to occupy in king of the hill
GVAR(0, defendflags, 0, 3, 3); // 0 = init all (neutral), 1 = init neutral and team only, 2 = init team only, 3 = init all (team + neutral + converted)
GVAR(0, defendbuffing, 0, 1, 7); // buffed; 0 = off, &1 = when guarding, &2 = when securing, &4 = even when enemies are present
GFVAR(0, defendbuffoccupy, 0, 0.5f, 1); // for defendbuffing&4, must be occupied this much before passing
GVAR(0, defendbuffdelay, 0, 1000, VAR_MAX); // buffed for this long after leaving
GFVAR(0, defendbuffarea, FVAR_NONZERO, 3, FVAR_MAX); // multiply affinity radius by this much for buff
GFVAR(0, defendbuffdamage, 1, 1.5f, FVAR_MAX); // multiply outgoing damage by this much when buffed
GFVAR(0, defendbuffshield, 1, 1.5f, FVAR_MAX); // divide incoming damage by this much when buffed
GVAR(0, defendregenbuff, 0, 1, 1); // 0 = off, 1 = modify regeneration when buffed
GVAR(0, defendregendelay, 0, 1000, VAR_MAX); // regen this often when buffed
GVAR(0, defendregenextra, 0, 2, VAR_MAX); // add this to regen when buffed

GVAR(0, bomberlimit, 0, 0, VAR_MAX); // finish when score is this or more (non-hold)
GVAR(0, bomberholdlimit, 0, 0, VAR_MAX); // finish when score is this or more (hold)
GVAR(0, bomberresetdelay, 0, 15000, VAR_MAX);
GVAR(0, bomberpickupdelay, -1, 5000, VAR_MAX);
GVAR(0, bombercarrytime, 0, 15000, VAR_MAX);
GFVAR(0, bombercarryspeed, 0, 0.75f, FVAR_MAX);
GVAR(0, bomberpoints, 0, 5, VAR_MAX); // points added to score
GVAR(0, bomberpenalty, 0, 5, VAR_MAX); // points taken from score
GVAR(0, bomberpickuppoints, 0, 3, VAR_MAX); // points added to score
GVAR(0, bomberholdpoints, 0, 1, VAR_MAX); // points added to score
GVAR(0, bomberholdpenalty, 0, 10, VAR_MAX); // penalty for holding too long
GVAR(0, bomberholdinterval, 0, 1000, VAR_MAX);
GVAR(0, bomberlockondelay, 0, 250, VAR_MAX);
GVAR(0, bomberreset, 0, 0, 2); // 0 = off, 1 = kill winners, 2 = kill everyone
GFVAR(0, bomberspeed, 0, 250, FVAR_MAX);
GFVAR(0, bomberdelta, 0, 1000, FVAR_MAX);
GVAR(0, bombercollide, 0, BOUNCE_GEOM, VAR_MAX);
GVAR(0, bomberextinguish, 0, 6, 7);
GFVAR(0, bomberrelativity, 0, 0.25f, FVAR_MAX);
GFVAR(0, bomberelasticity, FVAR_MIN, 0.65f, FVAR_MAX);
GFVAR(0, bomberwaterfric, FVAR_MIN, 1.75f, FVAR_MAX);
GFVAR(0, bomberweight, FVAR_MIN, 150, FVAR_MAX);
GFVAR(0, bomberminspeed, 0, 50, FVAR_MAX);
GFVAR(0, bomberthreshold, 0, 0, FVAR_MAX); // if someone 'warps' more than this distance, auto-drop
GVAR(0, bomberbuffing, 0, 1, 3); // buffed; 0 = off, &1 = when guarding, &2 = when securing
GVAR(0, bomberbuffdelay, 0, 1000, VAR_MAX); // buffed for this long after leaving
GFVAR(0, bomberbuffarea, FVAR_NONZERO, 3, FVAR_MAX); // multiply affinity radius by this much for buff
GFVAR(0, bomberbuffdamage, 1, 1.5f, FVAR_MAX); // multiply outgoing damage by this much when buffed
GFVAR(0, bomberbuffshield, 1, 1.5f, FVAR_MAX); // divide incoming damage by this much when buffed
GVAR(0, bomberregenbuff, 0, 1, 1); // 0 = off, 1 = modify regeneration when buffed
GVAR(0, bomberregendelay, 0, 1000, VAR_MAX); // regen this often when buffed
GVAR(0, bomberregenextra, 0, 2, VAR_MAX); // add this to regen when buffed
GVAR(0, bomberbasket, 0, 1, 1); // you can score by throwing the bomb into the goal

GVAR(0, trialstyle, 0, 0, 2); // 0 = all players are ghosts, 1 = all players are solid, but can't deal damage, 2 = regular gameplay style, solid+damage

GVAR(IDF_ADMIN, airefresh, 0, 1000, VAR_MAX);
GVAR(0, botbalance, -1, -1, VAR_MAX); // -1 = always use numplayers, 0 = don't balance, 1 or more = fill only with this*numteams
GVAR(0, botskillmin, 1, 50, 101);
GVAR(0, botskillmax, 1, 75, 101);
GVAR(0, botlimit, 0, 16, VAR_MAX);
GVAR(0, botoffset, VAR_MIN, 0, VAR_MAX);
GFVAR(0, botspeed, 0, 1, FVAR_MAX);
GFVAR(0, botscale, FVAR_NONZERO, 1, FVAR_MAX);
GVAR(0, coopbalance, 1, 2, VAR_MAX);
GVAR(0, coopskillmin, 1, 75, 101);
GVAR(0, coopskillmax, 1, 95, 101);
GVAR(0, enemybalance, 1, 1, 3);
GVAR(0, enemyskillmin, 1, 65, 101);
GVAR(0, enemyskillmax, 1, 85, 101);
GVAR(0, enemyspawntime, 1, 30000, VAR_MAX); // when enemies respawn
GVAR(0, enemyspawndelay, 0, 1000, VAR_MAX); // after map start enemies first spawn
GVAR(0, enemyspawnstyle, 0, 1, 3); // 0 = all at once, 1 = staggered, 2 = random, 3 = randomise between both
GFVAR(0, enemyspeed, 0, 1, FVAR_MAX);
GFVAR(0, enemyscale, FVAR_NONZERO, 1, FVAR_MAX);
GFVAR(0, enemystrength, FVAR_NONZERO, 1, FVAR_MAX); // scale enemy health values by this much

GFVAR(0, gravityforce, -1, -1, FVAR_MAX);
GFVAR(0, gravityscale, 0, 1, FVAR_MAX);
GFVAR(0, liquidspeedforce, -1, -1, 1);
GFVAR(0, liquidspeedscale, 0, 1, FVAR_MAX);
GFVAR(0, liquidcoastforce, -1, -1, FVAR_MAX);
GFVAR(0, liquidcoastscale, 0, 1, FVAR_MAX);
GFVAR(0, floorcoastforce, -1, -1, FVAR_MAX);
GFVAR(0, floorcoastscale, 0, 1, FVAR_MAX);
GFVAR(0, aircoastforce, -1, -1, FVAR_MAX);
GFVAR(0, aircoastscale, 0, 1, FVAR_MAX);
GFVAR(0, slidecoastforce, -1, -1, FVAR_MAX);
GFVAR(0, slidecoastscale, 0, 1, FVAR_MAX);

GFVAR(0, movespeed, FVAR_NONZERO, 100, FVAR_MAX); // speed
GFVAR(0, movecrawl, 0, 0.6f, FVAR_MAX); // crawl modifier
GFVAR(0, movesprint, FVAR_NONZERO, 1.6f, FVAR_MAX); // sprinting modifier
GFVAR(0, movejet, FVAR_NONZERO, 1.6f, FVAR_MAX); // jetpack modifier
GFVAR(0, movestraight, FVAR_NONZERO, 1.2f, FVAR_MAX); // non-strafe modifier
GFVAR(0, movestrafe, FVAR_NONZERO, 1, FVAR_MAX); // strafe modifier
GFVAR(0, moveinair, FVAR_NONZERO, 0.9f, FVAR_MAX); // in-air modifier
GFVAR(0, movestepup, FVAR_NONZERO, 0.95f, FVAR_MAX); // step-up modifier
GFVAR(0, movestepdown, FVAR_NONZERO, 1.15f, FVAR_MAX); // step-down modifier

GVAR(0, jetdelay, 0, 250, VAR_MAX); // minimum time between jetpack
GFVAR(0, jetheight, 0, 0, FVAR_MAX); // jetpack maximum height off ground
GFVAR(0, jetdecay, 1, 15, FVAR_MAX); // decay rate of one unit per this many ms

GFVAR(0, jumpspeed, FVAR_NONZERO, 110, FVAR_MAX); // extra velocity to add when jumping
GFVAR(0, impulsespeed, FVAR_NONZERO, 90, FVAR_MAX); // extra velocity to add when impulsing

GFVAR(0, impulselimit, 0, 0, FVAR_MAX); // maximum impulse speed
GFVAR(0, impulseboost, 0, 1, FVAR_MAX); // thrust modifier
GFVAR(0, impulsepower, 0, 1.5f, FVAR_MAX); // power jump modifier
GFVAR(0, impulsedash, 0, 1.2f, FVAR_MAX); // dashing/powerslide modifier
GFVAR(0, impulsejump, 0, 1.1f, FVAR_MAX); // jump modifier
GFVAR(0, impulseparkour, 0, 1, FVAR_MAX); // parkour modifier
GFVAR(0, impulseparkourkick, 0, 1.3f, FVAR_MAX); // parkour kick modifier
GFVAR(0, impulseparkourvault, 0, 1.3f, FVAR_MAX); // parkour vault modifier
GFVAR(0, impulseparkournorm, 0, 0.5f, FVAR_MAX); // minimum parkour surface z normal
GVAR(0, impulseallowed, 0, IM_A_ALL, IM_A_ALL); // impulse actions allowed; bit: 0 = off, 1 = dash, 2 = boost, 4 = sprint, 8 = parkour
GVAR(0, impulsestyle, 0, 1, 3); // impulse style; 0 = off, 1 = touch and count, 2 = count only, 3 = freestyle
GVAR(0, impulsecount, 0, 6, VAR_MAX); // number of impulse actions per air transit
GVAR(0, impulseslip, 0, 350, VAR_MAX); // time before floor friction kicks back in
GVAR(0, impulseslide, 0, 750, VAR_MAX); // time before powerslides end
GVAR(0, impulsedelay, 0, 250, VAR_MAX); // minimum time between boosts
GVAR(0, impulsedashdelay, 0, 750, VAR_MAX); // minimum time between dashes/powerslides
GVAR(0, impulsekickdelay, 0, 350, VAR_MAX); // minimum time between wall kicks/climbs
GFVAR(0, impulsevaultmin, FVAR_NONZERO, 0.25f, FVAR_MAX); // minimum percentage of height for vault
GFVAR(0, impulsevaultmax, FVAR_NONZERO, 1.f, FVAR_MAX); // maximum percentage of height for vault

GFVAR(0, impulsemelee, 0, 0.75f, FVAR_MAX); // melee modifier
GVAR(0, impulsemeleechain, 0, 0, VAR_MAX); // impulse melee style; 0 = any time, 1+ = only this long after an impulse in ms

GVAR(0, impulsemeter, 0, 20000, VAR_MAX); // impulse dash length; 0 = unlimited, anything else = timer
GVAR(0, impulsecost, 0, 4000, VAR_MAX); // cost of impulse jump
GVAR(0, impulsecostscale, 0, 0, 1); // whether the cost scales depend on the amount the impulse scales
GVAR(0, impulseskate, 0, 1000, VAR_MAX); // length of time a run along a wall can last
GFVAR(0, impulsesprint, 0, 0, FVAR_MAX); // sprinting impulse meter depletion
GFVAR(0, impulsejet, 0, 0.5f, FVAR_MAX); // jetpack impulse meter depletion
GFVAR(0, impulseregen, 0, 4.f, FVAR_MAX); // impulse regen multiplier
GFVAR(0, impulseregencrouch, 0, 2, FVAR_MAX); // impulse regen crouch modifier
GFVAR(0, impulseregensprint, 0, 0.75f, FVAR_MAX); // impulse regen sprinting modifier
GFVAR(0, impulseregenjet, 0, 1.5f, FVAR_MAX); // impulse regen jetpack modifier
GFVAR(0, impulseregenmove, 0, 1, FVAR_MAX); // impulse regen moving modifier
GFVAR(0, impulseregeninair, 0, 0.75f, FVAR_MAX); // impulse regen in-air modifier
GVAR(0, impulseregendelay, 0, 250, VAR_MAX); // delay before impulse regens
GVAR(0, impulseregenjetdelay, -1, -1, VAR_MAX); // delay before impulse regens after jetting, -1 = must touch ground

GFVAR(0, stillspread, 0, 0, FVAR_MAX);
GFVAR(0, movespread, 0, 1, FVAR_MAX);
GFVAR(0, inairspread, 0, 1, FVAR_MAX);
GFVAR(0, impulsespread, 0, 1, FVAR_MAX);

GVAR(0, quakefade, 0, 250, VAR_MAX);
GVAR(0, quakewobble, 1, 19, VAR_MAX);

GVAR(0, zoomlock, 0, 0, 4); // 0 = unrestricted, 1 = must be on floor, 2 = also must not be moving, 3 = also must be on flat floor, 4 = must also be crouched
GVAR(0, zoomlocktime, 0, 500, VAR_MAX); // time before zoomlock kicks in when in the air
GVAR(0, zoomlimit, 1, 10, 150);
GVAR(0, zoomtime, 1, 100, VAR_MAX);

GFVAR(0, explodescale, 0, 1, FVAR_MAX);
GFVAR(0, explodelimited, 0, 0.5f, FVAR_MAX);
GFVAR(0, damagescale, 0, 1, FVAR_MAX);
GVAR(0, criticalchance, 0, 100, VAR_MAX);
GVAR(0, weaponjamming, 0, 0, 4); // 0 = off, 1 = drop the weapon, 2 = drop all weapons, 3/4 = 1/2 + explode
GVAR(0, weaponjamtime, 0, 1000, VAR_MAX);
GVAR(0, weaponswitchdelay, 0, 500, VAR_MAX);

GFVAR(0, hitpushscale, 0, 1, FVAR_MAX);
GFVAR(0, hitslowscale, 0, 1, FVAR_MAX);
GFVAR(0, deadpushscale, 0, 2, FVAR_MAX);
GFVAR(0, wavepushscale, 0, 1, FVAR_MAX);
GFVAR(0, waveslowscale, 0, 0.5f, FVAR_MAX);
GFVAR(0, kickpushscale, 0, 1, FVAR_MAX);
GFVAR(0, kickpushcrouch, 0, 0, FVAR_MAX);
GFVAR(0, kickpushsway, 0, 0.0125f, FVAR_MAX);
GFVAR(0, kickpushzoom, 0, 0.125f, FVAR_MAX);

GVAR(0, fragbonus, 0, 3, VAR_MAX);
GVAR(0, enemybonus, 0, 1, VAR_MAX);
GVAR(0, teamkillpenalty, 0, 2, VAR_MAX);
GVAR(0, firstbloodpoints, 0, 1, VAR_MAX);
GVAR(0, headshotpoints, 0, 1, VAR_MAX);

GVAR(0, assistkilldelay, 0, 5000, VAR_MAX);
GVAR(0, multikilldelay, 0, 5000, VAR_MAX);
GVAR(0, multikillpoints, 0, 1, 1);
GVAR(0, multikillbonus, 0, 1, VAR_MAX);
GVAR(0, spreecount, 0, 5, VAR_MAX);
GVAR(0, spreepoints, 0, 1, 1);
GVAR(0, spreebonus, 0, 1, VAR_MAX);
GVAR(0, dominatecount, 0, 5, VAR_MAX);
GVAR(0, dominatepoints, 0, 1, VAR_MAX);
GVAR(0, revengepoints, 0, 1, VAR_MAX);
