#include "functions.h"
#include "render.h"

#ifdef NETPLAY
#include <string.h>
#endif

extern char errorString[];
extern char g_cdMessage[];
extern Texture *textureTable[];
extern ConfigData *gameData;

extern int loadingSky;
extern int skyPos[];

extern unsigned char opaqueAlpha;


/* Working storage of LoadPrmCommon: the PC vertices and sprites of the object being converted. */
extern D3DLVERTEX loadVertices[];
extern SpriteQuad loadQuads[];
extern int loadQuadCount;
extern int loadLastPage;


u_char FindNearestColourIndex(const struct tagPALETTEENTRY *colours, short count);

/* Registers of the inlined IntelShort calls differ from the exe; proven equivalent. */
#ifdef EQUIV
short IntelPrim(Prm prm)
{
    IntelShort(&prm.f3->type);
    IntelShort(&prm.f3->flag);
    switch (prm.f3->type) {
    case TypeF3:
        IntelShort(&prm.f3->coords[0]);
        IntelShort(&prm.f3->coords[1]);
        IntelShort(&prm.f3->coords[2]);
        break;
    case TypeF4:
        IntelShort(&prm.f4->coords[0]);
        IntelShort(&prm.f4->coords[1]);
        IntelShort(&prm.f4->coords[2]);
        IntelShort(&prm.f4->coords[3]);
        break;
    case TypeFT3:
        IntelShort(&prm.ft3->coords[0]);
        IntelShort(&prm.ft3->coords[1]);
        IntelShort(&prm.ft3->coords[2]);
        IntelShort(&prm.ft3->texture);
        break;
    case TypeFT4:
        IntelShort(&prm.ft4->coords[0]);
        IntelShort(&prm.ft4->coords[1]);
        IntelShort(&prm.ft4->coords[2]);
        IntelShort(&prm.ft4->coords[3]);
        IntelShort(&prm.ft4->texture);
        break;
    case TypeG3:
        IntelShort(&prm.g3->coords[0]);
        IntelShort(&prm.g3->coords[1]);
        IntelShort(&prm.g3->coords[2]);
        break;
    case TypeG4:
        IntelShort(&prm.g4->coords[0]);
        IntelShort(&prm.g4->coords[1]);
        IntelShort(&prm.g4->coords[2]);
        IntelShort(&prm.g4->coords[3]);
        break;
    case TypeGT3:
        IntelShort(&prm.gt3->coords[0]);
        IntelShort(&prm.gt3->coords[1]);
        IntelShort(&prm.gt3->coords[2]);
        IntelShort(&prm.gt3->texture);
        break;
    case TypeGT4:
        IntelShort(&prm.gt4->coords[0]);
        IntelShort(&prm.gt4->coords[1]);
        IntelShort(&prm.gt4->coords[2]);
        IntelShort(&prm.gt4->coords[3]);
        IntelShort(&prm.gt4->texture);
        break;
    case TypeLSF3:
        IntelShort(&prm.lsf3->coords[0]);
        IntelShort(&prm.lsf3->coords[1]);
        IntelShort(&prm.lsf3->coords[2]);
        IntelShort(&prm.lsf3->normal);
        break;
    case TypeLSF4:
        IntelShort(&prm.lsf4->coords[0]);
        IntelShort(&prm.lsf4->coords[1]);
        IntelShort(&prm.lsf4->coords[2]);
        IntelShort(&prm.lsf4->coords[3]);
        IntelShort(&prm.lsf4->normal);
        break;
    case TypeLSFT3:
        IntelShort(&prm.lsft3->coords[0]);
        IntelShort(&prm.lsft3->coords[1]);
        IntelShort(&prm.lsft3->coords[2]);
        IntelShort(&prm.lsft3->normal);
        IntelShort(&prm.lsft3->texture);
        break;
    case TypeLSFT4:
        IntelShort(&prm.lsft4->coords[0]);
        IntelShort(&prm.lsft4->coords[1]);
        IntelShort(&prm.lsft4->coords[2]);
        IntelShort(&prm.lsft4->coords[3]);
        IntelShort(&prm.lsft4->normal);
        IntelShort(&prm.lsft4->texture);
        break;
    case TypeLSG3:
        IntelShort(&prm.lsg3->coords[0]);
        IntelShort(&prm.lsg3->coords[1]);
        IntelShort(&prm.lsg3->coords[2]);
        IntelShort(&prm.lsg3->normals[0]);
        IntelShort(&prm.lsg3->normals[1]);
        IntelShort(&prm.lsg3->normals[2]);
        break;
    case TypeLSG4:
        IntelShort(&prm.lsg4->coords[0]);
        IntelShort(&prm.lsg4->coords[1]);
        IntelShort(&prm.lsg4->coords[2]);
        IntelShort(&prm.lsg4->coords[3]);
        IntelShort(&prm.lsg4->normals[0]);
        IntelShort(&prm.lsg4->normals[1]);
        IntelShort(&prm.lsg4->normals[2]);
        IntelShort(&prm.lsg4->normals[3]);
        break;
    case TypeLSGT3:
        IntelShort(&prm.lsgt3->coords[0]);
        IntelShort(&prm.lsgt3->coords[1]);
        IntelShort(&prm.lsgt3->coords[2]);
        IntelShort(&prm.lsgt3->normals[0]);
        IntelShort(&prm.lsgt3->normals[1]);
        IntelShort(&prm.lsgt3->normals[2]);
        IntelShort(&prm.lsgt3->texture);
        break;
    case TypeLSGT4:
        IntelShort(&prm.lsgt4->coords[0]);
        IntelShort(&prm.lsgt4->coords[1]);
        IntelShort(&prm.lsgt4->coords[2]);
        IntelShort(&prm.lsgt4->coords[3]);
        IntelShort(&prm.lsgt4->normals[0]);
        IntelShort(&prm.lsgt4->normals[1]);
        IntelShort(&prm.lsgt4->normals[2]);
        IntelShort(&prm.lsgt4->normals[3]);
        IntelShort(&prm.lsgt4->texture);
        break;
    case TypeTSPR:
    case TypeBSPR:
        IntelShort(&prm.spr->coord);
        IntelShort(&prm.spr->width);
        IntelShort(&prm.spr->height);
        IntelShort(&prm.spr->texture);
        break;
    case TypeSpline:
        IntelLong(&prm.spline->control1.vx);
        IntelLong(&prm.spline->control1.vy);
        IntelLong(&prm.spline->control1.vz);
        IntelLong(&prm.spline->position.vx);
        IntelLong(&prm.spline->position.vy);
        IntelLong(&prm.spline->position.vz);
        IntelLong(&prm.spline->control2.vx);
        IntelLong(&prm.spline->control2.vy);
        IntelLong(&prm.spline->control2.vz);
        break;
    case TypePointLight:
        IntelLong(&prm.pointLight->position.vx);
        IntelLong(&prm.pointLight->position.vy);
        IntelLong(&prm.pointLight->position.vz);
        IntelShort(&prm.pointLight->startFalloff);
        IntelShort(&prm.pointLight->endFalloff);
        break;
    case TypeSpotLight:
        IntelLong(&prm.spotLight->position.vx);
        IntelLong(&prm.spotLight->position.vy);
        IntelLong(&prm.spotLight->position.vz);
        IntelShort(&prm.spotLight->direction.vx);
        IntelShort(&prm.spotLight->direction.vy);
        IntelShort(&prm.spotLight->direction.vz);
        IntelShort(&prm.spotLight->startFalloff);
        IntelShort(&prm.spotLight->endFalloff);
        IntelShort(&prm.spotLight->coneAngle);
        IntelShort(&prm.spotLight->spreadAngle);
        break;
    case TypeInfiniteLight:
        IntelShort(&prm.infiniteLight->direction.vx);
        IntelShort(&prm.infiniteLight->direction.vy);
        IntelShort(&prm.infiniteLight->direction.vz);
        break;
    default:
        wsprintf(errorString, "Scene:IntelPrim: Bad primitive type $%x \n", prm.f3->type);
        return 1;
    }
    return 0;
}
#endif


void IntelLong(long *lvalue)
{
    char *cvalue;
    char swapByte;

    cvalue = (char *)lvalue;
    swapByte = cvalue[0];
    cvalue[0] = cvalue[3];
    cvalue[3] = swapByte;
    swapByte = cvalue[1];
    cvalue[1] = cvalue[2];
    cvalue[2] = swapByte;
}

