#include "game.h"
extern char *progresstitle, *progresstext;
extern float progresspart, progressamt;

namespace hud
{
    const int NUMSTATS = 11;
    int damageresidue = 0, hudwidth = 0, hudheight = 0, lastteam = 0, lastnewgame = 0, laststats = 0, prevstats[NUMSTATS] = {0}, curstats[NUMSTATS] = {0};
    bool forceprogress = false;

    #include "compass.h"
    vector<int> teamkills;

    struct damageloc
    {
        int attacker, outtime, damage; vec dir, colour;
        damageloc(int a, int t, int d, const vec &p, const vec &c) : attacker(a), outtime(t), damage(d), dir(p), colour(c) {}
    };
    vector<damageloc> damagelocs;
    VAR(IDF_PERSIST, damageresiduefade, 0, 500, VAR_MAX);

    ICOMMAND(0, conout, "is", (int *n, char *s), conoutft(clamp(*n, 0, CON_MAX-1), "%s", s));

    VAR(IDF_PERSIST, showhud, 0, 1, 1);
    VAR(IDF_PERSIST, huduioverride, 0, 1, 2); // 0=off, 1=except intermission, 2=interactive ui only
    VAR(IDF_PERSIST, hudsize, 0, 2048, VAR_MAX);
    FVAR(IDF_PERSIST, hudblend, 0, 1, 1);
    FVAR(IDF_PERSIST, gapsize, 0, 0.01f, 1000);

    VAR(IDF_PERSIST, showconsole, 0, 2, 2);
    VAR(IDF_PERSIST, shownotices, 0, 3, 4);
    VAR(IDF_PERSIST, showevents, 0, 2, 3);

    VAR(IDF_PERSIST, showfps, 0, 1, 3);
    VAR(IDF_PERSIST, showstats, 0, 1, 2);
    VAR(IDF_PERSIST, statrate, 0, 200, 1000);
    FVAR(IDF_PERSIST, statblend, 0, 1, 1);

    bool fullconsole = false;
    void toggleconsole() { fullconsole = !fullconsole; }
    COMMAND(0, toggleconsole, "");

    VAR(IDF_PERSIST, titlefade, 0, 1000, 10000);
    VAR(IDF_PERSIST, tvmodefade, 0, 1000, VAR_MAX);
    VAR(IDF_PERSIST, spawnfade, 0, 500, VAR_MAX);

    VAR(IDF_PERSIST, commandfade, 0, 200, VAR_MAX);
    FVAR(IDF_PERSIST, commandfadeamt, 0, 0.5f, 1);
    FVAR(IDF_PERSIST, commandfadeskew, 0, 0.25f, 1);
    VAR(IDF_PERSIST, uifade, 0, 200, VAR_MAX);
    FVAR(IDF_PERSIST, uifadeamt, 0, 0.5f, 1);

    int conskip = 0;
    void setconskip(int *n)
    {
        conskip += *n;
        if(conskip<0) conskip = 0;
    }
    COMMANDN(0, conskip, setconskip, "i");

    VAR(IDF_PERSIST, consize, 0, 5, 100);
    VAR(IDF_PERSIST, contime, 0, 30000, VAR_MAX);
    VAR(IDF_PERSIST, confade, 0, 1000, VAR_MAX);
    VAR(IDF_PERSIST, conoverflow, 0, 5, VAR_MAX);
    VAR(IDF_PERSIST, concenter, 0, 0, 1);
    VAR(IDF_PERSIST, confilter, 0, 1, 1);
    FVAR(IDF_PERSIST, conblend, 0, 1, 1);
    VAR(IDF_PERSIST, chatconsize, 0, 5, 100);
    VAR(IDF_PERSIST, chatcontime, 0, 30000, VAR_MAX);
    VAR(IDF_PERSIST, chatconfade, 0, 2000, VAR_MAX);
    VAR(IDF_PERSIST, chatconoverflow, 0, 5, VAR_MAX);
    FVAR(IDF_PERSIST, chatconblend, 0, 1, 1);
    FVAR(IDF_PERSIST, fullconblend, 0, 1, 1);

    FVAR(IDF_PERSIST, noticeoffset, -1, 0.3f, 1);
    FVAR(IDF_PERSIST, noticeblend, 0, 1, 1);
    FVAR(IDF_PERSIST, noticescale, 1e-4f, 1, 1000);
    VAR(IDF_PERSIST, noticetitle, 0, 10000, 60000);
    FVAR(IDF_PERSIST, eventoffset, -1, 0.3f, 1);
    FVAR(IDF_PERSIST, eventblend, 0, 1, 1);
    FVAR(IDF_PERSIST, eventscale, 1e-4f, 2.5f, 1000);
    VAR(IDF_PERSIST, noticetime, 0, 5000, VAR_MAX);
    VAR(IDF_PERSIST, obitnotices, 0, 2, 2);
    VAR(IDF_PERSIST, teamnotices, 0, 2, 2);
    VAR(IDF_PERSIST, teamnoticedelay, 0, 2500, VAR_MAX);

    TVAR(IDF_PERSIST, neutraltex, "<grey>textures/team", 3);
    TVAR(IDF_PERSIST, alphatex, "<grey>textures/teamalpha", 3);
    TVAR(IDF_PERSIST, omegatex, "<grey>textures/teamomega", 3);
    TVAR(IDF_PERSIST, kappatex, "<grey>textures/teamkappa", 3);
    TVAR(IDF_PERSIST, sigmatex, "<grey>textures/teamsigma", 3);
    TVAR(IDF_PERSIST, playertex, "<grey>textures/player", 3);
    TVAR(IDF_PERSIST, deadtex, "<grey>textures/dead", 3);
    TVAR(IDF_PERSIST, dominatingtex, "<grey>textures/dominating", 3);
    TVAR(IDF_PERSIST, dominatedtex, "<grey>textures/dominated", 3);
    TVAR(IDF_PERSIST, inputtex, "textures/menu", 3);
    TVAR(IDF_PERSIST, waittex, "<grey>textures/wait", 3);
    TVAR(IDF_PERSIST, spectex, "<grey>textures/spec", 3);
    TVAR(IDF_PERSIST, chattex, "<grey>textures/chat", 3);
    TVAR(IDF_PERSIST, healthtex, "<grey>textures/health", 3);
    TVAR(IDF_PERSIST, healthbgtex, "<grey>textures/healthbg", 3);
    TVAR(IDF_PERSIST, impulsetex, "<grey>textures/impulse", 3);
    TVAR(IDF_PERSIST, impulsebgtex, "<grey>textures/impulsebg", 3);
    TVAR(IDF_PERSIST, progresstex, "<grey>textures/progress", 3);
    TVAR(IDF_PERSIST, inventorytex, "<grey>textures/inventory", 3);
    TVAR(IDF_PERSIST, warningtex, "<grey>textures/warning", 3);

    VAR(IDF_PERSIST|IDF_HEX, inventorytone, -CTONE_MAX, -CTONE_TEAMED-1, 0xFFFFFF);
    VAR(IDF_PERSIST|IDF_HEX, healthtone, -CTONE_MAX, -CTONE_TEAMED-1, 0xFFFFFF);
    VAR(IDF_PERSIST|IDF_HEX, impulsetone, -CTONE_MAX, -CTONE_TEAMED-1, 0xFFFFFF);
    VAR(IDF_PERSIST|IDF_HEX, crosshairtone, -CTONE_MAX, 0, 0xFFFFFF);
    VAR(IDF_PERSIST|IDF_HEX, noticestone, -CTONE_MAX, 0, 0xFFFFFF);
    VAR(IDF_PERSIST|IDF_HEX, clipstone, -CTONE_MAX, -CTONE_TEAMED-1, 0xFFFFFF);
    VAR(IDF_PERSIST|IDF_HEX, radartone, -CTONE_MAX, -CTONE_TEAMED-1, 0xFFFFFF);

    VAR(IDF_PERSIST, teamhurttime, 0, 2500, VAR_MAX);
    VAR(IDF_PERSIST, teamhurtdist, 0, 0, VAR_MAX);
    FVAR(IDF_PERSIST, teamhurtsize, 0, 0.0175f, 1000);
    VAR(IDF_PERSIST, teamkillnum, 0, 3, VAR_MAX);
    VAR(IDF_PERSIST, teamkilltime, 0, 60000, VAR_MAX);

    TVAR(IDF_PERSIST, underlaytex, "", 3);
    VAR(IDF_PERSIST, underlaydisplay, 0, 0, 2); // 0 = only firstperson and alive, 1 = only when alive, 2 = always
    VAR(IDF_PERSIST, underlayblend, 0, 1, 1);
    TVAR(IDF_PERSIST, overlaytex, "", 3);
    VAR(IDF_PERSIST, overlaydisplay, 0, 0, 2); // 0 = only firstperson and alive, 1 = only when alive, 2 = always
    FVAR(IDF_PERSIST, overlayblend, 0, 1, 1);

    VAR(IDF_PERSIST, showdamage, 0, 2, 2); // 1 shows just damage texture, 2 blends as well
    VAR(IDF_PERSIST, damagefade, 0, 0, 1);
    TVAR(IDF_PERSIST, damagetex, "<grey>textures/damage", 3);
    FVAR(IDF_PERSIST, damageblend, 0, 1, 1);
    FVAR(IDF_PERSIST, damageskew, 0, 0.25f, 1);
    TVAR(IDF_PERSIST, burntex, "<grey>textures/burn", 3);
    FVAR(IDF_PERSIST, burnblend, 0, 1, 1);

    VAR(IDF_PERSIST, showindicator, 0, 3, 4);
    FVAR(IDF_PERSIST, indicatorsize, 0, 0.025f, 1000);
    FVAR(IDF_PERSIST, indicatorblend, 0, 1, 1);
    TVAR(IDF_PERSIST, indicatortex, "<grey>textures/progress", 3);
    TVAR(IDF_PERSIST, zoomtex, "textures/zoom", 3);

    VAR(IDF_PERSIST, showcrosshair, 0, 2, 2); // 0 = off, 1 = on, 2 = blend depending on current accuracy level
    VAR(IDF_PERSIST, crosshairweapons, 0, 0, 2); // 0 = off, 1 = crosshair-specific weapons, 2 = also appy colour
    FVAR(IDF_PERSIST, crosshairsize, 0, 0.04f, 1000);
    VAR(IDF_PERSIST, crosshairhitspeed, 0, 500, VAR_MAX);
    FVAR(IDF_PERSIST, crosshairblend, 0, 1, 1);
    FVAR(IDF_PERSIST, crosshairaccamt, 0, 0.75f, 1);
    VAR(IDF_PERSIST, crosshairflash, 0, 1, 1);
    FVAR(IDF_PERSIST, crosshairthrob, 1e-4f, 0.1f, 1000);
    TVAR(IDF_PERSIST, pointertex, "textures/pointer", 3);
    TVAR(IDF_PERSIST, guicursortex, "textures/cursor", 3);

    TVAR(IDF_PERSIST, crosshairtex, "crosshairs/cross-01", 3);
    TVAR(IDF_PERSIST, hithairtex, "crosshairs/cross-01-hit", 3);
    TVAR(IDF_PERSIST, meleecrosshairtex, "crosshairs/triangle-02", 3);
    TVAR(IDF_PERSIST, meleehithairtex, "crosshairs/triangle-02-hit", 3);
    TVAR(IDF_PERSIST, pistolcrosshairtex, "crosshairs/cross-01", 3);
    TVAR(IDF_PERSIST, pistolhithairtex, "crosshairs/cross-01-hit", 3);
    TVAR(IDF_PERSIST, swordcrosshairtex, "crosshairs/simple-02", 3);
    TVAR(IDF_PERSIST, swordhithairtex, "crosshairs/simple-02-hit", 3);
    TVAR(IDF_PERSIST, shotguncrosshairtex, "crosshairs/cross-02", 3);
    TVAR(IDF_PERSIST, shotgunhithairtex, "crosshairs/cross-02-hit", 3);
    TVAR(IDF_PERSIST, smgcrosshairtex, "crosshairs/simple-03", 3);
    TVAR(IDF_PERSIST, smghithairtex, "crosshairs/simple-03-hit", 3);
    TVAR(IDF_PERSIST, plasmacrosshairtex, "crosshairs/circle-03", 3);
    TVAR(IDF_PERSIST, plasmahithairtex, "crosshairs/circle-03-hit", 3);
    TVAR(IDF_PERSIST, flamercrosshairtex, "crosshairs/circle-04", 3);
    TVAR(IDF_PERSIST, flamerhithairtex, "crosshairs/circle-04-hit", 3);
    TVAR(IDF_PERSIST, riflecrosshairtex, "crosshairs/simple-01", 3);
    TVAR(IDF_PERSIST, riflehithairtex, "crosshairs/simple-01-hit", 3);
    TVAR(IDF_PERSIST, grenadecrosshairtex, "crosshairs/circle-02", 3);
    TVAR(IDF_PERSIST, grenadehithairtex, "crosshairs/circle-02-hit", 3);
    TVAR(IDF_PERSIST, rocketcrosshairtex, "crosshairs/circle-01", 3);
    TVAR(IDF_PERSIST, rockethithairtex, "crosshairs/circle-01-hit", 3);

    TVAR(IDF_PERSIST, editcursortex, "crosshairs/cross-01", 3);
    TVAR(IDF_PERSIST, speccursortex, "crosshairs/cross-01", 3);
    TVAR(IDF_PERSIST, tvcursortex, "", 3);
    TVAR(IDF_PERSIST, teamcrosshairtex, "", 3);
    VAR(IDF_PERSIST, cursorstyle, 0, 0, 1); // 0 = top left tracking, 1 = center
    FVAR(IDF_PERSIST, cursorsize, 0, 0.025f, 1000);
    FVAR(IDF_PERSIST, cursorblend, 0, 1, 1);

    TVAR(IDF_PERSIST, zoomcrosshairtex, "crosshairs/cross-01", 3);
    FVAR(IDF_PERSIST, zoomcrosshairsize, 0, 0.575f, 1000);

    VAR(IDF_PERSIST, showinventory, 0, 1, 1);
    VAR(IDF_PERSIST, inventorybg, 0, 1, 1);
    VAR(IDF_PERSIST, inventoryammo, 0, 1, 2);
    VAR(IDF_PERSIST, inventoryhidemelee, 0, 1, 1);
    VAR(IDF_PERSIST, inventorygame, 0, 2, 2);
    VAR(IDF_PERSIST, inventoryscore, 0, 1, VAR_MAX);
    VAR(IDF_PERSIST, inventoryscorebg, 0, 0, 1);
    VAR(IDF_PERSIST, inventoryweapids, 0, 1, 2);
    VAR(IDF_PERSIST, inventorycolour, 0, 2, 2);
    VAR(IDF_PERSIST, inventoryflash, 0, 0, 1);
    FVAR(IDF_PERSIST, inventorybgsize, 0, 0.05f, 1);
    FVAR(IDF_PERSIST, inventorysize, 0, 0.06f, 1000);
    FVAR(IDF_PERSIST, inventoryskew, 1e-4f, 0.65f, 1000);
    FVAR(IDF_PERSIST, inventoryscoresize, 0, 0.65f, 1);
    FVAR(IDF_PERSIST, inventoryscoreshrink, 0, 0.15f, 1);
    FVAR(IDF_PERSIST, inventoryscoreshrinkmax, 0, 0.6f, 1);
    FVAR(IDF_PERSIST, inventoryblend, 0, 1, 1);
    FVAR(IDF_PERSIST, inventoryglow, 0, 0.05f, 1);
    FVAR(IDF_PERSIST, inventoryglowblend, 0, 1, 1);

    VAR(IDF_PERSIST, inventoryedit, 0, 1, 1);
    FVAR(IDF_PERSIST, inventoryeditblend, 0, 1, 1);
    FVAR(IDF_PERSIST, inventoryeditskew, 1e-4f, 0.5f, 1000);

    VAR(IDF_PERSIST, inventoryhealth, 0, 3, 3); // 0 = off, 1 = text, 2 = bar, 3 = bar + text
    VAR(IDF_PERSIST, healthflash, 0, 1, 1);
    FVAR(IDF_PERSIST, healththrob, 0, 0.035f, 1);
    FVAR(IDF_PERSIST, healthbartop, 0, 0.09375f, 1); // starts from this offset
    FVAR(IDF_PERSIST, healthbarbottom, 0, 0.0859375f, 1); // ends at this offset
    FVAR(IDF_PERSIST, healthbgglow, 0, 0.05f, 1);
    FVAR(IDF_PERSIST, healthbgblend, 0, 1, 1);

