#ifndef __GAME_H__
#define __GAME_H__

#include "engine.h"

#define GAMEID              "fps"
#define GAMEVERSION         209
#define DEMO_VERSION        GAMEVERSION

#define MAXAI 256
#define MAXPLAYERS (MAXCLIENTS + MAXAI)

// network quantization scale
#define DMF 16.0f           // for world locations
#define DNF 1000.0f         // for normalized vectors
#define DVELF 1.0f          // for playerspeed based velocity vectors

enum
{
    S_JUMP = S_GAMESPECIFIC, S_IMPULSE, S_JETPACK, S_LAND,
    S_PAIN, S_PAIN2, S_PAIN3, S_PAIN4, S_PAIN5, S_PAIN6, S_DEATH, S_DEATH2,
    S_SPLASH, S_SPLASH2, S_UNDERWATER,
    S_SPLOSH, S_SPLOSH2, S_SPLOSH3,
    S_DEBRIS, S_DEBRIS2, S_DEBRIS3,
    S_BURNING, S_BURNFIRE, S_EXTINGUISH,
    S_SHELL, S_SHELL2, S_SHELL3, S_SHELL4, S_SHELL5, S_SHELL6,
    S_ITEMUSE, S_ITEMSPAWN,
    S_REGEN, S_DAMAGE, S_DAMAGE2, S_DAMAGE3, S_DAMAGE4, S_DAMAGE5, S_DAMAGE6, S_DAMAGE7, S_DAMAGE8, S_BURNED, S_CRITICAL,
    S_RESPAWN, S_CHAT, S_ERROR, S_ALARM,
    S_V_FLAGSECURED, S_V_FLAGOVERTHROWN, S_V_FLAGPICKUP, S_V_FLAGDROP, S_V_FLAGRETURN, S_V_FLAGSCORE, S_V_FLAGRESET,
    S_V_BOMBPICKUP, S_V_BOMBRESET,
    S_V_FIGHT, S_V_CHECKPOINT, S_V_ONEMINUTE, S_V_HEADSHOT,
    S_V_SPREE, S_V_SPREE2, S_V_SPREE3, S_V_SPREE4,
    S_V_MULTI, S_V_MULTI2, S_V_MULTI3,
    S_V_REVENGE, S_V_DOMINATE, S_V_FIRSTBLOOD,
    S_V_YOUWIN, S_V_YOULOSE, S_V_MCOMPLETE,
    S_V_FRAGGED, S_V_OWNED,
    S_GAME
};

enum { S_R_DIE = 2, S_R_PAIN = 6, S_R_SPLOSH = 3, S_R_DEBRIS = 3, S_R_SHELL = 6, S_R_DAMAGE = 8 };

enum                                // entity types
{
    NOTUSED = ET_EMPTY, LIGHT = ET_LIGHT, MAPMODEL = ET_MAPMODEL, PLAYERSTART = ET_PLAYERSTART, ENVMAP = ET_ENVMAP, PARTICLES = ET_PARTICLES,
    MAPSOUND = ET_SOUND, LIGHTFX = ET_LIGHTFX, SUNLIGHT = ET_SUNLIGHT, WEAPON = ET_GAMESPECIFIC,
    TELEPORT, ACTOR, TRIGGER, PUSHER, AFFINITY, CHECKPOINT, CAMERA, WAYPOINT, MAXENTTYPES
};

enum { EU_NONE = 0, EU_ITEM, EU_AUTO, EU_ACT, EU_MAX };

enum { TR_TOGGLE = 0, TR_LINK, TR_SCRIPT, TR_ONCE, TR_EXIT, TR_MAX };
enum { TA_MANUAL = 0, TA_AUTO, TA_ACTION, TA_MAX };
#define TRIGGERIDS      16
#define TRIGSTATE(a,b)  (b%2 ? !a : a)

enum { CP_RESPAWN = 0, CP_START, CP_FINISH, CP_LAST, CP_MAX };

struct enttypes
{
    int type,           priority, links,    radius, usetype,    numattrs,
            canlink, reclink;
    bool    noisy,  syncs,  resyncs;
    const char *name,           *attrs[9];
};
#ifdef GAMESERVER
enttypes enttype[] = {
    {
        NOTUSED,        -1,         0,      0,      EU_NONE,    0,
            0, 0,
            true,   false,  false,
                "none",         { "" }
    },
    {
        LIGHT,          1,          59,     0,      EU_NONE,    4,
            (1<<LIGHTFX), (1<<LIGHTFX),
            false,  false,  false,
                "light",        { "radius", "red",      "green",    "blue"  }
    },
    {
        MAPMODEL,       1,          58,     0,      EU_NONE,    7,
            (1<<TRIGGER), (1<<TRIGGER),
            false,  false,  false,
                "mapmodel",     { "type",   "yaw",      "rot",      "blend",    "scale",    "flags",    "colour" }
    },
    {
        PLAYERSTART,    1,          59,     0,      EU_NONE,    5,
            0, 0,
            false,  true,  false,
                "playerstart",  { "team",   "yaw",      "pitch",    "mode",     "id" }
    },
    {
        ENVMAP,         1,          0,      0,      EU_NONE,    3,
            0, 0,
            false,  false,  false,
                "envmap",       { "radius", "size", "blur" }
    },
    {
        PARTICLES,      1,          59,     0,      EU_NONE,    9,
            (1<<TELEPORT)|(1<<TRIGGER)|(1<<PUSHER),
            (1<<TRIGGER)|(1<<PUSHER),
            false,  false,  false,
                "particles",    { "type",   "a",        "b",        "c",        "d",        "e",        "f",        "g",        "i" }
    },
    {
        MAPSOUND,       1,          58,     0,      EU_NONE,    5,
            (1<<TELEPORT)|(1<<TRIGGER)|(1<<PUSHER),
            (1<<TRIGGER)|(1<<PUSHER),
            false,  false,  false,
                "sound",        { "type",   "maxrad",   "minrad",   "volume",   "flags" }
    },
    {
        LIGHTFX,        1,          1,      0,      EU_NONE,    5,
            (1<<LIGHT)|(1<<TELEPORT)|(1<<TRIGGER)|(1<<PUSHER),
            (1<<LIGHT)|(1<<TRIGGER)|(1<<PUSHER),
            false,  false,  false,
                "lightfx",      { "type",   "mod",      "min",      "max",      "flags" }
    },
    {
        SUNLIGHT,       1,          160,    0,      EU_NONE,    6,
            0, 0,
            false,  false,  false,
                "sunlight",     { "yaw",    "pitch",    "red",      "green",    "blue",     "offset" }
    },
    {
        WEAPON,         2,          59,     24,     EU_ITEM,    4,
            0, 0,
            false,  true,   true,
                "weapon",       { "type",   "flags",    "mode",     "id" }
    },
    {
        TELEPORT,       1,          50,     12,     EU_AUTO,    6,
            (1<<MAPSOUND)|(1<<PARTICLES)|(1<<LIGHTFX)|(1<<TELEPORT),
            (1<<MAPSOUND)|(1<<PARTICLES)|(1<<LIGHTFX),
            false,  false,  false,
                "teleport",     { "yaw",    "pitch",    "push",     "radius",   "colour",   "type" }
    },
    {
        ACTOR,          1,          59,     0,      EU_NONE,    9,
            (1<<AFFINITY)|(1<<WAYPOINT), 0,
            false,  true,   false,
                "actor",        { "type",   "yaw",      "pitch",    "mode",     "id",       "weap",     "health",   "speed",    "scale" }
    },
    {
        TRIGGER,        1,          58,     16,     EU_AUTO,    6,
            (1<<MAPMODEL)|(1<<MAPSOUND)|(1<<PARTICLES)|(1<<LIGHTFX),
            (1<<MAPMODEL)|(1<<MAPSOUND)|(1<<PARTICLES)|(1<<LIGHTFX),
            false,  true,   true,
                "trigger",      { "id",     "type",     "action",   "radius",   "state",    "mode" }
    },
    {
        PUSHER,         1,          58,     12,     EU_AUTO,    6,
            (1<<MAPSOUND)|(1<<PARTICLES)|(1<<LIGHTFX),
            (1<<MAPSOUND)|(1<<PARTICLES)|(1<<LIGHTFX),
            false,  false,  false,
                "pusher",       { "yaw",    "pitch",    "force",    "maxrad",   "minrad",   "type" }
    },
    {
        AFFINITY,       1,          48,     36,     EU_NONE,    5,
            (1<<AFFINITY), 0,
            false,  false,  false,
                "affinity",     { "team",   "yaw",      "pitch",    "mode",     "id" }
    },
    {
        CHECKPOINT,     1,          48,     16,     EU_AUTO,    6,
            0, 0,
            false,  true,   false,
                "checkpoint",   { "radius", "yaw",      "pitch",    "mode",     "id",       "type" }
    },
    {
        CAMERA,         1,          48,     0,      EU_NONE,    4,
            0, 0,
            false,  false,  false,
                "camera",       { "yaw",   "pitch",     "mindist",  "maxdist" }
    },
    {
        WAYPOINT,       0,          1,      16,     EU_NONE,    2,
            (1<<WAYPOINT), 0,
            true,   false,  false,
                "waypoint",     { "flags",  "weight" }
    }
};
#else
extern enttypes enttype[];
#endif

