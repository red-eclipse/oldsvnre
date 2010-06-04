// texture.cpp: texture slot management

#include "engine.h"

#define FUNCNAME(name) name##1
#define DEFPIXEL uint OP(r, 0);
#define PIXELOP OP(r, 0);
#define BPP 1
#include "scale.h"

#define FUNCNAME(name) name##2
#define DEFPIXEL uint OP(r, 0), OP(g, 1);
#define PIXELOP OP(r, 0); OP(g, 1);
#define BPP 2
#include "scale.h"

#define FUNCNAME(name) name##3
#define DEFPIXEL uint OP(r, 0), OP(g, 1), OP(b, 2);
#define PIXELOP OP(r, 0); OP(g, 1); OP(b, 2);
#define BPP 3
#include "scale.h"

#define FUNCNAME(name) name##4
#define DEFPIXEL uint OP(r, 0), OP(g, 1), OP(b, 2), OP(a, 3);
#define PIXELOP OP(r, 0); OP(g, 1); OP(b, 2); OP(a, 3);
#define BPP 4
#include "scale.h"

static void scaletexture(uchar *src, uint sw, uint sh, uint bpp, uint pitch, uchar *dst, uint dw, uint dh)
{
    if(sw == dw*2 && sh == dh*2)
    {
        switch(bpp)
        {
            case 1: return halvetexture1(src, sw, sh, pitch, dst);
            case 2: return halvetexture2(src, sw, sh, pitch, dst);
            case 3: return halvetexture3(src, sw, sh, pitch, dst);
            case 4: return halvetexture4(src, sw, sh, pitch, dst);
        }
    }
    else if(sw < dw || sh < dh || sw&(sw-1) || sh&(sh-1))
    {
        switch(bpp)
        {
            case 1: return scaletexture1(src, sw, sh, pitch, dst, dw, dh);
            case 2: return scaletexture2(src, sw, sh, pitch, dst, dw, dh);
            case 3: return scaletexture3(src, sw, sh, pitch, dst, dw, dh);
            case 4: return scaletexture4(src, sw, sh, pitch, dst, dw, dh);
        }
    }
    else
    {
        switch(bpp)
        {
            case 1: return shifttexture1(src, sw, sh, pitch, dst, dw, dh);
            case 2: return shifttexture2(src, sw, sh, pitch, dst, dw, dh);
            case 3: return shifttexture3(src, sw, sh, pitch, dst, dw, dh);
            case 4: return shifttexture4(src, sw, sh, pitch, dst, dw, dh);
        }
    }
}

static inline void reorienttexture(uchar *src, int sw, int sh, int bpp, int stride, uchar *dst, bool flipx, bool flipy, bool swapxy, bool normals = false)
{
    int stridex = bpp, stridey = bpp;
    if(swapxy) stridex *= sh; else stridey *= sw;
    if(flipx) { dst += (sw-1)*stridex; stridex = -stridex; }
    if(flipy) { dst += (sh-1)*stridey; stridey = -stridey; }
    uchar *srcrow = src;
    loopi(sh)
    {
        for(uchar *curdst = dst, *src = srcrow, *end = &srcrow[sw*bpp]; src < end;)
        {
            loopk(bpp) curdst[k] = *src++;
            if(normals)
            {
                if(flipx) curdst[0] = 255-curdst[0];
                if(flipy) curdst[1] = 255-curdst[1];
                if(swapxy) swap(curdst[0], curdst[1]);
            }
            curdst += stridex;
        }
        srcrow += stride;
        dst += stridey;
    }
}

#define writetex(t, body) \
    { \
        uchar *dstrow = t.data; \
        loop(y, t.h) \
        { \
            for(uchar *dst = dstrow, *end = &dstrow[t.w*t.bpp]; dst < end; dst += t.bpp) \
            { \
                body; \
            } \
            dstrow += t.pitch; \
        } \
    }

#define readwritetex(t, s, body) \
    { \
        uchar *dstrow = t.data, *srcrow = s.data; \
        loop(y, t.h) \
        { \
            for(uchar *dst = dstrow, *src = srcrow, *end = &srcrow[s.w*s.bpp]; src < end; dst += t.bpp, src += s.bpp) \
            { \
                body; \
            } \
            dstrow += t.pitch; \
            srcrow += s.pitch; \
        } \
    }

#define read2writetex(t, s1, src1, s2, src2, body) \
    { \
        uchar *dstrow = t.data, *src1row = s1.data, *src2row = s2.data; \
        loop(y, t.h) \
        { \
            for(uchar *dst = dstrow, *end = &dstrow[t.w*t.bpp], *src1 = src1row, *src2 = src2row; dst < end; dst += t.bpp, src1 += s1.bpp, src2 += s2.bpp) \
            { \
                body; \
            } \
            dstrow += t.pitch; \
            src1row += s1.pitch; \
            src2row += s2.pitch; \
        } \
    }

void texreorient(ImageData &s, bool flipx, bool flipy, bool swapxy, int type = TEX_DIFFUSE)
{
    ImageData d(swapxy ? s.h : s.w, swapxy ? s.w : s.h, s.bpp);
    reorienttexture(s.data, s.w, s.h, s.bpp, s.pitch, d.data, flipx, flipy, swapxy, type==TEX_NORMAL);
    s.replace(d);
}

void texrotate(ImageData &s, int numrots, int type = TEX_DIFFUSE)
{
    // 1..3 rotate through 90..270 degrees, 4 flips X, 5 flips Y
    if(numrots>=1 && numrots<=5)
        texreorient(s,
            numrots>=2 && numrots<=4, // flip X on 180/270 degrees
            numrots<=2 || numrots==5, // flip Y on 90/180 degrees
            (numrots&5)==1,           // swap X/Y on 90/270 degrees
            type);
}

void texoffset(ImageData &s, int xoffset, int yoffset)
{
    xoffset = max(xoffset, 0);
    xoffset %= s.w;
    yoffset = max(yoffset, 0);
    yoffset %= s.h;
    if(!xoffset && !yoffset) return;
    ImageData d(s.w, s.h, s.bpp);
    uchar *src = s.data;
    loop(y, s.h)
    {
        uchar *dst = (uchar *)d.data+((y+yoffset)%d.h)*d.pitch;
        memcpy(dst+xoffset*s.bpp, src, (s.w-xoffset)*s.bpp);
        memcpy(dst, src+(s.w-xoffset)*s.bpp, xoffset*s.bpp);
        src += s.pitch;
    }
    s.replace(d);
}

void texcrop(ImageData &s, ImageData &d, int x, int y, int w, int h)
{
    d.setdata(NULL, w, h, s.bpp);
    uchar *dst = d.data, *src = &s.data[y*s.pitch + x*s.bpp];
    loopi(h)
    {
        memcpy(dst, src, w*s.bpp);
        src += s.pitch;
        dst += d.pitch;
    }
}

void texcrop(ImageData &s, int x, int y, int w, int h)
{
    ImageData d;
    texcrop(s, d, x, y, w, h);
    s.replace(d);
}

void texcopy(ImageData &s, ImageData &d)
{
    texcrop(s, d, 0, 0, s.w, s.h);
}

#define readwritergbtex(t, s, body) \
    { \
        if(t.bpp >= 3) { readwritetex(t, s, body); } \
        else \
        { \
            ImageData rgb(t.w, t.h, 3); \
            read2writetex(rgb, t, orig, s, src, \
            { \
                switch(t.bpp) \
                { \
                    case 1: dst[0] = orig[0]; dst[1] = orig[0]; dst[2] = orig[0]; break; \
                    case 2: dst[0] = orig[0]; dst[1] = orig[1]; dst[2] = orig[1]; break; \
                } \
                body; \
            }); \
            t.replace(rgb); \
        } \
    }

void forcergbimage(ImageData &s)
{
    if(s.bpp >= 3) return;
    ImageData d(s.w, s.h, 3);
    readwritetex(d, s,
        switch(s.bpp)
        {
            case 1: dst[0] = src[0]; dst[1] = src[0]; dst[2] = src[0]; break;
            case 2: dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[1]; break;
        }
    );
    s.replace(d);
}

#define readwritergbatex(t, s, body) \
    { \
        if(t.bpp >= 4) { readwritetex(t, s, body); } \
        else \
        { \
            ImageData rgba(t.w, t.h, 4); \
            read2writetex(rgba, t, orig, s, src, \
            { \
                switch(t.bpp) \
                { \
                    case 1: dst[0] = orig[0]; dst[1] = orig[0]; dst[2] = orig[0]; break; \
                    case 2: dst[0] = orig[0]; dst[1] = orig[1]; dst[2] = orig[1]; break; \
                    case 3: dst[0] = orig[0]; dst[1] = orig[1]; dst[2] = orig[2]; break; \
                } \
                body; \
            }); \
            t.replace(rgba); \
        } \
    }

void forcergbaimage(ImageData &s)
{
    if(s.bpp >= 4) return;
    ImageData d(s.w, s.h, 4);
    readwritetex(d, s,
        switch(s.bpp)
        {
            case 1: dst[0] = src[0]; dst[1] = src[0]; dst[2] = src[0]; break;
            case 2: dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[1]; break;
            case 3: dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; break;
        }
    );
    s.replace(d);
}

void texmad(ImageData &s, const vec &mul, const vec &add)
{
    if(s.bpp < 3 && (mul.x!=mul.y || mul.x!=mul.z || add.x!=add.y || add.x!=add.z))
        forcergbimage(s);

    int maxk = min(int(s.bpp), 3);
    writetex(s,
        loopk(maxk) dst[k] = uchar(clamp(dst[k]*mul[k] + 255*add[k], 0.0f, 255.0f));
    );
}

void texcolorify(ImageData &s, const vec &color, vec weights)
{
    if(s.bpp < 3) return;
    if(weights.iszero()) weights = vec(0.21f, 0.72f, 0.07f);
    writetex(s,
        float lum = dst[0]*weights.x + dst[1]*weights.y + dst[2]*weights.z;
        loopk(3) dst[k] = uchar(clamp(lum*color[k], 0.0f, 255.0f));
    );
}

void texffmask(ImageData &s, float glowscale, float envscale)
{
    if(renderpath!=R_FIXEDFUNCTION) return;
    if(nomasks || s.bpp<3) { s.cleanup(); return; }
    const int minval = 0x18;
    bool glow = false, envmap = true;
    writetex(s,
        if(dst[1]>minval) glow = true;
        if(dst[2]>minval) { glow = envmap = true; goto needmask; }
    );
    if(!glow && !envmap) { s.cleanup(); return; }
needmask:
    ImageData m(s.w, s.h, envmap ? 2 : 1);
    readwritetex(m, s,
        dst[0] = uchar(src[1]*glowscale);
        if(envmap) dst[1] = uchar(src[2]*envscale);
    );
    s.replace(m);
}

void texdup(ImageData &s, int srcchan, int dstchan)
{
    if(srcchan==dstchan || max(srcchan, dstchan) >= s.bpp) return;
    writetex(s, dst[dstchan] = dst[srcchan]);
}

void texdecal(ImageData &s)
{
    if(renderpath!=R_FIXEDFUNCTION || hasTE) return;
    ImageData m(s.w, s.w, 2);
    readwritetex(m, s,
        dst[0] = src[0];
        dst[1] = 255 - src[0];
    );
    s.replace(m);
}

void texmix(ImageData &s, int c1, int c2, int c3, int c4)
{
    int numchans = c1 < 0 ? 0 : (c2 < 0 ? 1 : (c3 < 0 ? 2 : (c4 < 0 ? 3 : 4)));
    if(numchans <= 0) return;
    ImageData d(s.w, s.h, numchans);
    readwritetex(d, s,
        switch(numchans)
        {
            case 4: dst[3] = src[c4];
            case 3: dst[2] = src[c3];
            case 2: dst[1] = src[c2];
            case 1: dst[0] = src[c1];
        }
    );
    s.replace(d);
}

void texgrey(ImageData &s)
{
    if(s.bpp <= 2) return;
    ImageData d(s.w, s.h, s.bpp >= 4 ? 2 : 1);
    if(s.bpp >= 4)
    {
        readwritetex(d, s,
            dst[0] = src[0];
            dst[1] = src[3];
        );
    }
    else
    {
        readwritetex(d, s, dst[0] = src[0]);
    }
    s.replace(d);
}