    VAR(IDF_PERSIST, inventoryimpulse, 0, 2, 2); // 0 = off, 1 = text, 2 = bar
    VAR(IDF_PERSIST, impulseflash, 0, 1, 1);
    FVAR(IDF_PERSIST, impulsebartop, 0, 0.171875f, 1); // starts from this offset
    FVAR(IDF_PERSIST, impulsebarbottom, 0, 0.0859375f, 1); // ends at this offset
    FVAR(IDF_PERSIST, impulsebgglow, 0, 0.05f, 1);
    FVAR(IDF_PERSIST, impulsebgblend, 0, 1, 1);

    VAR(IDF_PERSIST, inventoryvelocity, 0, 0, 2);
    VAR(IDF_PERSIST, inventorytrial, 0, 2, 2);
    VAR(IDF_PERSIST, inventorystatus, 0, 3, 3); // 0 = off, 1 = text, 2 = icon, 3 = icon + tex

    VAR(IDF_PERSIST, inventoryresidual, 0, 1, 1);
    VAR(IDF_PERSIST, residualflash, 0, 1, 1);
    TVAR(IDF_PERSIST, burningtex, "<grey>textures/alertburn", 3);
    TVAR(IDF_PERSIST, bleedingtex, "<grey>textures/alertbleed", 3);

    TVAR(IDF_PERSIST, meleetex, "<grey>textures/melee", 3);
    TVAR(IDF_PERSIST, pistoltex, "<grey>textures/pistol", 3);
    TVAR(IDF_PERSIST, swordtex, "<grey>textures/sword", 3);
    TVAR(IDF_PERSIST, shotguntex, "<grey>textures/shotgun", 3);
    TVAR(IDF_PERSIST, smgtex, "<grey>textures/smg", 3);
    TVAR(IDF_PERSIST, grenadetex, "<grey>textures/grenade", 3);
    TVAR(IDF_PERSIST, rockettex, "<grey>textures/rocket", 3);
    TVAR(IDF_PERSIST, flamertex, "<grey>textures/flamer", 3);
    TVAR(IDF_PERSIST, plasmatex, "<grey>textures/plasma", 3);
    TVAR(IDF_PERSIST, rifletex, "<grey>textures/rifle", 3);

    VAR(IDF_PERSIST, showclips, 0, 2, 2);
    FVAR(IDF_PERSIST, clipsize, 0, 0.045f, 1000);
    FVAR(IDF_PERSIST, clipblend, 0, 1, 1000);
    FVAR(IDF_PERSIST, clipcolour, 0, 1, 1);
    TVAR(IDF_PERSIST, pistolcliptex, "<grey>textures/pistolclip", 3);
    TVAR(IDF_PERSIST, shotguncliptex, "<grey>textures/shotgunclip", 3);
    TVAR(IDF_PERSIST, smgcliptex, "<grey>textures/smgclip", 3);
    TVAR(IDF_PERSIST, grenadecliptex, "<grey>textures/grenadeclip", 3);
    TVAR(IDF_PERSIST, rocketcliptex, "<grey>textures/rocketclip", 3);
    TVAR(IDF_PERSIST, flamercliptex, "<grey>textures/flamerclip", 3);
    TVAR(IDF_PERSIST, plasmacliptex, "<grey>textures/plasmaclip", 3);
    TVAR(IDF_PERSIST, riflecliptex, "<grey>textures/rifleclip", 3);
    FVAR(IDF_PERSIST, pistolclipskew, 0, 0.85f, 1000);
    FVAR(IDF_PERSIST, shotgunclipskew, 0, 1, 1000);
    FVAR(IDF_PERSIST, smgclipskew, 0, 0.85f, 1000);
    FVAR(IDF_PERSIST, grenadeclipskew, 0, 1.25f, 1000);
    FVAR(IDF_PERSIST, rocketclipskew, 0, 1, 1000);
    FVAR(IDF_PERSIST, flamerclipskew, 0, 0.85f, 1000);
    FVAR(IDF_PERSIST, plasmaclipskew, 0, 0.85f, 1000);
    FVAR(IDF_PERSIST, rifleclipskew, 0, 1, 1000);

    VAR(IDF_PERSIST, showradar, 0, 2, 2);
    VAR(IDF_PERSIST, radarstyle, 0, 1, 2); // 0 = compass-sectional, 1 = compass-distance, 2 = right-corner-positional
    FVAR(IDF_PERSIST, radaraspect, 0, 1, 2); // 0 = off, else = (for radarstyle 0/1) radar forms an ellipse
    TVAR(IDF_PERSIST, radarcornertex, "<grey>textures/radar", 3);
    TVAR(IDF_PERSIST, bliptex, "<grey>textures/blip", 3);
    TVAR(IDF_PERSIST, playerbliptex, "<grey>textures/blip", 3);
    TVAR(IDF_PERSIST, flagtex, "<grey>textures/flag", 3);
    TVAR(IDF_PERSIST, bombtex, "<grey>textures/bomb", 3);
    TVAR(IDF_PERSIST, arrowtex, "<grey>textures/arrow", 3);
    TVAR(IDF_PERSIST, alerttex, "<grey>textures/alert", 3);
    TVAR(IDF_PERSIST, questiontex, "<grey>textures/question", 3);
    TVAR(IDF_PERSIST, hurttex, "<grey>textures/hurt", 3);
    TVAR(IDF_PERSIST, hinttex, "<grey>textures/hint", 3);
    FVAR(IDF_PERSIST, radarblend, 0, 1, 1);
    FVAR(IDF_PERSIST, radarplayerblend, 0, 1, 1);
    FVAR(IDF_PERSIST, radarplayerhintblend, 0, 1, 1);
    FVAR(IDF_PERSIST, radarplayersize, 0, 0.4f, 1000);
    FVAR(IDF_PERSIST, radarplayerhintsize, 0, 0.65f, 1);
    FVAR(IDF_PERSIST, radarblipblend, 0, 1, 1);
    FVAR(IDF_PERSIST, radarblipsize, 0, 0.5f, 1000);
    FVAR(IDF_PERSIST, radaraffinityblend, 0, 1, 1);
    FVAR(IDF_PERSIST, radaraffinitysize, 0, 1, 1000);
    FVAR(IDF_PERSIST, radaritemblend, 0, 1, 1);
    FVAR(IDF_PERSIST, radaritemsize, 0, 0.75f, 1000);
    FVAR(IDF_PERSIST, radarsize, 0, 0.045f, 1000);
    FVAR(IDF_PERSIST, radaroffset, 0, 0.065f, 1000);
    FVAR(IDF_PERSIST, radarcorner, 0, 0.15f, 1000);
    FVAR(IDF_PERSIST, radarcornersize, 0, 0.04f, 1000);
    FVAR(IDF_PERSIST, radarcorneroffset, 0, 0.045f, 1);
    FVAR(IDF_PERSIST, radarcornerblend, 0, 1, 1);
    FVAR(IDF_PERSIST, radartexblend, 0, 1, 1);
    FVAR(IDF_PERSIST, radartexbright, 0, 0.65f, 1);
    FVAR(IDF_PERSIST, radarcornerbright, 0, 0.8f, 1);
    VAR(IDF_PERSIST, radardist, 0, 512, VAR_MAX); // 0 = use world size
    VAR(IDF_PERSIST, radarcornerdist, 0, 512, VAR_MAX); // 0 = use world size
    VAR(IDF_PERSIST, radaritems, 0, 2, 2);
    VAR(IDF_PERSIST, radaritemspawn, 0, 1, 1);
    VAR(IDF_PERSIST, radaritemtime, 0, 5000, VAR_MAX);
    VAR(IDF_PERSIST, radaritemnames, 0, 0, 2);
    VAR(IDF_PERSIST, radarplayers, 0, 2, 2);
    VAR(IDF_PERSIST, radarplayerfilter, 0, 0, 3); // 0 = off, 1 = non-team, 2 = team, 3 = only in duel/survivor/edit/tv
    VAR(IDF_PERSIST, radarplayernames, 0, 0, 2);
    VAR(IDF_PERSIST, radarplayereffects, 0, 1, 1);
    VAR(IDF_PERSIST, radaraffinity, 0, 2, 2);
    VAR(IDF_PERSIST, radaraffinitynames, 0, 1, 2);

    VAR(IDF_PERSIST, radardamage, 0, 3, 5); // 0 = off, 1 = basic damage, 2 = with killer announce (+1 killer track, +2 and bots), 5 = verbose
    VAR(IDF_PERSIST, radardamagemerge, 1, 250, VAR_MAX);
    VAR(IDF_PERSIST, radardamagetime, 1, 250, VAR_MAX);
    VAR(IDF_PERSIST, radardamagefade, 1, 3500, VAR_MAX);
    FVAR(IDF_PERSIST, radardamagesize, 0, 20, 1000);
    FVAR(IDF_PERSIST, radardamageblend, 0, 1, 1);
    FVAR(IDF_PERSIST, radardamagetrack, 0, 1, 1000);
    VAR(IDF_PERSIST, radardamagemin, 1, 10, VAR_MAX);
    VAR(IDF_PERSIST, radardamagemax, 1, 100, VAR_MAX);

    VAR(IDF_PERSIST, showeditradar, 0, 1, 1);
    VAR(IDF_PERSIST, editradardist, 0, 128, VAR_MAX); // 0 = use radardist
    VAR(IDF_PERSIST, editradarnoisy, 0, 1, 2);

    VAR(IDF_PERSIST, motionblurfx, 0, 1, 2); // 0 = off, 1 = on, 2 = override
    FVAR(IDF_PERSIST, motionblurmin, 0, 0.0f, 1); // minimum
    FVAR(IDF_PERSIST, motionblurmax, 0, 0.75f, 1); // maximum
    FVAR(IDF_PERSIST, motionbluramt, 0, 0.5f, 1); // used for override

    TVAR(IDF_PERSIST, bgtex, "textures/background", 3);
    TVAR(IDF_PERSIST, logotex, "textures/logo", 3);
    TVAR(IDF_PERSIST, badgetex, "textures/cube2badge", 3);

    TVAR(IDF_PERSIST, spree1tex, "textures/carnage", 3);
    TVAR(IDF_PERSIST, spree2tex, "textures/slaughter", 3);
    TVAR(IDF_PERSIST, spree3tex, "textures/massacre", 3);
    TVAR(IDF_PERSIST, spree4tex, "textures/bloodbath", 3);
    TVAR(IDF_PERSIST, multi1tex, "textures/double", 3);
    TVAR(IDF_PERSIST, multi2tex, "textures/triple", 3);
    TVAR(IDF_PERSIST, multi3tex, "textures/multi", 3);
    TVAR(IDF_PERSIST, headshottex, "textures/headshot", 3);
    TVAR(IDF_PERSIST, criticaltex, "textures/critical", 3);
    TVAR(IDF_PERSIST, dominatetex, "textures/dominate", 3);
    TVAR(IDF_PERSIST, revengetex, "textures/revenge", 3);
    TVAR(IDF_PERSIST, firstbloodtex, "textures/firstblood", 3);

    TVAR(IDF_PERSIST, modeeditingtex, "<grey>textures/modeediting.png", 3);
    TVAR(IDF_PERSIST, modecampaigntex, "<grey>textures/modecampaign.png", 3);
    TVAR(IDF_PERSIST, modedeathmatchtex, "<grey>textures/modedeathmatch.png", 3);
    TVAR(IDF_PERSIST, modetimetrialtex, "<grey>textures/modetimetrial.png", 3);

    TVAR(IDF_PERSIST, modecapturetex, "<grey>textures/modecapture.png", 3);
    TVAR(IDF_PERSIST, modecapturereturntex, "<grey>textures/modecapturereturn.png", 3);
    TVAR(IDF_PERSIST, modecapturedefendtex, "<grey>textures/modecapturedefend.png", 3);
    TVAR(IDF_PERSIST, modecaptureprotecttex, "<grey>textures/modecaptureprotect.png", 3);

    TVAR(IDF_PERSIST, modedefendtex, "<grey>textures/modedefend.png", 3);
    TVAR(IDF_PERSIST, modedefendquicktex, "<grey>textures/modedefendquick.png", 3);
    TVAR(IDF_PERSIST, modedefendconquertex, "<grey>textures/modedefendconquer.png", 3);

    TVAR(IDF_PERSIST, modebombertex, "<grey>textures/modebomber.png", 3);
    TVAR(IDF_PERSIST, modebomberbaskettex, "<grey>textures/modebomberbasket.png", 3);
    TVAR(IDF_PERSIST, modebomberholdtex, "<grey>textures/modebomberhold.png", 3);

    TVAR(IDF_PERSIST, modemultitex, "<grey>textures/modemulti.png", 3);
    TVAR(IDF_PERSIST, modeteamtex, "<grey>textures/modeteam.png", 3);
    TVAR(IDF_PERSIST, modeinstatex, "<grey>textures/modeinsta.png", 3);
    TVAR(IDF_PERSIST, modemedievaltex, "<grey>textures/modemedieval.png", 3);
    TVAR(IDF_PERSIST, modeballistictex, "<grey>textures/modeballistic.png", 3);
    TVAR(IDF_PERSIST, modedueltex, "<grey>textures/modeduel.png", 3);
    TVAR(IDF_PERSIST, modesurvivortex, "<grey>textures/modesurvivor.png", 3);
    TVAR(IDF_PERSIST, modearenatex, "<grey>textures/modearena.png", 3);
    TVAR(IDF_PERSIST, modeonslaughttex, "<grey>textures/modeonslaught.png", 3);
    TVAR(IDF_PERSIST, modehovertex, "<grey>textures/modehover.png", 3);
    TVAR(IDF_PERSIST, modejetpacktex, "<grey>textures/modejetpack.png", 3);
    TVAR(IDF_PERSIST, modevampiretex, "<grey>textures/modevampire.png", 3);
    TVAR(IDF_PERSIST, modeexperttex, "<grey>textures/modeexpert.png", 3);
    TVAR(IDF_PERSIST, moderesizetex, "<grey>textures/moderesize.png", 3);

    void modetexs(int g, int m, bool implied, vector<char> &list)
    {
        server::modecheck(g, m);
        #define ADDMODE(s) { list.put(s, strlen(s)); list.add(' '); }
        if(m_edit(g)) ADDMODE(modeeditingtex)
        else if(m_campaign(g)) ADDMODE(modecampaigntex)
        else if(m_trial(g)) ADDMODE(modetimetrialtex)
        else if(m_capture(g))
        {
            if(m_gsp1(g, m)) ADDMODE(modecapturereturntex)
            else if(m_gsp2(g, m)) ADDMODE(modecapturedefendtex)
            else if(m_gsp3(g, m)) ADDMODE(modecaptureprotecttex)
            else ADDMODE(modecapturetex)
        }
        else if(m_defend(g))
        {
            if(m_gsp1(g, m)) ADDMODE(modedefendquicktex)
            else if(m_gsp2(g, m)) ADDMODE(modedefendconquertex)
            else ADDMODE(modedefendtex)
        }
        else if(m_bomber(g))
        {
            if(m_gsp1(g, m)) ADDMODE(modebomberbaskettex)
            else if(m_gsp2(g, m)) ADDMODE(modebomberholdtex)
            else ADDMODE(modebombertex)
        }
        else ADDMODE(modedeathmatchtex)

        if(m_multi(g, m) && (implied || !(m_implied(g, m)&G_M_MULTI))) ADDMODE(modemultitex)
        if(m_team(g, m) && (implied || !(m_implied(g, m)&G_M_TEAM))) ADDMODE(modeteamtex)
        if(m_insta(g, m) && (implied || !(m_implied(g, m)&G_M_INSTA))) ADDMODE(modeinstatex)
        if(m_medieval(g, m) && (implied || !(m_implied(g, m)&G_M_MEDIEVAL))) ADDMODE(modemedievaltex)
        if(m_ballistic(g, m) && (implied || !(m_implied(g, m)&G_M_BALLISTIC))) ADDMODE(modeballistictex)
        if(m_duel(g, m) && (implied || !(m_implied(g, m)&G_M_DUEL))) ADDMODE(modedueltex)
        if(m_survivor(g, m) && (implied || !(m_implied(g, m)&G_M_SURVIVOR))) ADDMODE(modesurvivortex)
        if(m_arena(g, m) && (implied || !(m_implied(g, m)&G_M_ARENA))) ADDMODE(modearenatex)
        if(m_onslaught(g, m) && (implied || !(m_implied(g, m)&G_M_ONSLAUGHT))) ADDMODE(modeonslaughttex)
        if(m_hover(g, m) && (implied || !(m_implied(g, m)&G_M_HOVER))) ADDMODE(modehovertex)
        if(m_jetpack(g, m) && (implied || !(m_implied(g, m)&G_M_JETPACK))) ADDMODE(modejetpacktex)
        if(m_vampire(g, m) && (implied || !(m_implied(g, m)&G_M_VAMPIRE))) ADDMODE(modevampiretex)
        if(m_expert(g, m) && (implied || !(m_implied(g, m)&G_M_EXPERT))) ADDMODE(modeexperttex)
        if(m_resize(g, m) && (implied || !(m_implied(g, m)&G_M_RESIZE))) ADDMODE(moderesizetex)
    }

