struct md2;

md2 *loadingmd2 = 0;

float md2normaltable[256][3] =
{
    { -0.525731f,  0.000000f,  0.850651f },     { -0.442863f,  0.238856f,  0.864188f },     { -0.295242f,  0.000000f,  0.955423f },     { -0.309017f,  0.500000f,  0.809017f },
    { -0.162460f,  0.262866f,  0.951056f },     {  0.000000f,  0.000000f,  1.000000f },     {  0.000000f,  0.850651f,  0.525731f },     { -0.147621f,  0.716567f,  0.681718f },
    {  0.147621f,  0.716567f,  0.681718f },     {  0.000000f,  0.525731f,  0.850651f },     {  0.309017f,  0.500000f,  0.809017f },     {  0.525731f,  0.000000f,  0.850651f },
    {  0.295242f,  0.000000f,  0.955423f },     {  0.442863f,  0.238856f,  0.864188f },     {  0.162460f,  0.262866f,  0.951056f },     { -0.681718f,  0.147621f,  0.716567f },
    { -0.809017f,  0.309017f,  0.500000f },     { -0.587785f,  0.425325f,  0.688191f },     { -0.850651f,  0.525731f,  0.000000f },     { -0.864188f,  0.442863f,  0.238856f },
    { -0.716567f,  0.681718f,  0.147621f },     { -0.688191f,  0.587785f,  0.425325f },     { -0.500000f,  0.809017f,  0.309017f },     { -0.238856f,  0.864188f,  0.442863f },
    { -0.425325f,  0.688191f,  0.587785f },     { -0.716567f,  0.681718f, -0.147621f },     { -0.500000f,  0.809017f, -0.309017f },     { -0.525731f,  0.850651f,  0.000000f },
    {  0.000000f,  0.850651f, -0.525731f },     { -0.238856f,  0.864188f, -0.442863f },     {  0.000000f,  0.955423f, -0.295242f },     { -0.262866f,  0.951056f, -0.162460f },
    {  0.000000f,  1.000000f,  0.000000f },     {  0.000000f,  0.955423f,  0.295242f },     { -0.262866f,  0.951056f,  0.162460f },     {  0.238856f,  0.864188f,  0.442863f },
    {  0.262866f,  0.951056f,  0.162460f },     {  0.500000f,  0.809017f,  0.309017f },     {  0.238856f,  0.864188f, -0.442863f },     {  0.262866f,  0.951056f, -0.162460f },
    {  0.500000f,  0.809017f, -0.309017f },     {  0.850651f,  0.525731f,  0.000000f },     {  0.716567f,  0.681718f,  0.147621f },     {  0.716567f,  0.681718f, -0.147621f },
    {  0.525731f,  0.850651f,  0.000000f },     {  0.425325f,  0.688191f,  0.587785f },     {  0.864188f,  0.442863f,  0.238856f },     {  0.688191f,  0.587785f,  0.425325f },
    {  0.809017f,  0.309017f,  0.500000f },     {  0.681718f,  0.147621f,  0.716567f },     {  0.587785f,  0.425325f,  0.688191f },     {  0.955423f,  0.295242f,  0.000000f },
    {  1.000000f,  0.000000f,  0.000000f },     {  0.951056f,  0.162460f,  0.262866f },     {  0.850651f, -0.525731f,  0.000000f },     {  0.955423f, -0.295242f,  0.000000f },
    {  0.864188f, -0.442863f,  0.238856f },     {  0.951056f, -0.162460f,  0.262866f },     {  0.809017f, -0.309017f,  0.500000f },     {  0.681718f, -0.147621f,  0.716567f },
    {  0.850651f,  0.000000f,  0.525731f },     {  0.864188f,  0.442863f, -0.238856f },     {  0.809017f,  0.309017f, -0.500000f },     {  0.951056f,  0.162460f, -0.262866f },
    {  0.525731f,  0.000000f, -0.850651f },     {  0.681718f,  0.147621f, -0.716567f },     {  0.681718f, -0.147621f, -0.716567f },     {  0.850651f,  0.000000f, -0.525731f },
    {  0.809017f, -0.309017f, -0.500000f },     {  0.864188f, -0.442863f, -0.238856f },     {  0.951056f, -0.162460f, -0.262866f },     {  0.147621f,  0.716567f, -0.681718f },
    {  0.309017f,  0.500000f, -0.809017f },     {  0.425325f,  0.688191f, -0.587785f },     {  0.442863f,  0.238856f, -0.864188f },     {  0.587785f,  0.425325f, -0.688191f },
    {  0.688191f,  0.587785f, -0.425325f },     { -0.147621f,  0.716567f, -0.681718f },     { -0.309017f,  0.500000f, -0.809017f },     {  0.000000f,  0.525731f, -0.850651f },
    { -0.525731f,  0.000000f, -0.850651f },     { -0.442863f,  0.238856f, -0.864188f },     { -0.295242f,  0.000000f, -0.955423f },     { -0.162460f,  0.262866f, -0.951056f },
    {  0.000000f,  0.000000f, -1.000000f },     {  0.295242f,  0.000000f, -0.955423f },     {  0.162460f,  0.262866f, -0.951056f },     { -0.442863f, -0.238856f, -0.864188f },
    { -0.309017f, -0.500000f, -0.809017f },     { -0.162460f, -0.262866f, -0.951056f },     {  0.000000f, -0.850651f, -0.525731f },     { -0.147621f, -0.716567f, -0.681718f },
    {  0.147621f, -0.716567f, -0.681718f },     {  0.000000f, -0.525731f, -0.850651f },     {  0.309017f, -0.500000f, -0.809017f },     {  0.442863f, -0.238856f, -0.864188f },
    {  0.162460f, -0.262866f, -0.951056f },     {  0.238856f, -0.864188f, -0.442863f },     {  0.500000f, -0.809017f, -0.309017f },     {  0.425325f, -0.688191f, -0.587785f },
    {  0.716567f, -0.681718f, -0.147621f },     {  0.688191f, -0.587785f, -0.425325f },     {  0.587785f, -0.425325f, -0.688191f },     {  0.000000f, -0.955423f, -0.295242f },
    {  0.000000f, -1.000000f,  0.000000f },     {  0.262866f, -0.951056f, -0.162460f },     {  0.000000f, -0.850651f,  0.525731f },     {  0.000000f, -0.955423f,  0.295242f },
    {  0.238856f, -0.864188f,  0.442863f },     {  0.262866f, -0.951056f,  0.162460f },     {  0.500000f, -0.809017f,  0.309017f },     {  0.716567f, -0.681718f,  0.147621f },
    {  0.525731f, -0.850651f,  0.000000f },     { -0.238856f, -0.864188f, -0.442863f },     { -0.500000f, -0.809017f, -0.309017f },     { -0.262866f, -0.951056f, -0.162460f },
    { -0.850651f, -0.525731f,  0.000000f },     { -0.716567f, -0.681718f, -0.147621f },     { -0.716567f, -0.681718f,  0.147621f },     { -0.525731f, -0.850651f,  0.000000f },
    { -0.500000f, -0.809017f,  0.309017f },     { -0.238856f, -0.864188f,  0.442863f },     { -0.262866f, -0.951056f,  0.162460f },     { -0.864188f, -0.442863f,  0.238856f },
    { -0.809017f, -0.309017f,  0.500000f },     { -0.688191f, -0.587785f,  0.425325f },     { -0.681718f, -0.147621f,  0.716567f },     { -0.442863f, -0.238856f,  0.864188f },
    { -0.587785f, -0.425325f,  0.688191f },     { -0.309017f, -0.500000f,  0.809017f },     { -0.147621f, -0.716567f,  0.681718f },     { -0.425325f, -0.688191f,  0.587785f },
    { -0.162460f, -0.262866f,  0.951056f },     {  0.442863f, -0.238856f,  0.864188f },     {  0.162460f, -0.262866f,  0.951056f },     {  0.309017f, -0.500000f,  0.809017f },
    {  0.147621f, -0.716567f,  0.681718f },     {  0.000000f, -0.525731f,  0.850651f },     {  0.425325f, -0.688191f,  0.587785f },     {  0.587785f, -0.425325f,  0.688191f },
    {  0.688191f, -0.587785f,  0.425325f },     { -0.955423f,  0.295242f,  0.000000f },     { -0.951056f,  0.162460f,  0.262866f },     { -1.000000f,  0.000000f,  0.000000f },
    { -0.850651f,  0.000000f,  0.525731f },     { -0.955423f, -0.295242f,  0.000000f },     { -0.951056f, -0.162460f,  0.262866f },     { -0.864188f,  0.442863f, -0.238856f },
    { -0.951056f,  0.162460f, -0.262866f },     { -0.809017f,  0.309017f, -0.500000f },     { -0.864188f, -0.442863f, -0.238856f },     { -0.951056f, -0.162460f, -0.262866f },
    { -0.809017f, -0.309017f, -0.500000f },     { -0.681718f,  0.147621f, -0.716567f },     { -0.681718f, -0.147621f, -0.716567f },     { -0.850651f,  0.000000f, -0.525731f },
    { -0.688191f,  0.587785f, -0.425325f },     { -0.587785f,  0.425325f, -0.688191f },     { -0.425325f,  0.688191f, -0.587785f },     { -0.425325f, -0.688191f, -0.587785f },
    { -0.587785f, -0.425325f, -0.688191f },     { -0.688191f, -0.587785f, -0.425325f }
};

