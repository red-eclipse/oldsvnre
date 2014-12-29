// menus.cpp: ingame menu system (also used for scores and serverlist)

#include "engine.h"

FVAR(IDF_PERSIST, menuscale, 0, 0.02f, 1);
VAR(0, guipasses, 1, -1, -1);

struct menu;

static guient *cgui = NULL;
static menu *cmenu = NULL;

struct menu : guicb
{
    char *name, *header;
    uint *contents, *initscript;
    int passes, menutab, menustart;
    bool world, useinput, usetitle, usebgfx, builtin;

    menu() : name(NULL), header(NULL), contents(NULL), initscript(NULL), passes(0), menutab(0), menustart(0), world(false), useinput(true), usetitle(true), usebgfx(true), builtin(false) {}

    void gui(guient &g, bool firstpass)
    {
        cgui = &g;
        cmenu = this;
        guipasses = passes;
        if(!passes) guiactionon = false;
        int oldflags = identflags;
        if(world && !builtin) identflags |= IDF_WORLD;
        if(initscript) execute(initscript);
        cgui->start(menustart, menuscale, &menutab, useinput, usetitle, usebgfx);
        cgui->tab(header ? header : name);
        if(contents) execute(contents);
        cgui->end();
        identflags = oldflags;
        guipasses = -1;
        cmenu = NULL;
        cgui = NULL;
        if((++passes) <= 0) passes = 1;
    }

    virtual void clear() {}
};

struct delayedupdate
{
    enum
    {
        INT,
        FLOAT,
        STRING,
        ACTION
    } type;
    ident *id;
    union
    {
        int i;
        float f;
        char *s;
    } val;
    bool world;
    delayedupdate() : type(ACTION), id(NULL) { val.s = NULL; }
    ~delayedupdate() { if(type == STRING || type == ACTION) DELETEA(val.s); }

    void schedule(const char *s) { type = ACTION; val.s = newstring(s); world = (identflags&IDF_WORLD)!=0; }
    void schedule(ident *var, int i) { type = INT; id = var; val.i = i; world = (identflags&IDF_WORLD)!=0; }
    void schedule(ident *var, float f) { type = FLOAT; id = var; val.f = f; world = (identflags&IDF_WORLD)!=0; }
    void schedule(ident *var, char *s) { type = STRING; id = var; val.s = newstring(s); world = (identflags&IDF_WORLD)!=0; }

    int getint() const
    {
        switch(type)
        {
            case INT: return val.i;
            case FLOAT: return int(val.f);
            case STRING: return int(strtol(val.s, NULL, 0));
            default: return 0;
        }
    }

    float getfloat() const
    {
        switch(type)
        {
            case INT: return float(val.i);
            case FLOAT: return val.f;
            case STRING: return float(parsefloat(val.s));
            default: return 0;
        }
    }

    const char *getstring() const
    {
        switch(type)
        {
            case INT: return intstr(val.i);
            case FLOAT: return intstr(int(floor(val.f)));
            case STRING: return val.s;
            default: return "";
        }
    }

    void run()
    {
        int _oldflags = identflags;
        if(world) identflags |= IDF_WORLD;
        if(type == ACTION) { if(val.s) execute(val.s); }
        else if(id) switch(id->type)
        {
            case ID_VAR: setvarchecked(id, getint()); break;
            case ID_FVAR: setfvarchecked(id, getfloat()); break;
            case ID_SVAR: setsvarchecked(id, getstring()); break;
            case ID_ALIAS: alias(id->name, getstring()); break;
        }
        identflags = _oldflags;
    }
};

static hashtable<const char *, menu> menus;
static vector<menu *> menustack;
static vector<delayedupdate> updatelater;
static bool shouldclearmenu = true, clearlater = false;

void popgui()
{
    menu *m = menustack.pop();
    m->passes = 0;
    m->clear();
}

void removegui(menu *m)
{
    loopv(menustack) if(menustack[i]==m)
    {
        menustack.remove(i);
        m->passes = 0;
        m->clear();
        return;
    }
}