enum
{
    ANIM_PAIN = ANIM_GAMESPECIFIC,
    ANIM_JUMP_FORWARD, ANIM_JUMP_BACKWARD, ANIM_JUMP_LEFT, ANIM_JUMP_RIGHT, ANIM_JUMP,
    ANIM_IMPULSE_FORWARD, ANIM_IMPULSE_BACKWARD, ANIM_IMPULSE_LEFT, ANIM_IMPULSE_RIGHT,
    ANIM_DASH_FORWARD, ANIM_DASH_BACKWARD, ANIM_DASH_LEFT, ANIM_DASH_RIGHT, ANIM_DASH_UP,
    ANIM_JETPACK_FORWARD, ANIM_JETPACK_BACKWARD, ANIM_JETPACK_LEFT, ANIM_JETPACK_RIGHT, ANIM_JETPACK_UP,
    ANIM_WALL_RUN_LEFT, ANIM_WALL_RUN_RIGHT, ANIM_WALL_JUMP,
    ANIM_SINK, ANIM_EDIT, ANIM_SWITCH, ANIM_USE, ANIM_WIN, ANIM_LOSE,
    ANIM_CROUCH, ANIM_CRAWL_FORWARD, ANIM_CRAWL_BACKWARD, ANIM_CRAWL_LEFT, ANIM_CRAWL_RIGHT,
    ANIM_CROUCH_JUMP_FORWARD, ANIM_CROUCH_JUMP_BACKWARD, ANIM_CROUCH_JUMP_LEFT, ANIM_CROUCH_JUMP_RIGHT, ANIM_CROUCH_JUMP,
    ANIM_MELEE, ANIM_MELEE_PRIMARY, ANIM_MELEE_SECONDARY,
    ANIM_PISTOL, ANIM_PISTOL_PRIMARY, ANIM_PISTOL_SECONDARY, ANIM_PISTOL_RELOAD,
    ANIM_SWORD, ANIM_SWORD_PRIMARY, ANIM_SWORD_SECONDARY,
    ANIM_SHOTGUN, ANIM_SHOTGUN_PRIMARY, ANIM_SHOTGUN_SECONDARY, ANIM_SHOTGUN_RELOAD,
    ANIM_SMG, ANIM_SMG_PRIMARY, ANIM_SMG_SECONDARY, ANIM_SMG_RELOAD,
    ANIM_FLAMER, ANIM_FLAMER_PRIMARY, ANIM_FLAMER_SECONDARY, ANIM_FLAMER_RELOAD,
    ANIM_PLASMA, ANIM_PLASMA_PRIMARY, ANIM_PLASMA_SECONDARY, ANIM_PLASMA_RELOAD,
    ANIM_RIFLE, ANIM_RIFLE_PRIMARY, ANIM_RIFLE_SECONDARY, ANIM_RIFLE_RELOAD,
    ANIM_GRENADE, ANIM_GRENADE_PRIMARY, ANIM_GRENADE_SECONDARY, ANIM_GRENADE_RELOAD, ANIM_GRENADE_POWER,
    ANIM_ROCKET, ANIM_ROCKET_PRIMARY, ANIM_ROCKET_SECONDARY, ANIM_ROCKET_RELOAD,
    ANIM_MAX
};

#define WEAPSWITCHDELAY PHYSMILLIS*2

#ifndef GAMESERVER
#define FIRECOLOURS 8
const int firecols[FIRECOLOURS] = { 0xFF5808, 0x981808, 0x782808, 0x481808, 0x983818, 0x601808, 0xFF1808, 0x381808 };
#endif

#include "weapons.h"

enum
{
    FRAG_NONE = 0, FRAG_HEADSHOT = 1<<1, FRAG_OBLITERATE = 1<<2,
    FRAG_SPREE1 = 1<<3, FRAG_SPREE2 = 1<<4, FRAG_SPREE3 = 1<<5, FRAG_SPREE4 = 1<<6,
    FRAG_MKILL1 = 1<<7, FRAG_MKILL2 = 1<<8, FRAG_MKILL3 = 1<<9,
    FRAG_REVENGE = 1<<10, FRAG_DOMINATE = 1<<11, FRAG_FIRSTBLOOD = 1<<12,
    FRAG_SPREES = 4, FRAG_SPREE = 3, FRAG_MKILL = 7,
    FRAG_CHECK = FRAG_SPREE1|FRAG_SPREE2|FRAG_SPREE3|FRAG_SPREE4,
    FRAG_MULTI = FRAG_MKILL1|FRAG_MKILL2|FRAG_MKILL3,
};

enum
{
    G_DEMO = 0, G_EDITMODE, G_CAMPAIGN, G_DEATHMATCH, G_CAPTURE, G_DEFEND, G_BOMBER, G_TRIAL, G_MAX,
    G_START = G_EDITMODE, G_PLAY = G_CAMPAIGN, G_FIGHT = G_DEATHMATCH, G_RAND = G_BOMBER-G_DEATHMATCH+1
};
enum
{
    G_M_NONE = 0,
    G_M_TEAM = 1<<0, G_M_INSTA = 1<<1, G_M_MEDIEVAL = 1<<2, G_M_BALLISTIC = 1<<3,
    G_M_DUEL = 1<<4, G_M_SURVIVOR = 1<<5, G_M_ARENA = 1<<6, G_M_ONSLAUGHT = 1<<7,
    G_M_JETPACK = 1<<8, G_M_VAMPIRE = 1<<9, G_M_EXPERT = 1<<10, G_M_RESIZE = 1<<11,
    G_M_GSP1 = 1<<12, G_M_GSP2 = 1<<13, G_M_GSP3 = 1<<14,
    G_M_ALL = G_M_TEAM|G_M_INSTA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE,
    G_M_GSN = 3, G_M_GSP = 12, G_M_NUM = 15
};

struct gametypes
{
    int type,           implied,
        mutators[G_M_GSN+1];
    const char *name,                       *gsp[G_M_GSN];
};
struct mutstypes
{
    int type,           implied,            mutators;
    const char *name;
};
#ifdef GAMESERVER
gametypes gametype[] = {
    {
        G_DEMO,         G_M_NONE,
        {
            G_M_NONE, G_M_NONE, G_M_NONE, G_M_NONE
        },
        "demo",                             { "", "", "" },
    },
    {
        G_EDITMODE,     G_M_NONE,
        {
            G_M_TEAM|G_M_INSTA|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE,
            G_M_NONE, G_M_NONE, G_M_NONE
        },
        "editing",                          { "", "", "" },
    },
    {
        G_CAMPAIGN,     G_M_TEAM,
        {
            G_M_TEAM|G_M_INSTA|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE,
            G_M_NONE, G_M_NONE, G_M_NONE
        },
        "campaign",                         { "", "", "" },
    },
    {
        G_DEATHMATCH,   G_M_NONE,
        {
            G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE,
            G_M_NONE, G_M_NONE, G_M_NONE
        },
        "deathmatch",                       { "", "", "" },
    },
    {
        G_CAPTURE,      G_M_TEAM,
        {
            G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
            G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1,
            G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP2,
            G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP3
        },
        "capture-the-flag",                 { "return", "defend", "protect" },
    },
    {
        G_DEFEND,       G_M_TEAM,
        {
            G_M_TEAM|G_M_INSTA|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2,
            G_M_TEAM|G_M_INSTA|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1,
            G_M_TEAM|G_M_INSTA|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP2,
            G_M_NONE
        },
        "defend-the-flag",                  { "quick", "conquer", "" },
    },
    {
        G_BOMBER,       G_M_TEAM,
        {
            G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2,
            G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2,
            G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2,
            G_M_NONE
        },
        "bomber-ball",                      { "multi", "hold", "" },
    },
    {
        G_TRIAL,        G_M_NONE,
        {
            G_M_TEAM|G_M_INSTA|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE,
            G_M_NONE, G_M_NONE, G_M_NONE
        },
        "time-trial",                       { "", "", "" },
    },
};
mutstypes mutstype[] = {
    {
        G_M_TEAM,       G_M_TEAM,           G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "team"
    },
    {
        G_M_INSTA,      G_M_INSTA,          G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "instagib"
    },
    {
        G_M_MEDIEVAL,   G_M_MEDIEVAL,       G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_MEDIEVAL|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "medieval"
    },
    {
        G_M_BALLISTIC,  G_M_BALLISTIC,      G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "ballistic"
    },
    {
        G_M_DUEL,       G_M_DUEL,           G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "duel"
    },
    {
        G_M_SURVIVOR,   G_M_SURVIVOR,       G_M_TEAM|G_M_INSTA|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "survivor",
    },
    {
        G_M_ARENA,      G_M_ARENA,          G_M_TEAM|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "arena",
    },
    {
        G_M_ONSLAUGHT,  G_M_ONSLAUGHT,      G_M_TEAM|G_M_INSTA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "onslaught",
    },
    {
        G_M_JETPACK,    G_M_JETPACK,        G_M_TEAM|G_M_INSTA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "jetpack",
    },
    {
        G_M_VAMPIRE,    G_M_VAMPIRE,        G_M_TEAM|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "vampire",
    },
    {
        G_M_EXPERT,     G_M_EXPERT,         G_M_TEAM|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "expert",
    },
    {
        G_M_RESIZE,     G_M_RESIZE,         G_M_TEAM|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "resize",
    },
    {
        G_M_GSP1,       G_M_GSP1,           G_M_TEAM|G_M_INSTA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "",
    },
    {
        G_M_GSP2,       G_M_GSP2,           G_M_TEAM|G_M_INSTA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "",
    },
    {
        G_M_GSP3,       G_M_GSP3,           G_M_TEAM|G_M_INSTA|G_M_MEDIEVAL|G_M_BALLISTIC|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_ONSLAUGHT|G_M_JETPACK|G_M_VAMPIRE|G_M_EXPERT|G_M_RESIZE|G_M_GSP1|G_M_GSP2|G_M_GSP3,
        "",
    },
};
#else
extern gametypes gametype[];
extern mutstypes mutstype[];
#endif

