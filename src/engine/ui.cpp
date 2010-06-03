#include "engine.h"

int uimillis = -1;

static bool layoutpass, actionon = false;
static int mouseaction[2] = {0}, guibound[2] = {0};

static float firstx, firsty;

enum {FIELDCOMMIT, FIELDABORT, FIELDEDIT, FIELDSHOW, FIELDKEY};
static int fieldmode = FIELDSHOW;
static bool fieldsactive = false;

VAR(IDF_PERSIST, guishadow, 0, 2, 8);
VAR(IDF_PERSIST, guiautotab, 6, 16, 40);
VAR(IDF_PERSIST, guiclicktab, 0, 1, 1);
VAR(IDF_PERSIST, guiblend, 1, 156, 255);
VAR(IDF_PERSIST, guilinesize, 1, 36, 128);

static bool needsinput = false, hastitle = true;

#include "textedit.h"
struct gui : guient
{
    struct list { int parent, w, h, mouse[2]; };

    int nextlist;
    static vector<list> lists;
    static float hitx, hity;
    static int curdepth, curlist, xsize, ysize, curx, cury, fontdepth, mergelist, mergedepth;
    static bool shouldautotab;

    static void reset() { lists.shrink(0); mergelist = mergedepth = -1; }

    static int ty, tx, tpos, *tcurrent, tcolor; //tracking tab size and position since uses different layout method...

    void allowautotab(bool on) { shouldautotab = on; }

    void autotab()
    {
        if(tcurrent)
        {
            if(layoutpass && !tpos) tcurrent = NULL; //disable tabs because you didn't start with one
            if(shouldautotab && !curdepth && (layoutpass ? 0 : cury) + ysize > guiautotab*guibound[1]) tab(NULL, tcolor);
        }
    }

    bool shouldtab()
    {
        if(tcurrent && shouldautotab)
        {
            if(layoutpass)
            {
                int space = guiautotab*guibound[1] - ysize;
                if(space < 0) return true;
                int l = lists[curlist].parent;
                while(l >= 0)
                {
                    space -= lists[l].h;
                    if(space < 0) return true;
                    l = lists[l].parent;
                }
            }
            else
            {
                int space = guiautotab*guibound[1] - cury;
                if(ysize > space) return true;
                int l = lists[curlist].parent;
                while(l >= 0)
                {
                    if(lists[l].h > space) return true;
                    l = lists[l].parent;
                }
            }
        }
        return false;
    }

    bool visible() { return (!tcurrent || tpos==*tcurrent) && !layoutpass; }

    //tab is always at top of page
    void tab(const char *name, int color)
    {
        if(curdepth != 0) return;
        tpos++;
        if(!hastitle)
        {
            if(layoutpass)
            {
                ty = max(ty, ysize);
                ysize = 0;
            }
            else cury = -ysize;
            return;
        }
        if(color) tcolor = color;
        if(!name)
        {
            static string title;
            formatstring(title)("%d", tpos);
            name = title;
        }
        defformatstring(tabtitle)("\fs\fd[\fS%s%s%s\fs\fd]\fS", visible() ? " " : "", name, visible() ? " " : "");
        gui::pushfont(visible() ? "super" : "default");
        int w = text_width(tabtitle);
        if(layoutpass)
        {
            ty = max(ty, ysize);
            ysize = 0;
        }
        else
        {
            cury = -ysize;
            int x1 = curx + tx + guibound[0], x2 = x1 + w, y1 = cury - guibound[1]*2, y2 = cury - guibound[1]*2 + FONTH, alpha = guiblend;
            if(visible()) { tcolor = 0xFFFFFF; alpha = 255; }
            else if(tcurrent && hitx>=x1 && hity>=y1 && hitx<x2 && hity<y2)
            {
                if(!guiclicktab || mouseaction[0]&GUI_UP) *tcurrent = tpos; // switch tab
                tcolor = 0xFF2222;
                alpha = max(guiblend, 200);
            }
            text_(tabtitle, x1, y1, tcolor, alpha, visible());
        }
        tx += w + guibound[0]*2;
        gui::popfont();
    }

    void uibuttons()
    {
        int x = curx+xsize-guibound[0]/2, y = -ysize-guibound[1]*2;
        #define uibtn(a,b) \
        { \
            bool hit = false; \
            if(hitx>=x && hity>=y && hitx<x+guibound[1] && hity<y+guibound[1]) \
            { \
                if(mouseaction[0]&GUI_UP) { b; } \
                hit = true; \
            } \
            icon_(textureload(a, 3, true, false), false, x, y, guibound[1], !hit); \
            y += guibound[1]*3/2; \
        }
        uibtn("textures/exit", cleargui(1));
    }

    bool ishorizontal() const { return curdepth&1; }
    bool isvertical() const { return !ishorizontal(); }

