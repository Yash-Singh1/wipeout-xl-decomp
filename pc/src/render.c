#include "functions.h"
#include "d3d.h"
#include "render_execute.h"
#include "render.h"
#include "d3dpoly.h"
#include <string.h>
#include <math.h>
#ifdef NETPLAY
#pragma intrinsic(memset)
#endif

extern IDirect3DDevice *lpD3DDevice;
extern D3DMATRIXHANDLE matrixWorld;
extern D3DMATRIXHANDLE matrixView;
extern D3DMATRIXHANDLE matrixProj;
extern int renderCount;

extern unsigned char *vertexData;
extern ConfigData *gameData;
#ifdef NETPLAY
extern signed char fadeActive;
#else
extern u_char fadeActive;
#endif
extern int paletteDirty;
extern int renderFrame;
extern int fadeAmount;
extern short fadeRed, fadeGreen, fadeBlue;
extern int cheatfade;
#include "pc_palette.h"

/* One bucket of triangles for a texture set and blend mode: emitted counts the triangles
   already streamed to the execute buffer. */
typedef struct TriBucket {
    int          emitted;
    int          count;
    D3DTRIANGLE  tris[1800];
} TriBucket;

#include "pc_texture.h"

extern RenderEntry renderQueue[];
#ifdef NETPLAY
extern TriBucket triBuckets[][2];
#else
extern TriBucket triBuckets[];
#endif
extern D3DLVERTEX quadVertices[];
extern D3DLVERTEX *sourceVertices[];
extern int quadVertexCount;
extern D3DMATRIX viewMatrix, worldMatrix;
extern int flipView;
extern int typeSwitchesMode;
extern int fadeSkipped;
double GetProfileTime(void);

/* Puts one source vertex into the execute buffer, transformed by the matrix of the triangle's
   owner, and records its slot in vertexIndices. */
#ifdef NETPLAY
/* Puts one source vertex into the execute buffer, transformed by the matrix of the triangle's
   owner, and records its slot in vertexIndices. */
#define ADD_VERTEX(vi, slot, mat) \
    slot = vertexIndices[vi]; \
    if (slot == -1) { \
        D3DLVERTEX *dst; \
        float x, y, z, tx; \
        float xFromX, yFromX, zFromX; \
        slot = renderVertexCount++; \
        dst = (D3DLVERTEX *)vertexData + slot; \
        *dst = *sourceVertices[vi]; \
        mat = *(D3DMATRIX *)renderQueue[owner].matrix; \
        x = dst->x; \
        y = dst->y; \
        z = dst->z; \
        xFromX = x * mat.m[0][0]; \
        yFromX = x * mat.m[0][1]; \
        zFromX = x * mat.m[0][2]; \
        tx = (z * mat.m[2][0] + y * mat.m[1][0]) + xFromX + mat.m[3][0]; \
        transformedY = (z * mat.m[2][1] + y * mat.m[1][1]) + yFromX + mat.m[3][1]; \
        tz = zFromX + (y * mat.m[1][2] + z * mat.m[2][2]) + mat.m[3][2]; \
        dst->x = tx; \
        dst->y = transformedY; \
        dst->z = tz; \
        if (renderQueue[owner].hasColour) { \
            unsigned char *rgb = (unsigned char *)&dst->color; \
            rgb[2] = renderQueue[owner].r; \
            rgb[1] = renderQueue[owner].g; \
            rgb[0] = renderQueue[owner].b; \
        } \
        vertexIndices[vi] = slot; \
    }
#else
/* Puts one source vertex into the execute buffer, transformed by the matrix of the triangle's
   owner, and records its slot in vertexIndices. */
#define ADD_VERTEX(vi, slot, mat) \
    slot = vertexIndices[vi]; \
    if (slot == -1) { \
        D3DLVERTEX *dst; \
        float x, y, z, tx; \
        float xFromX, yFromX, zFromX; \
        slot = renderVertexCount++; \
        dst = (D3DLVERTEX *)vertexData + slot; \
        *dst = *sourceVertices[vi]; \
        mat = *(D3DMATRIX *)renderQueue[owner].matrix; \
        x = dst->x; \
        y = dst->y; \
        z = dst->z; \
        xFromX = x * mat.m[0][0]; \
        yFromX = x * mat.m[0][1]; \
        zFromX = x * mat.m[0][2]; \
        tx = (z * mat.m[2][0] + y * mat.m[1][0]) + xFromX + mat.m[3][0]; \
        ty = (z * mat.m[2][1] + y * mat.m[1][1]) + yFromX + mat.m[3][1]; \
        tz = zFromX + (y * mat.m[1][2] + z * mat.m[2][2]) + mat.m[3][2]; \
        dst->x = tx; \
        dst->y = ty; \
        dst->z = tz; \
        if (renderQueue[owner].hasColour) { \
            unsigned char *rgb = (unsigned char *)&dst->color; \
            rgb[2] = renderQueue[owner].r; \
            rgb[1] = renderQueue[owner].g; \
            rgb[0] = renderQueue[owner].b; \
        } \
        vertexIndices[vi] = slot; \
    }
#endif

/* Blend setup for the bucket of one texture set when blending is decided per texture. */
#define SET_BUCKET_BLEND(first, set) \
    if (!(first)) { \
        OP_STATE_RENDER(3, renderInstruction); \
        STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction); \
        if (gameFlags & 2) { \
            STATE_DATA(D3DRENDERSTATE_DESTBLEND, 2, renderInstruction); \
            STATE_DATA(D3DRENDERSTATE_SRCBLEND, 2, renderInstruction); \
        } else { \
            STATE_DATA(D3DRENDERSTATE_DESTBLEND, 6, renderInstruction); \
            STATE_DATA(D3DRENDERSTATE_SRCBLEND, 5, renderInstruction); \
        } \
    } else if (textureSets[set].type == 3) { \
        OP_STATE_RENDER(3, renderInstruction); \
        STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction); \
        STATE_DATA(D3DRENDERSTATE_DESTBLEND, 6, renderInstruction); \
        STATE_DATA(D3DRENDERSTATE_SRCBLEND, 5, renderInstruction); \
    } else { \
        OP_STATE_RENDER(1, renderInstruction); \
        STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 0, renderInstruction); \
    }

/* Sets the texture handle of texture set j (none past the last set). The original tests
   nameDrawFirst here even though both arms store the same handle. */
#define SET_TEXTURE(j) \
    OP_STATE_RENDER(1, renderInstruction); \
    if (normalSetNo > (j)) { \
        if (nameDrawFirst) { \
            STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, textureHandles[j], renderInstruction); \
        } else { \
            STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, textureHandles[j], renderInstruction); \
        } \
    } else { \
        STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, 0, renderInstruction); \
    }

/* The same after an execute-buffer restart. The original reads normalSetNo before it stores
   the header count, and its second arm loads the handle before writing the state type. */
#define SET_TEXTURE_RESTART(j) \
    ((D3DINSTRUCTION *)renderInstruction)->opcode = D3DOP_STATERENDER; \
    ((D3DINSTRUCTION *)renderInstruction)->size = sizeof(D3DSTATE); \
    textureLimit = normalSetNo; \
    ((D3DINSTRUCTION *)renderInstruction)->count = 1; \
    renderInstruction = (void *)((char *)renderInstruction + sizeof(D3DINSTRUCTION)); \
    if (textureLimit > (j)) { \
        if (nameDrawFirst) { \
            ((D3DSTATE *)renderInstruction)->type = D3DRENDERSTATE_TEXTUREHANDLE; \
            ((D3DSTATE *)renderInstruction)->arg = textureHandles[j]; \
        } else { \
            register int handle; \
            handle = textureHandles[j]; \
            ((D3DSTATE *)renderInstruction)->type = D3DRENDERSTATE_TEXTUREHANDLE; \
            ((D3DSTATE *)renderInstruction)->arg = handle; \
        } \
    } else { \
        ((D3DSTATE *)renderInstruction)->type = D3DRENDERSTATE_TEXTUREHANDLE; \
        ((D3DSTATE *)renderInstruction)->arg = 0; \
    } \
    renderInstruction = (void *)((char *)renderInstruction + sizeof(D3DSTATE));

