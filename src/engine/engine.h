#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "cube.h"

#define RE_VER_MAJOR        1
#define RE_VER_MINOR        3
#define RE_VER_PATCH        1
#define RE_VER_STR          "1.3.1"
#define RE_VER_MAKE(a,b,c)  (((a)<<16) | ((b)<<8) | (c))
#define RE_VER              RE_VER_MAKE(RE_VER_MAJOR, RE_VER_MINOR, RE_VER_PATCH)
#define RE_VERSION          (RE_VER_MAJOR*100)+(RE_VER_MINOR*10)+RE_VER_PATCH

#define RE_NAME             "Red Eclipse"
#define RE_UNAME            "redeclipse"
#define RE_RELEASE          "Galactic"
#define RE_URL              "www.redeclipse.net"
#ifdef WIN32
#define RE_PLATFORM         "win"
#elif defined(__APPLE__)
#define RE_PLATFORM         "mac"
#else
#define RE_PLATFORM         "nix"
#endif
#define RE_ARCH             (8*sizeof(void *))

#define RE_LAN_PORT         28799
#define RE_MASTER_PORT      28800
#define RE_SERVER_PORT      28801
#define RE_MASTER_HOST      "play.redeclipse.net"

#ifdef STANDALONE
extern void setupmaster();
extern void checkmaster();
extern void cleanupmaster();
extern void reloadmaster();
extern int masterserver, masterport;
extern char *masterip;
#endif

#include "irc.h"
#include "sound.h"

enum { CON_DEBUG = 0, CON_MESG, CON_INFO, CON_SELF, CON_GAMESPECIFIC };

enum { PACKAGEDIR_OCTA = 1<<0 };
extern const char *disc_reasons[];
struct ipinfo
{
    enum { ALLOW = 0, BAN, MUTE, LIMIT, MAXTYPES };
    enum { TEMPORARY = 0, LOCAL, GLOBAL };
    enet_uint32 ip, mask;
    int type, flag, time;

    ipinfo() : ip(0), mask(0), type(-1), flag(TEMPORARY), time(-1) {}
    ~ipinfo() {}
};
extern vector<ipinfo> control;
extern const char *ipinfotypes[ipinfo::MAXTYPES];
extern void addipinfo(vector<ipinfo> &info, int type, const char *name);
extern char *printipinfo(const ipinfo &info, char *buf = NULL);
extern bool checkipinfo(vector<ipinfo> &info, int type, enet_uint32 ip);
extern void writecfg();
extern void rehash(bool reload = true);

#ifndef STANDALONE
#include "world.h"
#include "octa.h"
#include "lightmap.h"
#include "bih.h"
#include "texture.h"
#include "model.h"

// GL_ARB_multitexture
extern PFNGLACTIVETEXTUREARBPROC        glActiveTexture_;
extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTexture_;
extern PFNGLMULTITEXCOORD2FARBPROC   glMultiTexCoord2f_;
extern PFNGLMULTITEXCOORD3FARBPROC   glMultiTexCoord3f_;
extern PFNGLMULTITEXCOORD4FARBPROC   glMultiTexCoord4f_;

// GL_ARB_vertex_buffer_object
extern PFNGLGENBUFFERSARBPROC       glGenBuffers_;
extern PFNGLBINDBUFFERARBPROC       glBindBuffer_;
extern PFNGLMAPBUFFERARBPROC        glMapBuffer_;
extern PFNGLUNMAPBUFFERARBPROC      glUnmapBuffer_;
extern PFNGLBUFFERDATAARBPROC       glBufferData_;
extern PFNGLBUFFERSUBDATAARBPROC    glBufferSubData_;
extern PFNGLDELETEBUFFERSARBPROC    glDeleteBuffers_;
extern PFNGLGETBUFFERSUBDATAARBPROC glGetBufferSubData_;

// GL_ARB_occlusion_query
extern PFNGLGENQUERIESARBPROC       glGenQueries_;
extern PFNGLDELETEQUERIESARBPROC     glDeleteQueries_;
extern PFNGLBEGINQUERYARBPROC       glBeginQuery_;
extern PFNGLENDQUERYARBPROC       glEndQuery_;
extern PFNGLGETQUERYIVARBPROC       glGetQueryiv_;
extern PFNGLGETQUERYOBJECTIVARBPROC  glGetQueryObjectiv_;
extern PFNGLGETQUERYOBJECTUIVARBPROC glGetQueryObjectuiv_;

