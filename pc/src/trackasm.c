#include "functions.h"
#include "trackasm.h"
#include "ttf.h"
#include "render.h"
#include "pc_particles.h"
#include <string.h>

extern Texture *textureTable[];
extern Surface tileSurfaces[24][2];
extern POLY_FT4 trackLo0[0x230];
extern POLY_FT4 trackLo1[0x230];
extern POLY_FT4 *trackLo[2];
void AsmDefaultTile(TexTemplate *template, short index, short reversed, short alt);
extern TrackData *track;
extern ConfigData *gameData;
extern unsigned char opaqueAlpha;

extern int trackFacePage[];

extern short trackVertexCount;
extern short trackSectionCount;


void AsmLowResTile(TexTemplate *template, short index, short reversed)
{
    Texture *texture;
    Tile *tile;

    texture = textureTable[template[index].tex1[0]];
    tile = &tileSurfaces[index][reversed].low[0];
    tile->cba = texture->cba;
    tile->tsb = texture->tsb;
    if (reversed) {
        tile->u0 = texture->u1;
        tile->u1 = texture->u0;
        tile->u2 = texture->u3;
        tile->u3 = texture->u2;
        tile->v0 = texture->v1;
        tile->v1 = texture->v0;
        tile->v2 = texture->v3;
        tile->v3 = texture->v2;
    } else {
        tile->u0 = texture->u0;
        tile->u1 = texture->u1;
        tile->u2 = texture->u2;
        tile->u3 = texture->u3;
        tile->v0 = texture->v0;
        tile->v1 = texture->v1;
        tile->v2 = texture->v2;
        tile->v3 = texture->v3;
    }
}

void AsmMediumResTile(TexTemplate *template, short index, short reversed)
{
    Texture *texture;
    short i;
    Tile *tile;
    short tex;

    for (i = 0; i < 4; i++) {
        if (reversed) {
            tex = ((i & 2) + 1) - (i & 1);
        } else {
            tex = i;
        }
        texture = textureTable[template[index].tex4[tex]];
        tile = &tileSurfaces[index][reversed].med[i];
        tile->cba = texture->cba;
        tile->tsb = texture->tsb;
        if (reversed) {
            tile->u0 = texture->u1;
            tile->u1 = texture->u0;
            tile->u2 = texture->u3;
            tile->u3 = texture->u2;
            tile->v0 = texture->v1;
            tile->v1 = texture->v0;
            tile->v2 = texture->v3;
            tile->v3 = texture->v2;
        } else {
            tile->u0 = texture->u0;
            tile->u1 = texture->u1;
            tile->u2 = texture->u2;
            tile->u3 = texture->u3;
            tile->v0 = texture->v0;
            tile->v1 = texture->v1;
            tile->v2 = texture->v2;
            tile->v3 = texture->v3;
        }
    }
}

/* The whole texture becomes one tile; its page index selects the Direct3D texture. */
void AsmHighResTile(TexTemplate *template, short index, short reversed)
{
    Texture *texture;
    Tile *tile;
    int page;

    texture = textureTable[index];
    tile = &tileSurfaces[index][reversed].high[0];
    tile->cba = texture->cba;
    tile->tsb = texture->tsb;
    page = ((texture->textureY & 0x100) >> 4) + (texture->textureX >> 6);
    if (page > 15 && page < 100)
        page -= 10;
    else if (page < 16)
        page -= 5;
    tile->page = page;
    if (reversed == 0) {
        tile->u0 = texture->u1;
        tile->u1 = texture->u0;
        tile->u2 = texture->u3;
        tile->u3 = texture->u2;
        tile->v0 = texture->v1;
        tile->v1 = texture->v0;
        tile->v2 = texture->v3;
        tile->v3 = texture->v2;
    } else {
        tile->u0 = texture->u0;
        tile->u1 = texture->u1;
        tile->u2 = texture->u2;
        tile->u3 = texture->u3;
        tile->v0 = texture->v0;
        tile->v1 = texture->v1;
        tile->v2 = texture->v2;
        tile->v3 = texture->v3;
    }
}

