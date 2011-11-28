#include "engine.h"

VAR(IDF_PERSIST, blinkingtext, 0, 1, 1);

static inline bool htcmp(const char *key, const font &f) { return !strcmp(key, f.name); }

static hashset<font> fonts;
static font *fontdef = NULL;
static int fontdeftex = 0;

font *curfont = NULL;
int curfonttex = 0;

void newfont(char *name, char *tex, int *defaultw, int *defaulth)
{
    font *f = &fonts[name];
    if(!f->name) f->name = newstring(name);
    f->texs.shrink(0);
    f->texs.add(textureload(tex));
    f->chars.shrink(0);
    f->charoffset = '!';
    f->defaultw = *defaultw;
    f->defaulth = *defaulth;
    f->scale = f->defaulth;

    fontdef = f;
    fontdeftex = 0;
}

void fontoffset(char *c)
{
    if(!fontdef) return;

    fontdef->charoffset = c[0];
}

void fontscale(int *scale)
{
    if(!fontdef) return;

    fontdef->scale = *scale > 0 ? *scale : fontdef->defaulth;
}

void fonttex(char *s)
{
    if(!fontdef) return;

    Texture *t = textureload(s);
    loopv(fontdef->texs) if(fontdef->texs[i] == t) { fontdeftex = i; return; }
    fontdeftex = fontdef->texs.length();
    fontdef->texs.add(t);
}

void fontchar(int *x, int *y, int *w, int *h, int *offsetx, int *offsety, int *advance)
{
    if(!fontdef) return;

    font::charinfo &c = fontdef->chars.add();
    c.x = *x;
    c.y = *y;
    c.w = *w ? *w : fontdef->defaultw;
    c.h = *h ? *h : fontdef->defaulth;
    c.offsetx = *offsetx;
    c.offsety = *offsety;
    c.advance = *advance ? *advance : c.offsetx + c.w;
    c.tex = fontdeftex;
}

void fontskip(int *n)
{
    if(!fontdef) return;
    loopi(max(*n, 1))
    {
        font::charinfo &c = fontdef->chars.add();
        c.x = c.y = c.w = c.h = c.offsetx = c.offsety = c.advance = c.tex = 0;
    }
}

COMMANDN(0, font, newfont, "ssii");
COMMAND(0, fontoffset, "s");
COMMAND(0, fontscale, "i");
COMMAND(0, fonttex, "s");
COMMAND(0, fontchar, "iiiiiii");
COMMAND(0, fontskip, "i");

font *loadfont(const char *name)
{
    font *f = fonts.access(name);
    if(!f)
    {
        defformatstring(n)("fonts/%s.cfg", name);
        if(execfile(n, false)) f = fonts.access(name);
    }
    return f;
}

void fontalias(const char *dst, const char *src)
{
    font *s = loadfont(src);
    if(!s) return;
    font *d = &fonts[dst];
    if(!d->name) d->name = newstring(dst);
    d->texs = s->texs;
    d->chars = s->chars;
    d->charoffset = s->charoffset;
    d->defaultw = s->defaultw;
    d->defaulth = s->defaulth;
    d->scale = s->scale;

    fontdef = d;
    fontdeftex = d->texs.length()-1;
}

COMMAND(0, fontalias, "ss");

bool setfont(const char *name)
{
    font *f = loadfont(name);
    if(!f) return false;
    curfont = f;
    return true;
}

int text_width(const char *str, int flags) { //@TODO deprecate in favour of text_bounds(..)
    int width, height;
    text_bounds(str, width, height, -1, flags);
    return width;
}

void tabify(const char *str, int *numtabs)
{
    vector<char> tabbed;
    tabbed.put(str, strlen(str));
    int w = text_width(str), tw = max(*numtabs, 0)*PIXELTAB;
    while(w < tw)
    {
        tabbed.add('\t');
        w = ((w+PIXELTAB)/PIXELTAB)*PIXELTAB;
    }
    tabbed.add('\0');
    result(tabbed.getbuf());
}

COMMAND(0, tabify, "si");

int draw_textf(const char *fstr, int left, int top, ...)
{
    defvformatstring(str, top, fstr);
    return draw_text(str, left, top);
}

#define FONTFLOOR(n) (((n)*curfont->scale)/curfont->defaulth)
#define FONTCEIL(n) (((n)*curfont->scale + curfont->defaulth-1)/curfont->defaulth)
#define FONTROUND(n) (((n)*curfont->scale + (curfont->defaulth>>1))/curfont->defaulth)

