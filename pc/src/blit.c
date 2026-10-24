#include <windows.h>
#include <ddraw.h>
#include <stdio.h>
#include <string.h>
#include "pc_display.h"
#include "pc_blit.h"
#include "screen_offset.h"

#pragma intrinsic(memset, memcpy)

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned long u_long;

enum { BLIT_SURFACE = 1, BLIT_COLOR_FILL = 2 };

/* One queued 2D blit or colour fill; the queue is drained by FlushBlitQueue. */
typedef struct BlitEntry {
    int operation;              /* BLIT_SURFACE or BLIT_COLOR_FILL */
    union {
        LPDIRECTDRAWSURFACE surface;
        DWORD color;
    } source;
    RECT src;
    RECT dst;
} BlitEntry;

extern short CurrentScreen;
extern BlitEntry blitQueue[2][2][1000];
#include "scale_tables.h"
extern int doubleSourceRects;
extern LPDIRECTDRAWSURFACE textureSurfaces[];
extern TextureSurface blitTextures[];
extern LPDIRECTDRAWSURFACE backBuffer;
extern int fillEnabled;
extern long surfaceInfo[][3];
extern DDPIXELFORMAT ppmPixelFormat;
extern char g_cdMessage[];
extern HRESULT ddResult;
extern LPDIRECTDRAW lpDD;
extern BYTE bmpBuffer[];
extern PALETTEENTRY paletteEntries[256];
#include "palette_state.h"
extern LPDIRECTDRAWPALETTE defaultPalette;
extern LPDIRECTDRAWPALETTE gamePalette;
extern short videoModeIndex;
extern int pixelFormatGroup;
extern int pixelFormatIndex;

void FileAccessError(char *name);

u_short RGBToPixel(u_short r, u_short g, u_short b);

/* Queues a 320-pixel texture blit after clipping its horizontal bounds. */
void AddBlit320(int layer, int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1, int tex)
{
    x0 += screenOffsetX.x;
    y0 += screenOffsetX.y;
    x1 += screenOffsetX.x;
    y1 += screenOffsetX.y;
    if (x0 < 0) {
        u0 -= x0 - 0;
        x0 = 0;
    }
    if (x1 >= 320) {
        u1 -= (x1 - 320) - 0;
        x1 = 320;
    }
    blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].operation = BLIT_SURFACE;
    SetRect(&blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].dst, scaleTableX[x0], scaleTableY[y0], scaleTableX[x1], scaleTableY[y1]);
    if (doubleSourceRects) {
        SetRect(&blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].src, u0 * 2, v0 * 2, u1 * 2, v1 * 2);
    } else {
        SetRect(&blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].src, u0, v0, u1, v1);
    }
    blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].source.surface = textureSurfaces[tex];
    blitCount[CurrentScreen][layer]++;
}

/* Queues a scaled blit using the shifted texture index mapping. */
void AddBlit640Shifted(int layer, int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1, int tex)
{
    int textureIndex;

    x0 += screenOffsetX.x;
    y0 += screenOffsetX.y;
    x1 += screenOffsetX.x;
    y1 += screenOffsetX.y;
    if (x0 < 0) {
        u0 -= x0 - 0;
        x0 = 0;
    }
    if (x1 >= 640) {
        u1 -= (x1 - 640) - 0;
        x1 = 640;
    }
    blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].operation = BLIT_SURFACE;
    SetRect(&blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].dst,
            (int)(x0 * screenScaleX / 640.0f), (int)(y0 * screenScaleY / 480.0f),
            (int)(x1 * screenScaleX / 640.0f), (int)(y1 * screenScaleY / 480.0f));
    SetRect(&blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].src, u0, v0, u1, v1);
    textureIndex = tex;
    if (1) {
        if (textureIndex > 15 && textureIndex < 100) {
            textureIndex -= 10;
        } else if (textureIndex < 16) {
            textureIndex -= 5;
        }
    } else {
    }
    blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].source.surface = blitTextures[textureIndex].surface;
    blitCount[CurrentScreen][layer]++;
}