#define m_game(a)           (a > -1 && a < G_MAX)
#define m_check(a,b)        (!a || (a < 0 ? -a != b : a == b))

#define m_demo(a)           (a == G_DEMO)
#define m_edit(a)           (a == G_EDITMODE)
#define m_campaign(a)       (a == G_CAMPAIGN)
#define m_dm(a)             (a == G_DEATHMATCH)
#define m_capture(a)        (a == G_CAPTURE)
#define m_defend(a)         (a == G_DEFEND)
#define m_bomber(a)         (a == G_BOMBER)
#define m_trial(a)          (a == G_TRIAL)

#define m_play(a)           (a >= G_PLAY)
#define m_affinity(a)       (m_capture(a) || m_defend(a) || m_bomber(a))
#define m_fight(a)          (a >= G_FIGHT)

#define m_team(a,b)         ((b & G_M_TEAM) || (gametype[a].implied & G_M_TEAM))
#define m_insta(a,b)        ((b & G_M_INSTA) || (gametype[a].implied & G_M_INSTA))
#define m_medieval(a,b)     ((b & G_M_MEDIEVAL) || (gametype[a].implied & G_M_MEDIEVAL))
#define m_ballistic(a,b)    ((b & G_M_BALLISTIC) || (gametype[a].implied & G_M_BALLISTIC))
#define m_duel(a,b)         ((b & G_M_DUEL) || (gametype[a].implied & G_M_DUEL))
#define m_survivor(a,b)     ((b & G_M_SURVIVOR) || (gametype[a].implied & G_M_SURVIVOR))
#define m_arena(a,b)        ((b & G_M_ARENA) || (gametype[a].implied & G_M_ARENA))
#define m_onslaught(a,b)    ((b & G_M_ONSLAUGHT) || (gametype[a].implied & G_M_ONSLAUGHT))
#define m_jetpack(a,b)      ((b & G_M_JETPACK) || (gametype[a].implied & G_M_JETPACK))
#define m_vampire(a,b)      ((b & G_M_VAMPIRE) || (gametype[a].implied & G_M_VAMPIRE))
#define m_expert(a,b)       ((b & G_M_EXPERT) || (gametype[a].implied & G_M_EXPERT))
#define m_resize(a,b)       ((b & G_M_RESIZE) || (gametype[a].implied & G_M_RESIZE))

#define m_gsp1(a,b)         ((b & G_M_GSP1) || (gametype[a].implied & G_M_GSP1))
#define m_gsp2(a,b)         ((b & G_M_GSP2) || (gametype[a].implied & G_M_GSP2))
#define m_gsp3(a,b)         ((b & G_M_GSP3) || (gametype[a].implied & G_M_GSP3))
#define m_gsp(a,b)          (m_gsp1(a,b) || m_gsp2(a,b) || m_gsp3(a,b))

#define m_limited(a,b)      (m_insta(a, b) || m_medieval(a, b) || m_ballistic(a, b))
#define m_duke(a,b)         (m_duel(a, b) || m_survivor(a, b))
#define m_regen(a,b)        (!m_duke(a, b) && !m_insta(a, b))
#define m_enemies(a,b)      (m_campaign(a) || m_onslaught(a, b))
#define m_scores(a)         (a >= G_EDITMODE && a <= G_DEATHMATCH)
#define m_checkpoint(a)     (m_campaign(a) || m_trial(a))

#define m_weapon(a,b)       (m_arena(a,b) ? -1 : (m_medieval(a,b) ? WEAP_SWORD : (m_ballistic(a,b) ? WEAP_ROCKET : (m_insta(a,b) ? GAME(instaweapon) : (m_trial(a) ? GAME(trialweapon) : GAME(spawnweapon))))))
#define m_delay(a,b)        (m_play(a) && !m_duke(a,b) ? (m_trial(a) ? GAME(trialdelay) : (m_bomber(a) ? GAME(bomberdelay) : (m_insta(a, b) ? GAME(instadelay) : GAME(spawndelay)))) : 0)
#define m_protect(a,b)      (m_duke(a,b) ? GAME(duelprotect) : (m_insta(a, b) ? GAME(instaprotect) : GAME(spawnprotect)))
#define m_noitems(a,b)      (m_trial(a) || GAME(itemsallowed) < (m_limited(a,b) ? 2 : 1))
#define m_health(a,b)       (m_insta(a,b) ? 1 : GAME(spawnhealth))

#define w_reload(w1,w2)     (w1 != WEAP_MELEE && ((isweap(w2) ? w1 == w2 : w1 < WEAP_ITEM) || (isweap(w1) && WEAP(w1, reloads))))
#define w_carry(w1,w2)      (w1 > WEAP_MELEE && (isweap(w2) ? w1 != w2 : w1 >= WEAP_ITEM) && (isweap(w1) && WEAP(w1, carried)))
#define w_attr(a,w1,w2)     (m_edit(a) || (w1 >= WEAP_OFFSET && w1 != w2) ? w1 : (w2 == WEAP_GRENADE ? WEAP_ROCKET : WEAP_GRENADE))
#define w_spawn(weap)       int(GAME(itemspawntime)*WEAP(weap, frequency))

// network messages codes, c2s, c2c, s2c
enum
{
    N_CONNECT = 0, N_SERVERINIT, N_WELCOME, N_CLIENTINIT, N_POS, N_SPHY, N_TEXT, N_COMMAND, N_ANNOUNCE, N_DISCONNECT,
    N_SHOOT, N_DESTROY, N_SUICIDE, N_DIED, N_POINTS, N_DAMAGE, N_SHOTFX,
    N_LOADWEAP, N_TRYSPAWN, N_SPAWNSTATE, N_SPAWN, N_DROP, N_WEAPSELECT,
    N_MAPCHANGE, N_MAPVOTE, N_CLEARVOTE, N_CHECKPOINT, N_ITEMSPAWN, N_ITEMUSE, N_TRIGGER, N_EXECLINK,
    N_PING, N_PONG, N_CLIENTPING, N_TICK, N_NEWGAME, N_ITEMACC, N_SERVMSG, N_GAMEINFO, N_RESUME,
    N_EDITMODE, N_EDITENT, N_EDITLINK, N_EDITVAR, N_EDITF, N_EDITT, N_EDITM, N_FLIP, N_COPY, N_PASTE, N_ROTATE, N_REPLACE, N_DELCUBE, N_REMIP, N_CLIPBOARD, N_NEWMAP,
    N_GETMAP, N_SENDMAP, N_FAILMAP, N_SENDMAPFILE, N_SENDMAPSHOT, N_SENDMAPCONFIG,
    N_MASTERMODE, N_KICK, N_CLEARBANS, N_CURRENTMASTER, N_SPECTATOR, N_WAITING, N_SETMASTER, N_SETTEAM,
    N_AFFIN, N_INFOAFFIN,
    N_TAKEAFFIN, N_RETURNAFFIN, N_RESETAFFIN, N_DROPAFFIN, N_SCOREAFFIN, N_INITAFFIN, N_SCORE,
    N_LISTDEMOS, N_SENDDEMOLIST, N_GETDEMO, N_SENDDEMO,
    N_DEMOPLAYBACK, N_RECORDDEMO, N_STOPDEMO, N_CLEARDEMOS,
    N_CLIENT, N_RELOAD, N_REGEN,
    N_ADDBOT, N_DELBOT, N_INITAI,
    N_MAPCRC, N_CHECKMAPS,
    N_SWITCHNAME, N_SWITCHTEAM,
    N_AUTHTRY, N_AUTHCHAL, N_AUTHANS,
    NUMSV
};

#ifdef GAMESERVER
char msgsizelookup(int msg)
{
    static const int msgsizes[] =               // size inclusive message token, 0 for variable or not-checked sizes
    {
        N_CONNECT, 0, N_SERVERINIT, 5, N_WELCOME, 1, N_CLIENTINIT, 0, N_POS, 0, N_SPHY, 0, N_TEXT, 0, N_COMMAND, 0,
        N_ANNOUNCE, 0, N_DISCONNECT, 3,
        N_SHOOT, 0, N_DESTROY, 0, N_SUICIDE, 3, N_DIED, 8, N_POINTS, 4, N_DAMAGE, 10, N_SHOTFX, 0,
        N_LOADWEAP, 0, N_TRYSPAWN, 2, N_SPAWNSTATE, 0, N_SPAWN, 0,
        N_DROP, 0, N_WEAPSELECT, 0,
        N_MAPCHANGE, 0, N_MAPVOTE, 0, N_CLEARVOTE, 0, N_CHECKPOINT, 0, N_ITEMSPAWN, 3, N_ITEMUSE, 0, N_TRIGGER, 0, N_EXECLINK, 3,
        N_PING, 2, N_PONG, 2, N_CLIENTPING, 2,
        N_TICK, 2, N_NEWGAME, 1, N_ITEMACC, 0,
        N_SERVMSG, 0, N_GAMEINFO, 0, N_RESUME, 0,
        N_EDITMODE, 2, N_EDITENT, 0, N_EDITLINK, 4, N_EDITVAR, 0, N_EDITF, 16, N_EDITT, 16, N_EDITM, 15, N_FLIP, 14, N_COPY, 14, N_PASTE, 14, N_ROTATE, 15, N_REPLACE, 16, N_DELCUBE, 14, N_REMIP, 1, N_NEWMAP, 2,
        N_GETMAP, 0, N_SENDMAP, 0, N_FAILMAP, 0, N_SENDMAPFILE, 0, N_SENDMAPSHOT, 0, N_SENDMAPCONFIG, 0,
        N_MASTERMODE, 2, N_KICK, 2, N_CLEARBANS, 1, N_CURRENTMASTER, 3, N_SPECTATOR, 3, N_WAITING, 2, N_SETMASTER, 0, N_SETTEAM, 0,
        N_AFFIN, 0, N_INFOAFFIN, 0,
        N_DROPAFFIN, 0, N_SCOREAFFIN, 0, N_RETURNAFFIN, 0, N_TAKEAFFIN, 0, N_RESETAFFIN, 0, N_INITAFFIN, 0, N_SCORE, 0,
        N_LISTDEMOS, 1, N_SENDDEMOLIST, 0, N_GETDEMO, 2, N_SENDDEMO, 0,
        N_DEMOPLAYBACK, 3, N_RECORDDEMO, 2, N_STOPDEMO, 1, N_CLEARDEMOS, 2,
        N_CLIENT, 0, N_RELOAD, 0, N_REGEN, 0,
        N_ADDBOT, 0, N_DELBOT, 0, N_INITAI, 0,
        N_MAPCRC, 0, N_CHECKMAPS, 1,
        N_SWITCHNAME, 0, N_SWITCHTEAM, 0,
        N_AUTHTRY, 0, N_AUTHCHAL, 0, N_AUTHANS, 0,
        -1
    };
    static int sizetable[NUMSV] = { -1 };
    if(sizetable[0] < 0)
    {
        memset(sizetable, -1, sizeof(sizetable));
        for(const int *p = msgsizes; *p >= 0; p += 2) sizetable[p[0]] = p[1];
    }
    return msg >= 0 && msg < NUMSV ? sizetable[msg] : -1;
}
#else
extern char msgsizelookup(int msg);
#endif
enum { SPHY_NONE = 0, SPHY_JUMP, SPHY_BOOST, SPHY_KICK, SPHY_SKATE, SPHY_DASH, SPHY_POWER, SPHY_EXTINGUISH, SPHY_MAX };
enum { CON_CHAT = CON_GAMESPECIFIC, CON_EVENT, CON_MAX, CON_LO = CON_MESG, CON_HI = CON_SELF, CON_IMPORTANT = CON_SELF };

