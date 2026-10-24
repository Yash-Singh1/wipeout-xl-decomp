#include "pc_backbuffer.h"
#include <windows.h>
#include <ddraw.h>
#include <math.h>
#include "pc_window.h"
#include "pc_textures.h"
#include "pc_palette.h"
#include "display.h"
#include "screen_offset.h"
#include "pc_surface.h"

enum {
    PVR_COMMAND_FOURCC = 0x43525650,
    PVR_COMMAND_READY_FLAG = 0x20
};

/* These two were built without /Oa: the exe reloads rgbTextures and re-reads fields across calls. */
#pragma optimize("a", off)

#ifdef NETPLAY
/* Clears the back buffer twice, once for each display buffer. */
void ClearBackBufferRect(unsigned char colour)
{
    RECT rect;
    DDBLTFX fx;
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    SetRect(&rect, 0, 0, (int)screenScaleX, (int)screenScaleY);
    fx.dwFillColor = rgbTextures ? 0 : colour;
    ddResult = IDirectDrawSurface_Blt(backBuffer, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    RaceFrameStart();
    ddResult = IDirectDrawSurface_Blt(backBuffer, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    RaceFrameStart();
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Clears the palette and back buffer twice; the rgbTextures test is held in eax where the original uses ecx. */
void ClearBackBufferRect(unsigned char colour)
{
    RECT rect;
    DDBLTFX fx;
#ifndef NETPLAY
    PALETTEENTRY entries[256];

    memset(entries, 0, sizeof(entries));
    ddResult = IDirectDrawPalette_SetEntries(defaultPalette, 0, 0, 256, entries);
#endif
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    SetRect(&rect, 0, 0, (int)screenScaleX, (int)screenScaleY);
    fx.dwFillColor = rgbTextures ? 0 : colour;
    ddResult = IDirectDrawSurface_Blt(backBuffer, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    RaceFrameStart();
    ddResult = IDirectDrawSurface_Blt(backBuffer, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    RaceFrameStart();
}
#endif
#endif

#ifdef NETPLAY
/* Writes a 16-level colour ramp into a 16 by 16 block of the surface kept in the last texture record. */
void FillFadeRampSurface(void)
{
    PixelChannels channelInfo;
    short textureIndex;
    short rows;
    unsigned short *surfacePixels;
    short topEdge;
    LPDIRECTDRAWSURFACE2 lockedSurface2;
    short rampWidth;
    short originX;
    short pixelY;
    DDSURFACEDESC lockedDesc;
    short column;
    unsigned short shadeLevel;
    DDPIXELFORMAT surfacePixelFormat;

    shadeLevel = 15;
    rampWidth = 16;
    rows = 16;
    originX = 70;
    topEdge = 0;
    textureIndex = 16;
    surfacePixelFormat.dwSize = sizeof(surfacePixelFormat);
    ddResult = textureRecords[textureIndex].surfaces[0]->lpVtbl->GetPixelFormat(textureRecords[textureIndex].surfaces[0], &surfacePixelFormat);
    dxErrorLine = 0x1b7e;
    LogDXError();
    GetPixelChannels(&channelInfo, surfacePixelFormat);
    lockedDesc.dwSize = sizeof(lockedDesc);
    textureRecords[textureIndex].surfaces[0]->lpVtbl->GetSurfaceDesc(textureRecords[textureIndex].surfaces[0], &lockedDesc);
    ddResult = textureRecords[textureIndex].surfaces[0]->lpVtbl->QueryInterface(textureRecords[textureIndex].surfaces[0], &IID_IDirectDrawSurface2, (void **)&lockedSurface2);
    dxErrorLine = 0x1b88;
    LogDXError();
    lockedDesc.dwSize = sizeof(lockedDesc);
    ddResult = lockedSurface2->lpVtbl->Lock(lockedSurface2, NULL, &lockedDesc, DDLOCK_WAIT, NULL);
    dxErrorLine = 0x1b8a;
    LogDXError();
    surfacePixels = (unsigned short *)lockedDesc.lpSurface;
    for (pixelY = topEdge; pixelY < rows; pixelY++) {
        shadeLevel = 0;
        for (column = originX; column < originX + rampWidth; column++) {
            surfacePixels[(lockedDesc.lPitch >> 1) * pixelY + column] = 0;
            surfacePixels[(lockedDesc.lPitch >> 1) * pixelY + column] = shadeLevel << channelInfo.redShift;
            surfacePixels[(lockedDesc.lPitch >> 1) * pixelY + column] |= shadeLevel << channelInfo.greenShift;
            surfacePixels[(lockedDesc.lPitch >> 1) * pixelY + column] |= shadeLevel << channelInfo.blueShift;
            surfacePixels[(lockedDesc.lPitch >> 1) * pixelY + column] |= shadeLevel << channelInfo.alphaShift;
            shadeLevel++;
        }
    }
    ddResult = lockedSurface2->lpVtbl->Unlock(lockedSurface2, NULL);
    dxErrorLine = 0x1bac;
    LogDXError();
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Writes a 16-level colour ramp to the fade surface. The surface base is loaded once and kept in a
   register; each store recomputes its row from the locked pitch, as the original does. */
void FillFadeRampSurface(void)
{
    PixelChannels channels;
    DDPIXELFORMAT pixelFormat;
    DDSURFACEDESC desc;
    short x;
    short y;
    unsigned short level;
    LPDIRECTDRAWSURFACE2 surface2;
    char *pixels;
    unsigned short *row;

    pixelFormat.dwSize = sizeof(pixelFormat);
    ddResult = fadeRampSurface->lpVtbl->GetPixelFormat(fadeRampSurface, &pixelFormat);
    dxErrorLine = 0x1838;
    LogDXError();
    GetPixelChannels(&channels, pixelFormat);
    desc.dwSize = sizeof(desc);
    fadeRampSurface->lpVtbl->GetSurfaceDesc(fadeRampSurface, &desc);
    ddResult = fadeRampSurface->lpVtbl->QueryInterface(fadeRampSurface, &IID_IDirectDrawSurface2, (void **)&surface2);
    dxErrorLine = 0x1842;
    LogDXError();
    desc.dwSize = sizeof(desc);
    ddResult = surface2->lpVtbl->Lock(surface2, NULL, &desc, DDLOCK_WAIT, NULL);
    dxErrorLine = 0x1844;
    LogDXError();
    pixels = (char *)desc.lpSurface;
    for (y = 0; y < 16; y++) {
        level = 0;
        for (x = 70; x < 86; x++) {
            row = (unsigned short *)(pixels + (desc.lPitch & ~1) * y);
            row[x] = 0;
            row = (unsigned short *)(pixels + (desc.lPitch & ~1) * y);
            row[x] = level << channels.redShift;
            row = (unsigned short *)(pixels + (desc.lPitch & ~1) * y);
            row[x] |= level << channels.greenShift;
            row = (unsigned short *)(pixels + (desc.lPitch & ~1) * y);
            row[x] |= level << channels.blueShift;
            row = (unsigned short *)(pixels + (desc.lPitch & ~1) * y);
            row[x] |= level << channels.alphaShift;
            level++;
        }
    }
    ddResult = surface2->lpVtbl->Unlock(surface2, NULL);
    dxErrorLine = 0x1866;
    LogDXError();
}
#endif
#endif

#pragma optimize("a", on)

/* Creates the PVRC command surface, obtains its texture handle, and initializes its command words. */
int CreatePvrCommandSurface(LPDIRECTDRAW dd, IDirect3DDevice *device)
{
    DDSURFACEDESC desc;
#ifdef NETPLAY
    HRESULT ddStatus;
#endif

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwHeight = 2;
    desc.dwWidth = 2;
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    desc.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    desc.ddpfPixelFormat.dwFourCC = PVR_COMMAND_FOURCC;
#ifdef NETPLAY
    ddStatus = IDirectDraw_CreateSurface(dd, &desc, &pvrCommandSurface, NULL);
    if (ddStatus == DD_OK) {
#else
    if (IDirectDraw_CreateSurface(dd, &desc, &pvrCommandSurface, NULL) == DD_OK) {
#endif
        IDirectDrawSurface_QueryInterface(pvrCommandSurface, &IID_IDirect3DTexture, (void **)&pvrCommandTexture);
        if (pvrCommandTexture != NULL) {
            memset(&desc, 0, sizeof(desc));
            desc.dwSize = sizeof(desc);
            desc.dwFlags = 0;
#ifdef NETPLAY
            ddStatus = IDirectDrawSurface_Lock(pvrCommandSurface, NULL, &desc, DDLOCK_WAIT, NULL);
            if (ddStatus == DD_OK) {
#else
            if (IDirectDrawSurface_Lock(pvrCommandSurface, NULL, &desc, DDLOCK_WAIT, NULL) == DD_OK) {
#endif
                pvrCommandData = desc.lpSurface;
                pvrCommandData->command = 0;
                pvrCommandData->flags |= PVR_COMMAND_READY_FLAG;
                IDirectDrawSurface_Unlock(pvrCommandSurface, &desc);
            } else {
                pvrCommandTexture->lpVtbl->Release(pvrCommandTexture);
                IDirectDrawSurface_Release(pvrCommandSurface);
                pvrCommandHandle = 0;
                pvrCommandTexture = NULL;
                pvrCommandSurface = NULL;
            }
            pvrCommandTexture->lpVtbl->GetHandle(pvrCommandTexture, device, &pvrCommandHandle);
            if (pvrCommandHandle == 0 || pvrCommandData->result == 0) {
                pvrCommandTexture->lpVtbl->Release(pvrCommandTexture);
                IDirectDrawSurface_Release(pvrCommandSurface);
                pvrCommandTexture = NULL;
                pvrCommandSurface = NULL;
                pvrCommandHandle = 0;
            }
        } else {
            IDirectDrawSurface_Release(pvrCommandSurface);
            pvrCommandSurface = NULL;
        }
    } else {
        pvrCommandSurface = NULL;
    }
    return 1;
}
