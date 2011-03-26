#include "engine.h"

hashtable<const char *, soundsample> soundsamples;
vector<soundslot> gamesounds, mapsounds;
vector<sound> sounds;

bool nosound = true, changedvol = false;
Mix_Music *music = NULL;
SDL_RWops *musicrw = NULL;
stream *musicstream = NULL;
char *musicfile = NULL, *musicdonecmd = NULL;
int soundsatonce = 0, lastsoundmillis = 0, musictime = -1;

VARF(IDF_PERSIST, mastervol, 0, 255, 255, changedvol = true);
VAR(IDF_PERSIST, soundvol, 0, 255, 255);
VARF(0, soundmono, 0, 0, 1, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
VARF(0, soundchans, 1, 32, 128, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
VARF(0, soundfreq, 0, 44100, 48000, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
VARF(0, soundbufferlen, 128, 1024, INT_MAX-1, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));

VARF(IDF_PERSIST, musicvol, 0, 128, 255, changedvol = true);
VAR(IDF_PERSIST, musicfadein, 0, 250, INT_MAX-1);
VAR(IDF_PERSIST, musicfadeout, 0, 5000, INT_MAX-1);
SVAR(0, titlemusic, "sounds/theme");

void initsound()
{
    if(nosound)
    {
        if(Mix_OpenAudio(soundfreq, MIX_DEFAULT_FORMAT, soundmono ? 1 : 2, soundbufferlen) == -1)
        {
            conoutf("\frsound initialisation failed: %s", Mix_GetError());
            return;
        }

        Mix_AllocateChannels(soundchans);
        nosound = false;
    }
    initmumble();
}

void stopmusic(bool docmd)
{
    if(Mix_PlayingMusic()) Mix_HaltMusic();
    if(music)
    {
        Mix_FreeMusic(music);
        music = NULL;
    }
    if(musicrw) { SDL_FreeRW(musicrw); musicrw = NULL; }
    DELETEP(musicstream);
    DELETEA(musicfile);
    if(musicdonecmd != NULL)
    {
        char *cmd = musicdonecmd;
        musicdonecmd = NULL;
        if(docmd) execute(cmd);
        delete[] cmd;
    }
}

void musicdone(bool docmd)
{
    if(musicfadeout && !docmd) Mix_FadeOutMusic(musicfadeout);
    else stopmusic(docmd);
}

void stopsound()
{
    if(nosound) return;
    Mix_HaltChannel(-1);
    nosound = true;
    stopmusic(false);
    clearsound();
    soundsamples.clear();
    gamesounds.setsize(0);
    closemumble();
    Mix_CloseAudio();
}

void removesound(int c)
{
    Mix_HaltChannel(c);
    sounds[c].reset();
}

void clearsound()
{
    loopv(sounds) removesound(i);
    mapsounds.setsize(0);
}

Mix_Music *loadmusic(const char *name)
{
    if(!musicstream) musicstream = openzipfile(name, "rb");
    if(musicstream)
    {
        if(!musicrw) musicrw = musicstream->rwops();
        if(!musicrw) DELETEP(musicstream);
    }
    if(musicrw) music = Mix_LoadMUS_RW(musicrw);
    else music = Mix_LoadMUS(findfile(name, "rb"));
    if(!music)
    {
        if(musicrw) { SDL_FreeRW(musicrw); musicrw = NULL; }
        DELETEP(musicstream);
    }
    return music;
}

void playmusic(const char *name, const char *cmd)
{
    if(nosound) return;

    stopmusic(false);

    if(*name)
    {
        string buf;
        const char *dirs[] = { "", "sounds/" }, *exts[] = { "", ".wav", ".ogg" };
        bool found = false;
        loopi(sizeof(dirs)/sizeof(dirs[0]))
        {
            loopk(sizeof(exts)/sizeof(exts[0]))
            {
                formatstring(buf)("%s%s%s", dirs[i], name, exts[k]);
                if(loadmusic(buf))
                {
                    DELETEA(musicfile);
                    DELETEA(musicdonecmd);
                    musicfile = newstring(name);
                    if(cmd[0]) musicdonecmd = newstring(cmd);
                    if(musicfadein) Mix_FadeInMusic(music, cmd[0] ? 0 : -1, musicfadein);
                    else Mix_PlayMusic(music, cmd[0] ? 0 : -1);
                    Mix_VolumeMusic(int((mastervol/255.f)*(musicvol/255.f)*MIX_MAX_VOLUME));
                    found = true;
                }
                if(found) break;
            }
            if(found) break;
        }
        if(!music) { conoutf("\frcould not play music: %s", name); return; }
    }
}

COMMANDN(0, music, playmusic, "ss");

void smartmusic(bool cond, bool autooff)
{
    if(nosound || !mastervol || !musicvol || (!cond && Mix_PlayingMusic()) || !*titlemusic) return;
    if(!music || !Mix_PlayingMusic() || (cond && strcmp(musicfile, titlemusic))) playmusic(titlemusic, "");
    else if(music && Mix_PlayingMusic()) Mix_VolumeMusic(int((mastervol/255.f)*(musicvol/255.f)*MIX_MAX_VOLUME));
}
ICOMMAND(0, smartmusic, "ii", (int *a, int *b), smartmusic(*a, *b));

int findsound(const char *name, int vol, vector<soundslot> &sounds)
{
    loopv(sounds)
        if(!strcmp(sounds[i].sample->name, name) && (!vol || sounds[i].vol == vol)) return i;
    return -1;
}

static Mix_Chunk *loadwav(const char *name)
{
    Mix_Chunk *c = NULL;
    stream *z = openzipfile(name, "rb");
    if(z)
    {
        SDL_RWops *rw = z->rwops();
        if(rw)
        {
            c = Mix_LoadWAV_RW(rw, 0);
            SDL_FreeRW(rw);
        }
        delete z;
    }
    if(!c) c = Mix_LoadWAV(findfile(name, "rb"));
    return c;
}

int addsound(const char *name, int vol, int maxrad, int minrad, bool unique, vector<soundslot> &sounds)
{
    soundsample *sample = soundsamples.access(name);
    if(!sample)
    {
        char *n = newstring(name);
        sample = &soundsamples[n];
        sample->name = n;
        sample->sound = NULL;
    }
    if(!sample->sound)
    {
        string buf;
        const char *dirs[] = { "", "sounds/" }, *exts[] = { "", ".wav", ".ogg" };
        bool found = false;
        loopi(sizeof(dirs)/sizeof(dirs[0]))
        {
            loopk(sizeof(exts)/sizeof(exts[0]))
            {
                formatstring(buf)("%s%s%s", dirs[i], sample->name, exts[k]);
                if((sample->sound = loadwav(buf)) != NULL) found = true;
                if(found) break;
            }
            if(found) break;
        }

        if(!sample->sound) { conoutf("\frfailed to load sample: %s", sample->name); return -1; }
    }
    if(vol <= 0 || vol >= 255) vol = 255;
    if(maxrad <= 0) maxrad = -1;
    if(minrad < 0) minrad = -1;
    if(unique)
    {
        loopv(sounds)
        {
            soundslot &slot = sounds[i];
            if(slot.sample == sample && slot.vol == vol && slot.maxrad == maxrad && slot.minrad == minrad) return i;
        }
    }
    soundslot &slot = sounds.add();
    slot.sample = sample;
    slot.vol = vol;
    slot.maxrad = maxrad; // use these values if none are supplied when playing
    slot.minrad = minrad;
    return sounds.length()-1;
}

ICOMMAND(0, registersound, "sissi", (char *n, int *v, char *w, char *x, int *u), intret(addsound(n, *v, *w ? parseint(w) : -1, *x ? parseint(x) : -1, *u > 0, gamesounds)));
ICOMMAND(0, mapsound, "sissi", (char *n, int *v, char *w, char *x, int *u), intret(addsound(n, *v, *w ? parseint(w) : -1, *x ? parseint(x) : -1, *u > 0, mapsounds)));

void calcvol(int flags, int vol, int slotvol, int maxrad, int minrad, const vec &pos, int *curvol, int *curpan, bool liquid)
{
    int svol = flags&SND_CLAMPED ? 255 : clamp(vol, 0, 255), span = 127; vec v; float dist = pos.dist(camera1->o, v);
    if(!(flags&SND_NOATTEN) && dist > 0)
    {
        if(!(flags&SND_NOPAN) && !soundmono && (v.x != 0 || v.y != 0))
        {
            v.rotate_around_z(-camera1->yaw*RAD);
            span = int(255.9f*(0.5f - 0.5f*v.x/v.magnitude2())); // range is from 0 (left) to 255 (right)
        }
        if(!(flags&SND_NODIST))
        {
            float mrad = maxrad > 0 ? maxrad : 256, nrad = minrad > 0 ? (minrad <= mrad ? minrad : mrad) : 0;
            if(dist > nrad)
            {
                if(dist <= mrad) svol = int(svol*(1.f-((dist-nrad)/max(mrad-nrad,1e-16f))));
                else svol = 0;
            }
        }
    }
    if(!(flags&SND_NOQUIET) && svol > 0 && liquid) svol = int(svol*0.65f);
    if(flags&SND_CLAMPED) svol = max(svol, clamp(vol, 0, 255));
    *curvol = clamp(int((mastervol/255.f)*(soundvol/255.f)*(slotvol/255.f)*(svol/255.f)*MIX_MAX_VOLUME), 0, MIX_MAX_VOLUME);
    *curpan = span;
}

void updatesound(int chan)
{
    sound &s = sounds[chan];
    bool waiting = (!(s.flags&SND_NODELAY) && Mix_Paused(chan));
    if((s.flags&SND_NOCULL) || s.curvol > 0)
    {
        if(waiting)
        { // delay the sound based on average physical constants
            bool liquid = isliquid(lookupmaterial(s.pos)&MATF_VOLUME) || isliquid(lookupmaterial(camera1->o)&MATF_VOLUME);
            float dist = camera1->o.dist(s.pos);
            int delay = int((dist/8.f)*(liquid ? 1.5f : 0.35f));
            if(lastmillis >= s.millis+delay)
            {
                Mix_Resume(chan);
                waiting = false;
            }
        }
        if(!waiting)
        {
            Mix_Volume(chan, s.curvol);
            SDL_LockAudio(); // workaround for race condition in inside Mix_SetPanning
            Mix_SetPanning(chan, 255-s.curpan, s.curpan);
            SDL_UnlockAudio();
        }
    }
    else
    {
        removesound(chan);
        if(verbose >= 4) conoutf("culled sound %d (%d)", chan, s.curvol);
    }
}

void updatesounds()
{
    updatemumble();
    if(nosound) return;
    bool liquid = isliquid(lookupmaterial(camera1->o)&MATF_VOLUME);
    loopv(sounds) if(sounds[i].chan >= 0)
    {
        sound &s = sounds[i];
        if((!s.ends || lastmillis < s.ends) && Mix_Playing(sounds[i].chan))
        {
            if(s.owner) s.pos = s.owner->o;
            if(s.pos != s.oldpos) s.material = lookupmaterial(s.pos);
            calcvol(s.flags, s.vol, s.slot->vol, s.maxrad, s.minrad, s.pos, &s.curvol, &s.curpan, liquid || isliquid(s.material&MATF_VOLUME));
            s.oldpos = s.pos;
            updatesound(i);
        }
        else removesound(i);
    }
    if(music || Mix_PlayingMusic())
    {
        if(nosound || !mastervol || !musicvol) stopmusic(false);
        else if(!Mix_PlayingMusic()) musicdone(true);
        else if(changedvol)
        {
            Mix_VolumeMusic(int((mastervol/255.f)*(musicvol/255.f)*MIX_MAX_VOLUME));
            changedvol = false;
        }
    }
}

int playsound(int n, const vec &pos, physent *d, int flags, int vol, int maxrad, int minrad, int *hook, int ends, int *oldhook)
{
    if(nosound || !mastervol || !soundvol) return -1;

    vector<soundslot> &soundset = flags&SND_MAP ? mapsounds : gamesounds;

    if(soundset.inrange(n) && soundset[n].sample->sound)
    {
        soundslot *slot = &soundset[n];
        if(!oldhook || !issound(*oldhook) || (n != sounds[*oldhook].slotnum && strcmp(slot->sample->name, gamesounds[sounds[*oldhook].slotnum].sample->name)))
            oldhook = NULL;

        int cvol = 0, cpan = 0, v = vol > 0 && vol < 256 ? vol : (flags&SND_CLAMPED ? 64 : 255),
            x = maxrad > 0 ? maxrad : (flags&SND_CLAMPED ? getworldsize() : (slot->maxrad > 0 ? slot->maxrad : 256)),
            y = minrad >= 0 ? minrad : (flags&SND_CLAMPED ? 32 : (slot->minrad >= 0 ? slot->minrad : 0)),
            mat = lookupmaterial(pos);

        bool liquid = isliquid(lookupmaterial(camera1->o)&MATF_VOLUME);
        calcvol(flags, v, slot->vol, x, y, pos, &cvol, &cpan, liquid || isliquid(mat&MATF_VOLUME));

        if((flags&SND_NOCULL) || cvol > 0)
        {
            int chan = -1;
            if(oldhook) chan = *oldhook;
            else
            {
                oldhook = NULL;
                if((chan = Mix_PlayChannel(-1, soundset[n].sample->sound, flags&SND_LOOP ? -1 : 0)) < 0)
                {
                    int lowest = -1;
                    loopv(sounds) if(sounds[i].chan >= 0 && !(sounds[i].flags&SND_NOCULL) && !(sounds[i].flags&SND_MAP))
                        if(((flags&SND_NOCULL) || sounds[i].vol < cvol) && (!sounds.inrange(lowest) || sounds[i].vol < sounds[lowest].vol))
                            lowest = i;
                    if(sounds.inrange(lowest))
                    {
                        removesound(lowest);
                        chan = Mix_PlayChannel(-1, soundset[n].sample->sound, flags&SND_LOOP ? -1 : 0);
                        if(verbose >= 4) conoutf("culled channel %d (%d)", lowest, sounds[lowest].vol);
                    }
                }
            }
            if(chan >= 0)
            {
                if(!oldhook && !(flags&SND_NODELAY)) Mix_Pause(chan);

                while(chan >= sounds.length()) sounds.add();

                sound &s = sounds[chan];
                s.slot = slot;
                s.vol = v;
                s.maxrad = x;
                s.minrad = y;
                s.material = mat;
                s.flags = flags;
                s.millis = oldhook ? sounds[*oldhook].millis : lastmillis;
                s.ends = ends;
                s.slotnum = n;
                s.owner = d;
                s.pos = s.oldpos = pos;
                s.curvol = cvol;
                s.curpan = cpan;
                s.chan = chan;
                if(hook)
                {
                    if(issound(*hook) && (!oldhook || *hook != *oldhook)) removesound(*hook);
                    *hook = s.chan;
                    s.hook = hook;
                }
                else s.hook = NULL;
                if(oldhook) *oldhook = -1;
                updatesound(chan);
                return chan;
            }
            else if(verbose >= 2)
                conoutf("\frcannot play sound %d (%s): %s", n, soundset[n].sample->name, Mix_GetError());
        }
        else if(verbose >= 4) conoutf("culled sound %d (%d)", n, cvol);
    }
    else if(n > 0) conoutf("\frunregistered sound: %d", n);
    if(oldhook && issound(*oldhook)) removesound(*oldhook);
    return -1;
}

void sound(int *n, int *vol)
{
    intret(playsound(*n, camera1->o, camera1, SND_FORCED, *vol ? *vol : -1));
}
COMMAND(0, sound, "ii");

void removetrackedsounds(physent *d)
{
    loopv(sounds)
        if(sounds[i].chan >= 0 && sounds[i].owner == d)
            removesound(i);
}

void resetsound()
{
    const SDL_version *v = Mix_Linked_Version();
    if(SDL_VERSIONNUM(v->major, v->minor, v->patch) <= SDL_VERSIONNUM(1, 2, 8))
    {
        conoutf("\frsound reset not available in-game due to SDL_mixer-1.2.8 bug, please restart for changes to take effect.");
        return;
    }
    clearchanges(CHANGE_SOUND);
    if(!nosound)
    {
        loopv(sounds) removesound(i);
        enumerate(soundsamples, soundsample, s, { Mix_FreeChunk(s.sound); s.sound = NULL; });
        if(music)
        {
            Mix_HaltMusic();
            Mix_FreeMusic(music);
        }
        if(musicstream) musicstream->seek(0, SEEK_SET);
        Mix_CloseAudio();
        nosound = true;
    }
    initsound();
    if(nosound)
    {
        DELETEA(musicfile);
        DELETEA(musicdonecmd);
        music = NULL;
        gamesounds.setsize(0);
        mapsounds.setsize(0);
        soundsamples.clear();
        return;
    }
    if(music && loadmusic(musicfile))
    {
        if(musicfadein) Mix_FadeInMusic(music, musicdonecmd ? 0 : -1, musicfadein);
        else Mix_PlayMusic(music, musicdonecmd ? 0 : -1);
        Mix_VolumeMusic(int((mastervol/255.f)*(musicvol/255.f)*MIX_MAX_VOLUME));
    }
    else
    {
        DELETEA(musicfile);
        DELETEA(musicdonecmd);
    }
}

COMMAND(0, resetsound, "");

#ifdef WIN32

#include <wchar.h>

#else

#include <unistd.h>

#ifdef _POSIX_SHARED_MEMORY_OBJECTS
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <wchar.h>
#endif

#endif

#if defined(WIN32) || defined(_POSIX_SHARED_MEMORY_OBJECTS)
struct MumbleInfo
{
    int version, timestamp;
    vec pos, front, top;
    wchar_t name[256];
};
#endif

#ifdef WIN32
static HANDLE mumblelink = NULL;
static MumbleInfo *mumbleinfo = NULL;
#define VALID_MUMBLELINK (mumblelink && mumbleinfo)
#elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
static int mumblelink = -1;
static MumbleInfo *mumbleinfo = (MumbleInfo *)-1;
#define VALID_MUMBLELINK (mumblelink >= 0 && mumbleinfo != (MumbleInfo *)-1)
#endif

#ifdef VALID_MUMBLELINK
VARF(IDF_PERSIST, mumble, 0, 1, 1, { if(mumble) initmumble(); else closemumble(); });
#else
VARF(IDF_PERSIST, mumble, 0, 0, 1, { if(mumble) initmumble(); else closemumble(); });
#endif

void initmumble()
{
    if(!mumble) return;
#ifdef VALID_MUMBLELINK
    if(VALID_MUMBLELINK) return;

    #ifdef WIN32
        mumblelink = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "MumbleLink");
        if(mumblelink)
        {
            mumbleinfo = (MumbleInfo *)MapViewOfFile(mumblelink, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MumbleInfo));
            if(mumbleinfo) wcsncpy(mumbleinfo->name, L"redeclipse", 256);
        }
    #elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
        defformatstring(shmname)("/MumbleLink.%d", getuid());
        mumblelink = shm_open(shmname, O_RDWR, 0);
        if(mumblelink >= 0)
        {
            mumbleinfo = (MumbleInfo *)mmap(NULL, sizeof(MumbleInfo), PROT_READ|PROT_WRITE, MAP_SHARED, mumblelink, 0);
            if(mumbleinfo != (MumbleInfo *)-1) wcsncpy(mumbleinfo->name, L"redeclipse", 256);
        }
    #endif
    if(!VALID_MUMBLELINK) closemumble();
#else
    conoutf(CON_ERROR, "Mumble positional audio is not available on this platform.");
#endif
}