/* Queues a scaled texture blit for the current screen. */
void AddBlit640(int layer, int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1, int tex)
{
    x0 += screenOffsetX.x;
    y0 += screenOffsetX.y;
    x1 += screenOffsetX.x;
    y1 += screenOffsetX.y;
    if (x0 < 0) {
        u0 -= x0 - 0;
        x0 = 0;
    }
    if (x1 >= 640) {
        u1 -= (x1 - 640) - 0;
        x1 = 640;
    }
    blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].operation = BLIT_SURFACE;
    SetRect(&blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].dst,
            (int)(x0 * screenScaleX / 640.0f), (int)(y0 * screenScaleY / 480.0f),
            (int)(x1 * screenScaleX / 640.0f), (int)(y1 * screenScaleY / 480.0f));
    SetRect(&blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].src, u0, v0, u1, v1);
    blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].source.surface = textureSurfaces[tex];
    blitCount[CurrentScreen][layer]++;
}

/* Queues a solid-color rectangle when fills are enabled. */
void AddFill(int layer, Tile *tile)
{
    if (fillEnabled == 1) {
        blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].operation = BLIT_COLOR_FILL;
        SetRect(&blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].src,
                scaleTableX[tile->x0 + screenOffsetX.x], scaleTableY[tile->y0 + screenOffsetX.y],
                scaleTableX[tile->x0 + tile->w + screenOffsetX.x] - 1, scaleTableY[tile->y0 + tile->h + screenOffsetX.x] - 1);
        blitQueue[CurrentScreen][layer][blitCount[CurrentScreen][layer]].source.color = RGBToPixel(tile->r0, tile->g0, tile->b0);
        blitCount[CurrentScreen][layer]++;
    }
}

/* Draws queued blits and fills in reverse insertion order. */
void FlushBlitQueue(int layer)
{
    DDBLTFX fx;
    int i;

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    for (i = blitCount[CurrentScreen][layer] - 1; i >= 0; i--) {
        switch (blitQueue[CurrentScreen][layer][i].operation) {
        case BLIT_SURFACE:
            IDirectDrawSurface_Blt(backBuffer, &blitQueue[CurrentScreen][layer][i].dst, blitQueue[CurrentScreen][layer][i].source.surface, &blitQueue[CurrentScreen][layer][i].src, DDBLT_KEYSRC | DDBLT_ASYNC, &fx);
            break;
        case BLIT_COLOR_FILL:
            fx.dwFillColor = blitQueue[CurrentScreen][layer][i].source.color;
            IDirectDrawSurface_Blt(backBuffer, &blitQueue[CurrentScreen][layer][i].src, NULL, NULL, DDBLT_COLORFILL | DDBLT_ASYNC, &fx);
            break;
        default:
            break;
        }
    }
    blitCount[CurrentScreen][layer] = 0;
}

/* Releases loaded texture surfaces marked as owned. */
void ReleaseSurfaces(void)
{
    int i;

    for (i = 0; i < 7; i++) {
        if (surfaceInfo[i][2] == 1) {
            IDirectDrawSurface_Release(textureSurfaces[i]);
            surfaceInfo[i][2] = 0;
        }
    }
}

#ifndef NETPLAY
/* Loads a binary PPM (P6) into a DirectDraw surface, converting each pixel to the
   display's channel masks. */
