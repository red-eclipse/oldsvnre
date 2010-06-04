// worldio.cpp: loading & saving of maps and savegames

#include "engine.h"

generic mapexts[] = {
    { ".mpz", MAP_MAPZ },
    { ".ogz", MAP_OCTA },
};

generic mapdirs[] = {
    { "maps", MAP_MAPZ },
    { "base", MAP_OCTA },
};

VAR(0, maptype, 1, -1, -1);
SVAR(0, mapfile, "");
SVAR(0, mapname, "");

VAR(IDF_PERSIST, autosavebackups, 0, 2, 4); // make backups; 0 = off, 1 = single backup, 2 = named backup, 3/4 = same as 1/2 with move to "backups/"
VAR(IDF_PERSIST, autosaveconfigs, 0, 1, 1);
VAR(IDF_PERSIST, autosavemapshot, 0, 1, 1);

void fixmaptitle()
{
    const char *title = maptitle, *author = strstr(title, " by ");
    if(author && *author)
    {
        char *t = newstring(title, author-title);
        if(t)
        {
            if(*t)
            {
                loopi(4) if(*author) author += 1;
                if(*author) setsvar("mapauthor", author, true);
                setsvar("maptitle", t, false);
            }
            delete[] t;
        }
    }
}

SVARF(IDF_WORLD, maptitle, "", fixmaptitle());
SVAR(IDF_WORLD, mapauthor, "");

void setnames(const char *fname, int type)
{
    maptype = type >= 0 || type <= MAP_MAX-1 ? type : MAP_MAPZ;

    string fn, mn, mf;
    if(fname != NULL) formatstring(fn)("%s", fname);
    else formatstring(fn)("%s/untitled", mapdirs[maptype].name);

    if(strpbrk(fn, "/\\")) copystring(mn, fn);
    else formatstring(mn)("%s/%s", mapdirs[maptype].name, fn);
    setsvar("mapname", mn);

    formatstring(mf)("%s%s", mapname, mapexts[maptype].name);
    setsvar("mapfile", mf);
}

enum { OCTSAV_CHILDREN = 0, OCTSAV_EMPTY, OCTSAV_SOLID, OCTSAV_NORMAL, OCTSAV_LODCUBE };

void savec(cube *c, stream *f, bool nolms)
{
    loopi(8)
    {
        if(c[i].children && (!c[i].ext || !c[i].ext->surfaces))
        {
            f->putchar(OCTSAV_CHILDREN);
            savec(c[i].children, f, nolms);
        }
        else
        {
            int oflags = 0;
            if(c[i].ext && c[i].ext->merged) oflags |= 0x80;
            if(c[i].children) f->putchar(oflags | OCTSAV_LODCUBE);
            else if(isempty(c[i])) f->putchar(oflags | OCTSAV_EMPTY);
            else if(isentirelysolid(c[i])) f->putchar(oflags | OCTSAV_SOLID);
            else
            {
                f->putchar(oflags | OCTSAV_NORMAL);
                f->write(c[i].edges, 12);
            }
            loopj(6) f->putlil<ushort>(c[i].texture[j]);
            uchar mask = 0;
            if(c[i].ext)
            {
                if(c[i].ext->material != MAT_AIR) mask |= 0x80;
                if(c[i].ext->normals && !nolms)
                {
                    mask |= 0x40;
                    loopj(6) if(c[i].ext->normals[j].normals[0] != bvec(128, 128, 128)) mask |= 1 << j;
                }
            }
            // save surface info for lighting
            if(!c[i].ext || !c[i].ext->surfaces || nolms)
            {
                f->putchar(mask);
                if(c[i].ext)
                {
                    if(c[i].ext->material != MAT_AIR) f->putchar(c[i].ext->material);
                    if(c[i].ext->normals && !nolms) loopj(6) if(mask & (1 << j))
                    {
                        loopk(sizeof(surfaceinfo)) f->putchar(0);
                        f->write(&c[i].ext->normals[j], sizeof(surfacenormals));
                    }
                }
            }
            else
            {
                int numsurfs = 6;
                loopj(6)
                {
                    surfaceinfo &surface = c[i].ext->surfaces[j];
                    if(surface.lmid >= LMID_RESERVED || surface.layer!=LAYER_TOP)
                    {
                        mask |= 1 << j;
                        if(surface.layer&LAYER_BLEND) numsurfs++;
                    }
                }
                f->putchar(mask);
                if(c[i].ext->material != MAT_AIR) f->putchar(c[i].ext->material);
                loopj(numsurfs) if(j >= 6 || mask & (1 << j))
                {
                    surfaceinfo tmp = c[i].ext->surfaces[j];
                    lilswap(&tmp.x, 2);
                    f->write(&tmp, sizeof(surfaceinfo));
                    if(j < 6 && c[i].ext->normals) f->write(&c[i].ext->normals[j], sizeof(surfacenormals));
                }
            }
            if(c[i].ext && c[i].ext->merged)
            {
                f->putchar(c[i].ext->merged | (c[i].ext->mergeorigin ? 0x80 : 0));
                if(c[i].ext->mergeorigin)
                {
                    f->putchar(c[i].ext->mergeorigin);
                    int index = 0;
                    loopj(6) if(c[i].ext->mergeorigin&(1<<j))
                    {
                        mergeinfo tmp = c[i].ext->merges[index++];
                        lilswap(&tmp.u1, 4);
                        f->write(&tmp, sizeof(mergeinfo));
                    }
                }
            }
            if(c[i].children) savec(c[i].children, f, nolms);
        }
    }
}

cube *loadchildren(stream *f);