// GL_EXT_framebuffer_object
extern PFNGLBINDRENDERBUFFEREXTPROC     glBindRenderbuffer_;
extern PFNGLDELETERENDERBUFFERSEXTPROC   glDeleteRenderbuffers_;
extern PFNGLGENFRAMEBUFFERSEXTPROC       glGenRenderbuffers_;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC   glRenderbufferStorage_;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC  glCheckFramebufferStatus_;
extern PFNGLBINDFRAMEBUFFEREXTPROC       glBindFramebuffer_;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC     glDeleteFramebuffers_;
extern PFNGLGENFRAMEBUFFERSEXTPROC       glGenFramebuffers_;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2D_;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbuffer_;
extern PFNGLGENERATEMIPMAPEXTPROC         glGenerateMipmap_;

// GL_EXT_framebuffer_blit
#ifndef GL_EXT_framebuffer_blit
#define GL_READ_FRAMEBUFFER_EXT           0x8CA8
#define GL_DRAW_FRAMEBUFFER_EXT           0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_EXT   0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING_EXT   0x8CAA
typedef void (APIENTRYP PFNGLBLITFRAMEBUFFEREXTPROC) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
#endif
extern PFNGLBLITFRAMEBUFFEREXTPROC         glBlitFramebuffer_;

// GL_EXT_draw_range_elements
extern PFNGLDRAWRANGEELEMENTSEXTPROC glDrawRangeElements_;

// GL_EXT_blend_minmax
extern PFNGLBLENDEQUATIONEXTPROC glBlendEquation_;

// GL_EXT_blend_color
extern PFNGLBLENDCOLOREXTPROC glBlendColor_;

// GL_EXT_multi_draw_arrays
extern PFNGLMULTIDRAWARRAYSEXTPROC   glMultiDrawArrays_;
extern PFNGLMULTIDRAWELEMENTSEXTPROC glMultiDrawElements_;

// GL_EXT_packed_depth_stencil
#ifndef GL_DEPTH_STENCIL_EXT
#define GL_DEPTH_STENCIL_EXT 0x84F9
#endif
#ifndef GL_DEPTH24_STENCIL8_EXT
#define GL_DEPTH24_STENCIL8_EXT 0x88F0
#endif

// GL_ARB_texture_compression
extern PFNGLCOMPRESSEDTEXIMAGE3DARBPROC    glCompressedTexImage3D_;
extern PFNGLCOMPRESSEDTEXIMAGE2DARBPROC    glCompressedTexImage2D_;
extern PFNGLCOMPRESSEDTEXIMAGE1DARBPROC    glCompressedTexImage1D_;
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3D_;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2D_;
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1D_;
extern PFNGLGETCOMPRESSEDTEXIMAGEARBPROC   glGetCompressedTexImage_;

// GL_EXT_fog_coord
extern PFNGLFOGCOORDPOINTEREXTPROC glFogCoordPointer_;

#include "varray.h"

extern physent *camera1, camera;
extern mapz hdr;
extern int worldscale, octaentsize;
extern vector<ushort> texmru;
extern int xtraverts, xtravertsva;
extern const ivec cubecoords[8];
extern const ivec facecoords[6][4];
extern const uchar fv[6][4];
extern const uchar fvmasks[64];
extern const uchar faceedgesidx[6][4];
extern bool inbetweenframes, renderedframe;

extern SDL_Surface *screen;
extern int zpass, glowpass;

// rendertext
struct font
{
    struct charinfo
    {
        short x, y, w, h, offsetx, offsety, advance, tex;
    };

    char *name;
    vector<Texture *> texs;
    vector<charinfo> chars;
    int charoffset, defaultw, defaulth, scale;

    font() : name(NULL) {}
    ~font() { DELETEA(name); }
};

#define FONTH (curfont->scale)
#define FONTW (FONTH/2)
#define FONTTAB (4*FONTW)

extern font *curfont;

// texture
extern int hwtexsize, hwcubetexsize, hwmaxaniso, aniso, envmapradius;

