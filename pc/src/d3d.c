#include "d3d.h"

#include "screen_offset.h"

#include "pixel_shift.h"

#define SCREEN_X_FLOAT_SCALE 0.003125f
#define SCREEN_Y_FLOAT_SCALE 0.0041666668f
#define SCREEN_X_INTEGER_SCALE 0.0015625f
#define SCREEN_Y_INTEGER_SCALE 0.0020833334f
#define ZERO_DEPTH_RECIPROCAL 10000.0f

/* Packs an RGB colour into the current display's 16-bit pixel format. */
unsigned short RGBToPixel(unsigned short r, unsigned short g, unsigned short b)
{
    r >>= pixelRedDown;
    g >>= pixelGreenDown;
    b >>= pixelBlueDown;
#ifdef NETPLAY
    return (g << pixelGreenUp) | (r << pixelRedUp) | (b << pixelBlueUp);
#else
    b <<= pixelBlueUp;
    g <<= pixelGreenUp;
    b |= g;
    r <<= pixelRedUp;
    return b | r;
#endif
}

/* Fills a transformed vertex from a point in the 320x240 layout, scaled to the screen, and its colour.
   Without RGB textures the texture coordinates carry the red level. */
#ifdef NETPLAY
void SetScreenVertex(D3DTLVERTEX *v, int x, int y, float z, unsigned char r, unsigned char g, unsigned char b)
#else
void SetScreenVertex(D3DTLVERTEX *v, float x, float y, float z, unsigned char r, unsigned char g, unsigned char b)
#endif
{
#ifdef NETPLAY
    v->sx = screenOffsetX.x + (float)x;
    v->sy = screenOffsetX.y + (float)y;
#else
    v->sx = screenScaleX * x * SCREEN_X_FLOAT_SCALE + screenOffsetX.x;
    v->sy = screenScaleY * y * SCREEN_Y_FLOAT_SCALE + screenOffsetX.y;
#endif
    v->sz = z;
    v->alpha = vertexAlpha;
    v->red = r;
    v->green = g;
    v->blue = b;
#ifdef POWERVR
    v->rhw = 1.0f - z;
#else
    if (z != 0.0f) {
        v->rhw = 1.0f / z;
    } else {
        v->rhw = ZERO_DEPTH_RECIPROCAL;
    }
#endif
    if (rgbTextures == 0) {
#ifdef NETPLAY
        v->tu = (unsigned long)r / 255.0f;
        v->tv = 110.0f / 255.0f;
#else
        v->tu = (unsigned long)r * 0.00392156862745098f;
        v->tv = 110.0f / 255.0f;
#endif
    }
}

/* Fills a transformed vertex from an integer point in the 320x240 layout, scaled to the screen, and its colour. */
void SetScaledScreenVertex(D3DTLVERTEX *v, int x, int y, float z, unsigned char r, unsigned char g, unsigned char b)
{
#ifdef NETPLAY
    v->sx = x * screenScaleX / 640.0f + screenOffsetX.x;
    v->sy = y * screenScaleY / 480.0f + screenOffsetX.y;
#else
    v->sx = x * screenScaleX * SCREEN_X_INTEGER_SCALE + screenOffsetX.x;
    v->sy = y * screenScaleY * SCREEN_Y_INTEGER_SCALE + screenOffsetX.y;
#endif
    v->sz = z;
    v->alpha = vertexAlpha;
    v->red = r;
    v->green = g;
    v->blue = b;
#ifdef POWERVR
    v->rhw = 1.0f - z;
#else
    if (z != 0.0f) {
        v->rhw = 1.0f / z;
    } else {
        v->rhw = ZERO_DEPTH_RECIPROCAL;
    }
#endif
    if (rgbTextures == 0) {
#ifdef NETPLAY
        v->tu = (unsigned long)r / 256.0f;
        v->tv = 0.4296875f;
#else
        v->tu = (unsigned long)r * 0.00390625f;
        v->tv = 0.4296875f;
#endif
    }
}

/* Fills a transformed vertex from an integer screen point, unscaled, and its colour. */
#ifndef NETPLAY
void SetPixelScreenVertex(D3DTLVERTEX *v, int x, int y, float z, unsigned char r, unsigned char g, unsigned char b)
{
    v->sx = (float)x + screenOffsetX.x;
    v->sy = (float)y + screenOffsetX.y;
    v->sz = z;
    v->alpha = vertexAlpha;
    v->red = r;
    v->green = g;
    v->blue = b;
#ifdef POWERVR
    v->rhw = 1.0f - z;
#else
    if (z != 0.0f) {
        v->rhw = 1.0f / z;
    } else {
        v->rhw = ZERO_DEPTH_RECIPROCAL;
    }
#endif
    if (rgbTextures == 0) {
        v->tu = (unsigned long)r * 0.00392156862745098f;
        v->tv = 110.0f / 255.0f;
    }
}
#endif

#ifdef POWERVR
int Stub_417220(void)
{
    return 1;
}
#endif

#ifdef NETPLAY
/* Scales floating coordinates to the screen and stores their colour. */
void SetScaledFloatScreenVertex(D3DTLVERTEX *v, float x, float y, float z,
                               unsigned char r, unsigned char g, unsigned char b)
{
    v->sx = x * screenScaleX / 320.0f + screenOffsetX.x;
    v->sy = y * screenScaleY / 240.0f + screenOffsetX.y;
    v->sz = z;
    v->alpha = vertexAlpha;
    v->red = r;
    v->green = g;
    v->blue = b;
    if (z != 0.0f) {
        v->rhw = 1.0f / z;
    } else {
        v->rhw = ZERO_DEPTH_RECIPROCAL;
    }
    if (rgbTextures == 0) {
        v->tu = (unsigned long)r / 255.0f;
        v->tv = 110.0f / 255.0f;
    }
}
#endif