void loadc(stream *f, cube &c)
{
    bool haschildren = false;
    int octsav = f->getchar();
    switch(octsav&0x7)
    {
        case OCTSAV_CHILDREN:
            c.children = loadchildren(f);
            return;

        case OCTSAV_LODCUBE: haschildren = true;    break;
        case OCTSAV_EMPTY:  emptyfaces(c);        break;
        case OCTSAV_SOLID:  solidfaces(c);        break;
        case OCTSAV_NORMAL: f->read(c.edges, 12); break;

        default:
            fatal("garbage in map");
    }
    loopi(6) c.texture[i] = hdr.version<14 ? f->getchar() : f->getlil<ushort>();
    if(hdr.version < 7) loopi(3) f->getchar(); //f->read(c.colour, 3);
    else
    {
        uchar mask = f->getchar();
        if(mask & 0x80)
        {
            int mat = f->getchar();
            if((maptype == MAP_OCTA && hdr.version <= 26) || (maptype == MAP_MAPZ && hdr.version <= 30))
            {
                static uchar matconv[] = { MAT_AIR, MAT_WATER, MAT_CLIP, MAT_GLASS|MAT_CLIP, MAT_NOCLIP, MAT_LAVA|MAT_DEATH, MAT_AICLIP, MAT_DEATH };
                mat = size_t(mat) < sizeof(matconv)/sizeof(matconv[0]) ? matconv[mat] : MAT_AIR;
            }
            ext(c).material = mat;
        }
        if(mask & 0x3F)
        {
            uchar lit = 0, bright = 0;
            static surfaceinfo surfaces[12];
            memset(surfaces, 0, 6*sizeof(surfaceinfo));
            if(mask & 0x40) newnormals(c);
            int numsurfs = 6;
            loopi(numsurfs)
            {
                if(i >= 6 || mask & (1 << i))
                {
                    f->read(&surfaces[i], sizeof(surfaceinfo));
                    lilswap(&surfaces[i].x, 2);
                    if(hdr.version < 10) ++surfaces[i].lmid;
                    if(hdr.version < 18)
                    {
                        if(surfaces[i].lmid >= LMID_AMBIENT1) ++surfaces[i].lmid;
                        if(surfaces[i].lmid >= LMID_BRIGHT1) ++surfaces[i].lmid;
                    }
                    if(hdr.version < 19)
                    {
                        if(surfaces[i].lmid >= LMID_DARK) surfaces[i].lmid += 2;
                    }
                    if(i < 6)
                    {
                        if(mask & 0x40) f->read(&c.ext->normals[i], sizeof(surfacenormals));
                        if(surfaces[i].layer != LAYER_TOP) lit |= 1 << i;
                        else if(surfaces[i].lmid == LMID_BRIGHT) bright |= 1 << i;
                        else if(surfaces[i].lmid != LMID_AMBIENT) lit |= 1 << i;
                        if(surfaces[i].layer&LAYER_BLEND) numsurfs++;
                    }
                }
                else surfaces[i].lmid = LMID_AMBIENT;
            }
            if(lit) newsurfaces(c, surfaces, numsurfs);
            else if(bright) brightencube(c);
        }
        if(hdr.version >= 20)
        {
            if(octsav&0x80)
            {
                int merged = f->getchar();
                ext(c).merged = merged&0x3F;
                if(merged&0x80)
                {
                    c.ext->mergeorigin = f->getchar();
                    int nummerges = 0;
                    loopi(6) if(c.ext->mergeorigin&(1<<i)) nummerges++;
                    if(nummerges)
                    {
                        c.ext->merges = new mergeinfo[nummerges];
                        loopi(nummerges)
                        {
                            mergeinfo *m = &c.ext->merges[i];
                            f->read(m, sizeof(mergeinfo));
                            lilswap(&m->u1, 4);
                            if(hdr.version <= 25)
                            {
                                int uorigin = m->u1 & 0xE000, vorigin = m->v1 & 0xE000;
                                m->u1 = (m->u1 - uorigin) << 2;
                                m->u2 = (m->u2 - uorigin) << 2;
                                m->v1 = (m->v1 - vorigin) << 2;
                                m->v2 = (m->v2 - vorigin) << 2;
                            }
                        }
                    }
                }
            }
        }
    }
    c.children = (haschildren ? loadchildren(f) : NULL);
}

cube *loadchildren(stream *f)
{
    cube *c = newcubes();
    loopi(8) loadc(f, c[i]);
    // TODO: remip c from children here
    return c;
}

void savevslot(stream *f, VSlot &vs, int prev)
{
    f->putlil<int>(vs.changed);
    f->putlil<int>(prev);
    if(vs.changed & (1<<VSLOT_SHPARAM))
    {
        f->putlil<ushort>(vs.params.length());
        loopv(vs.params)
        {
            ShaderParam &p = vs.params[i];
            f->putlil<ushort>(strlen(p.name));
            f->write(p.name, strlen(p.name));
            loopk(4) f->putlil<float>(p.val[k]);
        }
    }
    if(vs.changed & (1<<VSLOT_SCALE)) f->putlil<float>(vs.scale);
    if(vs.changed & (1<<VSLOT_ROTATION)) f->putlil<int>(vs.rotation);
    if(vs.changed & (1<<VSLOT_OFFSET))
    {
        f->putlil<int>(vs.xoffset);
        f->putlil<int>(vs.yoffset);
    }
    if(vs.changed & (1<<VSLOT_SCROLL))
    {
        f->putlil<float>(vs.scrollS);
        f->putlil<float>(vs.scrollT);
    }
    if(vs.changed & (1<<VSLOT_LAYER)) f->putlil<int>(vs.layer);
    if(vs.changed & (1<<VSLOT_ALPHA))
    {
        f->putlil<float>(vs.alphafront);
        f->putlil<float>(vs.alphaback);
    }
    if(vs.changed & (1<<VSLOT_COLOR))
    {
        loopk(3) f->putlil<float>(vs.colorscale[k]);
    }
}

void savevslots(stream *f, int numvslots)
{
    if(vslots.empty()) return;
    int *prev = new int[numvslots];
    memset(prev, -1, numvslots*sizeof(int));
    loopi(numvslots)
    {
        VSlot *vs = vslots[i];
        if(vs->changed) continue;
        for(;;)
        {
            VSlot *cur = vs;
            do vs = vs->next; while(vs && vs->index >= numvslots);
            if(!vs) break;
            prev[vs->index] = cur->index;
        }
    }
    int lastroot = 0;
    loopi(numvslots)
    {
        VSlot &vs = *vslots[i];
        if(!vs.changed) continue;
        if(lastroot < i) f->putlil<int>(-(i - lastroot));
        savevslot(f, vs, prev[i]);
        lastroot = i+1;
    }
    if(lastroot < numvslots) f->putlil<int>(-(numvslots - lastroot));
    delete[] prev;
}

void loadvslot(stream *f, VSlot &vs, int changed)
{
    vs.changed = changed;
    if(vs.changed & (1<<VSLOT_SHPARAM))
    {
        int numparams = f->getlil<ushort>();
        string name;
        loopi(numparams)
        {
            ShaderParam &p = vs.params.add();
            int nlen = f->getlil<ushort>();
            f->read(name, min(nlen, MAXSTRLEN-1));
            name[min(nlen, MAXSTRLEN-1)] = '\0';
            if(nlen >= MAXSTRLEN) f->seek(nlen - (MAXSTRLEN-1), SEEK_CUR);
            p.name = getshaderparamname(name);
            p.type = SHPARAM_LOOKUP;
            p.index = -1;
            p.loc = -1;
            loopk(4) p.val[k] = f->getlil<float>();
        }
    }
    if(vs.changed & (1<<VSLOT_SCALE)) vs.scale = f->getlil<float>();
    if(vs.changed & (1<<VSLOT_ROTATION)) vs.rotation = f->getlil<int>();
    if(vs.changed & (1<<VSLOT_OFFSET))
    {
        vs.xoffset = f->getlil<int>();
        vs.yoffset = f->getlil<int>();
    }
    if(vs.changed & (1<<VSLOT_SCROLL))
    {
        vs.scrollS = f->getlil<float>();
        vs.scrollT = f->getlil<float>();
    }
    if(vs.changed & (1<<VSLOT_LAYER)) vs.layer = f->getlil<int>();
    if(vs.changed & (1<<VSLOT_ALPHA))
    {
        vs.alphafront = f->getlil<float>();
        vs.alphaback = f->getlil<float>();
    }
    if(vs.changed & (1<<VSLOT_COLOR))
    {
        loopk(3) vs.colorscale[k] = f->getlil<float>();
    }
}

void loadvslots(stream *f, int numvslots)
{
    int *prev = new int[numvslots];
    memset(prev, -1, numvslots*sizeof(int));
    while(numvslots > 0)
    {
        int changed = f->getlil<int>();
        if(changed < 0)
        {
            loopi(-changed) vslots.add(new VSlot(NULL, vslots.length()));
            numvslots += changed;
        }
        else
        {
            prev[vslots.length()] = f->getlil<int>();
            loadvslot(f, *vslots.add(new VSlot(NULL, vslots.length())), changed);
            numvslots--;
        }
    }
    loopv(vslots) if(vslots.inrange(prev[i])) vslots[prev[i]]->next = vslots[i];
    delete[] prev;
}