extern Texture *textureload(const char *name, int clamp = 0, bool mipit = true, bool msg = true);
extern int texalign(void *data, int w, int bpp);
extern void cleanuptexture(Texture *t);
extern void loadalphamask(Texture *t);
extern void loadlayermasks();
extern GLuint cubemapfromsky(int size);
extern Texture *cubemapload(const char *name, bool mipit = true, bool msg = true, bool transient = false);
extern void drawcubemap(int size, int level, const vec &o, float yaw, float pitch, bool flipx, bool flipy, bool swapxy);
extern void loadshaders();
extern void setuptexparameters(int tnum, void *pixels, int clamp, int filter, GLenum format = GL_RGB, GLenum target = GL_TEXTURE_2D);
extern void createtexture(int tnum, int w, int h, void *pixels, int clamp, int filter, GLenum component = GL_RGB, GLenum target = GL_TEXTURE_2D, int pw = 0, int ph = 0, int pitch = 0, bool resize = true, GLenum format = GL_FALSE);
extern void blurtexture(int n, int bpp, int w, int h, uchar *dst, const uchar *src, int margin = 0);
extern void blurnormals(int n, int w, int h, bvec *dst, const bvec *src, int margin = 0);
extern void renderpostfx();
extern void initenvmaps();
extern void genenvmaps();
extern ushort closestenvmap(const vec &o);
extern ushort closestenvmap(int orient, int x, int y, int z, int size);
extern GLuint lookupenvmap(ushort emid);
extern GLuint lookupenvmap(Slot &slot);
extern bool reloadtexture(Texture *t);
extern bool reloadtexture(const char *name);
extern void setuptexcompress();
extern void clearslots();
extern void compacteditvslots();
extern void compactmruvslots();
extern void compactvslot(int &index);
extern void compactvslots(cube *c, int n = 8);
extern int compactvslots(bool cull = false);

// shader

extern int useshaders, shaderprecision;

// shadowmap

extern int shadowmap, shadowmapcasters;
extern bool shadowmapping;

extern bool isshadowmapcaster(const vec &o, float rad);
extern bool addshadowmapcaster(const vec &o, float xyrad, float zrad);
extern bool isshadowmapreceiver(vtxarray *va);
extern void rendershadowmap();
extern void pushshadowmap();
extern void popshadowmap();
extern void rendershadowmapreceivers();
extern void guessshadowdir();

// pvs
extern void clearpvs();
extern bool pvsoccluded(const ivec &bborigin, const ivec &bbsize);
extern bool waterpvsoccluded(int height);
extern void setviewcell(const vec &p);
extern void savepvs(stream *f);
extern void loadpvs(stream *f);
extern int getnumviewcells();

static inline bool pvsoccluded(const ivec &bborigin, int size)
{
    return pvsoccluded(bborigin, ivec(size, size, size));
}

// rendergl
extern bool hasVBO, hasDRE, hasOQ, hasTR, hasFBO, hasDS, hasTF, hasBE, hasBC, hasCM, hasNP2, hasTC, hasTE, hasMT, hasD3, hasAF, hasVP2, hasVP3, hasPP, hasMDA, hasTE3, hasTE4, hasVP, hasFP, hasGLSL, hasGM, hasNVFB, hasSGIDT, hasSGISH, hasDT, hasSH, hasNVPCF, hasRN, hasPBO, hasFBB, hasUBO, hasBUE, hasFC, hasTEX;
extern int hasstencil;

extern bool envmapping, minimapping, renderedgame, playerpreviewing;
extern const glmatrixf viewmatrix;
extern glmatrixf mvmatrix, projmatrix, mvpmatrix, invmvmatrix, invmvpmatrix, fogmatrix, invfogmatrix, envmatrix;
extern bvec fogcolor;

extern float cursorx, cursory;
extern vec cursordir;

extern GLenum colormask[3];
#define COLORMASK colormask[0], colormask[1], colormask[2]
#define SAVECOLORMASK \
    GLenum oldcolormask[3]; \
    memcpy(oldcolormask, colormask, sizeof(oldcolormask)); \
    setcolormask(); \
    if(memcmp(colormask, oldcolormask, sizeof(oldcolormask))) glColorMask(COLORMASK, GL_TRUE);
#define RESTORECOLORMASK \
    if(memcmp(colormask, oldcolormask, sizeof(oldcolormask))) \
    { \
        memcpy(colormask, oldcolormask, sizeof(oldcolormask)); \
        glColorMask(COLORMASK, GL_TRUE); \
    }

extern void gl_checkextensions();
extern void gl_init(int w, int h, int bpp, int depth, int fsaa);
extern void cleangl();