void IntelShort(short *svalue)
{
    char *cvalue;
    char swapByte;

    cvalue = (char *)svalue;
    swapByte = cvalue[0];
    cvalue[0] = cvalue[1];
    cvalue[1] = swapByte;
}

/* The D3D body is equivalent despite register and spill differences. */
#ifdef EQUIV
#define TPAGE(x, y) ((((y) & 0x100) >> 4) + ((x) >> 6))

#ifdef NETPLAY
static __inline void SetLoadedPosition(int vertex, SVECTOR position)
{
    if (loadingSky) {
        loadVertices[vertex].x = position.vx / 350.0f;
        loadVertices[vertex].y = position.vy / 350.0f + skyPos[gameData->trackNum] / 512.0f;
        loadVertices[vertex].z = position.vz / 350.0f;
    } else {
        loadVertices[vertex].x = position.vx / 4096.0f;
        loadVertices[vertex].y = position.vy / 4096.0f;
        loadVertices[vertex].z = position.vz / 4096.0f;
    }
}

static __inline void SetLoadedColour(int vertex, CVECTOR colour)
{
    ((u_char *)&loadVertices[vertex].color)[3] = opaqueAlpha;
    ((u_char *)&loadVertices[vertex].color)[2] = colour.r;
    ((u_char *)&loadVertices[vertex].color)[1] = colour.g;
    ((u_char *)&loadVertices[vertex].color)[0] = colour.b;
}

static __inline void SetLoadedUV(int vertex, u_char u, u_char v)
{
    loadVertices[vertex].tu = (float)(u_long)u / 255.0f;
    loadVertices[vertex].tv = (float)(u_long)v / 255.0f;
}

static __inline int LoadedTexturePage(Texture *texture)
{
    int page = TPAGE(texture->textureX, texture->textureY) & 0x1f;
    if (page > 15 && page < 100)
        page -= 10;
    else if (page < 16)
        page -= 5;
    return page;
}

#define SET_POSITION(p, n) SetLoadedPosition(nextVertexIndex, currentObject->vertices[(p)->coords[n]])
#define SET_COLOUR(c) SetLoadedColour(nextVertexIndex, *(c))
#define SET_UV(u, v) SetLoadedUV(nextVertexIndex, (u_char)(u), (u_char)(v))
#define SET_PALETTE_UV() SetLoadedUV(nextVertexIndex, uPalette, paletteRow)
#else
#define SET_POSITION(p, n) \
    if (loadingSky) { \
        loadVertices[vertex].x = object->vertices[(p)->coords[n]].vx * (1.0f / 350.0f); \
        loadVertices[vertex].y = object->vertices[(p)->coords[n]].vy * (1.0f / 350.0f) + skyPos[gameData->trackNum] * (1.0f / 512.0f); \
        loadVertices[vertex].z = object->vertices[(p)->coords[n]].vz * (1.0f / 350.0f); \
    } else { \
        loadVertices[vertex].x = object->vertices[(p)->coords[n]].vx * (1.0f / 4096.0f); \
        loadVertices[vertex].y = object->vertices[(p)->coords[n]].vy * (1.0f / 4096.0f); \
        loadVertices[vertex].z = object->vertices[(p)->coords[n]].vz * (1.0f / 4096.0f); \
    }

#define SET_COLOUR(c) \
    ((u_char *)&loadVertices[vertex].color)[3] = opaqueAlpha; \
    ((u_char *)&loadVertices[vertex].color)[2] = (c)->r; \
    ((u_char *)&loadVertices[vertex].color)[1] = (c)->g; \
    ((u_char *)&loadVertices[vertex].color)[0] = (c)->b

#define SET_UV(u, w) \
    loadVertices[vertex].tu = (float)(u_long)(u_char)(u) * (1.0f / 255.0f); \
    loadVertices[vertex].tv = (float)(u_long)(u_char)(w) * (1.0f / 255.0f)

#define SET_PALETTE_UV() \
    loadVertices[vertex].tu = (float)(u_long)cu * (1.0f / 255.0f); \
    loadVertices[vertex].tv = tv
#endif

#ifdef NETPLAY
#pragma intrinsic(memcpy)
/* Page of the texture set a texture lives in, clamped to the sets that exist. */
#define TEXTURE_SET(texture) \
    textureSet = LoadedTexturePage(texture); \
    if (textureSet > normalSetNo) \
        textureSet = normalSetNo

#define ADD_TRIANGLE(set, a, b, c, f) \
    quadBuckets[set].triangles[quadBuckets[set].count].v1 = (a); \
    quadBuckets[set].triangles[quadBuckets[set].count].v2 = (b); \
    quadBuckets[set].triangles[quadBuckets[set].count].v3 = (c); \
    quadBuckets[set].triangles[quadBuckets[set].count].flags = (f); \
    quadBuckets[set].count++

#define ADD_TRIANGLE_PLAIN(set, a, b, c) \
    quadBuckets[set].triangles[quadBuckets[set].count].v1 = (a); \
    quadBuckets[set].triangles[quadBuckets[set].count].v2 = (b); \
    quadBuckets[set].triangles[quadBuckets[set].count].v3 = (c); \
    quadBuckets[set].count++

/* A quad is two triangles. */
#define ADD_QUAD(set) \
    ADD_TRIANGLE(set, nextVertexIndex, nextVertexIndex + 1, nextVertexIndex + 3, 1); \
    ADD_TRIANGLE(set, nextVertexIndex, nextVertexIndex + 3, nextVertexIndex + 2, 0x1f)
#else
/* Page of the texture set a texture lives in, clamped to the sets that exist. */
#define TEXTURE_SET(texture) \
    set = (TPAGE((texture)->textureX, (texture)->textureY)) & 0x1f; \
    if (set > 15 && set < 100) \
        set -= 10; \
    else if (set < 16) \
        set -= 5; \
    if (set > normalSetNo) \
        set = normalSetNo

#define ADD_TRIANGLE(a, b, c, f) \
    quadBuckets[set].triangles[quadBuckets[set].count].v1 = (a); \
    quadBuckets[set].triangles[quadBuckets[set].count].v2 = (b); \
    quadBuckets[set].triangles[quadBuckets[set].count].v3 = (c); \
    quadBuckets[set].triangles[quadBuckets[set].count].flags = (f); \
    quadBuckets[set].count++

#define ADD_TRIANGLE_PLAIN(a, b, c) \
    quadBuckets[set].triangles[quadBuckets[set].count].v1 = (a); \
    quadBuckets[set].triangles[quadBuckets[set].count].v2 = (b); \
    quadBuckets[set].triangles[quadBuckets[set].count].v3 = (c); \
    quadBuckets[set].count++

/* A quad is two triangles. */
#define ADD_QUAD() \
    ADD_TRIANGLE(vertex, vertex + 1, vertex + 3, 1); \
    ADD_TRIANGLE(vertex, vertex + 3, vertex + 2, 0x1f)
#endif