void saveslotconfig(stream *h, Slot &s, int index)
{
    if(index >= 0)
    {
        if(s.shader)
        {
            h->printf("setshader %s\n", s.shader->name);
        }
        loopvj(s.params)
        {
            h->printf("set%sparam", s.params[j].type == SHPARAM_LOOKUP ? "shader" : (s.params[j].type == SHPARAM_UNIFORM ? "uniform" : (s.params[j].type == SHPARAM_PIXEL ? "pixel" : "vertex")));
            if(s.params[j].type == SHPARAM_LOOKUP || s.params[j].type == SHPARAM_UNIFORM) h->printf(" \"%s\"", s.params[j].name);
            else h->printf(" %d", s.params[j].index);
            loopk(4) h->printf(" %f", s.params[j].val[k]);
            h->printf("\n");
        }
    }
    loopvj(s.sts)
    {
        h->printf("texture");
        if(index >= 0) h->printf(" %s", findtexturename(s.sts[j].type));
        else if(!j) h->printf(" %s", findmaterialname(-index));
        else h->printf(" 1");
        h->printf(" \"%s\"", s.sts[j].lname);
        if(!j)
        {
            h->printf(" %d %d %d %f",
                s.variants->rotation, s.variants->xoffset, s.variants->yoffset, s.variants->scale);
            if(index >= 0) h->printf(" // %d", index);
        }
        h->printf("\n");
    }
    if(index >= 0)
    {
        if(s.variants->scrollS != 0.f || s.variants->scrollT != 0.f)
            h->printf("texscroll %f %f\n", s.variants->scrollS * 1000.0f, s.variants->scrollT * 1000.0f);
        if(s.variants->layer != 0)
        {
            if(s.layermaskname) h->printf("texlayer %d \"%s\" %d %f\n", s.variants->layer, s.layermaskname, s.layermaskmode, s.layermaskscale);
            else h->printf("texlayer %d\n", s.variants->layer);
        }
        if(s.variants->alphafront != DEFAULT_ALPHA_FRONT || s.variants->alphaback != DEFAULT_ALPHA_BACK)
            h->printf("texalpha %f %f\n", s.variants->alphafront, s.variants->alphaback);
        if(s.variants->colorscale != vec(1, 1, 1))
            h->printf("texcolor %f %f %f\n", s.variants->colorscale.x, s.variants->colorscale.y, s.variants->colorscale.z);
        if(s.autograss) h->printf("autograss \"%s\"\n", s.autograss);
        if(s.ffenv) h->printf("texffenv 1\n");
    }
    h->printf("\n");
}

void save_config(char *mname)
{
    if(autosavebackups) backup(mname, ".cfg", hdr.revision, autosavebackups > 2, !(autosavebackups%2));
    defformatstring(fname)("%s.cfg", mname);
    stream *h = openfile(fname, "w");
    if(!h) { conoutf("\frcould not write config to %s", fname); return; }

    // config
    h->printf("// %s by %s (%s)\n// Config generated by Red Eclipse\n\n", *maptitle ? maptitle : "Untitled", *mapauthor ? mapauthor : "Unknown", mapname);

    int vars = 0;
    h->printf("// Variables stored in map file, may be uncommented here, or changed from editmode.\n");
    vector<ident *> ids;
    enumerate(*idents, ident, id, ids.add(&id));
    ids.sort(sortidents);
    loopv(ids)
    {
        ident &id = *ids[i];
        if(id.flags&IDF_WORLD) switch(id.type)
        {
            case ID_VAR: h->printf((id.flags&IDF_HEX ? (id.maxval==0xFFFFFF ? "// %s 0x%.6X\n" : "// %s 0x%X\n") : "// %s %d\n"), id.name, *id.storage.i); vars++;break;
            case ID_FVAR: h->printf("// %s %s\n", id.name, floatstr(*id.storage.f)); vars++; break;
            case ID_SVAR: h->printf("// %s ", id.name); writeescapedstring(h, *id.storage.s); h->putchar('\n'); vars++; break;
            default: break;
        }
    }
    if(vars) h->printf("\n");
    if(verbose >= 2) conoutf("\fawrote %d variable values", vars);

    int aliases = 0;
    loopv(ids)
    {
        ident &id = *ids[i];
        if(id.type == ID_ALIAS && id.flags&IDF_WORLD && strlen(id.name) && strlen(id.action))
        {
            aliases++;
            h->printf("\"%s\" = [%s]\n", id.name, id.action);
        }
    }
    if(aliases) h->printf("\n");
    if(verbose >= 2) conoutf("\fasaved %d aliases", aliases);

    // texture slots
    int nummats = sizeof(materialslots)/sizeof(materialslots[0]);
    loopi(nummats)
    {
        if(verbose) progress(float(i)/float(nummats), "saving material slots...");

        if(i == MAT_WATER || i == MAT_LAVA)
        {
            saveslotconfig(h, materialslots[i], -i);
        }
    }
    if(verbose) conoutf("\fasaved %d material slots", nummats);

    loopv(slots)
    {
        if(verbose) progress(float(i)/float(slots.length()), "saving texture slots...");
        saveslotconfig(h, *slots[i], i);
    }
    if(verbose) conoutf("\fasaved %d texture slots", slots.length());

    loopv(mapmodels)
    {
        if(verbose) progress(float(i)/float(mapmodels.length()), "saving mapmodel slots...");
        h->printf("mmodel \"%s\"\n", mapmodels[i].name);
    }
    if(mapmodels.length()) h->printf("\n");
    if(verbose) conoutf("\fasaved %d mapmodel slots", mapmodels.length());

    loopv(mapsounds)
    {
        if(verbose) progress(float(i)/float(mapsounds.length()), "saving mapsound slots...");
        h->printf("mapsound \"%s\" %d \"%s\" %d %d\n", mapsounds[i].sample->name, mapsounds[i].vol, findmaterialname(mapsounds[i].material), mapsounds[i].maxrad, mapsounds[i].minrad);
    }
    if(mapsounds.length()) h->printf("\n");
    if(verbose) conoutf("\fasaved %d mapsound slots", mapsounds.length());

    delete h;
    if(verbose) conoutf("\fasaved config %s", fname);
}
ICOMMAND(0, savemapconfig, "s", (char *mname), save_config(*mname ? mname : mapname));

VARF(IDF_PERSIST, mapshotsize, 0, 256, INT_MAX-1, mapshotsize -= mapshotsize%2);

void save_mapshot(char *mname)
{
    if(autosavebackups) backup(mname, ifmtexts[imageformat], hdr.revision, autosavebackups > 2, !(autosavebackups%2));

    GLuint tex;
    glGenTextures(1, &tex);
    glViewport(0, 0, mapshotsize, mapshotsize);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    ImageData image(mapshotsize, mapshotsize, 3);
    memset(image.data, 0, 3*mapshotsize*mapshotsize);
    drawcubemap(mapshotsize, 1, camera1->o, camera1->yaw, camera1->pitch, false, false, false);
    glReadPixels(0, 0, mapshotsize, mapshotsize, GL_RGB, GL_UNSIGNED_BYTE, image.data);

    saveimage(mname, image, imageformat, compresslevel, true);

    glDeleteTextures(1, &tex);
    glViewport(0, 0, screen->w, screen->h);

    reloadtexture(mname);
}
ICOMMAND(0, savemapshot, "s", (char *mname), save_mapshot(*mname ? mname : mapname));