extern void vecfromcursor(float x, float y, float z, vec &dir);
extern bool vectocursor(const vec &v, float &x, float &y, float &z, float clampxy = -1);
extern void findorientation(vec &o, float yaw, float pitch, vec &pos);
extern void rendergame();
extern void renderavatar(bool early);
extern void invalidatepostfx();
extern void drawnoview();
extern bool hasnoview();
extern void gl_drawframe(int w, int h);
extern void drawminimap();
extern void enablepolygonoffset(GLenum type);
extern void disablepolygonoffset(GLenum type);
extern void calcspherescissor(const vec &center, float size, float &sx1, float &sy1, float &sx2, float &sy2);
extern int pushscissor(float sx1, float sy1, float sx2, float sy2);
extern void popscissor();
extern void setcolormask(bool r = true, bool g = true, bool b = true);

// renderextras
extern void render3dbox(vec &o, float tofloor, float toceil, float xradius, float yradius = 0);
extern void renderellipse(vec &o, float xradius, float yradius, float yaw);

// octa
extern cube *newcubes(uint face = F_EMPTY, int mat = MAT_AIR);
extern cubeext *growcubeext(cubeext *ext, int maxverts);
extern void setcubeext(cube &c, cubeext *ext);
extern cubeext *newcubeext(cube &c, int maxverts = 0, bool init = true);
extern void getcubevector(cube &c, int d, int x, int y, int z, ivec &p);
extern void setcubevector(cube &c, int d, int x, int y, int z, const ivec &p);
extern int familysize(cube &c);
extern void freeocta(cube *c);
extern void discardchildren(cube &c, bool fixtex = false, int depth = 0);
extern void optiface(uchar *p, cube &c);
extern void validatec(cube *c, int size = 0);
extern bool isvalidcube(cube &c);
extern ivec lu;
extern int lusize;
extern cube &lookupcube(int tx, int ty, int tz, int tsize = 0, ivec &ro = lu, int &rsize = lusize);
extern cube *neighbourstack[32];
extern int neighbourdepth;
extern cube &neighbourcube(cube &c, int orient, int x, int y, int z, int size, ivec &ro = lu, int &rsize = lusize);
extern int lookupmaterial(const vec &o);
extern void resetclipplanes();
extern int getmippedtexture(cube &p, int orient);
extern void forcemip(cube &c, bool fixtex = true);
extern bool subdividecube(cube &c, bool fullcheck=true, bool brighten=true);
extern void edgespan2vectorcube(cube &c);
extern int faceconvexity(ivec v[4]);
extern int faceconvexity(ivec v[4], int &vis);
extern int faceconvexity(vertinfo *verts, int numverts, int size);
extern int faceconvexity(cube &c, int orient);
extern void calcvert(cube &c, int x, int y, int z, int size, ivec &vert, int i, bool solid = false);
extern void calcvert(cube &c, int x, int y, int z, int size, vec &vert, int i, bool solid = false);
extern uint faceedges(cube &c, int orient);
extern bool collapsedface(cube &c, int orient);
extern bool touchingface(cube &c, int orient);
extern bool flataxisface(cube &c, int orient);
extern bool collideface(cube &c, int orient);
extern int genclipplane(cube &c, int i, vec *v, plane *clip);
extern void genclipplanes(cube &c, int x, int y, int z, int size, clipplanes &p);
extern bool visibleface(cube &c, int orient, int x, int y, int z, int size, uchar mat = MAT_AIR, uchar nmat = MAT_AIR, uchar matmask = MATF_VOLUME);
extern int visibletris(cube &c, int orient, int x, int y, int z, int size, uchar nmat = MAT_AIR, uchar matmask = MAT_AIR);
extern int visibleorient(cube &c, int orient);
extern void genfaceverts(cube &c, int orient, ivec v[4]);
extern int calcmergedsize(int orient, const ivec &co, int size, const vertinfo *verts, int numverts);
extern void invalidatemerges(cube &c, const ivec &co, int size, bool msg);
extern void calcmerges();

extern int mergefaces(int orient, facebounds *m, int sz);
extern void mincubeface(cube &cu, int orient, const ivec &o, int size, const facebounds &orig, facebounds &cf, uchar nmat = MAT_AIR, uchar matmask = MATF_VOLUME);