struct md2 : vertmodel
{
    struct md2_header
    {
        int magic;
        int version;
        int skinwidth, skinheight;
        int framesize;
        int numskins, numvertices, numtexcoords;
        int numtriangles, numglcommands, numframes;
        int offsetskins, offsettexcoords, offsettriangles;
        int offsetframes, offsetglcommands, offsetend;
    };

    struct md2_vertex
    {
        uchar vertex[3], normalindex;
    };

    struct md2_frame
    {
        float      scale[3];
        float      translate[3];
        char       name[16];
    };

    md2(const char *name) : vertmodel(name) {}

    int type() const { return MDL_MD2; }

    int linktype(animmodel *m) const { return LINK_COOP; }

    struct md2meshgroup : vertmeshgroup
    {
        void genverts(int *glcommands, vector<tcvert> &tcverts, vector<ushort> &vindexes, vector<tri> &tris)
        {
            hashtable<ivec, int> tchash;
            vector<ushort> idxs;
            for(int *command = glcommands; (*command)!=0;)
            {
                int numvertex = *command++;
                bool isfan = numvertex<0;
                if(isfan) numvertex = -numvertex;
                idxs.setsize(0);
                loopi(numvertex)
                {
                    union { int i; float f; } u, v;
                    u.i = *command++;
                    v.i = *command++;
                    int vindex = *command++;
                    ivec tckey(u.i, v.i, vindex);
                    int *idx = tchash.access(tckey);
                    if(!idx)
                    {
                        idx = &tchash[tckey];
                        *idx = tcverts.length();
                        tcvert &tc = tcverts.add();
                        tc.u = u.f;
                        tc.v = v.f;
                        vindexes.add((ushort)vindex);
                    }
                    idxs.add(*idx);
                }
                loopi(numvertex-2)
                {
                    tri &t = tris.add();
                    if(isfan)
                    {
                        t.vert[0] = idxs[0];
                        t.vert[1] = idxs[i+1];
                        t.vert[2] = idxs[i+2];
                    }
                    else loopk(3) t.vert[k] = idxs[i&1 && k ? i+(1-(k-1))+1 : i+k];
                }
            }
        }

