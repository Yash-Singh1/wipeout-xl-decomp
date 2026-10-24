#include "d3dpoly.h"
#include "d3d_opcodes.h"
#include <math.h>
#include <string.h>

#define TRIANGLE_LIMIT_QUAD 1497
#define TRIANGLE_LIMIT_SINGLE 1498
#define LAST_TEXTURE_BUCKET 26
#define LAST_RENDER_VERTEX_INDEX 1015
#define CLIP_POLYGON_VERTEX_CAPACITY 20
#define D3D_TRIANGLE_FLAGS 0x700
#ifdef NETPLAY
/* Network build: the texture-page copy in DrawScaledTexturedQuadDirect sits behind this switch; the empty else leaves the jump the network build has there. */
#define NET_TEXTURE_PAGE_STEP 1
#endif

/* Expands a line to a quad. PowerVR uses a fixed diagonal pixel offset.
   The loading screen's extra compatibility argument is ignored. */
void AddLinePrim(float z, LINE_F2 *line, ...)
{
#ifdef NETPLAY
    /* NET's frame has the members in this order (length lowest, base highest). */
    struct { float length; float dx; float dy; int base; } lineData;
#else
    struct { int base; float length; float dx; float dy; } lineData;
#endif

    lineData.base = currentBuffer->vertexCount;
    if (triangleCount < TRIANGLE_LIMIT_QUAD) {
#ifndef POWERVR
        lineData.dx = (float)line->x0 - (float)line->x1;
        lineData.dy = (float)line->y0 - (float)line->y1;
        lineData.length = sqrt(lineData.dx * lineData.dx + lineData.dy * lineData.dy);
        if (lineData.length != 0.0f) {
            lineData.dy /= lineData.length;
            lineData.dx = -(lineData.dx / lineData.length);
#endif
#ifdef POWERVR
            #ifdef NETPLAY
            SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], line->x0 + 1, line->y0 + 1, z, line->r0, line->g0, line->b0);
            #else
            SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], line->x0 + 1, line->y0 + 1, z, line->r0, line->g0, line->b0);
            #endif
#else
            #ifdef NETPLAY
            SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], line->x0 + lineData.dy, line->y0 + lineData.dx, z, line->r0, line->g0, line->b0);
            #else
            SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], line->x0 + lineData.dy, line->y0 + lineData.dx, z, line->r0, line->g0, line->b0);
            #endif
#endif
            #ifdef NETPLAY
            SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], line->x0, line->y0, z, line->r0, line->g0, line->b0);
            #else
            SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], line->x0, line->y0, z, line->r0, line->g0, line->b0);
            #endif
#ifdef POWERVR
            #ifdef NETPLAY
            SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], line->x1 + 1, line->y1 + 1, z, line->r0, line->g0, line->b0);
            #else
            SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], line->x1 + 1, line->y1 + 1, z, line->r0, line->g0, line->b0);
            #endif
#else
            #ifdef NETPLAY
            SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], line->x1 + lineData.dy, line->y1 + lineData.dx, z, line->r0, line->g0, line->b0);
            #else
            SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], line->x1 + lineData.dy, line->y1 + lineData.dx, z, line->r0, line->g0, line->b0);
            #endif
#endif
            #ifdef NETPLAY
            SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], line->x1, line->y1, z, line->r0, line->g0, line->b0);
            #else
            SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], line->x1, line->y1, z, line->r0, line->g0, line->b0);
            #endif
            currentBuffer->triangles[currentBuffer->triangleCount].blendMode = 0;
            currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = 0;
            currentBuffer->triangles[currentBuffer->triangleCount].textureSet = LAST_TEXTURE_BUCKET;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = lineData.base;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = lineData.base + 1;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = lineData.base + 3;
            currentBuffer->triangleCount++;
            currentBuffer->triangles[currentBuffer->triangleCount].blendMode = 0;
            currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = 0;
            currentBuffer->triangles[currentBuffer->triangleCount].textureSet = LAST_TEXTURE_BUCKET;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = lineData.base;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = lineData.base + 3;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = lineData.base + 2;
            currentBuffer->triangleCount++;
            triangleCount += 2;
#ifndef POWERVR
        }
#endif
    }
}

/* Adds a flat triangle to the active render buffer. */
void AddPolyPrim(float z, POLY_F3 *poly)
{
    if (triangleCount < TRIANGLE_LIMIT_SINGLE) {
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = 0;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = 0;
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = LAST_TEXTURE_BUCKET;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = currentBuffer->vertexCount;
#ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
#else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
#endif
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = currentBuffer->vertexCount;
#ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
#else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
#endif
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = currentBuffer->vertexCount;
#ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
#else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
#endif
        currentBuffer->triangleCount++;
        triangleCount++;
    }
}

/* Splits a flat quadrilateral into two render triangles. */
void DrawPolyF4(float z, POLY_F4 *poly)
{
    int base;

    base = currentBuffer->vertexCount;
    if (triangleCount < TRIANGLE_LIMIT_QUAD) {
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        #endif
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
        #endif
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
        #endif
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x3, poly->y3, z, poly->r0, poly->g0, poly->b0);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x3, poly->y3, z, poly->r0, poly->g0, poly->b0);
        #endif
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = 0;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = 0;
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = LAST_TEXTURE_BUCKET;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = base;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = base + 1;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = base + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 1;
        currentBuffer->triangleCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = 0;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = 0;
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = LAST_TEXTURE_BUCKET;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = base;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = base + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = base + 2;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 31;
        currentBuffer->triangleCount++;
        triangleCount += 2;
    }
}

/* Converts the PS1 texture-page code to a Direct3D texture-set index. */
static __inline int AdjustTexturePage(int page)
{
    if (page > 15 && page < 100) page -= 10;
    else if (page < 16) page -= 5;
    return page;
}

#ifndef NETPLAY
/* Submits a textured quad given in 320x240 layout coordinates, scaled to the screen. */
void DrawPolyFT4(float z, POLY_FT4 *poly, int blendMode)
{
    int tpage;
    float divisor;
    int base;

    tpage = AdjustTexturePage(poly->tpage);
    if (nameDrawFirst) divisor = 256.0f;
    else divisor = 255.999f;

    base = currentBuffer->vertexCount;
    if (triangleCount < TRIANGLE_LIMIT_QUAD) {
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u0 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v0 / divisor;
        currentBuffer->vertexCount++;
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u1 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v1 / divisor;
        currentBuffer->vertexCount++;
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u2 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v2 / divisor;
        currentBuffer->vertexCount++;
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x3, poly->y3, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u3 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v3 / divisor;
        currentBuffer->vertexCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[tpage];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = tpage;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = base;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = base + 1;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = base + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 1;
        currentBuffer->triangleCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[tpage];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = tpage;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = base;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = base + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = base + 2;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 31;
        currentBuffer->triangleCount++;
        triangleCount += 2;
    }
}
#else
/* NET: the frame holds the quad base, divisor, texture page and page argument in that order. */
/* Submits a textured quad given in 320x240 layout coordinates, scaled to the screen. */
void DrawPolyFT4(float z, POLY_FT4 *poly, int blendMode)
{
    int quadBase;
    float divisor;
    int texturePageNo;
    int page;

    /* AdjustTexturePage is expanded in place in NET, with its parameter kept in a local. */
    page = poly->tpage;
    if (page > 15 && page < 100) page -= 10;
    else if (page < 16) page -= 5;
    if (NET_TEXTURE_PAGE_STEP) texturePageNo = page; else { }
    if (nameDrawFirst) divisor = 256.0f;
    else divisor = 255.999f;

    quadBase = currentBuffer->vertexCount;
    if (triangleCount < TRIANGLE_LIMIT_QUAD) {
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u0 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v0 / divisor;
        currentBuffer->vertexCount++;
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u1 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v1 / divisor;
        currentBuffer->vertexCount++;
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u2 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v2 / divisor;
        currentBuffer->vertexCount++;
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x3, poly->y3, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u3 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v3 / divisor;
        currentBuffer->vertexCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[texturePageNo];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = texturePageNo;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = quadBase;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = quadBase + 1;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = quadBase + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 1;
        currentBuffer->triangleCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[texturePageNo];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = texturePageNo;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = quadBase;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = quadBase + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = quadBase + 2;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 31;
        currentBuffer->triangleCount++;
        triangleCount += 2;
    }
}
#endif

#ifndef NETPLAY
/* Submits a textured quad using integer pixel coordinates. */
void DrawPixelTexturedQuad(float z, POLY_FT4 *poly, int blendMode)
{
    int tpage;
    float divisor;
    int base;

    tpage = AdjustTexturePage(poly->tpage);
    if (nameDrawFirst) divisor = 256.0f;
    else divisor = 255.999f;

    base = currentBuffer->vertexCount;
    if (triangleCount < TRIANGLE_LIMIT_QUAD) {
        SetPixelScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        if (texture16Bit) {
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u0 / divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v0 / divisor;
        }
        currentBuffer->vertexCount++;
        SetPixelScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
        if (texture16Bit) {
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u1 / divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v1 / divisor;
        }
        currentBuffer->vertexCount++;
        SetPixelScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
        if (texture16Bit) {
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u2 / divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v2 / divisor;
        }
        currentBuffer->vertexCount++;
        SetPixelScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x3, poly->y3, z, poly->r0, poly->g0, poly->b0);
        if (texture16Bit) {
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u3 / divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v3 / divisor;
        }
        currentBuffer->vertexCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = 0;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[tpage];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = tpage;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = base;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = base + 1;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = base + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 1;
        currentBuffer->triangleCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = 0;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[tpage];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = tpage;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = base;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = base + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = base + 2;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 31;
        currentBuffer->triangleCount++;
        triangleCount += 2;
    }
}
#else
/* NET: the frame holds the quad base, divisor, texture page and page argument in that order. */
/* Submits a textured quad using integer pixel coordinates. */
void DrawPixelTexturedQuad(float z, POLY_FT4 *poly, int blendMode)
{
    int quadBase;
    float divisor;
    int texturePageNo;
    int page;

    /* AdjustTexturePage is expanded in place in NET, with its parameter kept in a local. */
    page = poly->tpage;
    if (page > 15 && page < 100) page -= 10;
    else if (page < 16) page -= 5;
    if (NET_TEXTURE_PAGE_STEP) texturePageNo = page; else { }
    if (nameDrawFirst) divisor = 256.0f;
    else divisor = 255.999f;

    quadBase = currentBuffer->vertexCount;
    if (triangleCount < TRIANGLE_LIMIT_QUAD) {
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        if (texture16Bit) {
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u0 / divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v0 / divisor;
        }
        currentBuffer->vertexCount++;
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
        if (texture16Bit) {
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u1 / divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v1 / divisor;
        }
        currentBuffer->vertexCount++;
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
        if (texture16Bit) {
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u2 / divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v2 / divisor;
        }
        currentBuffer->vertexCount++;
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x3, poly->y3, z, poly->r0, poly->g0, poly->b0);
        if (texture16Bit) {
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u3 / divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v3 / divisor;
        }
        currentBuffer->vertexCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = 0;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[texturePageNo];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = texturePageNo;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = quadBase;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = quadBase + 1;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = quadBase + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 1;
        currentBuffer->triangleCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = 0;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[texturePageNo];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = texturePageNo;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = quadBase;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = quadBase + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = quadBase + 2;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 31;
        currentBuffer->triangleCount++;
        triangleCount += 2;
    }
}
#endif