static inline uchar octantrectangleoverlap(const ivec &c, int size, const ivec &o, const ivec &s)
{
    uchar p = 0xFF; // bitmask of possible collisions with octants. 0 bit = 0 octant, etc
    ivec v(c);
    v.add(size);
    if(v.z <= o.z)     p &= 0xF0; // not in a -ve Z octant
    else if(v.z >= o.z+s.z) p &= 0x0F; // not in a +ve Z octant
    if(v.y <= o.y)     p &= 0xCC; // not in a -ve Y octant
    else if(v.y >= o.y+s.y) p &= 0x33; // etc..
    if(v.x <= o.x)     p &= 0xAA;
    else if(v.x >= o.x+s.x) p &= 0x55;
    return p;
}

static inline bool insideworld(const vec &o)
{
    return o.x>=0 && o.x<hdr.worldsize && o.y>=0 && o.y<hdr.worldsize && o.z>=0 && o.z<hdr.worldsize;
}

static inline bool insideworld(const ivec &o)
{
    return uint(o.x)<uint(hdr.worldsize) && uint(o.y)<uint(hdr.worldsize) && uint(o.z)<uint(hdr.worldsize);
}

static inline cubeext &ext(cube &c)
{
    return *(c.ext ? c.ext : newcubeext(c));
}

// ents
extern bool haveselent();
extern undoblock *copyundoents(undoblock *u);
extern void pasteundoents(undoblock *u);

// octaedit
extern selinfo sel;
extern int texpaneltimer;
extern void cancelsel();
extern void render_texture_panel(int w, int h);
extern void addundo(undoblock *u);
extern void commitchanges(bool force = false);
extern editinfo *localedit;

// octarender
extern vector<tjoint> tjoints;

extern ushort encodenormal(const vec &n);
extern vec decodenormal(ushort norm);
extern void reduceslope(ivec &n);
extern void findtjoints();
extern void octarender();
extern void allchanged(bool load = false);
extern void clearvas(cube *c);
extern vtxarray *newva(int x, int y, int z, int size);
extern void destroyva(vtxarray *va, bool reparent = true);
extern bool readva(vtxarray *va, ushort *&edata, uchar *&vdata);
extern void updatevabb(vtxarray *va, bool force = false);
extern void updatevabbs(bool force = false);

// renderva
extern void visiblecubes(bool cull = true);
extern void setvfcP(float z = -1, const vec &bbmin = vec(-1, -1, -1), const vec &bbmax = vec(1, 1, 1));
extern void savevfcP();
extern void restorevfcP();
extern void rendergeom(float causticspass = 0, bool fogpass = false);
extern void renderalphageom(bool fogpass = false);
extern void rendermapmodels();
extern void renderreflectedgeom(bool causticspass = false, bool fogpass = false);
extern void renderreflectedmapmodels();
extern void renderoutline();
extern bool rendersky(bool explicitonly = false);

extern bool isfoggedsphere(float rad, const vec &cv);
extern int isvisiblesphere(float rad, const vec &cv);
extern bool bboccluded(const ivec &bo, const ivec &br);
extern occludequery *newquery(void *owner);
extern bool checkquery(occludequery *query, bool nowait = false);
extern void resetqueries();
extern int getnumqueries();
extern void drawbb(const ivec &bo, const ivec &br, const vec &camera = camera1->o);

#define startquery(query) { glBeginQuery_(GL_SAMPLES_PASSED_ARB, ((occludequery *)(query))->id); }
#define endquery(query) \
    { \
        glEndQuery_(GL_SAMPLES_PASSED_ARB); \
        extern int ati_oq_bug; \
        if(ati_oq_bug) glFlush(); \
    }

// dynlight

extern void updatedynlights();
extern int finddynlights();
extern void calcdynlightmask(vtxarray *va);
extern int setdynlights(vtxarray *va);
extern bool getdynlight(int n, vec &o, float &radius, vec &color);

// material

extern int showmat;

extern namemap materials[], textypes[];
extern const char *findmaterialname(int type);
extern int findmaterial(const char *name, bool tryint = false);
extern void genmatsurfs(cube &c, int cx, int cy, int cz, int size, vector<materialsurface> &matsurfs);
extern void rendermatsurfs(materialsurface *matbuf, int matsurfs);
extern void rendermatgrid(materialsurface *matbuf, int matsurfs);
extern int optimizematsurfs(materialsurface *matbuf, int matsurfs);
extern void setupmaterials(int start = 0, int len = 0);
extern void rendermaterials();
extern int visiblematerial(cube &c, int orient, int x, int y, int z, int size, uchar matmask = MATF_VOLUME);