        bool load(char *filename)
        {
            stream *file = openfile(filename, "rb");
            if(!file) return false;

            md2_header header;
            file->read(&header, sizeof(md2_header));
            lilswap(&header.magic, sizeof(md2_header)/sizeof(int));

            if(header.magic!=844121161 || header.version!=8)
            {
                delete file;
                return false;
            }

            name = newstring(filename);

            numframes = header.numframes;

            vertmesh &m = *new vertmesh;
            m.group = this;
            meshes.add(&m);

            int *glcommands = new int[header.numglcommands];
            file->seek(header.offsetglcommands, SEEK_SET);
            int numglcommands = file->read(glcommands, header.numglcommands*sizeof(int))/sizeof(int);
            lilswap(glcommands, numglcommands);
            if(numglcommands < header.numglcommands) memset(&glcommands[numglcommands], 0, (header.numglcommands-numglcommands)*sizeof(int));

            vector<tcvert> tcgen;
            vector<ushort> vgen;
            vector<tri> trigen;
            genverts(glcommands, tcgen, vgen,trigen);
            delete[] glcommands;

            m.numverts = tcgen.length();

            m.tcverts = new tcvert[m.numverts];
            memcpy(m.tcverts, tcgen.getbuf(), m.numverts*sizeof(tcvert));
            m.numtris = trigen.length();
            m.tris = new tri[m.numtris];
            memcpy(m.tris, trigen.getbuf(), m.numtris*sizeof(tri));

            m.verts = new vert[m.numverts*numframes];

            md2_vertex *tmpverts = new md2_vertex[header.numvertices];
            int frame_offset = header.offsetframes;
            vert *curvert = m.verts;
            loopi(header.numframes)
            {
                md2_frame frame;
                file->seek(frame_offset, SEEK_SET);
                file->read(&frame, sizeof(md2_frame));
                lilswap(frame.scale, 6);

                file->read(tmpverts, header.numvertices*sizeof(md2_vertex));
                loopj(m.numverts)
                {
                    const md2_vertex &v = tmpverts[vgen[j]];
                    curvert->pos = vec(v.vertex[0]*frame.scale[0]+frame.translate[0],
                                       -(v.vertex[1]*frame.scale[1]+frame.translate[1]),
                                       v.vertex[2]*frame.scale[2]+frame.translate[2]);
                    const float *norm = md2normaltable[v.normalindex];
                    curvert->norm = vec(norm[0], -norm[1], norm[2]);
                    curvert++;
                }
                frame_offset += header.framesize;
            }
            delete[] tmpverts;

            delete file;

            return true;
        }
    };