void pushgui(menu *m, int pos = -1, int tab = 0)
{
    if(menustack.empty()) resetcursor();
    if(pos < 0) menustack.add(m);
    else menustack.insert(pos, m);
    if(m)
    {
        m->passes = 0;
        m->menustart = totalmillis;
        if(tab > 0) m->menutab = tab;
        m->usetitle = tab >= 0 ? true : false;
        m->world = (identflags&IDF_WORLD)!=0;
    }
}

void restoregui(int pos, int tab = 0)
{
    int clear = menustack.length()-pos-1;
    loopi(clear) popgui();
    menu *m = menustack.last();
    if(m)
    {
        m->passes = 0;
        m->menustart = totalmillis;
        if(tab > 0) m->menutab = tab;
        m->world = (identflags&IDF_WORLD)!=0;
    }
}

void showgui(const char *name, int tab)
{
    menu *m = menus.access(name);
    if(!m) return;
    int pos = menustack.find(m);
    if(pos<0) pushgui(m, -1, tab);
    else if(pos < menustack.length()-1) restoregui(pos, tab);
    else
    {
        m->world = (identflags&IDF_WORLD)!=0;
        m->menutab = tab;
        return;
    }
    playsound(S_GUIPRESS, camera1->o, camera1, SND_FORCED);
}

extern bool closetexgui();
int cleargui(int n)
{
    if(closetexgui()) n--;
    int clear = menustack.length();
    if(n>0) clear = min(clear, n);
    loopi(clear) popgui();
    if(!menustack.empty()) restoregui(menustack.length()-1);
    return clear;
}

void cleargui_(int *n)
{
    intret(cleargui(*n));
}

void guishowtitle(int *n)
{
    if(!cmenu) return;
    cmenu->usetitle = *n ? true : false;
}

void guishowbgfx(int *n)
{
    if(!cmenu) return;
    cmenu->usebgfx = *n ? true : false;
}

void guistayopen(uint *contents)
{
    bool oldclearmenu = shouldclearmenu;
    shouldclearmenu = false;
    execute(contents);
    shouldclearmenu = oldclearmenu;
}

void guinohitfx(uint *contents)
{
    if(!cgui) return;
    cgui->allowhitfx(false);
    execute(contents);
    cgui->allowhitfx(true);
}

SVAR(0, guirollovername, "");
SVAR(0, guirolloveraction, "");
SVAR(0, guirollovertype, "");

void guibutton(char *name, char *action, char *altact, char *icon, int *colour, int *icolour, int *wrap)
{
    if(!cgui) return;
    int ret = cgui->button(name, *colour >= 0 ? *colour : 0xFFFFFF, *icon ? icon : NULL, *icolour >= 0 ? *icolour : 0xFFFFFF, *wrap > 0 ? *wrap : -1);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact[0] && ret&GUI_ALT) act = altact;
        else if(action[0]) act = action;
        if(act)
        {
            updatelater.add().schedule(act);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirollovername", name, true);
        setsvar("guirolloveraction", action, true);
        setsvar("guirollovertype", "button", true);
    }
}

void guiimage(char *path, char *action, float *scale, int *overlaid, char *altpath, char *altact, int *colour)
{
    if(!cgui) return;
    Texture *t = path[0] ? textureload(path, 0, true, false) : NULL;
    if(t == notexture)
    {
        if(*altpath) t = textureload(altpath, 0, true, false);
        if(t == notexture) return;
    }
    int ret = cgui->image(t, *scale, *overlaid!=0, *colour >= 0 ? *colour : 0xFFFFFF);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact[0] && ret&GUI_ALT) act = altact;
        else if(action[0]) act = action;
        if(act)
        {
            updatelater.add().schedule(act);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirollovername", path, true);
        setsvar("guirolloveraction", action, true);
        setsvar("guirollovertype", "image", true);
    }
}

