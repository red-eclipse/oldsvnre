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
    bool world, useinput, usetitle;

    menu() : name(NULL), header(NULL), contents(NULL), initscript(NULL), passes(0), menutab(0), menustart(0), world(false), useinput(true), usetitle(true) {}

    void gui(guient &g, bool firstpass)
    {
        cgui = &g;
        cmenu = this;
        guipasses = passes;
        if(!passes) world = (identflags&IDF_WORLD)!=0;
        if(initscript)
        {
            if(world && passes) { WITHWORLD(execute(initscript)); }
            else execute(initscript);
        }
        cgui->start(menustart, menuscale, &menutab, useinput, usetitle);
        cgui->tab(header ? header : name);
        if(contents)
        {
            if(world && passes) { WITHWORLD(execute(contents)); }
            else execute(contents);
        }
        cgui->end();
        guipasses = -1;
        cmenu = NULL;
        cgui = NULL;
        passes++;
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
    delayedupdate() : type(ACTION), id(NULL) { val.s = NULL; }
    ~delayedupdate() { if(type == STRING || type == ACTION) DELETEA(val.s); }

    void schedule(const char *s) { type = ACTION; val.s = newstring(s); }
    void schedule(ident *var, int i) { type = INT; id = var; val.i = i; }
    void schedule(ident *var, float f) { type = FLOAT; id = var; val.f = f; }
    void schedule(ident *var, char *s) { type = STRING; id = var; val.s = newstring(s); }

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
        if(type == ACTION) { if(val.s) execute(val.s); }
        else if(id) switch(id->type)
        {
            case ID_VAR: setvarchecked(id, getint()); break;
            case ID_FVAR: setfvarchecked(id, getfloat()); break;
            case ID_SVAR: setsvarchecked(id, getstring()); break;
            case ID_ALIAS: alias(id->name, getstring()); break;
        }
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
    }
}

void showgui(const char *name, int tab)
{
    menu *m = menus.access(name);
    if(!m) return;
    int pos = menustack.find(m);
    if(pos<0) pushgui(m, -1, tab);
    else restoregui(pos, tab);
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

void guistayopen(uint *contents)
{
    bool oldclearmenu = shouldclearmenu;
    shouldclearmenu = false;
    execute(contents);
    shouldclearmenu = oldclearmenu;
}

void guinoautotab(uint *contents)
{
    if(!cgui) return;
    cgui->allowautotab(false);
    execute(contents);
    cgui->allowautotab(true);
}

void guinohitfx(uint *contents)
{
    if(!cgui) return;
    cgui->allowhitfx(false);
    execute(contents);
    cgui->allowhitfx(true);
}

//@DOC name and icon are optional
SVAR(0, guirollovername, "");
SVAR(0, guirolloveraction, "");

void guibutton(char *name, char *action, char *altact, char *icon, int *colour)
{
    if(!cgui) return;
    int ret = cgui->button(name, *colour ? *colour : 0xFFFFFF, *icon ? icon : NULL);
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
    }
}

SVAR(0, guirolloverimgpath, "");
SVAR(0, guirolloverimgaction, "");

void guiimage(char *path, char *action, float *scale, int *overlaid, char *altpath, char *altact)
{
    if(!cgui) return;
    Texture *t = path[0] ? textureload(path, 0, true, false) : NULL;
    if(t == notexture)
    {
        if(*altpath) t = textureload(altpath, 0, true, false);
        if(t == notexture) return;
    }
    int ret = cgui->image(t, *scale, *overlaid!=0);
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
        setsvar("guirolloverimgpath", path, true);
        setsvar("guirolloverimgaction", action, true);
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
        setsvar("guirolloverimgpath", path, true);
        setsvar("guirolloverimgaction", action, true);
    }
}

void guitext(char *name, char *icon, int *colour, int *icolour)
{
    if(cgui) cgui->text(name, *colour ? *colour : 0xFFFFFF, icon[0] ? icon : NULL, *icolour ? *icolour : 0xFFFFFF);
}

void guititle(char *name)
{
    if(cgui) cgui->title(name);
}

void guitab(char *name)
{
    if(cgui) cgui->tab(name);
}

void guibar()
{
    if(cgui) cgui->separator();
}

void guibackground(int *colour, int *levels)
{
    if(cgui) cgui->background(*colour, *levels);
}

void guistrut(float *strut, int *alt)
{
    if(cgui)
    {
        if(*alt) cgui->strut(*strut); else cgui->space(*strut);
    }
}

void guispring(int *weight)
{
    if(cgui) cgui->spring(max(*weight, 1));
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

void guislider(char *var, int *min, int *max, char *onchange, int *reverse, int *scroll, int *colour)
{
    if(!cgui) return;
    int oldval = getval(var), val = oldval, vmin = *max ? *min : getvarmin(var), vmax = *max ? *max : getvarmax(var);
    if(vmax >= INT_MAX-1)
    { // not a sane value for a slider..
        int vdef = getvardef(var);
        vmax = vdef > vmin ? vdef*3 : vmin*4;
    }
    cgui->slider(val, vmin, vmax, *colour ? *colour : 0xFFFFFF, NULL, *reverse ? true : false, *scroll ? true : false);
    if(val != oldval) updateval(var, val, onchange);
}

void guilistslider(char *var, char *list, char *onchange, int *reverse, int *scroll, int *colour)
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
    cgui->slider(offset, 0, vals.length()-1, *colour ? *colour : 0xFFFFFF, intstr(val), *reverse ? true : false, *scroll ? true : false);
    if(offset != oldoffset) updateval(var, vals[offset], onchange);
}