static int draw_char(Texture *&tex, int c, int x, int y)
{
    font::charinfo &info = curfont->chars[c-curfont->charoffset];
    if(tex != curfont->texs[info.tex])
    {
        xtraverts += varray::end();
        tex = curfont->texs[info.tex];
        glBindTexture(GL_TEXTURE_2D, tex->id);
    }

    float x1 = x + FONTFLOOR(info.offsetx),
          y1 = y + FONTFLOOR(info.offsety),
          x2 = x + FONTROUND(info.offsetx + info.w),
          y2 = y + FONTROUND(info.offsety + info.h),
          tx1 = info.x / float(tex->xs),
          ty1 = info.y / float(tex->ys),
          tx2 = (info.x + info.w) / float(tex->xs),
          ty2 = (info.y + info.h) / float(tex->ys);

    varray::attrib<float>(x1, y1); varray::attrib<float>(tx1, ty1);
    varray::attrib<float>(x2, y1); varray::attrib<float>(tx2, ty1);
    varray::attrib<float>(x2, y2); varray::attrib<float>(tx2, ty2);
    varray::attrib<float>(x1, y2); varray::attrib<float>(tx1, ty2);

    return FONTROUND(info.advance);
}

static void text_color(char c, char *stack, int size, int &sp, bvec &color, int r, int g, int b, int a)
{
    char d = c;
    if(d=='s') // save color
    {
        d = stack[sp];
        if(sp<size-1) stack[++sp] = d;
    }
    else
    {
        if(d=='S') d = stack[(sp > 0) ? --sp : sp]; // restore color
        else stack[sp] = d;
    }
    xtraverts += varray::end();
    int f = a;
    switch(d)
    {
        case 'g': case '0': color = bvec( 64, 255,  64); break; // green
        case 'b': case '1': color = bvec(86,  92,  255); break; // blue
        case 'y': case '2': color = bvec(255, 255,   0); break; // yellow
        case 'r': case '3': color = bvec(255,  64,  64); break; // red
        case 'a': case '4': color = bvec(192, 192, 192); break; // grey
        case 'm': case '5': color = bvec(255, 186, 255); break; // magenta
        case 'o': case '6': color = bvec(255,  64,   0); break; // orange
        case 'w': case '7': color = bvec(255, 255, 255); break; // white
        case 'k': case '8': color = bvec(0,     0,   0); break; // black
        case 'c': case '9': color = bvec(64,  255, 255); break; // cyan
        case 'v': color = bvec(192,  96, 255); break; // violet
        case 'p': color = bvec(224,  64, 224); break; // purple
        case 'n': color = bvec(164,  72,  56); break; // brown
        case 'G': color = bvec( 86, 164,  56); break; // dark green
        case 'B': color = bvec( 56,  64, 172); break; // dark blue
        case 'Y': color = bvec(172, 172,   0); break; // dark yellow
        case 'R': color = bvec(172,  56,  56); break; // dark red
        case 'M': color = bvec(172,  72, 172); break; // dark magenta
        case 'O': color = bvec(172,  56,   0); break; // dark orange
        case 'C': color = bvec(48,  172, 172); break; // dark cyan
        case 'A': case 'd': color = bvec(102, 102, 102); break; // dark grey
        case 'e': case 'E': f -= d!='E' ? f/2 : f/4; break;
        case 'u': color = bvec(r, g, b); break; // user colour
        case 'Z': default: break; // everything else
    }
    glColor4ub(color.x, color.y, color.z, f);
}

#define FONTX ((FONTH*9)/8)
static int draw_icon(Texture *&tex, const char *name, int x, int y)
{
    Texture *t = textureload(name, 3);
    if(!t) return 0;
    if(tex != t)
    {
        xtraverts += varray::end();
        tex = t;
        glBindTexture(GL_TEXTURE_2D, tex->id);
    }
    int width = int((t->w/float(t->h))*FONTX);
    varray::attrib<float>(x,         y        ); varray::attrib<float>(0, 0);
    varray::attrib<float>(x + width, y        ); varray::attrib<float>(1, 0);
    varray::attrib<float>(x + width, y + FONTX); varray::attrib<float>(1, 1);
    varray::attrib<float>(x,         y + FONTX); varray::attrib<float>(0, 1);
    return width;
}

static int icon_width(const char *name)
{
    Texture *t = textureload(name, 3);
    if(t) return int((t->w/float(t->h))*FONTX);
    return 0;
}