void guislice(char *path, char *action, float *scale, float *start, float *end, char *text, char *altpath, char *altact)
{
    if(!cgui) return;
    Texture *t = path[0] ? textureload(path, 0, true, false) : NULL;
    if(t == notexture)
    {
        if(*altpath) t = textureload(altpath, 0, true, false);
        if(t == notexture) return;
    }
    int ret = cgui->slice(t, *scale, *start, *end, text[0] ? text : NULL);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact[0] && ret&GUI_ALT) act = altact;
        else if(action[0]) act = action;
        if(act[0])
        {
            updatelater.add().schedule(act);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirollovername", path, true);
        setsvar("guirolloveraction", action, true);
        setsvar("guirollovertype", "image", true);
    }
}

void guitext(char *name, char *icon, int *colour, int *icolour, int *wrap)
{
    if(cgui) cgui->text(name, *colour >= 0 ? *colour : 0xFFFFFF, icon[0] ? icon : NULL, *icolour >= 0 ? *icolour : 0xFFFFFF, *wrap > 0 ? *wrap : -1);
}

void guitab(char *name)
{
    if(cgui) cgui->tab(name);
}

void guistatus(char *str, int *width)
{
    if(cgui) cgui->setstatus("%s", *width, str);
}

void guitooltip(char *str, int *width)
{
    if(cgui) cgui->settooltip("%s", *width, str);
}

void guibar()
{
    if(cgui) cgui->separator();
}

void guifill(int *colour, int *levels)
{
    if(cgui) cgui->fill(*colour, *levels);
}

void guioutline(int *colour, int *levels, int *offsetx, int *offsety)
{
    if(cgui) cgui->outline(*colour, *levels, 0, *offsetx, *offsety);
}

void guibackground(int *colour1, float *blend1, int *colour2, float *blend2, int *skinborder, int *levels)
{
    if(cgui) cgui->background(*colour1, *blend1, *colour2, *blend2, *skinborder!=0, *levels);
}

void guistrut(float *strut, int *alt)
{
    if(cgui)
    {
        if(*alt) cgui->strut(*strut);
        else cgui->space(*strut);
    }
}

void guispring(int *weight)
{
    if(cgui) cgui->spring(max(*weight, 1));
}

void guivisible(uint *body)
{
    if(cgui && cgui->visible()) execute(body);
}

void guivisibletab(uint *body)
{
    if(cgui && cgui->visibletab()) execute(body);
}

void guifont(char *font, uint *body)
{
    if(cgui)
    {
        cgui->pushfont(font);
        execute(body);
        cgui->popfont();
    }
}

int guifontwidth(char *font)
{
    if(font && *font) pushfont(font);
    int width = FONTW;
    if(font && *font) popfont();
    return width;
}

int guifontheight(char *font)
{
    if(font && *font) pushfont(font);
    int height = FONTH;
    if(font && *font) popfont();
    return height;
}

int guitextwidth(char *text, char *font, int wrap)
{
    if(font && *font) pushfont(font);
    int width = 0, height = 0;
    text_bounds(text, width, height, wrap > 0 ? wrap : -1, TEXT_NO_INDENT);
    if(font && *font) popfont();
    return width;
}

int guitextheight(char *text, char *font, int wrap)
{
    if(font && *font) pushfont(font);
    int width = 0, height = 0;
    text_bounds(text, width, height, wrap > 0 ? wrap : -1, TEXT_NO_INDENT);
    if(font && *font) popfont();
    return height;
}

template<class T> static void updateval(char *var, T val, char *onchange)
{
    ident *id = writeident(var);
    updatelater.add().schedule(id, val);
    if(onchange[0]) updatelater.add().schedule(onchange);
}

static int getval(char *var)
{
    ident *id = readident(var);
    if(!id) return 0;
    switch(id->type)
    {
        case ID_VAR: return *id->storage.i;
        case ID_FVAR: return int(*id->storage.f);
        case ID_SVAR: return parseint(*id->storage.s);
        case ID_ALIAS: return id->getint();
        default: return 0;
    }
}

static float getfval(char *var)
{
    ident *id = readident(var);
    if(!id) return 0;
    switch(id->type)
    {
        case ID_VAR: return *id->storage.i;
        case ID_FVAR: return *id->storage.f;
        case ID_SVAR: return parsefloat(*id->storage.s);
        case ID_ALIAS: return id->getfloat();
        default: return 0;
    }
}

