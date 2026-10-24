#ifndef RENDER_H
#define RENDER_H

#include "d3d.h"
#include "wipeout.h"

/* Triangle list of one normal set of an object: count triangles starting at tris. */
typedef struct TriList {
    long          count;
    D3DTRIANGLE  *tris;
} TriList;

/* The triangles collected for one texture set while a mesh is built: count, 1800 triangles, one pad word. */
typedef struct TrackBucket {
    int          count;
    D3DTRIANGLE  triangles[1800];
    int          reserved;
} TrackBucket;

/* A billboard of an object (PRM sprite primitive), 0x20 bytes; the renderer turns it into four vertices. */
typedef struct SpriteQuad {
    SVECTOR       position;
    int           set;
    CVECTOR       colour;
    short         width, height;
    short         textureW, textureH;
    unsigned char u0, v0, u1, v1, u2, v2, u3, v3;
} SpriteQuad;

/* One object queued for the frame (0x60 bytes). The queue is flushed by DrawRenderQueue.
   quads: count 0x20-byte billboard records, turned into vertex quads by BuildQuads.
   vertices: vertexCount source vertices; triLists holds one TriList per normal set.
   mode: 0 opaque, nonzero uses the translucent bucket. */
typedef struct RenderEntry {
    float         matrix[16];
    int           quadCount;     /* 0x40 */
    void         *quads;         /* 0x44 */
    D3DLVERTEX   *vertices;      /* 0x48 */
    int           vertexCount;   /* 0x4C */
    TriList      *triLists;      /* 0x50 */
    int           mode;          /* 0x54 */
    int           hasColour;     /* 0x58 */
    unsigned char r, g, b;       /* 0x5C */
} RenderEntry;

/* Next free byte in the frame's shared primitive buffer. */
extern char *trackVertexCursor;
extern TrackBucket quadBuckets[];

void DrawRenderQueue(void);
void BuildQuads(int count, void *quads, float *matrix);
void StartTransformBuffer(void);
void FinishTransformBuffer(void);

#endif