#ifndef NETPLAY
/* Draws a scaled textured quad, or blits it on the software-rendered path. */
void DrawScaledTexturedQuad(float z, POLY_FT4 *poly, int blendMode)
{
    struct {
        int uOffset, base, softwarePage;
        float divisor;
        int tpage, transparent;
    } quad;

    quad.transparent = 0;
    quad.uOffset = 0;

    quad.tpage = AdjustTexturePage(poly->tpage);
    if (nameDrawFirst) {
        quad.divisor = 256.0f;

        quad.base = currentBuffer->vertexCount;
        if (triangleCount < TRIANGLE_LIMIT_QUAD) {
            SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u0 / quad.divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v0 / quad.divisor;
            currentBuffer->vertexCount++;
            SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u1 / quad.divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v1 / quad.divisor;
            currentBuffer->vertexCount++;
            SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u2 / quad.divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v2 / quad.divisor;
            currentBuffer->vertexCount++;
            SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x3, poly->y3, z, poly->r0, poly->g0, poly->b0);
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u3 / quad.divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v3 / quad.divisor;
            currentBuffer->vertexCount++;
            currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
            currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[quad.tpage];
            currentBuffer->triangles[currentBuffer->triangleCount].textureSet = quad.tpage;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = quad.base;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = quad.base + 1;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = quad.base + 3;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 1;
            currentBuffer->triangleCount++;
            currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
            currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[quad.tpage];
            currentBuffer->triangles[currentBuffer->triangleCount].textureSet = quad.tpage;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = quad.base;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = quad.base + 3;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = quad.base + 2;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 31;
            currentBuffer->triangleCount++;
            triangleCount += 2;
        }
    } else {
        if (z > 0.8f) quad.transparent = 1;
        if (mmxAvailable) {
            switch (poly->tpage) {
            case 6: quad.softwarePage = 6; break;
            case 7: quad.softwarePage = 6; quad.uOffset = 256; break;
            case 8: quad.softwarePage = 6; quad.uOffset = 512; break;
            case 32: quad.softwarePage = 3; break;
            case 33: quad.uOffset = 256; quad.softwarePage = 3; break;
            case 34: quad.uOffset = 512; quad.softwarePage = 3; break;
            case 35: quad.softwarePage = 4; break;
            case 36: quad.softwarePage = 5; break;
            default: quad.softwarePage = 3; break;
            }
            AddBlit640(quad.transparent, poly->x0, poly->y0, poly->x3, poly->y3,
                        poly->u0 + quad.uOffset, poly->v0, poly->u3 + quad.uOffset, poly->v3, quad.softwarePage);
        } else {
            AddBlit640Shifted(quad.transparent, poly->x0, poly->y0, poly->x3, poly->y3,
                        poly->u0, poly->v0, poly->u3, poly->v3, poly->tpage);
        }
    }
}
#else
/* NET: the frame holds these locals in this order (names chosen for the frame layout). */
/* Draws a scaled textured quad, or blits it on the software-rendered path. */
void DrawScaledTexturedQuad(float z, POLY_FT4 *poly, int blendMode)
{
    int quadBase;
    int softTpage;
    float divisor;
    int isTransparent;
    int uOffset;
    int texturePageNo;
    int page;

    isTransparent = 0;
    uOffset = 0;

    /* AdjustTexturePage is expanded in place in NET, with its parameter kept in a local. */
    page = poly->tpage;
    if (page > 15 && page < 100) page -= 10;
    else if (page < 16) page -= 5;
    if (NET_TEXTURE_PAGE_STEP) texturePageNo = page; else { }
    if (nameDrawFirst) {
        divisor = 256.0f;

        quadBase = currentBuffer->vertexCount;
        if (triangleCount < TRIANGLE_LIMIT_QUAD) {
            SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u0 / divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v0 / divisor;
            currentBuffer->vertexCount++;
            SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u1 / divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v1 / divisor;
            currentBuffer->vertexCount++;
            SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u2 / divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v2 / divisor;
            currentBuffer->vertexCount++;
            SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x3, poly->y3, z, poly->r0, poly->g0, poly->b0);
            currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u3 / divisor;
            currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v3 / divisor;
            currentBuffer->vertexCount++;
            currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
            currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[texturePageNo];
            currentBuffer->triangles[currentBuffer->triangleCount].textureSet = texturePageNo;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = quadBase;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = quadBase + 1;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = quadBase + 3;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 1;
            currentBuffer->triangleCount++;
            currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
            currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[texturePageNo];
            currentBuffer->triangles[currentBuffer->triangleCount].textureSet = texturePageNo;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = quadBase;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = quadBase + 3;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = quadBase + 2;
            currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 31;
            currentBuffer->triangleCount++;
            triangleCount += 2;
        }
    } else {
        if (z > 0.8f) isTransparent = 1;
        if (mmxAvailable) {
            switch (poly->tpage) {
            case 6: softTpage = 6; break;
            case 7: softTpage = 6; uOffset = 256; break;
            case 8: softTpage = 6; uOffset = 512; break;
            case 32: softTpage = 3; break;
            case 33: uOffset = 256; softTpage = 3; break;
            case 34: uOffset = 512; softTpage = 3; break;
            case 35: softTpage = 4; break;
            case 36: softTpage = 5; break;
            default: softTpage = 3; break;
            }
            AddBlit640(isTransparent, poly->x0, poly->y0, poly->x3, poly->y3,
                        poly->u0 + uOffset, poly->v0, poly->u3 + uOffset, poly->v3, softTpage);
        } else {
            AddBlit640Shifted(isTransparent, poly->x0, poly->y0, poly->x3, poly->y3,
                        poly->u0, poly->v0, poly->u3, poly->v3, poly->tpage);
        }
    }
}
#endif

#ifndef NETPLAY
/* Submits a textured quad using scaled coordinates and 256-based UVs. */
void DrawScaledTexturedQuadDirect(float z, POLY_FT4 *poly, int blendMode)
{
    int tpage;
    float divisor;
    int base;

    tpage = AdjustTexturePage(poly->tpage);
    divisor = 256.0f;

    base = currentBuffer->vertexCount;
    if (triangleCount < TRIANGLE_LIMIT_QUAD) {
        SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u0 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v0 / divisor;
        currentBuffer->vertexCount++;
        SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u1 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v1 / divisor;
        currentBuffer->vertexCount++;
        SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u2 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v2 / divisor;
        currentBuffer->vertexCount++;
        SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x3, poly->y3, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u3 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v3 / divisor;
        currentBuffer->vertexCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[tpage];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = tpage;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = base;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = base + 1;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = base + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 1;
        currentBuffer->triangleCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[tpage];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = tpage;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = base;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = base + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = base + 2;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 31;
        currentBuffer->triangleCount++;
        triangleCount += 2;
    }
}
#else
/* NET: the frame holds the quad base, divisor, texture page and page argument in that order. */
void DrawScaledTexturedQuadDirect(float z, POLY_FT4 *poly, int blendMode)
{
    int quadBase;
    float divisor;
    int texturePageNo;
    int page;

    /* AdjustTexturePage is expanded in place in NET, with its parameter kept in a local. */
    page = poly->tpage;
    if (page > 15 && page < 100) page -= 10;
    else if (page < 16) page -= 5;
    if (NET_TEXTURE_PAGE_STEP) texturePageNo = page; else { }
    divisor = 256.0f;

    quadBase = currentBuffer->vertexCount;
    if (triangleCount < TRIANGLE_LIMIT_QUAD) {
        SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u0 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v0 / divisor;
        currentBuffer->vertexCount++;
        SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u1 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v1 / divisor;
        currentBuffer->vertexCount++;
        SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u2 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v2 / divisor;
        currentBuffer->vertexCount++;
        SetScaledScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x3, poly->y3, z, poly->r0, poly->g0, poly->b0);
        currentBuffer->vertices[currentBuffer->vertexCount].tu = (unsigned long)poly->u3 / divisor;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = (unsigned long)poly->v3 / divisor;
        currentBuffer->vertexCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[texturePageNo];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = texturePageNo;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = quadBase;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = quadBase + 1;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = quadBase + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 1;
        currentBuffer->triangleCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[texturePageNo];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = texturePageNo;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = quadBase;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = quadBase + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = quadBase + 2;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 31;
        currentBuffer->triangleCount++;
        triangleCount += 2;
    }
}
#endif

/* Submits a Gouraud-shaded quad with its leading vertices opaque. */
void DrawPolyG4(float z, POLY_G4 *poly, int blendMode)
{
    int base;

    base = currentBuffer->vertexCount;
    if (triangleCount < TRIANGLE_LIMIT_QUAD) {
        vertexAlpha = opaqueAlpha;
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        #endif
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x1, poly->y1, z, poly->r1, poly->g1, poly->b1);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x1, poly->y1, z, poly->r1, poly->g1, poly->b1);
        #endif
        vertexAlpha = translucentAlpha;
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x2, poly->y2, z, poly->r2, poly->g2, poly->b2);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x2, poly->y2, z, poly->r2, poly->g2, poly->b2);
        #endif
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x3, poly->y3, z, poly->r3, poly->g3, poly->b3);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x3, poly->y3, z, poly->r3, poly->g3, poly->b3);
        #endif
        vertexAlpha = opaqueAlpha;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = 0;
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = LAST_TEXTURE_BUCKET;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = base;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = base + 1;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = base + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 1;
        currentBuffer->triangleCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = 0;
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = LAST_TEXTURE_BUCKET;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = base;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = base + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = base + 2;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 31;
        currentBuffer->triangleCount++;
        triangleCount += 2;
    }
}

/* Submits a quad with flat texture coordinates and translucent lower vertices. */
void DrawQuadWithAlphaGradient(float z, POLY_FT4 *poly, int blendMode)
{
    int base;

    base = currentBuffer->vertexCount;
    if (triangleCount < TRIANGLE_LIMIT_QUAD) {
        vertexAlpha = opaqueAlpha;
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        #endif
        currentBuffer->vertices[currentBuffer->vertexCount].tu = 0.0f;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = 0.0f;
        currentBuffer->vertexCount++;
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x1, poly->y1, z, poly->r0, poly->g0, poly->b0);
        #endif
        currentBuffer->vertices[currentBuffer->vertexCount].tu = 0.0f;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = 0.0f;
        currentBuffer->vertexCount++;
        vertexAlpha = translucentAlpha;
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x2, poly->y2, z, poly->r0, poly->g0, poly->b0);
        #endif
        currentBuffer->vertices[currentBuffer->vertexCount].tu = 0.0f;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = 0.0f;
        currentBuffer->vertexCount++;
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x3, poly->y3, z, poly->r0, poly->g0, poly->b0);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount], poly->x3, poly->y3, z, poly->r0, poly->g0, poly->b0);
        #endif
        currentBuffer->vertices[currentBuffer->vertexCount].tu = 0.0f;
        currentBuffer->vertices[currentBuffer->vertexCount].tv = 0.0f;
        currentBuffer->vertexCount++;
        vertexAlpha = opaqueAlpha;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[normalSetNo];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = normalSetNo;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = base;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = base + 1;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = base + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 1;
        currentBuffer->triangleCount++;
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = textureHandles[normalSetNo];
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = normalSetNo;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = base;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = base + 3;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = base + 2;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.flags = 31;
        currentBuffer->triangleCount++;
        triangleCount += 2;
    }
}

/* Submits a Gouraud-shaded triangle with per-vertex alpha settings. */
void DrawPolyG3(float z, POLY_G3 *poly, int blendMode)
{
    if (triangleCount < TRIANGLE_LIMIT_SINGLE) {
        currentBuffer->triangles[currentBuffer->triangleCount].blendMode = blendMode;
        currentBuffer->triangles[currentBuffer->triangleCount].textureHandle = 0;
        currentBuffer->triangles[currentBuffer->triangleCount].textureSet = LAST_TEXTURE_BUCKET;
        vertexAlpha = translucentAlpha;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[0] = currentBuffer->vertexCount;
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x0, poly->y0, z, poly->r0, poly->g0, poly->b0);
        #endif
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[1] = currentBuffer->vertexCount;
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x1, poly->y1, z, poly->r1, poly->g1, poly->b1);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x1, poly->y1, z, poly->r1, poly->g1, poly->b1);
        #endif
        vertexAlpha = particleAlpha;
        currentBuffer->triangles[currentBuffer->triangleCount].indices.vertices[2] = currentBuffer->vertexCount;
        #ifdef NETPLAY
        SetScaledFloatScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x2, poly->y2, z, poly->r2, poly->g2, poly->b2);
        #else
        SetScreenVertex(&currentBuffer->vertices[currentBuffer->vertexCount++], poly->x2, poly->y2, z, poly->r2, poly->g2, poly->b2);
        #endif
        vertexAlpha = opaqueAlpha;
        currentBuffer->triangleCount++;
        triangleCount++;
    }
}