    void pushlist(bool merge)
    {
        if(layoutpass)
        {
            if(curlist>=0)
            {
                lists[curlist].w = xsize;
                lists[curlist].h = ysize;
            }
            list &l = lists.add();
            l.parent = curlist;
            curlist = lists.length()-1;
            l.mouse[0] = l.mouse[1] = xsize = ysize = 0;
        }
        else
        {
            curlist = nextlist++;
            xsize = lists[curlist].w;
            ysize = lists[curlist].h;
        }
        curdepth++;
        if(!layoutpass && visible() && ishit(xsize, ysize)) loopi(2) lists[curlist].mouse[i] = mouseaction[i]|GUI_ROLLOVER;
        if(merge)
        {
            mergelist = curlist;
            mergedepth = curdepth;
        }
    }

    int poplist()
    {
        list &l = lists[curlist];
        if(layoutpass)
        {
            l.w = xsize;
            l.h = ysize;
        }
        curlist = l.parent;
        curdepth--;
        if(mergelist >= 0 && curdepth < mergedepth) mergelist = mergedepth = -1;
        if(curlist >= 0)
        {
            xsize = lists[curlist].w;
            ysize = lists[curlist].h;
            if(ishorizontal()) cury -= l.h;
            else curx -= l.w;
            return layout(l.w, l.h);
        }
        return 0;
    }

    int text  (const char *text, int color, const char *icon) { autotab(); return button_(text, color, icon, false, false); }
    int button(const char *text, int color, const char *icon, bool faded) { autotab(); return button_(text, color, icon, true, faded); }
    int title (const char *text, int color, const char *icon) { autotab(); return button_(text, color, icon, false, false, "emphasis"); }

    void separator() { autotab(); line_(5); }

    //use to set min size (useful when you have progress bars)
    void strut(float size) { layout(isvertical() ? int(size*guibound[0]) : 0, isvertical() ? 0 : int(size*guibound[1])); }
    //add space between list items
    void space(float size) { layout(isvertical() ? 0 : size*guibound[0], isvertical() ? size*guibound[1] : 0); }

    void pushfont(const char *font) { ::pushfont(font); fontdepth++; }
    void popfont() { if(fontdepth) { ::popfont(); fontdepth--; } }

    int layout(int w, int h)
    {
        if(layoutpass)
        {
            if(ishorizontal())
            {
                xsize += w;
                ysize = max(ysize, h);
            }
            else
            {
                xsize = max(xsize, w);
                ysize += h;
            }
        }
        else
        {
            bool hit = ishit(w, h);
            if(ishorizontal()) curx += w;
            else cury += h;
            if(hit && visible()) return mouseaction[0]|GUI_ROLLOVER;
        }
        return 0;
    }

    bool ishit(int w, int h, int x = curx, int y = cury)
    {
        if(mergelist >= 0 && curdepth >= mergedepth && lists[mergelist].mouse[0]) return true;
        if(ishorizontal()) h = ysize;
        else w = xsize;
        return hitx>=x && hity>=y && hitx<x+w && hity<y+h;
    }

    int image(Texture *t, float scale, bool overlaid)
    {
        autotab();
        if(scale == 0) scale = 1;
        int size = (int)(scale*2*guibound[1])-guishadow;
        if(visible()) icon_(t, overlaid, curx, cury, size, ishit(size+guishadow, size+guishadow));
        return layout(size+guishadow, size+guishadow);
    }

    int texture(VSlot &vslot, float scale, bool overlaid)
    {
        autotab();
        if(scale==0) scale = 1;
        int size = (int)(scale*2*guibound[1])-guishadow;
        if(visible()) previewslot(vslot, overlaid, curx, cury, size, ishit(size+guishadow, size+guishadow));
        return layout(size+guishadow, size+guishadow);
    }

    int slice(Texture *t, float scale, float start, float end, const char *text)
    {
        autotab();
        if(scale == 0) scale = 1;
        int size = (int)(scale*2*guibound[1]);
        if(t!=notexture && visible()) slice_(t, curx, cury, size, start, end, text);
        return layout(size, size);
    }

    void progress(float percent, float scale)
    {
        autotab();
        Texture *t = textureload(hud::progresstex, 3, true, false);
        if(scale == 0) scale = 1;
        int size = (int)(scale*2*guibound[1]), part = size*2/3;
        slice_(t, curx+part/8, cury+part/8, part, 0, percent);
        string s; if(percent > 0) formatstring(s)("\fg%d%%", int(percent*100)); else formatstring(s)("\fgload");
        slice_(t, curx, cury, size, (SDL_GetTicks()%1000)/1000.f, 0.1f, s);
        layout(size, size);
    }