LPDIRECTDRAWSURFACE LoadPPMSurface(char *name, int index)
{
    /* The executable initializes this unused local before opening the file. */
    int unusedZ = 0;
    FILE *file;
    char inputLine[100];
    int width;
    int numRows;
    DDSURFACEDESC ddsd;
    LPDIRECTDRAWSURFACE lpSurface;
    DWORD m;
    /* These spellings preserve the MSVC 4.20 /Od stack layout. */
    int shiftCount5;              /* Current mask's trailing zero bit count. */
    int rShift;                  /* Red, green and blue mask shifts. */
    int greenOffset;
    int bShiftCount;
    int rStep;                   /* Red, green and blue channel divisors. */
    int gRange;
    int bDiv;
    int x;
    int y;
    BYTE *row;
    WORD *dstWord;
    DWORD *dstPtr;
    DDCOLORKEY key;

    file = fopen(name, "rb");
    if (!file) {
        FileAccessError(name);
        wsprintf(g_cdMessage, "ERROR: Cannot find %s.\n", name);
        OutputDebugString(g_cdMessage);
        return 0;
    }
    fgets(inputLine, 100, file);
    if (lstrcmpA(inputLine, "P6\n") != 0) {
        fclose(file);
        wsprintf(g_cdMessage, "ERROR: %s is not a PPM file.\n", name);
        OutputDebugString(g_cdMessage);
        return 0;
    }
    do {
        fgets(inputLine, 100, file);
    } while (inputLine[0] == '#');
    sscanf(inputLine, "%d %d\n", &width, &numRows);
    fgets(inputLine, 100, file);

    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwHeight = numRows;
    ddsd.dwWidth = width;
    surfaceInfo[index][0] = width;
    surfaceInfo[index][1] = numRows;
    surfaceInfo[index][2] = 1;
    ddsd.ddpfPixelFormat = ppmPixelFormat;
    ddResult = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpSurface, NULL);
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddResult = IDirectDrawSurface_Lock(lpSurface, NULL, &ddsd, 0, NULL);

    for (shiftCount5 = 0, m = ppmPixelFormat.dwRBitMask; !(m & 1); shiftCount5++, m >>= 1)
        ;
    rShift = shiftCount5;
    rStep = 0xff / (ppmPixelFormat.dwRBitMask >> shiftCount5);

    for (shiftCount5 = 0, m = ppmPixelFormat.dwGBitMask; !(m & 1); shiftCount5++, m >>= 1)
        ;
    greenOffset = shiftCount5;
    gRange = 0xff / (ppmPixelFormat.dwGBitMask >> shiftCount5);

    for (shiftCount5 = 0, m = ppmPixelFormat.dwBBitMask; !(m & 1); shiftCount5++, m >>= 1)
        ;
    bShiftCount = shiftCount5;
    bDiv = 0xff / (ppmPixelFormat.dwBBitMask >> shiftCount5);

    switch (ppmPixelFormat.dwRGBBitCount) {
    case 32:
        for (y = 0; y < numRows; y++) {
            dstPtr = (DWORD *)((BYTE *)ddsd.lpSurface + ddsd.lPitch * y);
            for (x = 0; x < width; x++) {
                #ifdef NETPLAY
        int r = fgetc(file) / rStep;
#else
        int r = getc(file) / rStep;
#endif
                #ifdef NETPLAY
        int g = fgetc(file) / gRange;
#else
        int g = getc(file) / gRange;
#endif
                #ifdef NETPLAY
        int b = fgetc(file) / bDiv;
#else
        int b = getc(file) / bDiv;
#endif
                *dstPtr = (g << greenOffset) | (r << rShift) | (b << bShiftCount);
                dstPtr++;
            }
        }
        break;
    case 16:
        for (y = 0; y < numRows; y++) {
            dstWord = (WORD *)((BYTE *)ddsd.lpSurface + ddsd.lPitch * y);
            for (x = 0; x < width; x++) {
                #ifdef NETPLAY
        int r = fgetc(file) / rStep;
#else
        int r = getc(file) / rStep;
#endif
                #ifdef NETPLAY
        int g = fgetc(file) / gRange;
#else
        int g = getc(file) / gRange;
#endif
                #ifdef NETPLAY
        int b = fgetc(file) / bDiv;
#else
        int b = getc(file) / bDiv;
#endif
                *dstWord = (WORD)((b << bShiftCount) | (g << greenOffset) | (r << rShift));
                dstWord++;
            }
        }
        break;
    case 8:
        for (y = 0; y < numRows; y++) {
            row = (BYTE *)ddsd.lpSurface + ddsd.lPitch * y;
            for (x = 0; x < width; x++) {
                #ifdef NETPLAY
        int r = fgetc(file) / rStep;
#else
        int r = getc(file) / rStep;
#endif
                #ifdef NETPLAY
        int g = fgetc(file) / gRange;
#else
        int g = getc(file) / gRange;
#endif
                #ifdef NETPLAY
        int b = fgetc(file) / bDiv;
#else
        int b = getc(file) / bDiv;
#endif
                *row = (BYTE)((r << rShift) | (b << bShiftCount) | (g << greenOffset));
                row++;
            }
        }
        break;
    default:
        IDirectDrawSurface_Unlock(lpSurface, NULL);
        fclose(file);
        IDirectDrawSurface_Release(lpSurface);
        OutputDebugString("ERROR: Unknown pixel format (loadtex).");
        return 0;
    }
    ddResult = IDirectDrawSurface_Unlock(lpSurface, NULL);
    memset(&key, 0, sizeof(key));
    ddResult = IDirectDrawSurface_SetColorKey(lpSurface, DDCKEY_SRCBLT, &key);
    fclose(file);
    return lpSurface;
}