/* Gathers queued objects into texture buckets and streams them through execute buffers. */
#ifdef EQUIV
#ifdef NETPLAY
#pragma intrinsic(memcpy)
void DrawRenderQueue(void)
{
    TriBucket * currentBucket;
    double profileStart;
    D3DLVERTEX ** vertexCursor;
    int * ownerIndices;
    int sourceIndex;
    int firstTri;
    int renderTriTotal;
    int done;
    int queueObjectIndex;
    int triIndex;
    D3DTRIANGLE * dst;
    D3DTRIANGLE * src;
    D3DEXECUTEBUFFERDESC bufferDescription;
    int secondSlot;
    int currentTexture;
    int index1;
    int index;
    int setIndex;
    int startRenderVertexCount;
    int remaining;
    int m;
    unsigned char * triangleListPtr;
    int base;
    int i3;
    unsigned char * rgb;
    int quadEndIndex;
    int blendNo;
    int drawTotal;
    int sourceCount;
    int currentVertex;
    int mode;

    mode = 0;
    sourceVertexCount = 7000;
    vertexCursor = sourceVertices;
    ownerIndices = sourceOwner;
    startBufferTime.words.low = 0;
    startBufferTime.words.high = 0;
    renderTriTotal = 0;
    drawTotal = 0;
    if (fadeAmount <= 180 || fadeSkipped) {
        fadeSkipped = 0;
        renderFrame = 1;
    } else {
        fadeSkipped = 1;
        renderFrame = 0;
    }
    memcpy(&bufferDescription, &bufferDescriptionTemplate, sizeof(bufferDescription));
    ddResult = lpD3DDevice->lpVtbl->CreateMatrix(lpD3DDevice, &matrixView);
    if (flipView) {
        ddResult = lpD3DDevice->lpVtbl->SetMatrix(lpD3DDevice, matrixView, &viewMatrixFlipped);
    } else {
        ddResult = lpD3DDevice->lpVtbl->SetMatrix(lpD3DDevice, matrixView, &viewMatrix);
    }
    ddResult = lpD3DDevice->lpVtbl->CreateMatrix(lpD3DDevice, &matrixProj);
    ddResult = lpD3DDevice->lpVtbl->SetMatrix(lpD3DDevice, matrixProj, &projMatrix);
    ddResult = lpD3DDevice->lpVtbl->CreateMatrix(lpD3DDevice, &matrixWorld);
    ddResult = lpD3DDevice->lpVtbl->SetMatrix(lpD3DDevice, matrixWorld, &worldMatrix);
    sourceVertexCount = 0;
    quadVertexCount = 0;
    profileStart = GetProfileTime();
    for (blendNo = 0; blendNo < 2; blendNo++) {
        for (currentTexture = 0; currentTexture <= normalSetNo; currentTexture++) {
            triBuckets[currentTexture][blendNo].emitted = 0;
            triBuckets[currentTexture][blendNo].count = 0;
        }
    }
    for (currentTexture = 0; currentTexture <= normalSetNo; currentTexture++) {
        quadBuckets[currentTexture].count = 0;
    }
    for (queueObjectIndex = 0; queueObjectIndex < renderCount; queueObjectIndex++) {
        /* The original records this vertex count without reading it. */
        startRenderVertexCount = renderVertexCount;

        base = sourceVertexCount;
        if ((renderQueue[queueObjectIndex].triLists && renderQueue[queueObjectIndex].vertexCount) || renderQueue[queueObjectIndex].quadCount) {
            if (renderQueue[queueObjectIndex].quadCount) {
                BuildQuads(renderQueue[queueObjectIndex].quadCount, renderQueue[queueObjectIndex].quads, renderQueue[queueObjectIndex].matrix);
            }
            for (sourceIndex = 0; sourceIndex < renderQueue[queueObjectIndex].vertexCount; sourceIndex++) {
                *vertexCursor = renderQueue[queueObjectIndex].vertices + sourceIndex;
                *ownerIndices = queueObjectIndex;
                vertexCursor++;
                ownerIndices++;
                sourceVertexCount++;
            }
            quadEndIndex = quadVertexCount + renderQueue[queueObjectIndex].quadCount * 4;
            for (sourceIndex = quadVertexCount; sourceIndex < quadEndIndex; sourceIndex++) {
                *vertexCursor = quadVertices + sourceIndex;
                *ownerIndices = queueObjectIndex;
                vertexCursor++;
                ownerIndices++;
                sourceVertexCount++;
            }
            quadVertexCount = quadEndIndex;
            mode = renderQueue[queueObjectIndex].mode;
            for (setIndex = 0; setIndex < normalSetNo + 1; setIndex++) {
                if (typeSwitchesMode && renderQueue[queueObjectIndex].mode == 0 && normalSetNo > setIndex) {
                    if (textureSets[setIndex].type == 3) {
                        if (mode == 0) {
                            mode = 1;
                        }
                    } else if (mode == 1) {
                        mode = 0;
                    }
                }
                sourceCount = 0;
                if (renderQueue[queueObjectIndex].triLists) {
                    sourceCount = renderQueue[queueObjectIndex].triLists[setIndex].count;
                }
                if (quadBuckets[setIndex].count || sourceCount) {
                    m = 0;
                    if (mode) {
                        m = 1;
                    }
                    currentBucket = &triBuckets[setIndex][m];
                    if (sourceCount) {
                        src = renderQueue[queueObjectIndex].triLists[setIndex].tris;
                        for (triIndex = 0; triIndex < sourceCount; triIndex++) {
                            index = currentBucket->count;
                            dst = &currentBucket->tris[index];
                            dst->v1 = src->v1 + base;
                            dst->v2 = src->v2 + base;
                            dst->v3 = src->v3 + base;
                            dst->flags = src->flags;
                            currentBucket->count++;
                            src++;
                            renderTriTotal++;
                        }
                    }
                    if (!rgbTextures) {
                        m = 1;
                    }
                    currentBucket = &triBuckets[setIndex][m];
                    src = quadBuckets[setIndex].triangles;
                    for (triIndex = 0; triIndex < quadBuckets[setIndex].count; triIndex++) {
                        index = currentBucket->count;
                        dst = &currentBucket->tris[index];
                        dst->v1 = renderQueue[queueObjectIndex].vertexCount + src->v1 + base;
                        dst->v2 = renderQueue[queueObjectIndex].vertexCount + src->v2 + base;
                        dst->v3 = renderQueue[queueObjectIndex].vertexCount + src->v3 + base;
                        dst->flags = src->flags;
                        currentBucket->count++;
                        src++;
                        renderTriTotal++;
                    }
                }
            }
            if (renderQueue[queueObjectIndex].quadCount) {
                for (currentTexture = 0; currentTexture <= normalSetNo; currentTexture++) {
                    quadBuckets[currentTexture].count = 0;
                }
            }
        }
    }
    profileQueueTime = GetProfileTime() - profileStart;
    profileStart = GetProfileTime();
    StartTransformBuffer();
    startBufferTime.seconds = GetProfileTime() - profileStart;
    renderVertexCount = 0;
    profileStart = GetProfileTime();
    for (blendNo = 0; blendNo < 2; blendNo++) {
        if (blendByBucket) {
            if (blendNo != 0) {
                OP_STATE_RENDER(1, renderInstruction);
                STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction);
            } else {
                OP_STATE_RENDER(1, renderInstruction);
                STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 0, renderInstruction);
            }
        }
        for (currentTexture = 0; currentTexture <= normalSetNo; currentTexture++) {
            currentBucket = &triBuckets[currentTexture][blendNo];
            if (currentBucket->count) {
                if (!blendByBucket) {
                    if (blendNo != 0) {
                        OP_STATE_RENDER(3, renderInstruction);
                        STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction);
                        if (gameFlags & 2) {
                            STATE_DATA(D3DRENDERSTATE_DESTBLEND, 2, renderInstruction);
                            STATE_DATA(D3DRENDERSTATE_SRCBLEND, 2, renderInstruction);
                        } else {
                            STATE_DATA(D3DRENDERSTATE_DESTBLEND, 6, renderInstruction);
                            STATE_DATA(D3DRENDERSTATE_SRCBLEND, 5, renderInstruction);
                        }
                    } else {
                        if (textureSets[currentTexture].type == 3) {
                            OP_STATE_RENDER(3, renderInstruction);
                            STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction);
                            STATE_DATA(D3DRENDERSTATE_DESTBLEND, 6, renderInstruction);
                            STATE_DATA(D3DRENDERSTATE_SRCBLEND, 5, renderInstruction);
                        } else {
                            OP_STATE_RENDER(1, renderInstruction);
                            STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 0, renderInstruction);
                        }
                    }
                }
                OP_STATE_RENDER(1, renderInstruction);
                if (normalSetNo > currentTexture) {
                    if (nameDrawFirst) {
                        STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, textureHandles[currentTexture], renderInstruction);
                    } else {
                        STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, textureHandles[currentTexture], renderInstruction);
                    }
                } else {
                    STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, 0, renderInstruction);
                }
            }
            if (currentBucket->count) {
                while (currentBucket->count - currentBucket->emitted) {
                    remaining = currentBucket->count - currentBucket->emitted;
                    firstTri = currentBucket->emitted;
                    triangleListPtr = renderInstruction;
                    OP_TRIANGLE_LIST(remaining, renderInstruction);
                    done = remaining;
                    for (triIndex = 0; triIndex < remaining && renderVertexCount <= 1015; triIndex++) {
                        currentVertex = currentBucket->tris[firstTri + triIndex].v1;
                        queueObjectIndex = sourceOwner[currentVertex];
                        if (vertexIndices[currentVertex] == -1) {
                            D3DMATRIX matrix;
                            float z;
                            float y;
                            float x;
                            D3DLVERTEX *output;
                            index1 = renderVertexCount++;
                            ((D3DLVERTEX *)vertexData)[index1] = *sourceVertices[currentVertex];
                            matrix = *(D3DMATRIX *)renderQueue[queueObjectIndex].matrix;
                            output = (D3DLVERTEX *)vertexData + index1;
                            x = output->x;
                            y = output->y;
                            z = output->z;
                            output->x = x * matrix.m[0][0] + z * matrix.m[2][0] + y * matrix.m[1][0] + matrix.m[3][0];
                            output->y = x * matrix.m[0][1] + z * matrix.m[2][1] + y * matrix.m[1][1] + matrix.m[3][1];
                            output->z = z * matrix.m[2][2] + y * matrix.m[1][2] + x * matrix.m[0][2] + matrix.m[3][2];
                            if (renderQueue[queueObjectIndex].hasColour) {
                                rgb = (unsigned char *)&((D3DLVERTEX *)vertexData)[index1].color;
                                rgb[2] = renderQueue[queueObjectIndex].r;
                                rgb[1] = renderQueue[queueObjectIndex].g;
                                rgb[0] = renderQueue[queueObjectIndex].b;
                            }
                            vertexIndices[currentVertex] = index1;
                        } else {
                            index1 = vertexIndices[currentVertex];
                        }
                        currentVertex = currentBucket->tris[firstTri + triIndex].v2;
                        if (vertexIndices[currentVertex] == -1) {
                            float x;
                            D3DMATRIX matrix;
                            float y;
                            float z;
                            D3DLVERTEX *output;
                            secondSlot = renderVertexCount++;
                            ((D3DLVERTEX *)vertexData)[secondSlot] = *sourceVertices[currentVertex];
                            matrix = *(D3DMATRIX *)renderQueue[queueObjectIndex].matrix;
                            output = (D3DLVERTEX *)vertexData + secondSlot;
                            x = output->x;
                            y = output->y;
                            z = output->z;
                            output->x = z * matrix.m[2][0] + y * matrix.m[1][0] + x * matrix.m[0][0] + matrix.m[3][0];
                            output->y = z * matrix.m[2][1] + y * matrix.m[1][1] + x * matrix.m[0][1] + matrix.m[3][1];
                            output->z = z * matrix.m[2][2] + y * matrix.m[1][2] + x * matrix.m[0][2] + matrix.m[3][2];
                            if (renderQueue[queueObjectIndex].hasColour) {
                                rgb = (unsigned char *)&((D3DLVERTEX *)vertexData)[secondSlot].color;
                                rgb[2] = renderQueue[queueObjectIndex].r;
                                rgb[1] = renderQueue[queueObjectIndex].g;
                                rgb[0] = renderQueue[queueObjectIndex].b;
                            }
                            vertexIndices[currentVertex] = secondSlot;
                        } else {
                            secondSlot = vertexIndices[currentVertex];
                        }
                        currentVertex = currentBucket->tris[firstTri + triIndex].v3;
                        if (vertexIndices[currentVertex] == -1) {
                            D3DMATRIX matrix;
                            D3DLVERTEX *output;
                            float x;
                            float y;
                            float z;
                            i3 = renderVertexCount++;
                            ((D3DLVERTEX *)vertexData)[i3] = *sourceVertices[currentVertex];
                            matrix = *(D3DMATRIX *)renderQueue[queueObjectIndex].matrix;
                            output = (D3DLVERTEX *)vertexData + i3;
                            x = output->x;
                            y = output->y;
                            z = output->z;
                            output->x = z * matrix.m[2][0] + y * matrix.m[1][0] + x * matrix.m[0][0] + matrix.m[3][0];
                            output->y = z * matrix.m[2][1] + y * matrix.m[1][1] + x * matrix.m[0][1] + matrix.m[3][1];
                            output->z = x * matrix.m[0][2] + z * matrix.m[2][2] + y * matrix.m[1][2] + matrix.m[3][2];
                            if (renderQueue[queueObjectIndex].hasColour) {
                                rgb = (unsigned char *)&((D3DLVERTEX *)vertexData)[i3].color;
                                rgb[2] = renderQueue[queueObjectIndex].r;
                                rgb[1] = renderQueue[queueObjectIndex].g;
                                rgb[0] = renderQueue[queueObjectIndex].b;
                            }
                            vertexIndices[currentVertex] = i3;
                        } else {
                            i3 = vertexIndices[currentVertex];
                        }
                        ((D3DTRIANGLE *)renderInstruction)->v1 = index1;
                        ((D3DTRIANGLE *)renderInstruction)->v2 = secondSlot;
                        ((D3DTRIANGLE *)renderInstruction)->v3 = i3;
                        ((D3DTRIANGLE *)renderInstruction)->flags = 0x700;
                        renderInstruction += sizeof(D3DTRIANGLE);
                        currentBucket->emitted++;
                    }
                    if (triIndex != remaining) {
                        OP_TRIANGLE_LIST(triIndex, triangleListPtr);
                        done = triIndex;
                        FinishTransformBuffer();
                        StartTransformBuffer();
                        OP_STATE_RENDER(1, renderInstruction);
                        if (normalSetNo > currentTexture) {
                            if (nameDrawFirst) {
                                STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, textureHandles[currentTexture], renderInstruction);
                            } else {
                                STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, textureHandles[currentTexture], renderInstruction);
                            }
                        } else {
                            STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, 0, renderInstruction);
                        }
                        if (!blendByBucket) {
                            if (blendNo != 0) {
                                OP_STATE_RENDER(3, renderInstruction);
                                STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction);
                                if (gameFlags & 2) {
                                    STATE_DATA(D3DRENDERSTATE_DESTBLEND, 2, renderInstruction);
                                    STATE_DATA(D3DRENDERSTATE_SRCBLEND, 2, renderInstruction);
                                } else {
                                    STATE_DATA(D3DRENDERSTATE_DESTBLEND, 6, renderInstruction);
                                    STATE_DATA(D3DRENDERSTATE_SRCBLEND, 5, renderInstruction);
                                }
                            } else {
                                OP_STATE_RENDER(1, renderInstruction);
                                STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction);
                                if (textureSets[currentTexture].type == 3) {
                                    OP_STATE_RENDER(3, renderInstruction);
                                    STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction);
                                    STATE_DATA(D3DRENDERSTATE_DESTBLEND, 6, renderInstruction);
                                    STATE_DATA(D3DRENDERSTATE_SRCBLEND, 5, renderInstruction);
                                } else {
                                    OP_STATE_RENDER(1, renderInstruction);
                                    STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 0, renderInstruction);
                                }
                            }
                        } else if (blendNo != 0) {
                            OP_STATE_RENDER(1, renderInstruction);
                            STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction);
                        } else {
                            OP_STATE_RENDER(1, renderInstruction);
                            STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 0, renderInstruction);
                        }
                    }
                    drawTotal += done;
                }
            }
        }
    }
    FinishTransformBuffer();
    profileDrawTime = GetProfileTime() - profileStart;
}
#pragma function(memcpy)
#endif
#endif
#ifdef EQUIV
#ifndef NETPLAY
void DrawRenderQueue(void)
{
    D3DEXECUTEBUFFERDESC desc;
    double startTime;
    D3DLVERTEX *q;
    int v1, v2, v3, owner;
    unsigned char *block;
    TriBucket *cur;
    int s1, s2, s3;
    int n, j, k, m, mode, sets;
    int count, done, emitted, start;
    int vertexIndex;
    unsigned short *triangleIndexCursor;
    D3DTRIANGLE *dst;
    int base, drawn;
    TriBucket *bucket;
    D3DLVERTEX **vp;
    int *op;
    D3DTRIANGLE *src;
    float ty, tz;
    /* The triangle total is dead before the third transform uses its storage. */
    D3DMATRIX firstMatrix;
    D3DMATRIX secondMatrix;
    union {
        int triTotal;
        D3DMATRIX matrix;
    } thirdTransform;

    sourceVertexCount = 7000;
    startBufferTime.seconds = 0;
    thirdTransform.triTotal = 0;
    drawn = 0;
    vp = sourceVertices;
    op = sourceOwner;
    if (fadeAmount > 180 && !fadeSkipped) {
        fadeSkipped = 1;
        renderFrame = 0;
    } else {
        fadeSkipped = 0;
        renderFrame = 1;
    }
    memcpy(&desc, &bufferDescriptionTemplate, sizeof(desc));
    ddResult = lpD3DDevice->lpVtbl->CreateMatrix(lpD3DDevice, &matrixView);
    if (flipView) {
        ddResult = lpD3DDevice->lpVtbl->SetMatrix(lpD3DDevice, matrixView, &viewMatrixFlipped);
    } else {
        ddResult = lpD3DDevice->lpVtbl->SetMatrix(lpD3DDevice, matrixView, &viewMatrix);
    }
    ddResult = lpD3DDevice->lpVtbl->CreateMatrix(lpD3DDevice, &matrixProj);
    ddResult = lpD3DDevice->lpVtbl->SetMatrix(lpD3DDevice, matrixProj, &projMatrix);
    ddResult = lpD3DDevice->lpVtbl->CreateMatrix(lpD3DDevice, &matrixWorld);
    ddResult = lpD3DDevice->lpVtbl->SetMatrix(lpD3DDevice, matrixWorld, &worldMatrix);
    sourceVertexCount = 0;
    quadVertexCount = 0;
    startTime = GetProfileTime();
    for (bucket = triBuckets; bucket < triBuckets + 2; bucket++) {
        cur = bucket;
        j = normalSetNo + 1;
        do {
            cur->emitted = 0;
            cur->count = 0;
            cur += 2;
        } while (--j);
    }
    {
        TrackBucket *qb = quadBuckets;
        j = normalSetNo + 1;
        do {
            qb->count = 0;
            qb++;
        } while (--j);
    }
    for (n = 0; n < renderCount; n++) {
        int quadVerts;
        unsigned int quadVerticesAdded;

        base = sourceVertexCount;
        if (!((renderQueue[n].triLists && renderQueue[n].vertexCount) || renderQueue[n].quadCount)) {
            continue;
        }
        if (renderQueue[n].quadCount) {
            BuildQuads(renderQueue[n].quadCount, renderQueue[n].quads, renderQueue[n].matrix);
        }
        /* the vertex's index is read before the owner store and the counter is written after it */
        for (k = 0; k < renderQueue[n].vertexCount; k++) {
            *vp = renderQueue[n].vertices + k;
            vp++;
            vertexIndex = sourceVertexCount;
            *op = n;
            op++;
            sourceVertexCount = vertexIndex + 1;
        }
        quadVerts = quadVertexCount + renderQueue[n].quadCount * 4;
        quadVerticesAdded = (unsigned int)quadVerts - quadVertexCount;
        if (quadVertexCount < quadVerts) {
            q = &quadVertices[quadVertexCount];
            for (k = quadVertexCount; k < quadVerts; k++) {
                *op++ = n;
            }
            sourceVertexCount += quadVerticesAdded;
            do {
                *vp++ = q++;
            } while (--quadVerticesAdded);
        }
        quadVertexCount = quadVerts;
        sets = normalSetNo + 1;
        mode = renderQueue[n].mode;
        for (j = 0; j < sets; j++) {
            TriList *list = renderQueue[n].triLists;

            if (typeSwitchesMode && renderQueue[n].mode == 0 && j < sets - 1) {
                if (textureSets[j].type == 3) {
                    if (mode == 0)
                        mode = 1;
                } else if (mode == 1) {
                    mode = 0;
                }
            }
            count = 0;
            if (list)
                count = list[j].count;
            if (quadBuckets[j].count || count) {
                m = 0;
                if (mode)
                    m = 1;
                cur = &triBuckets[j * 2 + m];
                if (count) {
                    src = list[j].tris;
                    if (count > 0) {
                        thirdTransform.triTotal += count;
                        do {
                            dst = &cur->tris[cur->count];
                            dst->v1 = src->v1 + base;
                            dst->v2 = src->v2 + base;
                            dst->v3 = src->v3 + base;
                            dst->flags = src->flags;
                            cur->count++;
                            src++;
                        } while (--count);
                    }
                }
                if (!rgbTextures)
                    m = 1;
                cur = &triBuckets[j * 2 + m];
                for (k = 0; k < quadBuckets[j].count; k++) {
                    src = &quadBuckets[j].triangles[k];
                    dst = &cur->tris[cur->count];
                    dst->v1 = renderQueue[n].vertexCount + src->v1 + base;
                    dst->v2 = renderQueue[n].vertexCount + src->v2 + base;
                    dst->v3 = renderQueue[n].vertexCount + src->v3 + base;
                    dst->flags = src->flags;
                    /* The total is updated from the stored bucket count, after the store. */
                    count = cur->count;
                    cur->count = count + 1;
                    thirdTransform.triTotal += cur->count - count;
                }
            }
        }
        if (renderQueue[n].quadCount) {
            TrackBucket *qb = quadBuckets;
            j = sets;
            do {
                qb->count = 0;
                qb++;
            } while (--j);
        }
    }
    profileQueueTime = GetProfileTime() - startTime;
    startTime = GetProfileTime();
    StartTransformBuffer();
    startBufferTime.seconds = GetProfileTime() - startTime;
    renderVertexCount = 0;
    startTime = GetProfileTime();
    bucket = triBuckets;
    do {
        if (blendByBucket) {
            if (bucket != triBuckets) {
                OP_STATE_RENDER(1, renderInstruction);
                STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction);
            } else {
                OP_STATE_RENDER(1, renderInstruction);
                STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 0, renderInstruction);
            }
        }
        j = 0;
        do {
            cur = bucket + j * 2;
            if (cur->count) {
                if (!blendByBucket) {
                    SET_BUCKET_BLEND(bucket == triBuckets, j);
                }
                SET_TEXTURE(j);
            }
            if (cur->count && cur->emitted != cur->count) {
                do {
                    start = cur->emitted;
                    count = cur->count - start;
                    block = renderInstruction;
                    OP_TRIANGLE_LIST(count, renderInstruction);
                    done = count;
                    emitted = 0;
                    if (count > 0) {
                        triangleIndexCursor = (unsigned short *)&cur->tris[start];
                        do {
                            if (renderVertexCount > 1015)
                                break;
                            v1 = triangleIndexCursor[0];
                            owner = sourceOwner[v1];
                            ADD_VERTEX(v1, s1, firstMatrix);
                            v2 = triangleIndexCursor[1];
                            ADD_VERTEX(v2, s2, secondMatrix);
                            v3 = triangleIndexCursor[2];
                            s3 = vertexIndices[v3];
                            if (s3 == -1) {
                                D3DLVERTEX *dst;
                                float x, y, z, tx, ty, tz;
                                float xFromX, yFromX, zFromX;
                                s3 = renderVertexCount++;
                                dst = (D3DLVERTEX *)vertexData + s3;
                                *dst = *sourceVertices[v3];
                                thirdTransform.matrix = *(D3DMATRIX *)renderQueue[owner].matrix;
                                x = dst->x;
                                y = dst->y;
                                z = dst->z;
                                /* The third transform adds x and z before y. */
                                xFromX = x * thirdTransform.matrix.m[0][0];
                                yFromX = x * thirdTransform.matrix.m[0][1];
                                zFromX = x * thirdTransform.matrix.m[0][2];
                                tx = xFromX + z * thirdTransform.matrix.m[2][0]
                                   + y * thirdTransform.matrix.m[1][0] + thirdTransform.matrix.m[3][0];
                                ty = yFromX + z * thirdTransform.matrix.m[2][1]
                                   + y * thirdTransform.matrix.m[1][1] + thirdTransform.matrix.m[3][1];
                                tz = zFromX + z * thirdTransform.matrix.m[2][2]
                                   + y * thirdTransform.matrix.m[1][2] + thirdTransform.matrix.m[3][2];
                                dst->x = tx;
                                dst->y = ty;
                                dst->z = tz;
                                if (renderQueue[owner].hasColour) {
                                    unsigned char *rgb = (unsigned char *)&dst->color;
                                    rgb[2] = renderQueue[owner].r;
                                    rgb[1] = renderQueue[owner].g;
                                    rgb[0] = renderQueue[owner].b;
                                }
                                vertexIndices[v3] = s3;
                            }
                            ((D3DTRIANGLE *)renderInstruction)->v1 = s1;
                            ((D3DTRIANGLE *)renderInstruction)->v2 = s2;
                            ((D3DTRIANGLE *)renderInstruction)->v3 = s3;
                            ((D3DTRIANGLE *)renderInstruction)->flags = 0x700;
                            renderInstruction += sizeof(D3DTRIANGLE);
                            triangleIndexCursor += 4;
                            emitted++;
                            cur->emitted++;
                        } while (emitted < count);
                    }
                    if (emitted != count) {
                        register unsigned short textureLimit;
                        ((D3DINSTRUCTION *)block)->count = emitted;
                        ((D3DINSTRUCTION *)block)->opcode = D3DOP_TRIANGLE;
                        ((D3DINSTRUCTION *)block)->size = sizeof(D3DTRIANGLE);
                        done = emitted;
                        FinishTransformBuffer();
                        StartTransformBuffer();
                        SET_TEXTURE_RESTART(j);
                        if (!blendByBucket) {
                            if (bucket != triBuckets) {
                                OP_STATE_RENDER(3, renderInstruction);
                                STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction);
                                if (gameFlags & 2) {
                                    STATE_DATA(D3DRENDERSTATE_DESTBLEND, 2, renderInstruction);
                                    STATE_DATA(D3DRENDERSTATE_SRCBLEND, 2, renderInstruction);
                                } else {
                                    STATE_DATA(D3DRENDERSTATE_DESTBLEND, 6, renderInstruction);
                                    STATE_DATA(D3DRENDERSTATE_SRCBLEND, 5, renderInstruction);
                                }
                            } else {
                                OP_STATE_RENDER(1, renderInstruction);
                                STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction);
                                if (textureSets[j].type == 3) {
                                    OP_STATE_RENDER(3, renderInstruction);
                                    STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction);
                                    STATE_DATA(D3DRENDERSTATE_DESTBLEND, 6, renderInstruction);
                                    STATE_DATA(D3DRENDERSTATE_SRCBLEND, 5, renderInstruction);
                                } else {
                                    OP_STATE_RENDER(1, renderInstruction);
                                    STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 0, renderInstruction);
                                }
                            }
                        } else if (bucket != triBuckets) {
                            OP_STATE_RENDER(1, renderInstruction);
                            STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 1, renderInstruction);
                        } else {
                            OP_STATE_RENDER(1, renderInstruction);
                            STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, 0, renderInstruction);
                        }
                    }
                    drawn += done;
                } while (cur->count != cur->emitted);
            }
            j++;
        } while (j <= normalSetNo);
        bucket++;
    } while (bucket < triBuckets + 2);
    FinishTransformBuffer();
    profileDrawTime = GetProfileTime() - startTime;
}
#endif
#endif