#define DEMO_MAGIC "DMOZ"
struct demoheader
{
    char magic[16];
    int version, gamever;
};

enum
{
    TEAM_NEUTRAL = 0, TEAM_ALPHA, TEAM_OMEGA, TEAM_ENEMY, TEAM_MAX,
    TEAM_FIRST = TEAM_ALPHA, TEAM_LAST = TEAM_OMEGA, TEAM_NUM = (TEAM_LAST-TEAM_FIRST)+1, TEAM_COUNT = TEAM_LAST+1
};
struct teamtypes
{
    int type,           colour; const char  *name,
        *icon,              *chat,      *colname;
};
#ifdef GAMESERVER
teamtypes teamtype[] = {
    {
        TEAM_NEUTRAL,   0x66BB33,           "neutral",
        "team",             "\fa",      "green"
    },
    {
        TEAM_ALPHA,     0x6666FF,           "alpha",
        "teamalpha",        "\fb",      "blue"
    },
    {
        TEAM_OMEGA,     0xFF6666,           "omega",
        "teamomega",         "\fr",      "red"
    },
    {
        TEAM_ENEMY,     0xFFFF66,           "enemy",
        "team",             "\fy",      "yellow"
    }
};
#else
extern teamtypes teamtype[];
#endif
struct score
{
    int team, total;
    score() {}
    score(int s, int n) : team(s), total(n) {}
};
enum { BASE_NONE = 0, BASE_HOME = 1<<0, BASE_FLAG = 1<<1, BASE_BOTH = BASE_HOME|BASE_FLAG };

#define numteams(a,b)   (m_fight(a) && m_team(a,b) ? TEAM_NUM : 1)
#define isteam(a,b,c,d) (m_fight(a) && m_team(a,b) ? (c >= d && c <= numteams(a,b)) : c == TEAM_NEUTRAL)
#define valteam(a,b)    (a >= b && a <= TEAM_NUM)
#define adjustscaled(t,n,s) { \
    if(n > 0) { n = (t)(n/(1.f+sqrtf((float)curtime)/float(s))); if(n <= 0) n = (t)0; } \
    else if(n < 0) { n = (t)(n/(1.f+sqrtf((float)curtime)/float(s))); if(n >= 0) n = (t)0; } \
}

#define MAXNAMELEN 24
enum { SAY_NONE = 0, SAY_ACTION = 1<<0, SAY_TEAM = 1<<1, SAY_NUM = 2 };
enum { PRIV_NONE = 0, PRIV_MASTER, PRIV_ADMIN, PRIV_MAX };

#define MM_MODE 0xF
#define MM_AUTOAPPROVE 0x1000
#define MM_FREESERV (MM_AUTOAPPROVE|MM_MODE)
#define MM_VETOSERV ((1<<MM_OPEN)|(1<<MM_VETO))
#define MM_COOPSERV (MM_AUTOAPPROVE|MM_VETOSERV|(1<<MM_LOCKED))
#define MM_OPENSERV (MM_AUTOAPPROVE|(1<<MM_OPEN))

enum { MM_OPEN = 0, MM_VETO, MM_LOCKED, MM_PRIVATE, MM_PASSWORD };
enum { CAMERA_NONE = 0, CAMERA_PLAYER, CAMERA_FOLLOW, CAMERA_ENTITY, CAMERA_MAX };
enum { SINFO_STATUS = 0, SINFO_NAME, SINFO_PORT, SINFO_QPORT, SINFO_DESC, SINFO_MODE, SINFO_MUTS, SINFO_MAP, SINFO_TIME, SINFO_NUMPLRS, SINFO_MAXPLRS, SINFO_PING, SINFO_MAX };
enum { SSTAT_OPEN = 0, SSTAT_LOCKED, SSTAT_PRIVATE, SSTAT_FULL, SSTAT_UNKNOWN, SSTAT_MAX };

enum { AC_ATTACK = 0, AC_ALTERNATE, AC_RELOAD, AC_USE, AC_JUMP, AC_SPRINT, AC_CROUCH, AC_SPECIAL, AC_DROP, AC_AFFINITY, AC_TOTAL, AC_DASH = AC_TOTAL, AC_MAX };
enum { IM_METER = 0, IM_TYPE, IM_TIME, IM_LAST, IM_COUNT, IM_COLLECT, IM_SLIDE, IM_JUMP, IM_JETPACK, IM_MAX };
enum { IM_T_NONE = 0, IM_T_BOOST, IM_T_KICK, IM_T_SKATE, IM_T_DASH, IM_T_MAX, IM_T_WALL = IM_T_KICK };

#define CROUCHHEIGHT 0.7f
#define PHYSMILLIS 250

#include "ai.h"
#include "vars.h"

// inherited by gameent and server clients
struct gamestate
{
    int health, ammo[WEAP_MAX], entid[WEAP_MAX];
    int lastweap, loadweap[2], weapselect, weapload[WEAP_MAX], weapshot[WEAP_MAX], weapstate[WEAP_MAX], weapwait[WEAP_MAX], weaplast[WEAP_MAX];
    int lastdeath, lastspawn, lastrespawn, lastpain, lastregen, lastburn, lastbleed;
    int aitype, aientity, ownernum, skill, points, frags, deaths, cpmillis, cptime;

    gamestate() : weapselect(WEAP_MELEE), lastdeath(0), lastspawn(0), lastrespawn(0), lastpain(0), lastregen(0), lastburn(0), lastbleed(0),
        aitype(-1), aientity(-1), ownernum(-1), skill(0), points(0), frags(0), deaths(0), cpmillis(0), cptime(0)
    {
        loopj(2) loadweap[j] = -1;
    }
    ~gamestate() {}

    int hasweap(int weap, int sweap, int level = 0, int exclude = -1)
    {
        if(isweap(weap) && weap != exclude)
        {
            if(ammo[weap] > 0 || (w_reload(weap, sweap) && !ammo[weap])) switch(level)
            {
                case 0: default: return true; break; // has weap at all
                case 1: if(w_carry(weap, sweap)) return true; break; // only carriable
                case 2: if(ammo[weap] > 0) return true; break; // only with actual ammo
                case 3: if(ammo[weap] > 0 && w_reload(weap, sweap)) return true; break; // only reloadable with actual ammo
                case 4: if(ammo[weap] >= (w_reload(weap, sweap) ? 0 : WEAP(weap, max))) return true; break; // only reloadable or those with < max
                case 5: if(w_carry(weap, sweap) || (!w_reload(weap, sweap) && weap >= WEAP_OFFSET)) return true; break; // special case for usable weapons
            }
        }
        return false;
    }

    int bestweap(int sweap, bool last = false)
    {
        if(last && hasweap(lastweap, sweap)) return lastweap;
        loopirev(WEAP_MAX) if(hasweap(i, sweap, 3)) return i; // reloadable first
        loopirev(WEAP_MAX) if(hasweap(i, sweap, 1)) return i; // carriable second
        loopirev(WEAP_MAX) if(hasweap(i, sweap, 0)) return i; // any just to bail us out
        return weapselect;
    }

    int carry(int sweap, int level = 1, int exclude = -1)
    {
        int carry = 0;
        loopi(WEAP_MAX) if(hasweap(i, sweap, level, exclude)) carry++;
        return carry;
    }

    int drop(int sweap)
    {
        loopj(2)
        {
            if(hasweap(weapselect, sweap, 1) && (j || entid[weapselect] >= 0)) return weapselect;
            if(hasweap(lastweap, sweap, 1) && (j || entid[lastweap] >= 0)) return lastweap;
            loopi(WEAP_MAX) if(hasweap(i, sweap, 1) && (j || entid[i] >= 0)) return i;
        }
        return -1;
    }