VAR(0, hwtexsize, 1, 0, 0);
VAR(0, hwcubetexsize, 1, 0, 0);
VAR(0, hwmaxaniso, 1, 0, 0);
VARF(IDF_PERSIST, maxtexsize, 0, 0, 1<<12, initwarning("texture quality", INIT_LOAD));
VARF(IDF_PERSIST, texreduce, 0, 0, 12, initwarning("texture quality", INIT_LOAD));
VARF(IDF_PERSIST, texcompress, 0, 1<<10, 1<<12, initwarning("texture quality", INIT_LOAD));
VARF(IDF_PERSIST, texcompressquality, -1, -1, 1, setuptexcompress());
VARF(IDF_PERSIST, trilinear, 0, 1, 1, initwarning("texture filtering", INIT_LOAD));
VARF(IDF_PERSIST, bilinear, 0, 1, 1, initwarning("texture filtering", INIT_LOAD));
VARF(IDF_PERSIST, aniso, 0, 0, 16, initwarning("texture filtering", INIT_LOAD));

void setuptexcompress()
{
    if(!hasTC) return;

    GLenum hint = GL_DONT_CARE;
    switch(texcompressquality)
    {
        case 1: hint = GL_NICEST; break;
        case 0: hint = GL_FASTEST; break;
    }
    glHint(GL_TEXTURE_COMPRESSION_HINT_ARB, hint);
}

GLenum compressedformat(GLenum format, int w, int h, int force = 0)
{
    if(hasTC && texcompress && force >= 0 && (force || max(w, h) >= texcompress)) switch(format)
    {
        case GL_RGB5:
        case GL_RGB8:
#ifdef __APPLE__
        case GL_RGB: return GL_COMPRESSED_RGB_ARB;
        case GL_RGBA: return GL_COMPRESSED_RGBA_ARB;
#else
        case GL_RGB: return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        case GL_RGBA: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
#endif
    }
    return format;
}

int formatsize(GLenum format)
{
    switch(format)
    {
        case GL_LUMINANCE:
        case GL_ALPHA: return 1;
        case GL_LUMINANCE_ALPHA: return 2;
        case GL_RGB: return 3;
        case GL_RGBA: return 4;
        default: return 4;
    }
}

VARF(IDF_PERSIST, hwmipmap, 0, 0, 1, initwarning("texture filtering", INIT_LOAD));
VARF(IDF_PERSIST, usenp2, 0, 0, 1, initwarning("texture quality", INIT_LOAD));

void resizetexture(int w, int h, bool mipmap, bool canreduce, GLenum target, int compress, int &tw, int &th)
{
    int hwlimit = target==GL_TEXTURE_CUBE_MAP_ARB ? hwcubetexsize : hwtexsize,
        sizelimit = mipmap && maxtexsize ? min(maxtexsize, hwlimit) : hwlimit;
    if(compress > 0 && !hasTC)
    {
        w = max(w/compress, 1);
        h = max(h/compress, 1);
    }
    if(canreduce && texreduce)
    {
        w = max(w>>texreduce, 1);
        h = max(h>>texreduce, 1);
    }
    w = min(w, sizelimit);
    h = min(h, sizelimit);
    if((!hasNP2 || !usenp2) && target!=GL_TEXTURE_RECTANGLE_ARB && (w&(w-1) || h&(h-1)))
    {
        tw = th = 1;
        while(tw < w) tw *= 2;
        while(th < h) th *= 2;
        if(w < tw - tw/4) tw /= 2;
        if(h < th - th/4) th /= 2;
    }
    else
    {
        tw = w;
        th = h;
    }
}

void uploadtexture(GLenum target, GLenum internal, int tw, int th, GLenum format, GLenum type, void *pixels, int pw, int ph, int pitch, bool mipmap)
{
    int bpp = formatsize(format), row = 0, rowalign = 0;
    if(!pitch) pitch = pw*bpp;
    uchar *buf = NULL;
    if(pw!=tw || ph!=th)
    {
        buf = new uchar[tw*th*bpp];
        scaletexture((uchar *)pixels, pw, ph, bpp, pitch, buf, tw, th);
    }
    else if(tw*bpp != pitch)
    {
        row = pitch/bpp;
        rowalign = texalign(pixels, pitch, 1);
        while(rowalign > 0 && ((row*bpp + rowalign - 1)/rowalign)*rowalign != pitch) rowalign >>= 1;
        if(!rowalign)
        {
            row = 0;
            buf = new uchar[tw*th*bpp];
            loopi(th) memcpy(&buf[i*tw*bpp], &((uchar *)pixels)[i*pitch], tw*bpp);
        }
    }
    for(int level = 0, align = 0;; level++)
    {
        uchar *src = buf ? buf : (uchar *)pixels;
        if(buf) pitch = tw*bpp;
        int srcalign = row > 0 ? rowalign : texalign(src, pitch, 1);
        if(align != srcalign) glPixelStorei(GL_UNPACK_ALIGNMENT, align = srcalign);
        if(row > 0) glPixelStorei(GL_UNPACK_ROW_LENGTH, row);
        if(target==GL_TEXTURE_1D) glTexImage1D(target, level, internal, tw, 0, format, type, src);
        else glTexImage2D(target, level, internal, tw, th, 0, format, type, src);
        if(row > 0) glPixelStorei(GL_UNPACK_ROW_LENGTH, row = 0);
        if(!mipmap || (hasGM && hwmipmap) || max(tw, th) <= 1) break;
        int srcw = tw, srch = th;
        if(tw > 1) tw /= 2;
        if(th > 1) th /= 2;
        if(!buf) buf = new uchar[tw*th*bpp];
        scaletexture(src, srcw, srch, bpp, pitch, buf, tw, th);
    }
    if(buf) delete[] buf;
}

void uploadcompressedtexture(GLenum target, GLenum format, int w, int h, uchar *data, int align, int blocksize, int levels, bool mipmap)
{
    int hwlimit = target==GL_TEXTURE_CUBE_MAP_ARB ? hwcubetexsize : hwtexsize,
        sizelimit = levels > 1 && maxtexsize ? min(maxtexsize, hwlimit) : hwlimit;
    int level = 0;
    loopi(levels)
    {
        int size = ((w + align-1)/align) * ((h + align-1)/align) * blocksize;
        if(w <= sizelimit && h <= sizelimit)
        {
            if(target==GL_TEXTURE_1D) glCompressedTexImage1D_(target, level, format, w, 0, size, data);
           else glCompressedTexImage2D_(target, level, format, w, h, 0, size, data);
            level++;
            if(!mipmap) break;
        }
        if(max(w, h) <= 1) break;
        if(w > 1) w /= 2;
        if(h > 1) h /= 2;
        data += size;
    }
}

GLenum textarget(GLenum subtarget)
{
    switch(subtarget)
    {
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
            return GL_TEXTURE_CUBE_MAP_ARB;
            break;
    }
    return subtarget;
}

GLenum uncompressedformat(GLenum format)
{
    switch(format)
    {
        case GL_COMPRESSED_RGB_ARB:
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            return GL_RGB;
        case GL_COMPRESSED_RGBA_ARB:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return GL_RGBA;
    }
    return GL_FALSE;
}

void setuptexparameters(int tnum, void *pixels, int clamp, int filter, GLenum format, GLenum target)
{
    glBindTexture(target, tnum);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, clamp&1 ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    if(target!=GL_TEXTURE_1D) glTexParameteri(target, GL_TEXTURE_WRAP_T, clamp&2 ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    if(target==GL_TEXTURE_2D && hasAF && min(aniso, hwmaxaniso) > 0 && filter > 1) glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, min(aniso, hwmaxaniso));
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter && bilinear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
        filter > 1 ?
            (trilinear ?
                (bilinear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR) :
                (bilinear ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST)) :
            (filter && bilinear ? GL_LINEAR : GL_NEAREST));
    if(hasGM && filter > 1 && pixels && hwmipmap && !uncompressedformat(format))
        glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
}

void createtexture(int tnum, int w, int h, void *pixels, int clamp, int filter, GLenum component, GLenum subtarget, int pw, int ph, int pitch, bool resize)
{
    GLenum target = textarget(subtarget), format = component, type = GL_UNSIGNED_BYTE;
    switch(component)
    {
        case GL_FLOAT_RG16_NV:
        case GL_FLOAT_R32_NV:
        case GL_RGB16F_ARB:
        case GL_RGB32F_ARB:
            format = GL_RGB;
            type = GL_FLOAT;
            break;

        case GL_RGBA16F_ARB:
        case GL_RGBA32F_ARB:
            format = GL_RGBA;
            type = GL_FLOAT;
            break;

        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32:
            format = GL_DEPTH_COMPONENT;
            break;

        case GL_RGB5:
        case GL_RGB8:
        case GL_RGB16:
        case GL_COMPRESSED_RGB_ARB:
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            format = GL_RGB;
            break;

        case GL_RGBA8:
        case GL_RGBA16:
        case GL_COMPRESSED_RGBA_ARB:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            format = GL_RGBA;
            break;
    }
    if(tnum) setuptexparameters(tnum, pixels, clamp, filter, format, target);
    if(!pw) pw = w;
    if(!ph) ph = h;
    int tw = w, th = h;
    bool mipmap = filter > 1 && pixels;
    if(resize && pixels)
    {
        resizetexture(w, h, mipmap, false, target, 0, tw, th);
        if(mipmap) component = compressedformat(component, tw, th);
    }
    uploadtexture(subtarget, component, tw, th, format, type, pixels, pw, ph, pitch, mipmap);
}

void createcompressedtexture(int tnum, int w, int h, uchar *data, int align, int blocksize, int levels, int clamp, int filter, GLenum format, GLenum subtarget)
{
    GLenum target = textarget(subtarget);
    if(tnum) setuptexparameters(tnum, data, clamp, filter, format, target);
    uploadcompressedtexture(target, format, w, h, data, align, blocksize, levels, filter > 1);
}

hashtable<char *, Texture> textures;

Texture *notexture = NULL, *blanktexture = NULL; // used as default, ensured to be loaded

void updatetexture(Texture *t)
{
    if(t->frames.length())
    {
        if(t->delay)
        {
            while(lastmillis-t->last >= t->delay)
            {
                t->frame++;
                if(t->frame >= t->frames.length()) t->frame = 0;
                t->last += t->delay;
            }

            if(t->frames.inrange(t->frame)) t->id = t->frames[t->frame];
            else t->id = t->frames[0];

            if(t->id <= 0)
                t->id = t != notexture ? notexture->id : 0;
        }
        else t->id = t->frames[0];
    }
    else t->id = 0;
}

void updatetextures()
{
    enumerate(textures, Texture, tex, updatetexture(&tex));
}

void preloadtextures()
{
    enumerate(*idents, ident, id, {
        if(id.type == ID_SVAR && (id.flags & IDF_TEXTURE))
            id.changed();
    });
}

static GLenum texformat(int bpp)
{
    switch(bpp)
    {
        case 1: return GL_LUMINANCE;
        case 2: return GL_LUMINANCE_ALPHA;
        case 3: return GL_RGB;
        case 4: return GL_RGBA;
        default: return 0;
    }
}

int texalign(void *data, int w, int bpp)
{
    size_t address = size_t(data) | (w*bpp);
    if(address&1) return 1;
    if(address&2) return 2;
    if(address&4) return 4;
    return 8;
}