#endif

#ifdef EQUIV
#ifdef NETPLAY
/* Loads a binary PPM (P6) into a DirectDraw surface, converting each pixel to the
   display's channel masks. */
LPDIRECTDRAWSURFACE LoadPPMSurface(char *name, int index)
{
    int rShiftCount;
    int greenBits;
    int zeroBits;
    DDCOLORKEY colorKey;
    int blueShift;
    /* The original initializes this local but never uses it. */
    int unused;
    int gStep;
    FILE *inputFile;
    int x;
    int redQuantizer;
    int y;
    int numRows;
    int width;
    DWORD *outputPixels;
    WORD *targetWords;
    BYTE *target8;
    DWORD m;
    DDSURFACEDESC surfaceDescription;
    int blueComponentScale;
    LPDIRECTDRAWSURFACE createdSurface;
    char readBuffer[100];

    unused = 0;
    inputFile = fopen(name, "rb");
    if (!inputFile) {
        FileAccessError(name);
        wsprintf(g_cdMessage, "ERROR: Cannot find %s.\n", name);
        OutputDebugString(g_cdMessage);
        return 0;
    }
    fgets(readBuffer, 100, inputFile);
    if (lstrcmpA(readBuffer, "P6\n") != 0) {
        fclose(inputFile);
        wsprintf(g_cdMessage, "ERROR: %s is not a PPM file.\n", name);
        OutputDebugString(g_cdMessage);
        return 0;
    }
    do {
        fgets(readBuffer, 100, inputFile);
    } while (readBuffer[0] == '#');
    sscanf(readBuffer, "%d %d\n", &width, &numRows);
    fgets(readBuffer, 100, inputFile);

    surfaceDescription.dwSize = sizeof(surfaceDescription);
    surfaceDescription.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    surfaceDescription.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    surfaceDescription.dwHeight = numRows;
    surfaceDescription.dwWidth = width;
    surfaceInfo[index][0] = width;
    surfaceInfo[index][1] = numRows;
    surfaceInfo[index][2] = 1;
    surfaceDescription.ddpfPixelFormat = ppmPixelFormat;
    ddResult = IDirectDraw_CreateSurface(lpDD, &surfaceDescription, &createdSurface, NULL);
    memset(&surfaceDescription, 0, sizeof(surfaceDescription));
    surfaceDescription.dwSize = sizeof(surfaceDescription);
    ddResult = IDirectDrawSurface_Lock(createdSurface, NULL, &surfaceDescription, 0, NULL);

    for (zeroBits = 0, m = ppmPixelFormat.dwRBitMask; !(m & 1); zeroBits++, m >>= 1)
        ;
    rShiftCount = zeroBits;
    redQuantizer = 0xff / (ppmPixelFormat.dwRBitMask >> zeroBits);

    for (zeroBits = 0, m = ppmPixelFormat.dwGBitMask; !(m & 1); zeroBits++, m >>= 1)
        ;
    greenBits = zeroBits;
    gStep = 0xff / (ppmPixelFormat.dwGBitMask >> zeroBits);

    for (zeroBits = 0, m = ppmPixelFormat.dwBBitMask; !(m & 1); zeroBits++, m >>= 1)
        ;
    blueShift = zeroBits;
    blueComponentScale = 0xff / (ppmPixelFormat.dwBBitMask >> zeroBits);

    switch (ppmPixelFormat.dwRGBBitCount) {
    case 32:
        for (y = 0; y < numRows; y++) {
            outputPixels = (DWORD *)((BYTE *)surfaceDescription.lpSurface + surfaceDescription.lPitch * y);
            for (x = 0; x < width; x++) {
                int r = fgetc(inputFile) / redQuantizer;
                int g = fgetc(inputFile) / gStep;
                int b = fgetc(inputFile) / blueComponentScale;
                *outputPixels = (g << greenBits) | (r << rShiftCount) | (b << blueShift);
                outputPixels++;
            }
        }
        break;
    case 16:
        for (y = 0; y < numRows; y++) {
            targetWords = (WORD *)((BYTE *)surfaceDescription.lpSurface + surfaceDescription.lPitch * y);
            for (x = 0; x < width; x++) {
                int r;
                int b;
                int g;

                r = fgetc(inputFile) / redQuantizer;
                g = fgetc(inputFile) / gStep;
                b = fgetc(inputFile) / blueComponentScale;
                *targetWords = (WORD)((r << rShiftCount) | (b << blueShift) | (g << greenBits));
                targetWords++;
            }
        }
        break;
    case 8:
        for (y = 0; y < numRows; y++) {
            target8 = (BYTE *)surfaceDescription.lpSurface + surfaceDescription.lPitch * y;
            for (x = 0; x < width; x++) {
                int r = fgetc(inputFile) / redQuantizer;
                int g = fgetc(inputFile) / gStep;
                int b = fgetc(inputFile) / blueComponentScale;
                *target8 = (BYTE)((r << rShiftCount) | (b << blueShift) | (g << greenBits));
                target8++;
            }
        }
        break;
    default:
        IDirectDrawSurface_Unlock(createdSurface, NULL);
        fclose(inputFile);
        IDirectDrawSurface_Release(createdSurface);
        OutputDebugString("ERROR: Unknown pixel format (loadtex).");
        return 0;
    }
    ddResult = IDirectDrawSurface_Unlock(createdSurface, NULL);
    memset(&colorKey, 0, sizeof(colorKey));
    ddResult = IDirectDrawSurface_SetColorKey(createdSurface, DDCKEY_SRCBLT, &colorKey);
    fclose(inputFile);
    return createdSurface;
}