    void weapreset(bool full = false)
    {
        loopi(WEAP_MAX)
        {
            weapstate[i] = WEAP_S_IDLE;
            weapwait[i] = weaplast[i] = weapload[i] = weapshot[i] = 0;
            if(full) ammo[i] = entid[i] = -1;
        }
    }

    void setweapstate(int weap, int state, int delay, int millis)
    {
        weapstate[weap] = state;
        weapwait[weap] = delay;
        weaplast[weap] = millis;
    }

    void weapswitch(int weap, int millis, int state = WEAP_S_SWITCH)
    {
        if(isweap(weap))
        {
            lastweap = weapselect;
            setweapstate(lastweap, WEAP_S_SWITCH, WEAPSWITCHDELAY, millis);
            weapselect = weap;
            setweapstate(weap, state, WEAPSWITCHDELAY, millis);
        }
    }

    bool weapwaited(int weap, int millis, int skip = 0)
    {
        if(!weapwait[weap] || weapstate[weap] == WEAP_S_IDLE || weapstate[weap] == WEAP_S_POWER || (skip && skip&(1<<weapstate[weap]))) return true;
        return millis-weaplast[weap] >= weapwait[weap];
    }

    int skipwait(int weap, int flags, int millis, int skip = 0, bool override = false)
    {
        int skipstate = skip;
        if(WEAP2(weap, sub, flags&HIT_ALT) && (skip&(1<<WEAP_S_RELOAD)) && weapstate[weap] == WEAP_S_RELOAD && millis-weaplast[weap] < weapwait[weap])
        {
            if(!override && ammo[weap]-weapload[weap] < WEAP2(weap, sub, flags&HIT_ALT))
                skipstate &= ~(1<<WEAP_S_RELOAD);
        }
        return skipstate;
    }

    bool canswitch(int weap, int sweap, int millis, int skip = 0)
    {
        if((aitype >= AI_START || weap != WEAP_MELEE || sweap == WEAP_MELEE || weapselect == WEAP_MELEE) && weap != weapselect && weapwaited(weapselect, millis, skipwait(weapselect, 0, millis, skip, true)) && hasweap(weap, sweap) && weapwaited(weap, millis, skipwait(weap, 0, millis, skip, true)))
            return true;
        return false;
    }

    bool canshoot(int weap, int flags, int sweap, int millis, int skip = 0)
    {
        if((hasweap(weap, sweap) && ammo[weap] >= (WEAP2(weap, power, flags&HIT_ALT) ? 1 : WEAP2(weap, sub, flags&HIT_ALT))) && weapwaited(weap, millis, skipwait(weap, flags, millis, skip)))
            return true;
        return false;
    }

    bool canreload(int weap, int sweap, int millis)
    {
        if(weap == weapselect && w_reload(weap, sweap) && hasweap(weap, sweap) && ammo[weap] < WEAP(weap, max) && weapwaited(weap, millis))
            return true;
        return false;
    }

    bool canuse(int type, int attr, attrvector &attrs, int sweap, int millis, int skip = 0)
    {
        //if((type != TRIGGER || attrs[2] == TA_AUTO) && enttype[type].usetype == EU_AUTO) return true;
        //if(weapwaited(weapselect, millis, skipwait(weapselect, 0, millis, skip)))
        switch(enttype[type].usetype)
        {
            case EU_AUTO: case EU_ACT: return true; break;
            case EU_ITEM:
            { // can't use when reloading or firing
                if(isweap(attr) && !hasweap(attr, sweap, 4) && weapwaited(weapselect, millis, skipwait(weapselect, 0, millis, skip, true)))
                    return true;
                break;
            }
            default: break;
        }
        return false;
    }

    void useitem(int id, int type, int attr, int amt, int sweap, int millis)
    {
        switch(type)
        {
            case TRIGGER: break;
            case WEAPON:
            {
                int prev = ammo[attr], value = amt >= 0 ? amt : WEAPUSE(attr);
                weapswitch(attr, millis, hasweap(attr, sweap) ? WEAP_S_SWITCH : WEAP_S_USE);
                ammo[attr] = clamp(max(ammo[attr], 0)+value, 1, WEAP(attr, max));
                weapload[attr] = ammo[attr]-prev;
                entid[attr] = id;
                break;
            }
            default: break;
        }
    }

    void clearstate()
    {
        lastdeath = lastpain = lastregen = lastburn = lastbleed = 0;
        lastrespawn = -1;
    }

    void mapchange()
    {
        points = cpmillis = cptime = 0;
        loopj(2) loadweap[j] = -1;
    }

    void respawn(int millis, int heal)
    {
        health = heal;
        lastspawn = millis;
        clearstate();
        weapreset(true);
    }

    void spawnstate(int gamemode, int mutators, int sweap = -1, int heal = 0)
    {
        health = heal ? heal : m_health(gamemode, mutators);
        weapreset(true);
        if(!isweap(sweap)) sweap = aitype >= AI_START ? WEAP_MELEE : m_weapon(gamemode, mutators);
        if(isweap(sweap)) ammo[sweap] = max(WEAPUSE(sweap), 1);
        if(aitype >= AI_START)
        {
            loopj(2) loadweap[j] = -1;
            lastweap = weapselect = sweap;
        }
        else
        {
            if(sweap != WEAP_MELEE) ammo[WEAP_MELEE] = WEAP(WEAP_MELEE, max);
            if(GAME(spawngrenades) >= (m_insta(gamemode, mutators) || m_trial(gamemode) ? 2 : 1) && sweap != WEAP_GRENADE)
                ammo[WEAP_GRENADE] = max(WEAP(WEAP_GRENADE, max), 1);
            if(m_arena(gamemode, mutators))
            {
                int aweap[2] = { -1, -1 };
                loopj(2)
                {
                    aweap[j] = loadweap[j];
                    if(aweap[j] < WEAP_OFFSET || aweap[j] >= WEAP_ITEM || (j && aweap[0] == aweap[1]))
                    {
                        aweap[j] = rnd(WEAP_ITEM-WEAP_OFFSET)+WEAP_OFFSET; // random
                        int iters = 0;
                        while(++iters < 10 && ((j && aweap[0] == aweap[1]) || WEAP(aweap[j], allowed) <= (m_duke(gamemode, mutators) ? 1 : 0)))
                        {
                            if(++aweap[j] >= WEAP_ITEM) aweap[j] = WEAP_OFFSET;
                        }
                    }
                    ammo[aweap[j]] = max(WEAPUSE(aweap[j]), 1);
                }
                lastweap = weapselect = aweap[0];
            }
            else
            {
                loopj(2) loadweap[j] = -1;
                lastweap = weapselect = sweap;
            }
        }
    }

    void editspawn(int gamemode, int mutators, int sweap = -1, int heal = 0)
    {
        clearstate();
        spawnstate(gamemode, mutators, sweap, heal);
    }

    int respawnwait(int millis, int delay)
    {
        return lastdeath ? max(0, delay-(millis-lastdeath)) : 0;
    }

    int protect(int millis, int delay)
    {
        int amt = 0;
        if(aitype < AI_START && lastspawn && delay && millis-lastspawn <= delay) amt = delay-(millis-lastspawn);
        return amt;
    }

    bool burning(int millis, int len) { return len && lastburn && millis-lastburn <= len; }
    bool bleeding(int millis, int len) { return len && lastbleed && millis-lastbleed <= len; }
};

namespace server
{
    extern void stopdemo();
    extern void hashpassword(int cn, int sessionid, const char *pwd, char *result, int maxlen = MAXSTRLEN);
    extern bool servcmd(int nargs, const char *cmd, const char *arg);
}

#if !defined(GAMESERVER) && !defined(STANDALONE)
struct gameentity : extentity
{
    int schan;
    int lastuse, lastspawn;
    int mark;
    linkvector kin;

    gameentity() : schan(-1), lastuse(0), lastspawn(0), mark(0) {}
    ~gameentity()
    {
        if(issound(schan)) removesound(schan);
        schan = -1;
    }
};

enum
{
    ST_NONE     = 0,
    ST_CAMERA   = 1<<0,
    ST_CURSOR   = 1<<1,
    ST_GAME     = 1<<2,
    ST_SPAWNS   = 1<<3,
    ST_DEFAULT  = ST_CAMERA|ST_CURSOR|ST_GAME,
    ST_VIEW     = ST_CURSOR|ST_CAMERA,
    ST_ALL      = ST_CAMERA|ST_CURSOR|ST_GAME|ST_SPAWNS,
};

enum { ITEM_ENT = 0, ITEM_PROJ, ITEM_MAX };
struct actitem
{
    int type, target;
    float score;

    actitem() : type(ITEM_ENT), target(-1), score(0) {}
    ~actitem() {}
};
#ifdef GAMEWORLD
const char * const animnames[] =
{
    "idle", "forward", "backward", "left", "right", "dead", "dying", "swim",
    "mapmodel", "trigger on", "trigger off", "pain",
    "jump forward", "jump backward", "jump left", "jump right", "jump",
    "impulse forward", "impulse backward", "impulse left", "impulse right",
    "dash forward", "dash backward", "dash left", "dash right", "dash up",
    "jetpack forward", "jetpack backward", "jetpack left", "jetpack right", "jetpack up",
    "wall run left", "wall run right", "wall jump",
    "sink", "edit", "switch", "use", "win", "lose",
    "crouch", "crawl forward", "crawl backward", "crawl left", "crawl right",
    "crouch jump forward", "crouch jump backward", "crouch jump left", "crouch jump right", "crouch jump",
    "melee", "melee primary", "melee secondary",
    "pistol", "pistol primary", "pistol secondary", "pistol reload",
    "sword", "sword primary", "sword secondary",
    "shotgun", "shotgun primary", "shotgun secondary", "shotgun reload",
    "smg", "smg primary", "smg secondary", "smg reload",
    "flamer", "flamer primary", "flamer secondary", "flamer reload",
    "plasma", "plasma primary", "plasma secondary", "plasma reload",
    "rifle", "rifle primary", "rifle secondary", "rifle reload",
    "grenade", "grenade primary", "grenade secondary", "grenade reload", "grenade power",
    "rocket", "rocket primary", "rocket secondary", "rocket reload",
    ""
};
#else
extern const char * const animnames[];
#endif