static Texture *newtexture(Texture *t, const char *rname, ImageData &s, int clamp = 0, bool mipit = true, bool canreduce = false, bool transient = false, int compress = 0, TextureAnim *anim = NULL)
{
    if(!t)
    {
        char *key = newstring(rname);
        t = &textures[key];
        t->name = key;
        t->frames.shrink(0);
        t->frame = 0;
    }

    t->clamp = clamp;
    t->mipmap = mipit;
    t->type = Texture::IMAGE;
    if(transient) t->type |= Texture::TRANSIENT;
    if(!s.data)
    {
        t->type |= Texture::STUB;
        t->w = t->h = t->xs = t->ys = t->bpp = 0;
        return t;
    }
    if(s.compressed) t->type |= Texture::COMPRESSED;

    bool hasanim = anim && anim->count;
    t->bpp = s.compressed ? formatsize(uncompressedformat(s.compressed)) : s.bpp;
    t->delay = hasanim ? anim->delay : 0;

    t->w = t->xs = hasanim ? anim->w : s.w;
    t->h = t->ys = hasanim ? anim->h : s.h;

    if(s.compressed)
    {
        if(t->frames.empty()) t->frames.add(0);
        glGenTextures(1, &t->frames[0]);

        uchar *data = s.data;
        int levels = s.levels, level = 0;
        if(canreduce && texreduce) loopi(min(texreduce, s.levels-1))
        {
            data += s.calclevelsize(level++);
            levels--;
            if(t->w > 1) t->w /= 2;
            if(t->h > 1) t->h /= 2;
        }
        int sizelimit = mipit && maxtexsize ? min(maxtexsize, hwtexsize) : hwtexsize;
        while(t->w > sizelimit || t->h > sizelimit)
        {
            data += s.calclevelsize(level++);
            levels--;
            if(t->w > 1) t->w /= 2;
            if(t->h > 1) t->h /= 2;
        }
        createcompressedtexture(t->frames[0], t->w, t->h, data, s.align, s.bpp, levels, clamp, mipit ? 2 : 1, s.compressed, GL_TEXTURE_2D);
    }
    else
    {
        resizetexture(t->w, t->h, mipit, canreduce, GL_TEXTURE_2D, compress, t->w, t->h);
        GLenum format = compressedformat(texformat(t->bpp), t->w, t->h, compress);

        loopi(hasanim ? anim->count : 1)
        {
            while(!t->frames.inrange(i)) t->frames.add(0);
            glGenTextures(1, &t->frames[i]);

            ImageData cropped;
            uchar *data = s.data;
            int pitch = s.pitch;
            if(hasanim)
            {
                int sx = (i%anim->x)*anim->w, sy = (((i-(i%anim->x))/anim->x)%anim->y)*anim->h;
                texcrop(s, cropped, sx, sy, anim->w, anim->h);
                data = cropped.data;
                pitch = cropped.pitch;
            }

            createtexture(t->frames[i], t->w, t->h, data, clamp, mipit ? 2 : 1, format, GL_TEXTURE_2D, t->xs, t->ys, pitch, false);

            if(verbose >= 3)
                conoutf("\faadding frame: %s (%d) [%d,%d:%d,%d]", t->name, i+1, t->w, t->h, t->xs, t->ys);
        }
    }
    updatetexture(t);
    return t;
}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define RGBAMASKS 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#define RGBMASKS  0xff0000, 0x00ff00, 0x0000ff, 0
#else
#define RGBAMASKS 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#define RGBMASKS  0x0000ff, 0x00ff00, 0xff0000, 0
#endif

SDL_Surface *wrapsurface(void *data, int width, int height, int bpp)
{
    switch(bpp)
    {
        case 3: return SDL_CreateRGBSurfaceFrom(data, width, height, 8*bpp, bpp*width, RGBMASKS);
        case 4: return SDL_CreateRGBSurfaceFrom(data, width, height, 8*bpp, bpp*width, RGBAMASKS);
    }
    return NULL;
}

SDL_Surface *creatergbsurface(SDL_Surface *os)
{
    SDL_Surface *ns = SDL_CreateRGBSurface(SDL_SWSURFACE, os->w, os->h, 24, RGBMASKS);
    if(ns) SDL_BlitSurface(os, NULL, ns, NULL);
    SDL_FreeSurface(os);
    return ns;
}

SDL_Surface *creatergbasurface(SDL_Surface *os)
{
    SDL_Surface *ns = SDL_CreateRGBSurface(SDL_SWSURFACE, os->w, os->h, 32, RGBAMASKS);
    if(ns)
    {
        SDL_SetAlpha(os, 0, 0);
        SDL_BlitSurface(os, NULL, ns, NULL);
    }
    SDL_FreeSurface(os);
    return ns;
}

bool checkgrayscale(SDL_Surface *s)
{
    // gray scale images have 256 levels, no colorkey, and the palette is a ramp
    if(s->format->palette)
    {
        if(s->format->palette->ncolors != 256 || s->format->colorkey) return false;
        const SDL_Color *colors = s->format->palette->colors;
        loopi(256) if(colors[i].r != i || colors[i].g != i || colors[i].b != i) return false;
    }
    return true;
}

SDL_Surface *fixsurfaceformat(SDL_Surface *s)
{
    if(!s) return NULL;
    if(!s->pixels || min(s->w, s->h) <= 0 || s->format->BytesPerPixel <= 0)
    {
        SDL_FreeSurface(s);
        return NULL;
    }
    static const uint rgbmasks[] = { RGBMASKS }, rgbamasks[] = { RGBAMASKS };
    switch(s->format->BytesPerPixel)
    {
        case 1:
            if(!checkgrayscale(s)) return s->format->colorkey ? creatergbasurface(s) : creatergbsurface(s);
            break;
        case 3:
            if(s->format->Rmask != rgbmasks[0] || s->format->Gmask != rgbmasks[1] || s->format->Bmask != rgbmasks[2])
                return creatergbsurface(s);
            break;
        case 4:
            if(s->format->Rmask != rgbamasks[0] || s->format->Gmask != rgbamasks[1] || s->format->Bmask != rgbamasks[2] || s->format->Amask != rgbamasks[3])
                return s->format->Amask ? creatergbasurface(s) : creatergbsurface(s);
            break;
    }
    return s;
}

void texflip(ImageData &s)
{
    ImageData d(s.w, s.h, s.bpp);
    uchar *dst = d.data, *src = &s.data[s.pitch*s.h];
    loopi(s.h)
    {
        src -= s.pitch;
        memcpy(dst, src, s.bpp*s.w);
        dst += d.pitch;
    }
    s.replace(d);
}

void texnormal(ImageData &s, int emphasis)
{
    ImageData d(s.w, s.h, 3);
    uchar *src = s.data, *dst = d.data;
    loop(y, s.h) loop(x, s.w)
    {
        vec normal(0.0f, 0.0f, 255.0f/emphasis);
        normal.x += src[y*s.pitch + ((x+s.w-1)%s.w)*s.bpp];
        normal.x -= src[y*s.pitch + ((x+1)%s.w)*s.bpp];
        normal.y += src[((y+s.h-1)%s.h)*s.pitch + x*s.bpp];
        normal.y -= src[((y+1)%s.h)*s.pitch + x*s.bpp];
        normal.normalize();
        *dst++ = uchar(127.5f + normal.x*127.5f);
        *dst++ = uchar(127.5f + normal.y*127.5f);
        *dst++ = uchar(127.5f + normal.z*127.5f);
    }
    s.replace(d);
}

void scaleimage(ImageData &s, int w, int h)
{
    ImageData d(w, h, s.bpp);
    scaletexture(s.data, s.w, s.h, s.bpp, s.pitch, d.data, w, h);
    s.replace(d);
}

static vec parsevec(const char *arg)
{
    vec v(0, 0, 0);
    int i = 0;
    for(; arg[0] && (!i || arg[0]=='/') && i<3; arg += strcspn(arg, "/,><"), i++)
    {
        if(i) arg++;
        v[i] = atof(arg);
    }
    if(i==1) v.y = v.z = v.x;
    return v;
}

VAR(0, usedds, 0, 1, 1);

static bool texturedata(ImageData &d, const char *tname, Slot::Tex *tex = NULL, bool msg = true, int *compress = NULL, TextureAnim *anim = NULL)
{
    const char *cmds = NULL, *file = tname;
    if(!tname && tex) file = tname = tex->name;
    if(tname && tname[0]=='<')
    {
        cmds = tname;
        file = strrchr(tname, '>');
        if(!file) file = tex ? tex->name : NULL;
        else file++;
    }

    if(!file) { if(msg) conoutf("\frcould not load texture %s", tname); return NULL; }

    bool raw = !usedds || !compress, dds = false;
    for(const char *pcmds = cmds; pcmds;)
    {
        #define PARSETEXCOMMANDS(cmds) \
            const char *cmd = NULL, *end = NULL, *arg[4] = { NULL, NULL, NULL, NULL }; \
            cmd = &cmds[1]; \
            end = strchr(cmd, '>'); \
            if(!end) break; \
            cmds = strchr(cmd, '<'); \
            size_t len = strcspn(cmd, ":,><"); \
            loopi(4) \
            { \
                arg[i] = strchr(i ? arg[i-1] : cmd, i ? ',' : ':'); \
                if(!arg[i] || arg[i] >= end) arg[i] = ""; \
                else arg[i]++; \
            }
        PARSETEXCOMMANDS(pcmds);
        if(!strncmp(cmd, "noff", len))
        {
            if(renderpath==R_FIXEDFUNCTION) return true;
        }
        else if(!strncmp(cmd, "ffmask", len))
        {
            if(renderpath==R_FIXEDFUNCTION) raw = true;
        }
        else if(!strncmp(cmd, "decal", len))
        {
            if(renderpath==R_FIXEDFUNCTION && !hasTE) raw = true;
        }
        else if(!strncmp(cmd, "dds", len)) dds = true;
        else if(!strncmp(cmd, "thumbnail", len)) raw = true;
        else if(!strncmp(cmd, "stub", len)) return loadsurface(file, true)!=NULL;
    }

    if(msg) progress(loadprogress, file);

    int flen = strlen(file);
    if((flen >= 4 && !strcasecmp(file + flen - 4, ".dds")) || dds)
    {
        string dfile;
        copystring(dfile, file);
        if(flen >= 4 && dfile[flen-4]=='.') memcpy(dfile + flen - 4, ".dds", 4);
        else concatstring(dfile, ".dds");
        if(!raw && hasTC && loaddds(dfile, d)) return true;
        if(!dds) { if(msg) conoutf("\frcould not load texture %s", dfile); return false; }
    }

    SDL_Surface *s = loadsurface(file);
    if(!s) { if(msg) conoutf("\frcould not load texture %s", file); return false; }
    int bpp = s->format->BitsPerPixel;
    if(bpp%8 || !texformat(bpp/8)) { SDL_FreeSurface(s); conoutf("\frtexture must be 8, 16, 24, or 32 bpp: %s", file); return false; }
    d.wrap(s);

    while(cmds)
    {
        PARSETEXCOMMANDS(cmds);
        if(!strncmp(cmd, "mad", len)) texmad(d, parsevec(arg[0]), parsevec(arg[1]));
        else if(!strncmp(cmd, "colorify", len)) texcolorify(d, parsevec(arg[0]), parsevec(arg[1]));
        else if(!strncmp(cmd, "ffmask", len))
        {
            texffmask(d, atof(arg[0]), atof(arg[1]));
            if(!d.data) return true;
        }
        else if(!strncmp(cmd, "normal", len))
        {
            int emphasis = atoi(arg[0]);
            texnormal(d, emphasis > 0 ? emphasis : 3);
        }
        else if(!strncmp(cmd, "dup", len)) texdup(d, atoi(arg[0]), atoi(arg[1]));
        else if(!strncmp(cmd, "decal", len)) texdecal(d);
        else if(!strncmp(cmd, "offset", len)) texoffset(d, atoi(arg[0]), atoi(arg[1]));
        else if(!strncmp(cmd, "rotate", len)) texrotate(d, atoi(arg[0]), tex ? tex->type : 0);
        else if(!strncmp(cmd, "reorient", len)) texreorient(d, atoi(arg[0])>0, atoi(arg[1])>0, atoi(arg[2])>0, tex ? tex->type : TEX_DIFFUSE);
        else if(!strncmp(cmd, "crop", len)) texcrop(d, atoi(arg[0]), atoi(arg[1]), atoi(arg[2]), atoi(arg[3]));
        else if(!strncmp(cmd, "mix", len)) texmix(d, *arg[0] ? atoi(arg[0]) : -1, *arg[1] ? atoi(arg[1]) : -1, *arg[2] ? atoi(arg[2]) : -1, *arg[3] ? atoi(arg[3]) : -1);
        else if(!strncmp(cmd, "grey", len)) texgrey(d);
        else if(!strncmp(cmd, "compress", len) || !strncmp(cmd, "dds", len))
        {
            int scale = atoi(arg[0]);
            if(scale <= 0) scale = 2;
            if(compress) *compress = scale;
        }
        else if(!strncmp(cmd, "nocompress", len))
        {
            if(compress) *compress = -1;
        }
        else if(!strncmp(cmd, "anim", len))
        {
            if(anim)
            {
                anim->delay = arg[0] ? atoi(arg[0]) : 0;
                anim->w = arg[1] ? atoi(arg[1]) : 0;
                anim->h = arg[2] ? atoi(arg[2]) : 0;
                if(!anim->w) anim->w = d.h < d.w ? d.h : d.w;
                if(!anim->h) anim->h = d.h < d.w ? d.h : d.w;
                anim->x = d.w/anim->w;
                anim->y = d.h/anim->h;
                anim->count = anim->x*anim->y;
            }
        }
    }
    if((anim && anim->count ? max(anim->w, anim->h) : max(d.w, d.h)) > (1<<12))
    {
        d.cleanup();
        conoutf("\frtexture size exceeded %dx%d: %s", 1<<12, 1<<12, file);
        return false;
    }
    return true;
}