/* Buckets triangle index records by texture page and opacity pass. */
void SortRenderTriangles(void)
{
    int translucentCount = 0, opaqueCount = 0, totalCount = 0;
    int page, index;

    for (page = 0; page <= LAST_TEXTURE_BUCKET; page++) {
        triangleBuckets[page][0].emitted = 0;
        triangleBuckets[page][1].emitted = 0;
        triangleBuckets[page][0].count = 0;
        triangleBuckets[page][1].count = 0;
        for (index = 0; index < currentBuffer->triangleCount; index++) {
            if (currentBuffer->triangles[index].textureSet == page) {
                if (currentBuffer->triangles[index].blendMode) {
                    triangleBuckets[page][1].triangles[triangleBuckets[page][1].count] = &currentBuffer->triangles[index].indices;
                    triangleBuckets[page][1].count++;
                } else {
                    triangleBuckets[page][0].triangles[triangleBuckets[page][0].count] = &currentBuffer->triangles[index].indices;
                    triangleBuckets[page][0].count++;
                }
            }
        }
    }
}

/* The bucket address calculation uses the opposite operand order. */
#ifdef NETPLAY
void SubmitRenderTriangles(void)
{
    TriangleBucket *bucketRef;
    double startClock;
    RenderVertex *sourceVertexArray;
    int processedVertices;
    int ownerIndex;
    int triangleNumber;
    int pageCount;
    int *sourceOwnerTable;
    int firstTriangle;
    int renderInitialTrianglesAtStart;
    int trianglesSubmittedCount;
    int sourceVertexIndexNumber;
    int triangleCountNumber;
    unsigned char *instructionStartIndex;
    int renderVertex2Cursor;
    int blendModePassValue;
    int vertexIndex1Array;
    int renderPageNumber;
    int index0;

    renderInitialTrianglesAtStart = 0;
    sourceVertexCount = 7000;
    sourceVertexArray = clippedVertices;
    sourceOwnerTable = sourceOwner;
    if (currentBuffer->triangleCount != 0) ClipRenderBuffer();
    SortRenderTriangles();
    startBufferTime.words.low = 0;
    startBufferTime.words.high = 0;
    pageCount = 0;
    trianglesSubmittedCount = 0;
    /* The first timing measurement uses the original uninitialised timestamp. */
    profileQueueTime = GetProfileTime() - startClock;
    startClock = GetProfileTime();
    StartRenderBuffer();
    startBufferTime.seconds = GetProfileTime() - startClock;
    renderVertexCount = 0;
    startClock = GetProfileTime();
    for (blendModePassValue = 0; blendModePassValue < 2; blendModePassValue++) {
        if (blendModePassValue != 0) {
            ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
            ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
            ((RenderInstruction *)renderInstruction)->count = 1;
            renderInstruction += sizeof(RenderInstruction);
            ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
            ((RenderState *)renderInstruction)->argument = 1;
            renderInstruction += sizeof(RenderState);
        } else {
            ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
            ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
            ((RenderInstruction *)renderInstruction)->count = 1;
            renderInstruction += sizeof(RenderInstruction);
            ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
            ((RenderState *)renderInstruction)->argument = 0;
            renderInstruction += sizeof(RenderState);
        }
        for (renderPageNumber = 0; renderPageNumber <= LAST_TEXTURE_BUCKET; renderPageNumber++) {
            bucketRef = &triangleBuckets[renderPageNumber][blendModePassValue];
            if (bucketRef->count != 0) {
                if (!blendByBucket) {
                    if (blendModePassValue != 0) {
                        ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                        ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                        ((RenderInstruction *)renderInstruction)->count = 3;
                        renderInstruction += sizeof(RenderInstruction);
                        ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                        ((RenderState *)renderInstruction)->argument = 1;
                        renderInstruction += sizeof(RenderState);
                        if (gameFlags & 2) {
                            ((RenderState *)renderInstruction)->type = RENDER_DEST_BLEND;
                            ((RenderState *)renderInstruction)->argument = 2;
                            renderInstruction += sizeof(RenderState);
                            ((RenderState *)renderInstruction)->type = RENDER_SRC_BLEND;
                            ((RenderState *)renderInstruction)->argument = 2;
                            renderInstruction += sizeof(RenderState);
                        } else {
                            ((RenderState *)renderInstruction)->type = RENDER_DEST_BLEND;
                            ((RenderState *)renderInstruction)->argument = 6;
                            renderInstruction += sizeof(RenderState);
                            ((RenderState *)renderInstruction)->type = RENDER_SRC_BLEND;
                            ((RenderState *)renderInstruction)->argument = 5;
                            renderInstruction += sizeof(RenderState);
                        }
                    } else if (textureSets[renderPageNumber].type == 3) {
                        ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                        ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                        ((RenderInstruction *)renderInstruction)->count = 3;
                        renderInstruction += sizeof(RenderInstruction);
                        ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                        ((RenderState *)renderInstruction)->argument = 1;
                        renderInstruction += sizeof(RenderState);
                        ((RenderState *)renderInstruction)->type = RENDER_DEST_BLEND;
                        ((RenderState *)renderInstruction)->argument = 6;
                        renderInstruction += sizeof(RenderState);
                        ((RenderState *)renderInstruction)->type = RENDER_SRC_BLEND;
                        ((RenderState *)renderInstruction)->argument = 5;
                        renderInstruction += sizeof(RenderState);
                    } else {
                        ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                        ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                        ((RenderInstruction *)renderInstruction)->count = 1;
                        renderInstruction += sizeof(RenderInstruction);
                        ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                        ((RenderState *)renderInstruction)->argument = 0;
                        renderInstruction += sizeof(RenderState);
                    }
                }
                ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                ((RenderInstruction *)renderInstruction)->count = 1;
                renderInstruction += sizeof(RenderInstruction);
                if (renderPageNumber < LAST_TEXTURE_BUCKET) {
                    if (nameDrawFirst) {
                        ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                        ((RenderState *)renderInstruction)->argument = textureHandles[renderPageNumber];
                        renderInstruction += sizeof(RenderState);
                    } else {
                        ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                        ((RenderState *)renderInstruction)->argument = textureHandles[renderPageNumber];
                        renderInstruction += sizeof(RenderState);
                    }
                } else {
                    if (rgbTextures) {
                        ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                        ((RenderState *)renderInstruction)->argument = 0;
                        renderInstruction += sizeof(RenderState);
                    } else {
                        ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                        ((RenderState *)renderInstruction)->argument = textureHandles[11];
                        renderInstruction += sizeof(RenderState);
                    }
                }
            }
            if (bucketRef->count != 0) {
                while (bucketRef->count - bucketRef->emitted) {
                    if ((renderPageNumber == 23 || renderPageNumber == 24) && blendModePassValue == 1) triangleCountNumber = 1;
                    else triangleCountNumber = bucketRef->count - bucketRef->emitted;
                    firstTriangle = bucketRef->emitted;
                    instructionStartIndex = renderInstruction;
                    ((RenderInstruction *)renderInstruction)->opcode = D3DOP_TRIANGLE;
                    ((RenderInstruction *)renderInstruction)->size = sizeof(RenderTriangleInstruction);
                    ((RenderInstruction *)renderInstruction)->count = triangleCountNumber;
                    renderInstruction += sizeof(RenderInstruction);
                    processedVertices = triangleCountNumber;
                    for (triangleNumber = 0; triangleNumber < triangleCountNumber && renderVertexCount <= LAST_RENDER_VERTEX_INDEX; triangleNumber++) {
                        sourceVertexIndexNumber = bucketRef->triangles[firstTriangle + triangleNumber]->vertices[0];
                        ownerIndex = sourceOwner[sourceVertexIndexNumber];
                        if (vertexIndices[sourceVertexIndexNumber] == -1) {
                            index0 = renderVertexCount++;
                            renderVertices[index0] = currentBuffer->vertices[sourceVertexIndexNumber];
                            vertexIndices[sourceVertexIndexNumber] = index0;
                        } else {
                            index0 = vertexIndices[sourceVertexIndexNumber];
                        }
                        sourceVertexIndexNumber = bucketRef->triangles[firstTriangle + triangleNumber]->vertices[1];
                        if (vertexIndices[sourceVertexIndexNumber] == -1) {
                            vertexIndex1Array = renderVertexCount++;
                            renderVertices[vertexIndex1Array] = currentBuffer->vertices[sourceVertexIndexNumber];
                            vertexIndices[sourceVertexIndexNumber] = vertexIndex1Array;
                        } else {
                            vertexIndex1Array = vertexIndices[sourceVertexIndexNumber];
                        }
                        sourceVertexIndexNumber = bucketRef->triangles[firstTriangle + triangleNumber]->vertices[2];
                        if (vertexIndices[sourceVertexIndexNumber] == -1) {
                            renderVertex2Cursor = renderVertexCount++;
                            renderVertices[renderVertex2Cursor] = currentBuffer->vertices[sourceVertexIndexNumber];
                            vertexIndices[sourceVertexIndexNumber] = renderVertex2Cursor;
                        } else {
                            renderVertex2Cursor = vertexIndices[sourceVertexIndexNumber];
                        }
                        ((RenderTriangleInstruction *)renderInstruction)->vertex0 = index0;
                        ((RenderTriangleInstruction *)renderInstruction)->vertex1 = vertexIndex1Array;
                        ((RenderTriangleInstruction *)renderInstruction)->vertex2 = renderVertex2Cursor;
                        ((RenderTriangleInstruction *)renderInstruction)->flags = D3D_TRIANGLE_FLAGS;
                        renderInstruction += sizeof(RenderTriangleInstruction);
                        bucketRef->emitted++;
                    }
                    if (triangleNumber != triangleCountNumber) {
                        ((RenderInstruction *)instructionStartIndex)->opcode = D3DOP_TRIANGLE;
                        ((RenderInstruction *)instructionStartIndex)->size = sizeof(RenderTriangleInstruction);
                        ((RenderInstruction *)instructionStartIndex)->count = triangleNumber;
                        instructionStartIndex += sizeof(RenderInstruction);
                        processedVertices = triangleNumber;
                        FinishRenderBuffer();
                        StartRenderBuffer();
                        ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                        ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                        ((RenderInstruction *)renderInstruction)->count = 1;
                        renderInstruction += sizeof(RenderInstruction);
                        if (renderPageNumber < LAST_TEXTURE_BUCKET) {
                            if (nameDrawFirst) {
                                ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                                ((RenderState *)renderInstruction)->argument = textureHandles[renderPageNumber];
                                renderInstruction += sizeof(RenderState);
                            } else {
                                ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                                ((RenderState *)renderInstruction)->argument = textureHandles[renderPageNumber];
                                renderInstruction += sizeof(RenderState);
                            }
                        } else {
                            if (rgbTextures) {
                                ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                                ((RenderState *)renderInstruction)->argument = 0;
                                renderInstruction += sizeof(RenderState);
                            } else {
                                ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                                ((RenderState *)renderInstruction)->argument = textureHandles[11];
                                renderInstruction += sizeof(RenderState);
                            }
                        }
                        if (!blendByBucket) {
                            if (blendModePassValue != 0) {
                                ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                                ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                                ((RenderInstruction *)renderInstruction)->count = 3;
                                renderInstruction += sizeof(RenderInstruction);
                                ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                                ((RenderState *)renderInstruction)->argument = 1;
                                renderInstruction += sizeof(RenderState);
                                if (gameFlags & 2) {
                                    ((RenderState *)renderInstruction)->type = RENDER_DEST_BLEND;
                                    ((RenderState *)renderInstruction)->argument = 2;
                                    renderInstruction += sizeof(RenderState);
                                    ((RenderState *)renderInstruction)->type = RENDER_SRC_BLEND;
                                    ((RenderState *)renderInstruction)->argument = 2;
                                    renderInstruction += sizeof(RenderState);
                                } else {
                                    ((RenderState *)renderInstruction)->type = RENDER_DEST_BLEND;
                                    ((RenderState *)renderInstruction)->argument = 6;
                                    renderInstruction += sizeof(RenderState);
                                    ((RenderState *)renderInstruction)->type = RENDER_SRC_BLEND;
                                    ((RenderState *)renderInstruction)->argument = 5;
                                    renderInstruction += sizeof(RenderState);
                                }
                            } else if (textureSets[renderPageNumber].type == 3) {
                                ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                                ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                                ((RenderInstruction *)renderInstruction)->count = 3;
                                renderInstruction += sizeof(RenderInstruction);
                                ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                                ((RenderState *)renderInstruction)->argument = 1;
                                renderInstruction += sizeof(RenderState);
                                ((RenderState *)renderInstruction)->type = RENDER_DEST_BLEND;
                                ((RenderState *)renderInstruction)->argument = 6;
                                renderInstruction += sizeof(RenderState);
                                ((RenderState *)renderInstruction)->type = RENDER_SRC_BLEND;
                                ((RenderState *)renderInstruction)->argument = 5;
                                renderInstruction += sizeof(RenderState);
                            } else {
                                ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                                ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                                ((RenderInstruction *)renderInstruction)->count = 1;
                                renderInstruction += sizeof(RenderInstruction);
                                ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                                ((RenderState *)renderInstruction)->argument = 0;
                                renderInstruction += sizeof(RenderState);
                            }
                        } else {
                            if (blendModePassValue != 0) {
                                ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                                ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                                ((RenderInstruction *)renderInstruction)->count = 1;
                                renderInstruction += sizeof(RenderInstruction);
                                ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                                ((RenderState *)renderInstruction)->argument = 1;
                                renderInstruction += sizeof(RenderState);
                            } else {
                                ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                                ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                                ((RenderInstruction *)renderInstruction)->count = 1;
                                renderInstruction += sizeof(RenderInstruction);
                                ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                                ((RenderState *)renderInstruction)->argument = 0;
                                renderInstruction += sizeof(RenderState);
                            }
                        }
                        renderVertexCount = 0;
                    }
                    trianglesSubmittedCount += processedVertices;
                }
            }
        }
    }
    FinishRenderBuffer();
    profileDrawTime = GetProfileTime() - startClock;
    currentBuffer = &renderBuffer;
    currentBuffer->triangleCount = 0;
    currentBuffer->vertexCount = 0;
    triangleCount = 0;
}

