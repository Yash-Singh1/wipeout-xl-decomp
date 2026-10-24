#ifndef RENDER_VERTEX_H
#define RENDER_VERTEX_H

/* Transformed Direct3D vertex. The packed colour and channel bytes share the
   same four bytes, with blue first on the PC. */
typedef struct RenderVertex {
    float sx, sy, sz, rhw;
    union {
        unsigned long color;
        struct {
            unsigned char blue, green, red, alpha;
        };
    };
    unsigned long specular;
    float tu, tv;
} RenderVertex;

typedef RenderVertex D3DTLVERTEX;

#ifdef NETPLAY
void SetScreenVertex(RenderVertex *vertex, int x, int y, float z,
#else
void SetScreenVertex(RenderVertex *vertex, float x, float y, float z,
#endif
                     unsigned char red, unsigned char green, unsigned char blue);
#ifdef NETPLAY
void SetScaledFloatScreenVertex(RenderVertex *vertex, float x, float y, float z,
                                unsigned char red, unsigned char green, unsigned char blue);
#endif
void SetScaledScreenVertex(RenderVertex *vertex, int x, int y, float z,
                           unsigned char red, unsigned char green, unsigned char blue);
void SetPixelScreenVertex(RenderVertex *vertex, int x, int y, float z,
                          unsigned char red, unsigned char green, unsigned char blue);

#endif