void loadalphamask(Texture *t)
{
    if(t->alphamask || t->bpp!=4 || t->type&Texture::COMPRESSED) return;
    ImageData s;
    if(!texturedata(s, t->name, NULL, false) || !s.data || s.bpp!=4 || s.compressed) return;
    t->alphamask = new uchar[s.h * ((s.w+7)/8)];
    uchar *srcrow = s.data, *dst = t->alphamask-1;
    loop(y, s.h)
    {
        uchar *src = srcrow+s.bpp-1;
        loop(x, s.w)
        {
            int offset = x%8;
            if(!offset) *++dst = 0;
            if(*src) *dst |= 1<<offset;
            src += s.bpp;
        }
        srcrow += s.pitch;
    }
}

Texture *textureload(const char *name, int clamp, bool mipit, bool msg)
{
    string tname;
    copystring(tname, name);
    path(tname);
    Texture *t = textures.access(tname);
    if(!t)
    {
        ImageData s;
        TextureAnim anim;
        int compress = 0;
        if(texturedata(s, tname, NULL, msg, &compress, &anim))
            t = newtexture(NULL, tname, s, clamp, mipit, false, false, compress, &anim);
        else t = notexture;
    }
    return t;
}

void settexture(const char *name, int clamp)
{
    glBindTexture(GL_TEXTURE_2D, textureload(name, clamp, true, false)->id);
}


vector<VSlot *> vslots;
vector<Slot *> slots;
MSlot materialslots[MATF_VOLUME+1];
Slot dummyslot;
VSlot dummyvslot(&dummyslot);

void resettextures()
{
    resetslotshader();
    loopv(slots)
    {
        Slot *s = slots[i];
        for(VSlot *vs = s->variants; vs; vs = vs->next) vs->slot = &dummyslot;
        delete s;
    }
    slots.shrink(0);
}

ICOMMAND(0, texturereset, "", (void), if(editmode || worldidents) resettextures(););

void resetmaterials()
{
    loopi(MATF_VOLUME+1) materialslots[i].reset();
}

ICOMMAND(0, materialreset, "", (void), if(editmode || worldidents) resetmaterials(););

static int compactedvslots = 0, compactvslotsprogress = 0, clonedvslots = 0;
static bool markingvslots = false;

void clearslots()
{
    resetslotshader();
    slots.deletecontents();
    vslots.deletecontents();
    loopi(MATF_VOLUME+1) materialslots[i].reset();
    clonedvslots = 0;
}

void compactvslot(int &index)
{
    if(vslots.inrange(index))
    {
        VSlot &vs = *vslots[index];
        if(vs.index < 0) vs.index = compactedvslots++;
        if(!markingvslots) index = vs.index;
    }
}

void compactvslots(cube *c, int n)
{
    if((compactvslotsprogress++&0xFFF)==0) progress(min(float(compactvslotsprogress)/allocnodes, 1.0f), markingvslots ? "marking slots..." : "compacting slots...");
    loopi(n)
    {
        if(c[i].children) compactvslots(c[i].children);
        else loopj(6) if(vslots.inrange(c[i].texture[j]))
        {
            VSlot &vs = *vslots[c[i].texture[j]];
            if(vs.index < 0) vs.index = compactedvslots++;
            if(!markingvslots) c[i].texture[j] = vs.index;
        }
    }
}

void compactvslotlayers()
{
    loopv(vslots)
    {
        VSlot &vs = *vslots[i];
        if(vs.index >= 0 && vs.layer && vslots.inrange(vs.layer))
        {
            VSlot &layer = *vslots[vs.layer];
            if(layer.index < 0) layer.index = compactedvslots++;
            if(!markingvslots) vs.layer = layer.index;
        }
    }
}

int compactvslots(bool cull)
{
    clonedvslots = 0;
    markingvslots = cull;
    compactedvslots = 0;
    compactvslotsprogress = 0;
    loopv(vslots) vslots[i]->index = -1;
    if(!cull)
    {
        loopv(slots) slots[i]->variants->index = compactedvslots++;
        loopv(vslots)
        {
            VSlot &vs = *vslots[i];
            if(!vs.changed && vs.index < 0) { markingvslots = true; break; }
        }
    }
    compactvslots(worldroot);
    compactvslotlayers();
    int total = compactedvslots;
    compacteditvslots();
    compactvslotlayers();
    loopv(vslots)
    {
        VSlot *vs = vslots[i];
        if(vs->changed) continue;
        while(vs->next)
        {
            if(vs->next->index < 0) vs->next = vs->next->next;
            else vs = vs->next;
        }
    }
    if(markingvslots)
    {
        markingvslots = false;
        compactedvslots = 0;
        compactvslotsprogress = 0;
        int lastdiscard = 0;
        loopv(vslots)
        {
            VSlot &vs = *vslots[i];
            if(vs.changed || (vs.index < 0 && !vs.next)) vs.index = -1;
            else
            {
                if(cull) lastdiscard = i;
                else while(lastdiscard < i)
                {
                    VSlot &ds = *vslots[lastdiscard++];
                    if(!ds.changed && ds.index < 0) ds.index = compactedvslots++;
                }
                vs.index = compactedvslots++;
            }
        }
        compactvslots(worldroot);
        compactvslotlayers();
        total = compactedvslots;
        compacteditvslots();
        compactvslotlayers();
    }
    compactmruvslots();
    if(cull)
    {
        loopvrev(slots)
        {
            Slot *s = slots[i];
            if(s->variants->index < 0) delete slots.remove(i);
        }
        loopv(slots) slots[i]->index = i;
    }
    loopv(vslots)
    {
        while(vslots[i]->index >= 0 && vslots[i]->index != i)
            swap(vslots[i], vslots[vslots[i]->index]);
    }
    for(int i = compactedvslots; i < vslots.length(); i++) delete vslots[i];
    vslots.setsize(compactedvslots);
    return total;
}

ICOMMAND(0, compactvslots, "", (),
{
    if(multiplayer()) return;
    compactvslots();
    allchanged();
});

static Slot &loadslot(Slot &s, bool forceload);

static void clampvslotoffset(VSlot &dst, Slot *slot = NULL)
{
    if(!slot) slot = dst.slot;
    if(slot && slot->sts.inrange(0))
    {
        if(!slot->loaded) loadslot(*slot, false);
        int xs = slot->sts[0].t->xs, ys = slot->sts[0].t->ys;
        if((dst.rotation&5)==1) swap(xs, ys);
        dst.xoffset %= xs; if(dst.xoffset < 0) dst.xoffset += xs;
        dst.yoffset %= ys; if(dst.yoffset < 0) dst.yoffset += ys;
    }
    else
    {
        dst.xoffset = max(dst.xoffset, 0);
        dst.yoffset = max(dst.yoffset, 0);
    }
}

static void propagatevslot(VSlot &dst, const VSlot &src, int diff)
{
    if(diff & (1<<VSLOT_SHPARAM)) loopv(src.params) dst.params.add(src.params[i]);
    if(diff & (1<<VSLOT_SCALE)) dst.scale = src.scale;
    if(diff & (1<<VSLOT_ROTATION))
    {
        dst.rotation = src.rotation;
        if(dst.xoffset || dst.yoffset) clampvslotoffset(dst);
    }
    if(diff & (1<<VSLOT_OFFSET))
    {
        dst.xoffset = src.xoffset;
        dst.yoffset = src.yoffset;
        clampvslotoffset(dst);
    }
    if(diff & (1<<VSLOT_SCROLL))
    {
        dst.scrollS = src.scrollS;
        dst.scrollT = src.scrollT;
    }
    if(diff & (1<<VSLOT_LAYER)) dst.layer = src.layer;
    if(diff & (1<<VSLOT_ALPHA))
    {
        dst.alphafront = src.alphafront;
        dst.alphaback = src.alphaback;
    }
    if(diff & (1<<VSLOT_COLOR)) dst.colorscale = src.colorscale;
}

static void propagatevslot(VSlot *root, int changed)
{
    for(VSlot *vs = root->next; vs; vs = vs->next)
    {
        int diff = changed & ~vs->changed;
        if(diff) propagatevslot(*vs, *root, diff);
    }
}

static void mergevslot(VSlot &dst, const VSlot &src, int diff, Slot *slot = NULL)
{
    if(diff & (1<<VSLOT_SHPARAM)) loopv(src.params)
    {
        const ShaderParam &sp = src.params[i];
        loopvj(dst.params)
        {
            ShaderParam &dp = dst.params[j];
            if(sp.name == dp.name)
            {
                memcpy(dp.val, sp.val, sizeof(dp.val));
                goto nextparam;
            }
        }
        dst.params.add(sp);
    nextparam:;
    }
    if(diff & (1<<VSLOT_SCALE))
    {
        dst.scale = clamp(dst.scale*src.scale, 1/8.0f, 8.0f);
    }
    if(diff & (1<<VSLOT_ROTATION))
    {
        dst.rotation = clamp(dst.rotation + src.rotation, 0, 5);
        if(dst.xoffset || dst.yoffset) clampvslotoffset(dst, slot);
    }
    if(diff & (1<<VSLOT_OFFSET))
    {
        dst.xoffset += src.xoffset;
        dst.yoffset += src.yoffset;
        clampvslotoffset(dst, slot);
    }
    if(diff & (1<<VSLOT_SCROLL))
    {
        dst.scrollS += src.scrollS;
        dst.scrollT += src.scrollT;
    }
    if(diff & (1<<VSLOT_LAYER)) dst.layer = src.layer;
    if(diff & (1<<VSLOT_ALPHA))
    {
        dst.alphafront = src.alphafront;
        dst.alphaback = src.alphaback;
    }
    if(diff & (1<<VSLOT_COLOR)) dst.colorscale.mul(src.colorscale);
}

void mergevslot(VSlot &dst, const VSlot &src, const VSlot &delta)
{
    dst.changed = src.changed | delta.changed;
    propagatevslot(dst, src, (1<<VSLOT_NUM)-1);
    mergevslot(dst, delta, delta.changed, src.slot);
}

static VSlot *reassignvslot(Slot &owner, VSlot *vs)
{
    owner.variants = vs;
    while(vs)
    {
        vs->slot = &owner;
        vs->linked = false;
        vs = vs->next;
    }
    return owner.variants;
}

static VSlot *emptyvslot(Slot &owner)
{
    int offset = 0;
    loopvrev(slots) if(slots[i]->variants) { offset = slots[i]->variants->index + 1; break; }
    for(int i = offset; i < vslots.length(); i++) if(!vslots[i]->changed) return reassignvslot(owner, vslots[i]);
    return vslots.add(new VSlot(&owner, vslots.length()));
}

static bool comparevslot(const VSlot &dst, const VSlot &src, int diff)
{
    if(diff & (1<<VSLOT_SHPARAM))
    {
        if(src.params.length() != dst.params.length()) return false;
        loopv(src.params)
        {
            const ShaderParam &sp = src.params[i], &dp = dst.params[i];
            if(sp.name != dp.name || memcmp(sp.val, dp.val, sizeof(sp.val))) return false;
        }
    }
    if(diff & (1<<VSLOT_SCALE) && dst.scale != src.scale) return false;
    if(diff & (1<<VSLOT_ROTATION) && dst.rotation != src.rotation) return false;
    if(diff & (1<<VSLOT_OFFSET) && (dst.xoffset != src.xoffset || dst.yoffset != src.yoffset)) return false;
    if(diff & (1<<VSLOT_SCROLL) && (dst.scrollS != src.scrollS || dst.scrollT != src.scrollT)) return false;
    if(diff & (1<<VSLOT_LAYER) && dst.layer != src.layer) return false;
    if(diff & (1<<VSLOT_ALPHA) && (dst.alphafront != src.alphafront || dst.alphaback != src.alphaback)) return false;
    if(diff & (1<<VSLOT_COLOR) && dst.colorscale != src.colorscale) return false;
    return true;
}