#define istempname(n) (!strncmp(n, "temp/", 5) || !strncmp(n, "temp\\", 5))

void save_world(const char *mname, bool nodata, bool forcesave)
{
    int savingstart = SDL_GetTicks();

    setnames(mname, MAP_MAPZ);

    if(autosavebackups) backup(mapname, mapexts[MAP_MAPZ].name, hdr.revision, autosavebackups > 2, !(autosavebackups%2));
    stream *f = opengzfile(mapfile, "wb");
    if(!f) { conoutf("\frerror saving %s to %s: file error", mapname, mapfile); return; }

    if(autosavemapshot || forcesave) save_mapshot(mapname);
    if(autosaveconfigs || forcesave) save_config(mapname);

    int numvslots = vslots.length();
    if(!nodata && !multiplayer(false))
    {
        numvslots = compactvslots();
        allchanged();
    }

    progress(0, "saving map..");
    strncpy(hdr.head, "MAPZ", 4);
    hdr.version = MAPVERSION;
    hdr.headersize = sizeof(mapz);
    hdr.gamever = server::getver(1);
    hdr.numents = 0;
    hdr.numvslots = numvslots;
    hdr.revision++;
    strncpy(hdr.gameid, server::gameid(), 4);

    const vector<extentity *> &ents = entities::getents();
    loopv(ents)
    {
        if(ents[i]->type!=ET_EMPTY || forcesave)
        {
            hdr.numents++;
        }
    }

    hdr.numpvs = nodata ? 0 : getnumviewcells();
    hdr.blendmap = nodata ? 0 : shouldsaveblendmap();
    hdr.lightmaps = nodata ? 0 : lightmaps.length();

    mapz tmp = hdr;
    lilswap(&tmp.version, 10);
    f->write(&tmp, sizeof(mapz));

    // world variables
    int numvars = 0, vars = 0;
    enumerate(*idents, ident, id, {
        if((id.type == ID_VAR || id.type == ID_FVAR || id.type == ID_SVAR) && id.flags&IDF_WORLD && strlen(id.name)) numvars++;
    });
    f->putlil<int>(numvars);
    enumerate(*idents, ident, id, {
        if((id.type == ID_VAR || id.type == ID_FVAR || id.type == ID_SVAR) && id.flags&IDF_WORLD && strlen(id.name))
        {
            vars++;
            if(verbose) progress(float(vars)/float(numvars), "saving world variables...");
            f->putlil<int>((int)strlen(id.name));
            f->write(id.name, (int)strlen(id.name)+1);
            f->putlil<int>(id.type);
            switch(id.type)
            {
                case ID_VAR:
                    f->putlil<int>(*id.storage.i);
                    break;
                case ID_FVAR:
                    f->putlil<float>(*id.storage.f);
                    break;
                case ID_SVAR:
                    f->putlil<int>((int)strlen(*id.storage.s));
                    f->write(*id.storage.s, (int)strlen(*id.storage.s)+1);
                    break;
                default: break;
            }
        }
    });

    if(verbose) conoutf("\fasaved %d variables", vars);

    // texture slots
    f->putlil<ushort>(texmru.length());
    loopv(texmru) f->putlil<ushort>(texmru[i]);

    // entities
    int count = 0;
    vector<int> remapents;
    if(!forcesave) entities::remapents(remapents);
    loopv(ents) // extended
    {
        if(verbose) progress(float(i)/float(ents.length()), "saving entities...");
        int idx = remapents.inrange(i) ? remapents[i] : i;
        extentity &e = *(extentity *)ents[idx];
        if(e.type!=ET_EMPTY || forcesave)
        {
            entbase tmp = e;
            lilswap(&tmp.o.x, 3);
            f->write(&tmp, sizeof(entbase));
            f->putlil<int>(e.attrs.length());
            loopvk(e.attrs) f->putlil<int>(e.attrs[k]);
            entities::writeent(f, idx);
            if(entities::maylink(e.type))
            {
                vector<int> links;
                int n = 0;
                loopvk(ents)
                {
                    int kidx = remapents.inrange(k) ? remapents[k] : k;
                    extentity &f = (extentity &)*ents[kidx];
                    if(f.type != ET_EMPTY || forcesave)
                    {
                        if(entities::maylink(f.type) && e.links.find(kidx) >= 0)
                            links.add(n); // align to indices
                        n++;
                    }
                }

                f->putlil<int>(links.length());
                loopvj(links) f->putlil<int>(links[j]); // aligned index
                if(verbose >= 2) conoutf("\faentity %s (%d) saved %d links", entities::findname(e.type), i, links.length());
            }
            count++;
        }
    }
    if(verbose) conoutf("\fasaved %d entities", count);

    savevslots(f, numvslots);
    if(verbose) conoutf("\fasaved %d vslots", numvslots);

    savec(worldroot, f, nodata);
    if(!nodata)
    {
        loopv(lightmaps)
        {
            if(verbose) progress(float(i)/float(lightmaps.length()), "saving lightmaps...");
            LightMap &lm = lightmaps[i];
            f->putchar(lm.type | (lm.unlitx>=0 ? 0x80 : 0));
            if(lm.unlitx>=0)
            {
                f->putlil<ushort>(ushort(lm.unlitx));
                f->putlil<ushort>(ushort(lm.unlity));
            }
            f->write(lm.data, lm.bpp*LM_PACKW*LM_PACKH);
        }
        if(verbose) conoutf("\fasaved %d lightmaps", lightmaps.length());
        if(getnumviewcells()>0)
        {
            if(verbose) progress(0, "saving PVS...");
            savepvs(f);
            if(verbose) conoutf("\fasaved %d PVS view cells", getnumviewcells());
        }
        if(shouldsaveblendmap())
        {
            if(verbose) progress(0, "saving blendmap...");
            saveblendmap(f);
            if(verbose) conoutf("\fasaved blendmap");
        }
    }

    progress(0, "saving world...");
    game::saveworld(f);
    delete f;

    conoutf("\fasaved map %s v.%d:%d (r%d) in %.1f secs", mapname, hdr.version, hdr.gamever, hdr.revision, (SDL_GetTicks()-savingstart)/1000.0f);

    if(istempname(mapname)) setnames(&mapname[5], MAP_MAPZ);
}

ICOMMAND(0, savemap, "s", (char *mname), save_world(*mname ? (istempname(mname) ? &mname[5] : mname) : (istempname(mapname) ? &mapname[5] : mapname)));

static uint mapcrc = 0;

uint getmapcrc() { return mapcrc; }

void swapXZ(cube *c)
{
    loopi(8)
    {
        swap(c[i].faces[0],   c[i].faces[2]);
        swap(c[i].texture[0], c[i].texture[4]);
        swap(c[i].texture[1], c[i].texture[5]);
        if(c[i].ext && c[i].ext->surfaces)
        {
            swap(c[i].ext->surfaces[0], c[i].ext->surfaces[4]);
            swap(c[i].ext->surfaces[1], c[i].ext->surfaces[5]);
        }
        if(c[i].children) swapXZ(c[i].children);
    }
}

static void fixoversizedcubes(cube *c, int size)
{
    if(size <= 0x1000) return;
    loopi(8)
    {
        if(!c[i].children) subdividecube(c[i], true, false);
        fixoversizedcubes(c[i].children, size>>1);
    }
}