    void slider(int &val, int vmin, int vmax, int color, char *label, bool reverse, bool scroll)
    {
        autotab();
        int x = curx;
        int y = cury;
        line_(guilinesize);
        if(visible())
        {
            pushfont("emphasis");
            if(!label)
            {
                static string s;
                formatstring(s)("%d", val);
                label = s;
            }
            int w = text_width(label);

            bool hit = false;
            int px, py;
            if(ishorizontal())
            {
                hit = ishit(guilinesize, ysize, x, y);
                px = x + (guilinesize-w)/2;
                if(reverse) py = y + ((ysize-guibound[1])*(val-vmin))/((vmax==vmin) ? 1 : (vmax-vmin)); //vmin at top
                else py = y + (ysize-guibound[1]) - ((ysize-guibound[1])*(val-vmin))/((vmax==vmin) ? 1 : (vmax-vmin)); //vmin at bottom
            }
            else
            {
                hit = ishit(xsize, guilinesize, x, y);
                if(reverse) px = x + (xsize-guibound[0]/2-w/2) - ((xsize-w)*(val-vmin))/((vmax==vmin) ? 1 : (vmax-vmin)); //vmin at right
                else px = x + guibound[0]/2 - w/2 + ((xsize-w)*(val-vmin))/((vmax==vmin) ? 1 : (vmax-vmin)); //vmin at left
                py = y;
            }
            text_(label, px, py, hit ? 0xFF2222 : color, hit ? 255 : guiblend, hit && mouseaction[0]&GUI_DOWN);
            if(hit)
            {
                if(mouseaction[0]&GUI_PRESSED)
                {
                    int vnew = (vmin < vmax ? 1 : -1)+vmax-vmin;
                    if(ishorizontal()) vnew = reverse ? int(vnew*(hity-y-guibound[1]/2)/(ysize-guibound[1])) : int(vnew*(y+ysize-guibound[1]/2-hity)/(ysize-guibound[1]));
                    else vnew = reverse ? int(vnew*(x+xsize-guibound[0]/2-hitx)/(xsize-w)) : int(vnew*(hitx-x-guibound[0]/2)/(xsize-w));
                    vnew += vmin;
                    vnew = vmin < vmax ? clamp(vnew, vmin, vmax) : clamp(vnew, vmax, vmin);
                    if(vnew != val) val = vnew;
                }
                else if(mouseaction[1]&GUI_UP)
                {
                    int vval = val+((reverse ? !(mouseaction[1]&GUI_ALT) : (mouseaction[1]&GUI_ALT)) ? -1 : 1),
                        vnew = vmin < vmax ? clamp(vval, vmin, vmax) : clamp(vval, vmax, vmin);
                    if(vnew != val) val = vnew;
                }
            }
            else if(scroll && lists[curlist].mouse[1]&GUI_UP)
            {
                int vval = val+((reverse ? !(lists[curlist].mouse[1]&GUI_ALT) : (lists[curlist].mouse[1]&GUI_ALT)) ? -1 : 1),
                    vnew = vmin < vmax ? clamp(vval, vmin, vmax) : clamp(vval, vmax, vmin);
                if(vnew != val) val = vnew;
            }
            popfont();
        }
    }

    char *field(const char *name, int color, int length, int height, const char *initval, int initmode)
    {
        return field_(name, color, length, height, initval, initmode, FIELDEDIT, "console");
    }

    char *keyfield(const char *name, int color, int length, int height, const char *initval, int initmode)
    {
        return field_(name, color, length, height, initval, initmode, FIELDKEY, "console");
    }