VSlot *findvslot(Slot &slot, const VSlot &src, const VSlot &delta)
{
    for(VSlot *dst = slot.variants; dst; dst = dst->next)
    {
        if((!dst->changed || dst->changed == (src.changed | delta.changed)) &&
           comparevslot(*dst, src, src.changed & ~delta.changed) &&
           comparevslot(*dst, delta, delta.changed))
            return dst;
    }
    return NULL;
}

static VSlot *clonevslot(const VSlot &src, const VSlot &delta)
{
    VSlot *dst = vslots.add(new VSlot(src.slot, vslots.length()));
    dst->changed = src.changed | delta.changed;
    propagatevslot(*dst, src, ((1<<VSLOT_NUM)-1) & ~delta.changed);
    propagatevslot(*dst, delta, delta.changed);
    return dst;
}

VAR(IDF_PERSIST, autocompactvslots, 0, 256, 0x10000);

VSlot *editvslot(const VSlot &src, const VSlot &delta)
{
    VSlot *exists = findvslot(*src.slot, src, delta);
    if(exists) return exists;
    if(vslots.length()>=0x10000)
    {
        compactvslots();
        allchanged();
        if(vslots.length()>=0x10000) return NULL;
    }
    if(autocompactvslots && ++clonedvslots >= autocompactvslots)
    {
        compactvslots();
        allchanged();
    }
    return clonevslot(src, delta);
}

generic textypes[] =
{
    {"c", TEX_DIFFUSE},
    {"u", TEX_UNKNOWN},
    {"d", TEX_DECAL},
    {"n", TEX_NORMAL},
    {"g", TEX_GLOW},
    {"s", TEX_SPEC},
    {"z", TEX_DEPTH},
    {"e", TEX_ENVMAP}
};

int findtexturetype(char *name, bool tryint)
{
    loopi(sizeof(textypes)/sizeof(textypes[0])) if(!strcmp(textypes[i].name, name)) { return textypes[i].id; }
    return tryint && isnumeric(*name) ? atoi(name) : -1;
}

const char *findtexturename(int type)
{
    loopi(sizeof(textypes)/sizeof(textypes[0])) if(textypes[i].id == type) { return textypes[i].name; }
    return NULL;
}

void texture(char *type, char *name, int *rot, int *xoffset, int *yoffset, float *scale)
{
    if(slots.length()>=0x10000) return;
    static int lastmatslot = -1;
    int tnum = findtexturetype(type, true), matslot = findmaterial(type, false);
    if(tnum < 0) tnum = 0;
    if(tnum==TEX_DIFFUSE) lastmatslot = matslot;
    else if(lastmatslot>=0) matslot = lastmatslot;
    else if(slots.empty()) return;
    Slot &s = matslot>=0 ? materialslots[matslot] : *(tnum!=TEX_DIFFUSE ? slots.last() : slots.add(new Slot(slots.length())));
    s.loaded = false;
    s.texmask |= 1<<tnum;
    if(s.sts.length() >= TEX_MAX) conoutf("\frwarning: too many textures, [%d] %s (%d,%d)", slots.length()-1, name, matslot, lastmatslot);
    Slot::Tex &st = s.sts.add();
    st.type = tnum;
    st.combined = -1;
    st.t = NULL;
    copystring(st.lname, name);
    copystring(st.name, name);
    if(tnum==TEX_DIFFUSE)
    {
        setslotshader(s);
        VSlot &vs = matslot >= 0 ? materialslots[matslot] : *emptyvslot(s);
        vs.reset();
        vs.rotation = clamp(*rot, 0, 5);
        vs.xoffset = max(*xoffset, 0);
        vs.yoffset = max(*yoffset, 0);
        vs.scale = *scale <= 0 ? 1 : *scale;
        propagatevslot(&vs, (1<<VSLOT_NUM)-1);
    }
}

COMMAND(0, texture, "ssiiif");

void texturecull(bool local)
{
    if(slots.length() > 8) compactvslots(true);
}

ICOMMAND(0, texturecull, "", (void), {
    if(slots.length() > 8)
    {
        texturecull(true);
        allchanged();
    }
    else conoutf("\frnot enough texture slots");
});

void autograss(char *name)
{
    if(slots.empty()) return;
    Slot &s = *slots.last();
    DELETEA(s.autograss);
    s.autograss = name[0] ? newstring(name) : NULL;
}
COMMAND(0, autograss, "s");

void texscroll(float *scrollS, float *scrollT)
{
    if(slots.empty()) return;
    Slot &s = *slots.last();
    s.variants->scrollS = *scrollS/1000.0f;
    s.variants->scrollT = *scrollT/1000.0f;
    propagatevslot(s.variants, 1<<VSLOT_SCROLL);
}
COMMAND(0, texscroll, "ff");

void texoffset_(int *xoffset, int *yoffset)
{
    if(slots.empty()) return;
    Slot &s = *slots.last();
    s.variants->xoffset = max(*xoffset, 0);
    s.variants->yoffset = max(*yoffset, 0);
    propagatevslot(s.variants, 1<<VSLOT_OFFSET);
}
COMMANDN(0, texoffset, texoffset_, "ii");

void texrotate_(int *rot)
{
    if(slots.empty()) return;
    Slot &s = *slots.last();
    s.variants->rotation = clamp(*rot, 0, 5);
    propagatevslot(s.variants, 1<<VSLOT_ROTATION);
}
COMMANDN(0, texrotate, texrotate_, "i");

void texlayer(int *layer, char *name, int *mode, float *scale)
{
    if(slots.empty()) return;
    Slot &s = *slots.last();
    s.variants->layer = *layer < 0 ? max(slots.length()-1+*layer, 0) : *layer;
    s.layermaskname = name[0] ? newstring(name) : NULL;
    s.layermaskmode = *mode;
    s.layermaskscale = *scale <= 0 ? 1 : *scale;
    propagatevslot(s.variants, 1<<VSLOT_LAYER);
}
COMMAND(0, texlayer, "isif");

void texscale(float *scale)
{
    if(slots.empty()) return;
    Slot &s = *slots.last();
    s.variants->scale = *scale <= 0 ? 1 : *scale;
    propagatevslot(s.variants, 1<<VSLOT_SCALE);
}
COMMAND(0, texscale, "f");

void texalpha(float *front, float *back)
{
    if(slots.empty()) return;
    Slot &s = *slots.last();
    s.variants->alphafront = clamp(*front, 0.0f, 1.0f);
    s.variants->alphaback = clamp(*back, 0.0f, 1.0f);
    propagatevslot(s.variants, 1<<VSLOT_ALPHA);
}
COMMAND(0, texalpha, "ff");

void texcolor(float *r, float *g, float *b)
{
    if(slots.empty()) return;
    Slot &s = *slots.last();
    s.variants->colorscale = vec(clamp(*r, 0.0f, 1.0f), clamp(*g, 0.0f, 1.0f), clamp(*b, 0.0f, 1.0f));
    propagatevslot(s.variants, 1<<VSLOT_COLOR);
}
COMMAND(0, texcolor, "fff");

void texffenv(int *ffenv)
{
    if(slots.empty()) return;
    Slot &s = *slots.last();
    s.ffenv = *ffenv>0;
}
COMMAND(0, texffenv, "i");

static int findtextype(Slot &s, int type, int last = -1)
{
    for(int i = last+1; i<s.sts.length(); i++) if((type&(1<<s.sts[i].type)) && s.sts[i].combined<0) return i;
    return -1;
}

static void addbump(ImageData &c, ImageData &n, bool envmap, bool specmap)
{
    if(n.bpp < 3) return;
    if(envmap && c.bpp <= 3 && !specmap)
    {
        writetex(n, if(dst[2] < 0xF8) goto noenvmap;);
    }
    if(envmap)
    {
        if(c.bpp <= 3)
        {
            readwritergbatex(c, n,
                int z = max(int(src[2])*2-255, 0);
                loopk(3) dst[k] = int(dst[k])*z/255;
                dst[3] = z;
            );
        }
        else
        {
            readwritergbatex(c, n,
                int z = max(int(src[2])*2-255, 0);
                loopk(4) dst[k] = int(dst[k])*z/255;
            );
        }
    }
    else
    {
    noenvmap:
        readwritergbtex(c, n,
            int z = max(int(src[2])*2-255, 0);
            loopk(3) dst[k] = int(dst[k])*z/255;
        );
    }
}

static void addglow(ImageData &c, ImageData &g, const vec &glowcolor)
{
    if(g.bpp < 3)
    {
        readwritergbtex(c, g,
            loopk(3) dst[k] = clamp(int(dst[k]) + int(src[0]*glowcolor[k]), 0, 255);
        );
    }
    else
    {
        readwritergbtex(c, g,
            loopk(3) dst[k] = clamp(int(dst[k]) + int(src[k]*glowcolor[k]), 0, 255);
        );
    }
}

static void blenddecal(ImageData &c, ImageData &d)
{
    if(d.bpp < 4) return;
    readwritergbtex(c, d,
        uchar a = src[3];
        loopk(3) dst[k] = (int(src[k])*int(a) + int(dst[k])*int(255-a))/255;
    );
}

static void mergespec(ImageData &c, ImageData &s, bool envmap = false)
{
    if(s.bpp < 3)
    {
        if(envmap)
        {
            readwritergbatex(c, s,
                dst[3] = int(dst[3])*int(src[0])/255;
            );
        }
        else
        {
            readwritergbatex(c, s,
                dst[3] = src[0];
            );
        }
    }
    else if(envmap)
    {
        readwritergbatex(c, s,
            dst[3] = int(dst[3])*(int(src[0]) + int(src[1]) + int(src[2]))/(3*255);
        );
    }
    else
    {
        readwritergbatex(c, s,
            dst[3] = (int(src[0]) + int(src[1]) + int(src[2]))/3;
        );
    }
}

static void mergedepth(ImageData &c, ImageData &z)
{
    readwritergbatex(c, z,
        dst[3] = src[0];
    );
}

static void addname(vector<char> &key, Slot &slot, Slot::Tex &t, bool combined = false, const char *prefix = NULL)
{
    if(combined) key.add('&');
    if(prefix) { while(*prefix) key.add(*prefix++); }
    defformatstring(tname)("%s", t.name);
    for(const char *s = tname; *s; key.add(*s++));
}

static void texcombine(Slot &s, int index, Slot::Tex &t, bool forceload = false)
{
    if(renderpath==R_FIXEDFUNCTION && t.type!=TEX_DIFFUSE && t.type!=TEX_GLOW && !forceload) { t.t = notexture; return; }
    vector<char> key;
    addname(key, s, t);
    int texmask = 0;
    bool envmap = renderpath==R_FIXEDFUNCTION && s.shader->type&SHADER_ENVMAP && s.ffenv && hasCM && maxtmus >= 2;
    if(!forceload) switch(t.type)
    {
        case TEX_DIFFUSE:
            if(renderpath==R_FIXEDFUNCTION)
            {
                int mask = (1<<TEX_DECAL)|(1<<TEX_NORMAL);
                if(envmap) mask |= 1<<TEX_SPEC;
                for(int i = -1; (i = findtextype(s, mask, i))>=0;)
                {
                    texmask |= 1<<s.sts[i].type;
                    s.sts[i].combined = index;
                    addname(key, s, s.sts[i], true, envmap && (s.sts[i].type==TEX_NORMAL || s.sts[i].type==TEX_SPEC) ? "<ffenv>" : NULL);
                }
                break;
            } // fall through to shader case

        case TEX_NORMAL:
        {
            if(renderpath==R_FIXEDFUNCTION) break;
            int i = findtextype(s, t.type==TEX_DIFFUSE ? (1<<TEX_SPEC) : (1<<TEX_DEPTH));
            if(i<0) break;
            texmask |= 1<<s.sts[i].type;
            s.sts[i].combined = index;
            addname(key, s, s.sts[i], true);
            break;
        }
    }
    key.add('\0');
    t.t = textures.access(key.getbuf());
    if(t.t) return;

    ImageData ts;
    TextureAnim anim;
    int compress = 0;
    if(!texturedata(ts, NULL, &t, true, &compress, &anim)) { t.t = notexture; return; }
    switch(t.type)
    {
        case TEX_DIFFUSE:
            if(renderpath==R_FIXEDFUNCTION)
            {
                if(!ts.compressed) loopv(s.sts)
                {
                    Slot::Tex &b = s.sts[i];
                    if(b.combined!=index) continue;
                    ImageData bs;
                    if(!texturedata(bs, NULL, &b)) continue;
                    if(bs.w!=ts.w || bs.h!=ts.h) scaleimage(bs, ts.w, ts.h);
                    switch(b.type)
                    {
                        case TEX_DECAL: if(bs.bpp==4) blenddecal(ts, bs); break;
                        case TEX_NORMAL: addbump(ts, bs, envmap, (texmask&(1<<TEX_SPEC))!=0); break;
                        case TEX_SPEC: mergespec(ts, bs, envmap); break;
                    }
                }
                break;
            } // fall through to shader case

        case TEX_NORMAL:
            if(!ts.compressed) loopv(s.sts)
            {
                Slot::Tex &a = s.sts[i];
                if(a.combined!=index) continue;
                ImageData as;
                if(!texturedata(as, NULL, &a)) continue;
                //if(ts.bpp!=4) forcergbaimage(ts);
                if(as.w!=ts.w || as.h!=ts.h) scaleimage(as, ts.w, ts.h);
                switch(a.type)
                {
                    case TEX_SPEC: mergespec(ts, as); break;
                    case TEX_DEPTH: mergedepth(ts, as); break;
                }
                break; // only one combination
            }
            break;
    }
    t.t = newtexture(NULL, key.getbuf(), ts, 0, true, true, true, compress, &anim);
}