struct eventicon
{
    enum { SPREE = 0, MULTIKILL, HEADSHOT, CRITICAL, DOMINATE, REVENGE, FIRSTBLOOD, AFFINITY, WEAPON, TOTAL, SORTED = AFFINITY, VERBOSE = WEAPON };
    int type, millis, fade, length, value;
};

struct gameent : dynent, gamestate
{
    editinfo *edit; ai::aiinfo *ai;
    int team, clientnum, privilege, projid, lastnode, checkpoint, cplast, respawned, suicided, lastupdate, lastpredict, plag, ping, lastflag, totaldamage,
        actiontime[AC_MAX], impulse[IM_MAX], smoothmillis, turnmillis, turnside, aschan, cschan, vschan, wschan, pschan, fschan, jschan, lasthit, lastkill, lastattacker, lastpoints, quake;
    float deltayaw, deltapitch, newyaw, newpitch, deltaaimyaw, deltaaimpitch, newaimyaw, newaimpitch, turnyaw, turnroll;
    vec head, torso, muzzle, origin, eject, waist, jet[3], legs, hrad, trad, lrad;
    bool action[AC_MAX], conopen, k_up, k_down, k_left, k_right, obliterated;
    string name, info, obit;
    vector<int> airnodes;
    vector<gameent *> dominating, dominated;
    vector<eventicon> icons;

    gameent() : edit(NULL), ai(NULL), team(TEAM_NEUTRAL), clientnum(-1), privilege(PRIV_NONE), projid(0), checkpoint(-1), cplast(0), lastupdate(0), lastpredict(0), plag(0), ping(0),
        totaldamage(0), smoothmillis(-1), turnmillis(0), aschan(-1), cschan(-1), vschan(-1), wschan(-1), pschan(-1), fschan(-1), jschan(-1), lastattacker(-1), lastpoints(0), quake(0),
        conopen(false), k_up(false), k_down(false), k_left(false), k_right(false), obliterated(false)
    {
        name[0] = info[0] = obit[0] = 0;
        dominating.shrink(0);
        dominated.shrink(0);
        cleartags();
        checktags();
        respawn(-1, 100);
    }
    ~gameent()
    {
        removesounds();
        freeeditinfo(edit);
        if(ai) delete ai;
        removetrackedparticles(this);
        removetrackedsounds(this);
    }

    void setparams(bool reset = false)
    {
        int type = clamp(aitype, int(AI_BOT), int(AI_MAX-1));
        if(reset) bspeed = aistyle[type].speed;
        speed = bspeed*curscale;
        xradius = aistyle[type].xradius*curscale;
        yradius = aistyle[type].yradius*curscale;
        zradius = height = aistyle[type].height*curscale;
        weight = aistyle[type].weight*curscale;
        radius = max(xradius, yradius);
        aboveeye = curscale;
    }

    void setscale(float scale = 1, int millis = 0, bool reset = false)
    {
        if(scale != curscale)
        {
            if(state == CS_ALIVE && millis > 0)
                curscale = scale > curscale ? min(curscale+millis/2000.f, scale) : max(curscale-millis/2000.f, scale);
            else curscale = scale;
            setparams(reset);
        }
        else if(reset) setparams(reset);
    }

    int getprojid()
    {
        if(++projid < 2) projid = 2;
        return projid;
    }

    void removesounds()
    {
        if(issound(aschan)) removesound(aschan);
        if(issound(cschan)) removesound(cschan);
        if(issound(vschan)) removesound(vschan);
        if(issound(wschan)) removesound(wschan);
        if(issound(pschan)) removesound(pschan);
        if(issound(fschan)) removesound(fschan);
        if(issound(jschan)) removesound(jschan);
        aschan = cschan = vschan = wschan = pschan = fschan = jschan = -1;
    }

    void stopmoving(bool full)
    {
        if(full) move = strafe = 0;
        loopi(AC_MAX)
        {
            action[i] = false;
            actiontime[i] = 0;
        }
    }

    void clearstate()
    {
        loopi(IM_MAX) impulse[i] = 0;
        cplast = lasthit = lastkill = quake = turnmillis = turnside = 0;
        turnroll = turnyaw = 0;
        lastflag = respawned = suicided = lastnode = -1;
        obit[0] = 0;
        obliterated = false;
        setscale(1, 0, true);
        airnodes.shrink(0);
        icons.shrink(0);
    }

    void respawn(int millis, int heal)
    {
        stopmoving(true);
        removesounds();
        clearstate();
        physent::reset();
        gamestate::respawn(millis, heal);
    }

    void editspawn(int gamemode, int mutators, int sweap = -1, int heal = 0)
    {
        stopmoving(true);
        clearstate();
        inmaterial = timeinair = 0;
        inliquid = onladder = false;
        strafe = move = 0;
        physstate = PHYS_FALL;
        vel = falling = vec(0, 0, 0);
        floor = vec(0, 0, 1);
        resetinterp();
        gamestate::editspawn(gamemode, mutators, sweap, heal);
    }

    void resetstate(int millis, int heal)
    {
        respawn(millis, heal);
        frags = deaths = totaldamage = 0;
    }

    void mapchange(int millis, int heal)
    {
        checkpoint = -1;
        dominating.shrink(0);
        dominated.shrink(0);
        icons.shrink(0);
        resetstate(millis, heal);
        gamestate::mapchange();
    }

    void cleartags() { head = torso = muzzle = origin = eject = waist = jet[0] = jet[1] = jet[2] = vec(-1, -1, -1); }

    vec checkoriginpos()
    {
        if(origin == vec(-1, -1, -1))
        {
            vec dir, right; vecfromyawpitch(yaw, pitch, 1, 0, dir); vecfromyawpitch(yaw, pitch, 0, -1, right);
            dir.mul(radius*0.75f); right.mul(radius*0.85f); dir.z -= height*0.075f;
            origin = vec(o).add(dir).add(right);
        }
        return origin;
    }

    vec originpos(bool melee = false, bool secondary = false)
    {
        if(melee) return secondary ? feetpos() : headpos(height*0.075f);
        return checkoriginpos();
    }

    vec checkmuzzlepos(int weap = -1)
    {
        if(muzzle == vec(-1, -1, -1))
        {
            if(!isweap(weap)) weap = weapselect;
            if(weap == WEAP_SWORD && ((weapstate[weap] == WEAP_S_PRIMARY) || (weapstate[weap] == WEAP_S_SECONDARY)))
            {
                float frac = (lastmillis-weaplast[weap])/float(weapwait[weap]), yx = yaw, px = pitch;
                if(weapstate[weap] == WEAP_S_PRIMARY)
                {
                    yx -= 90;
                    yx += frac*180;
                    if(yx >= 360) yx -= 360;
                    if(yx < 0) yx += 360;
                }
                else
                {
                    px += 90;
                    px -= frac*180;
                    if(px >= 180) px -= 360;
                    if(px < -180) px += 360;
                }
                vec dir;
                vecfromyawpitch(yx, px, 1, 0, dir);
                muzzle = vec(originpos()).add(dir.mul(8));
            }
            else
            {
                vec dir, right; vecfromyawpitch(yaw, pitch, 1, 0, dir); vecfromyawpitch(yaw, pitch, 0, -1, right);
                dir.mul(radius*1.25f); right.mul(radius*0.75f); dir.z -= height*0.075f;
                muzzle = vec(o).add(dir).add(right);
            }
        }
        return muzzle;
    }

    vec muzzlepos(int weap = -1, bool secondary = false)
    {
        if(isweap(weap)) return checkmuzzlepos(weap);
        return originpos(weap == WEAP_MELEE, secondary);
    }

    vec checkejectpos()
    {
        if(eject == vec(-1, -1, -1)) eject = checkmuzzlepos();
        return eject;
    }

    vec ejectpos(int weap = -1)
    {
        if(isweap(weap)) return checkejectpos();
        return muzzlepos();
    }

    void checkhitboxes()
    {
        float hsize = max(xradius*0.45f, yradius*0.45f);
        if(head == vec(-1, -1, -1))
        {
            torso = head;
            head = o;
            head.z -= hsize*0.35f;
        }
        else head.z += hsize*0.45f;
        hrad = vec(xradius*0.45f, yradius*0.45f, hsize);
        if(torso == vec(-1, -1, -1))
        {
            torso = o;
            torso.z -= height*0.4f;
        }
        torso.z += hsize*0.75f;
        float tsize = (head.z-hrad.z)-torso.z;
        trad = vec(xradius, yradius, tsize);
        float lsize = ((torso.z-trad.z)-(o.z-height))*0.5f;
        legs = torso;
        legs.z -= trad.z+lsize;
        lrad = vec(xradius*0.75f, yradius*0.75f, lsize);
        if(waist == vec(-1, -1, -1))
        {
            vec dir; vecfromyawpitch(yaw, 0, -1, 0, dir);
            dir.mul(radius*1.5f);
            dir.z -= height*0.5f;
            waist = vec(o).add(dir);
        }
        if(jet[0] == vec(-1, -1, -1))
        {
            vec dir; vecfromyawpitch(yaw, 0, -1, -1, dir);
            dir.mul(radius);
            dir.z -= height;
            jet[0] = vec(o).add(dir);
        }
        if(jet[1] == vec(-1, -1, -1))
        {
            vec dir;
            vecfromyawpitch(yaw, 0, -1, 1, dir);
            dir.mul(radius);
            dir.z -= height;
            jet[1] = vec(o).add(dir);
        }
        if(jet[2] == vec(-1, -1, -1))
        {
            vec dir; vecfromyawpitch(yaw, 0, -1, 0, dir);
            dir.mul(radius*1.25f);
            dir.z -= height*0.35f;
            jet[2] = vec(o).add(dir);
        }
    }

