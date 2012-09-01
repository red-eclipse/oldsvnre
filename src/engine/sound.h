enum
{
    S_GUIPRESS = 0, S_GUIBACK, S_GUIACT,
    S_GAMESPECIFIC
};

enum
{
    SND_NONE    = 0,
    SND_NOATTEN = 1<<0, // disable attenuation
    SND_NODELAY = 1<<1, // disable delay
    SND_NOCULL  = 1<<2, // disable culling
    SND_NOPAN   = 1<<3, // disable panning (distance only attenuation)
    SND_NODIST  = 1<<4, // disable distance (panning only)
    SND_NOQUIET = 1<<5, // disable water effects (panning only)
    SND_CLAMPED = 1<<6, // makes volume the minimum volume to clamp to
    SND_LOOP    = 1<<7,
    SND_MAP     = 1<<8,
    SND_FORCED  = SND_NOATTEN|SND_NODELAY|SND_NOCULL|SND_NOQUIET,
    SND_DIRECT  = SND_NODIST|SND_NODELAY|SND_NOCULL|SND_NOQUIET|SND_CLAMPED,
    SND_MASKF   = SND_LOOP|SND_MAP,
    SND_LAST    = 7
};

#ifndef STANDALONE
#define SOUNDMINDIST        16.0f
#define SOUNDMAXDIST        10000.f

struct soundsample;

struct soundslot
{
    vector<soundsample *> samples;
    int vol, maxrad, minrad;
    char *name;

    soundslot();
    ~soundslot();
};

struct sound
{
    soundslot *slot;
    vec pos, oldpos;
    physent *owner;
    int vol, curvol, curpan;
    int flags, maxrad, minrad, material;
    int millis, ends, slotnum, chan, *hook;

    sound();
    ~sound();

    void reset();
    bool playing();
    bool valid() { return chan >= 0 && playing(); }
};

extern bool nosound;
extern int mastervol, soundvol, musicvol;
extern vector<soundslot> gamesounds, mapsounds;
extern vector<sound> sounds;

#define issound(c) (sounds.inrange(c) && sounds[c].valid())

extern void initsound();
extern void stopsound();
extern bool playmusic(const char *name, const char *cmd = NULL);
extern bool playingmusic(bool check = true);
extern void smartmusic(bool cond, bool autooff);
extern void musicdone(bool docmd);
extern void updatesounds();
extern int addsound(const char *name, int vol, int maxrad, int minrad, int value, vector<soundslot> &sounds);
extern void removesound(int c);
extern void clearsound();
extern int playsound(int n, const vec &pos, physent *d = NULL, int flags = 0, int vol = -1, int maxrad = -1, int minrad = -1, int *hook = NULL, int ends = 0, int *oldhook = NULL);
extern void removetrackedsounds(physent *d);

extern void initmumble();
extern void closemumble();
extern void updatemumble();
#endif