#ifndef NETPLAY
Object *LoadPrmCommon(WADFILE *wad, char *name, short textureStart)
{
    Prm prm;
    short j;
    char *start;
    u_char uOffset;
    Object *firstObject;
    char *finish;
    int vertex;
    Texture *texture;
    float tv;
    Object *lastObject;
    Object *object;
    short i;
    u_char cv;
    int set;
    u_char vOffset;
    u_char cu;
    long length;
/* Rebuild the object from the start after a malformed primitive. */
again:
    if (wad == 0) {
        length = FileLength(name);
        if (length <= 0) {
            FileAccessError(name);
            wsprintf(errorString, "scene.c:LoadPrm: File %s not found\n", name);
            Error(errorString, Fatal);
        }
        start = DAlloc(heap, length + 32);
        finish = start + length;
        if (!start) {
            wsprintf(errorString, "scene.c:LoadPrm(): Failed to allocate memory for file %s\n", name);
            Error(errorString, Fatal);
        }
        if (LoadFile(name, start) != length) {
            wsprintf(errorString, "scene.c:LoadPrm(): Failed to load file %s\n", name);
            Error(errorString, Fatal);
        }
    } else {
        length = wad->sizes[wad->current];
        start = DAlloc(heap, length + 32);
        finish = start + length;
        readnextfile(wad, start);
    }
    prm.ptr = start;
    firstObject = prm.object;
    lastObject = 0;
    while (finish > prm.ptr) {
        object = prm.object++;
        IntelShort(&object->vertexCount);
        IntelShort(&object->primitiveCount);
        IntelLong(&object->extent);
        object->normalCount = 12345;
        object->normals = DAlloc(heap, sizeof(TriList) * 23);
        if (!object->normals)
            OutputDebugString("Oh bugger\n");
        object->skeleton = prm.skeleton++;
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 3; j++)
                IntelShort(&object->skeleton->relative.m[i][j]);
            IntelLong(&object->skeleton->relative.t[i]);
            IntelLong(&object->skeleton->absolute.t[i]);
        }
        IntelShort(&object->skeleton->update);
        object->next = 0;
        object->vertices = prm.svector;
        prm.svector += object->vertexCount;
        for (j = 0; j < object->vertexCount; j++) {
            IntelShort(&object->vertices[j].vx);
            IntelShort(&object->vertices[j].vy);
            IntelShort(&object->vertices[j].vz);
        }
        object->primitives = prm.primitive;
        loadLastPage = -1;
        vertex = 0;
        loadQuadCount = 0;
        for (j = 0; j <= normalSetNo; j++)
            quadBuckets[j].count = 0;
        for (i = 0; i < object->primitiveCount; i++) {
            if (IntelPrim(prm) == 1) {
                DFree(heap, start);
                OutputDebugString("error with prm file\n");
                goto again;
            }
            switch (prm.f3->type) {
            case TypeF3:
                if (rgbTextures)
                    set = normalSetNo;
                else
                    set = 11;
                ADD_TRIANGLE_PLAIN(vertex, vertex + 1, vertex + 2);
                if (rgbTextures) {
                    cu = 0;
                    cv = 0;
                } else {
                    cu = FindNearestColourIndex(&prm.f3->colour, 1);
                    cv = 110;
                }
                SET_POSITION(prm.f3, 0);
                SET_COLOUR(&prm.f3->colour);
                loadVertices[vertex].tu = (float)(u_long)cu * (1.0f / 255.0f);
                tv = (float)(u_long)cv * (1.0f / 255.0f);
                loadVertices[vertex].tv = tv;
                vertex++;
                SET_POSITION(prm.f3, 1);
                SET_COLOUR(&prm.f3->colour);
                SET_PALETTE_UV();
                vertex++;
                SET_POSITION(prm.f3, 2);
                SET_COLOUR(&prm.f3->colour);
                SET_PALETTE_UV();
                vertex++;
                prm.f3++;
                break;
            case TypeFT3:
                prm.ft3->texture += textureStart;
                texture = textureTable[prm.ft3->texture];
                TEXTURE_SET(texture);
                ADD_TRIANGLE_PLAIN(vertex, vertex + 1, vertex + 2);
                uOffset = texture->u0;
                prm.ft3->cba = texture->cba;
                vOffset = texture->v0;
                prm.ft3->tsb = texture->tsb;
                prm.ft3->u0 += uOffset;
                prm.ft3->v0 += vOffset;
                prm.ft3->u1 += uOffset;
                prm.ft3->v1 += vOffset;
                prm.ft3->u2 += uOffset;
                prm.ft3->v2 += vOffset;
                SET_POSITION(prm.ft3, 0);
                SET_COLOUR(&prm.ft3->colour);
                SET_UV(prm.ft3->u0, prm.ft3->v0);
                vertex++;
                SET_POSITION(prm.ft3, 1);
                SET_COLOUR(&prm.ft3->colour);
                SET_UV(prm.ft3->u1, prm.ft3->v1);
                vertex++;
                SET_POSITION(prm.ft3, 2);
                SET_COLOUR(&prm.ft3->colour);
                SET_UV(prm.ft3->u2, prm.ft3->v2);
                vertex++;
                prm.ft3++;
                break;
            case TypeF4:
                if (rgbTextures)
                    set = normalSetNo;
                else
                    set = 11;
                ADD_QUAD();
                if (rgbTextures) {
                    cu = 0;
                    cv = 0;
                } else {
                    cu = FindNearestColourIndex(&prm.f4->colour, 1);
                    cv = 110;
                }
                SET_POSITION(prm.f4, 0);
                SET_COLOUR(&prm.f4->colour);
                loadVertices[vertex].tu = (float)(u_long)cu * (1.0f / 255.0f);
                tv = (float)(u_long)cv * (1.0f / 255.0f);
                loadVertices[vertex].tv = tv;
                vertex++;
                SET_POSITION(prm.f4, 1);
                SET_COLOUR(&prm.f4->colour);
                SET_PALETTE_UV();
                vertex++;
                SET_POSITION(prm.f4, 2);
                SET_COLOUR(&prm.f4->colour);
                SET_PALETTE_UV();
                vertex++;
                SET_POSITION(prm.f4, 3);
                SET_COLOUR(&prm.f4->colour);
                SET_PALETTE_UV();
                vertex++;
                prm.f4++;
                break;
            case TypeFT4:
                prm.ft4->texture += textureStart;
                texture = textureTable[prm.ft4->texture];
                TEXTURE_SET(texture);
                ADD_QUAD();
                uOffset = texture->u0;
                vOffset = texture->v0;
                prm.ft4->cba = texture->cba;
                prm.ft4->tsb = texture->tsb;
                prm.ft4->u0 += uOffset;
                prm.ft4->v0 += vOffset;
                prm.ft4->u1 += uOffset;
                prm.ft4->v1 += vOffset;
                prm.ft4->u2 += uOffset;
                prm.ft4->v2 += vOffset;
                prm.ft4->u3 += uOffset;
                prm.ft4->v3 += vOffset;
                SET_POSITION(prm.ft4, 0);
                SET_COLOUR(&prm.ft4->colour);
                SET_UV(prm.ft4->u0, prm.ft4->v0);
                vertex++;
                SET_POSITION(prm.ft4, 1);
                SET_COLOUR(&prm.ft4->colour);
                SET_UV(prm.ft4->u1, prm.ft4->v1);
                vertex++;
                SET_POSITION(prm.ft4, 2);
                SET_COLOUR(&prm.ft4->colour);
                SET_UV(prm.ft4->u2, prm.ft4->v2);
                vertex++;
                SET_POSITION(prm.ft4, 3);
                SET_COLOUR(&prm.ft4->colour);
                SET_UV(prm.ft4->u3, prm.ft4->v3);
                vertex++;
                prm.ft4++;
                break;
            case TypeG3:
                if (rgbTextures)
                    set = normalSetNo;
                else
                    set = 11;
                ADD_TRIANGLE_PLAIN(vertex, vertex + 1, vertex + 2);
                if (rgbTextures) {
                    cu = 0;
                    cv = 0;
                } else {
                    cu = FindNearestColourIndex(&prm.g3->colour[0], 3);
                    cv = 110;
                }
                SET_POSITION(prm.g3, 0);
                SET_COLOUR(&prm.g3->colour[0]);
                loadVertices[vertex].tu = (float)(u_long)cu * (1.0f / 255.0f);
                tv = (float)(u_long)cv * (1.0f / 255.0f);
                loadVertices[vertex].tv = tv;
                vertex++;
                SET_POSITION(prm.g3, 1);
                SET_COLOUR(&prm.g3->colour[1]);
                SET_PALETTE_UV();
                vertex++;
                SET_POSITION(prm.g3, 2);
                SET_COLOUR(&prm.g3->colour[2]);
                SET_PALETTE_UV();
                vertex++;
                prm.g3++;
                break;
            case TypeGT3:
                prm.gt3->texture += textureStart;
                texture = textureTable[prm.gt3->texture];
                TEXTURE_SET(texture);
                ADD_TRIANGLE_PLAIN(vertex, vertex + 1, vertex + 2);
                uOffset = texture->u0;
                vOffset = texture->v0;
                prm.gt3->cba = texture->cba;
                prm.gt3->tsb = texture->tsb;
                prm.gt3->u0 += uOffset;
                prm.gt3->v0 += vOffset;
                prm.gt3->u1 += uOffset;
                prm.gt3->v1 += vOffset;
                prm.gt3->u2 += uOffset;
                prm.gt3->v2 += vOffset;
                SET_POSITION(prm.gt3, 0);
                SET_COLOUR(&prm.gt3->colour[0]);
                SET_UV(prm.gt3->u0, prm.gt3->v0);
                vertex++;
                SET_POSITION(prm.gt3, 1);
                SET_COLOUR(&prm.gt3->colour[1]);
                SET_UV(prm.gt3->u1, prm.gt3->v1);
                vertex++;
                SET_POSITION(prm.gt3, 2);
                SET_COLOUR(&prm.gt3->colour[2]);
                SET_UV(prm.gt3->u2, prm.gt3->v2);
                vertex++;
                prm.gt3++;
                break;
            case TypeG4:
                if (rgbTextures)
                    set = normalSetNo;
                else
                    set = 11;
                ADD_QUAD();
                if (rgbTextures) {
                    cu = 0;
                    cv = 0;
                } else {
                    cu = FindNearestColourIndex(&prm.g4->colour[0], 4);
                    cv = 110;
                }
                SET_POSITION(prm.g4, 0);
                SET_COLOUR(&prm.g4->colour[0]);
                loadVertices[vertex].tu = (float)(u_long)cu * (1.0f / 255.0f);
                tv = (float)(u_long)cv * (1.0f / 255.0f);
                loadVertices[vertex].tv = tv;
                vertex++;
                SET_POSITION(prm.g4, 1);
                SET_COLOUR(&prm.g4->colour[1]);
                SET_PALETTE_UV();
                vertex++;
                SET_POSITION(prm.g4, 2);
                SET_COLOUR(&prm.g4->colour[2]);
                SET_PALETTE_UV();
                vertex++;
                SET_POSITION(prm.g4, 3);
                SET_COLOUR(&prm.g4->colour[3]);
                SET_PALETTE_UV();
                vertex++;
                prm.g4++;
                break;
            case TypeGT4:
                prm.gt4->texture += textureStart;
                texture = textureTable[prm.gt4->texture];
                TEXTURE_SET(texture);
                ADD_QUAD();
                uOffset = texture->u0;
                vOffset = texture->v0;
                prm.gt4->cba = texture->cba;
                prm.gt4->tsb = texture->tsb;
                prm.gt4->u0 += uOffset;
                prm.gt4->v0 += vOffset;
                prm.gt4->u1 += uOffset;
                prm.gt4->v1 += vOffset;
                prm.gt4->u2 += uOffset;
                prm.gt4->v2 += vOffset;
                prm.gt4->u3 += uOffset;
                prm.gt4->v3 += vOffset;
                SET_POSITION(prm.gt4, 0);
                SET_COLOUR(&prm.gt4->colour[0]);
                SET_UV(prm.gt4->u0, prm.gt4->v0);
                vertex++;
                SET_POSITION(prm.gt4, 1);
                SET_COLOUR(&prm.gt4->colour[1]);
                SET_UV(prm.gt4->u1, prm.gt4->v1);
                vertex++;
                SET_POSITION(prm.gt4, 2);
                SET_COLOUR(&prm.gt4->colour[2]);
                SET_UV(prm.gt4->u2, prm.gt4->v2);
                vertex++;
                SET_POSITION(prm.gt4, 3);
                SET_COLOUR(&prm.gt4->colour[3]);
                SET_UV(prm.gt4->u3, prm.gt4->v3);
                vertex++;
                prm.gt4++;
                break;
            case TypeTSPR:
            case TypeBSPR:
                prm.spr->texture += textureStart;
                texture = textureTable[prm.spr->texture];
                TEXTURE_SET(texture);
                loadQuads[loadQuadCount].position = object->vertices[prm.spr->coord];
                loadQuads[loadQuadCount].set = set;
                loadQuads[loadQuadCount].colour = prm.spr->colour;
                loadQuads[loadQuadCount].width = prm.spr->width * 2;
                loadQuads[loadQuadCount].height = prm.spr->height;
                loadQuads[loadQuadCount].textureW = texture->textureW;
                loadQuads[loadQuadCount].textureH = texture->textureH;
                loadQuads[loadQuadCount].u0 = texture->u0;
                loadQuads[loadQuadCount].v0 = texture->v0;
                loadQuads[loadQuadCount].u1 = texture->u1;
                loadQuads[loadQuadCount].v1 = texture->v1;
                loadQuads[loadQuadCount].u2 = texture->u2;
                loadQuads[loadQuadCount].v2 = texture->v2;
                loadQuads[loadQuadCount].u3 = texture->u3;
                loadQuads[loadQuadCount].v3 = texture->v3;
                loadQuadCount++;
                prm.spr++;
                break;
            case TypeLSF3:
                set = normalSetNo;
                ADD_TRIANGLE_PLAIN(vertex, vertex + 1, vertex + 2);
                SET_POSITION(prm.lsf3, 0);
                SET_COLOUR(&prm.lsf3->colour);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                SET_POSITION(prm.lsf3, 1);
                SET_COLOUR(&prm.lsf3->colour);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                SET_POSITION(prm.lsf3, 2);
                SET_COLOUR(&prm.lsf3->colour);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                prm.lsf3++;
                break;
            case TypeLSFT3:
                prm.lsft3->texture += textureStart;
                texture = textureTable[prm.lsft3->texture];
                TEXTURE_SET(texture);
                ADD_TRIANGLE_PLAIN(vertex, vertex + 1, vertex + 2);
                uOffset = texture->u0;
                vOffset = texture->v0;
                prm.lsft3->cba = texture->cba;
                prm.lsft3->tsb = texture->tsb;
                prm.lsft3->u0 += uOffset;
                prm.lsft3->v0 += vOffset;
                prm.lsft3->u1 += uOffset;
                prm.lsft3->v1 += vOffset;
                prm.lsft3->u2 += uOffset;
                prm.lsft3->v2 += vOffset;
                SET_POSITION(prm.lsft3, 0);
                SET_COLOUR(&prm.lsft3->colour);
                SET_UV(prm.lsft3->u0, prm.lsft3->v0);
                vertex++;
                SET_POSITION(prm.lsft3, 1);
                SET_COLOUR(&prm.lsft3->colour);
                SET_UV(prm.lsft3->u1, prm.lsft3->v1);
                vertex++;
                SET_POSITION(prm.lsft3, 2);
                SET_COLOUR(&prm.lsft3->colour);
                SET_UV(prm.lsft3->u2, prm.lsft3->v2);
                vertex++;
                prm.lsft3++;
                break;
            case TypeLSF4:
                set = normalSetNo;
                ADD_QUAD();
                SET_POSITION(prm.lsf4, 0);
                SET_COLOUR(&prm.lsf4->colour);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                SET_POSITION(prm.lsf4, 1);
                SET_COLOUR(&prm.lsf4->colour);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                SET_POSITION(prm.lsf4, 2);
                SET_COLOUR(&prm.lsf4->colour);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                SET_POSITION(prm.lsf4, 3);
                SET_COLOUR(&prm.lsf4->colour);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                prm.lsf4++;
                break;
            case TypeLSFT4:
                prm.lsft4->texture += textureStart;
                texture = textureTable[prm.lsft4->texture];
                TEXTURE_SET(texture);
                ADD_QUAD();
                uOffset = texture->u0;
                vOffset = texture->v0;
                prm.lsft4->cba = texture->cba;
                prm.lsft4->tsb = texture->tsb;
                prm.lsft4->u0 += uOffset;
                prm.lsft4->v0 += vOffset;
                prm.lsft4->u1 += uOffset;
                prm.lsft4->v1 += vOffset;
                prm.lsft4->u2 += uOffset;
                prm.lsft4->v2 += vOffset;
                prm.lsft4->u3 += uOffset;
                prm.lsft4->v3 += vOffset;
                SET_POSITION(prm.lsft4, 0);
                SET_COLOUR(&prm.lsft4->colour);
                SET_UV(prm.lsft4->u0, prm.lsft4->v0);
                vertex++;
                SET_POSITION(prm.lsft4, 1);
                SET_COLOUR(&prm.lsft4->colour);
                SET_UV(prm.lsft4->u1, prm.lsft4->v1);
                vertex++;
                SET_POSITION(prm.lsft4, 2);
                SET_COLOUR(&prm.lsft4->colour);
                SET_UV(prm.lsft4->u2, prm.lsft4->v2);
                vertex++;
                SET_POSITION(prm.lsft4, 3);
                SET_COLOUR(&prm.lsft4->colour);
                SET_UV(prm.lsft4->u3, prm.lsft4->v3);
                vertex++;
                prm.lsft4++;
                break;
            case TypeLSG3:
                set = normalSetNo;
                ADD_TRIANGLE_PLAIN(vertex, vertex + 1, vertex + 2);
                SET_POSITION(prm.lsg3, 0);
                SET_COLOUR(&prm.lsg3->colour[0]);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                SET_POSITION(prm.lsg3, 1);
                SET_COLOUR(&prm.lsg3->colour[1]);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                SET_POSITION(prm.lsg3, 2);
                SET_COLOUR(&prm.lsg3->colour[2]);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                prm.lsg3++;
                break;
            case TypeLSGT3:
                prm.lsgt3->texture += textureStart;
                texture = textureTable[prm.lsgt3->texture];
                TEXTURE_SET(texture);
                ADD_TRIANGLE_PLAIN(vertex, vertex + 1, vertex + 2);
                uOffset = texture->u0;
                vOffset = texture->v0;
                prm.lsgt3->cba = texture->cba;
                prm.lsgt3->tsb = texture->tsb;
                prm.lsgt3->u0 += uOffset;
                prm.lsgt3->v0 += vOffset;
                prm.lsgt3->u1 += uOffset;
                prm.lsgt3->v1 += vOffset;
                prm.lsgt3->u2 += uOffset;
                prm.lsgt3->v2 += vOffset;
                SET_POSITION(prm.lsgt3, 0);
                SET_COLOUR(&prm.lsgt3->colour[0]);
                SET_UV(prm.lsgt3->u0, prm.lsgt3->v0);
                vertex++;
                SET_POSITION(prm.lsgt3, 1);
                SET_COLOUR(&prm.lsgt3->colour[1]);
                SET_UV(prm.lsgt3->u1, prm.lsgt3->v1);
                vertex++;
                SET_POSITION(prm.lsgt3, 2);
                SET_COLOUR(&prm.lsgt3->colour[2]);
                SET_UV(prm.lsgt3->u2, prm.lsgt3->v2);
                vertex++;
                prm.lsgt3++;
                break;
            case TypeLSG4:
                set = normalSetNo;
                ADD_QUAD();
                SET_POSITION(prm.lsg4, 0);
                SET_COLOUR(&prm.lsg4->colour[0]);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                SET_POSITION(prm.lsg4, 1);
                SET_COLOUR(&prm.lsg4->colour[1]);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                SET_POSITION(prm.lsg4, 2);
                SET_COLOUR(&prm.lsg4->colour[2]);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                SET_POSITION(prm.lsg4, 3);
                SET_COLOUR(&prm.lsg4->colour[3]);
                loadVertices[vertex].tu = 0;
                loadVertices[vertex].tv = 0;
                vertex++;
                prm.lsg4++;
                break;
            case TypeLSGT4:
                prm.lsgt4->texture += textureStart;
                texture = textureTable[prm.lsgt4->texture];
                TEXTURE_SET(texture);
                ADD_QUAD();
                uOffset = texture->u0;
                vOffset = texture->v0;
                prm.lsgt4->cba = texture->cba;
                prm.lsgt4->tsb = texture->tsb;
                prm.lsgt4->u0 += uOffset;
                prm.lsgt4->v0 += vOffset;
                prm.lsgt4->u1 += uOffset;
                prm.lsgt4->v1 += vOffset;
                prm.lsgt4->u2 += uOffset;
                prm.lsgt4->v2 += vOffset;
                prm.lsgt4->u3 += uOffset;
                prm.lsgt4->v3 += vOffset;
                SET_POSITION(prm.lsgt4, 0);
                SET_COLOUR(&prm.lsgt4->colour[0]);
                SET_UV(prm.lsgt4->u0, prm.lsgt4->v0);
                vertex++;
                SET_POSITION(prm.lsgt4, 1);
                SET_COLOUR(&prm.lsgt4->colour[1]);
                SET_UV(prm.lsgt4->u1, prm.lsgt4->v1);
                vertex++;
                SET_POSITION(prm.lsgt4, 2);
                SET_COLOUR(&prm.lsgt4->colour[2]);
                SET_UV(prm.lsgt4->u2, prm.lsgt4->v2);
                vertex++;
                SET_POSITION(prm.lsgt4, 3);
                SET_COLOUR(&prm.lsgt4->colour[3]);
                SET_UV(prm.lsgt4->u3, prm.lsgt4->v3);
                vertex++;
                prm.lsgt4++;
                break;
            case TypeSpline:
                prm.spline++;
                break;
            case TypeInfiniteLight:
                prm.infiniteLight++;
                break;
            case TypePointLight:
                prm.pointLight++;
                break;
            case TypeSpotLight:
                prm.spotLight++;
                break;
            default:
                wsprintf(errorString, "Bad primitive type %x in $%x \n", prm.f3->type, name);
                Error(errorString, Fatal);
                break;
            }
        }
        object->vertexCount = vertex;
        object->floatVerts = DAlloc(heap, vertex * sizeof(D3DLVERTEX));
        if (!object->floatVerts) {
            wsprintf(g_cdMessage, "Memory alloc failed %d vertices\n", vertex);
            OutputDebugString(g_cdMessage);
        }
        memcpy(object->floatVerts, loadVertices, vertex * sizeof(D3DLVERTEX));
        object->spriteQuadCount = loadQuadCount;
        object->spriteQuads = DAlloc(heap, loadQuadCount * sizeof(SpriteQuad));
        memcpy(object->spriteQuads, loadQuads, loadQuadCount * sizeof(SpriteQuad));
        for (i = 0; normalSetNo >= i; i++) {
            if (quadBuckets[i].count) {
                object->triLists[i].count = quadBuckets[i].count;
                object->triLists[i].tris = DAlloc(heap, quadBuckets[i].count * sizeof(D3DTRIANGLE));
                memcpy(object->triLists[i].tris, quadBuckets[i].triangles, quadBuckets[i].count * sizeof(D3DTRIANGLE));
            } else {
                object->triLists[i].count = 0;
                object->triLists[i].tris = 0;
            }
        }
        if (lastObject)
            lastObject->next = object;
        lastObject = object;
    }
    return firstObject;
}
#endif