// water
extern int refracting;
extern bool reflecting, fading, fogging;
extern float reflectz;
extern int reflectdist, vertwater, waterrefract, waterreflect, waterfade, caustics, waterfallrefract, waterfog, lavafog;
extern bvec watercol, waterfallcol, lavacol;

extern void cleanreflections();
extern void queryreflections();
extern void drawreflections();
extern void renderwater();
extern void renderlava(const materialsurface &m, Texture *tex, float scale);
extern void loadcaustics(bool force = false);
extern void preloadwatershaders(bool force = false);

// glare
extern bool glaring;

extern void drawglaretex();
extern void addglare();

// depthfx
extern bool depthfxing;

extern void drawdepthfxtex();

// server
extern vector<char *> gameargs;
extern void initgame();
extern void changeservertype();
extern void cleanupserver();
extern void serverslice(uint timeout = 0);
extern int updatetimer(bool limit);

extern void retrieveservers(vector<char> &data);
extern void localclienttoserver(int chan, ENetPacket *);
extern void lanconnect();
extern bool serveroption(char *opt);
extern void localconnect(bool force = true);
extern void localdisconnect();

// serverbrowser
extern void addserver(const char *name, int port, const char *desc = NULL);

// client
extern char *connectname;
extern int connectport;
extern void localservertoclient(int chan, ENetPacket *packet);
extern bool connected(bool attempt = true, bool local = true);
extern void connectserv(const char *name = NULL, int port = RE_SERVER_PORT, const char *password = NULL);
extern void reconnect();
extern void lanconnect();
extern void abortconnect(bool msg = true);
extern void clientkeepalive();
extern ENetHost *clienthost;
extern ENetPeer *curpeer, *connpeer;

// console
#ifdef __APPLE__
    #define MOD_KEYS (KMOD_LMETA|KMOD_RMETA)
#else
    #define MOD_KEYS (KMOD_LCTRL|KMOD_RCTRL)
#endif

extern void writebinds(stream *f);
extern void writecompletions(stream *f);
extern const char *addreleaseaction(char *s);
extern const char *getkeyname(int code);
extern int findkeycode(char *key);

extern int uimillis, conskip, commandmillis,  commandpos, commandcolour;
extern string commandbuf;
extern char *commandaction, *commandicon;
extern bool fullconsole;
// main
extern void quit();
enum
{
    NOT_INITING = 0,
    INIT_LOAD,
    INIT_RESET
};
extern int initing;

extern bool progressing;
extern float loadprogress, progresspart, progressamt;
extern char *progresstitle, *progresstext;
extern void progress(float bar1 = 0, const char *text1 = NULL, float bar2 = 0, const char *text2 = NULL);
extern void limitfps(int &millis, int curmillis);

enum
{
    CHANGE_GFX   = 1<<0,
    CHANGE_SOUND = 1<<1
};
extern bool initwarning(const char *desc, int level = INIT_RESET, int type = CHANGE_GFX);
extern void resetcursor(bool warp = true, bool reset = true);
extern int compresslevel, imageformat;

extern void pushevent(const SDL_Event &e);
extern bool interceptkey(int sym);
extern void getfps(int &fps, int &bestdiff, int &worstdiff);
extern void swapbuffers();
extern int getclockmillis();

// menu
extern float menuscale;

extern void menuprocess();
extern void addchange(const char *desc, int type);
extern void clearchanges(int type);

// physics
extern bool pointincube(const clipplanes &p, const vec &v);
extern bool overlapsdynent(const vec &o, float radius);
extern void rotatebb(vec &center, vec &radius, int yaw, int roll = 0);
extern float shadowray(const vec &o, const vec &ray, float radius, int mode, extentity *t = NULL);
struct ShadowRayCache;
extern ShadowRayCache *newshadowraycache();
extern void freeshadowraycache(ShadowRayCache *&cache);
extern void resetshadowraycache(ShadowRayCache *cache);
extern float shadowray(ShadowRayCache *cache, const vec &o, const vec &ray, float radius, int mode, extentity *t = NULL);
extern bool getsight(vec &o, float yaw, float pitch, vec &q, vec &v, float mdist, float fovx, float fovy);

// worldio
extern char *maptitle, *mapauthor, *mapname;
extern int getmapversion();
extern int getmaprevision();


// world
extern void entcancel();
extern void entitiesinoctanodes();
extern void attachentities();
extern void freeoctaentities(cube &c);
extern bool pointinsel(selinfo &sel, vec &o);

