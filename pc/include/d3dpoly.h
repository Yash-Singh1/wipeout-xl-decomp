#ifndef D3DPOLY_H
#define D3DPOLY_H

#include "wipeout.h"

#include "render_vertex.h"

/* Three vertex indices of a render triangle, and its flags. */
typedef struct RenderTriangleIndices {
    int vertices[3];
    int flags;
} RenderTriangleIndices;

/* One triangle of the render buffer: blend mode, texture handle, texture set and vertex indices. */
typedef struct RenderTriangle {
    int blendMode;
    int textureHandle;
    int textureSet;
    RenderTriangleIndices indices;
} RenderTriangle;

/* One frame's render buffer: the triangles and the transformed vertices, with their counts. */
typedef struct RenderBuffer {
    RenderTriangle triangles[1500];
    RenderVertex vertices[6000];
    int triangleCount;
    int vertexCount;
} RenderBuffer;

extern RenderBuffer *currentBuffer;
extern short triangleCount;
/* Accept the loading screen's extra compatibility argument without reading it. */
void AddLinePrim(float z, LINE_F2 *line, ...);
void AddPolyPrim(float z, POLY_F3 *poly);
void DrawPolyF4(float z, POLY_F4 *poly);

extern unsigned char vertexAlpha, opaqueAlpha, translucentAlpha;
void DrawPolyG4(float z, POLY_G4 *poly, int blendMode);

extern unsigned char particleAlpha;
void DrawPolyG3(float z, POLY_G3 *poly, int blendMode);

extern int nameDrawFirst, textureHandles[];
void DrawPolyFT4(float z, POLY_FT4 *poly, int blendMode);

extern int texture16Bit;
void DrawPixelTexturedQuad(float z, POLY_FT4 *poly, int blendMode);
void DrawScaledTexturedQuadDirect(float z, POLY_FT4 *poly, int blendMode);

extern unsigned short normalSetNo;
void DrawQuadWithAlphaGradient(float z, POLY_FT4 *poly, int blendMode);

/* Non-zero when the CPU has MMX (HasMMX, set at display setup): selects the MMX software blit path. */
extern int mmxAvailable;
void AddBlit640(int transparent, int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1, int page);
void AddBlit640Shifted(int transparent, int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1, int page);
void DrawScaledTexturedQuad(float z, POLY_FT4 *poly, int blendMode);

/* A bucket of triangle index pointers, filled by the depth sort (SortRenderTriangles). */
typedef struct TriangleBucket {
    int emitted;
    int count;
    RenderTriangleIndices *triangles[1800];
} TriangleBucket;

extern TriangleBucket triangleBuckets[27][2];
void SortRenderTriangles(void);
typedef void RenderSubmissionFunction(void);
extern RenderSubmissionFunction SubmitRenderTriangles;

typedef struct RenderInstruction {
    unsigned char opcode, size;
    unsigned short count;
} RenderInstruction;

typedef struct ProcessRenderVertices {
    unsigned long flags;
    unsigned short start, destination;
    unsigned long count, reserved;
} ProcessRenderVertices;

#include "render_execute.h"

extern unsigned char *renderInstruction, *processInstruction, *instructionBuffer;
extern int renderVertexCount;
extern unsigned long vertexBufferSize, maxInstructionLength;
extern const float processVertexOffsetScale;
extern long ddResult;
void FinishRenderBuffer(void);

typedef struct RenderBufferDescription {
    unsigned long size, flags, caps, bufferSize;
    void *data;
} RenderBufferDescription;

typedef struct RenderState {
    int type;
    unsigned long argument;
} RenderState;

enum {
    RENDER_TEXTURE_HANDLE = 1,
    RENDER_Z_ENABLE = 7,
    RENDER_Z_WRITE_ENABLE = 14,
    RENDER_TEXTURE_MAG = 17,
    RENDER_TEXTURE_MIN = 18,
    RENDER_SRC_BLEND = 19,
    RENDER_DEST_BLEND = 20,
    RENDER_TEXTURE_BLEND = 21,
    RENDER_CULL_MODE = 22,
    RENDER_ALPHA_BLEND_ENABLE = 27
};

extern RenderBufferDescription bufferDescription, bufferDescriptionTemplate;
extern int vertexIndices[];
extern RenderVertex *renderVertices;
extern int rgbTextures;
void StartRenderBuffer(void);

#include "pc_texture.h"

typedef union RenderTiming {
    double seconds;
    struct { unsigned long low, high; } words;
} RenderTiming;

typedef struct RenderTriangleInstruction {
    unsigned short vertex0, vertex1, vertex2, flags;
} RenderTriangleInstruction;

extern RenderTextureInfo textureSets[];
extern int blendByBucket, gameFlags;
extern RenderVertex clippedVertices[];
extern int sourceOwner[];
extern RenderBuffer renderBuffer;
extern RenderTiming startBufferTime;
extern double profileQueueTime, profileDrawTime;
double GetProfileTime(void);
void ClipRenderBuffer(void);

extern RenderBuffer clippingBuffer;
extern float screenScaleX, screenScaleY;

#endif