    bool wantshitbox() { return type == ENT_PLAYER || (type == ENT_AI && (!isaitype(aitype) || aistyle[aitype].hitbox)); }

    void checktags()
    {
        checkoriginpos();
        checkmuzzlepos();
        checkejectpos();
        if(wantshitbox()) checkhitboxes();
    }

    float calcroll(bool crouch, bool thirdperson = false)
    {
        float r = thirdperson ? 0 : roll, wobble = float(rnd(15)-7)*(float(min(quake, 100))/100.f);
        switch(state)
        {
            case CS_SPECTATOR: case CS_WAITING: r = wobble*0.5f; break;
            case CS_ALIVE: if(crouch) wobble *= 0.5f; r += wobble; break;
            case CS_DEAD: r += wobble; break;
            default: break;
        }
        return r;
    }

    void doimpulse(int cost, int type, int millis)
    {
        impulse[IM_METER] += cost;
        impulse[IM_TIME] = impulse[IM_LAST] = millis;
        if(type == IM_T_DASH) impulse[IM_SLIDE] = millis;
        if(!impulse[IM_JUMP] && type > IM_T_NONE && type < IM_T_WALL) impulse[IM_JUMP] = millis;
        impulse[IM_TYPE] = type;
        impulse[IM_COUNT]++;
        resetphys();
    }

    void resetjump()
    {
        timeinair = turnside = impulse[IM_COUNT] = impulse[IM_TYPE] = impulse[IM_JUMP] = impulse[IM_JETPACK] = 0;
    }

    void resetair()
    {
        resetphys();
        resetjump();
    }

    void resetburning()
    {
        if(issound(fschan)) removesound(fschan);
        fschan = -1;
        lastburn = 0;
    }

    void addicon(int type, int millis, int fade, int value = 0)
    {
        int pos = -1;
        loopv(icons)
        {
            if(icons[i].type == type)
            {
                switch(icons[i].type)
                {
                    case eventicon::CRITICAL: case eventicon::WEAPON:
                    {
                        if(icons[i].value != value) continue;
                        break;
                    }
                    default: break;
                }
                icons[i].length = max(icons[i].fade, fade);
                icons[i].fade = millis-icons[i].millis+fade;
                icons[i].value = value;
                return;
            }
            if(pos < 0 && type >= eventicon::SORTED && icons[i].type > type) pos = i;
        }
        eventicon e;
        e.type = type;
        e.millis = millis;
        e.fade = e.length = fade;
        e.value = value;
        if(pos < 0) icons.add(e);
        else icons.insert(pos, e);
    }

    int colour()
    {
        return aitype >= AI_START && isweap(weapselect) ? weaptype[weapselect].colour : teamtype[team].colour;
    }
};

enum { PRJ_SHOT = 0, PRJ_GIBS, PRJ_DEBRIS, PRJ_EJECT, PRJ_ENT, PRJ_AFFINITY, PRJ_MAX };

struct projent : dynent
{
    vec from, to, norm, inertia;
    int addtime, lifetime, lifemillis, waittime, spawntime, fadetime, lastradial, lasteffect, lastbounce, beenused, extinguish;
    float movement, roll, lifespan, lifesize, minspeed;
    bool local, limited, stuck, escaped, child;
    int projtype, projcollide;
    float elasticity, reflectivity, relativity, waterfric;
    int schan, id, weap, flags, hitflags;
    entitylight light;
    gameent *owner;
    physent *hit, *target;
    const char *mdl;

    projent() : projtype(PRJ_SHOT), id(-1), hitflags(HITFLAG_NONE), owner(NULL), hit(NULL), target(NULL), mdl(NULL) { reset(); }
    ~projent()
    {
        removetrackedparticles(this);
        removetrackedsounds(this);
        if(issound(schan)) removesound(schan);
        schan = -1;
    }

    void reset()
    {
        physent::reset();
        type = ENT_PROJ;
        state = CS_ALIVE;
        norm = vec(0, 0, 1);
        inertia = vec(0, 0, 0);
        addtime = lifetime = lifemillis = waittime = spawntime = fadetime = lastradial = lasteffect = lastbounce = beenused = flags = 0;
        schan = id = weap = -1;
        movement = roll = lifespan = lifesize = minspeed = 0;
        curscale = 1;
        extinguish = 0;
        limited = stuck = escaped = child = false;
        projcollide = BOUNCE_GEOM|BOUNCE_PLAYER;
    }

    bool ready(bool used = true)
    {
        if(owner && (!used || !beenused) && waittime <= 0 && state != CS_DEAD)
            return true;
        return false;
    }
};

namespace client
{
    extern bool demoplayback, sendinfo, sendcrc;
    extern void clearvotes(gameent *d, bool msg = false);
    extern void addmsg(int type, const char *fmt = NULL, ...);
    extern void c2sinfo(bool force = false);
}

namespace physics
{
    extern float gravity, liquidspeed, liquidcurb, floorcurb, aircurb, slidecurb;
    extern int smoothmove, smoothdist, sprintstyle;
    extern bool carryaffinity(gameent *d);
    extern bool secondaryweap(gameent *d, bool zoom = false);
    extern bool allowimpulse(int level = 2);
    extern bool jetpack(physent *d);
    extern bool sprinting(physent *d, bool turn = true);
    extern bool canimpulse(physent *d, int cost = 0, int level = 2);
    extern bool movecamera(physent *pl, const vec &dir, float dist, float stepdist);
    extern void smoothplayer(gameent *d, int res, bool local);
    extern void update();
    extern void reset();
}

namespace projs
{
    extern vector<projent *> projs;

    extern void reset();
    extern void update();
    extern projent *create(const vec &from, const vec &to, bool local, gameent *d, int type, int lifetime, int lifemillis, int waittime, int speed, int id = 0, int weap = -1, int flags = 0, float scale = 1, bool child = false, projent *parent = NULL);
    extern void preload();
    extern void remove(gameent *owner);
    extern void destruct(gameent *d, int id);
    extern void shootv(int weap, int flags, int offset, float scale, vec &from, vector<shotmsg> &shots, gameent *d, bool local);
    extern void drop(gameent *d, int g, int n, int v = -1, bool local = true, int c = 0);
    extern void adddynlights();
    extern void render();
}

namespace weapons
{
    extern int autoreloading;
    extern int slot(gameent *d, int n, bool back = false);
    extern bool weapselect(gameent *d, int weap, bool local = true);
    extern bool weapreload(gameent *d, int weap, int load = -1, int ammo = -1, bool local = true);
    extern void weapdrop(gameent *d, int w = -1);
    extern void checkweapons(gameent *d);
    extern float accmod(gameent *d, bool zooming, int *x = NULL);
    extern bool doshot(gameent *d, vec &targ, int weap, bool pressed = false, bool secondary = false, int force = 0);
    extern void shoot(gameent *d, vec &targ, int force = 0);
    extern void preload();
}

namespace hud
{
    extern char *conopentex, *playertex, *deadtex, *dominatingtex, *dominatedtex, *inputtex, *bliptex, *cardtex, *flagtex, *bombtex, *arrowtex, *alerttex, *inventorytex, *indicatortex, *crosshairtex, *hitcrosshairtex,
                *spree1tex, *spree2tex, *spree3tex, *spree4tex, *multi1tex, *multi2tex, *multi3tex, *headshottex, *criticaltex, *dominatetex, *revengetex, *firstbloodtex;
    extern int hudwidth, hudheight, hudsize, lastteam, lastnewgame, damageresidue, damageresiduefade, shownotices, radaraffinitynames, inventorygame, inventoryaffinity, teamkillnum;
    extern float noticescale, inventoryblend, inventoryskew, radaraffinityblend, radarblipblend, radaraffinitysize;
    extern vector<int> teamkills;
    extern const char *icontex(int type, int value);
    extern bool chkcond(int val, bool cond);
    extern char *timetostr(int dur, int style = 0);
    extern void drawquad(float x, float y, float w, float h, float tx1 = 0, float ty1 = 0, float tx2 = 1, float ty2 = 1);
    extern void drawtex(float x, float y, float w, float h, float tx = 0, float ty = 0, float tw = 1, float th = 1);
    extern void drawsized(float x, float y, float s);
    extern void colourskew(float &r, float &g, float &b, float skew = 1);
    extern void skewcolour(float &r, float &g, float &b, bool t = false);
    extern void skewcolour(int &r, int &g, int &b, bool t = false);
    extern void drawindicator(int weap, int x, int y, int s);
    extern void drawclip(int weap, int x, int y, float s);
    extern void drawpointertex(const char *tex, int x, int y, int s, float r = 1, float g = 1, float b = 1, float fade = 1);
    extern void drawpointer(int w, int h, int index);
    extern int numteamkills();
    extern float radarrange();
    extern void drawblip(const char *tex, float area, int w, int h, float s, float blend, vec &dir, float r = 1, float g = 1, float b = 1, const char *font = "sub", const char *text = NULL, ...);
    extern int drawprogress(int x, int y, float start, float length, float size, bool left, float r = 1, float g = 1, float b = 1, float fade = 1, float skew = 1, const char *font = NULL, const char *text = NULL, ...);
    extern int drawitem(const char *tex, int x, int y, float size, bool left = false, float r = 1, float g = 1, float b = 1, float fade = 1, float skew = 1, const char *font = NULL, const char *text = NULL, ...);
    extern int drawitemsubtext(int x, int y, float size, int align = TEXT_RIGHT_UP, float skew = 1, const char *font = NULL, float blend = 1, const char *text = NULL, ...);
    extern int drawweapons(int x, int y, int s, float blend = 1);
    extern int drawhealth(int x, int y, int s, float blend = 1);
    extern void drawinventory(int w, int h, int edge, float blend = 1);
    extern void damage(int n, const vec &loc, gameent *actor, int weap, int flags);
    extern const char *teamtex(int team = TEAM_NEUTRAL);
    extern const char *itemtex(int type, int stype);
    extern bool canshowscores();
    extern void showscores(bool on, bool interm = false, bool onauto = true, bool ispress = false);
    extern score &teamscore(int team);
    extern void resetscores();
    extern int trialinventory(int x, int y, int s, float blend);
    extern int drawinventory(int x, int y, int s, int m, float blend);
}