extern void clearworldvars(bool msg = false);
extern void resetmap(bool empty);

// rendermodel
struct mapmodelinfo { string name; model *m; };
extern vector<mapmodelinfo> mapmodels;
extern void mmodel(char *name);
extern void resetmapmodels();

extern bool matchanim(const char *name, const char *pattern);
extern void loadskin(const char *dir, const char *altdir, Texture *&skin, Texture *&masks);
extern model *loadmodel(const char *name, int i = -1, bool msg = false);
extern mapmodelinfo *getmminfo(int i);
extern void startmodelquery(occludequery *query);
extern void endmodelquery();
extern void preloadmodelshaders();
extern void preloadusedmapmodels(bool msg = false, bool bih = false);

// renderparticles
extern void particleinit();
extern void clearparticles();
extern void makeparticle(const vec &o, attrvector &attr);
extern void makeparticles(extentity &e);
extern void updateparticles();
extern void renderparticles(bool mainpass = false);


// decal
extern void initdecals();
extern void cleardecals();
extern void renderdecals(bool mainpass = false);

// blob

enum
{
    BLOB_STATIC = 0,
    BLOB_DYNAMIC
};

extern int showblobs;

extern void initblobs(int type = -1);
extern void resetblobs();
extern void renderblob(int type, const vec &o, float radius, float fade = 1);
extern void flushblobs();

// rendersky
extern int explicitsky;
extern double skyarea;

extern void drawskybox(int farplane, bool limited);
extern bool limitsky();

// gui
extern void progressmenu();
extern void mainmenu();
extern void texturemenu();
extern bool menuactive();
extern int cleargui(int n = 0);

// octaedit
extern void replacetexcube(cube &c, int oldtex, int newtex);

// skybox
extern void loadsky(char *basename);

// main
extern void setcaption(const char *text);
extern int grabinput, colorpos, curfps, bestfps, worstfps, bestfpsdiff, worstfpsdiff, maxfps;

// editing
extern int getmatvec(vec v);
extern int fullbright, fullbrightlevel;
extern vector<int> entgroup;

extern void newentity(int type, const attrvector &attrs);
extern void newentity(const vec &v, int type, const attrvector &attrs);

// menu
enum { MN_BACK = 0, MN_INPUT, MN_MAX };

// console
struct cline { char *cref; int type, reftime, outtime; };
extern vector<cline> conlines;
extern void conline(int type, const char *sf, int n);

// command
extern char *gettime(char *format);

// rendergl
extern int dynentsize, watercolour, lavacolour, fog, fogcolour;
extern bvec ambientcolor, skylightcolor;
extern float curfov, fovy, aspect;

extern void project(float fovy, float aspect, int farplane, bool flipx = false, bool flipy = false, bool swapxy = false, float zscale = 1);
extern void transplayer();

extern void usetexturing(bool on);

#define rendermainview (!shadowmapping && !envmapping && !reflecting && !refracting)
#define renderatopview (glaring)
#define rendernormally (rendermainview || renderatopview)

extern void renderplayerpreview(int model, int color, int team, int weap, bool background);
extern void drawslice(float start, float length, float x, float y, float size);
extern void drawfadedslice(float start, float length, float x, float y, float size, float alpha, float r = 1.f, float g = 1.f, float b = 1.f, float minsize = 0.25f);
extern void polyhue(dynent *d, vec &colour, int flags = 0);
extern void polybox(vec o, float tofloor, float toceil, float xradius, float yradius);

// grass
extern void generategrass();
extern void rendergrass();

// blendmap
struct BlendMapCache;
extern BlendMapCache *newblendmapcache();
extern void freeblendmapcache(BlendMapCache *&cache);
extern bool setblendmaporigin(BlendMapCache *cache, const ivec &o, int size);
extern bool hasblendmap(BlendMapCache *cache);
extern uchar lookupblendmap(BlendMapCache *cache, const vec &pos);
extern void resetblendmap();
extern void enlargeblendmap();
extern void shrinkblendmap(int octant);
extern void optimizeblendmap();
extern void renderblendbrush(GLuint tex, float x, float y, float w, float h);
extern void renderblendbrush();
extern bool loadblendmap(stream *f);
extern void saveblendmap(stream *f);
extern uchar shouldsaveblendmap();

// recorder
namespace recorder
{
    extern void stop();
    extern void capture();
    extern void cleanup();
}
#endif // STANDALONE

#endif