void closemumble()
{
#ifdef WIN32
    if(mumbleinfo) { UnmapViewOfFile(mumbleinfo); mumbleinfo = NULL; }
    if(mumblelink) { CloseHandle(mumblelink); mumblelink = NULL; }
#elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
    if(mumbleinfo != (MumbleInfo *)-1) { munmap(mumbleinfo, sizeof(MumbleInfo)); mumbleinfo = (MumbleInfo *)-1; }
    if(mumblelink >= 0) { close(mumblelink); mumblelink = -1; }
#endif
}

static inline vec mumblevec(const vec &v, bool pos = false)
{
    // change from X left, Z up, Y forward to X right, Y up, Z forward
    // 8 cube units = 1 meter
    vec m(-v.x, v.z, v.y);
    if(pos) m.div(8);
    return m;
}

void updatemumble()
{
#ifdef VALID_MUMBLELINK
    if(!VALID_MUMBLELINK) return;

    static int timestamp = 0;

    mumbleinfo->version = 1;
    mumbleinfo->timestamp = ++timestamp;

    mumbleinfo->pos = mumblevec(camera1->o, true);
    mumbleinfo->front = mumblevec(vec(RAD*camera1->yaw, RAD*camera1->pitch));
    mumbleinfo->top = mumblevec(vec(RAD*camera1->yaw, RAD*(camera1->pitch+90)));
#endif
}