namespace game
{
    extern int numplayers, gamemode, mutators, nextmode, nextmuts, timeremaining, maptime,
            lastzoom, lasttvcam, lasttvchg, spectvtime, waittvtime, showplayerinfo,
            bloodfade, bloodsize, bloodsparks, debrisfade, eventiconfade, eventiconshort,
            announcefilter, dynlighteffects, aboveheadnames, thirdpersonfollow;
    extern float bloodscale, debrisscale;
    extern bool intermission, zooming;
    extern vec swaypush, swaydir;
    extern string clientmap;

    extern gameent *player1, *focus;
    extern vector<gameent *> players;

    struct avatarent : dynent
    {
        avatarent() { type = ENT_CAMERA; }
    };
    extern avatarent avatarmodel;

    struct camstate
    {
        int ent, idx;
        vec pos, dir;
        vector<int> cansee;
        float mindist, maxdist, score;

        camstate() : idx(-1), mindist(4), maxdist(4096), score(0) { reset(); }
        ~camstate() {}

        void reset()
        {
            cansee.shrink(0);
            dir = vec(0, 0, 0);
            score = 0;
        }

        static int camsort(const camstate *a, const camstate *b)
        {
            if(a->score < b->score) return -1;
            if(a->score > b->score) return 1;
            return 0;
        }
    };
    extern vector<camstate> cameras;

    extern gameent *newclient(int cn);
    extern gameent *getclient(int cn);
    extern gameent *intersectclosest(vec &from, vec &to, gameent *at);
    extern void clientdisconnected(int cn, int reason = DISC_NONE);
    extern char *colorname(gameent *d, char *name = NULL, const char *prefix = "", bool team = true, bool dupname = true);
    extern void announce(int idx, int targ, gameent *d, const char *msg, ...);
    extern void respawn(gameent *d);
    extern void impulseeffect(gameent *d, int effect = 0);
    extern void suicide(gameent *d, int flags);
    extern void fixrange(float &yaw, float &pitch);
    extern void fixfullrange(float &yaw, float &pitch, float &roll, bool full);
    extern void getyawpitch(const vec &from, const vec &pos, float &yaw, float &pitch);
    extern void scaleyawpitch(float &yaw, float &pitch, float targyaw, float targpitch, float frame = 1, float scale = 1);
    extern bool allowmove(physent *d);
    extern int mousestyle();
    extern int deadzone();
    extern int panspeed();
    extern void checkzoom();
    extern bool inzoom();
    extern bool inzoomswitch();
    extern void zoomview(bool down);
    extern bool tvmode();
    extern void resetcamera();
    extern void resetworld();
    extern void resetstate();
    extern void hiteffect(int weap, int flags, int damage, gameent *d, gameent *actor, vec &dir, bool local = false);
    extern void damaged(int weap, int flags, int damage, int health, gameent *d, gameent *actor, int millis, vec &dir);
    extern void killed(int weap, int flags, int damage, gameent *d, gameent *actor, vector<gameent*> &log, int style);
    extern void timeupdate(int timeremain);
}

namespace entities
{
    extern int showentdescs;
    extern vector<extentity *> ents;
    extern int lastenttype[MAXENTTYPES], lastusetype[EU_MAX];
    extern void clearentcache();
    extern int closestent(int type, const vec &pos, float mindist, bool links = false);
    extern bool collateitems(gameent *d, vector<actitem> &actitems);
    extern void checkitems(gameent *d);
    extern void putitems(packetbuf &p);
    extern void execlink(gameent *d, int index, bool local, int ignore = -1);
    extern void setspawn(int n, int m);
    extern bool tryspawn(dynent *d, const vec &o, short yaw = 0, short pitch = 0);
    extern void spawnplayer(gameent *d, int ent = -1, bool suicide = false);
    extern const char *entinfo(int type, attrvector &attr, bool full = false);
    extern void useeffects(gameent *d, int n, int c, bool s, int g, int r, int v = -1);
    extern const char *entmdlname(int type, attrvector &attr);
    extern bool clipped(const vec &o, bool aiclip = false);
    extern void edittoggled(bool edit);
    extern const char *findname(int type);
    extern void adddynlights();
    extern void render();
    extern void update();
    struct avoidset
    {
        struct obstacle
        {
            dynent *ent;
            int numentities;
            float above;

            obstacle(dynent *ent) : ent(ent), numentities(0), above(-1) {}
        };

        vector<obstacle> obstacles;
        vector<int> entities;

        void clear()
        {
            obstacles.setsize(0);
            entities.setsize(0);
        }

        void add(dynent *ent)
        {
            obstacle &ob = obstacles.add(obstacle(ent));
            if(!ent) ob.above = enttype[WAYPOINT].radius;
            else switch(ent->type)
            {
                case ENT_PLAYER: case ENT_AI:
                {
                    gameent *e = (gameent *)ent;
                    ob.above = e->abovehead().z;
                    break;
                }
                case ENT_PROJ:
                {
                    projent *p = (projent *)ob.ent;
                    if(p->projtype == PRJ_SHOT && WEAP2(p->weap, explode, p->flags&HIT_ALT))
                        ob.above = p->o.z+(WEAP2(p->weap, explode, p->flags&HIT_ALT)*p->lifesize)+1.f;
                    break;
                }
            }
        }

        void add(dynent *ent, int entity)
        {
            if(obstacles.empty() || ent != obstacles.last().ent) add(ent);
            obstacles.last().numentities++;
            entities.add(entity);
        }

        void add(avoidset &avoid)
        {
            entities.put(avoid.entities.getbuf(), avoid.entities.length());
            loopv(avoid.obstacles)
            {
                obstacle &o = avoid.obstacles[i];
                if(obstacles.empty() || o.ent != obstacles.last().ent) add(o.ent, o.above);
                obstacles.last().numentities += o.numentities;
            }
        }

        void avoidnear(dynent *d, const vec &pos, float limit);

        #define loopavoid(v, d, body) \
            if(!(v).obstacles.empty()) \
            { \
                int cur = 0; \
                loopv((v).obstacles) \
                { \
                    const entities::avoidset::obstacle &ob = (v).obstacles[i]; \
                    int next = cur + ob.numentities; \
                    if(!ob.ent || ob.ent != (d)) \
                    { \
                        for(; cur < next; cur++) if((v).entities.inrange(cur)) \
                        { \
                            int ent = (v).entities[cur]; \
                            body; \
                        } \
                    } \
                    cur = next; \
                } \
            }

        bool find(int entity, gameent *d) const
        {
            loopavoid(*this, d, { if(ent == entity) return true; });
            return false;
        }

        int remap(gameent *d, int n, vec &pos, bool retry = false)
        {
            if(!obstacles.empty())
            {
                int cur = 0;
                loopv(obstacles)
                {
                    obstacle &ob = obstacles[i];
                    int next = cur + ob.numentities;
                    if(!ob.ent || ob.ent != d || (d->blocked && ob.ent == d))
                    {
                        for(; cur < next; cur++) if(entities[cur] == n)
                        {
                            if(ob.above < 0) return retry ? n : -1;
                            vec above(pos.x, pos.y, ob.above);
                            if(above.z-d->o.z >= ai::JUMPMAX)
                                return retry ? n : -1; // too much scotty
                            int node = closestent(WAYPOINT, above, ai::CLOSEDIST, true);
                            if(ents.inrange(node) && node != n)
                            { // try to reroute above their head?
                                if(!find(node, d))
                                {
                                    pos = ents[node]->o;
                                    return node;
                                }
                                else return retry ? n : -1;
                            }
                            else
                            {
                                vec old = d->o;
                                d->o = vec(above).add(vec(0, 0, d->height));
                                bool col = collide(d, vec(0, 0, 1));
                                d->o = old;
                                if(col)
                                {
                                    pos = above;
                                    return n;
                                }
                                else return retry ? n : -1;
                            }
                        }
                    }
                    cur = next;
                }
            }
            return n;
        }
    };
    extern void findentswithin(int type, const vec &pos, float mindist, float maxdist, vector<int> &results);
    extern float route(int node, int goal, vector<int> &route, const avoidset &obstacles, gameent *d = NULL, bool retry = false);
}
#elif defined(GAMESERVER)
namespace client
{
    extern const char *getname();
    extern bool sendcmd(int nargs, const char *cmd, const char *arg);
}
#endif
#include "capture.h"
#include "defend.h"
#include "bomber.h"

#endif