static void sanevars()
{
    setvar("fullbright", 0, false);
    setvar("blankgeom", 0, false);
}

bool load_world(const char *mname, bool temp)       // still supports all map formats that have existed since the earliest cube betas!
{
    int loadingstart = SDL_GetTicks();
    loop(tempfile, 2) if(tempfile || temp)
    {
        loop(format, MAP_MAX)
        {
            setnames(mname, format);
            if(!tempfile) loopk(2)
            {
                defformatstring(s)("temp/%s", k ? mapfile : mapname);
                setsvar(k ? "mapfile" : "mapname", s);
            }

            stream *f = opengzfile(mapfile, "rb");
            if(!f) continue;

            bool samegame = true;
            int eif = 0;
            mapz newhdr;
            if(f->read(&newhdr, sizeof(binary))!=(int)sizeof(binary))
            {
                conoutf("\frerror loading %s: malformatted universal header", mapname);
                delete f;
                return false;
            }
            lilswap(&newhdr.version, 2);

            clearworldvars();
            if(strncmp(newhdr.head, "MAPZ", 4) == 0 || strncmp(newhdr.head, "BFGZ", 4) == 0)
            {
                // this check removed below due to breakage: (size_t)newhdr.headersize > sizeof(chdr) || f->read(&chdr.worldsize, newhdr.headersize-sizeof(binary))!=newhdr.headersize-(int)sizeof(binary)
                #define MAPZCOMPAT(ver) \
                    mapzcompat##ver chdr; \
                    memcpy(&chdr, &newhdr, sizeof(binary)); \
                    if(f->read(&chdr.worldsize, sizeof(chdr)-sizeof(binary))!=int(sizeof(chdr)-sizeof(binary))) \
                    { \
                        conoutf("\frerror loading %s: malformatted mapz v%d[%d] header", mapname, newhdr.version, ver); \
                        delete f; \
                        return false; \
                    }
                if(newhdr.version <= 25)
                {
                    MAPZCOMPAT(25);
                    lilswap(&chdr.worldsize, 5);
                    memcpy(&newhdr.worldsize, &chdr.worldsize, sizeof(int)*2);
                    newhdr.numpvs = 0;
                    newhdr.lightmaps = chdr.lightmaps;
                    newhdr.blendmap = 0;
                    newhdr.numvslots = 0;
                    memcpy(&newhdr.gamever, &chdr.gamever, sizeof(int)*2);
                    memcpy(&newhdr.gameid, &chdr.gameid, 4);
                    setsvar("maptitle", chdr.maptitle, true);
                }
                else if(newhdr.version <= 32)
                {
                    MAPZCOMPAT(32);
                    lilswap(&chdr.worldsize, 6);
                    memcpy(&newhdr.worldsize, &chdr.worldsize, sizeof(int)*4);
                    newhdr.blendmap = 0;
                    newhdr.numvslots = 0;
                    memcpy(&newhdr.gamever, &chdr.gamever, sizeof(int)*2);
                    memcpy(&newhdr.gameid, &chdr.gameid, 4);
                    setsvar("maptitle", chdr.maptitle, true);
                }
                else if(newhdr.version <= 33)
                {
                    MAPZCOMPAT(33);
                    lilswap(&chdr.worldsize, 7);
                    memcpy(&newhdr.worldsize, &chdr.worldsize, sizeof(int)*5);
                    newhdr.numvslots = 0;
                    memcpy(&newhdr.gamever, &chdr.gamever, sizeof(int)*2);
                    memcpy(&newhdr.gameid, &chdr.gameid, 4);
                    setsvar("maptitle", chdr.maptitle, true);
                }
                else if(newhdr.version <= 38)
                {
                    MAPZCOMPAT(38);
                    lilswap(&chdr.worldsize, 7);
                    memcpy(&newhdr.worldsize, &chdr.worldsize, sizeof(int)*5);
                    newhdr.numvslots = 0;
                    memcpy(&newhdr.gamever, &chdr.gamever, sizeof(int)*2);
                }
                else
                {
                    if((size_t)newhdr.headersize > sizeof(newhdr) || f->read(&newhdr.worldsize, newhdr.headersize-sizeof(binary))!=newhdr.headersize-(int)sizeof(binary))
                    {
                        conoutf("\frerror loading %s: malformatted mapz v%d header", mapname, newhdr.version);
                        delete f;
                        return false;
                    }
                    lilswap(&newhdr.worldsize, 8);
                }

                if(newhdr.version > MAPVERSION)
                {
                    conoutf("\frerror loading %s: requires a newer version of Red Eclipse", mapname);
                    delete f;
                    return false;
                }

                resetmap(false);
                hdr = newhdr;
                progress(0, "please wait..");
                maptype = MAP_MAPZ;

                if(hdr.version <= 24) copystring(hdr.gameid, "bfa", 4); // all previous maps were bfa-fps
                if(verbose) conoutf("\faloading v%d map from %s game v%d", hdr.version, hdr.gameid, hdr.gamever);

                if(hdr.version >= 25 || (hdr.version == 24 && hdr.gamever >= 44))
                {
                    int numvars = hdr.version >= 25 ? f->getlil<int>() : f->getchar(), vars = 0;
                    overrideidents = worldidents = true;
                    progress(0, "loading variables...");
                    loopi(numvars)
                    {
                        if(verbose) progress(float(i)/float(numvars), "loading variables...");
                        int len = hdr.version >= 25 ? f->getlil<int>() : f->getchar();
                        if(len)
                        {
                            string name;
                            f->read(name, len+1);
                            if(hdr.version <= 34 && !strcmp(name, "cloudcolour")) copystring(name, "cloudlayercolour");
                            ident *id = idents->access(name);
                            bool proceed = true;
                            int type = hdr.version >= 28 ? f->getlil<int>()+(hdr.version >= 29 ? 0 : 1) : (id ? id->type : ID_VAR);
                            if(!id || type != id->type)
                            {
                                if(id && hdr.version <= 28 && id->type == ID_FVAR && type == ID_VAR)
                                    type = ID_FVAR;
                                else proceed = false;
                            }
                            if(!id || !(id->flags&IDF_WORLD)) proceed = false;

                            switch(type)
                            {
                                case ID_VAR:
                                {
                                    int val = hdr.version >= 25 ? f->getlil<int>() : f->getchar();
                                    if(proceed)
                                    {
                                        if(val > id->maxval) val = id->maxval;
                                        else if(val < id->minval) val = id->minval;
                                        setvar(name, val, true);
                                    }
                                    break;
                                }
                                case ID_FVAR:
                                {
                                    float val = hdr.version >= 29 ? f->getlil<float>() : float(f->getlil<int>())/100.f;
                                    if(proceed)
                                    {
                                        if(val > id->maxvalf) val = id->maxvalf;
                                        else if(val < id->minvalf) val = id->minvalf;
                                        setfvar(name, val, true);
                                    }
                                    break;
                                }
                                case ID_SVAR:
                                {
                                    int slen = f->getlil<int>();
                                    string val;
                                    f->read(val, slen+1);
                                    if(proceed && slen) setsvar(name, val, true);
                                    break;
                                }
                                default:
                                {
                                    if(hdr.version <= 27)
                                    {
                                        if(hdr.version >= 25) f->getlil<int>();
                                        else f->getchar();
                                    }
                                    proceed = false;
                                    break;
                                }
                            }
                            if(!proceed)
                            {
                                if(verbose) conoutf("\frWARNING: ignoring variable %s stored in map", name);
                            }
                            else vars++;
                        }
                    }
                    overrideidents = worldidents = false;
                    if(verbose) conoutf("\faloaded %d variables", vars);
                }
                sanevars();

                if(!server::canload(hdr.gameid))
                {
                    if(verbose) conoutf("\frWARNING: loading map from %s game type in %s, ignoring game specific data", hdr.gameid, server::gameid());
                    samegame = false;
                }
            }
            else if(strncmp(newhdr.head, "OCTA", 4) == 0)
            {
                octa ohdr;
                memcpy(&ohdr, &newhdr, sizeof(binary));

                // this check removed due to breakage: (size_t)ohdr.headersize > sizeof(chdr) || f->read(&chdr.worldsize, ohdr.headersize-sizeof(binary))!=ohdr.headersize-(int)sizeof(binary)
                #define OCTACOMPAT(ver) \
                    octacompat##ver chdr; \
                    memcpy(&chdr, &ohdr, sizeof(binary)); \
                    if(f->read(&chdr.worldsize, sizeof(chdr)-sizeof(binary))!=int(sizeof(chdr)-sizeof(binary))) \
                    { \
                        conoutf("\frerror loading %s: malformatted octa v%d[%d] header", mapname, ver, ohdr.version); \
                        delete f; \
                        return false; \
                    }

                #define OCTAVARS \
                    if(chdr.lightprecision) setvar("lightprecision", chdr.lightprecision); \
                    if(chdr.lighterror) setvar("lighterror", chdr.lighterror); \
                    if(chdr.bumperror) setvar("bumperror", chdr.bumperror); \
                    setvar("lightlod", chdr.lightlod); \
                    if(chdr.ambient) setvar("ambient", chdr.ambient); \
                    setvar("skylight", (int(chdr.skylight[0])<<16) | (int(chdr.skylight[1])<<8) | int(chdr.skylight[2])); \
                    setvar("watercolour", (int(chdr.watercolour[0])<<16) | (int(chdr.watercolour[1])<<8) | int(chdr.watercolour[2]), true); \
                    setvar("waterfallcolour", (int(chdr.waterfallcolour[0])<<16) | (int(chdr.waterfallcolour[1])<<8) | int(chdr.waterfallcolour[2])); \
                    setvar("lavacolour", (int(chdr.lavacolour[0])<<16) | (int(chdr.lavacolour[1])<<8) | int(chdr.lavacolour[2])); \
                    setvar("fullbright", 0, true); \
                    if(chdr.lerpsubdivsize || chdr.lerpangle) setvar("lerpangle", chdr.lerpangle); \
                    if(chdr.lerpsubdivsize) \
                    { \
                        setvar("lerpsubdiv", chdr.lerpsubdiv); \
                        setvar("lerpsubdivsize", chdr.lerpsubdivsize); \
                    } \
                    setsvar("maptitle", chdr.maptitle, true); \
                    ohdr.numvars = 0;

                if(ohdr.version <= 25)
                {
                    OCTACOMPAT(25);
                    lilswap(&chdr.worldsize, 7);
                    memcpy(&ohdr.worldsize, &chdr.worldsize, sizeof(int)*2);
                    ohdr.numpvs = 0;
                    memcpy(&ohdr.lightmaps, &chdr.lightmaps, sizeof(int)*3);
                    ohdr.numvslots = 0;
                    OCTAVARS;
                }
                else if(ohdr.version <= 28)
                {
                    OCTACOMPAT(28);
                    lilswap(&chdr.worldsize, 7);
                    memcpy(&ohdr.worldsize, &chdr.worldsize, sizeof(int)*6);
                    OCTAVARS;
                    ohdr.blendmap = chdr.blendmap;
                    ohdr.numvslots = 0;
                }
                else if(ohdr.version <= 29)
                {
                    OCTACOMPAT(29);
                    lilswap(&chdr.worldsize, 6);
                    memcpy(&ohdr.worldsize, &chdr.worldsize, sizeof(int)*6);
                    ohdr.numvslots = 0;
                }
                else
                {
                    if(f->read(&ohdr.worldsize, sizeof(octa)-sizeof(binary))!=sizeof(octa)-(int)sizeof(binary))
                    {
                        conoutf("\frerror loading %s: malformatted octa v%d header", mapname, ohdr.version);
                        delete f;
                        return false;
                    }
                    lilswap(&ohdr.worldsize, 7);
                }

                if(ohdr.version > OCTAVERSION)
                {
                    conoutf("\frerror loading %s: requires a newer version of Cube 2 support", mapname);
                    delete f;
                    return false;
                }

                resetmap(false);
                hdr = newhdr;
                progress(0, "please wait..");
                maptype = MAP_OCTA;

                strncpy(hdr.head, ohdr.head, 4);
                hdr.gamever = 0; // sauer has no gamever
                hdr.worldsize = ohdr.worldsize;
                if(hdr.worldsize > 1<<18) hdr.worldsize = 1<<18;
                hdr.numents = ohdr.numents;
                hdr.numpvs = ohdr.numpvs;
                hdr.lightmaps = ohdr.lightmaps;
                hdr.blendmap = ohdr.blendmap;
                hdr.numvslots = ohdr.numvslots;
                hdr.revision = 1;

                if(ohdr.version >= 29) loopi(ohdr.numvars)
                {
                    int type = f->getchar(), ilen = f->getlil<ushort>();
                    string name;
                    f->read(name, min(ilen, OCTASTRLEN-1));
                    name[min(ilen, OCTASTRLEN-1)] = '\0';
                    if(ilen >= OCTASTRLEN) f->seek(ilen - (OCTASTRLEN-1), SEEK_CUR);
                    if(!strcmp(name, "cloudalpha")) copystring(name, "cloudblend");
                    if(!strcmp(name, "cloudcolour")) copystring(name, "cloudlayercolour");
                    if(!strcmp(name, "grassalpha")) copystring(name, "grassblend");
                    ident *id = getident(name);
                    bool exists = id && id->type == type;
                    switch(type)
                    {
                        case ID_VAR:
                        {
                            int val = f->getlil<int>();
                            if(exists && id->minval <= id->maxval) setvar(name, val, true);
                            break;
                        }

                        case ID_FVAR:
                        {
                            float val = f->getlil<float>();
                            if(exists && id->minvalf <= id->maxvalf) setfvar(name, val, true);
                            break;
                        }

                        case ID_SVAR:
                        {
                            int slen = f->getlil<ushort>();
                            string val;
                            f->read(val, min(slen, OCTASTRLEN-1));
                            val[min(slen, OCTASTRLEN-1)] = '\0';
                            if(slen >= OCTASTRLEN) f->seek(slen - (OCTASTRLEN-1), SEEK_CUR);
                            if(exists) setsvar(name, val, true);
                            break;
                        }
                    }
                }
                sanevars();

                string gameid;
                if(hdr.version >= 16)
                {
                    int len = f->getchar();
                    f->read(gameid, len+1);
                }
                else copystring(gameid, "fps");
                strncpy(hdr.gameid, gameid, 4);

                if(!server::canload(hdr.gameid))
                {
                    if(verbose) conoutf("\frWARNING: loading OCTA v%d map from %s game, ignoring game specific data", hdr.version, hdr.gameid);
                    samegame = false;
                }
                else if(verbose) conoutf("\faloading OCTA v%d map from %s game", hdr.version, hdr.gameid);

                if(hdr.version>=16)
                {
                    eif = f->getlil<ushort>();
                    int extrasize = f->getlil<ushort>();
                    loopj(extrasize) f->getchar();
                }

                if(hdr.version<25) hdr.numpvs = 0;
                if(hdr.version<28) hdr.blendmap = 0;
            }
            else
            {
                delete f;
                continue;
            }

            progress(0, "clearing world...");

            texmru.shrink(0);
            if(hdr.version<14)
            {
                uchar oldtl[256];
                f->read(oldtl, sizeof(oldtl));
                loopi(256) texmru.add(oldtl[i]);
            }
            else
            {
                ushort nummru = f->getlil<ushort>();
                loopi(nummru) texmru.add(f->getlil<ushort>());
            }

            freeocta(worldroot);
            worldroot = NULL;

            progress(0, "loading entities...");
            vector<extentity *> &ents = entities::getents();
            loopi(hdr.numents)
            {
                if(verbose) progress(float(i)/float(hdr.numents), "loading entities...");
                extentity &e = *entities::newent(); ents.add(&e);
                if(maptype == MAP_OCTA || (maptype == MAP_MAPZ && hdr.version <= 36))
                {
                    entcompat ec;
                    f->read(&ec, sizeof(entcompat));
                    lilswap(&ec.o.x, 3);
                    lilswap(ec.attr, 5);
                    e.o = ec.o;
                    e.type = ec.type;
                    loopk(5) e.attrs.add(ec.attr[k]);
                }
                else
                {
                    f->read(&e, sizeof(entbase));
                    lilswap(&e.o.x, 3);
                    int numattr = f->getlil<int>();
                    loopk(numattr)
                    {
                        int an = f->getlil<int>();
                        e.attrs.add(an);
                    }
                    if(numattr < 5) loopk(5-numattr) e.attrs.add(0);
                }
                if((maptype == MAP_OCTA && hdr.version <= 27) || (maptype == MAP_MAPZ && hdr.version <= 31)) e.attrs[4] = 0; // init ever-present attr5
                if(maptype == MAP_OCTA) loopj(eif) f->getchar();

                // sauerbraten version increments
                if(hdr.version <= 10 && e.type >= 7) e.type++;
                if(hdr.version <= 12 && e.type >= 8) e.type++;
                if(hdr.version <= 14 && e.type >= ET_MAPMODEL && e.type <= 16)
                {
                    if(e.type == 16) e.type = ET_MAPMODEL;
                    else e.type++;
                }
                if(hdr.version <= 20 && e.type >= ET_ENVMAP) e.type++;
                if(hdr.version <= 21 && e.type >= ET_PARTICLES) e.type++;
                if(hdr.version <= 22 && e.type >= ET_SOUND) e.type++;
                if(hdr.version <= 23 && e.type >= ET_LIGHTFX) e.type++;

                // redeclipse version increments
                if((maptype == MAP_OCTA || (maptype == MAP_MAPZ && hdr.version <= 35)) && e.type >= ET_SUNLIGHT) e.type++;
                entities::readent(f, maptype, hdr.version, hdr.gameid, hdr.gamever, i);
                if(maptype == MAP_MAPZ && entities::maylink(hdr.gamever <= 49 && e.type >= 10 ? e.type-1 : e.type, hdr.gamever))
                {
                    int links = f->getlil<int>();
                    loopk(links)
                    {
                        int ln = f->getlil<int>();
                        e.links.add(ln);
                    }
                    if(verbose >= 2) conoutf("\faentity %s (%d) loaded %d link(s)", entities::findname(e.type), i, links);
                }

                if(maptype == MAP_OCTA && e.type == ET_PARTICLES && e.attrs[0] >= 11)
                {
                    if(e.attrs[0] <= 12) e.attrs[0] += 3;
                    else e.attrs[0] = 0; // bork it up
                }
                if(hdr.version <= 14 && e.type == ET_MAPMODEL)
                {
                    e.o.z += e.attrs[2];
                    if(e.attrs[3] && verbose) conoutf("\frWARNING: mapmodel ent (index %d) uses texture slot %d", i, e.attrs[3]);
                    e.attrs[2] = e.attrs[3] = 0;
                }
                if(e.type == ET_MAPMODEL)
                {
                    if(maptype == MAP_OCTA || hdr.version <= 31)
                    {
                        int angle = e.attrs[0];
                        e.attrs[0] = e.attrs[1];
                        e.attrs[1] = angle;
                        e.attrs[2] = e.attrs[3] = e.attrs[4] = 0;
                    }
                    if(maptype == MAP_OCTA || hdr.version <= 37)
                    {
                        e.attrs[2] = e.attrs[3];
                        e.attrs[5] = e.attrs[4];
                        e.attrs[3] = e.attrs[4] = 0;
                    }
                }
                if(e.type == ET_SUNLIGHT && hdr.version <= 38) e.attrs[1] -= 90; // reorient pitch axis
                if((maptype == MAP_OCTA && hdr.version <= 30) || (maptype == MAP_MAPZ && hdr.version <= 39)) switch(e.type)
                {
                    case ET_MAPMODEL: case ET_PLAYERSTART: e.attrs[1] = (e.attrs[1] + 180)%360; break;
                    case ET_SUNLIGHT: e.attrs[0] = (e.attrs[0] + 180)%360; break;
                }
                if(verbose && !insideworld(e.o) && e.type != ET_LIGHT && e.type != ET_LIGHTFX && e.type != ET_SUNLIGHT)
                    conoutf("\frWARNING: ent outside of world: enttype[%s] index %d (%f, %f, %f)", entities::findname(e.type), i, e.o.x, e.o.y, e.o.z);
            }
            if(verbose) conoutf("\faloaded %d entities", hdr.numents);

            progress(0, "loading slots...");
            loadvslots(f, hdr.numvslots);

            progress(0, "loading octree...");
            worldroot = loadchildren(f);

            if(hdr.version <= 11)
                swapXZ(worldroot);

            if(hdr.version <= 8)
                converttovectorworld();

            if(hdr.worldsize > 0x1000 && hdr.version <= 25)
                fixoversizedcubes(worldroot, hdr.worldsize>>1);

            progress(0, "validating...");
            validatec(worldroot, hdr.worldsize>>1);

            worldscale = 0;
            while(1<<worldscale < hdr.worldsize) worldscale++;

            if(hdr.version >= 7) loopi(hdr.lightmaps)
            {
                if(verbose) progress(i/(float)hdr.lightmaps, "loading lightmaps...");
                LightMap &lm = lightmaps.add();
                if(hdr.version >= 17)
                {
                    int type = f->getchar();
                    lm.type = type&0x7F;
                    if(hdr.version >= 20 && type&0x80)
                    {
                        lm.unlitx = f->getlil<ushort>();
                        lm.unlity = f->getlil<ushort>();
                    }
                }
                if(lm.type&LM_ALPHA && (lm.type&LM_TYPE)!=LM_BUMPMAP1) lm.bpp = 4;
                lm.data = new uchar[lm.bpp*LM_PACKW*LM_PACKH];
                f->read(lm.data, lm.bpp * LM_PACKW * LM_PACKH);
                lm.finalize();
            }

            if(hdr.numpvs > 0) loadpvs(f);
            if(hdr.blendmap) loadblendmap(f);

            if(verbose) conoutf("\faloaded %d lightmaps", hdr.lightmaps);

            mapcrc = f->getcrc();
            progress(0, "loading world...");
            game::loadworld(f, maptype);
            entities::initents(f, maptype, hdr.version, hdr.gameid, hdr.gamever);

            overrideidents = worldidents = true;
            defformatstring(cfgname)("%s.cfg", mapname);
            if(maptype == MAP_OCTA)
            {
                execfile("octa.cfg"); // for use with -pSAUER_DIR
                execfile(cfgname);
            }
            else if(!execfile(cfgname, false)) execfile("map.cfg");
            if(maptype == MAP_OCTA || (maptype == MAP_MAPZ && hdr.version <= 34))
            {
                extern float cloudblend;
                setfvar("cloudlayerblend", cloudblend, true);
            }
            overrideidents = worldidents = false;

            loopv(ents)
            {
                extentity &e = *ents[i];

                if((maptype == MAP_OCTA || (maptype == MAP_MAPZ && hdr.version <= 29)) && ents[i]->type == ET_LIGHTFX && ents[i]->attrs[0] == LFX_SPOTLIGHT)
                {
                    int closest = -1;
                    float closedist = 1e10f;
                    loopvk(ents) if(ents[k]->type == ET_LIGHT)
                    {
                        extentity &a = *ents[k];
                        float dist = e.o.dist(a.o);
                        if(dist < closedist)
                        {
                            closest = k;
                            closedist = dist;
                        }
                    }
                    if(ents.inrange(closest) && closedist <= 100)
                    {
                        extentity &a = *ents[closest];
                        if(e.links.find(closest) < 0) e.links.add(closest);
                        if(a.links.find(i) < 0) a.links.add(i);
                        if(verbose) conoutf("\frWARNING: auto linked spotlight %d to light %d", i, closest);
                    }
                }
            }

            preloadusedmapmodels(true);

            delete f;
            conoutf("\faloaded map %s v.%d:%d (r%d) in %.1f secs", mapname, hdr.version, hdr.gamever, hdr.revision, (SDL_GetTicks()-loadingstart)/1000.0f);

            if((maptype == MAP_OCTA && hdr.version <= 25) || (maptype == MAP_MAPZ && hdr.version <= 26))
                fixlightmapnormals();

            entitiesinoctanodes();
            initlights();
            allchanged(true);

            progress(0, "starting world...");
            game::startmap(mapname, mname);
            return true;
        }
    }
    conoutf("\frunable to load %s", mname);
    return false;
}