#else
void SubmitRenderTriangles(void)
{
#pragma pack(push, 4)
    struct Submission {
        int vertex0;
        int unusedCount;
        int unusedSourceIndex;
        int *sourceMap;
        int unusedTriangles;
        double timestamp;
        int pass;
        TriangleBucket *bucket;
        int page;
        int processed;
        int count;
        int submitted;
        int sourceIndex;
        unsigned char *instruction;
        RenderVertex *sourceVertices;
        int triangle;
        int vertex2;
        int vertex1;
        int firstTriangle;
    } local;
#pragma pack(pop)

    local.unusedTriangles = 0;
    sourceVertexCount = 7000;
    local.sourceVertices = clippedVertices;
    local.sourceMap = sourceOwner;
    if (currentBuffer->triangleCount != 0) ClipRenderBuffer();
    SortRenderTriangles();
    startBufferTime.words.low = 0;
    startBufferTime.words.high = 0;
    local.unusedCount = 0;
    local.submitted = 0;
    /* The first timing measurement uses the original uninitialised timestamp. */
    profileQueueTime = GetProfileTime() - local.timestamp;
    local.timestamp = GetProfileTime();
    StartRenderBuffer();
    startBufferTime.seconds = GetProfileTime() - local.timestamp;
    renderVertexCount = 0;
    local.timestamp = GetProfileTime();
    for (local.pass = 0; local.pass < 2; local.pass++) {
        if (local.pass != 0) {
            ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
            ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
            ((RenderInstruction *)renderInstruction)->count = 1;
            renderInstruction += sizeof(RenderInstruction);
            ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
            ((RenderState *)renderInstruction)->argument = 1;
            renderInstruction += sizeof(RenderState);
        } else {
            ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
            ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
            ((RenderInstruction *)renderInstruction)->count = 1;
            renderInstruction += sizeof(RenderInstruction);
            ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
            ((RenderState *)renderInstruction)->argument = 0;
            renderInstruction += sizeof(RenderState);
        }
        for (local.page = 0; local.page <= LAST_TEXTURE_BUCKET; local.page++) {
            local.bucket = &triangleBuckets[local.page][local.pass];
            if (local.bucket->count != 0) {
                if (!blendByBucket) {
                    if (local.pass != 0) {
                        ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                        ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                        ((RenderInstruction *)renderInstruction)->count = 3;
                        renderInstruction += sizeof(RenderInstruction);
                        ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                        ((RenderState *)renderInstruction)->argument = 1;
                        renderInstruction += sizeof(RenderState);
                        if (gameFlags & 2) {
                            ((RenderState *)renderInstruction)->type = RENDER_DEST_BLEND;
                            ((RenderState *)renderInstruction)->argument = 2;
                            renderInstruction += sizeof(RenderState);
                            ((RenderState *)renderInstruction)->type = RENDER_SRC_BLEND;
                            ((RenderState *)renderInstruction)->argument = 2;
                            renderInstruction += sizeof(RenderState);
                        } else {
                            ((RenderState *)renderInstruction)->type = RENDER_DEST_BLEND;
                            ((RenderState *)renderInstruction)->argument = 6;
                            renderInstruction += sizeof(RenderState);
                            ((RenderState *)renderInstruction)->type = RENDER_SRC_BLEND;
                            ((RenderState *)renderInstruction)->argument = 5;
                            renderInstruction += sizeof(RenderState);
                        }
                    } else if (textureSets[local.page].type == 3) {
                        ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                        ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                        ((RenderInstruction *)renderInstruction)->count = 3;
                        renderInstruction += sizeof(RenderInstruction);
                        ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                        ((RenderState *)renderInstruction)->argument = 1;
                        renderInstruction += sizeof(RenderState);
                        ((RenderState *)renderInstruction)->type = RENDER_DEST_BLEND;
                        ((RenderState *)renderInstruction)->argument = 6;
                        renderInstruction += sizeof(RenderState);
                        ((RenderState *)renderInstruction)->type = RENDER_SRC_BLEND;
                        ((RenderState *)renderInstruction)->argument = 5;
                        renderInstruction += sizeof(RenderState);
                    } else {
                        ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                        ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                        ((RenderInstruction *)renderInstruction)->count = 1;
                        renderInstruction += sizeof(RenderInstruction);
                        ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                        ((RenderState *)renderInstruction)->argument = 0;
                        renderInstruction += sizeof(RenderState);
                    }
                }
                ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                ((RenderInstruction *)renderInstruction)->count = 1;
                renderInstruction += sizeof(RenderInstruction);
                if (local.page < LAST_TEXTURE_BUCKET) {
                    if (nameDrawFirst) {
                        ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                        ((RenderState *)renderInstruction)->argument = textureHandles[local.page];
                        renderInstruction += sizeof(RenderState);
                    } else {
                        ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                        ((RenderState *)renderInstruction)->argument = textureHandles[local.page];
                        renderInstruction += sizeof(RenderState);
                    }
                } else {
                    if (rgbTextures) {
                        ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                        ((RenderState *)renderInstruction)->argument = 0;
                        renderInstruction += sizeof(RenderState);
                    } else {
                        ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                        ((RenderState *)renderInstruction)->argument = textureHandles[11];
                        renderInstruction += sizeof(RenderState);
                    }
                }
            }
            if (local.bucket->count != 0) {
                while (local.bucket->count - local.bucket->emitted) {
                    if ((local.page == 23 || local.page == 24) && local.pass == 1) local.count = 1;
                    else local.count = local.bucket->count - local.bucket->emitted;
                    local.firstTriangle = local.bucket->emitted;
                    local.instruction = renderInstruction;
                    ((RenderInstruction *)renderInstruction)->opcode = D3DOP_TRIANGLE;
                    ((RenderInstruction *)renderInstruction)->size = sizeof(RenderTriangleInstruction);
                    ((RenderInstruction *)renderInstruction)->count = local.count;
                    renderInstruction += sizeof(RenderInstruction);
                    local.processed = local.count;
                    for (local.triangle = 0; local.triangle < local.count && renderVertexCount <= LAST_RENDER_VERTEX_INDEX; local.triangle++) {
                        local.sourceIndex = local.bucket->triangles[local.firstTriangle + local.triangle]->vertices[0];
                        local.unusedSourceIndex = sourceOwner[local.sourceIndex];
                        if (vertexIndices[local.sourceIndex] == -1) {
                            local.vertex0 = renderVertexCount++;
                            renderVertices[local.vertex0] = currentBuffer->vertices[local.sourceIndex];
                            vertexIndices[local.sourceIndex] = local.vertex0;
                        } else {
                            local.vertex0 = vertexIndices[local.sourceIndex];
                        }
                        local.sourceIndex = local.bucket->triangles[local.firstTriangle + local.triangle]->vertices[1];
                        if (vertexIndices[local.sourceIndex] == -1) {
                            local.vertex1 = renderVertexCount++;
                            renderVertices[local.vertex1] = currentBuffer->vertices[local.sourceIndex];
                            vertexIndices[local.sourceIndex] = local.vertex1;
                        } else {
                            local.vertex1 = vertexIndices[local.sourceIndex];
                        }
                        local.sourceIndex = local.bucket->triangles[local.firstTriangle + local.triangle]->vertices[2];
                        if (vertexIndices[local.sourceIndex] == -1) {
                            local.vertex2 = renderVertexCount++;
                            renderVertices[local.vertex2] = currentBuffer->vertices[local.sourceIndex];
                            vertexIndices[local.sourceIndex] = local.vertex2;
                        } else {
                            local.vertex2 = vertexIndices[local.sourceIndex];
                        }
                        ((RenderTriangleInstruction *)renderInstruction)->vertex0 = local.vertex0;
                        ((RenderTriangleInstruction *)renderInstruction)->vertex1 = local.vertex1;
                        ((RenderTriangleInstruction *)renderInstruction)->vertex2 = local.vertex2;
                        ((RenderTriangleInstruction *)renderInstruction)->flags = D3D_TRIANGLE_FLAGS;
                        renderInstruction += sizeof(RenderTriangleInstruction);
                        local.bucket->emitted++;
                    }
                    if (local.triangle != local.count) {
                        ((RenderInstruction *)local.instruction)->opcode = D3DOP_TRIANGLE;
                        ((RenderInstruction *)local.instruction)->size = sizeof(RenderTriangleInstruction);
                        ((RenderInstruction *)local.instruction)->count = local.triangle;
                        local.instruction += sizeof(RenderInstruction);
                        local.processed = local.triangle;
                        FinishRenderBuffer();
                        StartRenderBuffer();
                        ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                        ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                        ((RenderInstruction *)renderInstruction)->count = 1;
                        renderInstruction += sizeof(RenderInstruction);
                        if (local.page < LAST_TEXTURE_BUCKET) {
                            if (nameDrawFirst) {
                                ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                                ((RenderState *)renderInstruction)->argument = textureHandles[local.page];
                                renderInstruction += sizeof(RenderState);
                            } else {
                                ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                                ((RenderState *)renderInstruction)->argument = textureHandles[local.page];
                                renderInstruction += sizeof(RenderState);
                            }
                        } else {
                            if (rgbTextures) {
                                ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                                ((RenderState *)renderInstruction)->argument = 0;
                                renderInstruction += sizeof(RenderState);
                            } else {
                                ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
                                ((RenderState *)renderInstruction)->argument = textureHandles[11];
                                renderInstruction += sizeof(RenderState);
                            }
                        }
                        if (!blendByBucket) {
                            if (local.pass != 0) {
                                ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                                ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                                ((RenderInstruction *)renderInstruction)->count = 3;
                                renderInstruction += sizeof(RenderInstruction);
                                ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                                ((RenderState *)renderInstruction)->argument = 1;
                                renderInstruction += sizeof(RenderState);
                                if (gameFlags & 2) {
                                    ((RenderState *)renderInstruction)->type = RENDER_DEST_BLEND;
                                    ((RenderState *)renderInstruction)->argument = 2;
                                    renderInstruction += sizeof(RenderState);
                                    ((RenderState *)renderInstruction)->type = RENDER_SRC_BLEND;
                                    ((RenderState *)renderInstruction)->argument = 2;
                                    renderInstruction += sizeof(RenderState);
                                } else {
                                    ((RenderState *)renderInstruction)->type = RENDER_DEST_BLEND;
                                    ((RenderState *)renderInstruction)->argument = 6;
                                    renderInstruction += sizeof(RenderState);
                                    ((RenderState *)renderInstruction)->type = RENDER_SRC_BLEND;
                                    ((RenderState *)renderInstruction)->argument = 5;
                                    renderInstruction += sizeof(RenderState);
                                }
                            } else if (textureSets[local.page].type == 3) {
                                ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                                ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                                ((RenderInstruction *)renderInstruction)->count = 3;
                                renderInstruction += sizeof(RenderInstruction);
                                ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                                ((RenderState *)renderInstruction)->argument = 1;
                                renderInstruction += sizeof(RenderState);
                                ((RenderState *)renderInstruction)->type = RENDER_DEST_BLEND;
                                ((RenderState *)renderInstruction)->argument = 6;
                                renderInstruction += sizeof(RenderState);
                                ((RenderState *)renderInstruction)->type = RENDER_SRC_BLEND;
                                ((RenderState *)renderInstruction)->argument = 5;
                                renderInstruction += sizeof(RenderState);
                            } else {
                                ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                                ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                                ((RenderInstruction *)renderInstruction)->count = 1;
                                renderInstruction += sizeof(RenderInstruction);
                                ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                                ((RenderState *)renderInstruction)->argument = 0;
                                renderInstruction += sizeof(RenderState);
                            }
                        } else {
                            if (local.pass != 0) {
                                ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                                ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                                ((RenderInstruction *)renderInstruction)->count = 1;
                                renderInstruction += sizeof(RenderInstruction);
                                ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                                ((RenderState *)renderInstruction)->argument = 1;
                                renderInstruction += sizeof(RenderState);
                            } else {
                                ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
                                ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
                                ((RenderInstruction *)renderInstruction)->count = 1;
                                renderInstruction += sizeof(RenderInstruction);
                                ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
                                ((RenderState *)renderInstruction)->argument = 0;
                                renderInstruction += sizeof(RenderState);
                            }
                        }
                        renderVertexCount = 0;
                    }
                    local.submitted += local.processed;
                }
            }
        }
    }
    FinishRenderBuffer();
    profileDrawTime = GetProfileTime() - local.timestamp;
    currentBuffer = &renderBuffer;
    currentBuffer->triangleCount = 0;
    currentBuffer->vertexCount = 0;
    triangleCount = 0;
}