static Slot &loadslot(Slot &s, bool forceload)
{
    linkslotshader(s);
    loopv(s.sts)
    {
        Slot::Tex &t = s.sts[i];
        if(t.combined >= 0) continue;
        switch(t.type)
        {
            case TEX_ENVMAP:
                if(hasCM && (renderpath != R_FIXEDFUNCTION || (s.shader->type&SHADER_ENVMAP && s.ffenv && maxtmus >= 2) || forceload)) t.t = cubemapload(t.name);
                break;

            default:
                texcombine(s, i, t, forceload);
                break;
        }
    }
    s.loaded = true;
    return s;
}

MSlot &lookupmaterialslot(int index, bool load)
{
    MSlot &s = materialslots[index];
    if(load && !s.linked)
    {
        if(!s.loaded) loadslot(s, true);
        linkvslotshader(s);
        s.linked = true;
    }
    return s;
}

Slot &lookupslot(int index, bool load)
{
    Slot &s = slots.inrange(index) ? *slots[index] : (slots.length() ? *slots[0] : dummyslot);
    return s.loaded || !load ? s : loadslot(s, false);
}

VSlot &lookupvslot(int index, bool load)
{
    VSlot &s = vslots.inrange(index) && vslots[index]->slot ? *vslots[index] : (vslots.length() && vslots[0]->slot ? *vslots[0] : dummyvslot);
    if(load && !s.linked)
    {
        if(!s.slot->loaded) loadslot(*s.slot, false);
        linkvslotshader(s);
        s.linked = true;
    }
    return s;
}

void linkslotshaders()
{
    loopv(slots) if(slots[i]->loaded) linkslotshader(*slots[i]);
    loopv(vslots) if(vslots[i]->linked) linkvslotshader(*vslots[i]);
    loopi(MATF_VOLUME+1) if(materialslots[i].loaded)
    {
        linkslotshader(materialslots[i]);
        linkvslotshader(materialslots[i]);
    }
}

Texture *loadthumbnail(Slot &slot)
{
    if(slot.thumbnail) return slot.thumbnail;
    if(!slot.variants)
    {
        slot.thumbnail = notexture;
        return slot.thumbnail;
    }
    VSlot &vslot = *slot.variants;
    linkslotshader(slot, false);
    linkvslotshader(vslot, false);
    vector<char> name;
    if(vslot.colorscale == vec(1, 1, 1)) addname(name, slot, slot.sts[0], false, "<thumbnail>");
    else
    {
        defformatstring(prefix)("<thumbnail:%.2f/%.2f/%.2f>", vslot.colorscale.x, vslot.colorscale.y, vslot.colorscale.z);
        addname(name, slot, slot.sts[0], false, prefix);
    }
    int glow = -1;
    if(slot.texmask&(1<<TEX_GLOW))
    {
        loopvj(slot.sts) if(slot.sts[j].type==TEX_GLOW) { glow = j; break; }
        if(glow >= 0)
        {
            defformatstring(prefix)("<glow:%.2f/%.2f/%.2f>", vslot.glowcolor.x, vslot.glowcolor.y, vslot.glowcolor.z);
            addname(name, slot, slot.sts[glow], true, prefix);
        }
    }
    VSlot *layer = vslot.layer ? &lookupvslot(vslot.layer, false) : NULL;
    if(layer)
    {
        if(layer->colorscale == vec(1, 1, 1)) addname(name, *layer->slot, layer->slot->sts[0], true, "<layer>");
        else
        {
            defformatstring(prefix)("<layer:%.2f/%.2f/%.2f>", vslot.colorscale.x, vslot.colorscale.y, vslot.colorscale.z);
            addname(name, *layer->slot, layer->slot->sts[0], true, prefix);
        }
    }
    name.add('\0');
    Texture *t = textures.access(path(name.getbuf()));
    if(t) slot.thumbnail = t;
    else
    {
        ImageData s, g, l;
        texturedata(s, NULL, &slot.sts[0], false);
        if(vslot.colorscale != vec(1, 1, 1)) texmad(s, vslot.colorscale, vec(0, 0, 0));
        if(glow >= 0) texturedata(g, NULL, &slot.sts[glow], false);
        if(layer)
        {
            texturedata(l, NULL, &layer->slot->sts[0], false);
            if(layer->colorscale != vec(1, 1, 1)) texmad(l, layer->colorscale, vec(0, 0, 0));
        }
        if(!s.data) t = slot.thumbnail = notexture;
        else
        {
            int xs = s.w, ys = s.h;
            if(s.w > 64 || s.h > 64) scaleimage(s, min(s.w, 64), min(s.h, 64));
            if(g.data)
            {
                if(g.w != s.w || g.h != s.h) scaleimage(g, s.w, s.h);
                addglow(s, g, vslot.glowcolor);
            }
            if(l.data)
            {
                if(l.w != s.w/2 || l.h != s.h/2) scaleimage(l, s.w/2, s.h/2);
                forcergbimage(s);
                forcergbimage(l);
                uchar *dstrow = &s.data[s.pitch*l.h + s.bpp*l.w], *srcrow = l.data;
                loop(y, l.h)
                {
                    for(uchar *dst = dstrow, *src = srcrow, *end = &srcrow[l.w*l.bpp]; src < end; dst += s.bpp, src += l.bpp)
                        loopk(3) dst[k] = src[k];
                    dstrow += s.pitch;
                    srcrow += l.pitch;
                }
            }
            t = newtexture(NULL, name.getbuf(), s, 0, false, false, true);
            t->xs = xs;
            t->ys = ys;
            slot.thumbnail = t;
        }
    }
    return t;
}

void loadlayermasks()
{
    loopv(slots)
    {
        Slot &slot = *slots[i];
        if(slot.loaded && slot.layermaskname && !slot.layermask)
        {
            slot.layermask = new ImageData;
            texturedata(*slot.layermask, slot.layermaskname);
            if(!slot.layermask->data) DELETEP(slot.layermask);
        }
    }
}

// environment mapped reflections

void forcecubemapload(GLuint tex)
{
    extern int ati_cubemap_bug;
    if(!ati_cubemap_bug || !tex) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    cubemapshader->set();
    GLenum tex2d = glIsEnabled(GL_TEXTURE_2D), depthtest = glIsEnabled(GL_DEPTH_TEST), blend = glIsEnabled(GL_BLEND);
    if(tex2d) glDisable(GL_TEXTURE_2D);
    if(depthtest) glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_ARB);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, tex);
    if(!blend) glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_POINTS);
    glColor4f(1, 1, 1, 0);
    glTexCoord3f(0, 0, 1);
    glVertex2f(0, 0);
    glEnd();
    if(!blend) glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_CUBE_MAP_ARB);
    if(depthtest) glEnable(GL_DEPTH_TEST);
    if(tex2d) glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

cubemapside cubemapsides[6] =
{
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, "lf", true,  true,  true  },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, "rt", false, false, true  },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, "ft", true,  false, false },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, "bk", false, true,  false },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, "dn", false, false, true  },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, "up", false, false, true  },
};

VARF(IDF_PERSIST, envmapsize, 4, 7, 10, setupmaterials());

Texture *cubemaploadwildcard(Texture *t, const char *name, bool mipit, bool msg, bool transient = false)
{
    if(!hasCM) return NULL;
    string tname;
    if(!name) copystring(tname, t->name);
    else
    {
        copystring(tname, name);
        t = textures.access(path(tname));
        if(t)
        {
            if(!transient && t->type&Texture::TRANSIENT) t->type &= ~Texture::TRANSIENT;
            return t;
        }
    }
    char *wildcard = strchr(tname, '*');
    ImageData surface[6];
    string sname;
    if(!wildcard) copystring(sname, tname);
    GLenum format = GL_FALSE;
    int tsize = 0, compress = 0;
    loopi(6)
    {
        if(wildcard)
        {
            copystring(sname, tname, wildcard-tname+1);
            concatstring(sname, cubemapsides[i].name);
            concatstring(sname, wildcard+1);
        }
        ImageData &s = surface[i];
        texturedata(s, sname, NULL, msg, &compress);
        if(!s.data) return NULL;
        if(s.w != s.h)
        {
            if(msg) conoutf("\frcubemap texture %s does not have square size", sname);
            return NULL;
        }
        if(!format) format = s.compressed ? s.compressed : texformat(s.bpp);
        else if((s.compressed ? s.compressed : texformat(s.bpp))!=format || (s.compressed && (s.w!=surface[0].w || s.h!=surface[0].h || s.levels!=surface[0].levels)))
        {
            if(msg) conoutf("\frcubemap texture %s doesn't match other sides' format", sname);
            return NULL;
        }
        tsize = max(tsize, max(s.w, s.h));
    }
    if(name)
    {
        char *key = newstring(tname);
        t = &textures[key];
        t->name = key;
    }
    t->bpp = surface[0].compressed ? formatsize(uncompressedformat(format)) : surface[0].bpp;
    t->mipmap = mipit;
    t->clamp = 3;
    t->type = Texture::CUBEMAP | (transient ? Texture::TRANSIENT : 0);
    t->xs = t->ys = tsize;
    t->w = t->h = min(1<<envmapsize, tsize);
    resizetexture(t->w, t->h, mipit, false, GL_TEXTURE_CUBE_MAP_ARB, compress, t->w, t->h);
    format = compressedformat(format, t->w, t->h, compress);
    switch(format)
    {
        case GL_RGB: format = GL_RGB5; break;
    }
    if(t->frames.empty()) t->frames.add(0);
    glGenTextures(1, &t->frames[0]);
    int sizelimit = mipit && maxtexsize ? min(maxtexsize, hwcubetexsize) : hwcubetexsize;
    loopi(6)
    {
        ImageData &s = surface[i];
        cubemapside &side = cubemapsides[i];
        if(s.compressed)
        {
            int w = s.w, h = s.h, levels = s.levels, level = 0;
            uchar *data = s.data;
            while(levels > 1 && (w > sizelimit || h > sizelimit))
            {
                data += s.calclevelsize(level++);
                levels--;
                if(w > 1) w /= 2;
                if(h > 1) h /= 2;
            }
            createcompressedtexture(!i ? t->frames[0] : 0, w, h, data, s.align, s.bpp, levels, 3, mipit ? 2 : 1, s.compressed, side.target);
        }
        else
        {
            texreorient(s, side.flipx, side.flipy, side.swapxy);
            createtexture(!i ? t->frames[0] : 0, t->w, t->h, s.data, 3, mipit ? 2 : 1, format, side.target, s.w, s.h, s.pitch, false);
        }
    }
    updatetexture(t);
    forcecubemapload(t->frames[0]);
    return t;
}

Texture *cubemapload(const char *name, bool mipit, bool msg, bool transient)
{
    if(!hasCM) return NULL;
    Texture *t = NULL;
    if(!strchr(name, '*'))
    {
        defformatstring(pname)("%s_*", name);
        t = cubemaploadwildcard(NULL, pname, mipit, false, transient);
        if(!t && msg) conoutf("\frcould not load envmap %s", name);
    }
    else t = cubemaploadwildcard(NULL, name, mipit, msg, transient);
    return t;
}