    meshgroup *loadmeshes(char *name, va_list args)
    {
        md2meshgroup *group = new md2meshgroup;
        if(!group->load(name)) { delete group; return NULL; }
        return group;
    }

    bool load()
    {
        if(loaded) return true;
        part &mdl = *new part;
        parts.add(&mdl);
        mdl.model = this;
        mdl.index = 0;
        const char *pname = parentdir(loadname);
        defformatstring(name1)("models/%s/tris.md2", loadname);
        mdl.meshes = sharemeshes(path(name1));
        if(!mdl.meshes)
        {
            defformatstring(name2)("models/%s/tris.md2", pname);    // try md2 in parent folder (vert sharing)
            mdl.meshes = sharemeshes(path(name2));
            if(!mdl.meshes) return false;
        }
        Texture *tex, *masks;
        loadskin(loadname, pname, tex, masks);
        mdl.initskins(tex, masks);
        if(tex==notexture) conoutf("\frcould not load model skin for %s", name1);
        loadingmd2 = this;
        defformatstring(name3)("models/%s/md2.cfg", loadname);
        if(!execfile(name3, false))
        {
            formatstring(name3)("models/%s/md2.cfg", pname);
            execfile(name3, false);
        }
        loadingmd2 = 0;
        scale /= 4;
        translate.y = -translate.y;
        parts[0]->translate = translate;
        loopv(parts) parts[i]->meshes->shared++;
        preloadshaders();
        return loaded = true;
    }
};

void md2pitch(float *pitchscale, float *pitchoffset, float *pitchmin, float *pitchmax)
{
    if(!loadingmd2 || loadingmd2->parts.empty()) { conoutf("\frnot loading an md2"); return; }
    md2::part &mdl = *loadingmd2->parts.last();

    mdl.pitchscale = *pitchscale;
    mdl.pitchoffset = *pitchoffset;
    if(*pitchmin || *pitchmax)
    {
        mdl.pitchmin = *pitchmin;
        mdl.pitchmax = *pitchmax;
    }
    else
    {
        mdl.pitchmin = -360*mdl.pitchscale;
        mdl.pitchmax = 360*mdl.pitchscale;
    }
}

void md2anim(char *anim, int *frame, int *range, float *speed, int *priority)
{
    if(!loadingmd2 || loadingmd2->parts.empty()) { conoutf("\frnot loading an md2"); return; }
    vector<int> anims;
    game::findanims(anim, anims);
    if(anims.empty()) conoutf("\frcould not find animation %s in %s", anim, loadingmd2->loadname);
    else loopv(anims)
    {
        loadingmd2->parts.last()->setanim(0, anims[i], *frame, *range, *speed, *priority);
    }
}

COMMAND(0, md2pitch, "ffff");
COMMAND(0, md2anim, "siifi");