#endif


/* Locks a Direct3D execute buffer and writes its initial render state. */
void StartRenderBuffer(void)
{
    bufferDescription = bufferDescriptionTemplate;
    ddResult = executeBuffers[renderBufferIndex]->methods->Lock(executeBuffers[renderBufferIndex], &bufferDescription);
    renderVertexCount = 0;
    memset(vertexIndices, -1, sourceVertexCount * sizeof(int));
    instructionBuffer = (unsigned char *)bufferDescription.data + (long)(vertexBufferSize * processVertexOffsetScale);
    renderInstruction = instructionBuffer;
    renderVertices = bufferDescription.data;
    ((RenderInstruction *)renderInstruction)->opcode = D3DOP_STATERENDER;
    ((RenderInstruction *)renderInstruction)->size = sizeof(RenderState);
#ifdef POWERVR
    ((RenderInstruction *)renderInstruction)->count = 7;
#else
    ((RenderInstruction *)renderInstruction)->count = 10;
#endif
    renderInstruction += sizeof(RenderInstruction);
    ((RenderState *)renderInstruction)->type = RENDER_CULL_MODE;
    ((RenderState *)renderInstruction)->argument = 1;
    renderInstruction += sizeof(RenderState);
    ((RenderState *)renderInstruction)->type = RENDER_ALPHA_BLEND_ENABLE;
    ((RenderState *)renderInstruction)->argument = 0;
    renderInstruction += sizeof(RenderState);
    ((RenderState *)renderInstruction)->type = RENDER_DEST_BLEND;
#ifdef POWERVR
    ((RenderState *)renderInstruction)->argument = 6;
#else
    ((RenderState *)renderInstruction)->argument = 2;
#endif
    renderInstruction += sizeof(RenderState);
    ((RenderState *)renderInstruction)->type = RENDER_SRC_BLEND;
#ifdef POWERVR
    ((RenderState *)renderInstruction)->argument = 5;
#else
    ((RenderState *)renderInstruction)->argument = 2;
#endif
    renderInstruction += sizeof(RenderState);
    ((RenderState *)renderInstruction)->type = RENDER_Z_ENABLE;
    ((RenderState *)renderInstruction)->argument = 1;
    renderInstruction += sizeof(RenderState);
    ((RenderState *)renderInstruction)->type = RENDER_Z_WRITE_ENABLE;
    ((RenderState *)renderInstruction)->argument = 1;
    renderInstruction += sizeof(RenderState);
    ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_HANDLE;
    ((RenderState *)renderInstruction)->argument = 0;
    renderInstruction += sizeof(RenderState);
#ifndef POWERVR
    ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_MAG;
    ((RenderState *)renderInstruction)->argument = 1;
    renderInstruction += sizeof(RenderState);
    ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_MIN;
    ((RenderState *)renderInstruction)->argument = 1;
    renderInstruction += sizeof(RenderState);
    if (rgbTextures) {
        ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_BLEND;
        ((RenderState *)renderInstruction)->argument = 2;
        renderInstruction += sizeof(RenderState);
    } else {
        ((RenderState *)renderInstruction)->type = RENDER_TEXTURE_BLEND;
        ((RenderState *)renderInstruction)->argument = 7;
        renderInstruction += sizeof(RenderState);
    }
#endif
    processInstruction = renderInstruction;
    ((RenderInstruction *)renderInstruction)->opcode = D3DOP_PROCESSVERTICES;
    ((RenderInstruction *)renderInstruction)->size = sizeof(ProcessRenderVertices);
    ((RenderInstruction *)renderInstruction)->count = 1;
    renderInstruction += sizeof(RenderInstruction);
    ((ProcessRenderVertices *)renderInstruction)->flags = 2;
    ((ProcessRenderVertices *)renderInstruction)->start = 0;
    ((ProcessRenderVertices *)renderInstruction)->destination = 0;
    ((ProcessRenderVertices *)renderInstruction)->count = 0;
    ((ProcessRenderVertices *)renderInstruction)->reserved = 0;
    renderInstruction += sizeof(ProcessRenderVertices);
}

/* Terminates and unlocks the execute buffer, recording its submitted ranges. */
void FinishRenderBuffer(void)
{
    ((RenderInstruction *)renderInstruction)->opcode = D3DOP_EXIT;
    ((RenderInstruction *)renderInstruction)->size = 0;
    ((RenderInstruction *)renderInstruction)->count = 0;
    renderInstruction += sizeof(RenderInstruction);
    ((RenderInstruction *)processInstruction)->opcode = D3DOP_PROCESSVERTICES;
    ((RenderInstruction *)processInstruction)->size = sizeof(ProcessRenderVertices);
    ((RenderInstruction *)processInstruction)->count = 1;
    processInstruction += sizeof(RenderInstruction);
    ((ProcessRenderVertices *)processInstruction)->flags = 2;
    ((ProcessRenderVertices *)processInstruction)->start = 0;
    ((ProcessRenderVertices *)processInstruction)->destination = 0;
    ((ProcessRenderVertices *)processInstruction)->count = renderVertexCount;
    ((ProcessRenderVertices *)processInstruction)->reserved = 0;
    processInstruction += sizeof(ProcessRenderVertices);
    ddResult = executeBuffers[renderBufferIndex]->methods->Unlock(executeBuffers[renderBufferIndex]);
    memset(&executeData, 0, sizeof(executeData));
    executeData.size = sizeof(executeData);
    executeData.vertexCount = renderVertexCount;
    executeData.instructionOffset = vertexBufferSize * processVertexOffsetScale;
    executeData.instructionLength = renderInstruction - instructionBuffer;
    if (executeData.instructionLength > maxInstructionLength) maxInstructionLength = executeData.instructionLength;
    ddResult = executeBuffers[renderBufferIndex]->methods->SetExecuteData(executeBuffers[renderBufferIndex], &executeData);
    renderBufferState[renderBufferIndex] = 2;
    if (renderVertexCount != 0) renderBufferIndex++;
}