static const char *getsval(char *var)
{
    ident *id = getident(var);
    if(!id) return "";
    switch(id->type)
    {
        case ID_VAR: return intstr(*id->storage.i);
        case ID_FVAR: return floatstr(*id->storage.f);
        case ID_SVAR: return *id->storage.s;
        case ID_ALIAS: return id->getstr();
        default: return "";
    }
}

void guiprogress(float *percent, float *scale)
{
    if(!cgui) return;
    cgui->progress(*percent, *scale);
}

void guislider(char *var, int *min, int *max, char *onchange, int *reverse, int *scroll, int *colour, int *style, int *scolour)
{
    if(!cgui || !var || !*var) return;
    int oldval = getval(var), val = oldval, vmin = *max > INT_MIN ? *min : getvarmin(var), vmax = *max > INT_MIN ? *max : getvarmax(var);
    if(vmax >= INT_MAX-1)
    { // not a sane value for a slider..
        int vdef = getvardef(var);
        vmax = vdef > vmin ? vdef*3 : vmin*4;
    }
    cgui->slider(val, vmin, vmax, *colour >= 0 ? *colour : 0xFFFFFF, NULL, *reverse ? true : false, *scroll ? true : false, *style, *scolour);
    if(val != oldval) updateval(var, val, onchange);
}

void guilistslider(char *var, char *list, char *onchange, int *reverse, int *scroll, int *colour, int *style, int *scolour)
{
    if(!cgui) return;
    vector<int> vals;
    list += strspn(list, "\n\t ");
    while(*list)
    {
        vals.add(parseint(list));
        list += strcspn(list, "\n\t \0");
        list += strspn(list, "\n\t ");
    }
    if(vals.empty()) return;
    int val = getval(var), oldoffset = vals.length()-1, offset = oldoffset;
    loopv(vals) if(val <= vals[i]) { oldoffset = offset = i; break; }
    cgui->slider(offset, 0, vals.length()-1, *colour >= 0 ? *colour : 0xFFFFFF, intstr(val), *reverse ? true : false, *scroll ? true : false, *style, *scolour);
    if(offset != oldoffset) updateval(var, vals[offset], onchange);
}

void guinameslider(char *var, char *names, char *list, char *onchange, int *reverse, int *scroll, int *colour, int *style, int *scolour)
{
    if(!cgui) return;
    vector<int> vals;
    list += strspn(list, "\n\t ");
    while(*list)
    {
        vals.add(parseint(list));
        list += strcspn(list, "\n\t \0");
        list += strspn(list, "\n\t ");
    }
    if(vals.empty()) return;
    int val = getval(var), oldoffset = vals.length()-1, offset = oldoffset;
    loopv(vals) if(val <= vals[i]) { oldoffset = offset = i; break; }
    char *label = indexlist(names, offset);
    cgui->slider(offset, 0, vals.length()-1, *colour >= 0 ? *colour : 0xFFFFFF, label, *reverse ? true : false, *scroll ? true : false, *style, *scolour);
    if(offset != oldoffset) updateval(var, vals[offset], onchange);
    delete[] label;
}

void guicheckbox(char *name, char *var, float *on, float *off, char *onchange, int *colour)
{
    if(!cgui) return;
    bool enabled = getfval(var) != *off, two = getfvarmax(var) == 2, next = two && getfval(var) == 1.0f;
    int ret = cgui->button(name, *colour >= 0 ? *colour : 0xFFFFFF, enabled ? (two && !next ? "checkboxtwo" : "checkboxon") : "checkbox", 0xFFFFFF);
    if(ret&GUI_UP) updateval(var, enabled ? (two && next ? 2.0f : *off) : (*on || *off ? *on : 1.0f), onchange);
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirollovername", name, true);
        setsvar("guirolloveraction", var, true);
        setsvar("guirollovertype", "checkbox", true);
    }
}