#endif
#endif

/* Loads a bitmap image into a DirectDraw surface. */
/* NET local names follow the unoptimised executable's stack slots. */
#ifdef NETPLAY
LPDIRECTDRAWSURFACE LoadBMPSurface(char *name, int index, int usePalette)
{
    DDCOLORKEY colorKey;
    BYTE *destinationSurface;
    int imageSize;
    BITMAPFILEHEADER bitmapFileHeader;
    BITMAPINFOHEADER dibInfoHeaderData;
    RGBQUAD colorTable[256];
    BYTE *pixelData;
    int x;
    int zero = 0;
    FILE *bmpFile;
    unsigned int colorIndex;
    LPDIRECTDRAWSURFACE bitmapSurface;
    DDSURFACEDESC surfaceDesc;
    BYTE *pixelSourceDib;
    int scanline;

    bmpFile = fopen(name, "rb");
    if (!bmpFile) {
        FileAccessError(name);
        return 0;
    }
    fread(&bitmapFileHeader, 1, 14, bmpFile);
    fread(&dibInfoHeaderData, 1, 40, bmpFile);
    if (dibInfoHeaderData.biBitCount == 8) {
        imageSize = dibInfoHeaderData.biWidth * dibInfoHeaderData.biHeight;
    }
    pixelData = bmpBuffer;
    if (!pixelData) {
        OutputDebugString("Not enough memory to load textures");
        GlobalFree(pixelData);
        fclose(bmpFile);
        return 0;
    }
    fread(colorTable, 4, 256, bmpFile);
    if (usePalette) {
        for (colorIndex = 0; colorIndex < 256; colorIndex++) {
            paletteEntries[colorIndex].peRed = colorTable[colorIndex].rgbRed;
            paletteEntries[colorIndex].peGreen = colorTable[colorIndex].rgbGreen;
            paletteEntries[colorIndex].peBlue = colorTable[colorIndex].rgbBlue;
            paletteEntries[colorIndex].peFlags = 0x40;
        }
        ddResult = IDirectDraw_CreatePalette(lpDD, DDPCAPS_8BIT | DDPCAPS_ALLOW256, paletteEntries, &gamePalette, NULL);
    } else {
        for (colorIndex = 0; colorIndex < 256; colorIndex++) {
            defaultPaletteEntries[colorIndex].peRed = colorTable[colorIndex].rgbRed;
            defaultPaletteEntries[colorIndex].peGreen = colorTable[colorIndex].rgbGreen;
            defaultPaletteEntries[colorIndex].peBlue = colorTable[colorIndex].rgbBlue;
            defaultPaletteEntries[colorIndex].peFlags = 0x40;
        }
        memcpy(savedPalette, defaultPaletteEntries, sizeof(savedPalette));
        ddResult = IDirectDraw_CreatePalette(lpDD, DDPCAPS_8BIT | DDPCAPS_ALLOW256, defaultPaletteEntries, &defaultPalette, NULL);
    }
    fread(pixelData, 1, imageSize, bmpFile);
    fclose(bmpFile);
    surfaceDesc.dwSize = sizeof(surfaceDesc);
    surfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    surfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    surfaceDesc.dwHeight = dibInfoHeaderData.biHeight;
    surfaceDesc.dwWidth = dibInfoHeaderData.biWidth;
    surfaceInfo[index][0] = dibInfoHeaderData.biWidth;
    surfaceInfo[index][1] = dibInfoHeaderData.biHeight;
    surfaceInfo[index][2] = 1;
    surfaceDesc.ddpfPixelFormat = displayDrivers[videoModeIndex].formats8[pixelFormatGroup][pixelFormatIndex];
    ddResult = IDirectDraw_CreateSurface(lpDD, &surfaceDesc, &bitmapSurface, NULL);
    ddResult = IDirectDrawSurface_SetPalette(bitmapSurface, defaultPalette);
    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);
    ddResult = IDirectDrawSurface_Lock(bitmapSurface, NULL, &surfaceDesc, 0, NULL);
    destinationSurface = surfaceDesc.lpSurface;
    pixelSourceDib = pixelData;
    for (scanline = 0; surfaceDesc.dwHeight > scanline; scanline++) {
        for (x = 0; surfaceDesc.lPitch > x; x++) {
            destinationSurface[surfaceDesc.lPitch * scanline + x] = pixelSourceDib[surfaceDesc.dwWidth * scanline + x];
        }
    }
    ddResult = IDirectDrawSurface_Unlock(bitmapSurface, NULL);
    memset(&colorKey, 0, sizeof(colorKey));
    ddResult = IDirectDrawSurface_SetColorKey(bitmapSurface, DDCKEY_SRCBLT, &colorKey);
    return bitmapSurface;
}
#else
LPDIRECTDRAWSURFACE LoadBMPSurface(char *name, int index, int usePalette)
{
    int unusedX = 0;
    FILE *file;
    int imageSize2;
    LPDIRECTDRAWSURFACE surface3;
    BITMAPFILEHEADER hdrVal;
    BITMAPINFOHEADER bihPtr;
    RGBQUAD pal[256];
    BYTE *pixels2;
    DDCOLORKEY ck;
    int rowIndex;
    unsigned int k;
    BYTE *src;
    int x;
    BYTE *dst;
    DDSURFACEDESC sd;

    file = fopen(name, "rb");
    if (!file) {
        FileAccessError(name);
        return 0;
    }
    fread(&hdrVal, 1, 14, file);
    fread(&bihPtr, 1, 40, file);
    if (bihPtr.biBitCount == 8) {
        imageSize2 = bihPtr.biWidth * bihPtr.biHeight;
    }
    pixels2 = bmpBuffer;
    if (!pixels2) {
        OutputDebugString("Not enough memory to load textures");
        GlobalFree(pixels2);
        fclose(file);
        return 0;
    }
    fread(pal, 4, 256, file);
    if (usePalette) {
        for (k = 0; k < 256; k++) {
            paletteEntries[k].peRed = pal[k].rgbRed;
            paletteEntries[k].peGreen = pal[k].rgbGreen;
            paletteEntries[k].peBlue = pal[k].rgbBlue;
            paletteEntries[k].peFlags = 0x40;
        }
        ddResult = IDirectDraw_CreatePalette(lpDD, DDPCAPS_8BIT | DDPCAPS_ALLOW256, paletteEntries, &gamePalette, NULL);
    } else {
        for (k = 0; k < 256; k++) {
            defaultPaletteEntries[k].peRed = pal[k].rgbRed;
            defaultPaletteEntries[k].peGreen = pal[k].rgbGreen;
            defaultPaletteEntries[k].peBlue = pal[k].rgbBlue;
            defaultPaletteEntries[k].peFlags = 0x40;
        }
        memcpy(savedPalette, defaultPaletteEntries, sizeof(savedPalette));
        ddResult = IDirectDraw_CreatePalette(lpDD, DDPCAPS_8BIT | DDPCAPS_ALLOW256, defaultPaletteEntries, &defaultPalette, NULL);
    }
    fread(pixels2, 1, imageSize2, file);
    fclose(file);
    sd.dwSize = sizeof(sd);
    sd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    sd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    sd.dwHeight = bihPtr.biHeight;
    sd.dwWidth = bihPtr.biWidth;
    surfaceInfo[index][0] = bihPtr.biWidth;
    surfaceInfo[index][1] = bihPtr.biHeight;
    surfaceInfo[index][2] = 1;
    sd.ddpfPixelFormat = displayDrivers[videoModeIndex].formats8[pixelFormatGroup][pixelFormatIndex];
    ddResult = IDirectDraw_CreateSurface(lpDD, &sd, &surface3, NULL);
    ddResult = IDirectDrawSurface_SetPalette(surface3, defaultPalette);
    memset(&sd, 0, sizeof(sd));
    sd.dwSize = sizeof(sd);
    ddResult = IDirectDrawSurface_Lock(surface3, NULL, &sd, 0, NULL);
    dst = sd.lpSurface;
    src = pixels2;
    for (rowIndex = 0; sd.dwHeight > rowIndex; rowIndex++) {
        for (x = 0; sd.lPitch > x; x++) {
            dst[sd.lPitch * rowIndex + x] = src[sd.dwWidth * rowIndex + x];
        }
    }
    ddResult = IDirectDrawSurface_Unlock(surface3, NULL);
    memset(&ck, 0, sizeof(ck));
    ddResult = IDirectDrawSurface_SetColorKey(surface3, DDCKEY_SRCBLT, &ck);
    return surface3;
}
#endif