void guinameslider(char *var, char *names, char *list, char *onchange, int *reverse, int *scroll, int *colour)
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
    cgui->slider(offset, 0, vals.length()-1, *colour ? *colour : 0xFFFFFF, label, *reverse ? true : false, *scroll ? true : false);
    if(offset != oldoffset) updateval(var, vals[offset], onchange);
    delete[] label;
}

void guicheckbox(char *name, char *var, float *on, float *off, char *onchange, int *colour)
{
    bool enabled = getfval(var) != *off, two = getfvarmax(var) == 2, next = two && getfval(var) == 1.0f;
    if(cgui && cgui->button(name, *colour ? *colour : 0xFFFFFF, enabled ? (two && !next ? "checkboxtwo" : "checkboxon") : "checkbox", 0xFFFFFF, enabled ? false : true)&GUI_UP)
    {
        updateval(var, enabled ? (two && next ? 2.0f : *off) : (*on || *off ? *on : 1.0f), onchange);
    }
}

void guiradio(char *name, char *var, float *n, char *onchange, int *colour)
{
    bool enabled = getfval(var)==*n;
    if(cgui && cgui->button(name, *colour ? *colour : 0xFFFFFF, enabled ? "radioboxon" : "radiobox", *colour ? *colour : 0xFFFFFF, enabled ? false : true)&GUI_UP)
    {
        if(!enabled) updateval(var, *n, onchange);
    }
}

void guibitfield(char *name, char *var, int *mask, char *onchange, int *colour)
{
    int val = getval(var);
    bool enabled = (val & *mask) != 0;
    if(cgui && cgui->button(name, *colour ? *colour : 0xFFFFFF, enabled ? "checkboxon" : "checkbox", *colour ? *colour : 0xFFFFFF, enabled ? false : true)&GUI_UP)
    {
        updateval(var, enabled ? val & ~*mask : val | *mask, onchange);
    }
}

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void guifield(char *var, int *maxlength, char *onchange, int *colour)
{
    if(!cgui) return;
    const char *initval = getsval(var);
    char *result = cgui->field(var, *colour ? *colour : 0xFFFFFF, *maxlength ? *maxlength : 12, 0, initval);
    if(result) updateval(var, result, onchange);
}

//-ve maxlength indicates a wrapped text field of any (approx 260 chars) length, |maxlength| is the field width
void guieditor(char *name, int *maxlength, int *height, int *mode, int *colour)
{
    if(!cgui) return;
    cgui->field(name, *colour ? *colour : 0xFFFFFF, *maxlength ? *maxlength : 12, *height, NULL, *mode<=0 ? EDITORFOREVER : *mode);
    //returns a non-NULL pointer (the currentline) when the user commits, could then manipulate via text* commands
}

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void guikeyfield(char *var, int *maxlength, char *onchange, int *colour)
{
    if(!cgui) return;
    const char *initval = getsval(var);
    char *result = cgui->keyfield(var, *colour ? *colour : 0xFFFFFF, *maxlength ? *maxlength : -8, 0, initval);
    if(result) updateval(var, result, onchange);
}

//use text<action> to do more...

void guibody(uint *contents, char *action, char *altact)
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
COMMAND(0, guibutton, "ssssi");
COMMAND(0, guitext, "ssii");
COMMANDN(0, cleargui, cleargui_, "i");
ICOMMAND(0, showgui, "si", (const char *s, int *n), showgui(s, *n));
COMMAND(0, guishowtitle, "i");
COMMAND(0, guistayopen, "e");
COMMAND(0, guinoautotab, "e");
COMMAND(0, guinohitfx, "e");

ICOMMAND(0, guicount, "", (), intret(menustack.length()));

COMMAND(0, guilist, "e");
COMMAND(0, guibody, "ess");
COMMAND(0, guititle, "s");
COMMAND(0, guibar, "");
COMMAND(0, guibackground, "ii");
COMMAND(0, guistrut,"fi");
COMMAND(0, guispring, "i");
COMMAND(0, guifont,"se");
COMMAND(0, guiimage,"ssfiss");
COMMAND(0, guislice,"ssfffsss");
COMMAND(0, guiprogress,"ff");
COMMAND(0, guislider,"siisiii");
COMMAND(0, guilistslider, "sssiii");
COMMAND(0, guinameslider, "ssssiii");
COMMAND(0, guiradio,"ssfsi");
COMMAND(0, guibitfield, "ssisi");
COMMAND(0, guicheckbox, "ssffsi");
COMMAND(0, guitab, "s");
COMMAND(0, guifield, "sisi");
COMMAND(0, guikeyfield, "sisi");
COMMAND(0, guieditor, "siiii");

void guiplayerpreview(int *model, int *color, int *team, int *weap, char *action, float *scale, int *overlaid, char *altact)
{
    if(!cgui) return;
    int ret = cgui->playerpreview(*model, *color, *team, *weap, *scale, *overlaid!=0);
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
}
COMMAND(0, guiplayerpreview, "iiiisfis");

void guimodelpreview(char *model, char *animspec, char *action, float *scale, int *overlaid, char *altact)
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
    int ret = cgui->modelpreview(model, anim|ANIM_LOOP, *scale, *overlaid!=0);
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
}
COMMAND(0, guimodelpreview, "sssfis");

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
        g.start(menustart, menuscale, &menutab, true);
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
    {
        m->usetitle = m->useinput = false;
        UI::addcb(m);
    }
    else conoutf("cannot find menu 'loading'");
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