#define TEXTTAB(g) clamp(g + (PIXELTAB - (g % PIXELTAB)), g + FONTW, g + PIXELTAB)
#define TEXTCOLORIZE(g,h) \
{ \
    if(g[h] == 'z' && g[h+1]) \
    { \
        h++; \
        bool alt = blinkingtext && totalmillis%500 > 250; \
        TEXTCOLOR(h); \
        if(g[h+1]) \
        { \
            h++; \
            if(alt) TEXTCOLOR(h); \
        } \
    } \
    else if(g[h] == '[') \
    { \
        h++; \
        const char *start = &g[h]; \
        const char *end = strchr(start, ']'); \
        if(end) \
        { \
            if(end > start) { TEXTHEXCOLOR(bvec(parseint(start))); } \
            h += end-start; \
        } \
        else break; \
    } \
    else if(g[h] == '(') \
    { \
        h++; \
        const char *start = &g[h]; \
        const char *end = strchr(start, ')'); \
        if(end) \
        { \
            if(end > start) \
            { \
                string value; copystring(value, start, min(size_t(end - start + 1), sizeof(value))); \
                TEXTICON(value); \
            } \
            h += end-start; \
        } \
        else break; \
    } \
    else TEXTCOLOR(h); \
}
#define TEXTALIGN \
    x = (!(flags&TEXT_RIGHT_JUSTIFY) && !(flags&TEXT_NO_INDENT) ? TEXTTAB(0) : 0); \
    if(!y && (flags&TEXT_RIGHT_JUSTIFY) && !(flags&TEXT_NO_INDENT)) maxwidth -= PIXELTAB; \
    y += FONTH;
#define TEXTSKELETON \
    int y = 0, x = 0;\
    int i;\
    for(i = 0; str[i]; i++)\
    {\
        int c = uchar(str[i]);\
        TEXTINDEX(i)\
        if(c=='\t')      { x = TEXTTAB(x); TEXTWHITE(i) }\
        else if(c==' ')  { x += FONTROUND(curfont->defaultw); TEXTWHITE(i) }\
        else if(c=='\n') { TEXTLINE(i) TEXTALIGN }\
        else if(c=='\f') { if(str[i+1]) { i++; TEXTCOLORIZE(str, i); } }\
        else if(curfont->chars.inrange(c-curfont->charoffset))\
        {\
            int cw = FONTROUND(curfont->chars[c-curfont->charoffset].advance);\
            if(cw <= 0) continue;\
            if(maxwidth != -1)\
            {\
                int j = i;\
                int w = cw;\
                for(; str[i+1]; i++)\
                {\
                    int c = uchar(str[i+1]);\
                    if(c=='\f') { if(str[i+2]) i++; continue; }\
                    if(i-j > 16) break;\
                    if(!curfont->chars.inrange(c-curfont->charoffset)) break;\
                    int cw = FONTROUND(curfont->chars[c-curfont->charoffset].advance);\
                    if(cw <= 0 || w + cw >= maxwidth) break; \
                    w += cw;\
                }\
                if(x + w >= maxwidth && j!=0) { TEXTLINE(j-1) TEXTALIGN }\
                TEXTWORD\
            }\
            else { TEXTCHAR(i) }\
        }\
    }\
    TEXTINDEX(i)

//all the chars are guaranteed to be either drawable or color commands
#define TEXTWORDSKELETON \
                for(; j <= i; j++)\
                {\
                    TEXTINDEX(j)\
                    int c = uchar(str[j]);\
                    if(c=='\f') { if(str[j+1]) { j++; TEXTCOLORIZE(str, j); } }\
                    else { int cw = FONTROUND(curfont->chars[c-curfont->charoffset].advance); TEXTCHAR(j) }\
                }

int text_visible(const char *str, int hitx, int hity, int maxwidth, int flags)
{
    #define TEXTINDEX(idx)
    #define TEXTWHITE(idx) if(y+FONTH > hity && x >= hitx) return idx;
    #define TEXTLINE(idx) if(y+FONTH > hity) return idx;
    #define TEXTCOLOR(idx)
    #define TEXTHEXCOLOR(ret)
    #define TEXTICON(ret) x += icon_width(ret);
    #define TEXTCHAR(idx) x += cw; TEXTWHITE(idx)
    #define TEXTWORD TEXTWORDSKELETON
    TEXTSKELETON
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTHEXCOLOR
    #undef TEXTICON
    #undef TEXTCHAR
    #undef TEXTWORD
    return i;
}