static int mtlsort(const int *x, const int *y)
{
    if(*x < *y) return -1;
    if(*x > *y) return 1;
    return 0;
}

void writeobj(char *name)
{
    defformatstring(fname)("%s.obj", name);
    stream *f = openfile(path(fname), "w");
    if(!f) return;
    f->printf("# obj file of Cube 2 level\n\n");
    defformatstring(mtlname)("%s.mtl", name);
    path(mtlname);
    f->printf("mtllib %s\n\n", mtlname);
    extern vector<vtxarray *> valist;
    vector<vec> verts;
    vector<vec2> texcoords;
    hashtable<vec, int> shareverts(1<<16);
    hashtable<vec2, int> sharetc(1<<16);
    hashtable<int, vector<ivec> > mtls(1<<8);
    vector<int> usedmtl;
    vec bbmin(1e16f, 1e16f, 1e16f), bbmax(-1e16f, -1e16f, -1e16f);
    loopv(valist)
    {
        vtxarray &va = *valist[i];
        ushort *edata = NULL;
        uchar *vdata = NULL;
        if(!readva(&va, edata, vdata)) continue;
        int vtxsize = VTXSIZE;
        ushort *idx = edata;
        loopj(va.texs)
        {
            elementset &es = va.eslist[j];
            if(usedmtl.find(es.texture) < 0) usedmtl.add(es.texture);
            vector<ivec> &keys = mtls[es.texture];
            loopk(es.length[1])
            {
                int n = idx[k] - va.voffset;
                const vec &pos = renderpath==R_FIXEDFUNCTION ? ((const vertexff *)&vdata[n*vtxsize])->pos : ((const vertex *)&vdata[n*vtxsize])->pos;
                vec2 tc(renderpath==R_FIXEDFUNCTION ? ((const vertexff *)&vdata[n*vtxsize])->u : ((const vertex *)&vdata[n*vtxsize])->u,
                        renderpath==R_FIXEDFUNCTION ? ((const vertexff *)&vdata[n*vtxsize])->v : ((const vertex *)&vdata[n*vtxsize])->v);
                ivec &key = keys.add();
                key.x = shareverts.access(pos, verts.length());
                if(key.x == verts.length())
                {
                    verts.add(pos);
                    loopl(3)
                    {
                        bbmin[l] = min(bbmin[l], pos[l]);
                        bbmax[l] = max(bbmax[l], pos[l]);
                    }
                }
                key.y = sharetc.access(tc, texcoords.length());
                if(key.y == texcoords.length()) texcoords.add(tc);
            }
            idx += es.length[1];
        }
        delete[] edata;
        delete[] vdata;
    }

    vec center(-(bbmax.x + bbmin.x)/2, -(bbmax.y + bbmin.y)/2, -bbmin.z);
    loopv(verts)
    {
        vec v = verts[i];
        v.add(center);
        if(v.y != floor(v.y)) f->printf("v %.3f ", -v.y); else f->printf("v %d ", int(-v.y));
        if(v.z != floor(v.z)) f->printf("%.3f ", v.z); else f->printf("%d ", int(v.z));
        if(v.x != floor(v.x)) f->printf("%.3f\n", v.x); else f->printf("%d\n", int(v.x));
    }
    f->printf("\n");
    loopv(texcoords)
    {
        const vec2 &tc = texcoords[i];
        f->printf("vt %.6f %.6f\n", tc.x, 1-tc.y);
    }
    f->printf("\n");

    usedmtl.sort(mtlsort);
    loopv(usedmtl)
    {
        vector<ivec> &keys = mtls[usedmtl[i]];
        f->printf("g slot%d\n", usedmtl[i]);
        f->printf("usemtl slot%d\n\n", usedmtl[i]);
        for(int i = 0; i < keys.length(); i += 3)
        {
            f->printf("f");
            loopk(3) f->printf(" %d/%d", keys[i+2-k].x+1, keys[i+2-k].y+1);
            f->printf("\n");
        }
        f->printf("\n");
    }
    delete f;

    f = openfile(mtlname, "w");
    if(!f) return;
    f->printf("# mtl file of map\n\n");
    loopv(usedmtl)
    {
        VSlot &vslot = lookupvslot(usedmtl[i], false);
        f->printf("newmtl slot%d\n", usedmtl[i]);
        f->printf("map_Kd %s\n", findfile(vslot.slot->sts.empty() ? notexture->name : vslot.slot->sts[0].name, "r"));
        f->printf("\n");
    }
    delete f;
}

COMMAND(0, writeobj, "s");

int getworldsize() { return hdr.worldsize; }
int getmapversion() { return hdr.version; }
ICOMMAND(0, mapversion, "", (void), intret(getmapversion()));
int getmaprevision() { return hdr.revision; }
ICOMMAND(0, maprevision, "", (void), intret(getmaprevision()));