VAR(IDF_WORLD, envmapradius, 0, 128, 10000);

struct envmap
{
    int radius, size;
    vec o;
    GLuint tex;
};

static vector<envmap> envmaps;
static Texture *skyenvmap = NULL;

void clearenvmaps()
{
    if(skyenvmap)
    {
        if(skyenvmap->type&Texture::TRANSIENT) cleanuptexture(skyenvmap);
        skyenvmap = NULL;
    }
    loopv(envmaps) glDeleteTextures(1, &envmaps[i].tex);
    envmaps.shrink(0);
}

VAR(0, aaenvmap, 0, 2, 4);

GLuint genenvmap(const vec &o, int envmapsize)
{
    int rendersize = 1<<(envmapsize+aaenvmap), sizelimit = min(hwcubetexsize, min(screen->w, screen->h));
    if(maxtexsize) sizelimit = min(sizelimit, maxtexsize);
    while(rendersize > sizelimit) rendersize /= 2;
    int texsize = min(rendersize, 1<<envmapsize);
    if(!aaenvmap) rendersize = texsize;
    GLuint tex;
    glGenTextures(1, &tex);
    glViewport(0, 0, rendersize, rendersize);
    float yaw = 0, pitch = 0;
    uchar *pixels = new uchar[3*rendersize*rendersize];
    glPixelStorei(GL_PACK_ALIGNMENT, texalign(pixels, rendersize, 3));
    loopi(6)
    {
        const cubemapside &side = cubemapsides[i];
        switch(side.target)
        {
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB: // lf
                yaw = 90; pitch = 0; break;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB: // rt
                yaw = 270; pitch = 0; break;
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB: // ft
                yaw = 180; pitch = 0; break;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB: // bk
                yaw = 0; pitch = 0; break;
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB: // dn
                yaw = 270; pitch = -90; break;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB: // up
                yaw = 270; pitch = 90; break;
        }
        glFrontFace((side.flipx==side.flipy)!=side.swapxy ? GL_CW : GL_CCW);
        drawcubemap(rendersize, 0, o, yaw, pitch, !side.flipx, !side.flipy, side.swapxy);
        glReadPixels(0, 0, rendersize, rendersize, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        createtexture(tex, texsize, texsize, pixels, 3, 2, GL_RGB5, side.target, rendersize, rendersize);
    }
    glFrontFace(GL_CW);
    delete[] pixels;
    glViewport(0, 0, screen->w, screen->h);
    forcecubemapload(tex);
    return tex;
}

void initenvmaps()
{
    if(!hasCM) return;
    clearenvmaps();
    extern char *skybox;
    skyenvmap = skybox[0] ? cubemapload(skybox, true, false, true) : NULL;
    const vector<extentity *> &ents = entities::getents();
    loopv(ents)
    {
        const extentity &ent = *ents[i];
        if(ent.type != ET_ENVMAP) continue;
        envmap &em = envmaps.add();
        em.radius = ent.attrs[0] ? max(0, min(10000, int(ent.attrs[0]))) : envmapradius;
        em.size = ent.attrs[1] ? max(4, min(9, int(ent.attrs[1]))) : 0;
        em.o = ent.o;
        em.tex = 0;
    }
}

void genenvmaps()
{
    if(envmaps.empty()) return;
    progress(0, "generating environment maps...");
    int lastprogress = SDL_GetTicks();
    loopv(envmaps)
    {
        envmap &em = envmaps[i];
        em.tex = genenvmap(em.o, em.size ? em.size : envmapsize);
        if(renderedframe) continue;
        int millis = SDL_GetTicks();
        if(millis - lastprogress >= 250)
        {
            progress(float(i+1)/envmaps.length(), "generating environment maps...");
            lastprogress = millis;
        }
    }
}

ushort closestenvmap(const vec &o)
{
    ushort minemid = EMID_SKY;
    float mindist = 1e16f;
    loopv(envmaps)
    {
        envmap &em = envmaps[i];
        float dist = em.o.dist(o);
        if(dist < em.radius && dist < mindist)
        {
            minemid = EMID_RESERVED + i;
            mindist = dist;
        }
    }
    return minemid;
}

ushort closestenvmap(int orient, int x, int y, int z, int size)
{
    vec loc(x, y, z);
    int dim = dimension(orient);
    if(dimcoord(orient)) loc[dim] += size;
    loc[R[dim]] += size/2;
    loc[C[dim]] += size/2;
    return closestenvmap(loc);
}

GLuint lookupenvmap(Slot &slot)
{
    loopv(slot.sts) if(slot.sts[i].type==TEX_ENVMAP && slot.sts[i].t) return slot.sts[i].t->id;
    return skyenvmap ? skyenvmap->id : 0;
}

GLuint lookupenvmap(ushort emid)
{
    if(emid==EMID_SKY || emid==EMID_CUSTOM) return skyenvmap ? skyenvmap->id : 0;
    if(emid==EMID_NONE || !envmaps.inrange(emid-EMID_RESERVED)) return 0;
    GLuint tex = envmaps[emid-EMID_RESERVED].tex;
    return tex ? tex : (skyenvmap ? skyenvmap->id : 0);
}

void cleanuptexture(Texture *t)
{
    DELETEA(t->alphamask);

    loopvk(t->frames)
    {
        if(t->frames[k])
        {
            if(t->frames[k] == t->id) t->id = 0; // using a frame directly
            glDeleteTextures(1, &t->frames[k]);
            t->frames[k] = 0;
        }
    }
    t->frames.shrink(0);
    t->id = 0;

    if(t->type&Texture::TRANSIENT) textures.remove(t->name);
}

void cleanuptextures()
{
    clearenvmaps();
    loopv(slots) slots[i]->cleanup();
    loopv(vslots) vslots[i]->cleanup();
    loopi(MATF_VOLUME+1) materialslots[i].cleanup();
    vector<Texture *> transient;
    enumerate(textures, Texture, tex, cleanuptexture(&tex));
}

bool reloadtexture(const char *name)
{
    string tname;
    copystring(tname, name);
    path(tname);
    Texture *t = textures.access(tname);
    if(t) return reloadtexture(t);
    return true;
}

bool reloadtexture(Texture *t)
{
    loopv(t->frames) if(t->frames[i]) return true;
    switch(t->type&Texture::TYPE)
    {
        case Texture::STUB:
        case Texture::IMAGE:
        {
            ImageData s;
            TextureAnim anim;
            int compress = 0;
            if(!texturedata(s, t->name, NULL, true, &compress, &anim) || !newtexture(t, NULL, s, t->clamp, t->mipmap, false, false, compress, &anim)) return false;
            break;
        }

        case Texture::CUBEMAP:
            if(!cubemaploadwildcard(t, NULL, t->mipmap, true)) return false;
            break;
    }
    return true;
}

void reloadtex(char *name)
{
    Texture *t = textures.access(path(name, true));
    if(!t) { conoutf("\frtexture %s is not loaded", name); return; }
    if(t->type&Texture::TRANSIENT) { conoutf("\frcan't reload transient texture %s", name); return; }
    DELETEA(t->alphamask);
    Texture oldtex = *t;
    t->frames.shrink(0);
    if(!reloadtexture(t))
    {
        loopv(t->frames) if(t->frames[i]) glDeleteTextures(1, &t->frames[i]);
        *t = oldtex;
        conoutf("\frfailed to reload texture %s", name);
    }
}
COMMAND(0, reloadtex, "s");

void reloadtextures()
{
    int reloaded = 0;
    enumerate(textures, Texture, tex,
    {
        loadprogress = float(++reloaded)/textures.numelems;
        reloadtexture(&tex);
    });
    loadprogress = 0;
}

enum
{
    DDSD_CAPS                  = 0x00000001,
    DDSD_HEIGHT                = 0x00000002,
    DDSD_WIDTH                 = 0x00000004,
    DDSD_PITCH                 = 0x00000008,
    DDSD_PIXELFORMAT           = 0x00001000,
    DDSD_MIPMAPCOUNT           = 0x00020000,
    DDSD_LINEARSIZE            = 0x00080000,
    DDSD_BACKBUFFERCOUNT       = 0x00800000,
    DDPF_ALPHAPIXELS           = 0x00000001,
    DDPF_FOURCC                = 0x00000004,
    DDPF_INDEXED               = 0x00000020,
    DDPF_ALPHA                 = 0x00000002,
    DDPF_RGB                   = 0x00000040,
    DDPF_COMPRESSED            = 0x00000080,
    DDPF_LUMINANCE             = 0x00020000,
    DDSCAPS_COMPLEX            = 0x00000008,
    DDSCAPS_TEXTURE            = 0x00001000,
    DDSCAPS_MIPMAP             = 0x00400000,
    DDSCAPS2_CUBEMAP           = 0x00000200,
    DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400,
    DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800,
    DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000,
    DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000,
    DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000,
    DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000,
    DDSCAPS2_VOLUME            = 0x00200000,
    FOURCC_DXT1                = 0x31545844,
    FOURCC_DXT3                = 0x33545844,
    FOURCC_DXT5                = 0x35545844

};

struct DDCOLORKEY { uint dwColorSpaceLowValue, dwColorSpaceHighValue; };
struct DDPIXELFORMAT
{
    uint dwSize, dwFlags, dwFourCC;
    union { uint dwRGBBitCount, dwYUVBitCount, dwZBufferBitDepth, dwAlphaBitDepth, dwLuminanceBitCount, dwBumpBitCount, dwPrivateFormatBitCount; };
    union { uint dwRBitMask, dwYBitMask, dwStencilBitDepth, dwLuminanceBitMask, dwBumpDuBitMask, dwOperations; };
    union { uint dwGBitMask, dwUBitMask, dwZBitMask, dwBumpDvBitMask; struct { ushort wFlipMSTypes, wBltMSTypes; } MultiSampleCaps; };
    union { uint dwBBitMask, dwVBitMask, dwStencilBitMask, dwBumpLuminanceBitMask; };
    union { uint dwRGBAlphaBitMask, dwYUVAlphaBitMask, dwLuminanceAlphaBitMask, dwRGBZBitMask, dwYUVZBitMask; };

};
struct DDSCAPS2 { uint dwCaps, dwCaps2, dwCaps3, dwCaps4; };
struct DDSURFACEDESC2
{
    uint dwSize, dwFlags, dwHeight, dwWidth;
    union { int lPitch; uint dwLinearSize; };
    uint dwBackBufferCount;
    union { uint dwMipMapCount, dwRefreshRate, dwSrcVBHandle; };
    uint dwAlphaBitDepth, dwReserved, lpSurface;
    union { DDCOLORKEY ddckCKDestOverlay; uint dwEmptyFaceColor; };
    DDCOLORKEY ddckCKDestBlt, ddckCKSrcOverlay, ddckCKSrcBlt;
    union { DDPIXELFORMAT ddpfPixelFormat; uint dwFVF; };
    DDSCAPS2 ddsCaps;
    uint dwTextureStage;
};

VAR(0, dbgdds, 0, 0, 1);

bool loaddds(const char *filename, ImageData &image)
{
    stream *f = openfile(filename, "rb");
    if(!f) return false;
    GLenum format = GL_FALSE;
    uchar magic[4];
    if(f->read(magic, 4) != 4 || memcmp(magic, "DDS ", 4)) { delete f; return false; }
    DDSURFACEDESC2 d;
    if(f->read(&d, sizeof(d)) != sizeof(d)) { delete f; return false; }
    lilswap((uint *)&d, sizeof(d)/sizeof(uint));
    if(d.dwSize != sizeof(DDSURFACEDESC2) || d.ddpfPixelFormat.dwSize != sizeof(DDPIXELFORMAT)) { delete f; return false; }
    if(d.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
    {
        switch(d.ddpfPixelFormat.dwFourCC)
        {
            case FOURCC_DXT1: format = d.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS ? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT; break;
            case FOURCC_DXT3: format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
            case FOURCC_DXT5: format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
        }
    }
    if(!format) { delete f; return false; }
    if(dbgdds) conoutf("%s: format 0x%X, %d x %d, %d mipmaps", filename, format, d.dwWidth, d.dwHeight, d.dwMipMapCount);
    int bpp = 0;
    switch(format)
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: bpp = 8; break;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: bpp = 16; break;
    }
    image.setdata(NULL, d.dwWidth, d.dwHeight, bpp, d.dwMipMapCount, 4, format);
    int size = image.calcsize();
    if(f->read(image.data, size) != size) { delete f; image.cleanup(); return false; }
    delete f;
    return true;
}

void gendds(char *infile, char *outfile)
{
    if(!hasTC) { conoutf("\frOpenGL driver does not support texture compression"); return; }

    glHint(GL_TEXTURE_COMPRESSION_HINT_ARB, GL_NICEST);

    defformatstring(cfile)("<compress>%s", infile);
    extern void reloadtex(char *name);
    Texture *t = textures.access(path(cfile));
    if(t) reloadtex(cfile);
    t = textureload(cfile);
    if(t==notexture || t->frames.empty()) { conoutf("\frfailed loading %s", infile); return; }

    glBindTexture(GL_TEXTURE_2D, t->frames[0]);
    GLint compressed = 0, format = 0, width = 0, height = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &compressed);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    if(!compressed) { conoutf("\frfailed compressing %s", infile); return; }
    int fourcc = 0;
    switch(format)
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: fourcc = FOURCC_DXT1; conoutf("compressed as DXT1"); break;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: fourcc = FOURCC_DXT1; conoutf("compressed as DXT1a"); break;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: fourcc = FOURCC_DXT3; conoutf("compressed as DXT3"); break;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: fourcc = FOURCC_DXT5; conoutf("compressed as DXT5"); break;
        default:
            conoutf("\frfailed compressing %s: unknown format: 0x%X", infile, format); break;
            return;
    }

    if(!outfile[0])
    {
        static string buf;
        copystring(buf, infile);
        int len = strlen(buf);
        if(len > 4 && buf[len-4]=='.') memcpy(&buf[len-4], ".dds", 4);
        else concatstring(buf, ".dds");
        outfile = buf;
    }

    stream *f = openfile(outfile, "wb");
    if(!f) { conoutf("\frfailed writing to %s", outfile); return; }

    int csize = 0;
    for(int lw = width, lh = height, level = 0;;)
    {
        GLint size = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, level++, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &size);
        csize += size;
        if(max(lw, lh) <= 1) break;
        if(lw > 1) lw /= 2;
        if(lh > 1) lh /= 2;
    }

    DDSURFACEDESC2 d;
    memset(&d, 0, sizeof(d));
    d.dwSize = sizeof(DDSURFACEDESC2);
    d.dwWidth = width;
    d.dwHeight = height;
    d.dwLinearSize = csize;
    d.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_LINEARSIZE | DDSD_MIPMAPCOUNT;
    d.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    d.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    d.ddpfPixelFormat.dwFlags = DDPF_FOURCC | (format!=GL_COMPRESSED_RGB_S3TC_DXT1_EXT ? DDPF_ALPHAPIXELS : 0);
    d.ddpfPixelFormat.dwFourCC = fourcc;

    uchar *data = new uchar[csize], *dst = data;
    for(int lw = width, lh = height;;)
    {
        GLint size;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, d.dwMipMapCount, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &size);
        glGetCompressedTexImage_(GL_TEXTURE_2D, d.dwMipMapCount++, dst);
        dst += size;
        if(max(lw, lh) <= 1) break;
        if(lw > 1) lw /= 2;
        if(lh > 1) lh /= 2;
    }

    lilswap((uint *)&d, sizeof(d)/sizeof(uint));

    f->write("DDS ", 4);
    f->write(&d, sizeof(d));
    f->write(data, csize);
    delete f;

    delete[] data;

    conoutf("wrote DDS file %s", outfile);

    setuptexcompress();
}
COMMAND(0, gendds, "ss");