    char *field_(const char *name, int color, int length, int height, const char *initval, int initmode, int fieldtype = FIELDEDIT, const char *font = "")
    {
        if(font && *font) gui::pushfont(font);
        editor *e = useeditor(name, initmode, false, initval); // generate a new editor if necessary
        if(layoutpass)
        {
            if(initval && e->mode==EDITORFOCUSED && (e!=currentfocus() || fieldmode == FIELDSHOW))
            {
                if(strcmp(e->lines[0].text, initval)) e->clear(initval);
            }
            e->linewrap = (length<0);
            e->maxx = (e->linewrap) ? -1 : length;
            e->maxy = (height<=0)?1:-1;
            e->pixelwidth = abs(length)*guibound[0];
            if(e->linewrap && e->maxy==1)
            {
                int temp;
                text_bounds(e->lines[0].text, temp, e->pixelheight, e->pixelwidth); //only single line editors can have variable height
            }
            else
                e->pixelheight = guibound[1]*max(height, 1);
        }
        int h = e->pixelheight;
        int w = e->pixelwidth + guibound[0];

        bool wasvertical = isvertical();
        if(wasvertical && e->maxy != 1) pushlist(false);

        char *result = NULL;
        if(visible())
        {
            e->rendered = true;

            bool hit = ishit(w, h);
            bool editing = (fieldmode != FIELDSHOW) && (e==currentfocus());
            if(mouseaction[0]&GUI_UP && mergedepth >= 0 && hit) mouseaction[0] &= ~GUI_UP;
            if(mouseaction[0]&GUI_DOWN) //mouse request focus
            {
                if(hit)
                {
                    if(fieldtype==FIELDKEY) e->clear();
                    useeditor(e->name, initmode, true);
                    e->mark(false);
                    fieldmode = fieldtype;
                }
                else if(editing)
                {
                    fieldmode = FIELDCOMMIT;
                    e->mode = EDITORFOCUSED;
                }
            }
            if(hit && editing && (mouseaction[0]&GUI_PRESSED)!=0 && fieldtype==FIELDEDIT)
                e->hit(int(floor(hitx-(curx+guibound[0]/2))), int(floor(hity-cury)), (mouseaction[0]&GUI_DRAGGED)!=0); //mouse request position
            if(editing && (fieldmode==FIELDCOMMIT || fieldmode==FIELDABORT)) // commit field if user pressed enter
            {
                if(fieldmode==FIELDCOMMIT) result = e->currentline().text;
                e->active = (e->mode!=EDITORFOCUSED);
                fieldmode = FIELDSHOW;
            }
            else fieldsactive = true;

            e->draw(curx+guibound[0]/2, cury, color, editing);

            lineshader->set();
            glDisable(GL_TEXTURE_2D);
            if(editing) glColor3f(0.5f, 0.125f, 0.125f);
            else glColor3ub(color>>16, (color>>8)&0xFF, color&0xFF);
            rect_(curx, cury, w, h, -1, true);
            glEnable(GL_TEXTURE_2D);
            defaultshader->set();
        }
        layout(w, h);
        if(e->maxy != 1)
        {
            int slines = e->lines.length()-e->pixelheight/guibound[1];
            if(slines > 0)
            {
                int pos = e->scrolly;
                slider(e->scrolly, slines, 0, color, NULL, false, true);
                if(pos != e->scrolly) e->cy = e->scrolly;
            }
            if(wasvertical) poplist();
        }
        if(font && *font) gui::popfont();
        return result;
    }

    void fieldline(const char *name, const char *str)
    {
        if(!layoutpass) return;
        loopv(editors) if(strcmp(editors[i]->name, name) == 0)
        {
            editor *e = editors[i];
            e->lines.add().set(str);
            e->mark(false);
            return;
        }
    }

    void fieldclear(const char *name, const char *init)
    {
        if(!layoutpass) return;
        loopvrev(editors) if(strcmp(editors[i]->name, name) == 0)
        {
            editor *e = editors[i];
            e->clear(init);
            return;
        }
    }

    int fieldedit(const char *name)
    {
        loopvrev(editors) if(strcmp(editors[i]->name, name) == 0)
        {
            editor *e = editors[i];
            useeditor(e->name, e->mode, true);
            e->mark(false);
            e->cx = e->cy = 0;
            fieldmode = FIELDEDIT;
            return fieldmode;
        }
        return fieldmode;
    }

    void fieldscroll(const char *name, int n)
    {
        if(n < 0 && mouseaction[0]&GUI_PRESSED) return; // don't auto scroll during edits
        if(!layoutpass) return;
        loopv(editors) if(strcmp(editors[i]->name, name) == 0)
        {
            editor *e = editors[i];
            e->scrolly = e->cx = 0;
            e->cy = n >= 0 ? n : e->lines.length();
            return;
        }
    }