#ifndef NETPLAY
/* Clips render triangles to the Direct3D viewport and triangulates the results. */
void ClipRenderBuffer(void)
{
    RenderVertex * inVertices;
    float z0;
    float v0;
    float rhw0;
    unsigned char edgeRed1;
    unsigned char blue2;
    unsigned char b1;
    short edgeCase;
    float height;
    short copyVertexIndex;
    float y2;
    float u2;
    float xMax;
    short edgeNo;
    float y1;
    float u1;
    unsigned char a2;
    int clipFirstIndex;
    short tempPlane;
    unsigned char startAlpha;
    short numOut;
    float ratio;
    short triangle;
    RenderVertex polyVerts[CLIP_POLYGON_VERTEX_CAPACITY];
    float x2;
    int tempTranslucency;
    float x1;
    unsigned char g;
    int nextIndex;
    short unusedVertexCountNumber;
    short unusedInitialTriangleCount;
    int fanVert;
    int polyTexPage;
    RenderBuffer * source;
    RenderVertex localScratch[CLIP_POLYGON_VERTEX_CAPACITY];
    RenderVertex * localOutput;
    unsigned char g2;
    short clippingRequired;
    unsigned char r;
    unsigned char edgeFirstGreen;
    unsigned char b;
    int tempType;
    unsigned char alpha;
    int unusedCount;
    short screenVertexCount;
    float z2;
    float v2;
    float rhw2;
    unsigned char r2;

    screenVertexCount = 0;
    edgeCase = 0;
    numOut = 0;
    xMax = screenScaleX;
    height = screenScaleY;
    clippingRequired = 1;
    unusedCount = 0;
    unusedVertexCountNumber = 0;
    unusedInitialTriangleCount = 0;
    source = &renderBuffer;

    clippingBuffer.triangleCount = 0;
    clippingBuffer.vertexCount = 0;
    for (triangle = 0; triangle < source->triangleCount; triangle++) {
        polyTexPage = source->triangles[triangle].blendMode;
        tempTranslucency = source->triangles[triangle].textureHandle;
        tempType = source->triangles[triangle].textureSet;
        clippingRequired = 1;
        if (source->vertices[source->triangles[triangle].indices.vertices[0]].sx >= 0.0f &&
            source->vertices[source->triangles[triangle].indices.vertices[0]].sx <= xMax &&
            source->vertices[source->triangles[triangle].indices.vertices[0]].sy >= 0.0f &&
            source->vertices[source->triangles[triangle].indices.vertices[0]].sy <= height &&
            source->vertices[source->triangles[triangle].indices.vertices[1]].sx >= 0.0f &&
            source->vertices[source->triangles[triangle].indices.vertices[1]].sx <= xMax &&
            source->vertices[source->triangles[triangle].indices.vertices[1]].sy >= 0.0f &&
            source->vertices[source->triangles[triangle].indices.vertices[1]].sy <= height &&
            source->vertices[source->triangles[triangle].indices.vertices[2]].sx >= 0.0f &&
            source->vertices[source->triangles[triangle].indices.vertices[2]].sx <= xMax &&
            source->vertices[source->triangles[triangle].indices.vertices[2]].sy >= 0.0f &&
            source->vertices[source->triangles[triangle].indices.vertices[2]].sy <= height) {
            clipFirstIndex = clippingBuffer.vertexCount;
            clippingBuffer.vertices[clippingBuffer.vertexCount++] = source->vertices[source->triangles[triangle].indices.vertices[0]];
            clippingBuffer.vertices[clippingBuffer.vertexCount++] = source->vertices[source->triangles[triangle].indices.vertices[1]];
            clippingBuffer.vertices[clippingBuffer.vertexCount++] = source->vertices[source->triangles[triangle].indices.vertices[2]];
            clippingBuffer.triangles[clippingBuffer.triangleCount].indices.vertices[0] = clipFirstIndex;
            clippingBuffer.triangles[clippingBuffer.triangleCount].indices.vertices[1] = clipFirstIndex + 1;
            clippingBuffer.triangles[clippingBuffer.triangleCount].indices.vertices[2] = clipFirstIndex + 2;
            clippingBuffer.triangles[clippingBuffer.triangleCount].indices.flags = source->triangles[triangle].indices.flags;
            clippingBuffer.triangles[clippingBuffer.triangleCount].blendMode = source->triangles[triangle].blendMode;
            clippingBuffer.triangles[clippingBuffer.triangleCount].textureHandle = source->triangles[triangle].textureHandle;
            clippingBuffer.triangles[clippingBuffer.triangleCount].textureSet = source->triangles[triangle].textureSet;
            clippingBuffer.triangleCount++;
            clippingRequired = 0;
        }
        if (clippingRequired &&
            (source->vertices[source->triangles[triangle].indices.vertices[0]].sx < 0.0f ||
             source->vertices[source->triangles[triangle].indices.vertices[0]].sx > xMax ||
             source->vertices[source->triangles[triangle].indices.vertices[0]].sy < 0.0f ||
             source->vertices[source->triangles[triangle].indices.vertices[0]].sy > height) &&
            (source->vertices[source->triangles[triangle].indices.vertices[1]].sx < 0.0f ||
             source->vertices[source->triangles[triangle].indices.vertices[1]].sx > xMax ||
             source->vertices[source->triangles[triangle].indices.vertices[1]].sy < 0.0f ||
             source->vertices[source->triangles[triangle].indices.vertices[1]].sy > height) &&
            (source->vertices[source->triangles[triangle].indices.vertices[2]].sx < 0.0f ||
             source->vertices[source->triangles[triangle].indices.vertices[2]].sx > xMax ||
             source->vertices[source->triangles[triangle].indices.vertices[2]].sy < 0.0f ||
             source->vertices[source->triangles[triangle].indices.vertices[2]].sy > height)) clippingRequired = 0;
        if (clippingRequired) {
            polyVerts[0] = source->vertices[source->triangles[triangle].indices.vertices[0]];
            polyVerts[1] = source->vertices[source->triangles[triangle].indices.vertices[1]];
            polyVerts[2] = source->vertices[source->triangles[triangle].indices.vertices[2]];
            polyVerts[3] = source->vertices[source->triangles[triangle].indices.vertices[0]];
            screenVertexCount = 3;
            for (tempPlane = 0; tempPlane < 4; tempPlane++) {
                if (tempPlane % 2 == 0) { inVertices = polyVerts; localOutput = localScratch; }
                else { inVertices = localScratch; localOutput = polyVerts; }
                for (edgeNo = 0; edgeNo < screenVertexCount; edgeNo++) {
                    x1 = inVertices[edgeNo].sx; y1 = inVertices[edgeNo].sy; z0 = inVertices[edgeNo].sz;
                    rhw0 = inVertices[edgeNo].rhw; u1 = inVertices[edgeNo].tu; v0 = inVertices[edgeNo].tv;
                    startAlpha = inVertices[edgeNo].color >> 24;
                    edgeRed1 = inVertices[edgeNo].color >> 16;
                    edgeFirstGreen = inVertices[edgeNo].color >> 8;
                    b1 = inVertices[edgeNo].color;
                    x2 = inVertices[edgeNo + 1].sx; y2 = inVertices[edgeNo + 1].sy; z2 = inVertices[edgeNo + 1].sz;
                    rhw2 = inVertices[edgeNo + 1].rhw; u2 = inVertices[edgeNo + 1].tu; v2 = inVertices[edgeNo + 1].tv;
                    a2 = inVertices[edgeNo + 1].color >> 24;
                    r2 = inVertices[edgeNo + 1].color >> 16;
                    g2 = inVertices[edgeNo + 1].color >> 8;
                    blue2 = inVertices[edgeNo + 1].color;
                    edgeCase = 0;
                    switch (tempPlane) {
                    case 0:
                        if (x1 >= 0.0f && x2 >= 0.0f) {
                            localOutput[numOut++] = inVertices[edgeNo + 1];
                            edgeCase = 1;
                        } else if (x1 >= 0.0f && x2 < 0.0f) {
                            localOutput[numOut] = inVertices[edgeNo + 1];
                            ratio = (0.0f - x1) / (x2 - x1);
                            localOutput[numOut].sy = (y2 - y1) * ratio + y1;
                            localOutput[numOut].sx = 0.0f;
                            localOutput[numOut].sz = z0;
                            localOutput[numOut].rhw = (rhw2 - rhw0) * ratio + rhw0;
                            localOutput[numOut].tu = (u2 - u1) * ratio + u1;
                            localOutput[numOut].tv = (v2 - v0) * ratio + v0;
                            alpha = ((float)a2 - (float)startAlpha) * ratio + startAlpha;
                            r = ((float)r2 - (float)edgeRed1) * ratio + edgeRed1;
                            g = ((float)g2 - (float)edgeFirstGreen) * ratio + edgeFirstGreen;
                            b = ((float)blue2 - (float)b1) * ratio + b1;
                            localOutput[numOut].color = (alpha * 0x1000000UL) | ((unsigned long)r << 16) | ((unsigned long)g << 8) | b;
                            numOut++;
                            edgeCase = 2;
                        } else if (x1 < 0.0f && x2 < 0.0f) {
                            edgeCase = 3;
                        } else {
                            localOutput[numOut] = inVertices[edgeNo + 1];
                            ratio = (0.0f - x1) / (x2 - x1);
                            localOutput[numOut].sy = (y2 - y1) * ratio + y1;
                            localOutput[numOut].sx = 0.0f;
                            localOutput[numOut].sz = z0;
                            localOutput[numOut].rhw = (rhw2 - rhw0) * ratio + rhw0;
                            localOutput[numOut].tu = (u2 - u1) * ratio + u1;
                            localOutput[numOut].tv = (v2 - v0) * ratio + v0;
                            alpha = ((float)a2 - (float)startAlpha) * ratio + startAlpha;
                            r = ((float)r2 - (float)edgeRed1) * ratio + edgeRed1;
                            g = ((float)g2 - (float)edgeFirstGreen) * ratio + edgeFirstGreen;
                            b = ((float)blue2 - (float)b1) * ratio + b1;
                            localOutput[numOut].color = (alpha * 0x1000000UL) | ((unsigned long)r << 16) | ((unsigned long)g << 8) | b;
                            numOut++;
                            localOutput[numOut++] = inVertices[edgeNo + 1];
                            edgeCase = 4;
                        }
                        break;
                    case 1:
                        if (((height)) >= (y1) && ((height)) >= (y2)) {
                            localOutput[numOut++] = inVertices[edgeNo + 1];
                            edgeCase = 1;
                        } else if (((height)) >= (y1) && ((height)) < (y2)) {
                            localOutput[numOut] = inVertices[edgeNo + 1];
                            ratio = (height - y1) / (y2 - y1);
                            localOutput[numOut].sx = (x2 - x1) * ratio + x1;
                            localOutput[numOut].sy = height;
                            localOutput[numOut].sz = z0;
                            localOutput[numOut].rhw = (rhw2 - rhw0) * ratio + rhw0;
                            localOutput[numOut].tu = (u2 - u1) * ratio + u1;
                            localOutput[numOut].tv = (v2 - v0) * ratio + v0;
                            alpha = ((float)a2 - (float)startAlpha) * ratio + startAlpha;
                            r = ((float)r2 - (float)edgeRed1) * ratio + edgeRed1;
                            g = ((float)g2 - (float)edgeFirstGreen) * ratio + edgeFirstGreen;
                            b = ((float)blue2 - (float)b1) * ratio + b1;
                            localOutput[numOut].color = (alpha * 0x1000000UL) | ((unsigned long)r << 16) | ((unsigned long)g << 8) | b;
                            numOut++;
                            edgeCase = 2;
                        } else if (((height)) < (y1) && ((height)) < (y2)) {
                            edgeCase = 3;
                        } else {
                            localOutput[numOut] = inVertices[edgeNo + 1];
                            ratio = (height - y1) / (y2 - y1);
                            localOutput[numOut].sx = (x2 - x1) * ratio + x1;
                            localOutput[numOut].sy = height;
                            localOutput[numOut].sz = z0;
                            localOutput[numOut].rhw = (rhw2 - rhw0) * ratio + rhw0;
                            localOutput[numOut].tu = (u2 - u1) * ratio + u1;
                            localOutput[numOut].tv = (v2 - v0) * ratio + v0;
                            alpha = ((float)a2 - (float)startAlpha) * ratio + startAlpha;
                            r = ((float)r2 - (float)edgeRed1) * ratio + edgeRed1;
                            g = ((float)g2 - (float)edgeFirstGreen) * ratio + edgeFirstGreen;
                            b = ((float)blue2 - (float)b1) * ratio + b1;
                            localOutput[numOut].color = (alpha * 0x1000000UL) | ((unsigned long)r << 16) | ((unsigned long)g << 8) | b;
                            numOut++;
                            localOutput[numOut++] = inVertices[edgeNo + 1];
                            edgeCase = 4;
                        }
                        break;
                    case 2:
                        if (x1 <= xMax && x2 <= xMax) {
                            localOutput[numOut++] = inVertices[edgeNo + 1];
                            edgeCase = 1;
                        } else if (x1 <= xMax && x2 > xMax) {
                            localOutput[numOut] = inVertices[edgeNo + 1];
                            ratio = (xMax - x1) / (x2 - x1);
                            localOutput[numOut].sy = (y2 - y1) * ratio + y1;
                            localOutput[numOut].sx = xMax;
                            localOutput[numOut].sz = z0;
                            localOutput[numOut].rhw = (rhw2 - rhw0) * ratio + rhw0;
                            localOutput[numOut].tu = (u2 - u1) * ratio + u1;
                            localOutput[numOut].tv = (v2 - v0) * ratio + v0;
                            alpha = ((float)a2 - (float)startAlpha) * ratio + startAlpha;
                            r = ((float)r2 - (float)edgeRed1) * ratio + edgeRed1;
                            g = ((float)g2 - (float)edgeFirstGreen) * ratio + edgeFirstGreen;
                            b = ((float)blue2 - (float)b1) * ratio + b1;
                            localOutput[numOut].color = (alpha * 0x1000000UL) | ((unsigned long)r << 16) | ((unsigned long)g << 8) | b;
                            numOut++;
                            edgeCase = 2;
                        } else if (x1 > 0.0f && x2 > xMax) {
                            /* The original tests zero here rather than the right boundary. */
                            edgeCase = 3;
                        } else {
                            localOutput[numOut] = inVertices[edgeNo + 1];
                            ratio = (xMax - x1) / (x2 - x1);
                            localOutput[numOut].sy = (y2 - y1) * ratio + y1;
                            localOutput[numOut].sx = xMax;
                            localOutput[numOut].sz = z0;
                            localOutput[numOut].rhw = (rhw2 - rhw0) * ratio + rhw0;
                            localOutput[numOut].tu = (u2 - u1) * ratio + u1;
                            localOutput[numOut].tv = (v2 - v0) * ratio + v0;
                            alpha = ((float)a2 - (float)startAlpha) * ratio + startAlpha;
                            r = ((float)r2 - (float)edgeRed1) * ratio + edgeRed1;
                            g = ((float)g2 - (float)edgeFirstGreen) * ratio + edgeFirstGreen;
                            b = ((float)blue2 - (float)b1) * ratio + b1;
                            localOutput[numOut].color = (alpha * 0x1000000UL) | ((unsigned long)r << 16) | ((unsigned long)g << 8) | b;
                            numOut++;
                            localOutput[numOut++] = inVertices[edgeNo + 1];
                            edgeCase = 4;
                        }
                        break;
                    case 3:
                        if (y1 >= 0.0f && y2 >= 0.0f) {
                            localOutput[numOut++] = inVertices[edgeNo + 1];
                            edgeCase = 1;
                        } else if (y1 >= 0.0f && y2 < 0.0f) {
                            localOutput[numOut] = inVertices[edgeNo + 1];
                            ratio = (0.0f - y1) / (y2 - y1);
                            localOutput[numOut].sx = (x2 - x1) * ratio + x1;
                            localOutput[numOut].sy = 0.0f;
                            localOutput[numOut].sz = z0;
                            localOutput[numOut].rhw = (rhw2 - rhw0) * ratio + rhw0;
                            localOutput[numOut].tu = (u2 - u1) * ratio + u1;
                            localOutput[numOut].tv = (v2 - v0) * ratio + v0;
                            alpha = ((float)a2 - (float)startAlpha) * ratio + startAlpha;
                            r = ((float)r2 - (float)edgeRed1) * ratio + edgeRed1;
                            g = ((float)g2 - (float)edgeFirstGreen) * ratio + edgeFirstGreen;
                            b = ((float)blue2 - (float)b1) * ratio + b1;
                            localOutput[numOut].color = (alpha * 0x1000000UL) | ((unsigned long)r << 16) | ((unsigned long)g << 8) | b;
                            numOut++;
                            edgeCase = 2;
                        } else if (y1 < 0.0f && y2 < 0.0f) {
                            edgeCase = 3;
                        } else {
                            localOutput[numOut] = inVertices[edgeNo + 1];
                            ratio = (0.0f - y1) / (y2 - y1);
                            localOutput[numOut].sx = (x2 - x1) * ratio + x1;
                            localOutput[numOut].sy = 0.0f;
                            localOutput[numOut].sz = z0;
                            localOutput[numOut].rhw = (rhw2 - rhw0) * ratio + rhw0;
                            localOutput[numOut].tu = (u2 - u1) * ratio + u1;
                            localOutput[numOut].tv = (v2 - v0) * ratio + v0;
                            alpha = ((float)a2 - (float)startAlpha) * ratio + startAlpha;
                            r = ((float)r2 - (float)edgeRed1) * ratio + edgeRed1;
                            g = ((float)g2 - (float)edgeFirstGreen) * ratio + edgeFirstGreen;
                            b = ((float)blue2 - (float)b1) * ratio + b1;
                            localOutput[numOut].color = (alpha * 0x1000000UL) | ((unsigned long)r << 16) | ((unsigned long)g << 8) | b;
                            numOut++;
                            localOutput[numOut++] = inVertices[edgeNo + 1];
                            edgeCase = 4;
                        }
                        break;
                    }
                }
                screenVertexCount = numOut;
                numOut = 0;
                for (copyVertexIndex = screenVertexCount - 1; copyVertexIndex >= 0; copyVertexIndex--) localOutput[copyVertexIndex + 1] = localOutput[copyVertexIndex];
                localOutput[0] = localOutput[screenVertexCount];
            }
            clipFirstIndex = clippingBuffer.vertexCount;
            for (copyVertexIndex = 0; copyVertexIndex < screenVertexCount; copyVertexIndex++) clippingBuffer.vertices[clippingBuffer.vertexCount++] = localOutput[copyVertexIndex];
            fanVert = 1;
            nextIndex = 2;
            while (nextIndex != screenVertexCount && screenVertexCount != 0) {
                clippingBuffer.triangles[clippingBuffer.triangleCount].blendMode = polyTexPage;
                clippingBuffer.triangles[clippingBuffer.triangleCount].textureHandle = tempTranslucency;
                clippingBuffer.triangles[clippingBuffer.triangleCount].textureSet = tempType;
                clippingBuffer.triangles[clippingBuffer.triangleCount].indices.vertices[0] = clipFirstIndex;
                clippingBuffer.triangles[clippingBuffer.triangleCount].indices.vertices[1] = fanVert * 1 + clipFirstIndex;
                clippingBuffer.triangles[clippingBuffer.triangleCount].indices.vertices[2] = nextIndex * 1 + clipFirstIndex;
                fanVert++;
                nextIndex++;
                clippingBuffer.triangleCount++;
            }
        }
    }
    currentBuffer = &clippingBuffer;
}
#endif
#ifdef EQUIV
#ifdef NETPLAY
/* NET-WOXL: the local names are chosen so that the /Od frame slots follow the exe (odslots order). */
/* Clips render triangles to the Direct3D viewport and triangulates the results. */
void ClipRenderBuffer(void)
{
    unsigned char alpha;
    int baseIndex;
    float edgeStartU;
    unsigned char alpha1;
    float edgeT;
    int startFanNext;
    unsigned char r;
    unsigned char b;
    short edgeKind;
    int edgeFanIndex;
    float endX;
    float edgeZ0;
    float edgeV0;
    unsigned char clipStartRed;
    unsigned char edgeB1;
    RenderVertex * outVertices;
    unsigned char edgeOutGreen;
    short outCount;
    float endY;
    float segTexU2;
    unsigned char alpha2;
    float edgeClipY1;
    unsigned char clipBlue2;
    float clipRightX;
    float clipX1;
    RenderBuffer * source;
    float clipEndZ;
    float vEnd;
    unsigned char endRed;
    RenderVertex polyScratch[CLIP_POLYGON_VERTEX_CAPACITY];
    float clipMaxY;
    short edgeSavedTriangleCount;
    short triangleIndex;
    short planeNo;
    int blendPage;
    short vertexCountSaved;
    short edgeIndex;
    int leftoverCount;
    RenderVertex * edgeInList;
    unsigned char segEdgeFirstGreen;
    float edgeRhwEnd;
    float startInvW;
    int texHandleTemp;
    unsigned char clipG2;
    int texSet;
    short clipNeeded;
    short clipScreenCount;
    short copyVertexIndex;
    RenderVertex scratchVerts[CLIP_POLYGON_VERTEX_CAPACITY];

    clipScreenCount = 0;
    edgeKind = 0;
    outCount = 0;
    clipRightX = screenScaleX;
    clipMaxY = screenScaleY;
    clipNeeded = 1;
    leftoverCount = 0;
    vertexCountSaved = 0;
    edgeSavedTriangleCount = 0;
    source = &renderBuffer;

    clippingBuffer.triangleCount = 0;
    clippingBuffer.vertexCount = 0;
    for (triangleIndex = 0; triangleIndex < source->triangleCount; triangleIndex++) {
        blendPage = source->triangles[triangleIndex].blendMode;
        texHandleTemp = source->triangles[triangleIndex].textureHandle;
        texSet = source->triangles[triangleIndex].textureSet;
        clipNeeded = 1;
        if (source->vertices[source->triangles[triangleIndex].indices.vertices[0]].sx >= 0.0f &&
            source->vertices[source->triangles[triangleIndex].indices.vertices[0]].sx <= clipRightX &&
            source->vertices[source->triangles[triangleIndex].indices.vertices[0]].sy >= 0.0f &&
            source->vertices[source->triangles[triangleIndex].indices.vertices[0]].sy <= clipMaxY &&
            source->vertices[source->triangles[triangleIndex].indices.vertices[1]].sx >= 0.0f &&
            source->vertices[source->triangles[triangleIndex].indices.vertices[1]].sx <= clipRightX &&
            source->vertices[source->triangles[triangleIndex].indices.vertices[1]].sy >= 0.0f &&
            source->vertices[source->triangles[triangleIndex].indices.vertices[1]].sy <= clipMaxY &&
            source->vertices[source->triangles[triangleIndex].indices.vertices[2]].sx >= 0.0f &&
            source->vertices[source->triangles[triangleIndex].indices.vertices[2]].sx <= clipRightX &&
            source->vertices[source->triangles[triangleIndex].indices.vertices[2]].sy >= 0.0f &&
            source->vertices[source->triangles[triangleIndex].indices.vertices[2]].sy <= clipMaxY) {
            baseIndex = clippingBuffer.vertexCount;
            clippingBuffer.vertices[clippingBuffer.vertexCount++] = source->vertices[source->triangles[triangleIndex].indices.vertices[0]];
            clippingBuffer.vertices[clippingBuffer.vertexCount++] = source->vertices[source->triangles[triangleIndex].indices.vertices[1]];
            clippingBuffer.vertices[clippingBuffer.vertexCount++] = source->vertices[source->triangles[triangleIndex].indices.vertices[2]];
            clippingBuffer.triangles[clippingBuffer.triangleCount].indices.vertices[0] = baseIndex;
            clippingBuffer.triangles[clippingBuffer.triangleCount].indices.vertices[1] = baseIndex + 1;
            clippingBuffer.triangles[clippingBuffer.triangleCount].indices.vertices[2] = baseIndex + 2;
            clippingBuffer.triangles[clippingBuffer.triangleCount].indices.flags = source->triangles[triangleIndex].indices.flags;
            clippingBuffer.triangles[clippingBuffer.triangleCount].blendMode = source->triangles[triangleIndex].blendMode;
            clippingBuffer.triangles[clippingBuffer.triangleCount].textureHandle = source->triangles[triangleIndex].textureHandle;
            clippingBuffer.triangles[clippingBuffer.triangleCount].textureSet = source->triangles[triangleIndex].textureSet;
            clippingBuffer.triangleCount++;
            clipNeeded = 0;
        }
        if (clipNeeded &&
            (source->vertices[source->triangles[triangleIndex].indices.vertices[0]].sx < 0.0f ||
             source->vertices[source->triangles[triangleIndex].indices.vertices[0]].sx > clipRightX ||
             source->vertices[source->triangles[triangleIndex].indices.vertices[0]].sy < 0.0f ||
             source->vertices[source->triangles[triangleIndex].indices.vertices[0]].sy > clipMaxY) &&
            (source->vertices[source->triangles[triangleIndex].indices.vertices[1]].sx < 0.0f ||
             source->vertices[source->triangles[triangleIndex].indices.vertices[1]].sx > clipRightX ||
             source->vertices[source->triangles[triangleIndex].indices.vertices[1]].sy < 0.0f ||
             source->vertices[source->triangles[triangleIndex].indices.vertices[1]].sy > clipMaxY) &&
            (source->vertices[source->triangles[triangleIndex].indices.vertices[2]].sx < 0.0f ||
             source->vertices[source->triangles[triangleIndex].indices.vertices[2]].sx > clipRightX ||
             source->vertices[source->triangles[triangleIndex].indices.vertices[2]].sy < 0.0f ||
             source->vertices[source->triangles[triangleIndex].indices.vertices[2]].sy > clipMaxY)) clipNeeded = 0;
        if (clipNeeded) {
            polyScratch[0] = source->vertices[source->triangles[triangleIndex].indices.vertices[0]];
            polyScratch[1] = source->vertices[source->triangles[triangleIndex].indices.vertices[1]];
            polyScratch[2] = source->vertices[source->triangles[triangleIndex].indices.vertices[2]];
            polyScratch[3] = source->vertices[source->triangles[triangleIndex].indices.vertices[0]];
            clipScreenCount = 3;
            for (planeNo = 0; planeNo < 4; planeNo++) {
                if (planeNo % 2 == 0) { edgeInList = polyScratch; outVertices = scratchVerts; }
                else { edgeInList = scratchVerts; outVertices = polyScratch; }
                for (edgeIndex = 0; edgeIndex < clipScreenCount; edgeIndex++) {
                    clipX1 = edgeInList[edgeIndex].sx; edgeClipY1 = edgeInList[edgeIndex].sy; edgeZ0 = edgeInList[edgeIndex].sz;
                    startInvW = edgeInList[edgeIndex].rhw; edgeStartU = edgeInList[edgeIndex].tu; edgeV0 = edgeInList[edgeIndex].tv;
                    alpha1 = edgeInList[edgeIndex].color >> 24;
                    clipStartRed = edgeInList[edgeIndex].color >> 16;
                    segEdgeFirstGreen = edgeInList[edgeIndex].color >> 8;
                    edgeB1 = edgeInList[edgeIndex].color;
                    endX = edgeInList[edgeIndex + 1].sx; endY = edgeInList[edgeIndex + 1].sy; clipEndZ = edgeInList[edgeIndex + 1].sz;
                    edgeRhwEnd = edgeInList[edgeIndex + 1].rhw; segTexU2 = edgeInList[edgeIndex + 1].tu; vEnd = edgeInList[edgeIndex + 1].tv;
                    alpha2 = edgeInList[edgeIndex + 1].color >> 24;
                    endRed = edgeInList[edgeIndex + 1].color >> 16;
                    clipG2 = edgeInList[edgeIndex + 1].color >> 8;
                    clipBlue2 = edgeInList[edgeIndex + 1].color;
                    edgeKind = 0;
                    switch (planeNo) {
                    case 0:
                        if (clipX1 >= 0.0f && endX >= 0.0f) {
                            outVertices[outCount++] = edgeInList[edgeIndex + 1];
                            edgeKind = 1;
                        } else if (clipX1 >= 0.0f && endX < 0.0f) {
                            outVertices[outCount] = edgeInList[edgeIndex + 1];
                            edgeT = (0.0f - clipX1) / (endX - clipX1);
                            outVertices[outCount].sy = (endY - edgeClipY1) * edgeT + edgeClipY1;
                            outVertices[outCount].sx = 0.0f;
                            outVertices[outCount].sz = edgeZ0;
                            outVertices[outCount].rhw = (edgeRhwEnd - startInvW) * edgeT + startInvW;
                            outVertices[outCount].tu = (segTexU2 - edgeStartU) * edgeT + edgeStartU;
                            outVertices[outCount].tv = (vEnd - edgeV0) * edgeT + edgeV0;
                            alpha = ((float)alpha2 - (float)alpha1) * edgeT + alpha1;
                            r = ((float)endRed - (float)clipStartRed) * edgeT + clipStartRed;
                            edgeOutGreen = ((float)clipG2 - (float)segEdgeFirstGreen) * edgeT + segEdgeFirstGreen;
                            b = ((float)clipBlue2 - (float)edgeB1) * edgeT + edgeB1;
                            outVertices[outCount].color = ((unsigned long)edgeOutGreen << 8) | (alpha * 0x1000000UL) | ((unsigned long)r << 16) | b;
                            outCount++;
                            edgeKind = 2;
                        } else if (clipX1 < 0.0f && endX < 0.0f) {
                            edgeKind = 3;
                        } else {
                            outVertices[outCount] = edgeInList[edgeIndex + 1];
                            edgeT = (0.0f - clipX1) / (endX - clipX1);
                            outVertices[outCount].sy = (endY - edgeClipY1) * edgeT + edgeClipY1;
                            outVertices[outCount].sx = 0.0f;
                            outVertices[outCount].sz = edgeZ0;
                            outVertices[outCount].rhw = (edgeRhwEnd - startInvW) * edgeT + startInvW;
                            outVertices[outCount].tu = (segTexU2 - edgeStartU) * edgeT + edgeStartU;
                            outVertices[outCount].tv = (vEnd - edgeV0) * edgeT + edgeV0;
                            alpha = ((float)alpha2 - (float)alpha1) * edgeT + alpha1;
                            r = ((float)endRed - (float)clipStartRed) * edgeT + clipStartRed;
                            edgeOutGreen = ((float)clipG2 - (float)segEdgeFirstGreen) * edgeT + segEdgeFirstGreen;
                            b = ((float)clipBlue2 - (float)edgeB1) * edgeT + edgeB1;
                            outVertices[outCount].color = ((unsigned long)edgeOutGreen << 8) | (alpha * 0x1000000UL) | ((unsigned long)r << 16) | b;
                            outCount++;
                            outVertices[outCount++] = edgeInList[edgeIndex + 1];
                            edgeKind = 4;
                        }
                        break;
                    case 1:
                        if (((clipMaxY)) >= (edgeClipY1) && ((clipMaxY)) >= (endY)) {
                            outVertices[outCount++] = edgeInList[edgeIndex + 1];
                            edgeKind = 1;
                        } else if (((clipMaxY)) >= (edgeClipY1) && ((clipMaxY)) < (endY)) {
                            outVertices[outCount] = edgeInList[edgeIndex + 1];
                            edgeT = (clipMaxY - edgeClipY1) / (endY - edgeClipY1);
                            outVertices[outCount].sx = (endX - clipX1) * edgeT + clipX1;
                            outVertices[outCount].sy = clipMaxY;
                            outVertices[outCount].sz = edgeZ0;
                            outVertices[outCount].rhw = (edgeRhwEnd - startInvW) * edgeT + startInvW;
                            outVertices[outCount].tu = (segTexU2 - edgeStartU) * edgeT + edgeStartU;
                            outVertices[outCount].tv = (vEnd - edgeV0) * edgeT + edgeV0;
                            alpha = ((float)alpha2 - (float)alpha1) * edgeT + alpha1;
                            r = ((float)endRed - (float)clipStartRed) * edgeT + clipStartRed;
                            edgeOutGreen = ((float)clipG2 - (float)segEdgeFirstGreen) * edgeT + segEdgeFirstGreen;
                            b = ((float)clipBlue2 - (float)edgeB1) * edgeT + edgeB1;
                            outVertices[outCount].color = ((unsigned long)edgeOutGreen << 8) | (alpha * 0x1000000UL) | ((unsigned long)r << 16) | b;
                            outCount++;
                            edgeKind = 2;
                        } else if (((clipMaxY)) < (edgeClipY1) && ((clipMaxY)) < (endY)) {
                            edgeKind = 3;
                        } else {
                            outVertices[outCount] = edgeInList[edgeIndex + 1];
                            edgeT = (clipMaxY - edgeClipY1) / (endY - edgeClipY1);
                            outVertices[outCount].sx = (endX - clipX1) * edgeT + clipX1;
                            outVertices[outCount].sy = clipMaxY;
                            outVertices[outCount].sz = edgeZ0;
                            outVertices[outCount].rhw = (edgeRhwEnd - startInvW) * edgeT + startInvW;
                            outVertices[outCount].tu = (segTexU2 - edgeStartU) * edgeT + edgeStartU;
                            outVertices[outCount].tv = (vEnd - edgeV0) * edgeT + edgeV0;
                            alpha = ((float)alpha2 - (float)alpha1) * edgeT + alpha1;
                            r = ((float)endRed - (float)clipStartRed) * edgeT + clipStartRed;
                            edgeOutGreen = ((float)clipG2 - (float)segEdgeFirstGreen) * edgeT + segEdgeFirstGreen;
                            b = ((float)clipBlue2 - (float)edgeB1) * edgeT + edgeB1;
                            outVertices[outCount].color = ((unsigned long)edgeOutGreen << 8) | (alpha * 0x1000000UL) | ((unsigned long)r << 16) | b;
                            outCount++;
                            outVertices[outCount++] = edgeInList[edgeIndex + 1];
                            edgeKind = 4;
                        }
                        break;
                    case 2:
                        /* Parentheses preserve /Od's boundary-first float comparisons. */
                        if (clipX1 <= clipRightX && (clipRightX) >= (endX)) {
                            outVertices[outCount++] = edgeInList[edgeIndex + 1];
                            edgeKind = 1;
                        } else if (clipX1 <= clipRightX && (clipRightX) < (endX)) {
                            outVertices[outCount] = edgeInList[edgeIndex + 1];
                            edgeT = (clipRightX - clipX1) / (endX - clipX1);
                            outVertices[outCount].sy = (endY - edgeClipY1) * edgeT + edgeClipY1;
                            outVertices[outCount].sx = clipRightX;
                            outVertices[outCount].sz = edgeZ0;
                            outVertices[outCount].rhw = (edgeRhwEnd - startInvW) * edgeT + startInvW;
                            outVertices[outCount].tu = (segTexU2 - edgeStartU) * edgeT + edgeStartU;
                            outVertices[outCount].tv = (vEnd - edgeV0) * edgeT + edgeV0;
                            alpha = ((float)alpha2 - (float)alpha1) * edgeT + alpha1;
                            r = ((float)endRed - (float)clipStartRed) * edgeT + clipStartRed;
                            edgeOutGreen = ((float)clipG2 - (float)segEdgeFirstGreen) * edgeT + segEdgeFirstGreen;
                            b = ((float)clipBlue2 - (float)edgeB1) * edgeT + edgeB1;
                            outVertices[outCount].color = ((unsigned long)edgeOutGreen << 8) | (alpha * 0x1000000UL) | ((unsigned long)r << 16) | b;
                            outCount++;
                            edgeKind = 2;
                        } else if (clipX1 > 0.0f && (clipRightX) < (endX)) {
                            /* The original tests zero here rather than the right boundary. */
                            edgeKind = 3;
                        } else {
                            outVertices[outCount] = edgeInList[edgeIndex + 1];
                            edgeT = (clipRightX - clipX1) / (endX - clipX1);
                            outVertices[outCount].sy = (endY - edgeClipY1) * edgeT + edgeClipY1;
                            outVertices[outCount].sx = clipRightX;
                            outVertices[outCount].sz = edgeZ0;
                            outVertices[outCount].rhw = (edgeRhwEnd - startInvW) * edgeT + startInvW;
                            outVertices[outCount].tu = (segTexU2 - edgeStartU) * edgeT + edgeStartU;
                            outVertices[outCount].tv = (vEnd - edgeV0) * edgeT + edgeV0;
                            alpha = ((float)alpha2 - (float)alpha1) * edgeT + alpha1;
                            r = ((float)endRed - (float)clipStartRed) * edgeT + clipStartRed;
                            edgeOutGreen = ((float)clipG2 - (float)segEdgeFirstGreen) * edgeT + segEdgeFirstGreen;
                            b = ((float)clipBlue2 - (float)edgeB1) * edgeT + edgeB1;
                            outVertices[outCount].color = ((unsigned long)edgeOutGreen << 8) | (alpha * 0x1000000UL) | ((unsigned long)r << 16) | b;
                            outCount++;
                            outVertices[outCount++] = edgeInList[edgeIndex + 1];
                            edgeKind = 4;
                        }
                        break;
                    case 3:
                        if (edgeClipY1 >= 0.0f && endY >= 0.0f) {
                            outVertices[outCount++] = edgeInList[edgeIndex + 1];
                            edgeKind = 1;
                        } else if (edgeClipY1 >= 0.0f && endY < 0.0f) {
                            outVertices[outCount] = edgeInList[edgeIndex + 1];
                            edgeT = (0.0f - edgeClipY1) / (endY - edgeClipY1);
                            outVertices[outCount].sx = (endX - clipX1) * edgeT + clipX1;
                            outVertices[outCount].sy = 0.0f;
                            outVertices[outCount].sz = edgeZ0;
                            outVertices[outCount].rhw = (edgeRhwEnd - startInvW) * edgeT + startInvW;
                            outVertices[outCount].tu = (segTexU2 - edgeStartU) * edgeT + edgeStartU;
                            outVertices[outCount].tv = (vEnd - edgeV0) * edgeT + edgeV0;
                            alpha = ((float)alpha2 - (float)alpha1) * edgeT + alpha1;
                            r = ((float)endRed - (float)clipStartRed) * edgeT + clipStartRed;
                            edgeOutGreen = ((float)clipG2 - (float)segEdgeFirstGreen) * edgeT + segEdgeFirstGreen;
                            b = ((float)clipBlue2 - (float)edgeB1) * edgeT + edgeB1;
                            outVertices[outCount].color = ((unsigned long)edgeOutGreen << 8) | (alpha * 0x1000000UL) | ((unsigned long)r << 16) | b;
                            outCount++;
                            edgeKind = 2;
                        } else if (edgeClipY1 < 0.0f && endY < 0.0f) {
                            edgeKind = 3;
                        } else {
                            outVertices[outCount] = edgeInList[edgeIndex + 1];
                            edgeT = (0.0f - edgeClipY1) / (endY - edgeClipY1);
                            outVertices[outCount].sx = (endX - clipX1) * edgeT + clipX1;
                            outVertices[outCount].sy = 0.0f;
                            outVertices[outCount].sz = edgeZ0;
                            outVertices[outCount].rhw = (edgeRhwEnd - startInvW) * edgeT + startInvW;
                            outVertices[outCount].tu = (segTexU2 - edgeStartU) * edgeT + edgeStartU;
                            outVertices[outCount].tv = (vEnd - edgeV0) * edgeT + edgeV0;
                            alpha = ((float)alpha2 - (float)alpha1) * edgeT + alpha1;
                            r = ((float)endRed - (float)clipStartRed) * edgeT + clipStartRed;
                            edgeOutGreen = ((float)clipG2 - (float)segEdgeFirstGreen) * edgeT + segEdgeFirstGreen;
                            b = ((float)clipBlue2 - (float)edgeB1) * edgeT + edgeB1;
                            outVertices[outCount].color = ((unsigned long)edgeOutGreen << 8) | (alpha * 0x1000000UL) | ((unsigned long)r << 16) | b;
                            outCount++;
                            outVertices[outCount++] = edgeInList[edgeIndex + 1];
                            edgeKind = 4;
                        }
                        break;
                    }
                }
                clipScreenCount = outCount;
                outCount = 0;
                for (copyVertexIndex = clipScreenCount - 1; copyVertexIndex >= 0; copyVertexIndex--) outVertices[copyVertexIndex + 1] = outVertices[copyVertexIndex];
                outVertices[0] = outVertices[clipScreenCount];
            }
            baseIndex = clippingBuffer.vertexCount;
            for (copyVertexIndex = 0; copyVertexIndex < clipScreenCount; copyVertexIndex++) clippingBuffer.vertices[clippingBuffer.vertexCount++] = outVertices[copyVertexIndex];
            edgeFanIndex = 1;
            startFanNext = 2;
            while (startFanNext != clipScreenCount && clipScreenCount != 0) {
                clippingBuffer.triangles[clippingBuffer.triangleCount].blendMode = blendPage;
                clippingBuffer.triangles[clippingBuffer.triangleCount].textureHandle = texHandleTemp;
                clippingBuffer.triangles[clippingBuffer.triangleCount].textureSet = texSet;
                clippingBuffer.triangles[clippingBuffer.triangleCount].indices.vertices[0] = baseIndex;
                clippingBuffer.triangles[clippingBuffer.triangleCount].indices.vertices[1] = edgeFanIndex * 1 + baseIndex;
                clippingBuffer.triangles[clippingBuffer.triangleCount].indices.vertices[2] = startFanNext * 1 + baseIndex;
                edgeFanIndex++;
                startFanNext++;
                clippingBuffer.triangleCount++;
            }
        }
    }
    currentBuffer = &clippingBuffer;
}
#endif
#endif