    ICOMMAND(0, modetexlist, "iii", (int *g, int *m, int *p), vector<char> list; modetexs(*g, *m, *p!=0, list); result(list.getbuf()));

    bool needminimap() { return true; }

    bool chkcond(int val, bool cond)
    {
        if(val == 2 || (val && cond)) return true;
        return false;
    }

    bool hasinput(bool pass, bool focus)
    {
        if(focus && (commandmillis > 0 || curcompass)) return true;
        return UI::active(pass);
    }

    bool keypress(int code, bool isdown, int cooked)
    {
        if(curcompass) return keycmenu(code, isdown, cooked);
        return UI::keypress(code, isdown, cooked); // ignore UI if compass is open
    }

    char *timetostr(int dur, int style)
    {
        static string timestr; timestr[0] = 0;
        int tm = dur, ms = 0, ss = 0, mn = 0;
        if(style < 2 && tm > 0)
        {
            ms = tm%1000;
            tm = (tm-ms)/1000;
        }
        if(style < 0 && tm > 0) ss = tm;
        else if(style < 3 && tm > 0)
        {
            ss = tm%60;
            tm = (tm-ss)/60;
            if(tm > 0) mn = tm;
        }
        switch(style)
        {
            case -1: formatstring(timestr)("%d.%d", ss, ms/100); break;
            case 0: formatstring(timestr)("%d:%02d.%03d", mn, ss, ms); break;
            case 1: formatstring(timestr)("%d:%02d.%d", mn, ss, ms/100); break;
            case 2: formatstring(timestr)("%d:%02d", mn, ss); break;
        }
        return timestr;
    }
    ICOMMAND(0, timetostr, "ii", (int *d, int *s), result(timetostr(*d, *s)));

    float motionblur(float scale)
    {
        float amt = 0;
        switch(motionblurfx)
        {
            case 1:
            {
                if(game::focus->state >= CS_SPECTATOR || game::focus->state == CS_EDITING) break;
                float damage = game::focus->state == CS_ALIVE ? min(damageresidue, 100)/100.f : 1.f,
                      healthscale = float(m_health(game::gamemode, game::mutators));
                if(healthscale > 0) damage = max(damage, 1.f-max(game::focus->health, 0)/healthscale);
                amt += damage*0.65f;
                if(burntime && game::focus->burning(lastmillis, burntime))
                    amt += 0.25f+(float((lastmillis-game::focus->lastburn)%burndelay)/float(burndelay))*0.35f;
                if(bleedtime && game::focus->bleeding(lastmillis, bleedtime))
                    amt += 0.25f+(float((lastmillis-game::focus->lastbleed)%bleeddelay)/float(bleeddelay))*0.35f;
                if(game::focus->turnside || game::focus->impulse[IM_JUMP])
                    amt += game::focus->turnside ? 0.125f : 0.25f;
                if(physics::hover(game::focus)) amt += 0.125f;
                break;
            }
            case 2: amt += motionbluramt; break;
            default: break;
        }
        return clamp(amt, motionblurmin, motionblurmax)*scale;
    }

    void damage(int n, const vec &loc, gameent *actor, int weap, int flags)
    {
        damageresidue = clamp(damageresidue+(n*(flags&HIT_BLEED ? 3 : 1)), 0, 200);
        vec colour = doesburn(weap, flags) ? vec(1.f, 0.35f, 0.0625f) : (kidmode || game::bloodscale <= 0 ? vec(1, 0.25f, 1) : vec(1.f, 0, 0)),
            dir = vec(loc).sub(camera1->o).normalize();
        loopv(damagelocs)
        {
            damageloc &d = damagelocs[i];
            if(actor->clientnum != d.attacker) continue;
            if(lastmillis-d.outtime > radardamagemerge) continue;
            if(d.colour != colour) continue;
            d.damage += n;
            d.dir = dir;
            return; // accumulate
        }
        damagelocs.add(damageloc(actor->clientnum, lastmillis, n, dir, colour));
    }

    void drawquad(float x, float y, float w, float h, float tx1, float ty1, float tx2, float ty2, bool flipx, bool flipy)
    {
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(flipx ? tx2 : tx1, flipy ? ty2 : ty1); glVertex2f(x, y);
        glTexCoord2f(flipx ? tx1 : tx2, flipy ? ty2 : ty1); glVertex2f(x+w, y);
        glTexCoord2f(flipx ? tx2 : tx1, flipy ? ty1 : ty2); glVertex2f(x, y+h);
        glTexCoord2f(flipx ? tx1 : tx2, flipy ? ty1 : ty2); glVertex2f(x+w, y+h);
        glEnd();
    }
    void drawcoord(float x, float y, float w, float h, float tx, float ty, float tw, float th, bool flipx, bool flipy) { drawquad(x, y, w, h, tx, ty, tx+tw, ty+th, flipx, flipy); }
    void drawtexture(float x, float y, float w, float h, bool flipx, bool flipy) { drawquad(x, y, w, h, 0, 0, 1, 1, flipx, flipy); }
    void drawsized(float x, float y, float s, bool flipx, bool flipy) { drawquad(x, y, s, s, 0, 0, 1, 1, flipx, flipy); }

    void drawblend(int x, int y, int w, int h, float r, float g, float b, bool blend = false)
    {
        if(!blend) glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        glColor3f(r, g, b);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex2f(x, y);
        glVertex2f(x+w, y);
        glVertex2f(x, y+h);
        glVertex2f(x+w, y+h);
        glEnd();
        if(!blend) glDisable(GL_BLEND);
        else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void colourskew(float &r, float &g, float &b, float skew)
    {
        if(skew >= 2.f)
        { // fully overcharged to green
            r = b = 0;
        }
        else if(skew >= 1.f)
        { // overcharge to yellow
            b *= 1.f-(skew-1.f);
        }
        else if(skew >= 0.5f)
        { // fade to orange
            float off = skew-0.5f;
            g *= 0.5f+off;
            b *= off*2.f;
        }
        else
        { // fade to red
            g *= skew;
            b = 0;
        }
    }

    template<class T>
    void skewcolour(T &r, T &g, T &b, int colour = 0, bool faded = false)
    {
        if(colour < 0) colour = game::getcolour(game::focus, -1-colour);
        vec c = vec::hexcolor(colour);
        r = T(r*c.r);
        g = T(g*c.g);
        b = T(b*c.b);
        if(!game::focus->team && faded)
        {
            float f = game::focus->state == CS_SPECTATOR || game::focus->state == CS_EDITING ? 0.25f : 0.5f;
            r = T(r*f);
            g = T(g*f);
            b = T(b*f);
        }
    }

    template<class T>
    void flashcolour(T &r, T &g, T &b, T br, T bg, T bb, float amt)
    {
        r += (br-r)*amt;
        g += (bg-g)*amt;
        b += (bb-b)*amt;
    }

    template<class T>
    void flashcolourf(T &r, T &g, T &b, T &f, T br, T bg, T bb, T bf, float amt)
    {
        r += (br-r)*amt;
        g += (bg-g)*amt;
        b += (bb-b)*amt;
        f += (bf-f)*amt;
    }

    enum
    {
        POINTER_NONE = 0, POINTER_RELATIVE, POINTER_GUI, POINTER_EDIT, POINTER_SPEC,
        POINTER_HAIR, POINTER_TEAM, POINTER_ZOOM, POINTER_HIT, POINTER_MAX
    };

    const char *getpointer(int index, int weap = -1)
    {
        switch(index)
        {
            case POINTER_RELATIVE: return pointertex;
            case POINTER_GUI: return guicursortex;
            case POINTER_EDIT: return editcursortex;
            case POINTER_SPEC: return game::tvmode() ? tvcursortex : speccursortex;
            case POINTER_HAIR:
            {
                if(crosshairweapons && isweap(weap))
                {
                    const char *crosshairtexs[WEAP_MAX] = {
                        meleecrosshairtex, pistolcrosshairtex, swordcrosshairtex, shotguncrosshairtex, smgcrosshairtex,
                        flamercrosshairtex, plasmacrosshairtex, riflecrosshairtex, grenadecrosshairtex, rocketcrosshairtex, // end of regular weapons
                    };
                    if(*crosshairtexs[weap]) return crosshairtexs[weap];
                }
                return crosshairtex;
            }
            case POINTER_TEAM: return teamcrosshairtex;
            case POINTER_ZOOM: return zoomcrosshairtex;
            case POINTER_HIT:
            {
                if(crosshairweapons && isweap(weap))
                {
                    const char *hithairtexs[WEAP_MAX] = {
                        meleehithairtex, pistolhithairtex, swordhithairtex, shotgunhithairtex, smghithairtex,
                        flamerhithairtex, plasmahithairtex, riflehithairtex, grenadehithairtex, rockethithairtex, // end of regular weapons
                    };
                    if(*hithairtexs[weap]) return hithairtexs[weap];
                }
                return hithairtex;
            }
            default: break;
        }
        return NULL;
    }
    ICOMMAND(0, getpointer, "ii", (int *i, int *j), result(getpointer(*i, *j)));

    void drawindicator(int weap, int x, int y, int s, bool secondary)
    {
        int millis = lastmillis-game::focus->weaplast[weap];
        float r = 1, g = 1, b = 1, amt = 0;
        switch(game::focus->weapstate[weap])
        {
            case WEAP_S_POWER: if(game::focus->weapwait[weap])
            {
                amt = clamp(float(millis)/float(game::focus->weapwait[weap]), 0.f, 1.f);
                colourskew(r, g, b, 1.f-amt);
                break;
            }
            case WEAP_S_RELOAD: if(showindicator >= (WEAP(weap, add) < WEAP(weap, max) ? 3 : 2) && millis <= game::focus->weapwait[weap])
            {
                amt = 1.f-clamp(float(millis)/float(game::focus->weapwait[weap]), 0.f, 1.f);
                colourskew(r, g, b, 1.f-amt);
                break;
            }
            default: amt = 0; break;
        }
        if(amt > 0)
        {
            Texture *t = textureload(indicatortex, 3);
            if(t->bpp == 2 || t->bpp == 4) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            else glBlendFunc(GL_ONE, GL_ONE);
            glBindTexture(GL_TEXTURE_2D, t->id);
            float val = amt < 0.25f ? amt : (amt > 0.75f ? 1.f-amt : 0.25f);
            glColor4f(val*4.f, val*4.f, val*4.f, indicatorblend*hudblend*val);
            drawsized(x-s, y-s, s*2);
            glColor4f(r, g, b, indicatorblend*hudblend);
            drawslice(0, clamp(amt, 0.f, 1.f), x, y, s);
        }
    }

    static int clipsizes[WEAP_MAX] = {0};

    void drawclip(int weap, int x, int y, float s)
    {
        if(!isweap(weap) || (!WEAP2(weap, sub, false) && !WEAP2(weap, sub, true)) || WEAP(weap, max) <= 1) return;
        const char *cliptexs[WEAP_MAX] = {
            progresstex, pistolcliptex, progresstex, shotguncliptex, smgcliptex,
            flamercliptex, plasmacliptex, riflecliptex, grenadecliptex, rocketcliptex, // end of regular weapons
        };
        if(!clipsizes[weap])
        {
            WEAPSTR(clipmax, weap, max);
            clipsizes[weap] = getvardef(clipmax);
        }
        int ammo = game::focus->ammo[weap], maxammo = WEAP(weap, max), weapid = weap;
        if(clipsizes[weap] != maxammo) weapid = 0;
        Texture *t = textureload(cliptexs[weapid], 3);
        if(t->bpp == 2 || t->bpp == 4) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        else glBlendFunc(GL_ONE, GL_ONE);

        const float clipskew[WEAP_MAX] = {
            1, pistolclipskew, 1, shotgunclipskew, smgclipskew,
            flamerclipskew, plasmaclipskew, rifleclipskew, grenadeclipskew, rocketclipskew, // end of regular weapons
        };
        float fade = clipblend*hudblend, size = s*clipskew[weapid];
        int interval = lastmillis-game::focus->weaplast[weap];
        if(interval <= game::focus->weapwait[weap]) switch(game::focus->weapstate[weap])
        {
            case WEAP_S_PRIMARY:
            case WEAP_S_SECONDARY:
            {
                float amt = 1.f-clamp(float(interval)/float(game::focus->weapwait[weap]), 0.f, 1.f);
                fade *= amt;
                if(showclips >= 2) size *= amt;
                break;
            }
            case WEAP_S_RELOAD:
            {
                if(game::focus->weapload[weap] > 0)
                {
                    int check = game::focus->weapwait[weap]/2;
                    if(interval >= check)
                    {
                        float amt = clamp(float(interval-check)/float(check), 0.f, 1.f);
                        fade *= amt;
                        if(showclips >= 2) size *= amt*3/4;
                    }
                    else
                    {
                        fade = 0.f;
                        if(showclips >= 2) size = 0;
                    }
                    break;
                }
                // falls through
            }
            case WEAP_S_USE: case WEAP_S_SWITCH:
            {
                float amt = clamp(float(interval)/float(game::focus->weapwait[weap]), 0.f, 1.f); fade *= amt;
                if(showclips >= 2 && game::focus->weapstate[weap] != WEAP_S_RELOAD) size *= amt;
                break;
            }
            default: break;
        }
        vec c(clipcolour, clipcolour, clipcolour);
        if(clipcolour > 0) c.mul(vec::hexcolor(WEAP(weap, colour)));
        else if(clipstone) skewcolour(c.r, c.g, c.b, clipstone);
        glColor4f(c.r, c.g, c.b, fade);
        glBindTexture(GL_TEXTURE_2D, t->id);
        if(interval <= game::focus->weapwait[weap]) switch(game::focus->weapstate[weap])
        {
            case WEAP_S_PRIMARY:
            case WEAP_S_SECONDARY:
            {
                int shot = game::focus->weapshot[weap] ? game::focus->weapshot[weap] : 1;
                if(shot) switch(weapid)
                {
                    case WEAP_FLAMER: case WEAP_ROCKET:
                        drawslice(ammo/float(maxammo), shot/float(maxammo), x, y, size);
                        break;
                    case WEAP_GRENADE:
                        drawslice(0.25f/maxammo+ammo/float(maxammo), shot/float(maxammo), x, y, size);
                        break;
                    default:
                        drawslice(0.5f/maxammo+ammo/float(maxammo), shot/float(maxammo), x, y, size);
                        break;
                }
                glColor4f(c.r, c.g, c.b, clipblend*hudblend);
                size = s*clipskew[weapid];
                break;
            }
            case WEAP_S_RELOAD:
            {
                if(game::focus->weapload[weap] > 0)
                {
                    ammo -= game::focus->weapload[weap];
                    switch(weapid)
                    {
                        case WEAP_FLAMER: case WEAP_ROCKET:
                            drawslice(ammo/float(maxammo), game::focus->weapload[weap]/float(maxammo), x, y, size);
                            break;
                        case WEAP_GRENADE:
                            drawslice(0.25f/maxammo+ammo/float(maxammo), game::focus->weapload[weap]/float(maxammo), x, y, size);
                            break;
                        default:
                            drawslice(0.5f/maxammo+ammo/float(maxammo), game::focus->weapload[weap]/float(maxammo), x, y, size);
                            break;
                    }
                    glColor4f(c.r, c.g, c.b, clipblend*hudblend);
                    size = s*clipskew[weapid];
                }
                break;
            }
            default: break;
        }
        if(ammo > 0) switch(weapid)
        {
            case WEAP_FLAMER: case WEAP_ROCKET:
                drawslice(0, ammo/float(maxammo), x, y, size);
                break;
            case WEAP_GRENADE:
                drawslice(0.25f/maxammo, ammo/float(maxammo), x, y, size);
                break;
            default:
                drawslice(0.5f/maxammo, ammo/float(maxammo), x, y, size);
                break;
        }
    }

    void drawpointertex(const char *tex, int x, int y, int s, float r, float g, float b, float fade)
    {
        Texture *t = tex && *tex ? textureload(tex, 3) : NULL;
        if(t && t != notexture)
        {
            if(t->bpp == 2 || t->bpp == 4) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            else glBlendFunc(GL_ONE, GL_ONE);
            glColor4f(r, g, b, fade);
            glBindTexture(GL_TEXTURE_2D, t->id);
            drawsized(x, y, s);
        }
    }

    void drawpointer(int w, int h, int index)
    {
        int cs = int((index == POINTER_GUI ? cursorsize : crosshairsize)*hudsize);
        float fade = (index == POINTER_GUI ? cursorblend : crosshairblend)*hudblend;
        vec c(1, 1, 1);
        if(game::focus->state == CS_ALIVE && index >= POINTER_HAIR)
        {
            if(crosshairweapons >= 2) c = vec::hexcolor(WEAP(game::focus->weapselect, colour));
            if(index == POINTER_ZOOM && game::inzoom() && WEAP(game::focus->weapselect, zooms))
            {
                int frame = lastmillis-game::lastzoom, off = int(zoomcrosshairsize*hudsize)-cs;
                float amt = frame <= zoomtime ? clamp(float(frame)/float(zoomtime), 0.f, 1.f) : 1.f;
                if(!game::zooming) amt = 1.f-amt;
                cs += int(off*amt);
            }
            if(crosshairtone) skewcolour(c.r, c.g, c.b, crosshairtone);
            int heal = m_health(game::gamemode, game::mutators);
            if(crosshairflash && game::focus->state == CS_ALIVE && game::focus->health < heal)
            {
                int timestep = totalmillis%1000;
                float amt = clamp((timestep <= 500 ? timestep/500.f : (1000-timestep)/500.f)*(float(heal-game::focus->health)/float(heal)), 0.f, 1.f);
                flashcolour(c.r, c.g, c.b, 1.f, 0.f, 0.f, amt);
            }
            if(crosshairthrob > 0 && regentime && game::focus->lastregen && lastmillis-game::focus->lastregen <= regentime)
            {
                float skew = clamp((lastmillis-game::focus->lastregen)/float(regentime/2), 0.f, 2.f);
                cs += int(cs*(skew > 1.f ? 1.f-skew : skew)*crosshairthrob);
            }
            if(showcrosshair >= 2) fade /= weapons::accmod(game::focus, physics::secondaryweap(game::focus, true))*crosshairaccamt;
        }
        int cx = int(hudwidth*cursorx), cy = int(hudheight*cursory);
        if(index != POINTER_GUI)
        {
            int nx = int(hudwidth*0.5f), ny = int(hudheight*0.5f), ss = int(crosshairsize*hudsize),
                sx = game::mousestyle() != 1 ? cx : nx, sy = game::mousestyle() != 1 ? cy : ny;
            drawpointertex(getpointer(index, game::focus->weapselect), sx-cs/2, sy-cs/2, cs, c.r, c.g, c.b, fade);
            if(index > POINTER_GUI)
            {
                if(game::focus->state == CS_ALIVE && game::focus->hasweap(game::focus->weapselect, m_weapon(game::gamemode, game::mutators)))
                {
                    if(showclips) drawclip(game::focus->weapselect, nx, ny, clipsize*hudsize);
                    if(showindicator) drawindicator(game::focus->weapselect, nx, ny, int(indicatorsize*hudsize), physics::secondaryweap(game::focus));
                }
                if(game::mousestyle() >= 1) // renders differently
                    drawpointertex(getpointer(POINTER_RELATIVE), (game::mousestyle() != 1 ? nx : cx)-ss/2, (game::mousestyle() != 1 ? ny : cy)-ss/2, ss, c.r, c.g, c.b, crosshairblend*hudblend);
                if(crosshairhitspeed && totalmillis-game::focus->lasthit <= crosshairhitspeed)
                    drawpointertex(getpointer(POINTER_HIT, game::focus->weapselect), sx-ss/2, sy-ss/2, ss, c.r, c.g, c.b, crosshairblend*hudblend);
            }
        }
        else drawpointertex(getpointer(index, game::focus->weapselect), cx, cy, cs, c.r, c.g, c.b, fade);
    }

    void drawpointers(int w, int h)
    {
        int index = POINTER_NONE;
        if(hasinput()) index = !hasinput(true) || commandmillis > 0 ? POINTER_NONE : POINTER_GUI;
        else if(!showhud || !showcrosshair || game::focus->state == CS_DEAD || client::waiting() || (game::thirdpersonview(true) && game::focus != game::player1))
            index = POINTER_NONE;
        else if(game::focus->state == CS_EDITING) index = POINTER_EDIT;
        else if(game::focus->state >= CS_SPECTATOR) index = POINTER_SPEC;
        else if(game::inzoom() && WEAP(game::focus->weapselect, zooms)) index = POINTER_ZOOM;
        else if(m_team(game::gamemode, game::mutators))
        {
            vec pos = game::focus->headpos();
            gameent *d = game::intersectclosest(pos, worldpos, game::focus);
            if(d && d->type == ENT_PLAYER && d->team == game::focus->team)
                index = POINTER_TEAM;
            else index = POINTER_HAIR;
        }
        else index = POINTER_HAIR;
        if(index > POINTER_NONE)
        {
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, hudwidth, hudheight, 0, -1, 1);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            drawpointer(w, h, index);
            glDisable(GL_BLEND);
        }
    }