/* Locks the current execute buffer and writes the state setup for transformed (non-TL) geometry.
   Equivalent: the exe keeps bufferSize in ecx and schedules the buffer pointers differently. */
#ifdef EQUIV
void StartTransformBuffer(void)
{
    unsigned long bufferSize;
#ifndef NETPLAY
    ConfigData *config;
#endif
    bufferDescription = bufferDescriptionTemplate;
    ddResult = executeBuffers[renderBufferIndex]->methods->Lock(executeBuffers[renderBufferIndex], &bufferDescription);
    renderVertexCount = 0;
    memset(vertexIndices, -1, sourceVertexCount * sizeof(int));
    bufferSize = vertexBufferSize;
    instructionBuffer = (unsigned char *)bufferDescription.data + (long)(bufferSize * 0.75f);
    renderInstruction = instructionBuffer;
    vertexData = bufferDescription.data;
    OP_STATE_TRANSFORM(3, renderInstruction);
    STATE_DATA(D3DTRANSFORMSTATE_VIEW, matrixView, renderInstruction);
    STATE_DATA(D3DTRANSFORMSTATE_PROJECTION, matrixProj, renderInstruction);
    STATE_DATA(D3DTRANSFORMSTATE_WORLD, matrixWorld, renderInstruction);
    OP_STATE_RENDER(7, renderInstruction);
    STATE_DATA(D3DRENDERSTATE_TEXTUREPERSPECTIVE, 1, renderInstruction);
    STATE_DATA(D3DRENDERSTATE_SPECULARENABLE, 0, renderInstruction);
    STATE_DATA(D3DRENDERSTATE_DESTBLEND, 2, renderInstruction);
#ifndef NETPLAY
    config = gameData;
#endif
    STATE_DATA(D3DRENDERSTATE_SRCBLEND, 2, renderInstruction);
#ifdef NETPLAY
    if (gameData->smoothTexturesSigned) {
#else
    if (config->smoothTextures) {
#endif
        STATE_DATA(D3DRENDERSTATE_TEXTUREMAG, 2, renderInstruction);
        STATE_DATA(D3DRENDERSTATE_TEXTUREMIN, 2, renderInstruction);
    } else {
        STATE_DATA(D3DRENDERSTATE_TEXTUREMAG, 1, renderInstruction);
        STATE_DATA(D3DRENDERSTATE_TEXTUREMIN, 1, renderInstruction);
    }
    if (rgbTextures) {
        STATE_DATA(D3DRENDERSTATE_TEXTUREMAPBLEND, 2, renderInstruction);
    } else {
        STATE_DATA(D3DRENDERSTATE_TEXTUREMAPBLEND, 7, renderInstruction);
    }
    OP_STATE_RENDER(3, renderInstruction);
    STATE_DATA(D3DRENDERSTATE_CULLMODE, 1, renderInstruction);
    STATE_DATA(D3DRENDERSTATE_ZENABLE, 1, renderInstruction);
    STATE_DATA(D3DRENDERSTATE_ZWRITEENABLE, 1, renderInstruction);
    processInstruction = renderInstruction;
    OP_PROCESS_VERTICES(1, renderInstruction);
    PROCESSVERTICES_DATA(D3DPROCESSVERTICES_TRANSFORMLIGHT, 0, 0, renderInstruction);
}
#endif

/* Applies the screen fade to the queued vertices (or to the palette), then closes and unlocks the execute buffer.
   Equivalent: the exe keeps the palette fade locals in other registers, and the instruction-stream
   stores at the end are written in the exe's order (loads of the buffer index, buffer and methods
   pointers sit between them). */
#ifdef EQUIV
#ifdef NETPLAY
void FinishTransformBuffer(void)
{
    float fadeScale;
    int blueChannel;
    int paletteIndex;
    int vertexLoop;
    int redValue;
    float peakColor;
    int green;

    if (fadeActive) {
        if (rgbTextures) {
            for (vertexLoop = 0; vertexLoop < renderVertexCount; vertexLoop++) {
                redValue = ((u_char *)&((D3DLVERTEX *)vertexData)[vertexLoop].color)[2] + fadeRed + fadeAmount;
                green = ((u_char *)&((D3DLVERTEX *)vertexData)[vertexLoop].color)[1] + fadeGreen + cheatfade + fadeAmount;
                blueChannel = ((u_char *)&((D3DLVERTEX *)vertexData)[vertexLoop].color)[0] + fadeBlue + fadeAmount;
                peakColor = green > redValue ? green : redValue;
                peakColor = (peakColor < blueChannel) ? blueChannel : peakColor;
                if (peakColor > 255.0) {
                    fadeScale = 255.0f / peakColor;
                    redValue = (int)(redValue * fadeScale);
                    green = (int)(green * fadeScale);
                    blueChannel = (int)(blueChannel * fadeScale);
                }
                if (redValue > 255)
                    redValue = 255;
                else if (redValue < 0)
                    redValue = 0;
                if (green > 255)
                    green = 255;
                else if (green < 0)
                    green = 0;
                if (blueChannel > 255)
                    blueChannel = 255;
                else if (blueChannel < 0)
                    blueChannel = 0;
                ((u_char *)&((D3DLVERTEX *)vertexData)[vertexLoop].color)[2] = redValue;
                ((u_char *)&((D3DLVERTEX *)vertexData)[vertexLoop].color)[1] = green;
                ((u_char *)&((D3DLVERTEX *)vertexData)[vertexLoop].color)[0] = blueChannel;
            }
        } else {
            for (paletteIndex = 0; paletteIndex < 256; paletteIndex++) {
                redValue = savedPalette[paletteIndex].peRed + (fadeRed / 8 + fadeAmount) / 4;
                green = (fadeGreen / 8 + cheatfade + fadeAmount) / 4 + savedPalette[paletteIndex].peGreen;
                blueChannel = savedPalette[paletteIndex].peBlue + (fadeBlue / 8 + fadeAmount) / 4;
                peakColor = green > redValue ? green : redValue;
                peakColor = (peakColor < blueChannel) ? blueChannel : peakColor;
                if (peakColor > 255.0) {
                    fadeScale = 255.0f / peakColor;
                    redValue = (int)(redValue * fadeScale);
                    green = (int)(green * fadeScale);
                    blueChannel = (int)(blueChannel * fadeScale);
                }
                if (redValue > 255)
                    redValue = 255;
                else if (redValue < 0)
                    redValue = 0;
                if (green > 255)
                    green = 255;
                else if (green < 0)
                    green = 0;
                if (blueChannel > 255)
                    blueChannel = 255;
                else if (blueChannel < 0)
                    blueChannel = 0;
                savedPalette[paletteIndex].peRed = redValue;
                savedPalette[paletteIndex].peGreen = green;
                savedPalette[paletteIndex].peBlue = blueChannel;
            }
            paletteDirty = 2;
        }
    }
    *(u_char *)renderInstruction = D3DOP_EXIT;
    *((u_char *)renderInstruction + 1) = 0;
    *((u_short *)renderInstruction + 1) = 0;
    renderInstruction += sizeof(D3DINSTRUCTION);
    *(u_char *)processInstruction = D3DOP_PROCESSVERTICES;
    *((u_char *)processInstruction + 1) = sizeof(D3DPROCESSVERTICES);
    *((u_short *)processInstruction + 1) = 1;
    processInstruction += sizeof(D3DINSTRUCTION);
    *(u_int *)processInstruction = D3DPROCESSVERTICES_TRANSFORMLIGHT;
    *((u_short *)processInstruction + 2) = 0;
    *((u_short *)processInstruction + 3) = 0;
    *((int *)processInstruction + 2) = renderVertexCount;
    *((int *)processInstruction + 3) = 0;
    processInstruction += sizeof(D3DPROCESSVERTICES);
    ddResult = executeBuffers[renderBufferIndex]->methods->Unlock(executeBuffers[renderBufferIndex]);
    if (renderFrame) {
        memset(&executeData, 0, sizeof(executeData));
        executeData.size = sizeof(executeData);
        executeData.vertexCount = renderVertexCount;
        executeData.instructionOffset = (long)(vertexBufferSize * 0.75f);
        executeData.instructionLength = renderInstruction - instructionBuffer;
        if (maxInstructionLength < executeData.instructionLength)
            maxInstructionLength = executeData.instructionLength;
        ddResult = executeBuffers[renderBufferIndex]->methods->SetExecuteData(executeBuffers[renderBufferIndex], &executeData);
        renderBufferState[renderBufferIndex] = 1;
        if (renderVertexCount != 0)
            renderBufferIndex++;
    }
}
#else
void FinishTransformBuffer(void)
{
    int amount;
    int red, green, blue;
    int r, g, b;
    int maxRedGreen;
    int addBlue, addGreen, addRed;
    float blueFloat;
    float peak;
    int i;
    int vertexCount, bufferIndex;
    RenderExecuteBuffer *buffer;
    RenderExecuteBufferMethods *methods;
    u_char *p;

    if (fadeActive) {
        if (rgbTextures) {
            if (renderVertexCount > 0) {
                red = fadeRed;
                green = fadeGreen;
                blue = fadeBlue;
                p = (u_char *)&((D3DLVERTEX *)vertexData)->color + 2;
                i = renderVertexCount;
                amount = fadeAmount;
                peak = 0;
                blueFloat = 0;
                for (; i > 0; i--) {
                    r = p[0] + red + amount;
                    g = p[-1] + cheatfade + green + amount;
                    b = p[-2] + blue + amount;
                    maxRedGreen = (g > r) ? g : r;
                    blueFloat = b;
                    peak = (blueFloat < maxRedGreen) ? maxRedGreen : blueFloat;
                    if (peak > 255.0) {
                        r = (int)(r * (255.0f / peak));
                        g = (int)(g * (255.0f / peak));
                        b = (int)(b * (255.0f / peak));
                    }
                    if (r > 255)
                        r = 255;
                    else if (r < 0)
                        r = 0;
                    if (g > 255)
                        g = 255;
                    else if (g < 0)
                        g = 0;
                    if (b > 255)
                        b = 255;
                    else if (b < 0)
                        b = 0;
                    p[0] = r;
                    p[-1] = g;
                    p[-2] = b;
                    p += sizeof(D3DLVERTEX);
                }
            }
        } else {
            amount = fadeAmount;
            addRed = ((short)(fadeRed / 8) + amount) / 4;
            addGreen = ((short)(fadeGreen / 8) + cheatfade + amount) / 4;
            addBlue = ((short)(fadeBlue / 8) + amount) / 4;
            peak = 0;
            blueFloat = 0;
            for (p = (u_char *)savedPalette; p < (u_char *)(savedPalette + 256); p += sizeof(PALETTEENTRY)) {
                r = addRed + p[0];
                g = p[1] + addGreen;
                b = p[2] + addBlue;
                maxRedGreen = (g > r) ? g : r;
                blueFloat = b;
                peak = (blueFloat < maxRedGreen) ? maxRedGreen : blueFloat;
                if (peak > 255.0) {
                    r = (int)(r * (255.0f / peak));
                    g = (int)(g * (255.0f / peak));
                    b = (int)(b * (255.0f / peak));
                }
                if (r > 255)
                    r = 255;
                else if (r < 0)
                    r = 0;
                if (g > 255)
                    g = 255;
                else if (g < 0)
                    g = 0;
                if (b > 255)
                    b = 255;
                else if (b < 0)
                    b = 0;
                p[0] = r;
                p[1] = g;
                p[2] = b;
            }
            paletteDirty = 2;
        }
    }
    {
        unsigned char *ptr;
        D3DINSTRUCTION *instr;
        D3DPROCESSVERTICES *pv;

        /* OP_EXIT, with processInstruction read after its opcode store as in the exe */
        ((D3DINSTRUCTION *)renderInstruction)->opcode = D3DOP_EXIT;
        ptr = processInstruction;
        ((D3DINSTRUCTION *)renderInstruction)->size = 0;
        ((D3DINSTRUCTION *)renderInstruction)->count = 0;
        renderInstruction += sizeof(D3DINSTRUCTION);

        instr = (D3DINSTRUCTION *)ptr;
        instr->opcode = D3DOP_PROCESSVERTICES;
        instr++;
        pv = (D3DPROCESSVERTICES *)instr;
        pv->start = 0;
        vertexCount = renderVertexCount;
        pv->dest = 0;
        pv->count = vertexCount;
        pv->reserved = 0;
        bufferIndex = renderBufferIndex;
        instr[-1].size = sizeof(D3DPROCESSVERTICES);
        buffer = executeBuffers[bufferIndex];
        processInstruction = (unsigned char *)(pv + 1);
        instr[-1].count = 1;
        methods = buffer->methods;
        pv->flags = D3DPROCESSVERTICES_TRANSFORMLIGHT;
    }
    ddResult = methods->Unlock(buffer);
    if (renderFrame) {
        memset(&executeData, 0, sizeof(executeData));
        executeData.vertexCount = renderVertexCount;
        executeData.size = sizeof(executeData);
        executeData.instructionOffset = (long)(vertexBufferSize * 0.75f);
        executeData.instructionLength = renderInstruction - instructionBuffer;
        if (maxInstructionLength < executeData.instructionLength)
            maxInstructionLength = executeData.instructionLength;
        ddResult = executeBuffers[renderBufferIndex]->methods->SetExecuteData(executeBuffers[renderBufferIndex], &executeData);
        renderBufferState[renderBufferIndex] = 1;
        if (renderVertexCount)
            renderBufferIndex++;
    }
}
#endif
#endif

#ifdef NETPLAY
#define QUAD_COLOUR_UV(n) \
    ((unsigned char *)&quadVertices[startVertex].color)[3] = opaqueAlpha; \
    ((unsigned char *)&quadVertices[startVertex].color)[2] = spriteColour.r; \
    ((unsigned char *)&quadVertices[startVertex].color)[1] = spriteColour.g; \
    ((unsigned char *)&quadVertices[startVertex].color)[0] = spriteColour.b; \
    { \
        unsigned char textureU, textureV; \
        textureV = ((SpriteQuad *)quads)[quad].v##n; \
        textureU = ((SpriteQuad *)quads)[quad].u##n; \
        quadVertices[startVertex].tu = (unsigned long)textureU / 255.0f; \
        quadVertices[startVertex].tv = (unsigned long)textureV / 255.0f; \
    } \
    startVertex++
#else
#define QUAD_COLOUR_UV(n) \
    ((unsigned char *)&quadVertices[index].color)[3] = opaqueAlpha; \
    ((unsigned char *)&quadVertices[index].color)[2] = colour.r; \
    ((unsigned char *)&quadVertices[index].color)[1] = colour.g; \
    ((unsigned char *)&quadVertices[index].color)[0] = colour.b; \
    quadVertices[index].tu = (float)(unsigned long)quad->u##n * (1.0f / 255.0f); \
    quadVertices[index].tv = (float)(unsigned long)quad->v##n * (1.0f / 255.0f); \
    index++
#endif

/* Turns count billboard records into four vertices and two triangles each. */
#ifdef EQUIV
#ifdef NETPLAY
#pragma intrinsic(sqrt)
void BuildQuads(int count, void *quads, float *matrix)
{
    float cameraX;
    int quad;
    float vertexX;
    int startVertex = quadVertexCount;
    float xAxis;
    float textureHeight;
    float uScale;
    float vertexY;
    float spriteCosine;
    CVECTOR spriteColour;
    float cameraZ;
    float quadHeight;
    float cameraLocalZ;
    int quadIndex;
    float billboardWidth;
    float magnitude;
    float spriteRotationSine;
    SVECTOR center;
    float matrixZ;

    for (quad = 0; quad < count; quad++) {
        quadIndex = ((SpriteQuad *)quads)[quad].set;
        center = ((SpriteQuad *)quads)[quad].position;
        spriteColour = ((SpriteQuad *)quads)[quad].colour;
        billboardWidth = ((SpriteQuad *)quads)[quad].width;
        quadHeight = ((SpriteQuad *)quads)[quad].height;
        uScale = ((SpriteQuad *)quads)[quad].textureW;
        textureHeight = ((SpriteQuad *)quads)[quad].textureH;
        vertexX = ((SpriteQuad *)quads)[quad].position.vx / 4096.0f;
        vertexY = ((SpriteQuad *)quads)[quad].position.vy / 4096.0f;
        /* The first transform uses the depth before rounding it to float. */
        cameraX = (cameraLocalZ = ((SpriteQuad *)quads)[quad].position.vz / 4096.0f) * matrix[8]
                + vertexY * matrix[4] + vertexX * matrix[0] + matrix[12];
        cameraZ = cameraLocalZ * matrix[10] + vertexY * matrix[6] + vertexX * matrix[2] + matrix[14];
        magnitude = sqrt(cameraZ * cameraZ + cameraX * cameraX);
        cameraX /= magnitude;
        cameraZ /= magnitude;
        xAxis = matrix[0];
        matrixZ = matrix[2];
        magnitude = sqrt(matrixZ * matrixZ + xAxis * xAxis);
        xAxis /= magnitude;
        matrixZ = -(matrixZ / magnitude);
        spriteCosine = -cameraX * xAxis + matrixZ * cameraZ;
        spriteRotationSine = -cameraX * matrixZ - xAxis * cameraZ;
        quadBuckets[quadIndex].triangles[quadBuckets[quadIndex].count].v1 = startVertex - quadVertexCount;
        quadBuckets[quadIndex].triangles[quadBuckets[quadIndex].count].v2 = startVertex - quadVertexCount + 3;
        quadBuckets[quadIndex].triangles[quadBuckets[quadIndex].count].v3 = startVertex - quadVertexCount + 1;
        quadBuckets[quadIndex].triangles[quadBuckets[quadIndex].count].flags = 1;
        quadBuckets[quadIndex].count++;
        quadBuckets[quadIndex].triangles[quadBuckets[quadIndex].count].v1 = startVertex - quadVertexCount;
        quadBuckets[quadIndex].triangles[quadBuckets[quadIndex].count].v2 = startVertex - quadVertexCount + 2;
        quadBuckets[quadIndex].triangles[quadBuckets[quadIndex].count].v3 = startVertex - quadVertexCount + 3;
        quadBuckets[quadIndex].triangles[quadBuckets[quadIndex].count].flags = 0x1f;
        quadBuckets[quadIndex].count++;
        quadVertices[startVertex].x = (float)(billboardWidth * uScale * -2.0f / 256.0 * spriteRotationSine + center.vx) / 4096.0f;
        quadVertices[startVertex].y = (float)(quadHeight * textureHeight * -1.0f / 256.0 + center.vy) / 4096.0f;
        quadVertices[startVertex].z = (float)(billboardWidth * uScale * -2.0f / 256.0 * spriteCosine * -1.0 + center.vz) / 4096.0f;
        QUAD_COLOUR_UV(0);
        quadVertices[startVertex].x = (float)(billboardWidth * uScale * 2.0f / 256.0 * spriteRotationSine + center.vx) / 4096.0f;
        quadVertices[startVertex].y = (float)(quadHeight * textureHeight * -1.0f / 256.0 + center.vy) / 4096.0f;
        quadVertices[startVertex].z = (float)(billboardWidth * uScale * 2.0f / 256.0 * spriteCosine * -1.0 + center.vz) / 4096.0f;
        QUAD_COLOUR_UV(1);
        quadVertices[startVertex].x = (float)(billboardWidth * uScale * -2.0f / 256.0 * spriteRotationSine + center.vx) / 4096.0f;
        quadVertices[startVertex].y = (float)center.vy / 4096.0f;
        quadVertices[startVertex].z = (float)(billboardWidth * uScale * -2.0f / 256.0 * spriteCosine * -1.0 + center.vz) / 4096.0f;
        QUAD_COLOUR_UV(2);
        quadVertices[startVertex].x = (float)(billboardWidth * uScale * 2.0f / 256.0 * spriteRotationSine + center.vx) / 4096.0f;
        quadVertices[startVertex].y = (float)center.vy / 4096.0f;
        quadVertices[startVertex].z = (float)(billboardWidth * uScale * 2.0f / 256.0 * spriteCosine * -1.0 + center.vz) / 4096.0f;
        QUAD_COLOUR_UV(3);
    }
}

#endif
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* cx/cy/cz are the billboard centre as float, double, double, as spilled in D3D.
   Equivalent: the equivalence check assumes opaqueAlpha may alias the third vertex's tv store. */
void BuildQuads(int count, void *quads, float *matrix)
{
    SVECTOR position;
    CVECTOR colour;
    int set;
    short base;
    int index = quadVertexCount;
    float width, textureW, textureH;
    double height, cy, cz;
    float cx;
    SpriteQuad *quad = quads;
    float x = 0.0f;
    float y1, z;
    float px, py, pz;
    float matrixX, matrixZ, magnitude;
    float sine, cosine;
    float left, right;
    float x0, x1, y0, z0, z1;

    while (count > 0) {
        position = quad->position;
        set = quad->set;
        colour = quad->colour;
        width = quad->width;
        height = quad->height;
        textureW = quad->textureW;
        textureH = quad->textureH;
        px = quad->position.vx * (1.0f / 4096.0f);
        py = quad->position.vy * (1.0f / 4096.0f);
        pz = position.vz * (1.0f / 4096.0f);
        x = px * matrix[0] + pz * matrix[8] + py * matrix[4] + matrix[12];
        z = pz * matrix[10] + py * matrix[6] + px * matrix[2] + matrix[14];
        matrixZ = matrix[2];
        matrixX = matrix[0];
        base = index - quadVertexCount;
        quadBuckets[set].triangles[quadBuckets[set].count].v1 = base;
        quadBuckets[set].triangles[quadBuckets[set].count].v2 = base + 3;
        quadBuckets[set].triangles[quadBuckets[set].count].v3 = base + 1;
        quadBuckets[set].triangles[quadBuckets[set].count].flags = 1;
        quadBuckets[set].count++;
        quadBuckets[set].triangles[quadBuckets[set].count].v1 = base;
        quadBuckets[set].triangles[quadBuckets[set].count].v2 = base + 2;
        quadBuckets[set].triangles[quadBuckets[set].count].v3 = base + 3;
        quadBuckets[set].triangles[quadBuckets[set].count].flags = 0x1f;
        quadBuckets[set].count++;
        width *= textureW;
        magnitude = sqrt(x * x + z * z);
        x /= magnitude;
        z /= magnitude;
        magnitude = sqrt(matrixZ * matrixZ + matrixX * matrixX);
        matrixX /= magnitude;
        matrixZ = -(matrixZ / magnitude);
        cosine = matrixZ * z - matrixX * x;
        sine = -(matrixZ * x + matrixX * z);
        left = -2.0f * width * (1.0 / 256.0);
        height = -(height * textureH);
        height *= 1.0 / 256.0;
        cx = position.vx;
        x0 = (left * sine + cx) * (1.0f / 4096.0f);
        quadVertices[index].x = x0;
        cy = position.vy;
        y0 = (float)(height + cy) * (1.0f / 4096.0f);
        quadVertices[index].y = y0;
        cz = position.vz;
        z0 = (float)(cz - left * cosine) * (1.0f / 4096.0f);
        quadVertices[index].z = z0;
        QUAD_COLOUR_UV(0);
        right = 2.0f * width * (1.0 / 256.0);
        x1 = (right * sine + cx) * (1.0f / 4096.0f);
        quadVertices[index].x = x1;
        quadVertices[index].y = y0;
        z1 = (float)(cz - right * cosine) * (1.0f / 4096.0f);
        quadVertices[index].z = z1;
        QUAD_COLOUR_UV(1);
        quadVertices[index].x = x0;
        y1 = (float)cy * (1.0f / 4096.0f);
        quadVertices[index].y = y1;
        quadVertices[index].z = z0;
        QUAD_COLOUR_UV(2);
        quadVertices[index].x = x1;
        quadVertices[index].y = y1;
        quadVertices[index].z = z1;
        QUAD_COLOUR_UV(3);
        quad++;
        count--;
    }
}
#endif
#endif