/* Fills the two spare tiles after the track textures: quadrant 0x80..0xff of page 3 and quadrant 0..0x7f of page 4. */
void AsmDefaultTile(TexTemplate *template, short index, short reversed, short alt)
{
    u_char u0, u1, u2, u3, v0, v1, v2, v3;
    short x, y;
    short page;
    Tile *tile;

    u0 = 0x80;
    u1 = 0xff;
    u2 = 0x80;
    u3 = 0xff;
    v0 = 0x80;
    v1 = 0x80;
    v2 = 0xff;
    v3 = 0xff;
    x = 0xc0;
    y = 0x80;
    if (alt) {
        y = 0;
        u0 = 0;
        x = 0x100;
        u1 = 0x7f;
        u2 = 0;
        v0 = 0;
        v1 = 0;
        v2 = 0x7f;
        u3 = 0x7f;
        v3 = 0x7f;
        index++;
    }
    tile = &tileSurfaces[index][reversed].high[0];
    tile->cba = 0;
    page = ((y & 0x100) >> 4) + (x >> 6);
    tile->tsb = page | 0x20;
    tile->page = page;
    if (reversed) {
        tile->u0 = u1;
        tile->u1 = u0;
        tile->u2 = u3;
        tile->u3 = u2;
        tile->v0 = v1;
        tile->v1 = v0;
        tile->v2 = v3;
        tile->v3 = v2;
    } else {
        tile->u0 = u0;
        tile->u1 = u1;
        tile->u2 = u2;
        tile->u3 = u3;
        tile->v0 = v0;
        tile->v1 = v1;
        tile->v2 = v2;
        tile->v3 = v3;
    }
}

void AsmGenerateTiles(TexTemplate *tiles, short tileCount)
{
    short i;

    for (i = 0; i < tileCount; i++) {
        AsmLowResTile(tiles, i, 0);
        AsmLowResTile(tiles, i, 1);
        AsmMediumResTile(tiles, i, 0);
        AsmMediumResTile(tiles, i, 1);
        AsmHighResTile(tiles, i, 0);
        AsmHighResTile(tiles, i, 1);
    }
    AsmDefaultTile(tiles, tileCount, 0, 0);
    AsmDefaultTile(tiles, tileCount, 1, 0);
    AsmDefaultTile(tiles, tileCount, 0, 1);
    AsmDefaultTile(tiles, tileCount, 1, 1);
    trackLo[0] = trackLo0;
    trackLo[1] = trackLo1;
    for (i = 0; i < 0x230; i++) {
        SetPolyFT4(&trackLo0[i]);
        SetPolyFT4(&trackLo1[i]);
    }
}

void AsmNextFrame(void)
{
    trackVertexCount = 0;
    trackSectionCount = 0;
}

/* Fills one corner of a track quad: the position relative to the camera, the tile UVs and the face colour. */
#define TRACK_CORNER(n, uv) \
    vertex = &vertices[face->vertex[n]]; \
    out = (PCVertex *)(quads + index * sizeof(PCVertex)); \
    out->x = (vertex->vx + campos.vx) * (1.0f / 4096.0f); \
    out->y = (vertex->vy + campos.vy) * (1.0f / 4096.0f); \
    out->z = (vertex->vz + campos.vz) * (1.0f / 4096.0f); \
    out->u = (float)(u_long)tile->u##uv * (1.0f / 256.0f); \
    textureV = (float)(u_long)tile->v##uv * (1.0f / 256.0f); \
    memcpy(&out->v, &textureV, sizeof(textureV)); \
    out->a = opaqueAlpha; \
    out->r = face->red; \
    out->g = face->green; \
    out->b = face->blue; \
    index++

/* Turns highCount track sections into lit quads and queues them.
   Each corner's textureV is copied in (memcpy) before the colour bytes are written: the exe stores it
   there, while a plain assignment lets the compiler sink the store past them. */
#ifdef EQUIV
#ifdef NETPLAY
/* NET build: the corner is stored straight into the quad array (x87 values, divided by the exe's
   float constants 4096 and 256) and the texture V is not copied through a temporary. */