void writepngchunk(stream *f, const char *type, uchar *data = NULL, uint len = 0)
{
    f->putbig<uint>(len);
    f->write(type, 4);
    f->write(data, len);

    uint crc = crc32(0, Z_NULL, 0);
    crc = crc32(crc, (const Bytef *)type, 4);
    if(data) crc = crc32(crc, data, len);
    f->putbig<uint>(crc);
}

void savepng(const char *filename, ImageData &image, int compress, bool flip)
{
    uchar ctype = 0;
    switch(image.bpp)
    {
        case 1: ctype = 0; break;
        case 2: ctype = 4; break;
        case 3: ctype = 2; break;
        case 4: ctype = 6; break;
        default: conoutf("\frfailed saving png to %s", filename); return;
    }
    stream *f = openfile(filename, "wb");
    if(!f) { conoutf("\frcould not write to %s", filename); return; }

    uchar signature[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    f->write(signature, sizeof(signature));

    struct pngihdr
    {
        uint width, height;
        uchar bitdepth, colortype, compress, filter, interlace;
    } ihdr = { bigswap<uint>(image.w), bigswap<uint>(image.h), 8, ctype, 0, 0, 0 };
    writepngchunk(f, "IHDR", (uchar *)&ihdr, 13);

    int idat = f->tell();
    uint len = 0;
    f->write("\0\0\0\0IDAT", 8);
    uint crc = crc32(0, Z_NULL, 0);
    crc = crc32(crc, (const Bytef *)"IDAT", 4);

    z_stream z;
    z.zalloc = NULL;
    z.zfree = NULL;
    z.opaque = NULL;

    if(deflateInit(&z, compress) != Z_OK)
        goto error;

    uchar buf[1<<12];
    z.next_out = (Bytef *)buf;
    z.avail_out = sizeof(buf);

    loopi(image.h)
    {
        uchar filter = 0;
        loopj(2)
        {
            z.next_in = j ? (Bytef *)image.data + (flip ? image.h-i-1 : i)*image.pitch : (Bytef *)&filter;
            z.avail_in = j ? image.w*image.bpp : 1;
            while(z.avail_in > 0)
            {
                if(deflate(&z, Z_NO_FLUSH) != Z_OK) goto cleanuperror;
                #define FLUSHZ do { \
                    int flush = sizeof(buf) - z.avail_out; \
                    crc = crc32(crc, buf, flush); \
                    len += flush; \
                    f->write(buf, flush); \
                    z.next_out = (Bytef *)buf; \
                    z.avail_out = sizeof(buf); \
                } while(0)
                FLUSHZ;
            }
        }
    }

    for(;;)
    {
        int err = deflate(&z, Z_FINISH);
        if(err != Z_OK && err != Z_STREAM_END) goto cleanuperror;
        FLUSHZ;
        if(err == Z_STREAM_END) break;
    }

    deflateEnd(&z);

    f->seek(idat, SEEK_SET);
    f->putbig<uint>(len);
    f->seek(0, SEEK_END);
    f->putbig<uint>(crc);

    writepngchunk(f, "IEND");

    delete f;
    return;

cleanuperror:
    deflateEnd(&z);

error:
    delete f;

    conoutf("\frfailed saving png to %s", filename);
}

struct tgaheader
{
    uchar  identsize;
    uchar  cmaptype;
    uchar  imagetype;
    uchar  cmaporigin[2];
    uchar  cmapsize[2];
    uchar  cmapentrysize;
    uchar  xorigin[2];
    uchar  yorigin[2];
    uchar  width[2];
    uchar  height[2];
    uchar  pixelsize;
    uchar  descbyte;
};

void savetga(const char *filename, ImageData &image, int compress, bool flip)
{
    switch(image.bpp)
    {
        case 3: case 4: break;
        default: conoutf("\frfailed saving tga to %s", filename); return;
    }

    stream *f = openfile(filename, "wb");
    if(!f) { conoutf("\frcould not write to %s", filename); return; }

    tgaheader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.pixelsize = image.bpp*8;
    hdr.width[0] = image.w&0xFF;
    hdr.width[1] = (image.w>>8)&0xFF;
    hdr.height[0] = image.h&0xFF;
    hdr.height[1] = (image.h>>8)&0xFF;
    hdr.imagetype = compress ? 10 : 2;
    f->write(&hdr, sizeof(hdr));

    uchar buf[128*4];
    loopi(image.h)
    {
        uchar *src = image.data + (flip ? i : image.h - i - 1)*image.pitch;
        for(int remaining = image.w; remaining > 0;)
        {
            int raw = 1;
            if(compress)
            {
                int run = 1;
                for(uchar *scan = src; run < min(remaining, 128); run++)
                {
                    scan += image.bpp;
                    if(src[0]!=scan[0] || src[1]!=scan[1] || src[2]!=scan[2] || (image.bpp==4 && src[3]!=scan[3])) break;
                }
                if(run > 1)
                {
                    f->putchar(0x80 | (run-1));
                    f->putchar(src[2]); f->putchar(src[1]); f->putchar(src[0]);
                    if(image.bpp==4) f->putchar(src[3]);
                    src += run*image.bpp;
                    remaining -= run;
                    if(remaining <= 0) break;
                }
                for(uchar *scan = src; raw < min(remaining, 128); raw++)
                {
                    scan += image.bpp;
                    if(src[0]==scan[0] && src[1]==scan[1] && src[2]==scan[2] && (image.bpp!=4 || src[3]==scan[3])) break;
                }
                f->putchar(raw - 1);
            }
            else raw = min(remaining, 128);
            uchar *dst = buf;
            loopj(raw)
            {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
                if(image.bpp==4) dst[3] = src[3];
                dst += image.bpp;
                src += image.bpp;
            }
            f->write(buf, raw*image.bpp);
            remaining -= raw;
        }
    }

    delete f;
}

const char *ifmtexts[IFMT_MAX] = { "", ".bmp", ".png", ".tga" };

void saveimage(const char *fname, ImageData &image, int format, int compress, bool flip, bool skip)
{
    int f = format > IFMT_NONE && format < IFMT_MAX ? format : IFMT_PNG;
    const char *filename = makefile(fname, ifmtexts[f], 0, 1, false, skip);
    switch(f)
    {
        case IFMT_PNG: savepng(filename, image, compress, flip); break;
        case IFMT_TGA: savetga(filename, image, compress, flip); break;
        case IFMT_BMP:
        {
            ImageData flipped(image.w, image.h, image.bpp, image.data);
            if(flip) texflip(flipped);
            SDL_Surface *s = wrapsurface(flipped.data, flipped.w, flipped.h, flipped.bpp);
            if(!s) break;
            stream *f = openfile(filename, "wb");
            if(f)
            {
                SDL_SaveBMP_RW(s, f->rwops(), 1);
                delete f;
            }
            SDL_FreeSurface(s);
            break;
        }
        default: break;
    }
}

SDL_Surface *loadsurface(const char *name, bool noload)
{
    const char *exts[] = { "", ".png", ".tga", ".jpg", ".bmp" }; // bmp is a last resort!
    loopi(sizeof(exts)/sizeof(exts[0]))
    {
        defformatstring(buf)("%s%s", name, exts[i]);
        if(noload)
        {
            stream *f = openfile(buf, "rb");
            if(f) { delete f; return (SDL_Surface *)-1; }
            continue;
        }
        SDL_Surface *s = NULL;
        stream *z = openzipfile(buf, "rb");
        if(z)
        {
            SDL_RWops *rw = z->rwops();
            if(rw)
            {
                s = IMG_Load_RW(rw, 0);
                SDL_FreeRW(rw);
            }
            delete z;
        }
        if(!s) s = IMG_Load(findfile(buf, "rb"));
        if(s) return fixsurfaceformat(s);
    }
    return NULL;
}

bool loadimage(const char *name, ImageData &image)
{
    SDL_Surface *s = loadsurface(name);
    if(!s) return false;
    image.wrap(s);
    return true;
}

void flipnormalmapy(char *destfile, char *normalfile) // jpg/png/tga -> tga
{
    ImageData ns;
    if(!loadimage(normalfile, ns)) return;
    ImageData d(ns.w, ns.h, 3);
    readwritetex(d, ns,
        dst[0] = src[0];
        dst[1] = 255 - src[1];
        dst[2] = src[2];
    );
    savetga(destfile, d);
}

void mergenormalmaps(char *heightfile, char *normalfile) // jpg/png/tga + tga -> tga
{
    ImageData hs, ns;
    if(!loadimage(heightfile, hs) || !loadimage(normalfile, ns) || hs.w != ns.w || hs.h != ns.h) return;
    ImageData d(ns.w, ns.h, 3);
    read2writetex(d, hs, srch, ns, srcn,
        *(bvec *)dst = bvec(((bvec *)srcn)->tovec().mul(2).add(((bvec *)srch)->tovec()).normalize());
    );
    savetga(normalfile, d);
}

COMMAND(0, flipnormalmapy, "ss");
COMMAND(0, mergenormalmaps, "ss");

