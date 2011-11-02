#include "engine.h"

VAR(IDF_PERSIST, blinkingtext, 0, 1, 1);

static hashtable<const char *, font> fonts;
static font *fontdef = NULL;
static int fontdeftex = 0;

font *curfont = NULL;
int curfonttex = 0;

void newfont(char *name, char *tex, int *defaultw, int *defaulth)
{
    font *f = fonts.access(name);
    if(!f)
    {
        name = newstring(name);
        f = &fonts[name];
        f->name = name;
    }

    f->texs.shrink(0);
    f->texs.add(textureload(tex));
    f->chars.shrink(0);
    f->charoffset = '!';
    f->defaultw = *defaultw;
    f->defaulth = *defaulth;

    fontdef = f;
    fontdeftex = 0;
}

void fontoffset(char *c)
{
    if(!fontdef) return;

    fontdef->charoffset = c[0];
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

COMMANDN(0, font, newfont, "ssii");
COMMAND(0, fontoffset, "s");
COMMAND(0, fonttex, "s");
COMMAND(0, fontchar, "iiiiiii");

bool setfont(const char *name)
{
    font *f = fonts.access(name);
    if(!f)
    {
        defformatstring(n)("fonts/%s.cfg", name);
        if(!execfile(n, false) || !(f = fonts.access(name)))
            return false;
    }
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

static int draw_char(Texture *&tex, int c, int x, int y)
{
    font::charinfo &info = curfont->chars[c-curfont->charoffset];
    if(tex != curfont->texs[info.tex])
    {
        xtraverts += varray::end();
        tex = curfont->texs[info.tex];
        glBindTexture(GL_TEXTURE_2D, tex->id);
    }

    float tc_left    = info.x / float(tex->xs);
    float tc_top     = info.y / float(tex->ys);
    float tc_right   = (info.x + info.w) / float(tex->xs);
    float tc_bottom  = (info.y + info.h) / float(tex->ys);

    x += info.offsetx;
    y += info.offsety;

    varray::attrib<float>(x,          y         ); varray::attrib<float>(tc_left,  tc_top   );
    varray::attrib<float>(x + info.w, y         ); varray::attrib<float>(tc_right, tc_top   );
    varray::attrib<float>(x + info.w, y + info.h); varray::attrib<float>(tc_right, tc_bottom);
    varray::attrib<float>(x,          y + info.h); varray::attrib<float>(tc_left,  tc_bottom);

    return info.advance;
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

#define FONTX (max(curfont->defaultw, curfont->defaulth)*9/8)
static int draw_icon(const char *name, int x, int y)
{
    Texture *t = textureload(name, 3);
    if(t)
    {
        int width = int((t->w/float(t->h))*FONTX);
        glBindTexture(GL_TEXTURE_2D, t->id);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, 0); glVertex2f(x, y);
        glTexCoord2f(1, 0); glVertex2f(x+width, y);
        glTexCoord2f(0, 1); glVertex2f(x, y+FONTX);
        glTexCoord2f(1, 1); glVertex2f(x+width, y+FONTX);
        glEnd();
        return width;
    }
    return 0;
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
        else if(c==' ')  { x += curfont->defaultw; TEXTWHITE(i) }\
        else if(c=='\n') { TEXTLINE(i) TEXTALIGN }\
        else if(c=='\f') { if(str[i+1]) { i++; TEXTCOLORIZE(str, i); } }\
        else if(curfont->chars.inrange(c-curfont->charoffset))\
        {\
            if(maxwidth != -1)\
            {\
                int j = i;\
                int w = curfont->chars[c-curfont->charoffset].advance;\
                for(; str[i+1]; i++)\
                {\
                    int c = uchar(str[i+1]);\
                    if(c=='\f') { if(str[i+2]) i++; continue; }\
                    if(i-j > 16) break;\
                    if(!curfont->chars.inrange(c-curfont->charoffset)) break;\
                    int cw = curfont->chars[c-curfont->charoffset].advance;\
                    if(w + cw >= maxwidth) break; \
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
                    else { TEXTCHAR(j) }\
                }

int text_visible(const char *str, int hitx, int hity, int maxwidth, int flags)
{
    #define TEXTINDEX(idx)
    #define TEXTWHITE(idx) if(y+FONTH > hity && x >= hitx) return idx;
    #define TEXTLINE(idx) if(y+FONTH > hity) return idx;
    #define TEXTCOLOR(idx)
    #define TEXTHEXCOLOR(ret)
    #define TEXTICON(ret) x += icon_width(ret);
    #define TEXTCHAR(idx) x += curfont->chars[c-curfont->charoffset].advance; TEXTWHITE(idx)
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
    #define TEXTCHAR(idx) x += curfont->chars[c-curfont->charoffset].advance;
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
    #define TEXTCHAR(idx) x += curfont->chars[c-curfont->charoffset].advance;
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
    #define TEXTCOLOR(idx) text_color(str[idx], colorstack, sizeof(colorstack), colorpos, color, r, g, b, a);
    #define TEXTHEXCOLOR(ret) { color = ret; glColor4ub(color.x, color.y, color.z, a); }
    #define TEXTICON(ret) { x += (iconpass ? draw_icon(ret, left+x, top+y) : icon_width(ret)); neediconpass = true; }
    #define TEXTCHAR(idx) { x += (!iconpass ? draw_char(tex, c, left+x, top+y) : curfont->chars[c-curfont->charoffset].advance); }
    #define TEXTWORD TEXTWORDSKELETON
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Texture *tex = curfont->texs[0];
    glBindTexture(GL_TEXTURE_2D, tex->id);
    varray::enable();
    varray::defattrib(varray::ATTRIB_VERTEX, 2, GL_FLOAT);
    varray::defattrib(varray::ATTRIB_TEXCOORD0, 2, GL_FLOAT);
    varray::begin(GL_QUADS);
    bool neediconpass = false;
    int cx = -FONTW, cy = 0, ly = 0, left = rleft, top = rtop;
    loop(iconpass, 2)
    {
        char colorstack[10];
        bvec color(r, g, b);
        int colorpos = 0;
        loopi(10) colorstack[i] = 'u'; //indicate user color
        glColor4ub(color.x, color.y, color.z, a);
        cx = -FONTW; cy = 0;
        TEXTSKELETON
        if(!iconpass)
        {
            xtraverts += varray::end();
            if(cursor >= 0)
            {
                glColor4ub(255, 255, 255, int(255*clamp(1.f-(float(totalmillis%500)/500.f), 0.5f, 1.f)));
                draw_char(tex, '_', left+cx, top+cy+FONTH/6);
                xtraverts += varray::end();
            }
            if(!neediconpass) break;
        }
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