    void rect_(float x, float y, float w, float h, int usetc = -1, bool lines = false)
    {
        glBegin(lines ? GL_LINE_LOOP : GL_TRIANGLE_STRIP);
        static const GLfloat tc[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        if(usetc>=0) glTexCoord2fv(tc[usetc]);
        glVertex2f(x, y);
        if(usetc>=0) glTexCoord2fv(tc[(usetc+1)%4]);
        glVertex2f(x + w, y);
        if(lines)
        {
            if(usetc>=0) glTexCoord2fv(tc[(usetc+2)%4]);
            glVertex2f(x + w, y + h);
        }
        if(usetc>=0) glTexCoord2fv(tc[(usetc+3)%4]);
        glVertex2f(x, y + h);
        if(!lines)
        {
            if(usetc>=0) glTexCoord2fv(tc[(usetc+2)%4]);
            glVertex2f(x + w, y + h);
        }
        glEnd();
        xtraverts += 4;

    }

    void text_(const char *text, int x, int y, int color, int alpha, bool shadow)
    {
        if(FONTH < guibound[1]) y += (guibound[1]-FONTH)/2;
        else if(FONTH > guibound[1]) y -= (FONTH-guibound[1])/2;
        if(shadow) draw_text(text, x+guishadow, y+guishadow, 0x00, 0x00, 0x00, 0xC0*alpha/255);
        draw_text(text, x, y, color>>16, (color>>8)&0xFF, color&0xFF, alpha);
    }

    void background(int color, int inheritw, int inherith)
    {
        if(layoutpass) return;
        glDisable(GL_TEXTURE_2D);
        notextureshader->set();
        glColor4ub(color>>16, (color>>8)&0xFF, color&0xFF, 0x80);
        int w = xsize, h = ysize;
        if(inheritw>0)
        {
            int parentw = curlist;
            while(inheritw>0 && lists[parentw].parent>=0)
            {
                parentw = lists[parentw].parent;
                inheritw--;
            }
            w = lists[parentw].w;
        }
        if(inherith>0)
        {
            int parenth = curlist;
            while(inherith>0 && lists[parenth].parent>=0)
            {
                parenth = lists[parenth].parent;
                inherith--;
            }
            h = lists[parenth].h;
        }
        rect_(curx, cury, w, h);
        glEnable(GL_TEXTURE_2D);
        defaultshader->set();
    }

    void icon_(Texture *t, bool overlaid, int x, int y, int size, bool hit)
    {
        float xs = 0, ys = 0;
        if(t)
        {
            float scale = float(size)/max(t->xs, t->ys); //scale and preserve aspect ratio
            xs = t->xs*scale;
            ys = t->ys*scale;
            x += int((size-xs)/2);
            y += int((size-ys)/2);
            glBindTexture(GL_TEXTURE_2D, t->id);
        }
        else
        {
            extern GLuint lmprogtex;
            if(lmprogtex)
            {
                float scale = float(size)/256; //scale and preserve aspect ratio
                xs = 256*scale; ys = 256*scale;
                x += int((size-xs)/2);
                y += int((size-ys)/2);
                glBindTexture(GL_TEXTURE_2D, lmprogtex);
            }
            else
            {
                if((t = textureload(mapname, 3, true, false)) == notexture) t = textureload("textures/emblem", 3, true, false);
                float scale = float(size)/max(t->xs, t->ys); //scale and preserve aspect ratio
                xs = t->xs*scale; ys = t->ys*scale;
                x += int((size-xs)/2);
                y += int((size-ys)/2);
                glBindTexture(GL_TEXTURE_2D, t->id);
            }
        }
        float xi = x, yi = y, xpad = 0, ypad = 0;
        if(overlaid)
        {
            xpad = xs/32;
            ypad = ys/32;
            xi += xpad;
            yi += ypad;
            xs -= 2*xpad;
            ys -= 2*ypad;
        }
        static const float tc[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
        const vec &color = hit && !overlaid ? vec(0.5f, 0.5f, 0.5f) : vec(1, 1, 1);
        glColor3fv(color.v);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2fv(tc[0]); glVertex2f(xi,    yi);
        glTexCoord2fv(tc[1]); glVertex2f(xi+xs, yi);
        glTexCoord2fv(tc[3]); glVertex2f(xi,    yi+ys);
        glTexCoord2fv(tc[2]); glVertex2f(xi+xs, yi+ys);
        glEnd();
        if(overlaid)
        {
            if(!overlaytex) overlaytex = textureload(guioverlaytex, 3, true, false);
            const vec &ocolor = hit ? vec(1, 1, 1) : vec(0.5f, 0.5f, 0.5f);
            glColor3fv(ocolor.v);
            glBindTexture(GL_TEXTURE_2D, overlaytex->id);
            rect_(xi - xpad, yi - ypad, xs + 2*xpad, ys + 2*ypad, 0);
        }
    }

    void previewslot(VSlot &vslot, bool overlaid, int x, int y, int size, bool hit)
    {
        Slot &slot = *vslot.slot;
        if(slot.sts.empty()) return;
        VSlot *layer = NULL;
        Texture *t = NULL, *glowtex = NULL, *layertex = NULL;
        if(slot.loaded)
        {
            t = slot.sts[0].t;
            if(t == notexture) return;
            Slot &slot = *vslot.slot;
            if(slot.texmask&(1<<TEX_GLOW)) { loopvj(slot.sts) if(slot.sts[j].type==TEX_GLOW) { glowtex = slot.sts[j].t; break; } }
            if(vslot.layer)
            {
                layer = &lookupvslot(vslot.layer);
                if(!layer->slot->sts.empty()) layertex = layer->slot->sts[0].t;
            }
        }
        else if(slot.thumbnail && slot.thumbnail != notexture) t = slot.thumbnail;
        else return;
        float xt = min(1.0f, t->xs/(float)t->ys), yt = min(1.0f, t->ys/(float)t->xs), xs = size, ys = size;
        float xi = x, yi = y, xpad = 0, ypad = 0;
        if(overlaid)
        {
            xpad = xs/32;
            ypad = ys/32;
            xi += xpad;
            yi += ypad;
            xs -= 2*xpad;
            ys -= 2*ypad;
        }
        static Shader *rgbonlyshader = NULL;
        if(!rgbonlyshader) rgbonlyshader = lookupshaderbyname("rgbonly");
        rgbonlyshader->set();
        const vec &color = hit && !overlaid ? vec(0.5f, 0.5f, 0.5f) : vec(1, 1, 1);
        float tc[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
        int xoff = vslot.xoffset, yoff = vslot.yoffset;
        if(vslot.rotation)
        {
            if((vslot.rotation&5) == 1) { swap(xoff, yoff); loopk(4) swap(tc[k][0], tc[k][1]); }
            if(vslot.rotation >= 2 && vslot.rotation <= 4) { xoff *= -1; loopk(4) tc[k][0] *= -1; }
            if(vslot.rotation <= 2 || vslot.rotation == 5) { yoff *= -1; loopk(4) tc[k][1] *= -1; }
        }
        loopk(4) { tc[k][0] = tc[k][0]/xt - float(xoff)/t->xs; tc[k][1] = tc[k][1]/yt - float(yoff)/t->ys; }
        if(slot.loaded) glColor3f(color.x*vslot.colorscale.x, color.y*vslot.colorscale.y, color.z*vslot.colorscale.z);
        else glColor3fv(color.v);
        glBindTexture(GL_TEXTURE_2D, t->id);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2fv(tc[0]); glVertex2f(xi,    yi);
        glTexCoord2fv(tc[1]); glVertex2f(xi+xs, yi);
        glTexCoord2fv(tc[3]); glVertex2f(xi,    yi+ys);
        glTexCoord2fv(tc[2]); glVertex2f(xi+xs, yi+ys);
        glEnd();
        if(glowtex)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBindTexture(GL_TEXTURE_2D, glowtex->id);
            if(hit || overlaid) glColor3f(color.x*vslot.glowcolor.x, color.y*vslot.glowcolor.y, color.z*vslot.glowcolor.z);
            else glColor3fv(vslot.glowcolor.v);
            glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2fv(tc[0]); glVertex2f(x,    y);
            glTexCoord2fv(tc[1]); glVertex2f(x+xs, y);
            glTexCoord2fv(tc[3]); glVertex2f(x,    y+ys);
            glTexCoord2fv(tc[2]); glVertex2f(x+xs, y+ys);
            glEnd();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        if(layertex)
        {
            glBindTexture(GL_TEXTURE_2D, layertex->id);
            glColor3f(color.x*layer->colorscale.x, color.y*layer->colorscale.y, color.z*layer->colorscale.z);
            glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2fv(tc[0]); glVertex2f(x+xs/2, y+ys/2);
            glTexCoord2fv(tc[1]); glVertex2f(x+xs,   y+ys/2);
            glTexCoord2fv(tc[3]); glVertex2f(x+xs/2, y+ys);
            glTexCoord2fv(tc[2]); glVertex2f(x+xs,   y+ys);
            glEnd();
        }

        defaultshader->set();
        if(overlaid)
        {
            if(!overlaytex) overlaytex = textureload(guioverlaytex, 3, true, false);
            const vec &ocolor = hit ? vec(1, 1, 1) : vec(0.5f, 0.5f, 0.5f);
            glColor3fv(ocolor.v);
            glBindTexture(GL_TEXTURE_2D, overlaytex->id);
            rect_(xi - xpad, yi - ypad, xs + 2*xpad, ys + 2*ypad, 0);
        }
    }

    void slice_(Texture *t, int x, int y, int size, float start = 0, float end = 1, const char *text = NULL)
    {
        float scale = float(size)/max(t->xs, t->ys), xs = t->xs*scale, ys = t->ys*scale, fade = 1;
        if(start == end) { end = 1; fade = 0.5f; }
        glBindTexture(GL_TEXTURE_2D, t->id);
        glColor4f(1, 1, 1, fade);
        int s = max(xs,ys)/2;
        drawslice(start, end, x+s/2, y+s/2, s);
        if(text && *text)
        {
            int w = text_width(text);
            text_(text, x+s/2-w/2, y+s/2-FONTH/2, 0xFFFFFF, guiblend, false);
        }
    }

    void line_(int size, float percent = 1.0f)
    {
        if(visible())
        {
            if(!slidertex) slidertex = textureload(guislidertex, 3, true, false);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, slidertex->id);
            if(percent < 0.99f)
            {
                glColor4f(0.5f, 0.5f, 0.5f, 0.375f);
                if(ishorizontal())
                    rect_(curx + guibound[0]/2 - size, cury, size*2, ysize, 0);
                else
                    rect_(curx, cury + guibound[0]/2 - size, xsize, size*2, 1);
            }
            glColor3f(0.5f, 0.5f, 0.5f);
            if(ishorizontal())
                rect_(curx + guibound[0]/2 - size, cury + ysize*(1-percent), size*2, ysize*percent, 0);
            else
                rect_(curx, cury + guibound[0]/2 - size, xsize*percent, size*2, 1);
        }
        layout(ishorizontal() ? guibound[0] : 0, ishorizontal() ? 0 : guibound[1]);
    }

    int button_(const char *text, int color, const char *icon, bool clickable, bool faded, const char *font = "")
    {
        if(font && *font) gui::pushfont(font);
        int w = 0, h = max((int)FONTH, guibound[1]);
        if(icon) w += guibound[1];
        if(icon && text) w += 8;
        if(text) w += text_width(text);

        if(visible())
        {
            bool hit = ishit(w, FONTH);
            if(hit && clickable) color = 0xFF2222;
            int x = curx;
            if(icon)
            {
                defformatstring(tname)("%s%s", strncmp("textures/", icon, 9) ? "textures/" : "", icon);
                icon_(textureload(tname, 3, true, false), false, x, cury, guibound[1], faded && clickable && !hit);
                x += guibound[1];
            }
            if(icon && text) x += 8;
            if(text) text_(text, x, cury, color, hit || !faded || !clickable ? 255 : guiblend, hit && clickable);
        }
        if(font && *font) gui::popfont();
        return layout(w, h);
    }

    static Texture *overlaytex, *slidertex;

    vec origin, scale;
    guicb *cb;

    static float basescale, maxscale;
    static bool passthrough;

    void adjustscale()
    {
        int w = xsize + guibound[0]*2, h = ysize + guibound[1]*2;
        float aspect = float(screen->h)/float(screen->w), fit = 1.0f;
        if(w*aspect*basescale>1.0f) fit = 1.0f/(w*aspect*basescale);
        if(h*basescale*fit>maxscale) fit *= maxscale/(h*basescale*fit);
        origin = vec(0.5f-((w-xsize)/2 - guibound[0])*aspect*scale.x*fit, 0.5f + (0.5f*h-guibound[1])*scale.y*fit, 0);
        scale = vec(aspect*scale.x*fit, scale.y*fit, 1);
    }

    void start(int starttime, float initscale, int *tab, bool allowinput, bool wantstitle)
    {
        initscale *= 0.025f;
        basescale = initscale;
        if(layoutpass) scale.x = scale.y = scale.z = basescale; //min(basescale*(totalmillis-starttime)/300.0f, basescale);
        needsinput = allowinput;
        hastitle = wantstitle;
        passthrough = !allowinput;
        fontdepth = 0;
        gui::pushfont("sub");
        curdepth = curlist = mergedepth = mergelist = -1;
        tpos = tx = ty = 0;
        tcurrent = tab;
        tcolor = 0xFFFFFF;
        pushlist(false);
        if(layoutpass) nextlist = curlist;
        else
        {
            if(tcurrent && !*tcurrent) tcurrent = NULL;
            cury = -ysize;
            curx = -xsize/2;

            glPushMatrix();
            glTranslatef(origin.x, origin.y, origin.z);
            glScalef(scale.x, scale.y, scale.z);
        }
    }

    void end()
    {
        if(layoutpass)
        {
            xsize = max(tx, xsize);
            ysize = max(ty, ysize);
            ysize = max(ysize, guibound[1]);

            if(tcurrent) *tcurrent = max(1, min(*tcurrent, tpos));
            adjustscale();

            if(!passthrough)
            {
                hitx = (cursorx - origin.x)/scale.x;
                hity = (cursory - origin.y)/scale.y;
                if((mouseaction[0]&GUI_PRESSED) && (fabs(hitx-firstx) > 2 || fabs(hity - firsty) > 2)) mouseaction[0] |= GUI_DRAGGED;
            }
        }
        else
        {
            //if(needsinput && hastitle) uibuttons();
            glPopMatrix();
        }
        poplist();
        while(fontdepth) gui::popfont();
    }
};

Texture *gui::overlaytex = NULL, *gui::slidertex = NULL;
TVARN(IDF_PERSIST, guioverlaytex, "textures/guioverlay", gui::overlaytex, 0);
TVARN(IDF_PERSIST, guislidertex, "textures/guislider", gui::slidertex, 0);

vector<gui::list> gui::lists;
float gui::basescale, gui::maxscale = 1, gui::hitx, gui::hity;
bool gui::passthrough, gui::shouldautotab = true;
int gui::curdepth, gui::fontdepth, gui::curlist, gui::xsize, gui::ysize, gui::curx, gui::cury, gui::mergelist, gui::mergedepth;
int gui::ty, gui::tx, gui::tpos, *gui::tcurrent, gui::tcolor;
static vector<gui> guis;

namespace UI
{
    bool isopen = false, ready = false;

    void setup()
    {
        const char *fonts[2] = { "sub", "console" };
        loopi(2)
        {
            pushfont(fonts[i]);
            loopk(2) if((k ? FONTH : FONTW) > guibound[k]) guibound[k] = (k ? FONTH : FONTW);
            popfont();
        }
        ready = true;
    }

    bool keypress(int code, bool isdown, int cooked)
    {
        editor *e = currentfocus();
        if(fieldmode == FIELDKEY)
        {
            switch(code)
            {
                case SDLK_ESCAPE:
                    if(isdown) fieldmode = FIELDCOMMIT;
                    return true;
            }
            const char *keyname = getkeyname(code);
            if(keyname && isdown)
            {
                if(e->lines.length()!=1 || !e->lines[0].empty()) e->insert(" ");
                e->insert(keyname);
            }
            return true;
        }

        if(code<0) switch(code)
        { // fall-through-o-rama
            case -5: mouseaction[1] |= GUI_ALT;
            case -4: mouseaction[1] |= isdown ? GUI_DOWN : GUI_UP;
                if(active()) return true;
                break;
            case -3: mouseaction[0] |= GUI_ALT;
            case -1: mouseaction[0] |= (actionon=isdown) ? GUI_DOWN : GUI_UP;
                if(isdown) { firstx = gui::hitx; firsty = gui::hity; }
                if(active()) return true;
                break;
            case -2:
                cleargui(1);
                if(active()) return true;
            default: break;
        }

        if(fieldmode == FIELDSHOW || !e) return false;
        switch(code)
        {
            case SDLK_ESCAPE: //cancel editing without commit
                if(isdown) fieldmode = FIELDABORT;
                return true;
            case SDLK_RETURN:
            case SDLK_TAB:
                if(cooked && (e->maxy != 1)) break;
            case SDLK_KP_ENTER:
                if(isdown) fieldmode = FIELDCOMMIT; //signal field commit (handled when drawing field)
                return true;
            case SDLK_F4:
                if(SDL_GetModState()&MOD_KEYS)
                {
                    quit();
                    break;
                }
                // fall through
            case SDLK_HOME:
            case SDLK_END:
            case SDLK_PAGEUP:
            case SDLK_PAGEDOWN:
            case SDLK_DELETE:
            case SDLK_BACKSPACE:
            case SDLK_UP:
            case SDLK_DOWN:
            case SDLK_LEFT:
            case SDLK_RIGHT:
            case SDLK_LSHIFT:
            case SDLK_RSHIFT: break;
            default:
                if(!cooked || (code<32)) return false;
        }
        if(!isdown) return true;
        e->key(code, cooked);
        return true;
    }

    bool active(bool pass) { return guis.length() && (!pass || needsinput); }
    void limitscale(float scale) {  gui::maxscale = scale; }

    void addcb(guicb *cb)
    {
        gui &g = guis.add();
        g.cb = cb;
        g.adjustscale();
    }

    void update()
    {
        bool p = active(false);
        if(isopen != p) uimillis = (isopen = p) ? totalmillis : -totalmillis;
        setsvar("guirollovername", "", true);
        setsvar("guirolloveraction", "", true);
        setsvar("guirolloverimgpath", "", true);
        setsvar("guirolloverimgaction", "", true);
    }

    void render()
    {
        if(actionon) mouseaction[0] |= GUI_PRESSED;

        gui::reset(); guis.shrink(0);

        // call all places in the engine that may want to render a gui from here, they call addcb()
        if(progressing) progressmenu();
        else
        {
            texturemenu();
            hud::gamemenus();
            mainmenu();
        }

        readyeditors();
        bool wasfocused = (fieldmode!=FIELDSHOW);
        fieldsactive = false;

        needsinput = false;
        hastitle = true;

        if(!guis.empty())
        {
            layoutpass = true;
            //loopv(guis) guis[i].cb->gui(guis[i], true);
            guis.last().cb->gui(guis.last(), true);
            layoutpass = false;

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            glOrtho(0, 1, 1, 0, -1, 1);

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();

            //loopvrev(guis) guis[i].cb->gui(guis[i], false);
            guis.last().cb->gui(guis.last(), false);

            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();

            glDisable(GL_BLEND);
        }

        flusheditors();
        if(!fieldsactive) fieldmode = FIELDSHOW; //didn't draw any fields, so loose focus - mainly for menu closed
        if((fieldmode!=FIELDSHOW) != wasfocused)
        {
            SDL_EnableUNICODE(fieldmode!=FIELDSHOW);
            keyrepeat(fieldmode!=FIELDSHOW);
        }
        loopi(2) mouseaction[i] = 0;
    }
};
