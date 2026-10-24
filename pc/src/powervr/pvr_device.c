#include "pc_window.h"

int SelectedResolutionIndex(void)
{
    int resolution;
    int available;
    int i;

    resolution = 0;
    i = 0;
    available = 0;
    for (; i < 3; i++) {
        if (displayBits == 16) {
            if (displayDrivers[videoModeIndex].supported16[pixelFormatGroup][i]) {
                if (resolutionSelection == available)
                    return resolution;
                available++;
            }
        } else {
            if (displayDrivers[videoModeIndex].supported8[pixelFormatGroup][i]) {
                if (resolutionSelection == available)
                    return resolution;
                available++;
            }
        }
        resolution++;
    }
    return resolution;
}

extern void SfxSetVolume(unsigned char volume);
extern void CdMusicStop(void);

typedef struct PvrDisplayResolution {
    unsigned short width;
    unsigned short height;
} PvrDisplayResolution;

typedef struct PvrDisplayMode {
    unsigned char present8;
    unsigned char present16;
    unsigned char reserved[2];
    DDPIXELFORMAT format8;
    DDPIXELFORMAT format16;
} PvrDisplayMode;

extern PvrDisplayResolution pvrDisplayResolutions[];
extern PvrDisplayMode pvrDisplayModes[];

int __stdcall EnumDisplayModesCallback(LPDDSURFACEDESC surface, LPVOID context)
{
    int bitsPerPixel;
    int i;

    wsprintfA(g_cdMessage, "%dx%dx%d\n", surface->dwWidth, surface->dwHeight,
        surface->ddpfPixelFormat.dwRGBBitCount);
    OutputDebugStringA(g_cdMessage);
    bitsPerPixel = surface->ddpfPixelFormat.dwRGBBitCount;
    if (bitsPerPixel == 8 || bitsPerPixel == 16) {
        for (i = 0; i < 3; i++) {
            if (pvrDisplayResolutions[i].width == surface->dwWidth
                && pvrDisplayResolutions[i].height == surface->dwHeight) {
                if (bitsPerPixel == 8) {
                    pvrDisplayModes[videoModeIndex * 3 + i].present8 = 1;
                    pvrDisplayModes[videoModeIndex * 3 + i].format8 = surface->ddpfPixelFormat;
                } else {
                    pvrDisplayModes[videoModeIndex * 3 + i].present16 = 1;
                    pvrDisplayModes[videoModeIndex * 3 + i].format16 = surface->ddpfPixelFormat;
                }
            }
        }
    }
    return 1;
}

void PlayMovie(char *movieName)
{
    int cdStatus[2];

    if (screenWidth != 512) {
        joyButtonNow = 0;
        joyButtonPrev = 0;
        ClearBackBufferToBlack();
        CdMusicStop();
        moviePlaying = 1;
        SfxSetVolume(255);
        PlayMpegMovie(g_hwnd, movieName, bmpBuffer);
        SfxSetVolume((gameData->effectVolume * 255) / 100);
        InitCDAudio(gameData, (unsigned char *)cdStatus);
        moviePlaying = 0;
        ClearBackBufferToBlack();
    }
}