/* NET differs only in matrix index evaluation and the two bucket comparisons. */
#ifdef NETPLAY
Object *LoadPrmCommon(WADFILE *wad, char *name, short textureStart)
{
    int faceCount;
    int nextVertexIndex;
    int totalTriangles;
    char *fileData;
    u_char vTexOffset;
    int colourPage;
    Object *objects;
    Object *currentObject;
    long byteLength;
    u_char uPage;
    short prmError;
    char *bufferEnd;
    short i;
    u_char uPalette;
    Prm primitive;
    short loopIndex;
    int textureSet;
    u_char paletteRow;
    Texture *tex;
    Object *lastObject;

    prmError = 0;
    colourPage = normalSetNo;
    uPalette = 0;
    paletteRow = 0;
/* Rebuild the object from the start after a malformed primitive. */
again:
    if (wad == 0) {
        byteLength = FileLength(name);
        if (byteLength <= 0) {
            FileAccessError(name);
            wsprintf(errorString, "scene.c:LoadPrm: File %s not found\n", name);
            Error(errorString, Fatal);
        }
        fileData = DAlloc(heap, byteLength + 32);
        bufferEnd = fileData + byteLength;
        if (!fileData) {
            wsprintf(errorString, "scene.c:LoadPrm(): Failed to allocate memory for file %s\n", name);
            Error(errorString, Fatal);
        }
        /* The original tests the previous primitive error here, with an empty body. */
        if (prmError) {
        }
        if (LoadFile(name, fileData) != byteLength) {
            wsprintf(errorString, "scene.c:LoadPrm(): Failed to load file %s\n", name);
            Error(errorString, Fatal);
        }
    } else {
        byteLength = wad->sizes[wad->current];
        fileData = DAlloc(heap, byteLength + 32);
        bufferEnd = fileData + byteLength;
        readnextfile(wad, fileData);
    }
    primitive.ptr = fileData;
    objects = primitive.object;
    lastObject = 0;
    while (bufferEnd > primitive.ptr) {
        currentObject = primitive.object++;
        IntelShort(&currentObject->vertexCount);
        IntelShort(&currentObject->primitiveCount);
        IntelLong(&currentObject->extent);
        currentObject->normalCount = 12345;
        currentObject->normals = DAlloc(heap, sizeof(TriList) * 23);
        if (!currentObject->normals)
            OutputDebugString("Oh bugger\n");
        currentObject->skeleton = primitive.skeleton++;
        for (loopIndex = 0; loopIndex < 3; loopIndex++) {
            for (i = 0; i < 3; i++)
                IntelShort(&currentObject->skeleton->relative.m[loopIndex][i]);
            IntelLong(&currentObject->skeleton->relative.t[loopIndex]);
            IntelLong(&currentObject->skeleton->absolute.t[loopIndex]);
        }
        IntelShort(&currentObject->skeleton->update);
        currentObject->next = 0;
        currentObject->vertices = primitive.svector;
        primitive.svector += currentObject->vertexCount;
        for (loopIndex = 0; loopIndex < currentObject->vertexCount; loopIndex++) {
            IntelShort(&currentObject->vertices[loopIndex].vx);
            IntelShort(&currentObject->vertices[loopIndex].vy);
            IntelShort(&currentObject->vertices[loopIndex].vz);
        }
        currentObject->primitives = primitive.primitive;
        /* The original clears these counters but never reads them. */
        faceCount = 0;
        loadLastPage = -1;
        nextVertexIndex = 0;
        loadQuadCount = 0;
        totalTriangles = 0;
        for (loopIndex = 0; normalSetNo >= loopIndex; loopIndex++)
            quadBuckets[loopIndex].count = 0;
        for (loopIndex = 0; loopIndex < currentObject->primitiveCount; loopIndex++) {
            prmError = IntelPrim(primitive);
            if (prmError == 1) {
                DFree(heap, fileData);
                fileData = 0;
                OutputDebugString("error with prm file\n");
                goto again;
            }
            switch (primitive.f3->type) {
            case TypeF3:
                if (rgbTextures)
                    colourPage = normalSetNo;
                else
                    colourPage = 11;
                ADD_TRIANGLE_PLAIN(colourPage, nextVertexIndex, nextVertexIndex + 1, nextVertexIndex + 2);
                if (rgbTextures) {
                    uPalette = 0;
                    paletteRow = 0;
                } else {
                    uPalette = FindNearestColourIndex(&primitive.f3->colour, 1);
                    paletteRow = 110;
                }
                SET_POSITION(primitive.f3, 0);
                SET_COLOUR(&primitive.f3->colour);
                SET_PALETTE_UV();
                nextVertexIndex++;
                SET_POSITION(primitive.f3, 1);
                SET_COLOUR(&primitive.f3->colour);
                SET_PALETTE_UV();
                nextVertexIndex++;
                SET_POSITION(primitive.f3, 2);
                SET_COLOUR(&primitive.f3->colour);
                SET_PALETTE_UV();
                nextVertexIndex++;
                primitive.f3++;
                break;
            case TypeF4:
                if (rgbTextures)
                    colourPage = normalSetNo;
                else
                    colourPage = 11;
                ADD_QUAD(colourPage);
                if (rgbTextures) {
                    uPalette = 0;
                    paletteRow = 0;
                } else {
                    uPalette = FindNearestColourIndex(&primitive.f4->colour, 1);
                    paletteRow = 110;
                }
                SET_POSITION(primitive.f4, 0);
                SET_COLOUR(&primitive.f4->colour);
                SET_PALETTE_UV();
                nextVertexIndex++;
                SET_POSITION(primitive.f4, 1);
                SET_COLOUR(&primitive.f4->colour);
                SET_PALETTE_UV();
                nextVertexIndex++;
                SET_POSITION(primitive.f4, 2);
                SET_COLOUR(&primitive.f4->colour);
                SET_PALETTE_UV();
                nextVertexIndex++;
                SET_POSITION(primitive.f4, 3);
                SET_COLOUR(&primitive.f4->colour);
                SET_PALETTE_UV();
                nextVertexIndex++;
                primitive.f4++;
                break;
            case TypeFT3:
                primitive.ft3->texture += textureStart;
                tex = textureTable[primitive.ft3->texture];
                TEXTURE_SET(tex);
                ADD_TRIANGLE_PLAIN(textureSet, nextVertexIndex, nextVertexIndex + 1, nextVertexIndex + 2);
                uPage = tex->u0;
                vTexOffset = tex->v0;
                primitive.ft3->cba = tex->cba;
                primitive.ft3->tsb = tex->tsb;
                primitive.ft3->u0 += uPage;
                primitive.ft3->v0 += vTexOffset;
                primitive.ft3->u1 += uPage;
                primitive.ft3->v1 += vTexOffset;
                primitive.ft3->u2 += uPage;
                primitive.ft3->v2 += vTexOffset;
                SET_POSITION(primitive.ft3, 0);
                SET_COLOUR(&primitive.ft3->colour);
                SET_UV(primitive.ft3->u0, primitive.ft3->v0);
                nextVertexIndex++;
                SET_POSITION(primitive.ft3, 1);
                SET_COLOUR(&primitive.ft3->colour);
                SET_UV(primitive.ft3->u1, primitive.ft3->v1);
                nextVertexIndex++;
                SET_POSITION(primitive.ft3, 2);
                SET_COLOUR(&primitive.ft3->colour);
                SET_UV(primitive.ft3->u2, primitive.ft3->v2);
                nextVertexIndex++;
                primitive.ft3++;
                break;
            case TypeFT4:
                primitive.ft4->texture += textureStart;
                tex = textureTable[primitive.ft4->texture];
                TEXTURE_SET(tex);
                ADD_QUAD(textureSet);
                uPage = tex->u0;
                vTexOffset = tex->v0;
                primitive.ft4->cba = tex->cba;
                primitive.ft4->tsb = tex->tsb;
                primitive.ft4->u0 += uPage;
                primitive.ft4->v0 += vTexOffset;
                primitive.ft4->u1 += uPage;
                primitive.ft4->v1 += vTexOffset;
                primitive.ft4->u2 += uPage;
                primitive.ft4->v2 += vTexOffset;
                primitive.ft4->u3 += uPage;
                primitive.ft4->v3 += vTexOffset;
                SET_POSITION(primitive.ft4, 0);
                SET_COLOUR(&primitive.ft4->colour);
                SET_UV(primitive.ft4->u0, primitive.ft4->v0);
                nextVertexIndex++;
                SET_POSITION(primitive.ft4, 1);
                SET_COLOUR(&primitive.ft4->colour);
                SET_UV(primitive.ft4->u1, primitive.ft4->v1);
                nextVertexIndex++;
                SET_POSITION(primitive.ft4, 2);
                SET_COLOUR(&primitive.ft4->colour);
                SET_UV(primitive.ft4->u2, primitive.ft4->v2);
                nextVertexIndex++;
                SET_POSITION(primitive.ft4, 3);
                SET_COLOUR(&primitive.ft4->colour);
                SET_UV(primitive.ft4->u3, primitive.ft4->v3);
                nextVertexIndex++;
                primitive.ft4++;
                break;
            case TypeG3:
                if (rgbTextures)
                    colourPage = normalSetNo;
                else
                    colourPage = 11;
                ADD_TRIANGLE_PLAIN(colourPage, nextVertexIndex, nextVertexIndex + 1, nextVertexIndex + 2);
                if (rgbTextures) {
                    uPalette = 0;
                    paletteRow = 0;
                } else {
                    uPalette = FindNearestColourIndex(&primitive.g3->colour[0], 3);
                    paletteRow = 110;
                }
                SET_POSITION(primitive.g3, 0);
                SET_COLOUR(&primitive.g3->colour[0]);
                SET_PALETTE_UV();
                nextVertexIndex++;
                SET_POSITION(primitive.g3, 1);
                SET_COLOUR(&primitive.g3->colour[1]);
                SET_PALETTE_UV();
                nextVertexIndex++;
                SET_POSITION(primitive.g3, 2);
                SET_COLOUR(&primitive.g3->colour[2]);
                SET_PALETTE_UV();
                nextVertexIndex++;
                primitive.g3++;
                break;
            case TypeG4:
                if (rgbTextures)
                    colourPage = normalSetNo;
                else
                    colourPage = 11;
                ADD_QUAD(colourPage);
                if (rgbTextures) {
                    uPalette = 0;
                    paletteRow = 0;
                } else {
                    uPalette = FindNearestColourIndex(&primitive.g4->colour[0], 4);
                    paletteRow = 110;
                }
                SET_POSITION(primitive.g4, 0);
                SET_COLOUR(&primitive.g4->colour[0]);
                SET_PALETTE_UV();
                nextVertexIndex++;
                SET_POSITION(primitive.g4, 1);
                SET_COLOUR(&primitive.g4->colour[1]);
                SET_PALETTE_UV();
                nextVertexIndex++;
                SET_POSITION(primitive.g4, 2);
                SET_COLOUR(&primitive.g4->colour[2]);
                SET_PALETTE_UV();
                nextVertexIndex++;
                SET_POSITION(primitive.g4, 3);
                SET_COLOUR(&primitive.g4->colour[3]);
                SET_PALETTE_UV();
                nextVertexIndex++;
                primitive.g4++;
                break;
            case TypeGT3:
                primitive.gt3->texture += textureStart;
                tex = textureTable[primitive.gt3->texture];
                TEXTURE_SET(tex);
                ADD_TRIANGLE_PLAIN(textureSet, nextVertexIndex, nextVertexIndex + 1, nextVertexIndex + 2);
                uPage = tex->u0;
                vTexOffset = tex->v0;
                primitive.gt3->cba = tex->cba;
                primitive.gt3->tsb = tex->tsb;
                primitive.gt3->u0 += uPage;
                primitive.gt3->v0 += vTexOffset;
                primitive.gt3->u1 += uPage;
                primitive.gt3->v1 += vTexOffset;
                primitive.gt3->u2 += uPage;
                primitive.gt3->v2 += vTexOffset;
                SET_POSITION(primitive.gt3, 0);
                SET_COLOUR(&primitive.gt3->colour[0]);
                SET_UV(primitive.gt3->u0, primitive.gt3->v0);
                nextVertexIndex++;
                SET_POSITION(primitive.gt3, 1);
                SET_COLOUR(&primitive.gt3->colour[1]);
                SET_UV(primitive.gt3->u1, primitive.gt3->v1);
                nextVertexIndex++;
                SET_POSITION(primitive.gt3, 2);
                SET_COLOUR(&primitive.gt3->colour[2]);
                SET_UV(primitive.gt3->u2, primitive.gt3->v2);
                nextVertexIndex++;
                primitive.gt3++;
                break;
            case TypeGT4:
                primitive.gt4->texture += textureStart;
                tex = textureTable[primitive.gt4->texture];
                TEXTURE_SET(tex);
                ADD_QUAD(textureSet);
                uPage = tex->u0;
                vTexOffset = tex->v0;
                primitive.gt4->cba = tex->cba;
                primitive.gt4->tsb = tex->tsb;
                primitive.gt4->u0 += uPage;
                primitive.gt4->v0 += vTexOffset;
                primitive.gt4->u1 += uPage;
                primitive.gt4->v1 += vTexOffset;
                primitive.gt4->u2 += uPage;
                primitive.gt4->v2 += vTexOffset;
                primitive.gt4->u3 += uPage;
                primitive.gt4->v3 += vTexOffset;
                SET_POSITION(primitive.gt4, 0);
                SET_COLOUR(&primitive.gt4->colour[0]);
                SET_UV(primitive.gt4->u0, primitive.gt4->v0);
                nextVertexIndex++;
                SET_POSITION(primitive.gt4, 1);
                SET_COLOUR(&primitive.gt4->colour[1]);
                SET_UV(primitive.gt4->u1, primitive.gt4->v1);
                nextVertexIndex++;
                SET_POSITION(primitive.gt4, 2);
                SET_COLOUR(&primitive.gt4->colour[2]);
                SET_UV(primitive.gt4->u2, primitive.gt4->v2);
                nextVertexIndex++;
                SET_POSITION(primitive.gt4, 3);
                SET_COLOUR(&primitive.gt4->colour[3]);
                SET_UV(primitive.gt4->u3, primitive.gt4->v3);
                nextVertexIndex++;
                primitive.gt4++;
                break;
            case TypeLSF3:
                ADD_TRIANGLE_PLAIN(normalSetNo, nextVertexIndex, nextVertexIndex + 1, nextVertexIndex + 2);
                SET_POSITION(primitive.lsf3, 0);
                SET_COLOUR(&primitive.lsf3->colour);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                SET_POSITION(primitive.lsf3, 1);
                SET_COLOUR(&primitive.lsf3->colour);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                SET_POSITION(primitive.lsf3, 2);
                SET_COLOUR(&primitive.lsf3->colour);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                primitive.lsf3++;
                break;
            case TypeLSF4:
                ADD_QUAD(normalSetNo);
                SET_POSITION(primitive.lsf4, 0);
                SET_COLOUR(&primitive.lsf4->colour);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                SET_POSITION(primitive.lsf4, 1);
                SET_COLOUR(&primitive.lsf4->colour);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                SET_POSITION(primitive.lsf4, 2);
                SET_COLOUR(&primitive.lsf4->colour);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                SET_POSITION(primitive.lsf4, 3);
                SET_COLOUR(&primitive.lsf4->colour);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                primitive.lsf4++;
                break;
            case TypeLSFT3:
                primitive.lsft3->texture += textureStart;
                tex = textureTable[primitive.lsft3->texture];
                TEXTURE_SET(tex);
                ADD_TRIANGLE_PLAIN(textureSet, nextVertexIndex, nextVertexIndex + 1, nextVertexIndex + 2);
                uPage = tex->u0;
                vTexOffset = tex->v0;
                primitive.lsft3->cba = tex->cba;
                primitive.lsft3->tsb = tex->tsb;
                primitive.lsft3->u0 += uPage;
                primitive.lsft3->v0 += vTexOffset;
                primitive.lsft3->u1 += uPage;
                primitive.lsft3->v1 += vTexOffset;
                primitive.lsft3->u2 += uPage;
                primitive.lsft3->v2 += vTexOffset;
                SET_POSITION(primitive.lsft3, 0);
                SET_COLOUR(&primitive.lsft3->colour);
                SET_UV(primitive.lsft3->u0, primitive.lsft3->v0);
                nextVertexIndex++;
                SET_POSITION(primitive.lsft3, 1);
                SET_COLOUR(&primitive.lsft3->colour);
                SET_UV(primitive.lsft3->u1, primitive.lsft3->v1);
                nextVertexIndex++;
                SET_POSITION(primitive.lsft3, 2);
                SET_COLOUR(&primitive.lsft3->colour);
                SET_UV(primitive.lsft3->u2, primitive.lsft3->v2);
                nextVertexIndex++;
                primitive.lsft3++;
                break;
            case TypeLSFT4:
                primitive.lsft4->texture += textureStart;
                tex = textureTable[primitive.lsft4->texture];
                TEXTURE_SET(tex);
                ADD_QUAD(textureSet);
                uPage = tex->u0;
                vTexOffset = tex->v0;
                primitive.lsft4->cba = tex->cba;
                primitive.lsft4->tsb = tex->tsb;
                primitive.lsft4->u0 += uPage;
                primitive.lsft4->v0 += vTexOffset;
                primitive.lsft4->u1 += uPage;
                primitive.lsft4->v1 += vTexOffset;
                primitive.lsft4->u2 += uPage;
                primitive.lsft4->v2 += vTexOffset;
                primitive.lsft4->u3 += uPage;
                primitive.lsft4->v3 += vTexOffset;
                SET_POSITION(primitive.lsft4, 0);
                SET_COLOUR(&primitive.lsft4->colour);
                SET_UV(primitive.lsft4->u0, primitive.lsft4->v0);
                nextVertexIndex++;
                SET_POSITION(primitive.lsft4, 1);
                SET_COLOUR(&primitive.lsft4->colour);
                SET_UV(primitive.lsft4->u1, primitive.lsft4->v1);
                nextVertexIndex++;
                SET_POSITION(primitive.lsft4, 2);
                SET_COLOUR(&primitive.lsft4->colour);
                SET_UV(primitive.lsft4->u2, primitive.lsft4->v2);
                nextVertexIndex++;
                SET_POSITION(primitive.lsft4, 3);
                SET_COLOUR(&primitive.lsft4->colour);
                SET_UV(primitive.lsft4->u3, primitive.lsft4->v3);
                nextVertexIndex++;
                primitive.lsft4++;
                break;
            case TypeLSG3:
                ADD_TRIANGLE_PLAIN(normalSetNo, nextVertexIndex, nextVertexIndex + 1, nextVertexIndex + 2);
                SET_POSITION(primitive.lsg3, 0);
                SET_COLOUR(&primitive.lsg3->colour[0]);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                SET_POSITION(primitive.lsg3, 1);
                SET_COLOUR(&primitive.lsg3->colour[1]);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                SET_POSITION(primitive.lsg3, 2);
                SET_COLOUR(&primitive.lsg3->colour[2]);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                primitive.lsg3++;
                break;
            case TypeLSG4:
                ADD_QUAD(normalSetNo);
                SET_POSITION(primitive.lsg4, 0);
                SET_COLOUR(&primitive.lsg4->colour[0]);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                SET_POSITION(primitive.lsg4, 1);
                SET_COLOUR(&primitive.lsg4->colour[1]);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                SET_POSITION(primitive.lsg4, 2);
                SET_COLOUR(&primitive.lsg4->colour[2]);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                SET_POSITION(primitive.lsg4, 3);
                SET_COLOUR(&primitive.lsg4->colour[3]);
                loadVertices[nextVertexIndex].tu = 0;
                loadVertices[nextVertexIndex].tv = 0;
                nextVertexIndex++;
                primitive.lsg4++;
                break;
            case TypeLSGT3:
                primitive.lsgt3->texture += textureStart;
                tex = textureTable[primitive.lsgt3->texture];
                TEXTURE_SET(tex);
                ADD_TRIANGLE_PLAIN(textureSet, nextVertexIndex, nextVertexIndex + 1, nextVertexIndex + 2);
                uPage = tex->u0;
                vTexOffset = tex->v0;
                primitive.lsgt3->cba = tex->cba;
                primitive.lsgt3->tsb = tex->tsb;
                primitive.lsgt3->u0 += uPage;
                primitive.lsgt3->v0 += vTexOffset;
                primitive.lsgt3->u1 += uPage;
                primitive.lsgt3->v1 += vTexOffset;
                primitive.lsgt3->u2 += uPage;
                primitive.lsgt3->v2 += vTexOffset;
                SET_POSITION(primitive.lsgt3, 0);
                SET_COLOUR(&primitive.lsgt3->colour[0]);
                SET_UV(primitive.lsgt3->u0, primitive.lsgt3->v0);
                nextVertexIndex++;
                SET_POSITION(primitive.lsgt3, 1);
                SET_COLOUR(&primitive.lsgt3->colour[1]);
                SET_UV(primitive.lsgt3->u1, primitive.lsgt3->v1);
                nextVertexIndex++;
                SET_POSITION(primitive.lsgt3, 2);
                SET_COLOUR(&primitive.lsgt3->colour[2]);
                SET_UV(primitive.lsgt3->u2, primitive.lsgt3->v2);
                nextVertexIndex++;
                primitive.lsgt3++;
                break;
            case TypeLSGT4:
                primitive.lsgt4->texture += textureStart;
                tex = textureTable[primitive.lsgt4->texture];
                TEXTURE_SET(tex);
                ADD_QUAD(textureSet);
                uPage = tex->u0;
                vTexOffset = tex->v0;
                primitive.lsgt4->cba = tex->cba;
                primitive.lsgt4->tsb = tex->tsb;
                primitive.lsgt4->u0 += uPage;
                primitive.lsgt4->v0 += vTexOffset;
                primitive.lsgt4->u1 += uPage;
                primitive.lsgt4->v1 += vTexOffset;
                primitive.lsgt4->u2 += uPage;
                primitive.lsgt4->v2 += vTexOffset;
                primitive.lsgt4->u3 += uPage;
                primitive.lsgt4->v3 += vTexOffset;
                SET_POSITION(primitive.lsgt4, 0);
                SET_COLOUR(&primitive.lsgt4->colour[0]);
                SET_UV(primitive.lsgt4->u0, primitive.lsgt4->v0);
                nextVertexIndex++;
                SET_POSITION(primitive.lsgt4, 1);
                SET_COLOUR(&primitive.lsgt4->colour[1]);
                SET_UV(primitive.lsgt4->u1, primitive.lsgt4->v1);
                nextVertexIndex++;
                SET_POSITION(primitive.lsgt4, 2);
                SET_COLOUR(&primitive.lsgt4->colour[2]);
                SET_UV(primitive.lsgt4->u2, primitive.lsgt4->v2);
                nextVertexIndex++;
                SET_POSITION(primitive.lsgt4, 3);
                SET_COLOUR(&primitive.lsgt4->colour[3]);
                SET_UV(primitive.lsgt4->u3, primitive.lsgt4->v3);
                nextVertexIndex++;
                primitive.lsgt4++;
                break;
            case TypeTSPR:
            case TypeBSPR:
                primitive.spr->texture += textureStart;
                tex = textureTable[primitive.spr->texture];
                TEXTURE_SET(tex);
                loadQuads[loadQuadCount].position = currentObject->vertices[primitive.spr->coord];
                loadQuads[loadQuadCount].set = textureSet;
                loadQuads[loadQuadCount].colour = primitive.spr->colour;
                loadQuads[loadQuadCount].width = primitive.spr->width * 2;
                loadQuads[loadQuadCount].height = primitive.spr->height;
                loadQuads[loadQuadCount].textureW = tex->textureW;
                loadQuads[loadQuadCount].textureH = tex->textureH;
                loadQuads[loadQuadCount].u0 = tex->u0;
                loadQuads[loadQuadCount].v0 = tex->v0;
                loadQuads[loadQuadCount].u1 = tex->u1;
                loadQuads[loadQuadCount].v1 = tex->v1;
                loadQuads[loadQuadCount].u2 = tex->u2;
                loadQuads[loadQuadCount].v2 = tex->v2;
                loadQuads[loadQuadCount].u3 = tex->u3;
                loadQuads[loadQuadCount].v3 = tex->v3;
                loadQuadCount++;
                primitive.spr++;
                break;
            case TypeSpline:
                primitive.spline++;
                break;
            case TypePointLight:
                primitive.pointLight++;
                break;
            case TypeSpotLight:
                primitive.spotLight++;
                break;
            case TypeInfiniteLight:
                primitive.infiniteLight++;
                break;
            default:
                wsprintf(errorString, "Bad primitive type %x in $%x \n", primitive.f3->type, name);
                Error(errorString, Fatal);
                break;
            }
        }
        currentObject->vertexCount = nextVertexIndex;
        currentObject->floatVerts = DAlloc(heap, nextVertexIndex * sizeof(D3DLVERTEX));
        if (!currentObject->floatVerts) {
            wsprintf(g_cdMessage, "Memory alloc failed %d vertices\n", nextVertexIndex);
            OutputDebugString(g_cdMessage);
        }
        memcpy(currentObject->floatVerts, loadVertices, nextVertexIndex * sizeof(D3DLVERTEX));
        currentObject->spriteQuadCount = loadQuadCount;
        currentObject->spriteQuads = DAlloc(heap, loadQuadCount * sizeof(SpriteQuad));
        memcpy(currentObject->spriteQuads, loadQuads, loadQuadCount * sizeof(SpriteQuad));
        for (loopIndex = 0; normalSetNo >= loopIndex; loopIndex++) {
            if (quadBuckets[loopIndex].count) {
                currentObject->triLists[loopIndex].count = quadBuckets[loopIndex].count;
                currentObject->triLists[loopIndex].tris = DAlloc(heap, quadBuckets[loopIndex].count * sizeof(D3DTRIANGLE));
                memcpy(currentObject->triLists[loopIndex].tris, quadBuckets[loopIndex].triangles, quadBuckets[loopIndex].count * sizeof(D3DTRIANGLE));
            } else {
                currentObject->triLists[loopIndex].count = 0;
                currentObject->triLists[loopIndex].tris = 0;
            }
        }
        if (lastObject)
            lastObject->next = currentObject;
        lastObject = currentObject;
    }
    return objects;
}
#endif
#endif