void guiradio(char *name, char *var, float *n, char *onchange, int *colour)
{
    if(!cgui) return;
    bool enabled = getfval(var)==*n;
    int ret = cgui->button(name, *colour >= 0 ? *colour : 0xFFFFFF, enabled ? "radioboxon" : "radiobox", *colour >= 0 ? *colour : 0xFFFFFF);
    if(ret&GUI_UP)
    {
        if(!enabled) updateval(var, *n, onchange);
    }
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirollovername", name, true);
        setsvar("guirolloveraction", var, true);
        setsvar("guirollovertype", "radio", true);
    }
}

void guibitfield(char *name, char *var, int *mask, char *onchange, int *colour)
{
    if(!cgui) return;
    int val = getval(var);
    bool enabled = (val & *mask) != 0;
    int ret = cgui->button(name, *colour >= 0 ? *colour : 0xFFFFFF, enabled ? "checkboxon" : "checkbox", *colour >= 0 ? *colour : 0xFFFFFF);
    if(ret&GUI_UP) updateval(var, enabled ? val & ~*mask : val | *mask, onchange);
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirollovername", name, true);
        setsvar("guirolloveraction", var, true);
        setsvar("guirollovertype", "bitfield", true);
    }
}

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void guifield(char *var, int *maxlength, char *onchange, int *colour, int *focus, char *parent, int *height, char *prompt, int *immediate)
{
    if(!cgui) return;
    const char *initval = getsval(var);
    char *result = cgui->field(var, *colour >= 0 ? *colour : 0xFFFFFF, *maxlength ? *maxlength : 12, *height, initval, EDITORFOCUSED, *focus!=0, parent, prompt, *immediate!=0);
    if(result && (!*immediate || strcmp(initval, result))) updateval(var, result, onchange);
}

//-ve maxlength indicates a wrapped text field of any (approx 260 chars) length, |maxlength| is the field width
void guieditor(char *name, int *maxlength, int *height, int *mode, int *colour, int *focus, char *parent, char *str, char *prompt)
{
    if(!cgui) return;
    cgui->field(name, *colour >= 0 ? *colour : 0xFFFFFF, *maxlength ? *maxlength : 12, *height, str && *str ? str : NULL, *mode<=0 ? EDITORFOREVER : *mode, *focus!=0, parent, prompt);
    //returns a non-NULL pointer (the currentline) when the user commits, could then manipulate via text* commands
}

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void guikeyfield(char *var, int *maxlength, char *onchange, int *colour, int *focus, char *parent, char *prompt, int *immediate)
{
    if(!cgui) return;
    const char *initval = getsval(var);
    char *result = cgui->keyfield(var, *colour >= 0 ? *colour : 0xFFFFFF, *maxlength ? *maxlength : -8, 0, initval, EDITORFOCUSED, *focus!=0, parent, prompt, *immediate!=0);
    if(result && (!*immediate || strcmp(initval, result))) updateval(var, result, onchange);
}

//use text<action> to do more...

void guibody(uint *contents, char *action, char *altact, uint *onhover)
{
    if(!cgui) return;
    cgui->pushlist(action[0] ? true : false);
    execute(contents);
    int ret = cgui->poplist();
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(ret&GUI_ALT && altact[0]) act = altact;
        else if(action[0]) act = action;
        if(act)
        {
            updatelater.add().schedule(act);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER) execute(onhover);
}

void guilist(uint *contents)
{
    if(!cgui) return;
    cgui->pushlist();
    execute(contents);
    cgui->poplist();
}

void newgui(char *name, char *contents, char *initscript)
{
    menu *m = menus.access(name);
    if(!m)
    {
        name = newstring(name);
        m = &menus[name];
        m->name = name;
    }
    else
    {
        DELETEA(m->header);
        freecode(m->contents);
        freecode(m->initscript);
    }
    m->contents = contents && contents[0] ? compilecode(contents) : NULL;
    m->initscript = initscript && initscript[0] ? compilecode(initscript) : NULL;
}

void guiheader(char *name)
{
    if(!cmenu) return;
    DELETEA(cmenu->header);
    cmenu->header = name && name[0] ? newstring(name) : NULL;
}