#define TRACK_CORNER_NET(n, uv) \
    vertexPtr = &vertices[faceEntry->vertex[n]]; \
    quadOut[vertexIndex].x = (float)(vertexPtr->vx + campos.vx) / 4096.0f; \
    quadOut[vertexIndex].y = (float)(vertexPtr->vy + campos.vy) / 4096.0f; \
    quadOut[vertexIndex].z = (float)(vertexPtr->vz + campos.vz) / 4096.0f; \
    quadOut[vertexIndex].u = (float)(u_long)tileCell->u##uv / 256.0f; \
    quadOut[vertexIndex].v = (float)(u_long)tileCell->v##uv / 256.0f; \
    quadOut[vertexIndex].a = opaqueAlpha; \
    quadOut[vertexIndex].r = faceEntry->red; \
    quadOut[vertexIndex].g = faceEntry->green; \
    quadOut[vertexIndex].b = faceEntry->blue; \
    vertexIndex++

/* The network build expands the trisLists memcpy inline (rep movs), so memcpy is an intrinsic here. */
#pragma intrinsic(memcpy)
TrackSection *DrawTrackSection(TrackSection *firstSection, Skeleton *camera, short highCount)
{
    short faceCount;
    TriList *trisLists;
    int set;
    VECTOR *vertexPtr;
    TrackSection *sectionCursor;
    short quadIdx;
    short pageCursor;
    int quadNum;
    int vertexIndex;
    short counter;
    int faceCursor;
    Surface *surf;
    int total;
    Face *faceEntry;
    VECTOR campos;
    Tile *tileCell;
    int base;
    short faceNo;
    PCVertex *quadOut;
    VECTOR *vertices;




















































































































    for (counter = 0; normalSetNo >= counter; counter++) {
        quadBuckets[counter].count = 0;
    }
    SetSkeletonTransform(camera->super);
    campos.vx = camera->relative.t[0];
    campos.vy = camera->relative.t[1];
    campos.vz = camera->relative.t[2];
    vertices = track->vertices;
    sectionCursor = firstSection;
    total = 0;
    for (counter = 0; counter < highCount; counter++) {
        faceCount = sectionCursor->faceCount;
        total += faceCount;
        sectionCursor = sectionCursor->nextSection.ptr;
    }
    total *= 4;
    quadOut = (PCVertex *)trackVertexCursor;
    trackVertexCursor += total * sizeof(PCVertex);
    sectionCursor = firstSection;
    vertexIndex = 0;
    faceCursor = 0;
    for (counter = 0; counter < highCount; counter++) {
        faceEntry = track->faces + sectionCursor->faceStart;
        faceCount = sectionCursor->faceCount;
        for (faceNo = 0; faceNo < faceCount; faceNo++) {
            if (faceEntry->flags & FLIP) {
                surf = &tileSurfaces[faceEntry->texture][1];
            } else {
                surf = &tileSurfaces[faceEntry->texture][0];
            }
            tileCell = &surf->high[0];
            trackFacePage[faceCursor++] = tileCell->page;
            TRACK_CORNER_NET(0, 0);
            TRACK_CORNER_NET(1, 1);
            TRACK_CORNER_NET(2, 3);
            TRACK_CORNER_NET(3, 2);
            faceEntry++;
        }
        sectionCursor = sectionCursor->nextSection.ptr;
    }
    quadNum = total / 4;
    for (pageCursor = 0, quadIdx = 0, base = 0; quadIdx < quadNum; quadIdx++, pageCursor++, base += 4) {
        if (gameData->texturesOnSigned == 0) {
            set = normalSetNo;
        } else {
            set = trackFacePage[pageCursor];
        }
        quadBuckets[set].triangles[quadBuckets[set].count].v1 = base + 2;
        quadBuckets[set].triangles[quadBuckets[set].count].v2 = base + 1;
        quadBuckets[set].triangles[quadBuckets[set].count].v3 = base;
        quadBuckets[set].count++;
        quadBuckets[set].triangles[quadBuckets[set].count].v1 = base + 3;
        quadBuckets[set].triangles[quadBuckets[set].count].v2 = base + 2;
        quadBuckets[set].triangles[quadBuckets[set].count].v3 = base;
        quadBuckets[set].count++;
    }
    trisLists = (TriList *)trackVertexCursor;
    trackVertexCursor += 23 * sizeof(TriList);
    for (counter = 0; counter <= normalSetNo; counter++) {
        if (quadBuckets[counter].count) {
            trisLists[counter].count = quadBuckets[counter].count;
            trisLists[counter].tris = (D3DTRIANGLE *)trackVertexCursor;
            trackVertexCursor += quadBuckets[counter].count * sizeof(D3DTRIANGLE);
            memcpy(trisLists[counter].tris, quadBuckets[counter].triangles, quadBuckets[counter].count * sizeof(D3DTRIANGLE));
        } else {
            trisLists[counter].count = 0;
            trisLists[counter].tris = 0;
        }
    }
    AddRenderSkeleton(camera->super, quadOut, total, trisLists, 0, 0, 0);
    trackSectionCount++;
    return sectionCursor;
}
#pragma function(memcpy)
#else
TrackSection *DrawTrackSection(TrackSection *firstSection, Skeleton *camera, short highCount)
{
    Surface *surface;
    TrackSection *section;
    VECTOR *vertex;
    int base;
    int set;
    VECTOR campos;
    Face *face;
    short faceCount;
    int index;
    char *quads;
    PCVertex *out;
    float textureV;
    int faceIndex;
    short i, j, k;
    Tile *tile;
    TriList *lists;
    int total;
    VECTOR *vertices;


    for (j = 0; j <= normalSetNo; j++) {
        quadBuckets[j].count = 0;
    }
    SetSkeletonTransform(camera->super);
    campos.vx = camera->relative.t[0];
    campos.vy = camera->relative.t[1];
    campos.vz = camera->relative.t[2];
    vertices = track->vertices;
    section = firstSection;
    total = 0;
    for (i = 0; i < highCount; i++) {
        total += section->faceCount;
        section = section->nextSection.ptr;
    }
    total *= 4;
    quads = trackVertexCursor;
    trackVertexCursor += total * sizeof(PCVertex);
    section = firstSection;
    index = 0;
    faceIndex = 0;
    for (i = 0; i < highCount; i++) {
        face = track->faces + section->faceStart;
        faceCount = section->faceCount;
        for (j = 0; j < faceCount; j++) {
            if (face->flags & FLIP) {
                surface = &tileSurfaces[face->texture][1];
            } else {
                surface = &tileSurfaces[face->texture][0];
            }
            tile = &surface->high[0];
            trackFacePage[faceIndex++] = tile->page;
            TRACK_CORNER(0, 0);
            TRACK_CORNER(1, 1);
            TRACK_CORNER(2, 3);
            TRACK_CORNER(3, 2);
            face++;
        }
        section = section->nextSection.ptr;
    }
    base = 0;
    for (j = 0, k = 0; j < total / 4; j++) {
        if (gameData->texturesOn == 0) {
            set = normalSetNo;
        } else {
            set = trackFacePage[k];
        }
        k++;
        quadBuckets[set].triangles[quadBuckets[set].count].v1 = base + 2;
        quadBuckets[set].triangles[quadBuckets[set].count].v2 = base + 1;
        quadBuckets[set].triangles[quadBuckets[set].count].v3 = base;
        quadBuckets[set].count++;
        base += 4;
        quadBuckets[set].triangles[quadBuckets[set].count].v1 = base - 1;
        quadBuckets[set].triangles[quadBuckets[set].count].v2 = base - 2;
        quadBuckets[set].triangles[quadBuckets[set].count].v3 = base - 4;
        quadBuckets[set].count++;
    }
    lists = (TriList *)trackVertexCursor;
    trackVertexCursor += 23 * sizeof(TriList);
    for (j = 0; j <= normalSetNo; j++) {
        if (quadBuckets[j].count) {
            lists[j].count = quadBuckets[j].count;
            lists[j].tris = (D3DTRIANGLE *)trackVertexCursor;
            trackVertexCursor += quadBuckets[j].count * sizeof(D3DTRIANGLE);
            memcpy(lists[j].tris, quadBuckets[j].triangles, quadBuckets[j].count * sizeof(D3DTRIANGLE));
        } else {
            lists[j].count = 0;
            lists[j].tris = 0;
        }
    }
    AddRenderSkeleton(camera->super, (PCVertex *)quads, total, lists, 0, 0, 0);
    trackSectionCount++;
    return section;
}
#endif
#endif