void FreePrm(Object *object)
{
}

Object *WADLoadPrm(WADFILE *wad, short textureStart)
{
    return LoadPrmCommon(wad, 0, textureStart);
}

Object *LoadPrm(char *name, short textureStart)
{
    return LoadPrmCommon(0, name, textureStart);
}

Object *NewInstance(Object *source)
{
    Object *copy;
#ifdef NETPLAY
    int i;
#else
    char *name;
    char *sourceName;
#endif

    copy = DAlloc(heap, sizeof(Object));
    copy->vertexCount = source->vertexCount;
    copy->vertices = source->vertices;
    copy->primitiveCount = source->primitiveCount;
    copy->primitives = source->primitives;
    copy->normalCount = 12345;
    copy->normals = source->normals;
    copy->bspTree = source->bspTree;
    copy->spriteQuads = source->spriteQuads;
    copy->spriteQuadCount = source->spriteQuadCount;
    copy->skeleton = DAlloc(heap, sizeof(Skeleton));
    copy->skeleton->super = source->skeleton->super;
    copy->skeleton->sub = source->skeleton->sub;
    copy->skeleton->next = source->skeleton->next;
    copy->extent = source->extent;
    copy->next = 0;
#ifdef NETPLAY
    for (i = 0; i < 16; i++) {
        copy->name[i] = source->name[i];
    }
#else
    name = copy->name;
    sourceName = source->name;
    memcpy(name, sourceName, 16);
#endif
    return copy;
}