void guimodify(char *name, char *contents)
{
    menu *m = menus.access(name);
    if(!m) return;
    freecode(m->contents);
    m->contents = contents && contents[0] ? compilecode(contents) : NULL;
}

COMMAND(0, newgui, "sss");
COMMAND(0, guiheader, "s");
COMMAND(0, guimodify, "ss");
COMMAND(0, guibutton, "ssssbbb");
COMMAND(0, guitext, "ssbbb");
COMMANDN(0, cleargui, cleargui_, "i");
ICOMMAND(0, showgui, "si", (const char *s, int *n), showgui(s, *n));
COMMAND(0, guishowtitle, "i");
COMMAND(0, guishowbgfx, "i");
COMMAND(0, guistayopen, "e");
COMMAND(0, guinohitfx, "e");

COMMAND(0, guilist, "e");
COMMAND(0, guibody, "esse");
COMMAND(0, guibar, "");
COMMAND(0, guifill, "ii");
COMMAND(0, guioutline, "iiii");
COMMAND(0, guibackground, "bgbgii");
COMMAND(0, guistrut,"fi");
COMMAND(0, guispring, "i");
COMMAND(0, guivisible, "e");
COMMAND(0, guivisibletab, "e");
COMMAND(0, guifont,"se");
COMMAND(0, guiimage,"ssfissb");
COMMAND(0, guislice,"ssfffsss");
COMMAND(0, guiprogress,"ff");
COMMAND(0, guislider,"sbbsiibib");
COMMAND(0, guilistslider, "sssiibib");
COMMAND(0, guinameslider, "ssssiibib");
COMMAND(0, guiradio,"ssfsb");
COMMAND(0, guibitfield, "ssisb");
COMMAND(0, guicheckbox, "ssffsb");
COMMAND(0, guitab, "s");
COMMAND(0, guistatus, "si");
COMMAND(0, guitooltip, "si");
COMMAND(0, guifield, "sisbisisi");
COMMAND(0, guikeyfield, "sisbissi");
COMMAND(0, guieditor, "siiibisss");

ICOMMAND(0, guicount, "", (), intret(menustack.length()));
ICOMMAND(0, guifontwidth, "s", (char *font), intret(guifontwidth(font)));
ICOMMAND(0, guifontheight, "s", (char *font), intret(guifontheight(font)));
ICOMMAND(0, guitextwidth, "ssb", (char *text, char *font, int *wrap), intret(guitextwidth(text, font, *wrap)));
ICOMMAND(0, guitextheight, "ssb", (char *text, char *font, int *wrap), intret(guitextheight(text, font, *wrap)));

void guiplayerpreview(int *model, int *color, int *team, int *weap, char *vanity, char *action, float *scale, int *overlaid, float *size, float *blend, char *altact)
{
    if(!cgui) return;
    int ret = cgui->playerpreview(*model, *color, *team, *weap, vanity, *scale, *overlaid!=0, *size!=0 ? *size : 1.f, *blend >= 0 ? *blend : 1.f);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact[0] && ret&GUI_ALT) act = altact;
        else if(action[0]) act = action;
        if(act)
        {
            updatelater.add().schedule(act);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        defformatstring(str)("%d %d %d %d %s", *model, *color, *team, *weap, vanity);
        setsvar("guirollovername", str, true);
        setsvar("guirolloveraction", action, true);
        setsvar("guirollovertype", "player", true);
    }
}
COMMAND(0, guiplayerpreview, "iiiissfifgs");