    int numteamkills()
    {
        int numkilled = 0;
        loopvrev(teamkills)
        {
            if(totalmillis-teamkills[i] <= teamkilltime) numkilled++;
            else teamkills.remove(i);
        }
        return numkilled;
    }

    bool showname()
    {
        if(game::focus != game::player1)
        {
            if(game::thirdpersonview(true) && game::aboveheadnames > 1) return false;
            return true;
        }
        return false;
    }

    void drawnotices()
    {
        glPushMatrix();
        glScalef(noticescale, noticescale, 1);
        pushfont("default");
        int ty = ((hudheight/2)+int(hudheight/2*noticeoffset))*(1.f/noticescale), tx = (hudwidth/2)*(1.f/noticescale),
            tf = int(255*hudblend*noticeblend), tr = 255, tg = 255, tb = 255,
            tw = hudwidth-(int(hudsize*gapsize)*2+int(hudsize*inventorysize)*2);
        if(noticestone) skewcolour(tr, tg, tb, noticestone);
        if(lastmillis-game::maptime <= noticetitle)
        {

            ty += draw_textx("%s", tx, ty, 255, 255, 255, tf, TEXT_CENTERED, -1, tw, *maptitle ? maptitle : mapname);
            pushfont("reduced");
            if(*mapauthor) ty += draw_textx("by %s", tx, ty, 255, 255, 255, tf, TEXT_CENTERED, -1, tw, mapauthor);
            popfont();
            pushfont("little");
            defformatstring(gname)("%s", server::gamename(game::gamemode, game::mutators));
            if(strlen(gname) > 32) formatstring(gname)("%s", server::gamename(game::gamemode, game::mutators, 1));
            ty += draw_textx("[ \fs\fa%s\fS ]", tx, ty, 255, 255, 255, tf, TEXT_CENTERED, -1, tw, gname);
            popfont();
        }

        if(game::player1->state == CS_SPECTATOR)
            ty += draw_textx("[ %s ]", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, showname() ? game::colorname(game::focus) : (game::tvmode() ? "SpecTV" : "Spectating"));
        else if(game::player1->state == CS_WAITING && showname())
            ty += draw_textx("[ %s ]", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, game::colorname(game::focus));

        gameent *target = game::player1->state != CS_SPECTATOR ? game::player1 : game::focus;
        if(target->state == CS_DEAD || target->state == CS_WAITING)
        {
            int sdelay = m_delay(game::gamemode, game::mutators), delay = target->lastdeath ? target->respawnwait(lastmillis, sdelay) : 0;
            const char *msg = target->state == CS_WAITING && target == game::player1 ? "Please Wait" : "Fragged";
            ty += draw_textx("%s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, msg);
            if(obitnotices && target->lastdeath && (delay || target->state == CS_DEAD) && *target->obit)
            {
                pushfont("reduced");
                ty += draw_textx("%s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, target->obit);
                popfont();
            }
            if(shownotices >= 2)
            {
                SEARCHBINDCACHE(attackkey)("action 0", 0);
                if(delay || m_campaign(game::gamemode) || (m_trial(game::gamemode) && !target->lastdeath) || m_duke(game::gamemode, game::mutators) || (m_fight(game::gamemode) && maxalive > 0))
                {
                    pushfont("reduced");
                    if(m_duke(game::gamemode, game::mutators)) ty += draw_textx("Queued for new round", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw);
                    else if(delay) ty += draw_textx("Down for \fs\fy%s\fS", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, timetostr(delay, -1));
                    else if(target == game::player1 && target->state == CS_WAITING && m_fight(game::gamemode) && maxalive > 0 && maxalivequeue)
                    {
                        int n = game::numwaiting();
                        if(n) ty += draw_textx("Respawn queued, waiting for \fs\fy%d\fS %s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, n, n != 1 ? "players" : "player");
                        else ty += draw_textx("Prepare to respawn, you are \fs\fgnext\fS in the queue", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw);
                    }
                    popfont();
                    if(target == game::player1 && target->state != CS_WAITING && shownotices >= 3 && lastmillis-target->lastdeath >= 500)
                    {
                        pushfont("little");
                        ty += draw_textx("Press \fs\fc%s\fS to enter respawn queue", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, attackkey);
                        popfont();
                    }
                }
                else
                {
                    pushfont("reduced");
                    ty += draw_textx("Ready to respawn", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1);
                    popfont();
                    if(target == game::player1 && target->state != CS_WAITING && shownotices >= 3)
                    {
                        pushfont("little");
                        ty += draw_textx("Press \fs\fc%s\fS to respawn now", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, attackkey);
                        popfont();
                    }
                }
                if(target == game::player1 && target->state == CS_WAITING && shownotices >= 3)
                {
                    SEARCHBINDCACHE(waitmodekey)("waitmodeswitch", 3);
                    pushfont("little");
                    ty += draw_textx("Press \fs\fc%s\fS to %s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, waitmodekey, game::tvmode() ? "interact" : "switch to TV");
                    popfont();
                }
                if(target == game::player1 && m_arena(game::gamemode, game::mutators))
                {
                    SEARCHBINDCACHE(loadkey)("showgui loadout", 0);
                    pushfont("little");
                    ty += draw_textx("Press \fs\fc%s\fS to \fs%s\fS loadout", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, loadkey, target->loadweap[0] < 0 ? "\fzoyselect" : "change");
                    popfont();
                }
                if(target == game::player1 && m_fight(game::gamemode) && m_team(game::gamemode, game::mutators))
                {
                    SEARCHBINDCACHE(teamkey)("showgui team", 0);
                    pushfont("little");
                    ty += draw_textx("Press \fs\fc%s\fS to change teams", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, teamkey);
                    popfont();
                }
            }
        }
        else if(target->state == CS_ALIVE)
        {
            if(m_fight(game::gamemode) && target == game::player1 && teamnotices)
            {
                if(teamkillnum && m_team(game::gamemode, game::mutators) && numteamkills() >= teamkillnum)
                    ty += draw_textx("\fzZyDon't shoot team mates", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1);
                if(teamnotices >= 2)
                {
                    if(target->state == CS_ALIVE && !lastteam) lastteam = totalmillis;
                    if(totalmillis-lastteam <= teamnoticedelay)
                    {
                        if(m_campaign(game::gamemode)) ty += draw_textx("Campaign Mission", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1);
                        else if(!m_team(game::gamemode, game::mutators))
                        {
                            if(m_trial(game::gamemode)) ty += draw_textx("Time Trial", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1);
                            else ty += draw_textx("\fzZeFree-for-all Deathmatch", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1);
                        }
                        else ty += draw_textx("\fzZeYou are on team \fs\f[%d]\f(%s)%s\fS", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, TEAM(target->team, colour), hud::teamtex(target->team), TEAM(target->team, name));
                    }
                }
            }
            if(obitnotices && totalmillis-target->lastkill <= noticetime && *target->obit)
            {
                pushfont("reduced");
                ty += draw_textx("%s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, target->obit);
                popfont();
            }
            if(target == game::player1 && shownotices >= 3 && game::allowmove(target))
            {
                pushfont("emphasis");
                static vector<actitem> actitems;
                actitems.setsize(0);
                if(entities::collateitems(target, actitems))
                {
                    SEARCHBINDCACHE(actionkey)("action 3", 0);
                    while(!actitems.empty())
                    {
                        actitem &t = actitems.last();
                        int ent = -1;
                        switch(t.type)
                        {
                            case ITEM_ENT:
                            {
                                if(!entities::ents.inrange(t.target)) break;
                                ent = t.target;
                                break;
                            }
                            case ITEM_PROJ:
                            {
                                if(!projs::projs.inrange(t.target)) break;
                                projent &proj = *projs::projs[t.target];
                                ent = proj.id;
                                break;
                            }
                            default: break;
                        }
                        if(entities::ents.inrange(ent))
                        {
                            extentity &e = *entities::ents[ent];
                            if(enttype[e.type].usetype == EU_ITEM)
                            {
                                int drop = -1, sweap = m_weapon(game::gamemode, game::mutators), attr = e.type == WEAPON ? w_attr(game::gamemode, e.attrs[0], sweap) : e.attrs[0];
                                if(target->canuse(e.type, attr, e.attrs, sweap, lastmillis, WEAP_S_ALL))
                                {
                                    if(e.type == WEAPON && w_carry(target->weapselect, sweap) && target->ammo[attr] < 0 && w_carry(attr, sweap) && target->carry(sweap) >= maxcarry)
                                        drop = target->drop(sweap);
                                    if(isweap(drop))
                                    {
                                        static struct dropattrs : attrvector { dropattrs() { add(0, 5); } } attrs;
                                        attrs[0] = drop;
                                        defformatstring(dropweap)("%s", entities::entinfo(WEAPON, attrs, false, true));
                                        ty += draw_textx("Press \fs\fc%s\fS to swap \fs%s\fS for \fs%s\fS", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, actionkey, dropweap, entities::entinfo(e.type, e.attrs, false, true));
                                    }
                                    else ty += draw_textx("Press \fs\fc%s\fS to pickup \fs%s\fS", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, actionkey, entities::entinfo(e.type, e.attrs, false, true));
                                    break;
                                }
                            }
                            else if(e.type == TRIGGER && e.attrs[2] == TA_ACTION)
                            {
                                ty += draw_textx("Press \fs\fc%s\fS to interact", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, actionkey);
                                break;
                            }
                        }
                        actitems.pop();
                    }
                }
                popfont();
                if(shownotices >= 4)
                {
                    pushfont("little");
                    if(target->canshoot(target->weapselect, 0, m_weapon(game::gamemode, game::mutators), lastmillis, (1<<WEAP_S_RELOAD)))
                    {
                        SEARCHBINDCACHE(attackkey)("action 0", 0);
                        ty += draw_textx("Press \fs\fc%s\fS to attack", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, attackkey);
                        SEARCHBINDCACHE(altkey)("action 1", 0);
                        ty += draw_textx("Press \fs\fc%s\fS to %s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, altkey, WEAP(target->weapselect, zooms) ? "zoom" : "alt-attack");
                    }
                    if(target->canreload(target->weapselect, m_weapon(game::gamemode, game::mutators), lastmillis))
                    {
                        SEARCHBINDCACHE(reloadkey)("action 2", 0);
                        ty += draw_textx("Press \fs\fc%s\fS to reload ammo", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, reloadkey);
                    }
                    popfont();
                }
            }
        }

        if(game::player1->state == CS_SPECTATOR)
        {
            SEARCHBINDCACHE(speconkey)("spectator 0", 1);
            pushfont("little");
            ty += draw_textx("Press \fs\fc%s\fS to join the game", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, speconkey);
            if(shownotices >= 2)
            {
                SEARCHBINDCACHE(specmodekey)("specmodeswitch", 1);
                ty += draw_textx("Press \fs\fc%s\fS to %s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, specmodekey, game::tvmode() ? "interact" : "switch to TV");
            }
            popfont();
        }

        if(m_capture(game::gamemode)) capture::drawnotices(hudwidth, hudheight, tx, ty, tf/255.f);
        else if(m_defend(game::gamemode)) defend::drawnotices(hudwidth, hudheight, tx, ty, tf/255.f);
        else if(m_bomber(game::gamemode)) bomber::drawnotices(hudwidth, hudheight, tx, ty, tf/255.f);

        popfont();
        glPopMatrix();
    }

    void drawlast()
    {
        if(!progressing && showhud)
        {
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, hudwidth, hudheight, 0, -1, 1);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            if(commandmillis <= 0 && curcompass) rendercmenu();
            else if(shownotices && !client::waiting() && !hasinput(false) && !texpaneltimer) drawnotices();
            if(overlaydisplay >= 2 || (game::focus->state == CS_ALIVE && (overlaydisplay || !game::thirdpersonview(true))))
            {
                Texture *t = *overlaytex ? textureload(overlaytex, 3) : notexture;
                if(t != notexture)
                {
                    glBindTexture(GL_TEXTURE_2D, t->id);
                    glColor4f(1.f, 1.f, 1.f, overlayblend*hudblend);
                    drawtexture(0, 0, hudwidth, hudheight);
                }
            }
            glDisable(GL_BLEND);
        }
        if(UI::ready && (progressing || (commandmillis <= 0 && !curcompass))) UI::render();
        if(!progressing) drawpointers(hudwidth, hudheight);
    }

    void drawconsole(int type, int w, int h, int x, int y, int s, float fade)
    {
        static vector<int> refs; refs.setsize(0);
        bool full = fullconsole || commandmillis > 0;
        int z = 0;
        pushfont("console");
        if(type >= 2)
        {
            int numl = chatconsize, numo = chatconsize+chatconoverflow;
            if(numl)
            {
                loopvj(conlines) if(conlines[j].type >= CON_CHAT)
                {
                    int len = !full && conlines[j].type > CON_CHAT ? chatcontime/2 : chatcontime;
                    if(full || totalmillis-conlines[j].reftime <= len+chatconfade)
                    {
                        if(refs.length() >= numl)
                        {
                            if(refs.length() >= numo)
                            {
                                if(full) break;
                                bool found = false;
                                loopvrev(refs) if(conlines[refs[i]].reftime+(conlines[refs[i]].type > CON_CHAT ? chatcontime/2 : chatcontime) < conlines[j].reftime+len)
                                {
                                    refs.remove(i);
                                    found = true;
                                    break;
                                }
                                if(!found) continue;
                            }
                            conlines[j].reftime = min(conlines[j].reftime, totalmillis-len);
                        }
                        refs.add(j);
                    }
                }
                int r = x+FONTW, z = 0;
                loopvj(refs)
                {
                    int len = !full && conlines[refs[j]].type > CON_CHAT ? chatcontime/2 : chatcontime;
                    float f = full || !chatconfade ? 1.f : clamp(((len+chatconfade)-(totalmillis-conlines[refs[j]].reftime))/float(chatconfade), 0.f, 1.f),
                        g = conlines[refs[j]].type > CON_CHAT ? conblend : chatconblend;
                    z += draw_textx("%s", r, y-z, 255, 255, 255, int(255*fade*f*g), TEXT_LEFT_UP, -1, s, conlines[refs[j]].cref)*f;
                }
            }
        }
        else
        {
            int numl = consize, numo = consize+conoverflow;
            if(numl)
            {
                loopvj(conlines) if(type ? conlines[j].type >= (confilter && !full ? CON_LO : 0) && conlines[j].type <= CON_HI : conlines[j].type >= (confilter && !full ? CON_LO : 0))
                {
                    int len = !full && conlines[j].type < CON_IMPORTANT ? contime/2 : contime;
                    if(conskip ? j>=conskip-1 || j>=conlines.length()-numl : full || totalmillis-conlines[j].reftime <= len+confade)
                    {
                        if(refs.length() >= numl)
                        {
                            if(refs.length() >= numo)
                            {
                                if(full) break;
                                bool found = false;
                                loopvrev(refs) if(conlines[refs[i]].reftime+(conlines[refs[i]].type < CON_IMPORTANT ? contime/2 : contime) < conlines[j].reftime+len)
                                {
                                    refs.remove(i);
                                    found = true;
                                    break;
                                }
                                if(!found) continue;
                            }
                            conlines[j].reftime = min(conlines[j].reftime, totalmillis-len);
                        }
                        refs.add(j);
                    }
                }
                loopvrev(refs)
                {
                    int len = !full && conlines[refs[i]].type < CON_IMPORTANT ? contime/2 : contime;
                    float f = full || !confade ? 1.f : clamp(((len+confade)-(totalmillis-conlines[refs[i]].reftime))/float(confade), 0.f, 1.f),
                        g = full || conlines[refs[i]].type >= CON_IMPORTANT ? fullconblend : conblend;
                    z += draw_textx("%s", concenter ? x+s/2 : x, y+z, 255, 255, 255, int(255*fade*f*g), concenter ? TEXT_CENTERED : TEXT_LEFT_JUSTIFY, -1, s, conlines[refs[i]].cref)*f;
                }
            }
            if(commandmillis > 0)
            {
                pushfont("command");
                Texture *t = textureload(commandicon ? commandicon : inputtex, 3);
                vec c(1, 1, 1);
                if(commandcolour) c = vec::hexcolor(commandcolour);
                float f = float(totalmillis%1000)/1000.f;
                if(f < 0.5f) f = 1.f-f;
                glBindTexture(GL_TEXTURE_2D, t->id);
                glColor4f(c.x, c.y, c.z, fullconblend*fade*f);
                float th = FONTH, tw = float(t->w)/float(t->h)*th;
                drawtexture(x, y+z, th, tw);
                z += draw_textx("%s", (concenter ? x+s/2-FONTW*3 : x)+(tw+FONTW), y+z, 255, 255, 255, int(255*fullconblend*fade), concenter ? TEXT_CENTERED : TEXT_LEFT_JUSTIFY, commandpos >= 0 ? commandpos : strlen(commandbuf), s-(FONTH+FONTW), commandbuf);
                popfont();
            }
        }
        popfont();
    }

    float radarrange()
    {
        if(game::focus->state == CS_EDITING) return editradardist ? editradardist : getworldsize();
        switch(radarstyle)
        {
            case 2: return radarcornerdist ? radarcornerdist : getworldsize();
            case 1: case 0: case -1: default: return radardist ? radardist : getworldsize();
        }
        return getworldsize();
    }

    void drawblip(const char *tex, float area, int w, int h, float s, float blend, int style, vec &pos, const vec &colour, const char *font, const char *text, ...)
    {
        vec dir;
        float dist = 1;
        if(style < 0)
        {
            style = -style-1;
            dir = pos;
        }
        else
        {
            dir = vec(pos).sub(camera1->o);
            dist = clamp(dir.magnitude()/hud::radarrange(), 0.f, 1.f);
        }
        dir.rotate_around_z(-camera1->yaw*RAD).normalize();
        float yaw = -atan2(dir.x, dir.y)/RAD, x = sinf(RAD*yaw), y = -cosf(RAD*yaw),
            size = max(w, h)/2, ts = size*(radarstyle == 2 ? radarcornersize : radarsize),
                tp = ts*s, tq = tp*0.5f, tx = 0, ty = 0, tr = 0;
        switch(style)
        {
            case 2:
            {
                ts = size*radarcorner;
                tx = w-ts;
                ty = ts;
                ts -= ts*radarcorneroffset;
                tr = ts*dist;
                break;
            }
            case 1:
            {
                tx = w/2;
                ty = h/2;
                tr = (size*radaroffset)+(ts*4*dist);
                break;
            }
            case 0: default:
            {
                tx = w/2;
                ty = h/2;
                tr = (size*radaroffset)+(ts*area);
                break;
            }
        }
        vec loc(tr*x, tr*y, 0);
        if(radarstyle != 2 && radaraspect != 0 && w != h)
        {
            if(w > h) loc.x *= (float(w)/float(h))*radaraspect;
            else loc.y *= (float(h)/float(w))*radaraspect;
        }
        loc.x += tx; loc.y += ty;
        glColor4f(colour.x, colour.y, colour.z, blend);
        Texture *t = textureload(tex, 3);
        if(t)
        {
            glBindTexture(GL_TEXTURE_2D, t->id);
            glBegin(GL_TRIANGLE_STRIP);
            loopk(4)
            {
                vec norm;
                switch(k)
                {
                    case 0: vecfromyawpitch(yaw, 0, 1, -1, norm);   glTexCoord2f(0, 1); break;
                    case 1: vecfromyawpitch(yaw, 0, 1, 1, norm);    glTexCoord2f(1, 1); break;
                    case 2: vecfromyawpitch(yaw, 0, -1, -1, norm);  glTexCoord2f(0, 0); break;
                    case 3: vecfromyawpitch(yaw, 0, -1, 1, norm);   glTexCoord2f(1, 0); break;
                }
                norm.normalize().mul(tq).add(loc);
                glVertex2f(norm.x, norm.y);
            }
            glEnd();
        }
        if(text && *text)
        {
            if(font && *font) pushfont(font);
            defvformatstring(str, text, text);
            loc.x -= x*tq; loc.y -= y*tq*0.75f;
            if(y > 0)
            {
                int width, height; text_bounds(str, width, height, -1, TEXT_CENTERED|TEXT_NO_INDENT);
                loc.y -= y*height;
            }
            draw_textx("%s", int(loc.x), int(loc.y), 255, 255, 255, int(blend*255.f), TEXT_CENTERED|TEXT_NO_INDENT, -1, -1, str);
            if(font && *font) popfont();
        }
    }

    void drawplayerblip(gameent *d, int w, int h, float blend)
    {
        vec dir = vec(d->o).sub(camera1->o);
        float dist = dir.magnitude();
        if(dist <= radarrange())
        {
            bool burning = radarplayereffects && burntime && lastmillis%150 < 50 && d->burning(lastmillis, burntime),
                 dominated = radarplayereffects && d->dominated.find(game::focus) >= 0;
            vec colour[2];
            if(burning) colour[0] = game::burncolour(d);
            else if(dominated) colour[0] = vec::hexcolor(pulsecols[2][clamp((lastmillis/100)%PULSECOLOURS, 0, PULSECOLOURS-1)]);
            else colour[0] = vec::hexcolor(game::getcolour(d, game::playerovertone));
            colour[1] = vec::hexcolor(game::getcolour(d, game::playerundertone));
            const char *tex = dominated ? dominatedtex : playerbliptex;
            float fade = clamp(1.f-(dist/radarrange()), dominated ? 0.25f : 0.f, 1.f)*blend, size = dominated ? 1.25f : 1.f;
            if(d->state == CS_DEAD || d->state == CS_WAITING)
            {
                int millis = d->lastdeath ? lastmillis-d->lastdeath : 0;
                if(millis > 0)
                {
                    int len = min(d->aitype >= AI_START && aistyle[d->aitype].canmove ? min(ai::aideadfade, enemyspawntime ? enemyspawntime : INT_MAX-1) : m_delay(game::gamemode, game::mutators), 2500);
                    if(len > 0) fade *= clamp(float(len-millis)/float(len), 0.f, 1.f);
                    else return;
                }
                else return;
                tex = deadtex;

            }
            else if(d->state == CS_ALIVE)
            {
                int len = m_protect(game::gamemode, game::mutators), millis = d->protect(lastmillis, len);
                if(millis > 0) fade *= clamp(float(len-millis)/float(len), 0.f, 1.f);
                fade *= clamp(vec(d->vel).add(d->falling).magnitude()/movespeed, 0.f, 1.f);
            }
            else if(d->state != CS_EDITING) return;
            loopi(2)
            {
                if(!i && chkcond(radarplayernames, game::tvmode()))
                    drawblip(i ? tex : hinttex, 1, w, h, size*(i ? radarplayersize : radarplayerhintsize), fade*(i ? radarplayerblend : radarplayerhintblend), radarstyle, d->o, colour[i], "tiny", "%s", game::colorname(d, NULL, "", false));
                else drawblip(i ? tex : hinttex, 1, w, h, size*(i ? radarplayersize : radarplayerhintsize), fade*(i ? radarplayerblend : radarplayerhintblend), radarstyle, d->o, colour[i]);
            }
        }
    }

    void drawentblip(int w, int h, float blend, int n, vec &o, int type, attrvector &attr, bool spawned, int lastspawn, bool insel)
    {
        if(type > NOTUSED && type < MAXENTTYPES && ((enttype[type].usetype == EU_ITEM && spawned) || game::focus->state == CS_EDITING))
        {
            float inspawn = radaritemtime && spawned && lastspawn && lastmillis-lastspawn <= radaritemtime ? float(lastmillis-lastspawn)/float(radaritemtime) : 0.f;
            if(enttype[type].noisy && (game::focus->state != CS_EDITING || !editradarnoisy || (editradarnoisy < 2 && !insel))) return;
            if(game::focus->state != CS_EDITING && radaritemspawn && (enttype[type].usetype != EU_ITEM || inspawn <= 0.f)) return;
            vec dir = vec(o).sub(camera1->o);
            float dist = dir.magnitude();
            if(dist >= radarrange())
            {
                if(insel || inspawn > 0.f) dir.mul(radarrange()/dist);
                else return;
            }
            const char *tex = bliptex;
            vec colour(1, 1, 1);
            float fade = insel ? 1.f : clamp(1.f-(dist/radarrange()), 0.1f, 1.f), size = radarblipsize;
            if(type == WEAPON)
            {
                int attr1 = w_attr(game::gamemode, attr[0], m_weapon(game::gamemode, game::mutators));
                tex = itemtex(WEAPON, attr1);
                colour = vec::hexcolor(WEAP(attr1, colour));
                fade *= radaritemblend;
                size = radaritemsize;
            }
            else fade *= radarblipblend;
            if(game::focus->state != CS_EDITING && !insel && inspawn > 0.f)
                fade = radaritemspawn ? 1.f-inspawn : fade+((1.f-fade)*(1.f-inspawn));
            if(insel) drawblip(tex, 0, w, h, size, fade*blend, radarstyle, o, colour, "tiny", "%s %s", enttype[type].name, entities::entinfo(type, attr, insel));
            else if(chkcond(radaritemnames, game::tvmode())) drawblip(tex, 0, w, h, size, fade*blend, radarstyle, o, colour, "tiny", "%s", entities::entinfo(type, attr, false));
            else drawblip(tex, 0, w, h, size, fade*blend, radarstyle, o, colour);
        }
    }

    void drawentblips(int w, int h, float blend)
    {
        if(m_edit(game::gamemode) && game::focus->state == CS_EDITING)
        {
            int hover = !entities::ents.inrange(enthover) && !entgroup.empty() ? entgroup[0] : -1;
            loopv(entgroup) if(entities::ents.inrange(entgroup[i]) && entgroup[i] != hover)
            {
                gameentity &e = *(gameentity *)entities::ents[entgroup[i]];
                drawentblip(w, h, blend, entgroup[i], e.o, e.type, e.attrs, e.spawned, e.lastspawn, true);
            }
            if(entities::ents.inrange(hover))
            {
                gameentity &e = *(gameentity *)entities::ents[hover];
                drawentblip(w, h, blend, hover, e.o, e.type, e.attrs, e.spawned, e.lastspawn, true);
            }
        }
        else
        {
            loopi(entities::lastusetype[EU_ITEM])
            {
                gameentity &e = *(gameentity *)entities::ents[i];
                drawentblip(w, h, blend, i, e.o, e.type, e.attrs, e.spawned, e.lastspawn, false);
            }
            loopv(projs::projs) if(projs::projs[i]->projtype == PRJ_ENT && projs::projs[i]->ready())
            {
                projent &proj = *projs::projs[i];
                if(entities::ents.inrange(proj.id))
                    drawentblip(w, h, blend, -1, proj.o, entities::ents[proj.id]->type, entities::ents[proj.id]->attrs, true, proj.spawntime, false);
            }
        }
    }

    void drawdamageblips(int w, int h, float blend)
    {
        loopv(damagelocs)
        {
            damageloc &d = damagelocs[i];
            int millis = lastmillis-d.outtime;
            if(millis >= radardamagetime+radardamagefade) { damagelocs.remove(i--); continue; }
            if(game::focus->state != CS_SPECTATOR && game::focus->state != CS_EDITING)
            {
                float amt = millis >= radardamagetime ? 1.f-(float(millis-radardamagetime)/float(radardamagefade)) : float(millis)/float(radardamagetime),
                    range = clamp(max(d.damage, radardamagemin)/float(max(radardamagemax-radardamagemin, 1)), radardamagemin/100.f, 1.f),
                    fade = clamp(radardamageblend*blend, min(radardamageblend*radardamagemin/100.f, 1.f), radardamageblend)*amt,
                    size = clamp(range*radardamagesize, min(radardamagesize*radardamagemin/100.f, 1.f), radardamagesize)*amt;
                vec dir = d.dir;
                if(dir.iszero()) vecfromyawpitch(camera1->yaw, camera1->pitch, 1, 0, dir);
                if(radardamage >= 5)
                {
                    gameent *a = game::getclient(d.attacker);
                    drawblip(hurttex, 2+size/3, w, h, size, fade, 0, dir, d.colour, "tiny", "%s +%d", a ? game::colorname(a) : "?", d.damage);
                }
                else drawblip(hurttex, 2+size/3, w, h, size, fade, 0, dir, d.colour);
            }
        }
        if(radardamage >= 2)
        {
            bool dead = (game::focus->state == CS_DEAD || game::focus->state == CS_WAITING) && game::focus->lastdeath;
            if(dead && lastmillis-game::focus->lastdeath <= m_delay(game::gamemode, game::mutators))
                drawblip(arrowtex, 3+radardamagetrack/2, w, h, radardamagetrack, blend*radardamageblend, radarstyle, game::focus->o, vec::hexcolor(game::getcolour(game::focus, game::playereffecttone)), "tiny", "you");
            gameent *a = game::getclient(game::focus->lastattacker);
            if(a && a != game::focus && (dead || (radardamage >= 3 && (a->aitype == AI_NONE || radardamage >= 4))))
            {
                vec colour = vec::hexcolor(game::getcolour(a, game::playereffecttone));
                if(dead && (a->state == CS_ALIVE || a->state == CS_DEAD || a->state == CS_WAITING))
                {
                    if(a->state == CS_ALIVE) drawblip(arrowtex, 3+radardamagetrack/2, w, h, radardamagetrack, blend*radardamageblend, radarstyle, a->o, colour, "tiny", "%s (%d)", game::colorname(a), a->health);
                    else drawblip(arrowtex, 3+radardamagetrack/2, w, h, radardamagetrack, blend*radardamageblend, radarstyle, a->o, colour, "tiny", "%s", game::colorname(a));
                }
            }
        }
    }

    void drawradar(int w, int h, float blend)
    {
        if(radarstyle == 2)
        {
            vec pos = vec(camera1->o).sub(minimapcenter).mul(minimapscale).add(0.5f), dir;
            vecfromyawpitch(camera1->yaw, 0, 1, 0, dir);
            float scale = radarrange(), size = max(w, h)/2, s = size*radarcorner, x = w-s*2, y = 0, q = s*2*radarcorneroffset, r = s-q;
            bindminimap();
            glColor4f(radarcornerbright, radarcornerbright, radarcornerbright, radarcornerblend);
            glBegin(GL_TRIANGLE_FAN);
            loopi(16)
            {
                vec tc = vec(dir).rotate_around_z(i/16.0f*2*M_PI);
                glTexCoord2f(pos.x + tc.x*scale*minimapscale.x, pos.y + tc.y*scale*minimapscale.y);
                vec v = vec(0, -1, 0).rotate_around_z(i/16.0f*2*M_PI);
                glVertex2f(x + q + r*(1.0f + v.x), y + q + r*(1.0f + v.y));
            }
            glEnd();
            float gr = 1, gg = 1, gb = 1;
            if(radartone) skewcolour(gr, gg, gb, radartone);
            settexture(radarcornertex, 3);
            glColor4f(gr*radartexbright, gg*radartexbright, gb*radartexbright, radartexblend);
            drawsized(w-s*2, 0, s*2);
        }
        if(chkcond(radaritems, game::tvmode()) || m_edit(game::gamemode)) drawentblips(w, h, blend*radarblend); // 2
        if(chkcond(radaraffinity, game::tvmode())) // 3
        {
            if(m_capture(game::gamemode)) capture::drawblips(w, h, blend*radarblend);
            else if(m_defend(game::gamemode)) defend::drawblips(w, h, blend);
            else if(m_bomber(game::gamemode)) bomber::drawblips(w, h, blend*radarblend);
        }
        if(chkcond(radarplayers, radarplayerfilter != 3 || m_duke(game::gamemode, game::mutators) || m_edit(game::gamemode) || game::tvmode())) // 4
        {
            gameent *d = NULL;
            int numdyns = game::numdynents();
            loopi(numdyns) if((d = (gameent *)game::iterdynents(i)) && d != game::focus && d->state != CS_SPECTATOR && d->aitype < AI_START)
            {
                switch(radarplayerfilter)
                {
                    case 0: case 3: default: break;
                    case 1: if(m_team(game::gamemode, game::mutators) && d->team == game::focus->team) continue; break;
                    case 2: if(m_team(game::gamemode, game::mutators) && d->team != game::focus->team) continue; break;
                }
                drawplayerblip(d, w, h, blend*radarblend);
            }
        }
        if(radardamage) drawdamageblips(w, h, blend*radarblend); // 5+
    }

    int drawprogress(int x, int y, float start, float length, float size, bool left, float r, float g, float b, float fade, float skew, const char *font, const char *text, ...)
    {
        if(skew <= 0.f) return 0;
        float q = clamp(skew, 0.f, 1.f), cr = r*q, cg = g*q, cb = b*q, s = size*skew, cs = s/2, cx = left ? x+cs : x-cs, cy = y-cs;
        settexture(progresstex, 3);
        glColor4f(cr, cg, cb, fade*0.25f);
        drawslice(0, 1, cx, cy, cs*2/3);
        glColor4f(cr, cg, cb, fade);
        drawslice((SDL_GetTicks()%1000)/1000.f, 0.1f, cx, cy, cs);
        drawslice(start, length, cx, cy, cs*2/3);
        if(text && *text)
        {
            glPushMatrix();
            glScalef(skew, skew, 1);
            if(font && *font) pushfont(font);
            int tx = int(cx*(1.f/skew)), ty = int((cy-FONTH/2)*(1.f/skew)), ti = int(255.f*fade);
            defvformatstring(str, text, text);
            draw_textx("%s", tx, ty, 255, 255, 255, ti, TEXT_CENTERED, -1, -1, str);
            if(font && *font) popfont();
            glPopMatrix();
        }
        return int(s);
    }

    int drawitem(const char *tex, int x, int y, float size, bool bg, bool left, float r, float g, float b, float fade, float skew, const char *font, const char *text, ...)
    {
        if(skew <= 0.f) return 0;
        Texture *t = textureload(tex, 3);
        float q = clamp(skew, 0.f, 1.f), cr = left ? r : r*q, cg = left ? g : g*q, cb = left ? b : b*q, s = size*skew, w = float(t->w)/float(t->h)*s;
        int heal = m_health(game::gamemode, game::mutators), sy = int(s);
        bool pulse = inventoryflash && game::focus->state == CS_ALIVE && game::focus->health < heal;
        if(bg && inventorybg)
        {
            int glow = int(s*inventorybgsize);
            sy += glow;
            float gr = 1, gb = 1, gg = 1, gf = game::focus->state == CS_ALIVE && game::focus->lastspawn && lastmillis-game::focus->lastspawn <= 1000 ? (lastmillis-game::focus->lastspawn)/2000.f : inventoryglowblend;
            if(inventorytone) skewcolour(gr, gg, gb, inventorytone);
            if(pulse)
            {
                int timestep = totalmillis%1000;
                float amt = clamp((timestep <= 500 ? timestep/500.f : (1000-timestep)/500.f)*(float(heal-game::focus->health)/float(heal)), 0.f, 1.f);
                flashcolourf(gr, gg, gb, gf, 1.f, 0.f, 0.f, 1.f, amt);
                glow += int(s*inventoryglow*amt);
            }
            settexture(inventorytex, 3);
            glColor4f(gr, gg, gb, fade*gf);
            drawtexture(left ? x-glow : x-w-glow, y-s-glow, s+glow*2, w+glow*2, left);
        }
        glColor4f(cr, cg, cb, fade);
        glBindTexture(GL_TEXTURE_2D, t->id);
        drawtexture(left ? x : x-w, y-s, s, w);
        if(text && *text)
        {
            glPushMatrix();
            glScalef(skew, skew, 1);
            if(font && *font) pushfont(font);
            int tx = int((left ? (x+w) : x)*(1.f/skew)), ty = int((y-s+s/32)*(1.f/skew));
            defvformatstring(str, text, text);
            draw_textx("%s", tx, ty, 255, 255, 255, int(255*fade), TEXT_RIGHT_JUSTIFY, -1, -1, str);
            if(font && *font) popfont();
            glPopMatrix();
        }
        return sy;
    }

    int drawitemsubtext(int x, int y, float size, int align, float skew, const char *font, float blend, const char *text, ...)
    {
        if(skew <= 0.f) return 0;
        glPushMatrix();
        glScalef(skew, skew, 1);
        if(font && *font) pushfont(font);
        int sy = int(FONTH*skew), tx = int(x*(1.f/skew)), ty = int((y-size/32)*(1.f/skew)), ti = int(255.f*blend);
        defvformatstring(str, text, text);
        draw_textx("%s", tx, ty, 255, 255, 255, ti, align, -1, -1, str);
        if(font && *font) popfont();
        glPopMatrix();
        return sy;
    }

    const char *teamtex(int team)
    {
        const char *teamtexs[TEAM_MAX] = { neutraltex, alphatex, omegatex, kappatex, sigmatex, neutraltex };
        return teamtexs[team];
    }

    const char *itemtex(int type, int stype)
    {
        switch(type)
        {
            case WEAPON:
            {
                const char *weaptexs[WEAP_MAX] = {
                    meleetex, pistoltex, swordtex, shotguntex, smgtex, flamertex, plasmatex, rifletex, grenadetex, rockettex
                };
                if(isweap(stype)) return weaptexs[stype];
                break;
            }
            default: break;
        }
        return "";
    }

    const char *icontex(int type, int value)
    {
        switch(type)
        {
            case eventicon::SPREE:
            {
                switch(value)
                {
                    case 0: return spree1tex; break;
                    case 1: return spree2tex; break;
                    case 2: return spree3tex; break;
                    case 3: default: return spree4tex; break;
                }
                break;
            }
            case eventicon::MULTIKILL:
            {
                switch(value)
                {
                    case 0: return multi1tex; break;
                    case 1: return multi2tex; break;
                    case 2: default: return multi3tex; break;
                }
                break;
            }
            case eventicon::HEADSHOT: return headshottex; break;
            case eventicon::CRITICAL: return criticaltex; break;
            case eventicon::DOMINATE: return dominatetex; break;
            case eventicon::REVENGE: return revengetex; break;
            case eventicon::FIRSTBLOOD: return firstbloodtex; break;
            case eventicon::WEAPON: return itemtex(WEAPON, value);
            case eventicon::AFFINITY:
            {
                if(m_bomber(game::gamemode)) return bombtex;
                return flagtex;
            }
        }
        return "";
    }

    int drawentitem(int n, int x, int y, int s, float skew, float fade)
    {
        if(entities::ents.inrange(n))
        {
            gameentity &e = *(gameentity *)entities::ents[n];
            const char *itext = itemtex(e.type, e.attrs[0]);
            int ty = drawitem(itext && *itext ? itext : "textures/blank", x, y, s, true, false, 1.f, 1.f, 1.f, fade*inventoryblend, skew, "default", "%s (%d)", enttype[e.type].name, n);
            mkstring(attrstr);
            loopi(enttype[e.type].numattrs)
            {
                defformatstring(s)("%s%d", i ? " " : "", e.attrs[i]);
                concatstring(attrstr, s);
            }
            if(attrstr[0]) drawitemsubtext(x, y-int(s/3*skew), s, TEXT_RIGHT_UP, skew, "default", fade*inventoryblend, "%s", attrstr);
            drawitemsubtext(x, y, s, TEXT_RIGHT_UP, skew, "default", fade*inventoryblend, "%s", entities::entinfo(e.type, e.attrs, true));
            return ty;
        }
        return 0;
    }

    int drawselection(int x, int y, int s, int m, float blend)
    {
        int sy = 0;
        if(game::focus->state == CS_ALIVE)
        {
            if(inventoryammo)
            {
                const char *hudtexs[WEAP_MAX] = {
                    meleetex, pistoltex, swordtex, shotguntex, smgtex, flamertex, plasmatex, rifletex, grenadetex, rockettex
                };
                int sweap = m_weapon(game::gamemode, game::mutators);
                loopi(WEAP_MAX) if((i != WEAP_MELEE || sweap == WEAP_MELEE || game::focus->weapselect == WEAP_MELEE || !inventoryhidemelee) && (game::focus->hasweap(i, sweap) || i == game::focus->weapselect || lastmillis-game::focus->weaplast[i] < game::focus->weapwait[i]))
                {
                    if(y-sy-s < m) break;
                    float fade = blend*inventoryblend, size = s, skew = 0.f;
                    if((game::focus->weapstate[i] == WEAP_S_SWITCH || game::focus->weapstate[i] == WEAP_S_USE) && (i != game::focus->weapselect || i != game::focus->lastweap))
                    {
                        float amt = clamp(float(lastmillis-game::focus->weaplast[i])/float(game::focus->weapwait[i]), 0.f, 1.f);
                        if(i != game::focus->weapselect) skew = game::focus->hasweap(i, sweap) ? 1.f-(amt*(1.f-inventoryskew)) : 1.f-amt;
                        else skew = game::focus->weapstate[i] == WEAP_S_USE ? amt : inventoryskew+(amt*(1.f-inventoryskew));
                    }
                    else if(game::focus->hasweap(i, sweap) || i == game::focus->weapselect) skew = i != game::focus->weapselect ? inventoryskew : 1.f;
                    else continue;
                    bool instate = (i == game::focus->weapselect || game::focus->weapstate[i] != WEAP_S_USE);
                    vec c(1, 1, 1);
                    if(inventorycolour) c.mul(vec::hexcolor(WEAP(i, colour)));
                    else if(inventorytone) skewcolour(c.r, c.g, c.b, inventorytone);
                    int oldy = y-sy;
                    if(inventoryammo && (instate || inventoryammo > 1) && WEAP(i, max) > 1 && game::focus->hasweap(i, sweap))
                        sy += drawitem(hudtexs[i], x, y-sy, size, true, false, c.r, c.g, c.b, fade, skew, "super", "%d", game::focus->ammo[i]);
                    else sy += drawitem(hudtexs[i], x, y-sy, size, true, false, c.r, c.g, c.b, fade, skew);
                    if(inventoryweapids && (instate || inventoryweapids > 1))
                    {
                        static string weapids[WEAP_MAX];
                        static int lastweapids = -1;
                        int n = weapons::slot(game::focus, i);
                        if(isweap(n))
                        {
                            if(lastweapids != changedkeys)
                            {
                                loopj(WEAP_MAX)
                                {
                                    defformatstring(action)("weapon %d", j);
                                    const char *actkey = searchbind(action, 0);
                                    if(actkey && *actkey) copystring(weapids[j], actkey);
                                    else formatstring(weapids[j])("%d", j);
                                }
                                lastweapids = changedkeys;
                            }
                            drawitemsubtext(x, oldy, size, TEXT_RIGHT_UP, skew, "reduced", fade, "\f[%d]%s", inventorycolour >= 2 ? WEAP(i, colour) : 0xAAAAAA, weapids[n]);
                        }
                    }
                }
            }
        }
        else if(game::focus->state == CS_EDITING && inventoryedit)
        {
            int hover = entities::ents.inrange(enthover) ? enthover : (!entgroup.empty() ? entgroup[0] : -1);
            sy += FONTH*4;
            if(y-sy-s >= m) sy += drawentitem(hover, x, y-sy, s, 1.f, blend*inventoryeditblend);
            loopv(entgroup) if(entgroup[i] != hover)
            {
                if(y-sy-s < m) break;
                sy += drawentitem(entgroup[i], x, y-sy, s, inventoryeditskew, blend*inventoryeditblend);
            }
        }
        return sy;
    }

    int drawbar(int x, int y, int w, int h, int type, float top, float bottom, float fade, float amt, const char *tex, const char *bgtex, int tone, float bgglow, float blend, float pulse, float throb)
    {
        int sy = 0, offset = int(w*throb);
        if(*bgtex)
        {
            int glow = 0;
            float gr = 1, gg = 1, gb = 1, gf = blend;
            if(tone) skewcolour(gr, gg, gb, tone);
            if(pulse > 0)
            {
                int timestep = totalmillis%1000;
                float skew = clamp((timestep <= 500 ? timestep/500.f : (1000-timestep)/500.f)*pulse, 0.f, 1.f);
                flashcolourf(gr, gg, gb, gf, type ? 0.5f : 1.f, 0.f, type ? 0.5f : 0.f, 1.f, skew);
                glow += int(w*bgglow*skew);
            }
            settexture(bgtex, 3);
            glColor4f(gr, gg, gb, fade*gf);
            drawtexture(x-offset-glow, y-h-offset-glow, w+glow*2+offset*2, h+glow*2+offset*2);
        }
        const struct barstep
        {
            float amt, r, g, b;
        } steps[2][4] = {
            { { 0, 0.75f, 0, 0 }, { 0.35f, 1, 0.5f, 0 }, { 0.65f, 1, 1, 0 }, { 1, 0, 1, 0 } },
            { { 0, 1, 0.25f, 0.25f }, { 0.35f, 1, 0, 1 }, { 0.65f, 0.25f, 0.25f, 1 }, { 1, 0, 1, 1 } }
        };
        settexture(tex, 3);
        glBegin(GL_TRIANGLE_STRIP);
        float btoff = 1-bottom, middle = btoff-top;
        int cx = x-offset, cy = y-h+int(h*top)-offset, cw = w+offset*2, ch = int(h*middle)+offset*2;
        const float margin = 0.1f;
        loopi(4)
        {
            const barstep &step = steps[type][i];
            if(i > 0)
            {
                if(step.amt > amt && steps[type][i-1].amt <= amt)
                {
                    float hoff = 1 - amt, hlerp = (amt - steps[type][i-1].amt) / (step.amt - steps[type][i-1].amt),
                          r = step.r*hlerp + steps[type][i-1].r*(1-hlerp),
                          g = step.g*hlerp + steps[type][i-1].g*(1-hlerp),
                          b = step.b*hlerp + steps[type][i-1].b*(1-hlerp);
                    glColor4f(r, g, b, fade); glTexCoord2f(0, hoff*middle+top); glVertex2f(cx, cy + hoff*ch);
                    glColor4f(r, g, b, fade); glTexCoord2f(1, hoff*middle+top); glVertex2f(cx + cw, cy + hoff*ch);
                }
                if(step.amt > amt + margin)
                {
                    float hoff = 1 - (amt + margin), hlerp = (amt + margin - steps[type][i-1].amt) / (step.amt - steps[type][i-1].amt),
                          r = step.r*hlerp + steps[type][i-1].r*(1-hlerp),
                          g = step.g*hlerp + steps[type][i-1].g*(1-hlerp),
                          b = step.b*hlerp + steps[type][i-1].b*(1-hlerp);
                    glColor4f(r, g, b, 0); glTexCoord2f(0, hoff*middle+top); glVertex2f(cx, cy + hoff*ch);
                    glColor4f(r, g, b, 0); glTexCoord2f(1, hoff*middle+top); glVertex2f(cx + cw, cy + hoff*ch);
                    break;
                }
            }
            float off = 1 - step.amt, hfade = fade, r = step.r, g = step.g, b = step.b;
            if(step.amt > amt) hfade *= 1 - (step.amt - amt)/margin;
            glColor4f(r, g, b, hfade); glTexCoord2f(0, off*middle+top); glVertex2f(cx, cy + off*ch);
            glColor4f(r, g, b, hfade); glTexCoord2f(1, off*middle+top); glVertex2f(cx + cw, cy + off*ch);
        }
        glEnd();
        sy += h;
        return sy;
    }

    int drawhealth(int x, int y, int s, float blend)
    {
        float fade = blend*inventoryblend;
        int size = s+s/2, width = s-s/4, sy = 0;
        if(game::focus->state == CS_ALIVE)
        {
            fade *= game::focus->lastspawn && lastmillis-game::focus->lastspawn <= 1000 ? (lastmillis-game::focus->lastspawn)/2000.f : 1.f;
            if(inventoryhealth && (!m_trial(game::gamemode) || trialdamage))
            {
                int heal = m_health(game::gamemode, game::mutators);
                float pulse = healthflash && game::focus->health < heal ? float(heal-game::focus->health)/float(heal) : 0.f,
                    throb = healththrob > 0 && regentime && game::focus->lastregen && lastmillis-game::focus->lastregen <= regentime ? clamp((lastmillis-game::focus->lastregen)/float(regentime/2), 0.f, 2.f) : 0.f;
                if(inventoryhealth&2)
                    sy += drawbar(x, y, width, size, 0, healthbartop, healthbarbottom, fade, clamp(game::focus->health/float(heal), 0.0f, 1.0f), healthtex, healthbgtex, healthtone, healthbgglow, healthbgblend, pulse, (throb > 1.f ? 1.f-throb : throb)*healththrob);
                float gr = 1, gg = 1, gb = 1;
                if(pulse > 0)
                {
                    int timestep = totalmillis%1000;
                    float amt = clamp((timestep <= 500 ? timestep/500.f : (1000-timestep)/500.f), 0.f, 1.f)*pulse;
                    flashcolour(gr, gg, gb, 1.f, 0.f, 0.f, amt);
                }
                pushfont("super");
                int ty = 0;
                if(inventoryhealth&1)
                    ty = draw_textx("%d", x+width/2, y-sy+(inventoryhealth == 1 ? FONTH : size/2-FONTH), int(gr*255), int(gg*255), int(gb*255), int(fade*255), TEXT_CENTERED, -1, -1, max(game::focus->health, 0));
                if(inventoryhealth == 1) sy += ty;
                popfont();
            }
            if(inventoryvelocity >= (m_trial(game::gamemode) ? 1 : 2))
            {
                pushfont("emphasis");
                sy += draw_textx("%d", x+width/2, y-sy, 255, 255, 255, int(fade*255), TEXT_CENTER_UP, -1, -1, int(vec(game::focus->vel).add(game::focus->falling).magnitude()));
                popfont();
                pushfont("reduced");
                sy += draw_textx("speed", x+width/2, y-sy, 255, 255, 255, int(fade*255), TEXT_CENTER_UP, -1, -1);
                popfont();
            }
            if(game::focus->aitype < AI_START && physics::allowimpulse(game::focus) && impulsemeter && impulsecost && inventoryimpulse)
            {
                float amt = 1-clamp(float(game::focus->impulse[IM_METER])/float(impulsemeter), 0.f, 1.f);
                if(inventoryimpulse == 2)
                    sy += drawbar(x, y-sy, width, size, 1, impulsebartop, impulsebarbottom, fade, amt, impulsetex, impulsebgtex, impulsetone, impulsebgglow, impulsebgblend, impulseflash && game::focus->impulse[IM_METER] ? 1-amt : 0.f, 0.f);
                else
                {
                    pushfont("emphasis");
                    sy += draw_textx("%s%d%%", x+width/2, y-sy, 255, 255, 255, int(fade*255), TEXT_CENTER_UP, -1, -1,
                        game::focus->impulse[IM_METER] > 0 ? (impulsemeter-game::focus->impulse[IM_METER] > impulsecost ? "\fc" : "\fy") : "\fg",
                            int(amt*100));
                    popfont();
                    pushfont("reduced");
                    sy += draw_textx("impulse", x+width/2, y-sy, 255, 255, 255, int(fade*255), TEXT_CENTER_UP, -1, -1);
                    popfont();
                }
            }
            if(inventoryresidual)
            {
                if(bleedtime && game::focus->bleeding(lastmillis, bleedtime))
                {
                    float gr = 1, gg = 1, gb = 1;
                    if(inventorytone) skewcolour(gr, gg, gb, inventorytone);
                    if(residualflash)
                    {
                        int millis = lastmillis-game::focus->lastbleedtime, delay = bleeddelay;
                        if(millis <= delay)
                        {
                            delay /= 2;
                            float amt = millis <= delay ? millis/float(delay) : 1.f-((millis-delay)/float(delay));
                            flashcolour(gr, gg, gb, 1.f, 0.f, 0.f, amt);
                        }
                    }
                    sy += drawitem(bleedingtex, x, y-sy, width, false, true, gr, gg, gb, fade*inventoryblend);
                }
                if(burntime && game::focus->burning(lastmillis, burntime))
                {
                    float gr = 1, gg = 1, gb = 1;
                    if(inventorytone) skewcolour(gr, gg, gb, inventorytone);
                    if(residualflash)
                    {
                        int millis = lastmillis-game::focus->lastburntime, delay = burndelay;
                        if(millis <= delay)
                        {
                            delay /= 2;
                            float amt = millis <= delay ? millis/float(delay) : 1.f-((millis-delay)/float(delay));
                            flashcolour(gr, gg, gb, 1.f, 0.5f, 0.f, amt);
                        }
                    }
                    sy += drawitem(burningtex, x, y-sy, width, false, true, gr, gg, gb, fade*inventoryblend);
                }
            }
        }
        else
        {
            const char *state = "", *tex = "";
            switch(game::player1->state)
            {
                case CS_EDITING: state = "EDIT"; break;
                case CS_SPECTATOR: state = "SPEC"; tex = spectex; break;
                case CS_WAITING: state = "WAIT"; tex = waittex; break;
                case CS_DEAD: state = "DEAD"; tex = deadtex; break;
            }
            if(inventorystatus&1 && *state)
            {
                sy -= x/2;
                pushfont("emphasis");
                sy += draw_textx("%s", x+width/2, y-sy, 255, 255, 255, int(fade*255), TEXT_CENTER_UP, -1, -1, state);
                popfont();
            }
            if(inventorystatus&2 && *tex) sy += drawitem(tex, x, y-sy, width, false, true, 1.f, 1.f, 1.f, fade, 1.f);
        }
        return sy;
    }

    int drawtimer(int x, int y, int s, float blend)
    {
        int sy = 0;
        if(inventorytrial && m_trial(game::gamemode) && game::focus->state != CS_EDITING && game::focus->state != CS_SPECTATOR)
        {
            float fade = blend*inventoryblend;
            if((game::focus->cpmillis > 0 || game::focus->cptime) && (game::focus->state == CS_ALIVE || game::focus->state == CS_DEAD || game::focus->state == CS_WAITING))
            {
                pushfont("default");
                if(game::focus->cpmillis > 0)
                    sy += draw_textx("%s", x, y-sy, 255, 255, 255, int(fade*255), TEXT_LEFT_UP, -1, -1, timetostr(lastmillis-game::focus->cpmillis, 1));
                else if(game::focus->cplast)
                    sy += draw_textx("\fzwe%s", x, y-sy, 255, 255, 255, int(fade*255), TEXT_LEFT_UP, -1, -1, timetostr(game::focus->cplast));
                popfont();
                if(game::focus->cptime)
                {
                    pushfont("reduced");
                    sy += draw_textx("\fy%s", x, y-sy, 255, 255, 255, int(fade*255), TEXT_LEFT_UP, -1, -1, timetostr(game::focus->cptime));
                    popfont();
                }
            }
            sy += trialinventory(x, y-sy, s, fade);
        }
        return sy;
    }

    void drawinventory(int w, int h, int edge, float blend)
    {
        pushfont("console");
        int cx[2] = { edge, w-edge }, cy[2] = { h-edge, h-edge }, cs = int(inventorysize*w), cr = cs/8, cc = 0;
        popfont();
        loopi(2) switch(i)
        {
            case 0: default:
            {
                if((cc = drawhealth(cx[i], cy[i], cs, blend)) > 0) cy[i] -= cc+cr;
                if((cc = drawtimer(cx[i], cy[i], cs, blend)) > 0) cy[i] -= cc+cr;
                break;
            }
            case 1:
            {
                int cm = edge;
                if(radarstyle == 2) cm += int(max(w, h)/2*radarcorner*2);
                if(!texpaneltimer)
                {
                    cy[i] -= showfps || showstats > (m_edit(game::gamemode) ? 0 : 1) ? cs/2 : cs/16;
                    if(lastnewgame)
                    {
                        if(!game::intermission) lastnewgame = 0;
                        else
                        {
                            int millis = votelimit-(totalmillis-lastnewgame);
                            float amt = float(millis)/float(votelimit);
                            const char *col = "\fw";
                            if(amt > 0.75f) col = "\fg";
                            else if(amt > 0.5f) col = "\fc";
                            else if(amt > 0.25f) col = "\fy";
                            else col = "\fo";
                            drawprogress(cx[i], cm+cs, 0, 1, cs, false, 1, 1, 1, blend*inventoryblend*0.25f, 1);
                            cm += drawprogress(cx[i], cm+cs, 1-amt, amt, cs, false, 1, 1, 1, blend*inventoryblend, 1, "default", "%s%d", col, int(millis/1000.f));
                        }
                    }
                    else if(!m_edit(game::gamemode) && inventoryscore && ((cc = drawscore(cx[i], cm, cs, (h-edge*2)/2, blend)) > 0)) cm += cc+cr;

                    if((cc = drawselection(cx[i], cy[i], cs, cm, blend)) > 0) cy[i] -= cc+cr;
                    if(inventorygame)
                    {
                        if(m_capture(game::gamemode) && ((cc = capture::drawinventory(cx[i], cy[i], cs, cm, blend)) > 0)) cy[i] -= cc+cr;
                        else if(m_defend(game::gamemode) && ((cc = defend::drawinventory(cx[i], cy[i], cs, cm, blend)) > 0)) cy[i] -= cc+cr;
                        else if(m_bomber(game::gamemode) && ((cc = bomber::drawinventory(cx[i], cy[i], cs, cm, blend)) > 0)) cy[i] -= cc+cr;
                    }
                }
                break;
            }
        }
    }

    void drawdamage(int w, int h, int s, float blend)
    {
        float pc = game::focus->state == CS_DEAD ? 0.5f : (game::focus->state == CS_ALIVE ? min(damageresidue, 100)/100.f : 0.f);
        if(pc > 0)
        {
            Texture *t = *damagetex ? textureload(damagetex, 3) : notexture;
            if(t != notexture)
            {
                glBindTexture(GL_TEXTURE_2D, t->id);
                glColor4f(0.85f, 0.09f, 0.09f, pc*blend*damageblend);
                drawtexture(0, 0, w, h);
            }
        }
    }

    void drawfire(int w, int h, int s, float blend)
    {
        if(game::focus->burning(lastmillis, burntime))
        {
            int interval = lastmillis-game::focus->lastburn;
            Texture *t = *burntex ? textureload(burntex, 3) : notexture;
            if(t != notexture)
            {
                float pc = interval >= burntime-500 ? 1.f+(interval-(burntime-500))/500.f : (interval%burndelay)/float(burndelay/2); if(pc > 1.f) pc = 2.f-pc;
                glBindTexture(GL_TEXTURE_2D, t->id);
                glColor4f(0.9f*max(pc,0.5f), 0.3f*pc, 0.0625f*max(pc,0.25f), blend*burnblend*(interval >= burntime-(burndelay/2) ? pc : min(pc+0.5f, 1.f)));
                drawtexture(0, 0, w, h);
            }
        }
    }

    void drawzoom(int w, int h)
    {
        Texture *t = textureload(zoomtex, 3);
        int frame = lastmillis-game::lastzoom;
        float pc = frame <= zoomtime ? float(frame)/float(zoomtime) : 1.f;
        if(!game::zooming) pc = 1.f-pc;
        int x = 0, y = 0, c = 0;
        if(w > h)
        {
            float rc = 1.f-pc;
            c = h; x += (w-h)/2;
            usetexturing(false);
            drawblend(0, 0, x, c, rc, rc, rc, true);
            drawblend(x+c, 0, x+1, c, rc, rc, rc, true);
            usetexturing(true);
        }
        else if(h > w)
        {
            float rc = 1.f-pc;
            c = w; y += (h-w)/2;
            usetexturing(false);
            drawblend(0, 0, c, y, rc, rc, rc, true);
            drawblend(0, y+c, c, y, rc, rc, rc, true);
            usetexturing(true);
        }
        else c = h;
        glBindTexture(GL_TEXTURE_2D, t->id);
        glColor4f(1.f, 1.f, 1.f, pc);
        drawtexture(x, y, c, c);
    }

    void drawbackground(int w, int h)
    {
        Texture *t = textureload(bgtex, 3);
        glBindTexture(GL_TEXTURE_2D, t->id);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(1, 0); glVertex2f(w, 0);
        glTexCoord2f(0, 1); glVertex2f(0, h);
        glTexCoord2f(1, 1); glVertex2f(w, h);
        glEnd();

        t = textureload(logotex, 3);
        glBindTexture(GL_TEXTURE_2D, t->id);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, 0); glVertex2f(w-1024, 0);
        glTexCoord2f(1, 0); glVertex2f(w, 0);
        glTexCoord2f(0, 1); glVertex2f(w-1024, 256);
        glTexCoord2f(1, 1); glVertex2f(w, 256);
        glEnd();

        t = textureload(badgetex, 3);
        glBindTexture(GL_TEXTURE_2D, t->id);
        glBegin(GL_TRIANGLE_STRIP); // goes off the edge on purpose
        glTexCoord2f(0, 0); glVertex2f(w-336, 0);
        glTexCoord2f(1, 0); glVertex2f(w-80, 0);
        glTexCoord2f(0, 1); glVertex2f(w-336, 128);
        glTexCoord2f(1, 1); glVertex2f(w-80, 128);
        glEnd();
        if(!forceprogress)
        {
            pushfont("console");
            int y = h-FONTH/2;
            if(progressing)
            {
                if(progressamt > 0) drawprogress(FONTH, y, 0, progressamt, FONTH*2, true, 1, 1, 1, 1, 1, "consub", "\fy%d%%", int(progressamt*100));
                else drawprogress(FONTH, y, 0, progressamt, FONTH*2, true, 1, 1, 1, 1, 1, "consub", "\fg...");
                y -= FONTH/2;
                if(*progresstext) y -= draw_textx("%s %s [\fs\fa%d%%\fS]", FONTH*7/2, y, 255, 255, 255, 255, TEXT_LEFT_UP, -1, -1, *progresstitle ? progresstitle : "please wait...", progresstext, int(progresspart*100));
                else y -= draw_textx("%s", FONTH*7/2, y, 255, 255, 255, 255, TEXT_LEFT_UP, -1, -1, *progresstitle ? progresstitle : "please wait...");
            }
            y = h-FONTH/2;
            y -= draw_textx("v%s-%s (%s)", w-FONTH, y, 255, 255, 255, 255, TEXT_RIGHT_UP, -1, -1, RE_VER_STR, RE_PLATFORM, RE_RELEASE);
            y -= draw_textx("%s", w-FONTH, y, 255, 255, 255, 255, TEXT_RIGHT_UP, -1, -1, RE_URL);
            popfont();
        }
    }

    void drawheadsup(int w, int h, float fade, int os, int is, int br, int bs, int bx, int by)
    {
        if(underlaydisplay >= 2 || (game::focus->state == CS_ALIVE && (underlaydisplay || !game::thirdpersonview(true))))
        {
            Texture *t = *underlaytex ? textureload(underlaytex, 3) : notexture;
            if(t != notexture)
            {
                glBindTexture(GL_TEXTURE_2D, t->id);
                glColor4f(1.f, 1.f, 1.f, underlayblend*hudblend);
                drawtexture(0, 0, w, h);
            }
        }
        bool third = game::thirdpersonview(true) && game::focus != game::player1;
        if(game::focus->state == CS_ALIVE && game::inzoom() && WEAP(game::focus->weapselect, zooms)) drawzoom(w, h);
        if(showdamage && !third)
        {
            if(burntime && game::focus->state == CS_ALIVE) drawfire(w, h, os, fade);
            if(!kidmode && game::bloodscale > 0) drawdamage(w, h, os, fade);
        }
        if(!hasinput() && (game::focus->state == CS_EDITING ? showeditradar > 0 : !third && chkcond(showradar, game::tvmode())))
            drawradar(w, h, fade);
        if(showinventory) drawinventory(w, h, os, fade);
        if(teamhurttime && game::focus == game::player1 && lastmillis-game::player1->lastteamhit <=  teamhurttime)
        {
            vec targ;
            bool hasbound = false;
            int dist = teamhurtdist ? teamhurtdist : getworldsize();
            loopv(game::players) if(game::players[i] && game::players[i]->team == game::player1->team)
            {
                if(lastmillis-game::players[i]->lastteamhit > teamhurttime) continue;
                if(!getsight(camera1->o, camera1->yaw, camera1->pitch, game::players[i]->o, targ, dist, curfov, fovy)) continue;
                if(!hasbound)
                {
                    Texture *t = textureload(hud::warningtex, 3);
                    glBindTexture(GL_TEXTURE_2D, t->id);
                    float amt = float(lastmillis%250)/250.f, value = (amt > 0.5f ? 1.f-amt : amt)*2.f;
                    glColor4f(value, value*0.125f, value*0.125f, value);
                    hasbound = true;
                }
                float cx = 0.5f, cy = 0.5f, cz = 1;
                vectocursor(game::players[i]->o, cx, cy, cz);
                int s = int(teamhurtsize*w), sx = int(cx*w-s), sy = int(cy*h-s);
                hud::drawsized(sx, sy, s*2);
            }
        }

        if(!texpaneltimer)
        {
            int bf = int(255*fade*statblend);
            bx -= os;
            if(totalmillis-laststats >= statrate)
            {
                memcpy(prevstats, curstats, sizeof(prevstats));
                laststats = totalmillis-(totalmillis%statrate);
            }
            int nextstats[NUMSTATS] = {
                vtris*100/max(wtris, 1), vverts*100/max(wverts, 1), xtraverts/1024, xtravertsva/1024, glde, gbatches, getnumqueries(), rplanes, curfps, bestfpsdiff, worstfpsdiff
            };
            loopi(NUMSTATS) if(prevstats[i] == curstats[i]) curstats[i] = nextstats[i];
            pushfont("consub");
            if(showfps)
            {
                pushfont("console");
                draw_textx("%d", w-br/2, by-FONTH*2, 255, 255, 255, bf, TEXT_CENTERED, -1, bs, curstats[8]);
                draw_textx("fps", w-br/2, by-FONTH, 255, 255, 255, bf, TEXT_CENTERED, -1, -1);
                popfont();
                switch(showfps)
                {
                    case 3:
                        by -= draw_textx("max:%d range:+%d-%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs, maxfps, curstats[9], curstats[10]);
                        break;
                    case 2:
                        by -= draw_textx("max:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs, maxfps);
                        break;
                    default: break;
                }
            }
            if(showstats > (m_edit(game::gamemode) ? 0 : 1))
            {
                by -= draw_textx("ond:%d va:%d gl:%d(%d) oq:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs, allocnodes*8, allocva, curstats[4], curstats[5], curstats[6]);
                by -= draw_textx("wtr:%dk(%d%%) wvt:%dk(%d%%) evt:%dk eva:%dk", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs, wtris/1024, curstats[0], wverts/1024, curstats[1], curstats[2], curstats[3]);
                by -= draw_textx("ents:%d(%d) lm:%d rp:%d pvs:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs, entities::ents.length(), entgroup.length(), lightmaps.length(), curstats[7], getnumviewcells());
                if(game::focus->state == CS_EDITING)
                {
                    by -= draw_textx("cube:%s%d corner:%d orient:%d grid:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs,
                            selchildcount<0 ? "1/" : "", abs(selchildcount), sel.corner, sel.orient, sel.grid);
                    by -= draw_textx("sel:%d,%d,%d %d,%d,%d (%d,%d,%d,%d)", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs,
                            sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z,
                                sel.cx, sel.cxs, sel.cy, sel.cys);
                    by -= draw_textx("pos:%d,%d,%d yaw:%d pitch:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs,
                            (int)game::focus->o.x, (int)game::focus->o.y, (int)game::focus->o.z,
                            (int)game::focus->yaw, (int)game::focus->pitch);
                }
            }
            popfont();
        }
    }

    void drawevents(float blend)
    {
        if(game::focus->state != CS_EDITING && game::focus->state != CS_SPECTATOR)
        {
            glPushMatrix();
            glScalef(eventscale, eventscale, 1);
            pushfont("emphasis");
            int ty = ((hudheight/2)-int(hudheight/2*eventoffset))*(1.f/eventscale), tx = (hudwidth/2)*(1.f/eventscale);
            loopv(game::focus->icons)
            {
                if(game::focus->icons[i].type >= eventicon::SORTED && 2+(game::focus->icons[i].type-eventicon::SORTED) > showevents) break;
                if(game::focus->icons[i].type == eventicon::CRITICAL && !game::focus->icons[i].value) continue;
                int millis = lastmillis-game::focus->icons[i].millis;
                if(millis <= game::focus->icons[i].fade)
                {
                    Texture *t = textureload(hud::icontex(game::focus->icons[i].type, game::focus->icons[i].value));
                    if(t && t != notexture)
                    {
                        int olen = min(game::focus->icons[i].length/5, 1000), ilen = olen/2, colour = 0xFFFFFF;
                        float skew = millis < ilen ? millis/float(ilen) : (millis > game::focus->icons[i].fade-olen ? (game::focus->icons[i].fade-millis)/float(olen) : 1.f),
                              fade = blend*eventblend*skew;
                        int size = int(FONTH*skew), width = int((t->w/float(t->h))*size);
                        switch(game::focus->icons[i].type)
                        {
                            case eventicon::WEAPON: colour = WEAP(game::focus->icons[i].value, colour); break;
                            case eventicon::AFFINITY: colour = m_bomber(game::gamemode) ? pulsecols[2][clamp((totalmillis/100)%PULSECOLOURS, 0, PULSECOLOURS-1)] : TEAM(game::focus->icons[i].value, colour); break;
                            default: break;
                        }
                        glBindTexture(GL_TEXTURE_2D, t->id);
                        vec c = vec::hexcolor(colour);
                        glColor4f(c.r, c.g, c.b, fade);
                        drawtexture(tx-width/2, ty-size, width, size);
                        ty -= game::focus->icons[i].type < eventicon::AFFINITY ? int(size*2/3) : int(size);
                    }
                }
            }
            popfont();
            glPopMatrix();
        }
    }

    void drawhud(bool noview)
    {
        float fade = hudblend, consolefade = hudblend;
        if(!progressing)
        {
            vec colour = vec(1, 1, 1);
            if(commandfade && (commandmillis > 0 || totalmillis-abs(commandmillis) <= commandfade))
            {
                float a = min(float(totalmillis-abs(commandmillis))/float(commandfade), 1.f)*commandfadeamt;
                if(commandmillis > 0) a = 1.f-a;
                else a += (1.f-commandfadeamt);
                loopi(3) if(a < colour[i]) colour[i] *= a;
            }
            if(compassfade && (compassmillis > 0 || totalmillis-abs(compassmillis) <= compassfade))
            {
                float a = min(float(totalmillis-abs(compassmillis))/float(compassfade), 1.f)*compassfadeamt;
                if(compassmillis > 0) a = 1.f-a;
                else a += (1.f-compassfadeamt);
                loopi(3) if(a < colour[i]) colour[i] *= a;
            }
            if(huduioverride >= (game::intermission ? 2 : 1) && uifade && (uimillis > 0 || totalmillis-abs(uimillis) <= uifade))
            {
                float n = min(float(totalmillis-abs(uimillis))/float(uifade), 1.f), a = n*uifadeamt;
                if(uimillis > 0) a = 1.f-a;
                else a += (1.f-uifadeamt);
                loopi(3) if(a < colour[i]) colour[i] *= a;
            }
            if(!noview)
            {
                if(titlefade && (client::waiting() || lastmillis-game::maptime <= titlefade))
                {
                    float a = !client::waiting() ? float(lastmillis-game::maptime)/float(titlefade) : 0.f;
                    loopi(3) if(a < colour[i]) colour[i] *= a;
                }
                if(tvmodefade && game::tvmode())
                {
                    float a = game::lasttvchg ? (lastmillis-game::lasttvchg <= tvmodefade ? float(lastmillis-game::lasttvchg)/float(tvmodefade) : 1.f) : 0.f;
                    loopi(3) if(a < colour[i]) colour[i] *= a;
                }
                if(game::focus == game::player1 || !game::thirdpersonview(true))
                {
                    if(spawnfade && game::focus->state == CS_ALIVE && game::focus->lastspawn && lastmillis-game::focus->lastspawn <= spawnfade)
                    {
                        float a = (lastmillis-game::focus->lastspawn)/float(spawnfade/3);
                        if(a < 3.f)
                        {
                            vec col = vec(1, 1, 1); skewcolour(col.x, col.y, col.z, game::getcolour(game::focus, game::playereffecttone));
                            if(a < 1.f) { loopi(3) col[i] *= a; }
                            else { a = (a-1.f)*0.5f; loopi(3) col[i] += (1.f-col[i])*a; }
                            loopi(3) if(col[i] < colour[i]) colour[i] *= col[i];
                        }
                    }
                    if(showdamage >= 2 && damageresidue > 0)
                    {
                        float pc = min(damageresidue, 100)/100.f*damageskew;
                        loopi(2) if(colour[i+1] > 0) colour[i+1] -= colour[i+1]*pc;
                    }
                }
            }
            if(colour.x < 1 || colour.y < 1 || colour.z < 1)
            {
                usetexturing(false);
                drawblend(0, 0, screen->w, screen->h, colour.x, colour.y, colour.z);
                usetexturing(true);
                float amt = (colour.x+colour.y+colour.z)/3.f;
                if(!commandmillis || (commandmillis < 0 && totalmillis-abs(commandmillis) > commandfade)) consolefade *= amt+((1.f-amt)*commandfadeskew);
                fade *= amt;
            }
        }

        int gap = int(hudsize*gapsize), inv = int(hudsize*inventorysize), br = inv+gap*2, bs = (hudwidth-br*2)/2, bx = hudwidth-br, by = hudheight-gap;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLoadIdentity();
        glOrtho(0, hudwidth, hudheight, 0, -1, 1);
        glColor3f(1, 1, 1);

        if(noview) drawbackground(hudwidth, hudheight);
        else if(!client::waiting() && showhud && fade > 0)
        {
            drawheadsup(hudwidth, hudheight, fade, gap, inv, br, bs, bx, by);
            if(showevents && !hasinput(false)) drawevents(fade);
        }
        if(UI::ready && showconsole && showhud)
        {
            drawconsole(showconsole >= 2 ? 1 : 0, hudwidth, hudheight, gap, gap, hudwidth-gap*2, consolefade);
            if(showconsole >= 2 && ((!noview && !progressing) || forceprogress))
                drawconsole(2, hudwidth, hudheight, br+gap*2, by, showfps > 1 || showstats > (m_edit(game::gamemode) ? 0 : 1) ? bs-gap*4 : (bs-gap*4)*2, consolefade);
        }

        glDisable(GL_BLEND);
    }

    void update(int w, int h)
    {
        aspect = w/float(h);
        fovy = 2*atan2(tan(curfov/2*RAD), aspect)/RAD;
        if(w > h)
        {
            hudheight = hudsize;
            hudwidth = int(ceil(hudsize*(w/float(h))));
        }
        else if(w < h)
        {
            hudwidth = hudsize;
            hudheight = int(ceil(hudsize*(h/float(w))));
        }
        else hudwidth = hudheight = hudsize;

        int wait = client::waiting();
        if(wait > 1)
        {
            forceprogress = progressing = true;
            setfvar("progressamt", 0);
            switch(wait)
            {
                case 3: setsvar("progresstitle", "downloading map.."); break;
                case 2: setsvar("progresstitle", "requesting map.."); break;
                case 1: case 0: default: break;
            }
            setsvar("progresstext", "this could take some time..");
            setfvar("progresspart", 0);
        }
        else if(forceprogress) forceprogress = progressing = false;
    }
}