//inverse of text_visible
void text_pos(const char *str, int cursor, int &cx, int &cy, int maxwidth, int flags)
{
    #define TEXTINDEX(idx) if(cursor == idx) { cx = x; cy = y; }
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx)
    #define TEXTCOLOR(idx)
    #define TEXTHEXCOLOR(ret)
    #define TEXTICON(ret) x += icon_width(ret);
    #define TEXTCHAR(idx) x += cw;
    #define TEXTWORD TEXTWORDSKELETON if(i >= cursor) break;
    cx = cy = 0;
    TEXTSKELETON
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTHEXCOLOR
    #undef TEXTICON
    #undef TEXTCHAR
    #undef TEXTWORD
}

void text_bounds(const char *str, int &width, int &height, int maxwidth, int flags)
{
    #define TEXTINDEX(idx)
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx) if(x > width) width = x;
    #define TEXTCOLOR(idx)
    #define TEXTHEXCOLOR(ret)
    #define TEXTICON(ret) x += icon_width(ret);
    #define TEXTCHAR(idx) x += cw;
    #define TEXTWORD TEXTWORDSKELETON
    width = 0;
    TEXTSKELETON
    height = y + FONTH;
    TEXTLINE(_)
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTHEXCOLOR
    #undef TEXTICON
    #undef TEXTCHAR
    #undef TEXTWORD
}

int draw_text(const char *str, int rleft, int rtop, int r, int g, int b, int a, int flags, int cursor, int maxwidth)
{
    #define TEXTINDEX(idx) if(cursor == idx) { cx = x; cy = y; }
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx) ly += FONTH;
    #define TEXTCOLOR(idx) if(usecolor) text_color(str[idx], colorstack, sizeof(colorstack), colorpos, color, r, g, b, a);
    #define TEXTHEXCOLOR(ret) if(usecolor) { xtraverts += varray::end(); color = ret; glColor4ub(color.x, color.y, color.z, a); }
    #define TEXTICON(ret) { x += draw_icon(tex, ret, left+x, top+y); }
    #define TEXTCHAR(idx) { draw_char(tex, c, left+x, top+y); x += cw; }
    #define TEXTWORD TEXTWORDSKELETON
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Texture *tex = curfont->texs[0];
    glBindTexture(GL_TEXTURE_2D, tex->id);
    varray::enable();
    varray::defattrib(varray::ATTRIB_VERTEX, 2, GL_FLOAT);
    varray::defattrib(varray::ATTRIB_TEXCOORD0, 2, GL_FLOAT);
    varray::begin(GL_QUADS);
    bool usecolor = true;
    if(a < 0) { usecolor = false; a = -a; }
    int colorpos = 0, cx = -FONTW, cy = 0, ly = 0, left = rleft, top = rtop;
    char colorstack[10];
    memset(colorstack, 'u', sizeof(colorstack)); //indicate user color
    bvec color(r, g, b);
    glColor4ub(color.x, color.y, color.z, a);
    TEXTSKELETON
    xtraverts += varray::end();
    if(cursor >= 0)
    {
        glColor4ub(255, 255, 255, int(255*clamp(1.f-(float(totalmillis%500)/500.f), 0.5f, 1.f)));
        draw_char(tex, '_', left+cx, top+cy+FONTH/6);
        xtraverts += varray::end();
    }
    varray::disable();
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTHEXCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
    return ly + FONTH;
}

void reloadfonts()
{
    enumerate(fonts, font, f,
        loopv(f.texs) if(!reloadtexture(f.texs[i])) fatal("failed to reload font texture");
    );
}


int draw_textx(const char *fstr, int left, int top, int r, int g, int b, int a, int flags, int cursor, int maxwidth, ...)
{
    defvformatstring(str, maxwidth, fstr);

    int width = 0, height = 0;
    text_bounds(str, width, height, maxwidth, flags);
    if(flags&TEXT_ALIGN) switch(flags&TEXT_ALIGN)
    {
        case TEXT_CENTERED: left -= width/2; break;
        case TEXT_RIGHT_JUSTIFY: left -= width; break;
        default: break;
    }
    if(flags&TEXT_UPWARD) top -= height;
    if(flags&TEXT_SHADOW) draw_text(str, left-2, top-2, 0, 0, 0, a, flags, cursor, maxwidth);
    return draw_text(str, left, top, r, g, b, a, flags, cursor, maxwidth);
}

vector<const char *> fontstack;

bool pushfont(const char *name)
{
    if(!fontstack.length() && curfont)
        fontstack.add(curfont->name);

    if(setfont(name))
    {
        fontstack.add(name);
        return true;
    }
    return false;
}

bool popfont(int num)
{
    loopi(num)
    {
        if(!fontstack.length()) break;
        fontstack.pop();
    }
    return setfont(fontstack.length() ? fontstack.last() : "default");
}