void guimodelpreview(char *model, char *animspec, char *action, float *scale, int *overlaid, float *size, float *blend, char *altact)
{
    if(!cgui) return;
    int anim = ANIM_ALL;
    if(animspec[0])
    {
        if(isdigit(animspec[0]))
        {
            anim = parseint(animspec);
            if(anim >= 0) anim %= ANIM_INDEX;
            else anim = ANIM_ALL;
        }
        else
        {
            vector<int> anims;
            game::findanims(animspec, anims);
            if(anims.length()) anim = anims[0];
        }
    }
    int ret = cgui->modelpreview(model, anim|ANIM_LOOP, *scale, *overlaid!=0, *size!=0 ? *size : 1.f, *blend!=0 ? *blend : 1.f);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact[0] && ret&GUI_ALT) act = altact;
        else if(action[0]) act = action;
        if(act)
        {
            updatelater.add().schedule(act);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        defformatstring(str)("%d %s", *model, animspec);
        setsvar("guirollovername", str, true);
        setsvar("guirolloveraction", action, true);
        setsvar("guirollovertype", "model", true);
    }
}
COMMAND(0, guimodelpreview, "sssfiffs");

struct change
{
    int type;
    const char *desc;

    change() {}
    change(int type, const char *desc) : type(type), desc(desc) {}
};
static vector<change> needsapply;

static struct applymenu : menu
{
    void gui(guient &g, bool firstpass)
    {
        if(menustack.empty()) return;
        g.start(menustart, menuscale, NULL, true);
        g.text("the following settings have changed:", 0xFFFFFF, "info");
        loopv(needsapply) g.text(needsapply[i].desc, 0xFFFFFF, "info");
        g.separator();
        g.text("apply changes now?", 0xFFFFFF, "info");
        if(g.button("yes", 0xFFFFFF, "action")&GUI_UP)
        {
            int changetypes = 0;
            loopv(needsapply) changetypes |= needsapply[i].type;
            if(changetypes&CHANGE_GFX) updatelater.add().schedule("resetgl");
            if(changetypes&CHANGE_SOUND) updatelater.add().schedule("resetsound");
            clearlater = true;
        }
        if(g.button("no", 0xFFFFFF, "action")&GUI_UP)
            clearlater = true;
        g.end();
    }

    void clear()
    {
        needsapply.shrink(0);
    }
} applymenu;

VAR(IDF_PERSIST, applydialog, 0, 1, 1);

void addchange(const char *desc, int type, bool force)
{
    if(!applydialog || force)
    {
        int changetypes = type;
        if(menustack.find(&applymenu) >= 0)
        {
            loopv(needsapply) changetypes |= needsapply[i].type;
            clearlater = true;
        }
        if(changetypes&CHANGE_GFX) updatelater.add().schedule("resetgl");
        if(changetypes&CHANGE_SOUND) updatelater.add().schedule("resetsound");
    }
    else
    {
        loopv(needsapply) if(!strcmp(needsapply[i].desc, desc)) return;
        needsapply.add(change(type, desc));
        if(needsapply.length() && menustack.find(&applymenu) < 0)
            pushgui(&applymenu, max(menustack.length()-1, 0));
    }
}

void clearchanges(int type)
{
    loopv(needsapply)
    {
        if(needsapply[i].type&type)
        {
            needsapply[i].type &= ~type;
            if(!needsapply[i].type) needsapply.remove(i--);
        }
    }
    if(needsapply.empty()) removegui(&applymenu);
}

void menuprocess()
{
    int level = menustack.length();
    interactive = true;
    loopv(updatelater) updatelater[i].run();
    updatelater.shrink(0);
    interactive = false;
    if(clearlater)
    {
        if(level==menustack.length()) loopi(level) popgui();
        clearlater = false;
    }
}

void progressmenu()
{
    menu *m = menus.access("loading");
    if(m)
    { // it doesn't need to exist
        m->usetitle = m->useinput = m->world = false;
        m->builtin = true;
        UI::addcb(m);
    }
}

void mainmenu()
{
    if(!menustack.empty()) UI::addcb(menustack.last());
}

bool menuactive()
{
    return !menustack.empty();
}

ICOMMAND(0, menustacklen, "", (void), intret(menustack.length()));

void guiirc(const char *s)
{
    extern bool ircgui(guient *g, const char *s);
    if(cgui)
    {
        if(!ircgui(cgui, s) && shouldclearmenu) clearlater = true;
    }
}
ICOMMAND(0, ircgui, "s", (char *s), guiirc(s));
