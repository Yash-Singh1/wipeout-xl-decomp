#include "texture_pages.h"
#include "setup_window.h"
#include <windows.h>
#include <ddraw.h>
#include "d3d.h"

#include "pc_window.h"
#include "pc_input.h"
#include "pc_image.h"
#include "vector_length.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mmsystem.h>
#include "pc_render.h"
#include "pc_bitmap.h"
#include "pc_textures.h"
#include "pc_palette.h"
#include "pc_surface.h"

#ifdef NETPLAY
#pragma intrinsic(memset)
#pragma intrinsic(memcmp)
#pragma intrinsic(strcpy, strcat)
#endif

extern const char g_w32sKernelName[];
extern const char g_imteFromHModuleProcName[];
extern const char g_baseAddrFromImteProcName[];
extern int textureHandles[];
void ConvertTexturePage(short page);
#ifdef NETPLAY
extern int bestRedUpNet, bestGreenUpNet, bestBlueUpNet;
#endif

#ifdef EQUIV
#ifdef NETPLAY
/* Releases sound, input, rendering objects and the enumerated driver strings. */
void ShutdownGame(void)
{
    short deviceIndex;
    short i;
    int spin;

    if (sfxStarted) {
        /* The delay loop increments i before it has ever been assigned. */
        for (spin = 0; spin < 50000; spin++)
            i++;
        TerminateSound();
        SetShieldHum(0);
        DroidSound(0);
        SetPitAmbient(0);
        SetAmbientLoop3(0);
        CrowdSound(0);
        ScrapeSound(0);
        SfxUnInitLow();
    }
    CdStop();
    CdClose();
    if (heap)
        GlobalFree(heap);
    if (reserveHeap)
        GlobalFree(reserveHeap);
    ReleaseDirectInput();
    ReleaseRendererObjects();
    ReleaseDirectDrawObjects();
    ShowCursor(TRUE);
    for (i = 0; i < displayDriverCount; i++) {
        if (displayDrivers[i].description)
            GlobalFree(displayDrivers[i].description);
        if (displayDrivers[i].name)
            GlobalFree(displayDrivers[i].name);
    }
    for (i = 0; i < videoModeIndex; i++) {
        deviceIndex = displayDrivers[i].deviceCount;
        if (displayDrivers[i].devices[deviceIndex].description)
            GlobalFree(displayDrivers[i].devices[deviceIndex].description);
        if (displayDrivers[i].devices[deviceIndex].name)
            GlobalFree(displayDrivers[i].devices[deviceIndex].name);
    }
}
#else
/* Releases sound, input, rendering objects and the enumerated driver strings. */
void ShutdownGame(void)
{
    short i;
    short deviceIndex;

    if (sfxStarted) {
        TerminateSound();
        SetShieldHum(0);
        DroidSound(0);
        SetPitAmbient(0);
        SetAmbientLoop3(0);
        CrowdSound(0);
        ScrapeSound(0);
        SfxUnInitLow();
    }
    CdStop();
    CdClose();
    if (heap) {
        GlobalFree(heap);
    }
    if (reserveHeap) {
        GlobalFree(reserveHeap);
    }
#ifdef POWERVR
    if (pvrCommandEnabled) {
        ReleasePvrCommandSurface();
    }
#endif
    ReleaseDirectInput();
    ReleaseRendererObjects();
    OutputDebugStringA("Shutting it all down\n");
    if (lpDD) {
        if (primarySurface) {
#ifdef POWERVR
            if (windowClipper) {
                IDirectDrawClipper_Release(windowClipper);
            }
#else
            if (windowPalette) {
                IDirectDrawPalette_Release(windowPalette);
            }
#endif
            IDirectDrawSurface_Release(primarySurface);
            primarySurface = NULL;
        }
        ddResult = IDirectDraw_RestoreDisplayMode(lpDD);
        dxErrorLine = 0xae3;
        LogDXError();
        ddResult = IDirectDraw_SetCooperativeLevel(lpDD, g_hwnd, DDSCL_NORMAL);
        dxErrorLine = 0xae4;
        LogDXError();
        IDirectDraw_Release(lpDD);
        IDirectDraw_Release(lpDD);
    }
    ShowCursor(TRUE);
    i = 0;
    if (i < displayDriverCount) {
        do {
            if (displayDrivers[i].description) {
                GlobalFree(displayDrivers[i].description);
            }
            if (displayDrivers[i].name) {
                GlobalFree(displayDrivers[i].name);
            }
            i++;
        } while (i < displayDriverCount);
    }
    /* The cleanup uses the selected driver index as its limit and each driver's device count as an index. */
    i = 0;
    if (i < videoModeIndex) {
        do {
            deviceIndex = displayDrivers[i].deviceCount;
            if (displayDrivers[i].devices[deviceIndex].description) {
                GlobalFree(displayDrivers[i].devices[deviceIndex].description);
            }
            if (displayDrivers[i].devices[deviceIndex].name) {
                GlobalFree(displayDrivers[i].devices[deviceIndex].name);
            }
            i++;
        } while (i < videoModeIndex);
    }
}
#endif
#endif

#ifdef EQUIV

/* Same computation as the exe. Built assuming no aliasing: the exe reads the primary surface's vtable
   between the profiling-timer stores, which only an /Oa compile may do. */
#pragma optimize("a", on)
#ifdef NETPLAY
#pragma intrinsic(memcpy)
int PresentRenderFrame(void)
{
    short spare = 0;
    unsigned long now;
    D3DRECT viewportRect;
    unsigned long elapsed;
    DDBLTFX effects;
    RECT screenRect;
    int i;

    now = timeGetTime();
    elapsed = now - presentStartTime;
    presentStartTime = now;
    renderProfileTimer = GetProfileTime();
    renderProfileTimer2 = renderProfileTimer;
    ddResult = IDirectDrawSurface_Flip(primarySurface, NULL, DDFLIP_WAIT);
    dxErrorLine = 0x916;
    LogDXError();
    flipProfileTime = GetProfileTime() - renderProfileTimer;
    if (!rgbTextures && defaultPalette) {
        if (paletteDirty == 1) {
            if (savedPaletteInUse) {
                ddResult = IDirectDrawPalette_SetEntries(defaultPalette, 0, 0, 256, savedPalette);
            } else {
                ddResult = IDirectDrawPalette_SetEntries(defaultPalette, 0, 240, 16, defaultPaletteEntries + 240);
            }
            paletteDirty = 0;
            savedPaletteInUse = 0;
        } else if (paletteDirty > 1) {
            ddResult = IDirectDrawPalette_SetEntries(defaultPalette, 0, 0, 256, savedPalette);
            memcpy(savedPalette, defaultPaletteEntries, sizeof(savedPalette));
            savedPaletteInUse = 1;
            paletteDirty = 0;
        }
    }
    renderProfileTimer = GetProfileTime();
    memset(&effects, 0, sizeof(effects));
    effects.dwSize = sizeof(effects);
    SetRect(&screenRect, 0, 0, screenScaleX, screenScaleY);
    if (fadeAmount > 180)
        effects.dwFillColor = ((fadeAmount >> pixelBlueDown) << pixelBlueUp) | ((fadeAmount >> pixelGreenDown) << pixelGreenUp) | ((fadeAmount >> pixelRedDown) << pixelRedUp);
    ddResult = IDirectDrawSurface_Blt(backBuffer, &screenRect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &effects);
    fillProfileTime = GetProfileTime() - renderProfileTimer;
    viewportRect.x1 = 0;
    viewportRect.y1 = 0;
    viewportRect.x2 = screenScaleX;
    viewportRect.y2 = screenScaleY;
    ddResult = renderViewport->lpVtbl->Clear(renderViewport, 1, &viewportRect, 2);
    dxErrorLine = 0x969;
    LogDXError();
    framesThisSecond++;
    if (time(NULL) != lastFpsSecond) {
        lastFpsSecond = time(NULL);
        displayStat0 = framesThisSecond;
        if (raceFinished) {
            fpsSampleCount++;
            fpsSampleTotal += framesThisSecond;
            displayStat1 = fpsSampleTotal / fpsSampleCount;
        }
        framesThisSecond = 0;
    }
    ddResult = lpD3DDevice->lpVtbl->BeginScene(lpD3DDevice);
    dxErrorLine = 0x980;
    LogDXError();
    for (i = 0; i < renderBufferIndex; i++) {
        ddResult = lpD3DDevice->lpVtbl->Execute(lpD3DDevice, (IDirect3DExecuteBuffer *)executeBuffers[i], renderViewport, renderBufferState[i]);
        dxErrorLine = 0x982;
        LogDXError();
    }
    ReleaseRenderMatrices();
    ddResult = lpD3DDevice->lpVtbl->EndScene(lpD3DDevice);
    dxErrorLine = 0x985;
    LogDXError();
    renderBufferIndex = 0;
    renderProfileTimer = GetProfileTime();
    if (!setupQuitRequested) {
        if (PeekMessageA(&windowMessage, NULL, 0, 0, PM_NOREMOVE)) {
            GetMessageA(&windowMessage, NULL, 0, 0);
            if (g_hwnd) {
                if (windowMessage.message == WM_QUIT) {
                    setupQuitRequested = 1;
                    return 1;
                }
                if (windowMessage.message == WM_KEYDOWN && !directInput && !(windowMessage.lParam & KEY_PREVIOUSLY_DOWN))
                    g_keyboardCode = windowMessage.wParam;
                TranslateMessage(&windowMessage);
                DispatchMessageA(&windowMessage);
            }
        }
        renderLoopProfileTime = GetProfileTime() - renderProfileTimer;
        renderLoopProfileTime2 = GetProfileTime() - renderProfileTimer2;
    } else {
        return 1;
    }
    return 0;
}
#pragma function(memcpy)
#else
int PresentRenderFrame(void)
{
    D3DRECT viewportRect;
    RECT screenRect;
    DDBLTFX effects;
    int i;
    int red, green, blue;

    presentStartTime = timeGetTime();
    {
        IDirectDrawSurface *surface = primarySurface;

        renderProfileTimer = 0.0;
        renderProfileTimer2 = 0.0;
        ddResult = surface->lpVtbl->Flip(surface, NULL, DDFLIP_WAIT);
    }
    dxErrorLine = 0x689;
    LogDXError();
    flipProfileTime = -renderProfileTimer;
    if (!rgbTextures && defaultPalette) {
        if (paletteDirty == 1) {
            if (savedPaletteInUse) {
                ddResult = IDirectDrawPalette_SetEntries(defaultPalette, 0, 0, 256, savedPalette);
            } else {
                ddResult = IDirectDrawPalette_SetEntries(defaultPalette, 0, 240, 16, reservedPalette);
            }
            savedPaletteInUse = 0;
            paletteDirty = 0;
        } else if (paletteDirty > 1) {
            ddResult = IDirectDrawPalette_SetEntries(defaultPalette, 0, 0, 256, savedPalette);
            memcpy(savedPalette, defaultPaletteEntries, sizeof(savedPalette));
            savedPaletteInUse = 1;
            paletteDirty = 0;
        }
    }
    renderProfileTimer = 0.0;
    memset(&effects, 0, sizeof(effects));
    effects.dwSize = sizeof(effects);
    SetRect(&screenRect, 0, 0, screenScaleX, screenScaleY);
    if (fadeAmount > 180) {
        blue = (fadeAmount >> pixelBlueDown) << pixelBlueUp;
        green = (fadeAmount >> pixelGreenDown) << pixelGreenUp;
        blue |= green;
        red = (fadeAmount >> pixelRedDown) << pixelRedUp;
        effects.dwFillColor = blue | red;
    }
#ifdef POWERVR
    if (!raceFinished)
#endif
    ddResult = IDirectDrawSurface_Blt(backBuffer, &screenRect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &effects);
    fillProfileTime = -renderProfileTimer;
    viewportRect.x1 = 0;
    viewportRect.y1 = 0;
    viewportRect.x2 = screenScaleX;
    viewportRect.y2 = screenScaleY;
    ddResult = renderViewport->lpVtbl->Clear(renderViewport, 1, &viewportRect, 2);
    dxErrorLine = 0x6db;
    LogDXError();
    framesThisSecond++;
    if (time(NULL) != lastFpsSecond) {
        lastFpsSecond = time(NULL);
        displayStat0 = framesThisSecond;
        if (raceFinished) {
            fpsSampleCount++;
            fpsSampleTotal += framesThisSecond;
            displayStat1 = fpsSampleTotal / fpsSampleCount;
        }
        framesThisSecond = 0;
    }
#ifndef POWERVR
    ddResult = lpD3DDevice->lpVtbl->BeginScene(lpD3DDevice);
    dxErrorLine = 0x6f2;
    LogDXError();
    for (i = 0; i < renderBufferIndex; i++) {
        ddResult = lpD3DDevice->lpVtbl->Execute(lpD3DDevice, (IDirect3DExecuteBuffer *)executeBuffers[i], renderViewport, renderBufferState[i]);
        dxErrorLine = 0x6f4;
        LogDXError();
    }
    ReleaseRenderMatrices();
    ddResult = lpD3DDevice->lpVtbl->EndScene(lpD3DDevice);
    dxErrorLine = 0x6f7;
    LogDXError();
    renderBufferIndex = 0;
#endif
    renderProfileTimer = 0.0;
    if (!setupQuitRequested) {
        if (PeekMessageA(&windowMessage, NULL, 0, 0, PM_NOREMOVE)) {
            GetMessageA(&windowMessage, NULL, 0, 0);
            if (g_hwnd) {
                if (windowMessage.message == WM_QUIT) {
                    setupQuitRequested = 1;
                    return 1;
                }
                if (windowMessage.message == WM_KEYDOWN && !directInput && !(windowMessage.lParam & KEY_PREVIOUSLY_DOWN)) {
                    g_keyboardCode = windowMessage.wParam;
                }
                TranslateMessage(&windowMessage);
                DispatchMessageA(&windowMessage);
            }
        }
        renderLoopProfileTime = -renderProfileTimer;
        renderLoopProfileTime2 = -renderProfileTimer2;
        return 0;
    }
    return 1;
}
#endif
#pragma optimize("a", off)

#endif

int AlwaysReturnOne(void)
{
    return 1;
}

#ifdef NETPLAY
extern unsigned char connectionCount;
extern unsigned char gstSwitch;
extern int gstProviderChoice;
#include "provider_state.h"
extern HWND connectionListBox;
#pragma intrinsic(strlen)

/* Adds a named connection to the multiplayer setup list. */
int AddConnectionListEntry(void *connection, const char *displayName)
{
    char text[100];
    unsigned int index;

    if (connectionCount >= 10)
        return 1;
    if (memcmp(displayName, "Gamers Online", 13) == 0 && !gstSwitch)
        return 1;
    strncpy(text, displayName, 100);
    for (index = 0; index < strlen(text) - 11; index++) {
        if (memcmp(text + index, " Connection", 11) == 0) {
            text[index] = 0;
            break;
        }
    }
    SendMessageA(connectionListBox, 0x143, 0, (LPARAM)text);
    gstProviderIds[connectionCount] = connection;
    if (memcmp(displayName, "IPX Connection", 14) == 0)
        gstProviderChoice = connectionCount;
    connectionCount++;
    return 1;
}
#endif

#ifdef NETPLAY
/* Registers the main window class and creates the centered game window. */
int CreateSetupWindow(HINSTANCE instance, char *unusedCommandLine)
{
    RECT rect;
    WNDCLASSA windowClass;
    int windowWidth;
    int winHeight;
    int posX;
    int posY;

    g_hInstance = instance;
    windowBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconA(instance, "AppIcon");
    windowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
    windowClass.hbrBackground = windowBrush;
    windowClass.lpszMenuName = "AppMenu";
    windowClass.lpszClassName = "PSYGWO2";
    SetCodeSectionReadWrite();
    if (!RegisterClassA(&windowClass))
        return 0;
    GetWindowRect(GetDesktopWindow(), &rect);
    windowWidth = 500;
    winHeight = 360;
    winHeight += 50;
    posX = (rect.right - rect.left - windowWidth) / 2;
    posY = (rect.bottom - rect.top - winHeight) / 2;
    g_hwnd = CreateWindowExA(WS_EX_APPWINDOW, "PSYGWO2", "Wipeout XL",
                             WS_POPUP | WS_CAPTION | WS_SYSMENU, posX, posY, windowWidth, winHeight,
                             NULL, NULL, instance, NULL);
    if (!g_hwnd) {
        ShowMessage("CreateWindowEx failed");
        return 0;
    }
    EnumerateDisplayDrivers();
    if (!acceleratorAvailable) {
        g_cdValidating = 1;
        MessageBoxA(NULL, "You must have a 3D accelerator card to run Wipeout XL",
                    "Accelerator card validation", 0);
        g_cdValidating = 0;
        return 0;
    }
    SetWindowPos(g_hwnd, NULL, posX, posY, windowWidth, winHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    return 1;
}
#endif


/* Reads BMP dimensions, allocating storage and reading pixels only for the 24-bit case. */
#ifdef NETPLAY
extern unsigned char gstSwitch;
extern unsigned char gstDebugOutputEnabled;

/* Applies the NET and debug command-line switches. */
int ReadGameCommandLineOptions(char *commandLine)
{
    char *argument;

    while ((argument = strchr(commandLine, '-')) != 0) {
        if (memcmp(argument, "-gst", 4) == 0)
            gstSwitch = 1;
        if (memcmp(argument, "-debug", 6) == 0)
            gstDebugOutputEnabled = 1;
        argument++;
        commandLine = argument;
    }
    return 1;
}
#endif

#ifdef NETPLAY
int LoadBitmapData(char *filename, BitmapData **bitmap)
{
    BITMAPFILEHEADER header;
    BITMAPINFOHEADER bitmapInfo;
    int *imageData;
    int bitmapWidth;
    int bitmapHeight;
    int bytes;
    FILE *bitmapFile;

    bitmapFile = fopen(filename, "rb");
    if (bitmapFile) {
        if (fread(&header, 1, sizeof(header), bitmapFile) != sizeof(header))
            return 1;
        if (fread(&bitmapInfo, 1, sizeof(bitmapInfo), bitmapFile) != sizeof(bitmapInfo))
            return 1;
        bitmapWidth = bitmapInfo.biWidth;
        bitmapHeight = bitmapInfo.biHeight;
        if (bitmapInfo.biBitCount == 8) {
            bytes = bitmapHeight * bitmapWidth;
            *bitmap = GlobalAlloc(GPTR, bytes);
            imageData = (int *)*bitmap;
            *imageData++ = bitmapWidth;
            *imageData++ = bitmapHeight;
            fclose(bitmapFile);
        } else if (bitmapInfo.biBitCount == 16) {
            bytes = bitmapHeight * bitmapWidth * 2;
            *bitmap = GlobalAlloc(GPTR, bytes);
            imageData = (int *)*bitmap;
            *imageData++ = bitmapWidth;
            *imageData++ = bitmapHeight;
            fclose(bitmapFile);
        } else if (bitmapInfo.biBitCount == 24) {
            bytes = bitmapHeight * bitmapWidth * 3;
            *bitmap = GlobalAlloc(GPTR, bytes);
            imageData = (int *)*bitmap;
            *imageData++ = bitmapWidth;
            *imageData++ = bitmapHeight;
            if (fread((char *)*bitmap + 8, 1, bytes, bitmapFile) != bytes)
                return 1;
            fclose(bitmapFile);
        } else {
            return 2;
        }
        return 0;
    }
    /* The original falls off the end when fopen fails. */
}
#else
int LoadBitmapData(char *filename, BitmapData **bitmap)
{
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    FILE *file;
    int width, height;
    int bytes;
    BitmapData *data;
    int result;

    /* A failed open leaves the result at the null FILE value. */
    result = (int)fopen(filename, "rb");
    file = (FILE *)result;
    if (file) {
        if (fread(&fileHeader, 1, sizeof(fileHeader), file) != sizeof(fileHeader)) {
            return 1;
        }
        if (fread(&infoHeader, 1, sizeof(infoHeader), file) != sizeof(infoHeader)) {
            return 1;
        }
        width = infoHeader.biWidth;
        height = infoHeader.biHeight;
        /* The allocation omits space for the dimensions, as in the executable. */
        if (infoHeader.biBitCount == 8) {
            data = GlobalAlloc(GPTR, width * height);
            data->width = width;
            data->height = height;
            *bitmap = data;
            fclose(file);
            return 0;
        }
        if (infoHeader.biBitCount == 16) {
            data = GlobalAlloc(GPTR, width * height * 2);
            data->width = width;
            data->height = height;
            *bitmap = data;
            fclose(file);
            return 0;
        }
        if (infoHeader.biBitCount == 24) {
            bytes = width * height * 3;
            data = GlobalAlloc(GPTR, bytes);
            data->width = width;
            *bitmap = data;
            data->height = height;
            if (fread(data->pixels, 1, bytes, file) != bytes) {
                return 1;
            }
            fclose(file);
            return 0;
        }
        result = 2;
    }
    return result;
}
#endif


#ifdef NETPLAY
void FlushRenderQueues(void)
{
    if (!lpD3DDevice)
        return;

    renderQueueCounter0 = 0;
    renderQueueCounter1 = 0;
    renderQueueCounter2 = 0;
    renderQueueCounter3 = 0;
    FlushBlitQueue(1);
    DrawRenderQueue();
    SubmitRenderTriangles();
    FlushBlitQueue(0);
}
#endif

#ifdef EQUIV
/* Does not match yet: the device check uses TEST rather than comparing with the cached zero. */
/* Drains the background blits, object queue and triangles before the foreground blits. */
#ifndef NETPLAY
void FlushRenderQueues(void)
{
#ifdef POWERVR
    int i;
#endif

    if (lpD3DDevice) {
        renderQueueCounter0 = 0;
        renderQueueCounter1 = 0;
        renderQueueCounter2 = 0;
        renderQueueCounter3 = 0;
        FlushBlitQueue(1);
        DrawRenderQueue();
        SubmitRenderTriangles();
#ifdef POWERVR
        ddResult = lpD3DDevice->lpVtbl->BeginScene(lpD3DDevice);
        dxErrorLine = 0x7a8;
        LogDXError();
        for (i = 0; i < renderBufferIndex; i++) {
            ddResult = lpD3DDevice->lpVtbl->Execute(lpD3DDevice, (IDirect3DExecuteBuffer *)executeBuffers[i], renderViewport, renderBufferState[i]);
            dxErrorLine = 0x7ab;
            LogDXError();
        }
        ReleaseRenderMatrices();
        ddResult = lpD3DDevice->lpVtbl->EndScene(lpD3DDevice);
        dxErrorLine = 0x7ae;
        LogDXError();
        renderBufferIndex = 0;
#endif
        FlushBlitQueue(0);
    }
}
#endif

/* Submits the game's objects and triangles, then flushes foreground blits unless names go first. */
/* Execute-buffer vtable loads may precede profiling stores under the no-aliasing contract. */
#pragma optimize("a", on)
void FlushGameRenderQueues(void)
{
#ifdef POWERVR
    int i;
#endif

    if (lpD3DDevice) {
        renderQueueCounter0 = 0;
        renderQueueCounter1 = 0;
        renderQueueCounter2 = 0;
        renderQueueCounter3 = 0;
        DrawRenderQueue();
        queueProfileTime0 = 0.0;
#ifdef POWERVR
        DrawPolyF4(1.0f, &setupBackground);
#endif
        SubmitRenderTriangles();
        queueProfileTime1 = 0.0;
#ifdef POWERVR
        ddResult = lpD3DDevice->lpVtbl->BeginScene(lpD3DDevice);
        dxErrorLine = 0x7d8;
        LogDXError();
#endif
        queueProfileTime2 = 0.0;
#ifdef POWERVR
        lastSpecialRenderBuffer = -1;
        if (renderBufferIndex > 0) {
            ddResult = lpD3DDevice->lpVtbl->Execute(lpD3DDevice, (IDirect3DExecuteBuffer *)executeBuffers[0], renderViewport, renderBufferState[0]);
            dxErrorLine = 0x7de;
            LogDXError();
            executeProfileTime0 = 0.0;
            if (renderBufferState[0] == 2)
                lastSpecialRenderBuffer = 0;
        }
        if (renderBufferIndex > 1) {
            ddResult = lpD3DDevice->lpVtbl->Execute(lpD3DDevice, (IDirect3DExecuteBuffer *)executeBuffers[1], renderViewport, renderBufferState[1]);
            dxErrorLine = 0x7e5;
            LogDXError();
            executeProfileTime1 = 0.0;
            if (renderBufferState[1] == 2)
                lastSpecialRenderBuffer = 1;
        }
        if (renderBufferIndex > 2) {
            ddResult = lpD3DDevice->lpVtbl->Execute(lpD3DDevice, (IDirect3DExecuteBuffer *)executeBuffers[2], renderViewport, renderBufferState[2]);
            dxErrorLine = 0x7ec;
            LogDXError();
            executeProfileTime2 = 0.0;
            if (renderBufferState[2] == 2)
                lastSpecialRenderBuffer = 2;
        }
        if (renderBufferIndex > 3) {
            ddResult = lpD3DDevice->lpVtbl->Execute(lpD3DDevice, (IDirect3DExecuteBuffer *)executeBuffers[3], renderViewport, renderBufferState[3]);
            dxErrorLine = 0x7f3;
            LogDXError();
            executeProfileTime3 = 0.0;
            if (renderBufferState[3] == 2)
                lastSpecialRenderBuffer = 3;
        }
        if (renderBufferIndex > 4) {
            ddResult = lpD3DDevice->lpVtbl->Execute(lpD3DDevice, (IDirect3DExecuteBuffer *)executeBuffers[4], renderViewport, renderBufferState[4]);
            dxErrorLine = 0x7fa;
            LogDXError();
            executeProfileTime4 = 0.0;
            if (renderBufferState[4] == 2)
                lastSpecialRenderBuffer = 4;
        }
        for (i = 5; i < renderBufferIndex; i++) {
            ddResult = lpD3DDevice->lpVtbl->Execute(lpD3DDevice, (IDirect3DExecuteBuffer *)executeBuffers[i], renderViewport, renderBufferState[i]);
            dxErrorLine = 0x801;
            LogDXError();
        }
        releaseMatricesProfileTime = 0.0;
        ReleaseRenderMatrices();
        renderBufferIndex = 0;
        sceneEndProfileTime = 0.0;
        ddResult = lpD3DDevice->lpVtbl->EndScene(lpD3DDevice);
        dxErrorLine = 0x80c;
        LogDXError();
#endif
        queueProfileTime3 = 0.0;
        if (!nameDrawFirst) {
            FlushBlitQueue(0);
        }
    }
}

#pragma optimize("a", off)
#endif

#ifdef NETPLAY
extern double D_9751b0;
extern double D_9751a8;
extern double D_9750c8;
extern double D_9754a0;

/* Draws queued objects and records the NET renderer timings. */
void FlushTimedRenderQueues(void)
{
    double profileStart;
    /* The original initializes this frame limit and never reads it. */
    double frameTimeLimit = 1.2;
    double stageTime;

    if (!lpD3DDevice)
        return;

    profileStart = GetProfileTime();
    renderQueueCounter0 = 0;
    renderQueueCounter1 = 0;
    renderQueueCounter2 = 0;
    renderQueueCounter3 = 0;
    stageTime = GetProfileTime();
    DrawRenderQueue();
    D_9751b0 = GetProfileTime() - stageTime;
    stageTime = GetProfileTime();
    SubmitRenderTriangles();
    D_9751a8 = GetProfileTime() - stageTime;
    D_9750c8 = GetProfileTime() - profileStart;
    profileStart = GetProfileTime();
    D_9754a0 = GetProfileTime() - profileStart;
    if (!nameDrawFirst)
        FlushBlitQueue(0);
}
#endif

/* Returns a colour index relative to either colour range of a texture page. */
short GetTextureColour(short page, unsigned char colour, short alternate)
{
#ifdef NETPLAY
    short result = 0;

    page &= 0x1f;
    if (alternate == 0) {
        result = textureRecords[page].colourBase0 + colour;
    } else {
        result = textureRecords[page].colourBase1 + colour;
    }
    return result;
#else
    page &= 0x1f;
    if (alternate == 0) {
        return textureRecords[page].colourBase[0] + colour;
    } else {
        return textureRecords[page].colourBase[1] + colour;
    }
#endif
}


int LoadImageStub(void *rect, unsigned long *pixels)
{
    return 0;
}


#ifndef NETPLAY
int ClearResultAndReturnOne(int *result, int b, int c, int d)
{
    *result = 0;
    switch (d) {
    case 15:
        return 1;
    }
    return 1;
}
#endif
#ifdef NETPLAY
/* The NET callback retains its empty Windows message handlers. */
int ClearResultAndReturnOne(int *handled, LRESULT *result, HWND hwnd, UINT msg,
                            WPARAM wParam, LPARAM lParam)
{
    *handled = 0;
    switch (msg) {
    case WM_MOVE:
        break;
    case WM_SIZE:
        break;
    case WM_ACTIVATE:
        break;
    case WM_SHOWWINDOW:
        break;
    case WM_SETCURSOR:
        break;
    case WM_LBUTTONUP:
        break;
    case WM_RBUTTONUP:
        break;
    case WM_PAINT:
        return 1;
    case WM_ACTIVATEAPP:
        break;
    }
    return 1;
}
#endif


/* Returns the text of a DirectDraw or Direct3D error code. Unknown codes are formatted into the message buffer. */
char *DDErrorString(HRESULT code)
{
    switch (code) {
    case DD_OK:
        return "No error.\n";
    case DDERR_ALREADYINITIALIZED:
        return "This object is already initialized.\n";
    case DDERR_BLTFASTCANTCLIP:
        return "Return if a clipper object is attached to the source surface passed into a BltFast call.\n";
    case DDERR_CANNOTATTACHSURFACE:
        return "This surface can not be attached to the requested surface.\n";
    case DDERR_CANNOTDETACHSURFACE:
        return "This surface can not be detached from the requested surface.\n";
    case DDERR_CANTCREATEDC:
        return "Windows can not create any more DCs.\n";
    case DDERR_CANTDUPLICATE:
        return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.\n";
    case DDERR_CLIPPERISUSINGHWND:
        return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.\n";
    case DDERR_COLORKEYNOTSET:
        return "No src color key specified for this operation.\n";
    case DDERR_CURRENTLYNOTAVAIL:
        return "Support is currently not available.\n";
    case DDERR_DIRECTDRAWALREADYCREATED:
        return "A DirectDraw object representing this driver has already been created for this process.\n";
    case DDERR_EXCEPTION:
        return "An exception was encountered while performing the requested operation.\n";
    case DDERR_EXCLUSIVEMODEALREADYSET:
        return "An attempt was made to set the cooperative level when it was already set to exclusive.\n";
    case DDERR_GENERIC:
        return "Generic failure.\n";
    case DDERR_HEIGHTALIGN:
        return "Height of rectangle provided is not a multiple of reqd alignment.\n";
    case DDERR_HWNDALREADYSET:
        return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.\n";
    case DDERR_HWNDSUBCLASSED:
        return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.\n";
    case DDERR_IMPLICITLYCREATED:
        return "This surface can not be restored because it is an implicitly created surface.\n";
    case DDERR_INCOMPATIBLEPRIMARY:
        return "Unable to match primary surface creation request with existing primary surface.\n";
    case DDERR_INVALIDCAPS:
        return "One or more of the caps bits passed to the callback are incorrect.\n";
    case DDERR_INVALIDCLIPLIST:
        return "DirectDraw does not support the provided cliplist.\n";
    case DDERR_INVALIDDIRECTDRAWGUID:
        return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.\n";
    case DDERR_INVALIDMODE:
        return "DirectDraw does not support the requested mode.\n";
    case DDERR_INVALIDOBJECT:
        return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.\n";
    case DDERR_INVALIDPARAMS:
        return "One or more of the parameters passed to the function are incorrect.\n";
    case DDERR_INVALIDPIXELFORMAT:
        return "The pixel format was invalid as specified.\n";
    case DDERR_INVALIDPOSITION:
        return "Returned when the position of the overlay on the destination is no longer legal for that destination.\n";
    case DDERR_INVALIDRECT:
        return "Rectangle provided was invalid.\n";
    case DDERR_LOCKEDSURFACES:
        return "Operation could not be carried out because one or more surfaces are locked.\n";
    case DDERR_NO3D:
        return "There is no 3D present.\n";
    case DDERR_NOALPHAHW:
        return "Operation could not be carried out because there is no alpha accleration hardware present or available.\n";
    case DDERR_NOBLTHW:
        return "No blitter hardware present.\n";
    case DDERR_NOCLIPLIST:
        return "No cliplist available.\n";
    case DDERR_NOCLIPPERATTACHED:
        return "No clipper object attached to surface object.\n";
    case DDERR_NOCOLORCONVHW:
        return "Operation could not be carried out because there is no color conversion hardware present or available.\n";
    case DDERR_NOCOLORKEY:
        return "Surface doesn't currently have a color key.\n";
    case DDERR_NOCOLORKEYHW:
        return "Operation could not be carried out because there is no hardware support of the destination color key.\n";
    case DDERR_NOCOOPERATIVELEVELSET:
        return "Create function called without DirectDraw object method SetCooperativeLevel being called.\n";
    case DDERR_NODC:
        return "No DC was ever created for this surface.\n";
    case DDERR_NODDROPSHW:
        return "No DirectDraw ROP hardware.\n";
    case DDERR_NODIRECTDRAWHW:
        return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.\n";
    case DDERR_NOEMULATION:
        return "Software emulation not available.\n";
    case DDERR_NOEXCLUSIVEMODE:
        return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.\n";
    case DDERR_NOFLIPHW:
        return "Flipping visible surfaces is not supported.\n";
    case DDERR_NOGDI:
        return "There is no GDI present.\n";
    case DDERR_NOHWND:
        return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.\n";
    case DDERR_NOMIRRORHW:
        return "Operation could not be carried out because there is no hardware present or available.\n";
    case DDERR_NOOVERLAYDEST:
        return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.\n";
    case DDERR_NOOVERLAYHW:
        return "Operation could not be carried out because there is no overlay hardware present or available.\n";
    case DDERR_NOPALETTEATTACHED:
        return "No palette object attached to this surface.\n";
    case DDERR_NOPALETTEHW:
        return "No hardware support for 16 or 256 color palettes.\n";
    case DDERR_NORASTEROPHW:
        return "Operation could not be carried out because there is no appropriate raster op hardware present or available.\n";
    case DDERR_NOROTATIONHW:
        return "Operation could not be carried out because there is no rotation hardware present or available.\n";
    case DDERR_NOSTRETCHHW:
        return "Operation could not be carried out because there is no hardware support for stretching.\n";
    case DDERR_NOT4BITCOLOR:
        return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.\n";
    case DDERR_NOT4BITCOLORINDEX:
        return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.\n";
    case DDERR_NOT8BITCOLOR:
        return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.\n";
    case DDERR_NOTAOVERLAYSURFACE:
        return "Returned when an overlay member is called for a non-overlay surface.\n";
    case DDERR_NOTEXTUREHW:
        return "Operation could not be carried out because there is no texture mapping hardware present or available.\n";
    case DDERR_NOTFLIPPABLE:
        return "An attempt has been made to flip a surface that is not flippable.\n";
    case DDERR_NOTFOUND:
        return "Requested item was not found.\n";
    case DDERR_NOTLOCKED:
        return "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.\n";
    case DDERR_NOTPALETTIZED:
        return "The surface being used is not a palette-based surface.\n";
    case DDERR_NOVSYNCHW:
        return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.\n";
    case DDERR_NOZBUFFERHW:
        return "Operation could not be carried out because there is no hardware support for zbuffer blitting.\n";
    case DDERR_NOZOVERLAYHW:
        return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.\n";
    case DDERR_OUTOFCAPS:
        return "The hardware needed for the requested operation has already been allocated.\n";
    case DDERR_OUTOFMEMORY:
        return "DirectDraw does not have enough memory to perform the operation.\n";
    case DDERR_OUTOFVIDEOMEMORY:
        return "DirectDraw does not have enough memory to perform the operation.\n";
    case DDERR_OVERLAYCANTCLIP:
        return "The hardware does not support clipped overlays.\n";
    case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
        return "Can only have ony color key active at one time for overlays.\n";
    case DDERR_OVERLAYNOTVISIBLE:
        return "Returned when GetOverlayPosition is called on a hidden overlay.\n";
    case DDERR_PALETTEBUSY:
        return "Access to this palette is being refused because the palette is already locked by another thread.\n";
    case DDERR_PRIMARYSURFACEALREADYEXISTS:
        return "This process already has created a primary surface.\n";
    case DDERR_REGIONTOOSMALL:
        return "Region passed to Clipper::GetClipList is too small.\n";
    case DDERR_SURFACEALREADYATTACHED:
        return "This surface is already attached to the surface it is being attached to.\n";
    case DDERR_SURFACEALREADYDEPENDENT:
        return "This surface is already a dependency of the surface it is being made a dependency of.\n";
    case DDERR_SURFACEBUSY:
        return "Access to this surface is being refused because the surface is already locked by another thread.\n";
    case DDERR_SURFACEISOBSCURED:
        return "Access to surface refused because the surface is obscured.\n";
    case DDERR_SURFACELOST:
        return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.\n";
    case DDERR_SURFACENOTATTACHED:
        return "The requested surface is not attached.\n";
    case DDERR_TOOBIGHEIGHT:
        return "Height requested by DirectDraw is too large.\n";
    case DDERR_TOOBIGSIZE:
        return "Size requested by DirectDraw is too large, but the individual height and width are OK.\n";
    case DDERR_TOOBIGWIDTH:
        return "Width requested by DirectDraw is too large.\n";
    case DDERR_UNSUPPORTED:
        return "Action not supported.\n";
    case DDERR_UNSUPPORTEDFORMAT:
        return "FOURCC format requested is unsupported by DirectDraw.\n";
    case DDERR_UNSUPPORTEDMASK:
        return "Bitmask in the pixel format requested is unsupported by DirectDraw.\n";
    case DDERR_VERTICALBLANKINPROGRESS:
        return "Vertical blank is in progress.\n";
    case DDERR_WASSTILLDRAWING:
        return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.\n";
    case DDERR_WRONGMODE:
        return "This surface can not be restored because it was created in a different mode.\n";
    case DDERR_XALIGN:
        return "Rectangle provided was not horizontally aligned on required boundary.\n";
    case D3DERR_BADMAJORVERSION:
        return "D3DERR_BADMAJORVERSION\n";
    case D3DERR_BADMINORVERSION:
        return "D3DERR_BADMINORVERSION\n";
    case D3DERR_EXECUTE_LOCKED:
        return "D3DERR_EXECUTE_LOCKED\n";
    case D3DERR_EXECUTE_NOT_LOCKED:
        return "D3DERR_EXECUTE_NOT_LOCKED\n";
    case D3DERR_EXECUTE_CREATE_FAILED:
        return "D3DERR_EXECUTE_CREATE_FAILED\n";
    case D3DERR_EXECUTE_DESTROY_FAILED:
        return "D3DERR_EXECUTE_DESTROY_FAILED\n";
    case D3DERR_EXECUTE_LOCK_FAILED:
        return "D3DERR_EXECUTE_LOCK_FAILED\n";
    case D3DERR_EXECUTE_UNLOCK_FAILED:
        return "D3DERR_EXECUTE_UNLOCK_FAILED\n";
    case D3DERR_EXECUTE_FAILED:
        return "D3DERR_EXECUTE_FAILED\n";
    case D3DERR_EXECUTE_CLIPPED_FAILED:
        return "D3DERR_EXECUTE_CLIPPED_FAILED\n";
    case D3DERR_TEXTURE_NO_SUPPORT:
        return "D3DERR_TEXTURE_NO_SUPPORT\n";
    case D3DERR_TEXTURE_NOT_LOCKED:
        return "D3DERR_TEXTURE_NOT_LOCKED\n";
    case D3DERR_TEXTURELOCKED:
        return "D3DERR_TEXTURELOCKED\n";
    case D3DERR_TEXTURE_CREATE_FAILED:
        return "D3DERR_TEXTURE_CREATE_FAILED\n";
    case D3DERR_TEXTURE_DESTROY_FAILED:
        return "D3DERR_TEXTURE_DESTROY_FAILED\n";
    case D3DERR_TEXTURE_LOCK_FAILED:
        return "D3DERR_TEXTURE_LOCK_FAILED\n";
    case D3DERR_TEXTURE_UNLOCK_FAILED:
        return "D3DERR_TEXTURE_UNLOCK_FAILED\n";
    case D3DERR_TEXTURE_LOAD_FAILED:
        return "D3DERR_TEXTURE_LOAD_FAILED\n";
    case D3DERR_MATRIX_CREATE_FAILED:
        return "D3DERR_MATRIX_CREATE_FAILED\n";
    case D3DERR_MATRIX_DESTROY_FAILED:
        return "D3DERR_MATRIX_DESTROY_FAILED\n";
    case D3DERR_MATRIX_SETDATA_FAILED:
        return "D3DERR_MATRIX_SETDATA_FAILED\n";
    case D3DERR_SETVIEWPORTDATA_FAILED:
        return "D3DERR_SETVIEWPORTDATA_FAILED\n";
    case D3DERR_MATERIAL_CREATE_FAILED:
        return "D3DERR_MATERIAL_CREATE_FAILED\n";
    case D3DERR_MATERIAL_DESTROY_FAILED:
        return "D3DERR_MATERIAL_DESTROY_FAILED\n";
    case D3DERR_MATERIAL_SETDATA_FAILED:
        return "D3DERR_MATERIAL_SETDATA_FAILED\n";
    case D3DERR_LIGHT_SET_FAILED:
        return "D3DERR_LIGHT_SET_FAILED\n";
    default:
        wsprintfA(g_cdMessage, "Unrecognized error value (%08lx).\n", code);
        return g_cdMessage;
    }
}

#ifdef NETPLAY
/* Creates the DirectDraw objects for the selected driver: display mode, primary and back surfaces, z-buffer and palette. */
#pragma intrinsic(memcpy)
void CreateDirectDrawObjects(void)
{
    DDSURFACEDESC surfaceDesc;
    DDSCAPS caps;
    unsigned long depthMask;
    HDC desktopDC;
    int i;

    if (displayDrivers[videoModeIndex].devices[pixelFormatGroup].hardwareCaps[1] /* dwFlags */) {
        nameDrawFirst = 1;
        hudTexturePage = 0;
    } else {
        nameDrawFirst = 0;
        hudTexturePage = 1;
    }
    ddResult = DirectDrawCreate(displayDrivers[videoModeIndex].guid, &lpDD, NULL);
    dxErrorLine = 0xcdd;
    LogDXError();
    ddResult = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **)&lpDD2);
    dxErrorLine = 0xcde;
    LogDXError();
    ddResult = IDirectDraw_SetCooperativeLevel(lpDD, g_hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWMODEX);
    dxErrorLine = 0xce2;
    LogDXError();
    fullscreenDisplay = 1;
    viewScaleX = screenScaleX / 320.0f;
    viewScaleY = screenScaleY / 240.0f;
    ddResult = lpDD2->lpVtbl->SetDisplayMode(lpDD2, (int)screenScaleX, (int)screenScaleY, displayBits, 0, 0);
    dxErrorLine = 0xcec;
    LogDXError();
    surfaceDesc.dwSize = sizeof(surfaceDesc);
    surfaceDesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
    surfaceDesc.dwBackBufferCount = 1;
    ddResult = IDirectDraw_CreateSurface(lpDD, &surfaceDesc, &primarySurface, NULL);
    dxErrorLine = 0xd18;
    LogDXError();
    caps.dwCaps = DDSCAPS_BACKBUFFER;
    ddResult = IDirectDrawSurface_GetAttachedSurface(primarySurface, &caps, &backBuffer);
    dxErrorLine = 0xd1b;
    LogDXError();
    if (nameDrawFirst != 0) {
        depthMask = displayDrivers[videoModeIndex].devices[pixelFormatGroup].hardwareCaps[40] /* dwDeviceZBufferBitDepth */;
        memcpy(activeDeviceCaps, displayDrivers[videoModeIndex].devices[pixelFormatGroup].hardwareCaps, sizeof(activeDeviceCaps));
        surfaceDesc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    } else {
        depthMask = displayDrivers[videoModeIndex].devices[pixelFormatGroup].softwareCaps[40] /* dwDeviceZBufferBitDepth */;
        memcpy(activeDeviceCaps, displayDrivers[videoModeIndex].devices[pixelFormatGroup].softwareCaps, sizeof(activeDeviceCaps));
        surfaceDesc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
        surfaceDesc.ddsCaps.dwCaps |= DDSCAPS_ZBUFFER;
    }
    if (depthMask & DDBD_32) {
        surfaceDesc.dwZBufferBitDepth = 32;
    } else if (depthMask & DDBD_24) {
        surfaceDesc.dwZBufferBitDepth = 24;
    } else if (depthMask & DDBD_16) {
        surfaceDesc.dwZBufferBitDepth = 16;
    } else if (depthMask & DDBD_8) {
        surfaceDesc.dwZBufferBitDepth = 8;
    }
    surfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_ZBUFFERBITDEPTH;
    surfaceDesc.dwHeight = renderSize.cy;
    surfaceDesc.dwWidth = renderSize.cx;
    ddResult = IDirectDraw_CreateSurface(lpDD, &surfaceDesc, &zBufferSurface, NULL);
    dxErrorLine = 0xd44;
    LogDXError();
    ddResult = IDirectDrawSurface_AddAttachedSurface(backBuffer, zBufferSurface);
    dxErrorLine = 0xd54;
    LogDXError();
    ppmPixelFormat.dwSize = sizeof(ppmPixelFormat);
    ddResult = IDirectDrawSurface_GetPixelFormat(primarySurface, &ppmPixelFormat);
    dxErrorLine = 0xd5d;
    LogDXError();
    if (displayBits != 8) {
        SetPixelShifts();
    }
    usingSystemPalette = 0;
    if (nameDrawFirst == 0 && displayBits == 16) {
        mmxAvailable = HasMMX();
        if (mmxAvailable) {
            texture16Bit = 0;
        }
    }
    if ((ppmPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) || texture16Bit == 0) {
        usingSystemPalette = 1;
        desktopDC = GetDC(NULL);
        GetSystemPaletteEntries(desktopDC, 0, 256, systemPalette);
        ReleaseDC(NULL, desktopDC);
        if (fullscreenDisplay == 0) {
            for (i = 0; i < 10; i++)
                systemPalette[i].peFlags = 0x40;
            for (i = 10; i < 246; i++)
                systemPalette[i].peFlags = 0;
            for (i = 246; i < 256; i++)
                systemPalette[i].peFlags = 0x40;
        } else {
            systemPalette[0].peFlags = 0x40;
            systemPalette[0].peRed = 0;
            systemPalette[0].peGreen = 0;
            systemPalette[0].peBlue = 0;
            for (i = 1; i < 255; i++) {
                systemPalette[i].peFlags = 0;
                systemPalette[i].peRed = 0;
                systemPalette[i].peGreen = 0;
                systemPalette[i].peBlue = 0;
            }
            systemPalette[255].peFlags = 0x40;
            systemPalette[255].peRed = 0;
            systemPalette[255].peGreen = 0;
            systemPalette[255].peBlue = 0;
        }
        ddResult = IDirectDraw_CreatePalette(lpDD, DDPCAPS_8BIT | DDPCAPS_INITIALIZE, systemPalette, &windowPalette, NULL);
        dxErrorLine = 0xd9b;
        LogDXError();
        if (rgbTextures == 0) {
            ddResult = IDirectDrawSurface_SetPalette(backBuffer, windowPalette);
            dxErrorLine = 0xda1;
            LogDXError();
            ddResult = IDirectDrawSurface_SetPalette(primarySurface, windowPalette);
            dxErrorLine = 0xda2;
            LogDXError();
        }
    }
}
#pragma function(memcpy)
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Differs in register allocation and instruction scheduling only (70% similar). */
/* Creates the DirectDraw objects for the selected driver: display mode, primary and back surfaces, z-buffer and palette. */
void CreateDirectDrawObjects(void)
{
    DDSURFACEDESC surfaceDesc;
    DDSCAPS caps;
    unsigned long zBufferDepths;
    HDC screenDC;
    unsigned char *flags;
    int i;

    if (displayDrivers[videoModeIndex].devices[pixelFormatGroup].hardwareCaps[1] /* dwFlags */) {
        nameDrawFirst = 1;
        hudTexturePage = 0;
    } else {
        nameDrawFirst = 0;
        hudTexturePage = 1;
    }
    ddResult = DirectDrawCreateThunk(displayDrivers[videoModeIndex].guid, &lpDD, NULL);
    dxErrorLine = 0xa42;
    LogDXError();
    ddResult = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **)&lpDD2);
    dxErrorLine = 0xa43;
    LogDXError();
    ddResult = IDirectDraw_SetCooperativeLevel(lpDD, g_hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWMODEX);
    dxErrorLine = 0xa47;
    LogDXError();
    viewScaleX = screenScaleX * 0.003125f;
    viewScaleY = screenScaleY * 0.004166667f;
    fullscreenDisplay = 1;
    ddResult = lpDD2->lpVtbl->SetDisplayMode(lpDD2, (int)screenScaleX, (int)screenScaleY, displayBits, 0, 0);
    dxErrorLine = 0xa4e;
    LogDXError();
    surfaceDesc.dwSize = sizeof(surfaceDesc);
    surfaceDesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surfaceDesc.dwBackBufferCount = 1;
    surfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
    ddResult = IDirectDraw_CreateSurface(lpDD, &surfaceDesc, &primarySurface, NULL);
    dxErrorLine = 0xa55;
    LogDXError();
    caps.dwCaps = DDSCAPS_BACKBUFFER;
    ddResult = IDirectDrawSurface_GetAttachedSurface(primarySurface, &caps, &backBuffer);
    dxErrorLine = 0xa58;
    LogDXError();
    if (nameDrawFirst != 0) {
        zBufferDepths = displayDrivers[videoModeIndex].devices[pixelFormatGroup].hardwareCaps[40] /* dwDeviceZBufferBitDepth */;
        memcpy(activeDeviceCaps, displayDrivers[videoModeIndex].devices[pixelFormatGroup].hardwareCaps, sizeof(activeDeviceCaps));
        surfaceDesc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    } else {
        zBufferDepths = displayDrivers[videoModeIndex].devices[pixelFormatGroup].softwareCaps[40] /* dwDeviceZBufferBitDepth */;
        memcpy(activeDeviceCaps, displayDrivers[videoModeIndex].devices[pixelFormatGroup].softwareCaps, sizeof(activeDeviceCaps));
        surfaceDesc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_SYSTEMMEMORY;
    }
    if (zBufferDepths & DDBD_32) {
        surfaceDesc.dwZBufferBitDepth = 32;
    } else if (zBufferDepths & DDBD_24) {
        surfaceDesc.dwZBufferBitDepth = 24;
    } else if (zBufferDepths & DDBD_16) {
        surfaceDesc.dwZBufferBitDepth = 16;
    } else if (zBufferDepths & DDBD_8) {
        surfaceDesc.dwZBufferBitDepth = 8;
    }
    surfaceDesc.dwHeight = renderSize.cy;
    surfaceDesc.dwWidth = renderSize.cx;
    surfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_ZBUFFERBITDEPTH;
    ddResult = IDirectDraw_CreateSurface(lpDD, &surfaceDesc, &zBufferSurface, NULL);
    dxErrorLine = 0xa7f;
    LogDXError();
    ddResult = IDirectDrawSurface_AddAttachedSurface(backBuffer, zBufferSurface);
    dxErrorLine = 0xa81;
    LogDXError();
    ppmPixelFormat.dwSize = sizeof(ppmPixelFormat);
    ddResult = IDirectDrawSurface_GetPixelFormat(primarySurface, &ppmPixelFormat);
    dxErrorLine = 0xa87;
    LogDXError();
    if (displayBits != 8) {
        SetPixelShifts();
    }
    usingSystemPalette = 0;
    if (nameDrawFirst == 0 && displayBits == 16) {
        mmxAvailable = HasMMX();
        if (mmxAvailable) {
            texture16Bit = 0;
        }
    }
    if ((ppmPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) || texture16Bit == 0) {
        usingSystemPalette = 1;
        screenDC = GetDC(NULL);
        GetSystemPaletteEntries(screenDC, 0, 256, systemPalette);
        ReleaseDC(NULL, screenDC);
        if (fullscreenDisplay == 0) {
            for (flags = &systemPalette[0].peFlags; flags < &systemPalette[10].peFlags; flags += 4) {
                *flags = 0x40;
            }
            for (flags = &systemPalette[10].peFlags; flags < &systemPalette[246].peFlags; flags += 4) {
                *flags = 0;
            }
            for (flags = &systemPalette[246].peFlags; flags < &systemPalette[256].peFlags; flags += 4) {
                *flags = 0x40;
            }
        } else {
            systemPalette[0].peRed = 0;
            systemPalette[0].peGreen = 0;
            systemPalette[0].peBlue = 0;
            systemPalette[0].peFlags = 0x40;
            for (i = 1; i < 255; i++) {
                systemPalette[i].peFlags = 0;
                systemPalette[i].peRed = 0;
                systemPalette[i].peGreen = 0;
                systemPalette[i].peBlue = 0;
            }
            systemPalette[255].peFlags = 0x40;
            systemPalette[255].peRed = 0;
            systemPalette[255].peGreen = 0;
            systemPalette[255].peBlue = 0;
        }
        ddResult = IDirectDraw_CreatePalette(lpDD, DDPCAPS_8BIT | DDPCAPS_INITIALIZE, systemPalette, &windowPalette, NULL);
        dxErrorLine = 0xac5;
        LogDXError();
        if (rgbTextures == 0) {
            ddResult = IDirectDrawSurface_SetPalette(backBuffer, windowPalette);
            dxErrorLine = 0xacb;
            LogDXError();
            ddResult = IDirectDrawSurface_SetPalette(primarySurface, windowPalette);
            dxErrorLine = 0xacc;
            LogDXError();
        }
    }
}
#endif
#endif

#ifdef NETPLAY
/* Releases the DirectDraw objects. */
void ReleaseDirectDrawObjects(void)
{
    OutputDebugStringA("Shutting it all down\n");
    if (lpDD) {
        if (primarySurface) {
            if (windowPalette) {
                IDirectDrawPalette_Release(windowPalette);
            }
            IDirectDrawSurface_Release(primarySurface);
            primarySurface = NULL;
        }
        ddResult = IDirectDraw_RestoreDisplayMode(lpDD);
        dxErrorLine = 0xdb9;
        LogDXError();
        ddResult = IDirectDraw_SetCooperativeLevel(lpDD, g_hwnd, DDSCL_NORMAL);
        dxErrorLine = 0xdba;
        LogDXError();
        IDirectDraw_Release(lpDD);
        IDirectDraw_Release(lpDD);
    }
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Register choice differs in the SetCooperativeLevel call. */
/* Releases the DirectDraw objects. */
void ReleaseDirectDrawObjects(void)
{
    OutputDebugStringA("Shutting it all down\n");
    if (lpDD) {
        if (primarySurface) {
            if (windowPalette) {
                IDirectDrawPalette_Release(windowPalette);
            }
            IDirectDrawSurface_Release(primarySurface);
            primarySurface = NULL;
        }
        ddResult = IDirectDraw_RestoreDisplayMode(lpDD);
        dxErrorLine = 0xae3;
        LogDXError();
        ddResult = IDirectDraw_SetCooperativeLevel(lpDD, g_hwnd, DDSCL_NORMAL);
        dxErrorLine = 0xae4;
        LogDXError();
        IDirectDraw_Release(lpDD);
        IDirectDraw_Release(lpDD);
    }
}
#endif
#endif

#ifdef EQUIV
/* The executable caches rendering flags between calls and retains preliminary buffer-size stores
   that the later final assignment overwrites before any read or call. */
/* Creates the Direct3D device, viewport, texture pages and execute buffers, then installs the keyboard hook.
   Returns 0x3039 when the device cannot be created and 1 when the back buffer provides none. */
#pragma optimize("a", on)
#ifndef NETPLAY
int CreateDirect3DDevice(void)
{
    DWORD softwareCaps[43];
    DWORD hardwareCaps[43];
    DDSURFACEDESC plainDesc;
    DDSURFACEDESC textureDesc;
    DDSURFACEDESC sourceDesc;
    DDCOLORKEY colourKey;
    DWORD totalMemory;
    DDSCAPS caps;
    DWORD freeMemory;
    int i;
    int bufferSize;
    TextureFormat *format;
    TextureRecord *record;
    RenderExecuteBuffer **buffer;
    int pageCount;

    ddResult = IDirectDraw_QueryInterface(lpDD, &IID_IDirect3D, (void **)&direct3D);
#ifdef NETPLAY
    dxErrorLine = 0xdd9;
#else
    dxErrorLine = 0xb00;
#endif
    LogDXError();
    if (direct3D) {
        ddResult = IDirectDrawSurface_QueryInterface(backBuffer, displayDrivers[videoModeIndex].devices[pixelFormatGroup].guid, (void **)&lpD3DDevice);
        if (ddResult) {
            return 0x3039;
        }
        if (lpD3DDevice != NULL) {
            hardwareCaps[0] = sizeof(hardwareCaps);
            softwareCaps[0] = sizeof(softwareCaps);
            ddResult = lpD3DDevice->lpVtbl->GetCaps(lpD3DDevice, &hardwareCaps, &softwareCaps);
#ifdef NETPLAY
            dxErrorLine = 0xdeb;
#else
            dxErrorLine = 0xb10;
#endif
            LogDXError();
            ddResult = direct3D->lpVtbl->CreateViewport(direct3D, &renderViewport, NULL);
#ifdef NETPLAY
            dxErrorLine = 0xdef;
#else
            dxErrorLine = 0xb14;
#endif
            LogDXError();
            ddResult = lpD3DDevice->lpVtbl->AddViewport(lpD3DDevice, renderViewport);
#ifdef NETPLAY
            dxErrorLine = 0xdf0;
#else
            dxErrorLine = 0xb15;
#endif
            LogDXError();
            renderViewportData.dwX = 0;
            renderViewportData.dwY = 0;
            renderViewportData.dwSize = sizeof(renderViewportData);
            renderViewportData.dwWidth = (int)screenScaleX;
            renderViewportData.dwHeight = (int)screenScaleY;
            renderViewportData.dvScaleX = screenScaleX;
            renderViewportData.dvScaleY = screenScaleY;
            renderViewportData.dvMaxX = renderViewportData.dwWidth / (screenScaleX * 2.0f);
            renderViewportData.dvMaxY = renderViewportData.dwHeight / (screenScaleY * 2.0f);
            renderViewportData.dvMinZ = 0.00066666666f;
            renderViewportData.dvMaxZ = 1.0f;
            ddResult = renderViewport->lpVtbl->SetViewport(renderViewport, &renderViewportData);
#ifdef NETPLAY
            dxErrorLine = 0xdfe;
#else
            dxErrorLine = 0xb23;
#endif
            LogDXError();

            memset(&sourceDesc, 0, sizeof(sourceDesc));
            memset(&textureDesc, 0, sizeof(textureDesc));
            memset(&plainDesc, 0, sizeof(plainDesc));
            plainDesc.dwSize = sizeof(plainDesc);
            textureDesc.dwSize = sizeof(textureDesc);
            sourceDesc.dwSize = sizeof(sourceDesc);
            plainDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
            textureDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
            plainDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
            sourceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
            textureDesc.dwHeight = texturePageSize;
            sourceDesc.dwHeight = texturePageSize;
            textureDesc.dwWidth = texturePageSize;
            sourceDesc.dwWidth = texturePageSize;
            textureDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
            if (nameDrawFirst == 0) {
                textureDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
            }
            plainDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            textureDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            sourceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            bestBlueBits = 0;
            bestGreenBits = 0;
            bestRedBits = 0;
            sourceDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
            ddResult = lpD3DDevice->lpVtbl->EnumTextureFormats(lpD3DDevice, EnumTextureFormatCallback, &sourceDesc.ddpfPixelFormat);
#ifdef NETPLAY
            dxErrorLine = 0xe19;
#else
            dxErrorLine = 0xb3e;
#endif
            LogDXError();
            if (texture16Bit) {
                textureDesc.ddpfPixelFormat = sourceDesc.ddpfPixelFormat;
                plainDesc.ddpfPixelFormat = ppmPixelFormat;
            } else {
                for (i = 0; i < textureFormatCount; i++) {
                    if (textureFormats[i].kind == 8) {
                        format = &textureFormats[i];
                        sourceDesc.ddpfPixelFormat = format->pixelFormat;
                        textureDesc.ddpfPixelFormat = format->pixelFormat;
                        plainDesc.ddpfPixelFormat = ppmPixelFormat;
                        break;
                    }
                }
            }
            if (!(activeDeviceCaps[33] & 8)) {
                blendByBucket = 0;
            }
            if (blendByBucket == 0) {
                if (!(gameFlags & 2)) {
                    typeSwitchesMode = 1;
                }
            }
            if (gameFlags & 2) {
                particleAlpha = 255;
                translucentAlpha = 255;
            } else {
                particleAlpha = 176;
                translucentAlpha = 0;
            }
            opaqueAlpha = 255;
            vertexAlpha = 255;
            if (blendByBucket == 0) {
                for (i = 0; i < textureFormatCount; i++) {
                    if (textureFormats[i].kind == 16 && textureFormats[i].shifts.redBits == 5
                        && textureFormats[i].shifts.greenBits == 5 && textureFormats[i].shifts.blueBits == 5
                        && textureFormats[i].shifts.alphaBits == 1) {
                        format = &textureFormats[i];
                        textureDesc.ddpfPixelFormat = format->pixelFormat;
                        plainDesc.ddpfPixelFormat = ppmPixelFormat;
                        sourceDesc.ddpfPixelFormat = format->pixelFormat;
                        argb1555Format = i;
                        break;
                    }
                }
            }
            bestRedDown = 8 - bestRedBits;
            bestGreenDown = 8 - bestGreenBits;
            bestBlueDown = 8 - bestBlueBits;
            textureDesc.ddsCaps.dwCaps |= DDSCAPS_ALLOCONLOAD;
            caps.dwCaps = DDSCAPS_TEXTURE;
            if (nameDrawFirst) {
                ddResult = IDirectDraw2_GetAvailableVidMem(lpDD2, &caps, &totalMemory, &freeMemory);
#ifdef NETPLAY
                dxErrorLine = 0xe82;
#else
                dxErrorLine = 0xba7;
#endif
                LogDXError();
            }
            pageCount = freeMemory >> 17;
            for (i = 0; i < 26; i++) {
                textureRecords[i].memorySize = 0x10000;
                textureRecords[i].level = 0;
                textureRecords[i].width = 256;
                textureRecords[i].height = 256;
                if (blendByBucket == 0) {
                    if (i >= 0 && i <= 4) {
                        textureRecords[i].type = 1;
                        textureRecords[i].typeVariant = 0;
                    } else if (i >= 5 && i <= 10) {
                        textureRecords[i].type = 2;
                        textureRecords[i].typeVariant = 0;
                    } else if (i >= 15 && i <= 16) {
                        textureRecords[i].type = 3;
                        textureRecords[i].typeVariant = 1;
                    } else if (i >= 18 && i <= 21) {
                        textureRecords[i].type = 3;
                        textureRecords[i].typeVariant = 1;
                    } else if (i == 22 || i == 23) {
                        textureRecords[i].type = 3;
                        textureRecords[i].typeVariant = 0;
                    } else if (i == 24) {
                        textureRecords[i].type = 3;
                        textureRecords[i].typeVariant = 1;
                    } else if (i == 25) {
                        textureRecords[i].type = 3;
                        textureRecords[i].typeVariant = 0;
                    } else {
                        textureRecords[i].type = 0;
                        textureRecords[i].typeVariant = 0;
                    }
                    textureRecords[i].reserved2a = 0;
                }
            }
            if (nameDrawFirst && (short)pageCount < 26) {
                i = 0;
                do {
                    caps.dwCaps = DDSCAPS_TEXTURE;
                    ddResult = IDirectDraw2_GetAvailableVidMem(lpDD2, &caps, &totalMemory, &freeMemory);
#ifdef NETPLAY
                    dxErrorLine = 0xf01;
#else
                    dxErrorLine = 0xc26;
#endif
                    LogDXError();
                    bufferSize = 0;
                    for (record = textureRecords; record < textureRecords + 26; record++) {
                        bufferSize += record->memorySize * 2;
                    }
                    if ((DWORD)bufferSize < freeMemory) {
                        break;
                    }
                    if (textureRecords[i].level < 2) {
                        textureRecords[i].level++;
                    }
                    switch (textureRecords[i].level) {
                    case 0:
                        textureRecords[i].memorySize = 0x10000;
                        textureRecords[i].width = 256;
                        textureRecords[i].height = 256;
                        break;
                    case 1:
                        textureRecords[i].memorySize = 0x4000;
                        textureRecords[i].width = 128;
                        textureRecords[i].height = 128;
                        break;
                    case 2:
                        textureRecords[i].memorySize = 0x1000;
                        textureRecords[i].width = 64;
                        textureRecords[i].height = 64;
                        break;
                    }
                    i++;
                    if (i == 26) {
                        i = 0;
                    }
                } while ((short)pageCount < 26);
            }
            normalSetNo = 22;
            colourKey.dwColorSpaceLowValue = 0;
            colourKey.dwColorSpaceHighValue = 0;
            {
                int page;

                for (page = 0; page < 26; page++) {
                    caps.dwCaps = DDSCAPS_TEXTURE;
                    if (nameDrawFirst) {
                        ddResult = IDirectDraw2_GetAvailableVidMem(lpDD2, &caps, &totalMemory, &freeMemory);
#ifdef NETPLAY
                        dxErrorLine = 0xf35;
#else
                        dxErrorLine = 0xc5a;
#endif
                        LogDXError();
                        wsprintfA(g_cdMessage, "Memory - Total:%ld, Free:%ld\n", totalMemory, freeMemory);
                        OutputDebugStringA(g_cdMessage);
                    }
                    if (blendByBucket == 0) {
                        if (textureRecords[page].type == 3) {
                            textureDesc.ddpfPixelFormat = textureFormats[argb1555Format].pixelFormat;
                            plainDesc.ddpfPixelFormat = ppmPixelFormat;
                            sourceDesc.ddpfPixelFormat = textureFormats[argb1555Format].pixelFormat;
                        } else {
                            textureDesc.ddpfPixelFormat = textureFormats[bestTextureFormat].pixelFormat;
                            plainDesc.ddpfPixelFormat = ppmPixelFormat;
                            sourceDesc.ddpfPixelFormat = textureFormats[bestTextureFormat].pixelFormat;
                        }
                    }
                    wsprintfA(g_cdMessage, "Making texture page %d\n", page);
                    OutputDebugStringA(g_cdMessage);
                    plainDesc.dwHeight = texturePageSize;
                    sourceDesc.dwHeight = texturePageSize;
                    plainDesc.dwWidth = texturePageSize;
                    sourceDesc.dwWidth = texturePageSize;
                    if (texture16Bit) {
                        ddResult = IDirectDraw_CreateSurface(lpDD, &sourceDesc, &textureRecords[page].surfaces[0], NULL);
#ifdef NETPLAY
                        dxErrorLine = 0xf60;
#else
                        dxErrorLine = 0xc85;
#endif
                        LogDXError();
                    }
                    sourceDesc.dwHeight = textureRecords[page].height;
                    sourceDesc.dwWidth = textureRecords[page].width;
                    textureDesc.dwHeight = textureRecords[page].height;
                    textureDesc.dwWidth = textureRecords[page].width;
                    if (texture16Bit) {
                        ddResult = IDirectDraw_CreateSurface(lpDD, &sourceDesc, &textureRecords[page].surfaces[1], NULL);
#ifdef NETPLAY
                        dxErrorLine = 0xf74;
#else
                        dxErrorLine = 0xc99;
#endif
                        LogDXError();
                    }
                    ddResult = IDirectDraw_CreateSurface(lpDD, &textureDesc, &textureRecords[page].surfaces[2], NULL);
#ifdef NETPLAY
                    dxErrorLine = 0xf7a;
#else
                    dxErrorLine = 0xc9f;
#endif
                    LogDXError();
                    if (texture16Bit == 0) {
                        ddResult = IDirectDrawSurface_SetPalette(textureRecords[page].surfaces[2], windowPalette);
#ifdef NETPLAY
                        dxErrorLine = 0xf7c;
#else
                        dxErrorLine = 0xca1;
#endif
                        LogDXError();
                    }
                    if (texture16Bit && blendByBucket) {
                        ddResult = IDirectDrawSurface_SetColorKey(textureRecords[page].surfaces[0], DDCKEY_SRCBLT, &colourKey);
#ifdef NETPLAY
                        dxErrorLine = 0xf83;
#else
                        dxErrorLine = 0xca8;
#endif
                        LogDXError();
                        ddResult = IDirectDrawSurface_SetColorKey(textureRecords[page].surfaces[1], DDCKEY_SRCBLT, &colourKey);
#ifdef NETPLAY
                        dxErrorLine = 0xf84;
#else
                        dxErrorLine = 0xca9;
#endif
                        LogDXError();
                    }
                    if (blendByBucket) {
                        ddResult = IDirectDrawSurface_SetColorKey(textureRecords[page].surfaces[2], DDCKEY_SRCBLT, &colourKey);
#ifdef NETPLAY
                        dxErrorLine = 0xf8a;
#else
                        dxErrorLine = 0xcaf;
#endif
                        LogDXError();
                    }
                    if (texture16Bit) {
                        ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[0], &IID_IDirect3DTexture, (void **)&surfaceTextures0[page]);
#ifdef NETPLAY
                        dxErrorLine = 0xf97;
#else
                        dxErrorLine = 0xcbc;
#endif
                        LogDXError();
                        ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[1], &IID_IDirect3DTexture, (void **)&surfaceTextures1[page]);
#ifdef NETPLAY
                        dxErrorLine = 0xf98;
#else
                        dxErrorLine = 0xcbd;
#endif
                        LogDXError();
                    }
                    ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[2], &IID_IDirect3DTexture, (void **)&surfaceTextures2[page]);
#ifdef NETPLAY
                    dxErrorLine = 0xf9a;
#else
                    dxErrorLine = 0xcbf;
#endif
                    LogDXError();
                }
            }
            hardwareCaps[0] = sizeof(hardwareCaps);
            softwareCaps[0] = sizeof(softwareCaps);
            ddResult = lpD3DDevice->lpVtbl->GetCaps(lpD3DDevice, &hardwareCaps, &softwareCaps);
#ifdef NETPLAY
            dxErrorLine = 0xfa1;
#else
            dxErrorLine = 0xcc6;
#endif
            LogDXError();
            bufferSize = 0x10000;
            if (nameDrawFirst) {
                if (hardwareCaps[41]) {
                    vertexBufferSize = 0x10000;
                    if (hardwareCaps[41] < 0x10000) {
                        bufferSize = hardwareCaps[41];
                    }
                }
            } else {
                if (softwareCaps[41]) {
                    vertexBufferSize = 0x10000;
                    if (softwareCaps[41] < 0x10000) {
                        bufferSize = softwareCaps[41];
                    }
                }
            }
            vertexBufferSize = bufferSize;
            executeBuffersCreated = 1;
            memset(&bufferDescriptionTemplate, 0, sizeof(bufferDescriptionTemplate));
            bufferDescriptionTemplate.flags = 1;
            bufferDescriptionTemplate.bufferSize = bufferSize;
            bufferDescriptionTemplate.size = sizeof(bufferDescriptionTemplate);
            for (buffer = executeBuffers; buffer < executeBuffers + 50; buffer++) {
                ddResult = lpD3DDevice->lpVtbl->CreateExecuteBuffer(lpD3DDevice, &bufferDescriptionTemplate, (IDirect3DExecuteBuffer **)buffer, NULL);
#ifdef NETPLAY
                dxErrorLine = 0xfc0;
#else
                dxErrorLine = 0xce5;
#endif
                LogDXError();
            }
        } else {
            return 1;
        }
    }
    if (nameDrawFirst) {
        ddResult = IDirectDraw2_GetAvailableVidMem(lpDD2, &caps, &totalMemory, &freeMemory);
#ifdef NETPLAY
        dxErrorLine = 0xfcc;
#else
        dxErrorLine = 0xcf1;
#endif
        LogDXError();
        wsprintfA(g_cdMessage, "Memory - Total:%ld, Free:%ld\n", totalMemory, freeMemory);
        OutputDebugStringA(g_cdMessage);
    }
    windowHook = SetWindowsHookExA(WH_KEYBOARD, KeyboardHookProc, g_hInstance, GetCurrentThreadId());
    return 0;
}
#endif
#pragma optimize("a", off)
#endif

#ifdef NETPLAY
int CreateDirect3DDevice(void)
{
    DDSCAPS memoryFlags;
    DDSURFACEDESC plainDesc;
    DWORD emulatedCaps[43];
    DWORD hardwareDeviceCaps[43];
    DDCOLORKEY key;
    DDSURFACEDESC sourceSurface;
    int i;
    DWORD videoMemoryTotal;
    DDSURFACEDESC textureSurfaceDesc;
    DWORD videoMemoryFree;
    int j;
    DWORD requiredTextureBytes;
    short texturePageCount;
    int page;

    requiredTextureBytes = 0;
    ddResult = IDirectDraw_QueryInterface(lpDD, &IID_IDirect3D, (void **)&direct3D);
    dxErrorLine = 0xdd9;
    LogDXError();
    if (direct3D) {
        ddResult = IDirectDrawSurface_QueryInterface(backBuffer, displayDrivers[videoModeIndex].devices[pixelFormatGroup].guid, (void **)&lpD3DDevice);
        dxErrorLine = 0xddf;
        ReportDDError();
        if (ddResult) {
            return 0x3039;
        }
        if (lpD3DDevice != NULL) {
            hardwareDeviceCaps[0] = sizeof(hardwareDeviceCaps);
            emulatedCaps[0] = sizeof(emulatedCaps);
            ddResult = lpD3DDevice->lpVtbl->GetCaps(lpD3DDevice, &hardwareDeviceCaps, &emulatedCaps);
            dxErrorLine = 0xdeb;
            LogDXError();
            ddResult = direct3D->lpVtbl->CreateViewport(direct3D, &renderViewport, NULL);
            dxErrorLine = 0xdef;
            LogDXError();
            ddResult = lpD3DDevice->lpVtbl->AddViewport(lpD3DDevice, renderViewport);
            dxErrorLine = 0xdf0;
            LogDXError();
            renderViewportData.dwSize = sizeof(renderViewportData);
            renderViewportData.dwX = 0;
            renderViewportData.dwY = 0;
            renderViewportData.dwWidth = (int)screenScaleX;
            renderViewportData.dwHeight = (int)screenScaleY;
            renderViewportData.dvScaleX = screenScaleX;
            renderViewportData.dvScaleY = screenScaleY;
            renderViewportData.dvMaxX = renderViewportData.dwWidth / (renderViewportData.dvScaleX * 2.0f);
            renderViewportData.dvMaxY = renderViewportData.dwHeight / (renderViewportData.dvScaleY * 2.0f);
            renderViewportData.dvMinZ = 0.00066666666f;
            renderViewportData.dvMaxZ = 1.0f;
            ddResult = renderViewport->lpVtbl->SetViewport(renderViewport, &renderViewportData);
            dxErrorLine = 0xdfe;
            LogDXError();

            memset(&sourceSurface, 0, sizeof(sourceSurface));
            memset(&textureSurfaceDesc, 0, sizeof(textureSurfaceDesc));
            memset(&plainDesc, 0, sizeof(plainDesc));
            sourceSurface.dwSize = textureSurfaceDesc.dwSize = plainDesc.dwSize = sizeof(plainDesc);
            sourceSurface.dwFlags = textureSurfaceDesc.dwFlags = plainDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
            sourceSurface.dwHeight = textureSurfaceDesc.dwHeight = texturePageSize;
            sourceSurface.dwWidth = textureSurfaceDesc.dwWidth = texturePageSize;
            sourceSurface.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
            textureSurfaceDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            plainDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
            if (nameDrawFirst) {
                textureSurfaceDesc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
            } else {
                textureSurfaceDesc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
            }
            sourceSurface.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
            sourceSurface.ddpfPixelFormat.dwSize = textureSurfaceDesc.ddpfPixelFormat.dwSize = plainDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            bestRedBits = bestGreenBits = bestBlueBits = 0;
            ddResult = lpD3DDevice->lpVtbl->EnumTextureFormats(lpD3DDevice, EnumTextureFormatCallback, &sourceSurface.ddpfPixelFormat);
            dxErrorLine = 0xe19;
            LogDXError();
            if (texture16Bit) {
                textureSurfaceDesc.ddpfPixelFormat = sourceSurface.ddpfPixelFormat;
                plainDesc.ddpfPixelFormat = ppmPixelFormat;
            } else {
                for (i = 0; i < textureFormatCount; i++) {
                    if (textureFormats[i].kind == 8) {
                        sourceSurface.ddpfPixelFormat = textureFormats[i].pixelFormat;
                        textureSurfaceDesc.ddpfPixelFormat = textureFormats[i].pixelFormat;
                        plainDesc.ddpfPixelFormat = ppmPixelFormat;
                        break;
                    }
                }
            }
            if (!(activeDeviceCaps[33] & 8)) {
                blendByBucket = 0;
            }
            if (blendByBucket == 0) {
                if (!(gameFlags & 2)) {
                    typeSwitchesMode = 1;
                }
            }
            if (gameFlags & 2) {
                opaqueAlpha = 255;
                particleAlpha = 255;
                translucentAlpha = 255;
            } else {
                opaqueAlpha = 255;
                particleAlpha = 176;
                translucentAlpha = 0;
            }
            vertexAlpha = opaqueAlpha;
            if (blendByBucket == 0) {
                for (i = 0; i < textureFormatCount; i++) {
                    if (textureFormats[i].kind == 16 && textureFormats[i].shifts.redBits == 5
                        && textureFormats[i].shifts.greenBits == 5 && textureFormats[i].shifts.blueBits == 5
                        && textureFormats[i].shifts.alphaBits == 1) {
                        textureSurfaceDesc.ddpfPixelFormat = textureFormats[i].pixelFormat;
                        plainDesc.ddpfPixelFormat = ppmPixelFormat;
                        sourceSurface.ddpfPixelFormat = textureFormats[i].pixelFormat;
                        argb1555Format = i;
                        break;
                    }
                }
            }
            bestRedDown = 8 - bestRedBits;
            bestGreenDown = 8 - bestGreenBits;
            bestBlueDown = 8 - bestBlueBits;
            textureSurfaceDesc.ddsCaps.dwCaps |= DDSCAPS_ALLOCONLOAD;
            memoryFlags.dwCaps = DDSCAPS_TEXTURE;
            if (nameDrawFirst) {
                ddResult = IDirectDraw2_GetAvailableVidMem(lpDD2, &memoryFlags, &videoMemoryTotal, &videoMemoryFree);
                dxErrorLine = 0xe82;
                LogDXError();
            }
            texturePageCount = videoMemoryFree >> 17;
            for (i = 0; i < 26; i++) {
                textureRecords[i].memorySize = 0x10000;
                textureRecords[i].level = 0;
                textureRecords[i].width = 256;
                textureRecords[i].height = 256;
                if (blendByBucket == 0) {
                    if (i >= 0 && i <= 4) {
                        textureRecords[i].type = 1;
                        textureRecords[i].typeVariant = 0;
                    } else if (i >= 5 && i <= 10) {
                        textureRecords[i].type = 2;
                        textureRecords[i].typeVariant = 0;
                    } else if (i >= 15 && i <= 16) {
                        textureRecords[i].type = 3;
                        textureRecords[i].typeVariant = 1;
                    } else if (i >= 18 && i <= 21) {
                        textureRecords[i].type = 3;
                        textureRecords[i].typeVariant = 1;
                    } else if (i == 22 || i == 23) {
                        textureRecords[i].type = 3;
                        textureRecords[i].typeVariant = 0;
                    } else if (i == 24) {
                        textureRecords[i].type = 3;
                        textureRecords[i].typeVariant = 1;
                    } else if (i == 25) {
                        textureRecords[i].type = 3;
                        textureRecords[i].typeVariant = 0;
                    } else {
                        textureRecords[i].type = 0;
                        textureRecords[i].typeVariant = 0;
                    }
                    textureRecords[i].reserved2a = 0;
                }
            }
            if (nameDrawFirst && (short)texturePageCount < 26) {
                i = 0;
                do {
                    memoryFlags.dwCaps = DDSCAPS_TEXTURE;
                    ddResult = IDirectDraw2_GetAvailableVidMem(lpDD2, &memoryFlags, &videoMemoryTotal, &videoMemoryFree);
                    dxErrorLine = 0xf01;
                    LogDXError();
                    requiredTextureBytes = 0;
                    for (j = 0; j < 26; j++) {
                        requiredTextureBytes += textureRecords[j].memorySize * 2;
                    }
                    if (requiredTextureBytes < videoMemoryFree) {
                        break;
                    }
                    if (textureRecords[i].level < 2) {
                        textureRecords[i].level++;
                    }
                    switch (textureRecords[i].level) {
                    case 0:
                        textureRecords[i].memorySize = 0x10000;
                        textureRecords[i].width = 256;
                        textureRecords[i].height = 256;
                        break;
                    case 1:
                        textureRecords[i].memorySize = 0x4000;
                        textureRecords[i].width = 128;
                        textureRecords[i].height = 128;
                        break;
                    case 2:
                        textureRecords[i].memorySize = 0x1000;
                        textureRecords[i].width = 64;
                        textureRecords[i].height = 64;
                        break;
                    }
                    i++;
                    if (i == 26) {
                        i = 0;
                    }
                } while ((short)texturePageCount < 26);
            }
            normalSetNo = 22;
            memset(&key, 0, sizeof(key));
            {
                for (page = 0; page < 26; page++) {
                    memoryFlags.dwCaps = DDSCAPS_TEXTURE;
                    if (nameDrawFirst) {
                        ddResult = IDirectDraw2_GetAvailableVidMem(lpDD2, &memoryFlags, &videoMemoryTotal, &videoMemoryFree);
                        dxErrorLine = 0xf35;
                        LogDXError();
                        wsprintfA(g_cdMessage, "Memory - Total:%ld, Free:%ld\n", videoMemoryTotal, videoMemoryFree);
                        OutputDebugStringA(g_cdMessage);
                    }
                    if (blendByBucket == 0) {
                        if (textureRecords[page].type == 3) {
                            textureSurfaceDesc.ddpfPixelFormat = textureFormats[argb1555Format].pixelFormat;
                            plainDesc.ddpfPixelFormat = ppmPixelFormat;
                            sourceSurface.ddpfPixelFormat = textureFormats[argb1555Format].pixelFormat;
                        } else {
                            textureSurfaceDesc.ddpfPixelFormat = textureFormats[bestTextureFormat].pixelFormat;
                            plainDesc.ddpfPixelFormat = ppmPixelFormat;
                            sourceSurface.ddpfPixelFormat = textureFormats[bestTextureFormat].pixelFormat;
                        }
                    }
                    wsprintfA(g_cdMessage, "Making texture page %d\n", page);
                    OutputDebugStringA(g_cdMessage);
                    sourceSurface.dwHeight = plainDesc.dwHeight = texturePageSize;
                    sourceSurface.dwWidth = plainDesc.dwWidth = texturePageSize;
                    if (texture16Bit) {
                        ddResult = IDirectDraw_CreateSurface(lpDD, &sourceSurface, &textureRecords[page].surfaces[0], NULL);
                        dxErrorLine = 0xf60;
                        LogDXError();
                    }
                    sourceSurface.dwHeight = textureRecords[page].height;
                    sourceSurface.dwWidth = textureRecords[page].width;
                    textureSurfaceDesc.dwHeight = textureRecords[page].height;
                    textureSurfaceDesc.dwWidth = textureRecords[page].width;
                    if (texture16Bit) {
                        ddResult = IDirectDraw_CreateSurface(lpDD, &sourceSurface, &textureRecords[page].surfaces[1], NULL);
                        dxErrorLine = 0xf74;
                        LogDXError();
                    }
                    ddResult = IDirectDraw_CreateSurface(lpDD, &textureSurfaceDesc, &textureRecords[page].surfaces[2], NULL);
                    dxErrorLine = 0xf7a;
                    LogDXError();
                    if (texture16Bit == 0) {
                        ddResult = IDirectDrawSurface_SetPalette(textureRecords[page].surfaces[2], windowPalette);
                        dxErrorLine = 0xf7c;
                        LogDXError();
                    }
                    if (texture16Bit && blendByBucket) {
                        ddResult = IDirectDrawSurface_SetColorKey(textureRecords[page].surfaces[0], DDCKEY_SRCBLT, &key);
                        dxErrorLine = 0xf83;
                        LogDXError();
                        ddResult = IDirectDrawSurface_SetColorKey(textureRecords[page].surfaces[1], DDCKEY_SRCBLT, &key);
                        dxErrorLine = 0xf84;
                        LogDXError();
                    }
                    if (blendByBucket) {
                        ddResult = IDirectDrawSurface_SetColorKey(textureRecords[page].surfaces[2], DDCKEY_SRCBLT, &key);
                        dxErrorLine = 0xf8a;
                        LogDXError();
                    }
                    if (texture16Bit) {
                        ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[0], &IID_IDirect3DTexture, (void **)&surfaceTextures0[page]);
                        dxErrorLine = 0xf97;
                        LogDXError();
                        ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[1], &IID_IDirect3DTexture, (void **)&surfaceTextures1[page]);
                        dxErrorLine = 0xf98;
                        LogDXError();
                    }
                    ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[2], &IID_IDirect3DTexture, (void **)&surfaceTextures2[page]);
                    dxErrorLine = 0xf9a;
                    LogDXError();
                }
            }
            hardwareDeviceCaps[0] = sizeof(hardwareDeviceCaps);
            emulatedCaps[0] = sizeof(emulatedCaps);
            ddResult = lpD3DDevice->lpVtbl->GetCaps(lpD3DDevice, &hardwareDeviceCaps, &emulatedCaps);
            dxErrorLine = 0xfa1;
            LogDXError();
            vertexBufferSize = 0x10000;
            if (nameDrawFirst) {
                if (hardwareDeviceCaps[41] && hardwareDeviceCaps[41] < vertexBufferSize) {
                    vertexBufferSize = hardwareDeviceCaps[41];
                }
                executeBuffersCreated = hardwareDeviceCaps[1] & 0x20;
            } else {
                if (emulatedCaps[41] && emulatedCaps[41] < vertexBufferSize) {
                    vertexBufferSize = emulatedCaps[41];
                }
                executeBuffersCreated = emulatedCaps[1] & 0x20;
            }
            executeBuffersCreated = 1;
            memset(&bufferDescriptionTemplate, 0, sizeof(bufferDescriptionTemplate));
            bufferDescriptionTemplate.size = sizeof(bufferDescriptionTemplate);
            bufferDescriptionTemplate.flags = 1;
            bufferDescriptionTemplate.bufferSize = vertexBufferSize;
            for (page = 0; page < 50; page++) {
                ddResult = lpD3DDevice->lpVtbl->CreateExecuteBuffer(lpD3DDevice, &bufferDescriptionTemplate, (IDirect3DExecuteBuffer **)&executeBuffers[page], NULL);
                dxErrorLine = 0xfc0;
                LogDXError();
            }
        } else {
            return 1;
        }
    }
    if (nameDrawFirst) {
        ddResult = IDirectDraw2_GetAvailableVidMem(lpDD2, &memoryFlags, &videoMemoryTotal, &videoMemoryFree);
        dxErrorLine = 0xfcc;
        LogDXError();
        wsprintfA(g_cdMessage, "Memory - Total:%ld, Free:%ld\n", videoMemoryTotal, videoMemoryFree);
        OutputDebugStringA(g_cdMessage);
    }
    windowHook = SetWindowsHookExA(WH_KEYBOARD, KeyboardHookProc, g_hInstance, GetCurrentThreadId());
    return 0;
}
#endif

/* Releases execute buffers, texture variants and the viewport, then clears the ship shape table. */
void ReleaseRendererObjects(void)
{
    int i;

    if (windowHook) {
        UnhookWindowsHookEx(windowHook);
    }
    if (direct3D) {
        ReleaseSurfaces();
        for (i = 0; i < 50; i++) {
            if (executeBuffers[i]) {
                executeBuffers[i]->methods->Release(executeBuffers[i]);
            }
        }
        for (i = 0; i < 26; i++) {
            if (textureRecords[i].surfaces[0]) {
                surfaceTextures0[i]->lpVtbl->Release(surfaceTextures0[i]);
                IDirectDrawSurface_Release(textureRecords[i].surfaces[0]);
            }
            if (textureRecords[i].surfaces[1]) {
                surfaceTextures1[i]->lpVtbl->Release(surfaceTextures1[i]);
                IDirectDrawSurface_Release(textureRecords[i].surfaces[1]);
            }
            if (textureRecords[i].surfaces[2]) {
                surfaceTextures2[i]->lpVtbl->Release(surfaceTextures2[i]);
                IDirectDrawSurface_Release(textureRecords[i].surfaces[2]);
            }
        }
        if (lpD3DDevice && renderViewport) {
            ddResult = lpD3DDevice->lpVtbl->DeleteViewport(lpD3DDevice, renderViewport);
#ifdef NETPLAY
            dxErrorLine = 0x100d;
#else
            dxErrorLine = 0xd21;
#endif
            LogDXError();
            renderViewport->lpVtbl->Release(renderViewport);
        }
    }
    shipShapeTable[0] = NULL;
}

#ifdef EQUIV
/* Alpha width starts at zero even without an alpha mask; the remaining counts stay untouched on skipped formats.
   The channel shifts and widths are grouped in one local block, in the order the exe's frame holds them. */
/* Direct3D texture format enumeration callback: records each RGB format in textureFormats and remembers the
   16-bit format with the widest channels, which it also copies to the caller's buffer. */
int __stdcall EnumTextureFormatCallback(LPDDSURFACEDESC surface, LPVOID context)
{
#ifdef NETPLAY
    int alphaUpValue;
    DDPIXELFORMAT *callbackFormatPosition;
    int greenUpCount;
    int localGreenBits;
    unsigned long maskBitsCount;
    int greenShiftChannelShift;
    int redBitsWidth;
    int blueBitsCtx;
    int textureAlphaBits;
    int redShiftUp;
    int blueShiftValue;
    int channelRedUp;
    int blueUpOfFormat;
    int alphaShiftCount;

    alphaShiftCount = 0;
    alphaUpValue = 0;
    textureAlphaBits = 0;
    callbackFormatPosition = (DDPIXELFORMAT *)context;
    if (surface && (surface->ddpfPixelFormat.dwFlags & DDPF_RGB)) {
        if (surface->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4)
            textureFormats[textureFormatCount].kind = 4;
        if (surface->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXEDTO8)
            textureFormats[textureFormatCount].kind = 8;
        if (surface->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
            textureFormats[textureFormatCount].kind = 8;
            textureFormats[textureFormatCount].pixelFormat.dwFlags = 0x60;
        }
        if (surface->ddpfPixelFormat.dwRGBBitCount > 16) {
            return 1;
        } else {
            if (surface->ddpfPixelFormat.dwRGBBitCount > 8) {
                textureFormats[textureFormatCount].kind = 16;
                wsprintfA(g_cdMessage, "---------\n%d bpp\n", surface->ddpfPixelFormat.dwRGBBitCount);
                OutputDebugStringA(g_cdMessage);
                wsprintfA(g_cdMessage, "R=%08lx\n", surface->ddpfPixelFormat.dwRBitMask);
                OutputDebugStringA(g_cdMessage);
                wsprintfA(g_cdMessage, "G=%08lx\n", surface->ddpfPixelFormat.dwGBitMask);
                OutputDebugStringA(g_cdMessage);
                wsprintfA(g_cdMessage, "B=%08lx\n", surface->ddpfPixelFormat.dwBBitMask);
                OutputDebugStringA(g_cdMessage);
                wsprintfA(g_cdMessage, "A=%08lx\n", surface->ddpfPixelFormat.dwRGBAlphaBitMask);
                OutputDebugStringA(g_cdMessage);
                redShiftUp = 0;
                maskBitsCount = surface->ddpfPixelFormat.dwRBitMask;
                for (; (maskBitsCount & 1) == 0; redShiftUp++, maskBitsCount >>= 1) { }
                channelRedUp = redShiftUp;
                redShiftUp = 0;
                for (; maskBitsCount & 1; redShiftUp++, maskBitsCount >>= 1) { }
                redBitsWidth = redShiftUp;
                greenShiftChannelShift = 0;
                maskBitsCount = surface->ddpfPixelFormat.dwGBitMask;
                for (; (maskBitsCount & 1) == 0; greenShiftChannelShift++, maskBitsCount >>= 1) { }
                greenUpCount = greenShiftChannelShift;
                greenShiftChannelShift = 0;
                for (; maskBitsCount & 1; greenShiftChannelShift++, maskBitsCount >>= 1) { }
                localGreenBits = greenShiftChannelShift;
                blueShiftValue = 0;
                maskBitsCount = surface->ddpfPixelFormat.dwBBitMask;
                for (; (maskBitsCount & 1) == 0; blueShiftValue++, maskBitsCount >>= 1) { }
                blueUpOfFormat = blueShiftValue;
                    blueShiftValue = 0;
                for (; maskBitsCount & 1; blueShiftValue++, maskBitsCount >>= 1) { }
                blueBitsCtx = blueShiftValue;
                if (surface->ddpfPixelFormat.dwRGBAlphaBitMask) {
                    alphaShiftCount = 0;
                    maskBitsCount = surface->ddpfPixelFormat.dwRGBAlphaBitMask;
                    for (; (maskBitsCount & 1) == 0; alphaShiftCount++, maskBitsCount >>= 1) { }
                    alphaUpValue = alphaShiftCount;
                    alphaShiftCount = 0;
                    for (; maskBitsCount & 1; alphaShiftCount++, maskBitsCount >>= 1) { }
                    textureAlphaBits = alphaShiftCount;
                }
                if ((bestRedBits < redBitsWidth || localGreenBits > bestGreenBits || blueBitsCtx > bestBlueBits)
                    && surface->ddpfPixelFormat.dwRGBBitCount == 16) {
                    bestRedUpNet = channelRedUp;
                    bestGreenUpNet = greenUpCount;
                    bestBlueUpNet = blueUpOfFormat;
                    bestRedBits = redBitsWidth;
                    bestGreenBits = localGreenBits;
                    bestBlueBits = blueBitsCtx;
                    *callbackFormatPosition = surface->ddpfPixelFormat;
                    bestTextureFormat = textureFormatCount;
                }
            }
        }
    }
    textureFormats[textureFormatCount].pixelFormat = surface->ddpfPixelFormat;
    textureFormats[textureFormatCount].shifts.redUp = channelRedUp;
    textureFormats[textureFormatCount].shifts.greenUp = greenUpCount;
    textureFormats[textureFormatCount].shifts.blueUp = blueUpOfFormat;
    textureFormats[textureFormatCount].shifts.alphaUp = alphaUpValue;
    textureFormats[textureFormatCount].shifts.redBits = redBitsWidth;
    textureFormats[textureFormatCount].shifts.greenBits = localGreenBits;
    textureFormats[textureFormatCount].shifts.blueBits = blueBitsCtx;
    textureFormats[textureFormatCount].shifts.alphaBits = textureAlphaBits;
    textureFormats[textureFormatCount].shifts.redDown = 8 - redBitsWidth;
    textureFormats[textureFormatCount].shifts.greenDown = 8 - localGreenBits;
    textureFormats[textureFormatCount].shifts.blueDown = 8 - blueBitsCtx;
    textureFormats[textureFormatCount].shifts.alphaDown = 8 - textureAlphaBits;
    textureFormatCount++;
    return 1;
#else
    struct {
        int redUp;
        int alphaBits;
        int greenUp;
        int blueUp;
        int greenBits;
        int blueBits;
    } channels;
    int redBits;
    int alphaUp;
    unsigned long mask;
    TextureFormat *format;

    alphaUp = 0;
    channels.alphaBits = 0;
    if (surface && (surface->ddpfPixelFormat.dwFlags & DDPF_RGB)) {
        if (surface->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4) {
            textureFormats[textureFormatCount].kind = 4;
        }
        if (surface->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXEDTO8) {
            textureFormats[textureFormatCount].kind = 8;
        }
        if (surface->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
            textureFormats[textureFormatCount].kind = 8;
            textureFormats[textureFormatCount].pixelFormat.dwFlags = 0x60;
        }
        if (surface->ddpfPixelFormat.dwRGBBitCount > 16) {
            return 1;
        }
        if (surface->ddpfPixelFormat.dwRGBBitCount > 8) {
            textureFormats[textureFormatCount].kind = 16;
            wsprintfA(g_cdMessage, "---------\n%d bpp\n", surface->ddpfPixelFormat.dwRGBBitCount);
            OutputDebugStringA(g_cdMessage);
            wsprintfA(g_cdMessage, "R=%08lx\n", surface->ddpfPixelFormat.dwRBitMask);
            OutputDebugStringA(g_cdMessage);
            wsprintfA(g_cdMessage, "G=%08lx\n", surface->ddpfPixelFormat.dwGBitMask);
            OutputDebugStringA(g_cdMessage);
            wsprintfA(g_cdMessage, "B=%08lx\n", surface->ddpfPixelFormat.dwBBitMask);
            OutputDebugStringA(g_cdMessage);
            wsprintfA(g_cdMessage, "A=%08lx\n", surface->ddpfPixelFormat.dwRGBAlphaBitMask);
            OutputDebugStringA(g_cdMessage);
            channels.redUp = 0;
            mask = surface->ddpfPixelFormat.dwRBitMask;
            while ((mask & 1) == 0) {
                mask >>= 1;
                channels.redUp++;
            }
            redBits = 0;
            while (mask & 1) {
                mask >>= 1;
                redBits++;
            }
            channels.greenUp = 0;
            mask = surface->ddpfPixelFormat.dwGBitMask;
            while ((mask & 1) == 0) {
                mask >>= 1;
                channels.greenUp++;
            }
            channels.greenBits = 0;
            while (mask & 1) {
                mask >>= 1;
                channels.greenBits++;
            }
            channels.blueUp = 0;
            mask = surface->ddpfPixelFormat.dwBBitMask;
            while ((mask & 1) == 0) {
                mask >>= 1;
                channels.blueUp++;
            }
            channels.blueBits = 0;
            while (mask & 1) {
                mask >>= 1;
                channels.blueBits++;
            }
            if (surface->ddpfPixelFormat.dwRGBAlphaBitMask) {
                mask = surface->ddpfPixelFormat.dwRGBAlphaBitMask;
                alphaUp = 0;
                while ((mask & 1) == 0) {
                    mask >>= 1;
                    alphaUp++;
                }
                channels.alphaBits = 0;
                while (mask & 1) {
                    mask >>= 1;
                    channels.alphaBits++;
                }
            }
            if ((bestRedBits < redBits || channels.greenBits > bestGreenBits || channels.blueBits > bestBlueBits)
                && surface->ddpfPixelFormat.dwRGBBitCount == 16) {
                bestRedUp = channels.redUp;
                bestGreenUp = channels.greenUp;
                bestBlueUp = channels.blueUp;
                bestRedBits = redBits;
                bestGreenBits = channels.greenBits;
                bestBlueBits = channels.blueBits;
                *(DDPIXELFORMAT *)context = surface->ddpfPixelFormat;
                bestTextureFormat = textureFormatCount;
            }
        } else {
            redBits = channels.redUp;
        }
    } else {
        redBits = channels.redUp;
    }
    format = &textureFormats[textureFormatCount];
    format->pixelFormat = surface->ddpfPixelFormat;
    format->shifts.redUp = channels.redUp;
    format->shifts.greenUp = channels.greenUp;
    format->shifts.blueUp = channels.blueUp;
    format->shifts.alphaUp = alphaUp;
    format->shifts.redBits = redBits;
    format->shifts.greenBits = channels.greenBits;
    format->shifts.blueBits = channels.blueBits;
    format->shifts.alphaBits = channels.alphaBits;
    format->shifts.redDown = 8 - redBits;
    format->shifts.greenDown = 8 - channels.greenBits;
    textureFormatCount++;
    format->shifts.blueDown = 8 - channels.blueBits;
    format->shifts.alphaDown = 8 - channels.alphaBits;
    return 1;
#endif
}

#endif

/* Derives each channel's shift and width from the active RGB masks. */
#ifndef NETPLAY
void SetPixelShifts(void)
{
    unsigned long mask;
    int redUp;
    int redBits;
    int greenUp;
    int greenBits;
    int blueUp;
    int blueBits;

    redUp = 0;
    mask = ppmPixelFormat.dwRBitMask;
    while ((mask & 1) == 0) {
        mask >>= 1;
        redUp++;
    }
    redBits = 0;
    while (mask & 1) {
        mask >>= 1;
        redBits++;
    }
    greenUp = 0;
    mask = ppmPixelFormat.dwGBitMask;
    while ((mask & 1) == 0) {
        mask >>= 1;
        greenUp++;
    }
    greenBits = 0;
    while (mask & 1) {
        mask >>= 1;
        greenBits++;
    }
    blueUp = 0;
    mask = ppmPixelFormat.dwBBitMask;
    while ((mask & 1) == 0) {
        mask >>= 1;
        blueUp++;
    }
    blueBits = 0;
    while (mask & 1) {
        mask >>= 1;
        blueBits++;
    }
    pixelRedUp = redUp;
    pixelGreenUp = greenUp;
    pixelBlueUp = blueUp;
    pixelRedBits = redBits;
    pixelGreenBits = greenBits;
    pixelBlueBits = blueBits;
    pixelRedDown = 8 - redBits;
    pixelGreenDown = 8 - greenBits;
    pixelBlueDown = 8 - blueBits;
}
#else
/* The NET build keeps the channel position, run length and width in R/G/B-prefixed locals. */
void SetPixelShifts(void)
{
    int GPos;
    int GSize;
    unsigned long m;
    int GNum;
    int RSize;
    int BSize;
    int RNum;
    int BNum;
    int RPos;
    int BPos;

    pixelBlueBits = 0;
    pixelGreenBits = pixelBlueBits;
    pixelRedBits = pixelGreenBits;
    RNum = 0;
    m = ppmPixelFormat.dwRBitMask;
    for (; !(m & 1); RNum++, m >>= 1)
        ;
    RPos = RNum;
    for (RNum = 0; m & 1; RNum++, m >>= 1)
        ;
    RSize = RNum;
    GNum = 0;
    m = ppmPixelFormat.dwGBitMask;
    for (; !(m & 1); GNum++, m >>= 1)
        ;
    GPos = GNum;
    for (GNum = 0; m & 1; GNum++, m >>= 1)
        ;
    GSize = GNum;
    BNum = 0;
    m = ppmPixelFormat.dwBBitMask;
    for (; !(m & 1); BNum++, m >>= 1)
        ;
    BPos = BNum;
    for (BNum = 0; m & 1; BNum++, m >>= 1)
        ;
    BSize = BNum;
    pixelRedUp = RPos;
    pixelGreenUp = GPos;
    pixelBlueUp = BPos;
    pixelRedBits = RSize;
    pixelGreenBits = GSize;
    pixelBlueBits = BSize;
    pixelRedDown = 8 - pixelRedBits;
    pixelGreenDown = 8 - pixelGreenBits;
    pixelBlueDown = 8 - pixelBlueBits;
}
#endif

/* Writes the text of the last DirectDraw error, with the line that reported it, to the debug output. */
void ReportDDError(void)
{
    if (ddResult != 0) {
        wsprintfA(g_cdMessage, "DX ERROR:Line %05d \t%s", dxErrorLine, DDErrorString(ddResult));
        OutputDebugStringA(g_cdMessage);
    }
}

/* Updates the setup timer and dispatches one pending window message. */
int ProcessSetupMessages(void)
{
#ifdef NETPLAY
    DWORD currentTime;
    DWORD elapsedTime;

    currentTime = timeGetTime();
    elapsedTime = currentTime - setupLoopTick;
    setupLoopTick = currentTime;
#else
    setupLoopTick = timeGetTime();
#endif
    framesThisSecond++;
    if (time(NULL) != lastFpsSecond) {
        lastFpsSecond = time(NULL);
        framesThisSecond = 0;
    }
    if (!setupQuitRequested) {
        if (PeekMessageA(&windowMessage, NULL, 0, 0, PM_REMOVE) && g_hwnd) {
            if (windowMessage.message == WM_QUIT) {
                setupQuitRequested = 1;
                setupWindowActive = 0;
                return 1;
            }
            if (windowMessage.message == WM_KEYDOWN && !(windowMessage.lParam & KEY_PREVIOUSLY_DOWN)) {
                g_keyboardCode = windowMessage.wParam;
            }
            TranslateMessage(&windowMessage);
            DispatchMessageA(&windowMessage);
        }
    } else {
        return 1;
    }
    return 0;
}

/* Makes the current module's code section writable, including the Win32s handle conversion. */
#ifdef NETPLAY
void SetCodeSectionReadWrite(void)
{
    unsigned char *basePointer;
    /* The original records success without reading it. */
    int writeEnabled;
    HMODULE currentModule;
    DWORD (__stdcall *imteFromHModule)(HMODULE);
    unsigned char *(__stdcall *baseAddrFromImte)(DWORD);
    HMODULE win32s;
    DWORD moduleEntry;

    basePointer = NULL;
    writeEnabled = 0;
    currentModule = GetModuleHandleA(NULL);
    if ((GetVersion() & 0xc0000000) == 0x80000000) {
        win32s = GetModuleHandleA(g_w32sKernelName);
        imteFromHModule = (DWORD (__stdcall *)(HMODULE))GetProcAddress(win32s, g_imteFromHModuleProcName);
        baseAddrFromImte = (unsigned char *(__stdcall *)(DWORD))GetProcAddress(win32s, g_baseAddrFromImteProcName);
        if (imteFromHModule && baseAddrFromImte) {
            moduleEntry = imteFromHModule(currentModule);
            basePointer = baseAddrFromImte(moduleEntry);
        }
    } else {
        basePointer = (unsigned char *)currentModule;
    }
    if (basePointer) {
        DWORD previousProtection;
        IMAGE_OPTIONAL_HEADER *codeHeader;

        codeHeader = &((IMAGE_NT_HEADERS *)(basePointer + ((IMAGE_DOS_HEADER *)basePointer)->e_lfanew))->OptionalHeader;
        if (VirtualProtect(basePointer + codeHeader->BaseOfCode,
                           codeHeader->SizeOfCode, PAGE_READWRITE, &previousProtection))
            writeEnabled = 1;
    }
}
#else
void SetCodeSectionReadWrite(void)
{
    HMODULE module;
    HMODULE win32sKernel;
    DWORD (__stdcall *imteFromHModule)(HMODULE);
    unsigned char *(__stdcall *baseAddrFromImte)(DWORD);
    unsigned char *imageBase;
    DWORD oldProtect;
    IMAGE_OPTIONAL_HEADER *imageHeaders;

    imageBase = NULL;
    module = GetModuleHandleA(NULL);
    if ((GetVersion() & 0xc0000000) == 0x80000000) {
        win32sKernel = GetModuleHandleA(g_w32sKernelName);
        imteFromHModule = (DWORD (__stdcall *)(HMODULE))GetProcAddress(win32sKernel, g_imteFromHModuleProcName);
        baseAddrFromImte = (unsigned char *(__stdcall *)(DWORD))GetProcAddress(win32sKernel, g_baseAddrFromImteProcName);
        if (imteFromHModule && baseAddrFromImte) {
            imageBase = baseAddrFromImte(imteFromHModule(module));
        }
    } else {
        imageBase = (unsigned char *)module;
    }
    if (imageBase) {
        imageHeaders = &((IMAGE_NT_HEADERS *)(imageBase + ((IMAGE_DOS_HEADER *)imageBase)->e_lfanew))->OptionalHeader;
        VirtualProtect(imageBase + imageHeaders->BaseOfCode,
                       imageHeaders->SizeOfCode, PAGE_READWRITE, &oldProtect);
    }
}
#endif

#include "pc_surface.h"

#ifdef NETPLAY
/* Derives each channel's shift and width from the masks of a pixel format. The alpha shift and width repeat the green width when the format has no alpha mask. */
void GetPixelChannels(PixelChannels *shifts, DDPIXELFORMAT format)
{
    int alphaOffset;
    int alphaLength;
    int redPosition;
    int blueShift;
    int blueCount;
    int alphaBits;
    int rCount;
    int blueWidth;
    int redWidth;
    unsigned long mask;
    int greenLength;
    int gBits;
    int greenUp;

    pixelBlueBits = 0;
    pixelGreenBits = pixelBlueBits;
    pixelRedBits = pixelGreenBits;
    rCount = 0;
    mask = format.dwRBitMask;
    for (; !(mask & 1); rCount++, mask >>= 1)
        ;
    redPosition = rCount;
    for (rCount = 0; mask & 1; rCount++, mask >>= 1)
        ;
    redWidth = rCount;
    greenLength = 0;
    mask = format.dwGBitMask;
    for (; !(mask & 1); greenLength++, mask >>= 1)
        ;
    greenUp = greenLength;
    for (greenLength = 0; mask & 1; greenLength++, mask >>= 1)
        ;
    gBits = greenLength;
    blueCount = 0;
    mask = format.dwBBitMask;
    for (; !(mask & 1); blueCount++, mask >>= 1)
        ;
    blueShift = blueCount;
    for (blueCount = 0; mask & 1; blueCount++, mask >>= 1)
        ;
    blueWidth = blueCount;
    if (format.dwRGBAlphaBitMask) {
        alphaOffset = 0;
        mask = format.dwRGBAlphaBitMask;
        for (; !(mask & 1); alphaOffset++, mask >>= 1)
            ;
        alphaLength = alphaOffset;
        for (alphaOffset = 0; mask & 1; alphaOffset++, mask >>= 1)
            ;
        alphaBits = alphaOffset;
    }
    shifts->redShift = redPosition;
    shifts->greenShift = greenUp;
    shifts->blueShift = blueShift;
    shifts->alphaShift = alphaLength;
    shifts->redBits = redWidth;
    shifts->greenBits = gBits;
    shifts->blueBits = blueWidth;
    shifts->alphaBits = alphaBits;
    shifts->redLoss = 8 - shifts->redBits;
    shifts->greenLoss = 8 - shifts->greenBits;
    shifts->blueLoss = 8 - shifts->blueBits;
    shifts->alphaLoss = 8 - shifts->alphaBits;
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Register allocation differs (58%). */
/* NET draft also needs a frame layout closer to the target. */
/* Derives each channel's shift and width from the masks of a pixel format. The alpha shift and width repeat the green width when the format has no alpha mask. */
void GetPixelChannels(PixelChannels *shifts, DDPIXELFORMAT format)
{
    int redBits;
    int redUp;
    int alphaBits;
    int blueBits;
    int alphaUp;
    int blueUp;
    unsigned long mask;
    int greenBits;
    int greenUp;

    pixelRedBits = 0;
    pixelGreenBits = 0;
    pixelBlueBits = 0;
    redUp = 0;
    mask = format.dwRBitMask;
    while ((mask & 1) == 0) {
        mask >>= 1;
        redUp++;
    }
    redBits = 0;
    while (mask & 1) {
        mask >>= 1;
        redBits++;
    }
    greenUp = 0;
    mask = format.dwGBitMask;
    while ((mask & 1) == 0) {
        mask >>= 1;
        greenUp++;
    }
    greenBits = 0;
    while (mask & 1) {
        mask >>= 1;
        greenBits++;
    }
    blueUp = 0;
    mask = format.dwBBitMask;
    while ((mask & 1) == 0) {
        mask >>= 1;
        blueUp++;
    }
    blueBits = 0;
    while (mask & 1) {
        mask >>= 1;
        blueBits++;
    }
    if (format.dwRGBAlphaBitMask) {
        alphaUp = 0;
        mask = format.dwRGBAlphaBitMask;
        while ((mask & 1) == 0) {
            mask >>= 1;
            alphaUp++;
        }
        alphaBits = 0;
        while (mask & 1) {
            mask >>= 1;
            alphaBits++;
        }
    } else {
        alphaUp = greenBits;
        alphaBits = greenBits;
    }
    shifts->redShift = redUp;
    shifts->greenShift = greenUp;
    shifts->blueShift = blueUp;
    shifts->alphaShift = alphaUp;
    shifts->redBits = redBits;
    shifts->greenBits = greenBits;
    shifts->blueBits = blueBits;
    shifts->alphaBits = alphaBits;
    shifts->redLoss = 8 - redBits;
    shifts->greenLoss = 8 - greenBits;
    shifts->blueLoss = 8 - blueBits;
    shifts->alphaLoss = 8 - alphaBits;
}
#endif
#endif

/* Saves the given pixels to "<directory>screenN.tga", where N is the number of screenshots taken so far. */
void SaveScreenshot(char *directory, void *pixels, int size)
{
    char fileName[20];
    char path[256];
    FILE *file;

    sprintf(fileName, "screen%d", screenshotNumber);
    strcpy(path, directory);
    strcat(path, fileName);
    strcat(path, ".tga");
    file = fopen(path, "wb");
    if (!file)
        return;
    fwrite(pixels, 1, size, file);
    fclose(file);
    screenshotNumber++;
}

#ifdef NETPLAY
/* Plays an MPEG movie between two black frames and then restarts the CD audio. Skipped at 512 pixel width. */
void PlayMovie(char *name)
{
    unsigned char status[8];

    if (screenWidth != 512) {
        joyButtonNow = 0;
        joyButtonPrev = 0;
        ClearBackBufferForTextureMode();
        CdMusicStop();
        moviePlaying = 1;
        SfxSetVolume(255);
        PlayMpegMovie(g_hwnd, name, bmpBuffer);
        SfxSetVolume(gameData->effectVolume * 255 / 100);
        InitCDAudio(gameData, status);
        moviePlaying = 0;
        ClearBackBufferForTextureMode();
    }
}
#endif

#ifdef EQUIV
#if !defined(NETPLAY) && !defined(POWERVR)
/* Plays an MPEG movie between two black frames and then restarts the CD audio. Skipped at 512 pixel width. */
void PlayMovie(char *name)
{
    unsigned char status[8];

    if (screenWidth != 512) {
        joyButtonNow = 0;
        joyButtonPrev = 0;
        ClearBackBufferToBlack();
        CdMusicStop();
        moviePlaying = 1;
        SfxSetVolume(255);
        PlayMpegMovie(g_hwnd, name, bmpBuffer);
        SfxSetVolume(gameData->effectVolume * 255 / 100);
        InitCDAudio(gameData, status);
        moviePlaying = 0;
        ClearBackBufferToBlack();
    }
}
#endif
#endif

#ifdef NETPLAY
/* Clears the back buffer according to the active texture mode. */
void ClearBackBufferForTextureMode(void)
{
    if (rgbTextures)
        ClearBackBufferRect(0);
    else
        ClearBackBufferToBlack(0);
}
#endif

#ifdef NETPLAY
/* Clears the back buffer to black twice, presenting a frame after each fill. */
void ClearBackBufferToBlack(int colour)
{
    PALETTEENTRY palette[256];

    memset(palette, 0, sizeof(palette));
    ddResult = IDirectDrawPalette_SetEntries(defaultPalette, 0, 0, 256, palette);
    ClearBackBufferRect(colour);
}
#endif

#ifdef EQUIV
#if !defined(NETPLAY) && !defined(POWERVR)
/* Clears the back buffer to black twice, presenting a frame after each fill. */
void ClearBackBufferToBlack(void)
{
    if (rgbTextures) {
        ClearBackBuffer(0);
    } else {
        PALETTEENTRY palette[256];

        memset(palette, 0, sizeof(palette));
        ddResult = IDirectDrawPalette_SetEntries(defaultPalette, 0, 0, 256, palette);
        ClearBackBuffer(0);
    }
}
#endif
#endif

#ifdef EQUIV
/* Does not match yet: rgbTextures is read into eax where the exe uses ecx, which shifts the rest of the function. */
/* Fills the back buffer with a colour index, or with black when RGB textures are in use, and presents a frame after each fill. */
void ClearBackBuffer(unsigned char colour)
{
    RECT rect;
    DDBLTFX effects;

    memset(&effects, 0, sizeof(effects));
    effects.dwSize = sizeof(effects);
    SetRect(&rect, 0, 0, (int)screenScaleX, (int)screenScaleY);
    if (rgbTextures) {
        effects.dwFillColor = 0;
    } else {
        effects.dwFillColor = colour;
    }
    ddResult = IDirectDrawSurface_Blt(backBuffer, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &effects);
    RaceFrameStart();
    ddResult = IDirectDrawSurface_Blt(backBuffer, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &effects);
    RaceFrameStart();
}
#endif

#include <direct.h>
extern char wobbleDir2[], wobbleDir3[], wobbleDir5[], wobbleDir6[], wobbleDir7[];
extern char trackDirectory[];

/* Builds each asset path as wobbleDir0 + subdirectory (falling back to wobbleDir1, without the
   leading backslash, when the first directory cannot be entered) and stores it in the matching
   wobbleDirN slot. The track path loses its trailing "01\\". Each subdirectory literal carries
   a second terminating NUL, as in the original data. */
void SetAssetDirectories(void)
{
    char suffixes[7][12] = { "\\common\\\0", "\\textures\\\0", "\\pics\\\0", "\\blitsurf\\\0",
                            "\\sound\\\0", "\\mpeg\\\0", "\\track01\\\0" };
    char path[256];
    short length;
    short i;

    for (i = 0; i < 7; i++) {
        strcpy(path, wobbleDir0);
        strcat(path, suffixes[i]);
        if (_chdir(path) != 0) {
            strcpy(path, wobbleDir1);
            strcat(path, suffixes[i] + 1);
        }
        switch (i) {
        case 0:
            strcpy(wobbleDir2, path);
            break;
        case 1:
            strcpy(wobbleDir3, path);
            break;
        case 2:
            strcpy(wobbleDir4, path);
            break;
        case 3:
            strcpy(wobbleDir5, path);
            break;
        case 4:
            strcpy(wobbleDir6, path);
            break;
        case 5:
            strcpy(wobbleDir7, path);
            break;
        case 6:
            length = strlen(path) - 3;
            strncpy(trackDirectory, path, length);
            trackDirectory[length] = 0;
            break;
        }
    }
}

#ifdef EQUIV
#ifdef NETPLAY
/* Halves the brightness of the frame eight times: pixel by pixel for RGB textures, through the palette otherwise. */
void FadeScreenToBlack(void)
{
    unsigned short redMask = 0;
    unsigned short greenMask = 0;
    unsigned short blueMask = 0;
    unsigned short keepMask = 0;
    DDPIXELFORMAT pixelFormat;
    PixelChannels channels;
    DDSURFACEDESC desc;
    RECT destRect;
    RECT srcRect;
    LPDIRECTDRAWSURFACE2 surface2;
    LPDIRECTDRAWPALETTE palette;
    unsigned short *base;
    unsigned int i;
    unsigned int j;
    unsigned int y;
    unsigned int x;
    int rowStart;
    int index;
    unsigned short *pixel;

    if (rgbTextures) {
        pixelFormat.dwSize = sizeof(pixelFormat);
        ddResult = IDirectDrawSurface_GetPixelFormat(primarySurface, &pixelFormat);
        dxErrorLine = 0x1293;
        LogDXError();
        GetPixelChannels(&channels, pixelFormat);
        desc.dwSize = sizeof(desc);
        IDirectDrawSurface_GetSurfaceDesc(backBuffer, &desc);
        destRect.top = srcRect.top = 0;
        destRect.left = srcRect.left = 0;
        destRect.bottom = srcRect.bottom = desc.dwHeight;
        destRect.right = srcRect.right = desc.dwWidth;
        ddResult = IDirectDrawSurface_Blt(backBuffer, &destRect, primarySurface, &srcRect, DDBLT_WAIT, NULL);
        dxErrorLine = 0x129f;
        LogDXError();
        ddResult = IDirectDrawSurface_QueryInterface(backBuffer, &IID_IDirectDrawSurface2, (void **)&surface2);
        dxErrorLine = 0x12a3;
        LogDXError();
        redMask = (1 << channels.redBits) - 1;
        greenMask = (1 << channels.greenBits) - 1;
        blueMask = (1 << channels.blueBits) - 1;
        keepMask = 1 << (channels.redBits - 1);
        keepMask = (keepMask & 0xffff) | (1 << (channels.greenBits + channels.redBits - 1));
        keepMask = (keepMask & 0xffff) | (1 << (channels.blueBits + channels.greenBits + channels.redBits - 1));
        keepMask = ~(keepMask & 0xffff);
        for (i = 0; i < 8; i++) {
            desc.dwSize = sizeof(desc);
            ddResult = IDirectDrawSurface2_Lock(surface2, NULL, &desc, DDLOCK_WAIT, NULL);
            dxErrorLine = 0x12d4;
            LogDXError();
            base = desc.lpSurface;
            for (y = 0; y < desc.dwHeight; y++) {
                rowStart = (desc.lPitch >> 1) * y;
                for (x = 0; x < desc.dwWidth; x++) {
                    index = rowStart + x;
                    pixel = base + index;
                    *pixel = (*pixel >> 1) & (keepMask & 0xffff);
                }
            }
            ddResult = IDirectDrawSurface2_Unlock(surface2, NULL);
            dxErrorLine = 0x12e6;
            LogDXError();
            ddResult = IDirectDrawSurface_Blt(primarySurface, &destRect, backBuffer, &srcRect, DDBLT_WAIT, NULL);
            dxErrorLine = 0x12e7;
            LogDXError();
        }
    } else {
        ddResult = IDirectDraw_CreatePalette(lpDD, 0x44, mpegPaletteEntries, &palette, NULL);
        dxErrorLine = 0x12f1;
        LogDXError();
        ddResult = IDirectDrawSurface_SetPalette(primarySurface, palette);
        dxErrorLine = 0x12f5;
        LogDXError();
        ddResult = IDirectDrawSurface_SetPalette(backBuffer, palette);
        dxErrorLine = 0x12f6;
        LogDXError();
        for (j = 0; j < 8; j++) {
            for (i = 0; i < 256; i++) {
                mpegPaletteEntries[i].peRed >>= 1;
                mpegPaletteEntries[i].peGreen >>= 1;
                mpegPaletteEntries[i].peBlue >>= 1;
            }
            ddResult = IDirectDrawPalette_SetEntries(palette, 0, 0, 256, mpegPaletteEntries);
            dxErrorLine = 0x1302;
            LogDXError();
        }
    }
}
#else
void FadeScreenToBlack(void)
{
    /* The original keeps the pixel format and its decoded channels side by side in the frame. */
    struct {
        DDPIXELFORMAT pixelFormat;
        PixelChannels channels;
    } channelFormat;
    DDSURFACEDESC desc;
    LPDIRECTDRAWSURFACE2 surface2;
    int x, y;
    RECT destRect;
    LPDIRECTDRAWPALETTE palette;
    PALETTEENTRY *entry;
    unsigned short *pixels;
    RECT srcRect;
    unsigned int rowStart;
    int pass;
    short mask;
    if (rgbTextures) {
        channelFormat.pixelFormat.dwSize = sizeof(channelFormat.pixelFormat);
        ddResult = IDirectDrawSurface_GetPixelFormat(primarySurface, &channelFormat.pixelFormat);
        dxErrorLine = 0xfa7;
        LogDXError();
        GetPixelChannels(&channelFormat.channels, channelFormat.pixelFormat);
        desc.dwSize = sizeof(desc);
        IDirectDrawSurface_GetSurfaceDesc(backBuffer, &desc);
        srcRect.left = 0;
        srcRect.top = 0;
        srcRect.right = desc.dwWidth;
        srcRect.bottom = desc.dwHeight;
        destRect.left = 0;
        destRect.top = 0;
        destRect.right = desc.dwWidth;
        destRect.bottom = desc.dwHeight;
        ddResult = IDirectDrawSurface_Blt(backBuffer, &destRect, primarySurface, &srcRect,
                                          DDBLT_WAIT, NULL);
        dxErrorLine = 0xfb3;
        LogDXError();
        ddResult = IDirectDrawSurface_QueryInterface(backBuffer, &IID_IDirectDrawSurface2,
                                                     (void **)&surface2);
        dxErrorLine = 0xfb7;
        LogDXError();
        mask = ~((1 << (channelFormat.channels.redBits + channelFormat.channels.greenBits + channelFormat.channels.blueBits - 1)) |
                 (1 << (channelFormat.channels.redBits + channelFormat.channels.greenBits - 1)) |
                 (1 << (channelFormat.channels.redBits - 1)));
        for (pass = 8; pass != 0; pass--) {
            desc.dwSize = sizeof(desc);
            ddResult = IDirectDrawSurface2_Lock(surface2, NULL, &desc, DDLOCK_WAIT, NULL);
            dxErrorLine = 0xfe8;
            LogDXError();
            pixels = desc.lpSurface;
            for (y = 0; y < desc.dwHeight; y++) {
                rowStart = (desc.lPitch >> 1) * y;
                for (x = 0; x < desc.dwWidth; x++) {
                    pixels[rowStart + x] = (pixels[rowStart + x] >> 1) & mask;
                }
            }
            ddResult = IDirectDrawSurface2_Unlock(surface2, NULL);
            dxErrorLine = 0xffa;
            LogDXError();
            ddResult = IDirectDrawSurface_Blt(primarySurface, &destRect, backBuffer, &srcRect,
                                              DDBLT_WAIT, NULL);
            dxErrorLine = 0xffb;
            LogDXError();
        }
    } else {
        ddResult = IDirectDraw_CreatePalette(lpDD, 0x44, mpegPaletteEntries, &palette, NULL);
        dxErrorLine = 0x1005;
        LogDXError();
        ddResult = IDirectDrawSurface_SetPalette(primarySurface, palette);
        dxErrorLine = 0x1009;
        LogDXError();
        ddResult = IDirectDrawSurface_SetPalette(backBuffer, palette);
        dxErrorLine = 0x100a;
        LogDXError();
        for (pass = 8; pass != 0; pass--) {
            entry = mpegPaletteEntries;
            do {
                entry->peRed >>= 1;
                entry->peGreen >>= 1;
                entry->peBlue >>= 1;
                entry++;
            } while (entry < (PALETTEENTRY *)&mpegDestBase);
            ddResult = IDirectDrawPalette_SetEntries(palette, 0, 0, 256, mpegPaletteEntries);
            dxErrorLine = 0x1016;
            LogDXError();
        }
    }
}
#endif
#endif

/* Reads install paths from the registry; opened keys are left open by the original. */
#ifdef NETPLAY
int LoadInstallPaths(void)
{
    HKEY wipeoutKey;
    HKEY softwareKey;
    HKEY psygnosisKey;
    HKEY studiosKey;
    char *softwareName;
    char *psygnosisName;
    char *studiosName;
    char *wipeoutName;
    char *directoryValue;
    char *sourceValue;
    DWORD pathSize;
    char path[1024];
    int result;

    result = 1;
    softwareName = "SOFTWARE";
    psygnosisName = "Psygnosis";
    studiosName = "Studios";
    wipeoutName = "WipeoutXL";
    directoryValue = "DirectoryName";
    sourceValue = "Source Directory";
    pathSize = sizeof(path);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, softwareName, 0, KEY_READ, &softwareKey) == ERROR_SUCCESS) {
        if (RegOpenKeyExA(softwareKey, psygnosisName, 0, KEY_READ, &psygnosisKey) == ERROR_SUCCESS) {
            if (RegOpenKeyExA(psygnosisKey, studiosName, 0, KEY_READ, &studiosKey) == ERROR_SUCCESS) {
                if (RegOpenKeyExA(studiosKey, wipeoutName, 0, KEY_READ, &wipeoutKey) == ERROR_SUCCESS) {
                    if (RegQueryValueExA(wipeoutKey, directoryValue, NULL, NULL, (LPBYTE)path, &pathSize) == ERROR_SUCCESS) {
                        strcpy(wobbleDir0, path);
                    }
                    RegQueryValueExA(wipeoutKey, sourceValue, NULL, NULL, (LPBYTE)path, &pathSize);
                    if (RegQueryValueExA(wipeoutKey, sourceValue, NULL, NULL, (LPBYTE)path, &pathSize) == ERROR_SUCCESS) {
                        result = 0;
                        strcpy(wobbleDir1, path);
                    }
                }
            }
        }
    }
    return result;
}
#else
int LoadInstallPaths(void)
{
    HKEY wipeoutKey;
    HKEY softwareKey;
    HKEY psygnosisKey;
    HKEY studiosKey;
    DWORD pathSize;
    char path[1024];
    int result;

    result = 1;
    pathSize = sizeof(path);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE", 0, KEY_READ, &softwareKey) == ERROR_SUCCESS) {
        if (RegOpenKeyExA(softwareKey, "Psygnosis", 0, KEY_READ, &psygnosisKey) == ERROR_SUCCESS) {
            if (RegOpenKeyExA(psygnosisKey, "Studios", 0, KEY_READ, &studiosKey) == ERROR_SUCCESS) {
                if (RegOpenKeyExA(studiosKey, "WipeoutXL", 0, KEY_READ, &wipeoutKey) == ERROR_SUCCESS) {
                    if (RegQueryValueExA(wipeoutKey, "DirectoryName", NULL, NULL, (LPBYTE)path, &pathSize) == ERROR_SUCCESS) {
                        strcpy(wobbleDir0, path);
                    }
                    RegQueryValueExA(wipeoutKey, "Source Directory", NULL, NULL, (LPBYTE)path, &pathSize);
                    if (RegQueryValueExA(wipeoutKey, "Source Directory", NULL, NULL, (LPBYTE)path, &pathSize) == ERROR_SUCCESS) {
                        result = 0;
                        strcpy(wobbleDir1, path);
                    }
                }
            }
        }
    }
    return result;
}
#endif

#include "d3denum.h"


#ifdef EQUIV
#ifndef NETPLAY
/* Copies an 8-bit bitmap into the texture pages starting at firstPage. The pages are interleaved row by
   row in the file: page k takes image rows k, k + pageCount, k + 2 * pageCount, ... */
void LoadBitmapTextures(int unused, char *filename, short firstPage, short pageCount)
{
    FILE *file;
    DDSURFACEDESC desc;
    DDPIXELFORMAT pixelFormat;
    struct {
        BITMAPFILEHEADER file;
        BITMAPINFOHEADER info;
        RGBQUAD colours[256];
        unsigned char *pixels;
    } bitmap;
    PixelChannels channels;
    IDirectDrawSurface2 *surface2;
    unsigned short page;
    unsigned short i;
    int bitmapRow = 0;
    int bitmapRowStep;
    unsigned int rowStep;
    int srcRow;
    int endPage;
    int imageSize;
    DWORD row, column;
    unsigned char *target;
    unsigned char *source;

    file = fopen(filename, "rb");
    if (file == NULL) {
        FileAccessError(filename);
        return;
    }
    fread(&bitmap.file, 1, sizeof(bitmap.file), file);
    fread(&bitmap.info, 1, sizeof(bitmap.info), file);
    if (bitmap.info.biBitCount == 24) {
        imageSize = bitmap.info.biWidth * bitmap.info.biHeight * 3;
    } else if (bitmap.info.biBitCount == 8) {
        imageSize = bitmap.info.biWidth * bitmap.info.biHeight;
    } else {
        imageSize = *(int *)&bitmap.file;
    }
    bitmap.pixels = bmpBuffer;
    if (bitmap.pixels == NULL) {
        OutputDebugStringA("Not enough memory to load textures");
        GlobalFree(bitmap.pixels);
        fclose(file);
        return;
    }
    if (bitmap.info.biBitCount == 8) {
        fread(bitmap.colours, 4, 256, file);
        for (i = 0; i < 256; i++) {
            defaultPaletteEntries[i].peRed = bitmap.colours[i].rgbRed;
            defaultPaletteEntries[i].peGreen = bitmap.colours[i].rgbGreen;
            defaultPaletteEntries[i].peBlue = bitmap.colours[i].rgbBlue;
            defaultPaletteEntries[i].peFlags = 0x40;
        }
        memcpy(savedPalette, defaultPaletteEntries, sizeof(savedPalette));
        if (defaultPalette == NULL) {
            ddResult = IDirectDraw_CreatePalette(lpDD, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
                savedPalette, &defaultPalette, NULL);
#ifdef NETPLAY
            dxErrorLine = 0x1457;
#else
            dxErrorLine = 0x116b;
#endif
            LogDXError();
        } else {
            ddResult = IDirectDrawPalette_SetEntries(defaultPalette, 0, 0, 256, savedPalette);
        }
        if (!rgbTextures) {
            ddResult = IDirectDrawSurface_SetPalette(primarySurface, defaultPalette);
#ifdef NETPLAY
            dxErrorLine = 0x145e;
#else
            dxErrorLine = 0x1172;
#endif
            LogDXError();
            ddResult = IDirectDrawSurface_SetPalette(backBuffer, defaultPalette);
#ifdef NETPLAY
            dxErrorLine = 0x145f;
#else
            dxErrorLine = 0x1173;
#endif
            LogDXError();
        }
        for (page = firstPage; page < firstPage + pageCount; page++) {
            ddResult = IDirectDrawSurface_SetPalette(textureRecords[page].surfaces[2], defaultPalette);
#ifdef NETPLAY
            dxErrorLine = 0x146e;
#else
            dxErrorLine = 0x1182;
#endif
            LogDXError();
        }
    }
    fread(bitmap.pixels, 1, imageSize, file);
    fclose(file);

    pixelFormat.dwSize = sizeof(pixelFormat);
    ddResult = IDirectDrawSurface_GetPixelFormat(textureRecords[0].surfaces[2], &pixelFormat);
#ifdef NETPLAY
    dxErrorLine = 0x1477;
#else
    dxErrorLine = 0x118b;
#endif
    LogDXError();
    if (bitmap.info.biBitCount != 8) {
        GetPixelChannels(&channels, pixelFormat);
    }
    desc.dwSize = sizeof(desc);
    IDirectDrawSurface_GetSurfaceDesc(textureRecords[0].surfaces[2], &desc);
    page = firstPage;
    bitmapRowStep = pageCount;

    endPage = firstPage + pageCount;
    for (; page < endPage; page++) {
        desc.dwSize = sizeof(desc);
        IDirectDrawSurface_GetSurfaceDesc(textureRecords[page].surfaces[2], &desc);
        ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[2],
            &IID_IDirectDrawSurface2, (void **)&surface2);
#ifdef NETPLAY
        dxErrorLine = 0x1490;
#else
        dxErrorLine = 0x11a4;
#endif
        LogDXError();
        desc.dwSize = sizeof(desc);
        ddResult = IDirectDrawSurface2_Lock(surface2, NULL, &desc, DDLOCK_WAIT, NULL);
#ifdef NETPLAY
        dxErrorLine = 0x1492;
#else
        dxErrorLine = 0x11a6;
#endif
        LogDXError();
        target = desc.lpSurface;
        source = bitmap.pixels;
        row = 0;
        if (row < desc.dwHeight) {
            srcRow = bitmapRow;
            rowStep = bitmapRowStep;
            do {
                for (column = 0; column < desc.dwWidth; column++) {
                    target[row * desc.lPitch + column] = source[srcRow * desc.dwWidth + column];
                }
                srcRow += rowStep;
                row++;
            } while (row < desc.dwHeight);
        }
        bitmapRow++;
        ddResult = IDirectDrawSurface2_Unlock(surface2, NULL);
#ifdef NETPLAY
        dxErrorLine = 0x14a9;
#else
        dxErrorLine = 0x11bd;
#endif
        LogDXError();
    }
    ddResult = IDirectDrawSurface_QueryInterface(textureRecords[11].surfaces[2], &IID_IDirectDrawSurface2,
        (void **)&surface2);
#ifdef NETPLAY
    dxErrorLine = 0x14b4;
#else
    dxErrorLine = 0x11c8;
#endif
    LogDXError();
    desc.dwSize = sizeof(desc);
    ddResult = IDirectDrawSurface2_Lock(surface2, NULL, &desc, DDLOCK_WAIT, NULL);
#ifdef NETPLAY
    dxErrorLine = 0x14b6;
#else
    dxErrorLine = 0x11ca;
#endif
    LogDXError();
    target = desc.lpSurface;
    for (column = 0; column < desc.dwWidth; column++) {
        target[110 * desc.lPitch + column] = (unsigned char)column;
    }
    ddResult = IDirectDrawSurface2_Unlock(surface2, NULL);
#ifdef NETPLAY
    dxErrorLine = 0x14c0;
#else
    dxErrorLine = 0x11d4;
#endif
    LogDXError();

    for (page = firstPage; page < endPage; page++) {
        ddResult = surfaceTextures2[page]->lpVtbl->GetHandle(surfaceTextures2[page], lpD3DDevice, &textureHandles[page]);
#ifdef NETPLAY
        dxErrorLine = 0x14d6;
#else
        dxErrorLine = 0x11ea;
#endif
        LogDXError();
    }
}
#endif
#endif

#ifdef NETPLAY
#pragma intrinsic(memcpy)
void LoadBitmapTextures(int unused, char *filename, short firstPage, short pageCount)
{
    int size;
    unsigned char *pixelOutput;
    PixelChannels rgbChannels;
    unsigned short maskRed;
    struct {
        BITMAPFILEHEADER inputFile;
        BITMAPINFOHEADER info;
        RGBQUAD colours[256];
        unsigned char *pixels;
    } bitmapData;
    unsigned short maskBlue;
    unsigned short greenChannelMask;
    DWORD column;
    FILE *inputFile;
    DDSURFACEDESC surfaceDescription;
    DWORD pixelCount;
    IDirectDrawSurface2 *textureSurfaceObject;
    DDPIXELFORMAT surfacePixelFormat;
    int imageRowNumber;
    unsigned char *sourcePixelData;
    DWORD lineCounter;
    unsigned short localIndex;

    maskRed = 0;
    greenChannelMask = 0;
    maskBlue = 0;
    imageRowNumber = 0;
    pixelCount = 0;
    inputFile = fopen(filename, "rb");
    if (inputFile == NULL) {
        FileAccessError(filename);
        return;
    }
    fread(&bitmapData.inputFile, 1, sizeof(bitmapData.inputFile), inputFile);
    fread(&bitmapData.info, 1, sizeof(bitmapData.info), inputFile);
    if (bitmapData.info.biBitCount == 24) {
        size = bitmapData.info.biWidth * bitmapData.info.biHeight * 3;
    } else if (bitmapData.info.biBitCount == 8) {
        size = bitmapData.info.biWidth * bitmapData.info.biHeight;
    }
    bitmapData.pixels = bmpBuffer;
    if (bitmapData.pixels == NULL) {
        OutputDebugStringA("Not enough memory to load textures");
        GlobalFree(bitmapData.pixels);
        fclose(inputFile);
        return;
    }
    if (bitmapData.info.biBitCount == 8) {
        fread(bitmapData.colours, 4, 256, inputFile);
        for (localIndex = 0; localIndex < 256; localIndex++) {
            defaultPaletteEntries[localIndex].peRed = bitmapData.colours[localIndex].rgbRed;
            defaultPaletteEntries[localIndex].peGreen = bitmapData.colours[localIndex].rgbGreen;
            defaultPaletteEntries[localIndex].peBlue = bitmapData.colours[localIndex].rgbBlue;
            defaultPaletteEntries[localIndex].peFlags = 0x40;
        }
        memcpy(savedPalette, defaultPaletteEntries, sizeof(savedPalette));
        if (defaultPalette == NULL) {
            ddResult = IDirectDraw_CreatePalette(lpDD, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
                savedPalette, &defaultPalette, NULL);
            dxErrorLine = 0x1457;
            LogDXError();
        } else {
            ddResult = IDirectDrawPalette_SetEntries(defaultPalette, 0, 0, 256, savedPalette);
        }
        if (!rgbTextures) {
            ddResult = IDirectDrawSurface_SetPalette(primarySurface, defaultPalette);
            dxErrorLine = 0x145e;
            LogDXError();
            ddResult = IDirectDrawSurface_SetPalette(backBuffer, defaultPalette);
            dxErrorLine = 0x145f;
            LogDXError();
        }
        for (localIndex = firstPage; localIndex < firstPage + pageCount; localIndex++) {
            ddResult = IDirectDrawSurface_SetPalette(textureRecords[localIndex].surfaces[2], defaultPalette);
            dxErrorLine = 0x146e;
            LogDXError();
        }
    }
    fread(bitmapData.pixels, 1, size, inputFile);
    fclose(inputFile);

    surfacePixelFormat.dwSize = sizeof(surfacePixelFormat);
    ddResult = IDirectDrawSurface_GetPixelFormat(textureRecords[0].surfaces[2], &surfacePixelFormat);
    dxErrorLine = 0x1477;
    LogDXError();
    if (bitmapData.info.biBitCount != 8) {
        GetPixelChannels(&rgbChannels, surfacePixelFormat);
    }
    surfaceDescription.dwSize = sizeof(surfaceDescription);
    IDirectDrawSurface_GetSurfaceDesc(textureRecords[0].surfaces[2], &surfaceDescription);
    maskRed = (1 << rgbChannels.redBits) - 1;
    greenChannelMask = (1 << rgbChannels.greenBits) - 1;
    maskBlue = (1 << rgbChannels.blueBits) - 1;
    localIndex = firstPage;
    for (; localIndex < firstPage + pageCount; localIndex++) {
        surfaceDescription.dwSize = sizeof(surfaceDescription);
        IDirectDrawSurface_GetSurfaceDesc(textureRecords[localIndex].surfaces[2], &surfaceDescription);
        ddResult = IDirectDrawSurface_QueryInterface(textureRecords[localIndex].surfaces[2],
            &IID_IDirectDrawSurface2, (void **)&textureSurfaceObject);
        dxErrorLine = 0x1490;
        LogDXError();
        surfaceDescription.dwSize = sizeof(surfaceDescription);
        ddResult = IDirectDrawSurface2_Lock(textureSurfaceObject, NULL, &surfaceDescription, DDLOCK_WAIT, NULL);
        dxErrorLine = 0x1492;
        LogDXError();
        pixelOutput = surfaceDescription.lpSurface;
        sourcePixelData = bitmapData.pixels;
        pixelCount = 0;
        for (lineCounter = 0; lineCounter < surfaceDescription.dwHeight; lineCounter++) {
            for (column = 0; column < surfaceDescription.dwWidth; column++) {
                pixelOutput[lineCounter * surfaceDescription.lPitch + column] = sourcePixelData[pageCount * surfaceDescription.dwWidth * pixelCount + imageRowNumber * surfaceDescription.dwWidth + column];
            }
            pixelCount++;
        }
        imageRowNumber++;
        ddResult = IDirectDrawSurface2_Unlock(textureSurfaceObject, NULL);
        dxErrorLine = 0x14a9;
        LogDXError();
    }
    ddResult = IDirectDrawSurface_QueryInterface(textureRecords[11].surfaces[2], &IID_IDirectDrawSurface2,
        (void **)&textureSurfaceObject);
    dxErrorLine = 0x14b4;
    LogDXError();
    surfaceDescription.dwSize = sizeof(surfaceDescription);
    ddResult = IDirectDrawSurface2_Lock(textureSurfaceObject, NULL, &surfaceDescription, DDLOCK_WAIT, NULL);
    dxErrorLine = 0x14b6;
    LogDXError();
    pixelOutput = surfaceDescription.lpSurface;
    for (column = 0; column < surfaceDescription.dwWidth; column++) {
        pixelOutput[110 * surfaceDescription.lPitch + column] = (unsigned char)column;
    }
    ddResult = IDirectDrawSurface2_Unlock(textureSurfaceObject, NULL);
    dxErrorLine = 0x14c0;
    LogDXError();

    for (localIndex = firstPage; localIndex < firstPage + pageCount; localIndex++) {
        ddResult = surfaceTextures2[localIndex]->lpVtbl->GetHandle(surfaceTextures2[localIndex], lpD3DDevice, &textureHandles[localIndex]);
        dxErrorLine = 0x14d6;
        LogDXError();
    }
}
#pragma function(memcpy)
#endif

/* The 16-bit depth test reads the capability mask's high byte, as in the executable. */
#ifndef NETPLAY
#pragma optimize("a", on)
void EnumerateDisplayDrivers(void)
{
    short driverIndex;
    short deviceIndex;
    short modeIndex;

    nameDrawFirst = 0;
    ddResult = DirectDrawEnumerateThunk((LPDDENUMCALLBACKA)EnumDriverCallback, &lpDD);
    dxErrorLine = 0x11fb;
    LogDXError();
    for (driverIndex = 0; driverIndex < displayDriverCount; driverIndex++) {
        videoModeIndex = driverIndex;
        InitDirectDrawDriver(0, driverIndex);
        ddResult = IDirectDraw2_EnumDisplayModes(lpDD2, 1, NULL, NULL,
                                                 (LPDDENUMMODESCALLBACK)EnumDisplayModesCallback);
        dxErrorLine = 0x1207;
        LogDXError();
        displayDrivers[driverIndex].deviceCount = 0;
        ddResult = direct3D->lpVtbl->EnumDevices(direct3D, (void *)EnumDeviceCallback, NULL);
        dxErrorLine = 0x120c;
        LogDXError();
        for (deviceIndex = 0; deviceIndex < displayDrivers[driverIndex].deviceCount; deviceIndex++) {
            DWORD renderBitDepth = displayDrivers[driverIndex].devices[deviceIndex].hardwareCaps[39] |
                                   displayDrivers[driverIndex].devices[deviceIndex].softwareCaps[39];
            if (renderBitDepth & DDBD_8) {
                for (modeIndex = 0; modeIndex < 6; modeIndex++) {
                    if (availableModes[driverIndex][modeIndex].present8) {
                        displayDrivers[driverIndex].supported8[deviceIndex][modeIndex] = 1;
                        displayDrivers[driverIndex].formats8[deviceIndex][modeIndex] = availableModes[driverIndex][modeIndex].format8;
                    }
                }
            }
            if (HIBYTE(renderBitDepth) & HIBYTE(DDBD_16)) {
                for (modeIndex = 0; modeIndex < 6; modeIndex++) {
                    if (availableModes[driverIndex][modeIndex].present16) {
                        displayDrivers[driverIndex].supported16[deviceIndex][modeIndex] = 1;
                    }
                }
            }
        }
        ddResult = IDirectDraw2_SetCooperativeLevel(lpDD2, g_hwnd, DDSCL_NORMAL);
        dxErrorLine = 0x123e;
        LogDXError();
        direct3D->lpVtbl->Release(direct3D);
        lpDD2->lpVtbl->Release(lpDD2);
        lpDD->lpVtbl->Release(lpDD);
        lpDD = NULL;
        lpDD2 = NULL;
        direct3D = NULL;
    }
}
#endif
#ifdef EQUIV
#ifdef NETPLAY
/* Adding zero preserves the NET loop tests' global-first load order. */
#pragma optimize("a", off)
void EnumerateDisplayDrivers(void)
{
    short driverIndex;
    short deviceIdx;
    short modeIdx;
    DDSURFACEDESC surfaceDesc;

    nameDrawFirst = 0;
    ddResult = DirectDrawEnumerateA((LPDDENUMCALLBACKA)EnumDriverCallback, &lpDD);
    dxErrorLine = 0x14eb;
    LogDXError();
    for (driverIndex = 0; driverIndex < displayDriverCount + 0; driverIndex++) {
        videoModeIndex = driverIndex;
        InitDirectDrawDriver(0, videoModeIndex);
        ddResult = IDirectDraw2_EnumDisplayModes(lpDD2, 1, NULL, NULL,
                                                 (LPDDENUMMODESCALLBACK)EnumDisplayModesCallback);
        dxErrorLine = 0x14f7;
        LogDXError();
        displayDrivers[driverIndex].deviceCount = 0;
        ddResult = direct3D->lpVtbl->EnumDevices(direct3D, (void *)EnumDeviceCallback, NULL);
        dxErrorLine = 0x14fc;
        LogDXError();
        /* The NET build fills in a flippable back-buffer surface description here; nothing reads it. */
        surfaceDesc.dwSize = sizeof(surfaceDesc);
        surfaceDesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        surfaceDesc.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
        surfaceDesc.dwBackBufferCount = 1;
        for (deviceIdx = 0; deviceIdx < displayDrivers[driverIndex].deviceCount + 0; deviceIdx++) {
            DWORD renderDepthMask = displayDrivers[driverIndex].devices[deviceIdx].hardwareCaps[39] |
                                   displayDrivers[driverIndex].devices[deviceIdx].softwareCaps[39];
            if (renderDepthMask & DDBD_8) {
                for (modeIdx = 0; modeIdx < 6; modeIdx++) {
                    if ((unsigned char)availableModes[driverIndex][modeIdx].present8) {
                        displayDrivers[driverIndex].supported8[deviceIdx][modeIdx] = 1;
                        displayDrivers[driverIndex].formats8[deviceIdx][modeIdx] = availableModes[driverIndex][modeIdx].format8;
                    }
                }
            }
            if (renderDepthMask & DDBD_16) {
                for (modeIdx = 0; modeIdx < 6; modeIdx++) {
                    if ((unsigned char)availableModes[driverIndex][modeIdx].present16) {
                        displayDrivers[driverIndex].supported16[deviceIdx][modeIdx] = 1;
                    }
                }
            }
        }
        ddResult = IDirectDraw2_SetCooperativeLevel(lpDD2, g_hwnd, DDSCL_NORMAL);
        dxErrorLine = 0x153c;
        LogDXError();
        direct3D->lpVtbl->Release(direct3D);
        lpDD2->lpVtbl->Release(lpDD2);
        lpDD->lpVtbl->Release(lpDD);
        lpDD = NULL;
        lpDD2 = NULL;
        direct3D = NULL;
    }
}
#endif
#endif
#pragma optimize("a", off)

/* Creates the DirectDraw object for an enumerated driver, then its DirectDraw2 and Direct3D interfaces. The first argument is never read. */
void InitDirectDrawDriver(int unused, short driverIndex)
{
#ifdef NETPLAY
    ddResult = DirectDrawCreate(displayDrivers[driverIndex].guid, &lpDD, NULL);
#else
    ddResult = DirectDrawCreateThunk(displayDrivers[driverIndex].guid, &lpDD, NULL);
#endif
#ifdef NETPLAY
    dxErrorLine = 0x1553;
#else
    dxErrorLine = 0x1255;
#endif
    LogDXError();
    ddResult = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **)&lpDD2);
#ifdef NETPLAY
    dxErrorLine = 0x1554;
#else
    dxErrorLine = 0x1256;
#endif
    LogDXError();
    ddResult = IDirectDraw_SetCooperativeLevel(lpDD, g_hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWMODEX);
#ifdef NETPLAY
    dxErrorLine = 0x1555;
#else
    dxErrorLine = 0x1257;
#endif
    LogDXError();
    ddResult = IDirectDraw_QueryInterface(lpDD, &IID_IDirect3D, (void **)&direct3D);
#ifdef NETPLAY
    dxErrorLine = 0x1556;
#else
    dxErrorLine = 0x1258;
#endif
    LogDXError();
}

/* Records supported resolutions and their pixel formats. */
#ifndef POWERVR
int __stdcall EnumDisplayModesCallback(LPDDSURFACEDESC surface, LPVOID context)
{
    int i;
    int bitCount;

    wsprintfA(g_cdMessage, "%dx%dx%d\n", surface->dwWidth, surface->dwHeight, surface->ddpfPixelFormat.dwRGBBitCount);
    OutputDebugStringA(g_cdMessage);
    bitCount = surface->ddpfPixelFormat.dwRGBBitCount;
    if (bitCount == 8 || bitCount == 16) {
        for (i = 0; i < 6; i++) {
            if (displayResolutions[i].width == surface->dwWidth && displayResolutions[i].height == surface->dwHeight) {
                if (bitCount == 8) {
                    availableModes[videoModeIndex][i].present8 = 1;
                    availableModes[videoModeIndex][i].format8 = surface->ddpfPixelFormat;
                } else {
                    availableModes[videoModeIndex][i].present16 = 1;
                    availableModes[videoModeIndex][i].format16 = surface->ddpfPixelFormat;
                }
            }
        }
    }
    return 1;
}
#endif

#include "pc_texture.h"

#ifdef EQUIV
/* Loads a 16-bit bitmap into the texture pages starting at firstPage. Each pixel is 5:5:5 RGB; the
   channels are rescaled to the device format and the alpha bit is set for visible texels. */
#ifndef NETPLAY
void Load16BitBitmapTextures(int unused, char *filename, short firstPage, short pageCount)
{
    FILE *file;
    int bitmapRow = 0;
    unsigned short red5, green5;
    struct {
        PixelChannels channels;
        DDSURFACEDESC desc;
        short header[9];
        DDPIXELFORMAT pixelFormat;
    } bitmap;
    int srcRow;
    unsigned int rowStep;
    int pageStride;
    int endPage;
    int startPage;
    DWORD row;
    unsigned short pixel;
    unsigned short red, green, blue;
    unsigned short blue5;
    unsigned short *target;

    file = fopen(filename, "rb");
    if (file == NULL) {
        FileAccessError(filename);
        return;
    }
    fread(bitmap.header, 9, 2, file);
    fread(bmpBuffer, 1, (bitmap.header[6] & 0xffff) * (unsigned short)bitmap.header[7] * 2, file);
    fclose(file);

    {
        short page;
        IDirectDrawSurface2 *surface2;

        page = firstPage;
        startPage = page;
        pageStride = pageCount;
        endPage = startPage + pageStride;
        if (endPage > startPage) {
            do {
                bitmap.pixelFormat.dwSize = sizeof(bitmap.pixelFormat);
                ddResult = IDirectDrawSurface_GetPixelFormat(textureRecords[page].surfaces[0], &bitmap.pixelFormat);
#ifdef NETPLAY
                dxErrorLine = 0x1676;
#else
                dxErrorLine = 0x1378;
#endif
                LogDXError();
                ddResult = IDirectDrawSurface_GetPixelFormat(textureRecords[page].surfaces[2], &bitmap.pixelFormat);
#ifdef NETPLAY
                dxErrorLine = 0x1677;
#else
                dxErrorLine = 0x1379;
#endif
                LogDXError();
                GetPixelChannels(&bitmap.channels, bitmap.pixelFormat);
                bitmap.desc.dwSize = sizeof(bitmap.desc);
                IDirectDrawSurface_GetSurfaceDesc(textureRecords[page].surfaces[0], &bitmap.desc);
                IDirectDrawSurface_GetSurfaceDesc(textureRecords[page].surfaces[0], &bitmap.desc);
                ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[0],
                    &IID_IDirectDrawSurface2, (void **)&surface2);
#ifdef NETPLAY
                dxErrorLine = 0x1687;
#else
                dxErrorLine = 0x1389;
#endif
                LogDXError();
                bitmap.desc.dwSize = sizeof(bitmap.desc);
                ddResult = IDirectDrawSurface2_Lock(surface2, NULL, &bitmap.desc, DDLOCK_WAIT, NULL);
#ifdef NETPLAY
                dxErrorLine = 0x1689;
#else
                dxErrorLine = 0x138b;
#endif
                LogDXError();
                target = (unsigned short *)bitmap.desc.lpSurface;
                row = 0;
                if (row < bitmap.desc.dwHeight) {
                    srcRow = bitmapRow;
                    rowStep = pageStride;
                    do {
                        DWORD column;

                        for (column = 0; column < bitmap.desc.dwWidth; column++) {
                            target[row * (bitmap.desc.lPitch >> 1) + column] = 0;
                            pixel = ((unsigned short *)bmpBuffer)[srcRow * bitmap.desc.dwWidth + column];
                            red5 = (pixel & 0x7c00) >> 10;
                            green5 = (pixel & 0x3e0) >> 5;
                            blue5 = pixel & 0x1f;
                            red = red5;
                            if (bitmap.channels.redBits < 5) {
                                red >>= 5 - bitmap.channels.redBits;
                                target[row * (bitmap.desc.lPitch >> 1) + column] = red << bitmap.channels.redShift;
                            } else {
                                red <<= bitmap.channels.redBits - 5;
                                target[row * (bitmap.desc.lPitch >> 1) + column] = red << bitmap.channels.redShift;
                            }
                            green = green5;
                            if (bitmap.channels.greenBits < 5) {
                                green >>= 5 - bitmap.channels.greenBits;
                                target[row * (bitmap.desc.lPitch >> 1) + column] |= green << bitmap.channels.greenShift;
                            } else {
                                green <<= bitmap.channels.greenBits - 5;
                                target[row * (bitmap.desc.lPitch >> 1) + column] |= green << bitmap.channels.greenShift;
                            }
                            blue = blue5;
                            if (bitmap.channels.blueBits < 5) {
                                blue >>= 5 - bitmap.channels.blueBits;
                                target[row * (bitmap.desc.lPitch >> 1) + column] |= blue << bitmap.channels.blueShift;
                            } else {
                                blue <<= bitmap.channels.blueBits - 5;
                                target[row * (bitmap.desc.lPitch >> 1) + column] |= blue << bitmap.channels.blueShift;
                            }
                            if (!blendByBucket && textureRecords[page].type == 3) {
                                if (textureRecords[page].typeVariant == 1) {
                                    if (red5 || green5 || blue5) {
                                        target[row * (bitmap.desc.lPitch >> 1) + column] |= 1 << bitmap.channels.alphaShift;
                                    }
                                } else {
                                    if (red5 || green5 || blue5) {
                                        target[row * (bitmap.desc.lPitch >> 1) + column] |= 1 << bitmap.channels.alphaShift;
                                    }
                                }
                            }
                        }
                        srcRow += rowStep;
                        row++;
                    } while (row < bitmap.desc.dwHeight);
                }
                bitmapRow++;
                ddResult = IDirectDrawSurface2_Unlock(surface2, NULL);
#ifdef NETPLAY
                dxErrorLine = 0x16f4;
#else
                dxErrorLine = 0x13f6;
#endif
                LogDXError();
                page++;
            } while (page < endPage);
        }
    }

    {
        short page;

        page = firstPage;
        if (endPage > startPage) {
            do {
                ConvertTexturePage(page);
                ddResult = surfaceTextures2[page]->lpVtbl->Load(surfaceTextures2[page], surfaceTextures1[page]);
#ifdef NETPLAY
                dxErrorLine = 0x1719;
#else
                dxErrorLine = 0x141b;
#endif
                LogDXError();
                ddResult = surfaceTextures2[page]->lpVtbl->GetHandle(surfaceTextures2[page], lpD3DDevice, &textureHandles[page]);
#ifdef NETPLAY
                dxErrorLine = 0x171b;
#else
                dxErrorLine = 0x141d;
#endif
                LogDXError();
                page++;
            } while (page < endPage);
        }
    }
}
#endif
#endif

#ifdef EQUIV
#ifdef NETPLAY
void Load16BitBitmapTextures(int unused, char *filename, short firstPage, short pageCount)
{
    PixelChannels rgbChannels;
    unsigned short maskRed;
    unsigned short maskBlue;
    unsigned short greenSample;
    unsigned short header[9];
    RECT imageArea;
    unsigned short redPixel;
    int imageSize;
    DDSURFACEDESC textureDescription;
    DWORD textureX;
    unsigned short greenColorMask;
    FILE *inputFile;
    unsigned short *pixelSource;
    unsigned short *wordOutput;
    DWORD lineIndex;
    short page;
    unsigned short blueValue;
    RECT targetRect;
    int rowNumber;
    unsigned char *bitmapSource;
    DWORD pixelCount;
    DDPIXELFORMAT surfacePixelFormat;
    IDirectDrawSurface2 *textureSurfaceObject;

    maskRed = 0;
    greenColorMask = 0;
    maskBlue = 0;
    rowNumber = 0;
    pixelCount = 0;
    inputFile = fopen(filename, "rb");
    if (inputFile == NULL) {
        FileAccessError(filename);
        return;
    }
    fread(header, 9, 2, inputFile);
    imageSize = (header[6] & 0xffff) * (unsigned short)header[7] * 2;
    bitmapSource = bmpBuffer;
    fread(bitmapSource, 1, imageSize, inputFile);
    fclose(inputFile);

    for (page = firstPage; page < firstPage + pageCount; page++) {
        surfacePixelFormat.dwSize = sizeof(surfacePixelFormat);
        ddResult = IDirectDrawSurface_GetPixelFormat(textureRecords[page].surfaces[0], &surfacePixelFormat);
        dxErrorLine = 0x1676;
        LogDXError();
        ddResult = IDirectDrawSurface_GetPixelFormat(textureRecords[page].surfaces[2], &surfacePixelFormat);
        dxErrorLine = 0x1677;
        LogDXError();
        GetPixelChannels(&rgbChannels, surfacePixelFormat);
        textureDescription.dwSize = sizeof(textureDescription);
        IDirectDrawSurface_GetSurfaceDesc(textureRecords[page].surfaces[0], &textureDescription);
        maskRed = (1 << rgbChannels.redBits) - 1;
        greenColorMask = (1 << rgbChannels.greenBits) - 1;
        maskBlue = (1 << rgbChannels.blueBits) - 1;
        IDirectDrawSurface_GetSurfaceDesc(textureRecords[page].surfaces[0], &textureDescription);
        ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[0],
            &IID_IDirectDrawSurface2, (void **)&textureSurfaceObject);
        dxErrorLine = 0x1687;
        LogDXError();
        textureDescription.dwSize = sizeof(textureDescription);
        ddResult = IDirectDrawSurface2_Lock(textureSurfaceObject, NULL, &textureDescription, DDLOCK_WAIT, NULL);
        dxErrorLine = 0x1689;
        LogDXError();
        wordOutput = (unsigned short *)textureDescription.lpSurface;
        pixelSource = (unsigned short *)bitmapSource;
        pixelCount = 0;
        for (lineIndex = 0; lineIndex < textureDescription.dwHeight; lineIndex++) {
            for (textureX = 0; textureX < textureDescription.dwWidth; textureX++) {
                wordOutput[(textureDescription.lPitch >> 1) * lineIndex + textureX] = 0;
                redPixel = (pixelSource[pageCount * textureDescription.dwWidth * pixelCount + rowNumber * textureDescription.dwWidth + textureX] >> 10) & 0x1f;
                greenSample = (pixelSource[pageCount * textureDescription.dwWidth * pixelCount + rowNumber * textureDescription.dwWidth + textureX] >> 5) & 0x1f;
                blueValue = pixelSource[pageCount * textureDescription.dwWidth * pixelCount + rowNumber * textureDescription.dwWidth + textureX] & 0x1f;
                if (rgbChannels.redBits < 5) {
                    wordOutput[(textureDescription.lPitch >> 1) * lineIndex + textureX] =
                        (redPixel >> (5 - rgbChannels.redBits)) << rgbChannels.redShift;
                } else {
                    wordOutput[(textureDescription.lPitch >> 1) * lineIndex + textureX] =
                        (redPixel << (rgbChannels.redBits - 5)) << rgbChannels.redShift;
                }
                if (rgbChannels.greenBits < 5) {
                    wordOutput[(textureDescription.lPitch >> 1) * lineIndex + textureX] |=
                        (greenSample >> (5 - rgbChannels.greenBits)) << rgbChannels.greenShift;
                } else {
                    wordOutput[(textureDescription.lPitch >> 1) * lineIndex + textureX] |=
                        (greenSample << (rgbChannels.greenBits - 5)) << rgbChannels.greenShift;
                }
                if (rgbChannels.blueBits < 5) {
                    wordOutput[(textureDescription.lPitch >> 1) * lineIndex + textureX] |=
                        (blueValue >> (5 - rgbChannels.blueBits)) << rgbChannels.blueShift;
                } else {
                    wordOutput[(textureDescription.lPitch >> 1) * lineIndex + textureX] |=
                        (blueValue << (rgbChannels.blueBits - 5)) << rgbChannels.blueShift;
                }
                if (!blendByBucket && textureRecords[page].type == 3) {
                    if (textureRecords[page].typeVariant == 1) {
                        if (redPixel || greenSample || blueValue) {
                            wordOutput[(textureDescription.lPitch >> 1) * lineIndex + textureX] |= 1 << rgbChannels.alphaShift;
                        }
                    } else {
                        if (redPixel || greenSample || blueValue) {
                            wordOutput[(textureDescription.lPitch >> 1) * lineIndex + textureX] |= 1 << rgbChannels.alphaShift;
                        }
                    }
                }
            }
            pixelCount++;
        }
        rowNumber++;
        ddResult = IDirectDrawSurface2_Unlock(textureSurfaceObject, NULL);
        dxErrorLine = 0x16f4;
        LogDXError();
    }
    for (page = firstPage; page < firstPage + pageCount; page++) {
        /* The original prepares both rectangles but never passes them to a blit. */
        targetRect.left = 0;
        imageArea.left = targetRect.left;
        targetRect.top = 0;
        imageArea.top = targetRect.top;
        imageArea.right = textureRecords[page].width;
        imageArea.bottom = textureRecords[page].height;
        targetRect.right = 256;
        targetRect.bottom = 256;
        ConvertTexturePage(page);
        ddResult = surfaceTextures2[page]->lpVtbl->Load(surfaceTextures2[page], surfaceTextures1[page]);
        dxErrorLine = 0x1719;
        LogDXError();
        ddResult = surfaceTextures2[page]->lpVtbl->GetHandle(surfaceTextures2[page], lpD3DDevice, &textureHandles[page]);
        dxErrorLine = 0x171b;
        LogDXError();
    }
}
#endif
#endif

/* Destroys the setup screen's buttons, combo boxes and labels. */
#ifndef NETPLAY
void DestroySetupControls(void)
{
    HWND window;
    HWND *control;

    control = setupButtons;
    do {
        window = *control;
        control++;
        DestroyWindow(window);
    } while (control < setupButtons + 4);
    control = setupCombos;
    do {
        window = *control;
        control++;
        DestroyWindow(window);
    } while (control < setupCombos + 2);
    control = setupLabels;
    do {
        window = *control;
        control++;
        DestroyWindow(window);
    } while (control < setupLabels + 6);
}
#else
void DestroySetupControls(void)
{
    int i;
    int result;

    for (i = 0; i < 4; i++) {
        result = DestroyWindow(setupButtons[i]);
    }
    for (i = 0; i < 3; i++) {
        result = DestroyWindow(setupCombos[i]);
    }
    for (i = 0; i < 7; i++) {
        result = DestroyWindow(setupLabels[i]);
    }
}
#endif

#ifdef NETPLAY
/* Index of the resolution chosen in the setup list, counting only supported modes; falls off the end if none is selected. */
int SelectedResolutionIndex(void)
{
    int resolution;
    int matched;
    int i;

    resolution = 0;
    matched = 0;
    for (i = 0; i < 6; i++) {
        if (displayBits == 16) {
            if (displayDrivers[videoModeIndex].supported16[pixelFormatGroup][i]) {
                if (matched == resolutionSelection)
                    return resolution;
                matched++;
            }
        } else {
            if (displayDrivers[videoModeIndex].supported8[pixelFormatGroup][i]) {
                if (matched == resolutionSelection)
                    return resolution;
                matched++;
            }
        }
        resolution++;
    }
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* The rebuilt loop hoists displayBits and assigns its counters differently. */
int SelectedResolutionIndex(void)
{
    int i;
    int matched = 0;

    for (i = 0; i < 6; i++) {
        if (displayBits == 16) {
            if (displayDrivers[videoModeIndex].supported16[pixelFormatGroup][i]) {
                if (matched == resolutionSelection) {
                    break;
                }
                matched++;
            }
        } else {
            if (displayDrivers[videoModeIndex].supported8[pixelFormatGroup][i]) {
                if (matched == resolutionSelection) {
                    break;
                }
                matched++;
            }
        }
    }
    return i;
}
#endif
#endif

/* Destroys the first two setup buttons and labels, the combo boxes and the setup window. */
#ifndef NETPLAY
void DestroySetupScreen(void)
{
    HWND window;
    HWND *control;

    control = setupButtons;
    do {
        window = *control;
        control++;
        DestroyWindow(window);
    } while (control < setupButtons + 2);
    control = setupLabels;
    do {
        window = *control;
        control++;
        DestroyWindow(window);
    } while (control < setupLabels + 2);
    DestroyWindow(formatComboBox);
    DestroyWindow(modeComboBox);
    DestroyWindow(setupWindow);
}
#endif

#ifdef NETPLAY
void DestroySetupScreen(void)
{
    int i;
    int result;

    for (i = 0; i < 2; i++)
        result = DestroyWindow(setupButtons[i]);
    for (i = 0; i < 2; i++)
        result = DestroyWindow(setupLabels[i]);
    result = DestroyWindow(formatComboBox);
    result = DestroyWindow(modeComboBox);
    result = DestroyWindow(setupWindow);
}
#endif

/* Rebuilds the setup resolution and color lists for the selected display device. */
#ifndef NETPLAY
void UpdateSetupDisplayOptions(void)
{
    int i;
    int available;

    available = 0;
    if (formatOverride != available) {
        displayBits = 16;
        rgbTextures = 1;
    } else {
        displayBits = 8;
        rgbTextures = 0;
    }
    texture16Bit = rgbTextures;

    SendMessageA(setupCombos[0], CB_RESETCONTENT, 0, 0);
#ifdef POWERVR
    /* The PowerVR setup exposes three resolutions. */
    for (i = 0; i < 3; i++) {
#else
    for (i = 0; i < 6; i++) {
#endif
        if (displayBits == 16) {
            if (displayDrivers[videoModeIndex].supported16[pixelFormatGroup][i]) {
                SendMessageA(setupCombos[0], CB_ADDSTRING, 0,
                             (LPARAM)displayResolutionLabels[i]);
                if (pixelFormatIndex == i) {
                    resolutionSelection = available;
                }
                available++;
            }
        } else if (displayDrivers[videoModeIndex].supported8[pixelFormatGroup][i]) {
            SendMessageA(setupCombos[0], CB_ADDSTRING, 0,
                         (LPARAM)displayResolutionLabels[i]);
            if (pixelFormatIndex == i) {
                resolutionSelection = available;
            }
            available++;
        }
    }
    SendMessageA(setupCombos[0], CB_SETCURSEL, resolutionSelection, 0);

    SendMessageA(setupCombos[2], CB_RESETCONTENT, 0, 0);
    SendMessageA(setupCombos[2], CB_ADDSTRING, 0, (LPARAM)"256 Colors");
    SendMessageA(setupCombos[2], CB_ADDSTRING, 0, (LPARAM)"64K Colors");
    SendMessageA(setupCombos[2], CB_SETCURSEL, formatOverride, 0);
    SetWindowTextA(setupLabels[0],
                   displayDrivers[videoModeIndex].devices[pixelFormatGroup].name);
}
#endif
#ifdef NETPLAY
void UpdateSetupDisplayOptions(void)
{
    int i;
    int result;
    int available;
    int connectionResult;
    BOOL labelResult;

    available = 0;
    if (formatOverride != 0) {
        displayBits = 16;
        rgbTextures = 1;
        texture16Bit = 1;
    } else {
        displayBits = 8;
        rgbTextures = 0;
        texture16Bit = 0;
    }

    result = SendMessageA(setupCombos[0], CB_RESETCONTENT, 0, 0);
    for (i = 0; i < 6; i++) {
        if (displayBits == 16) {
            if (displayDrivers[videoModeIndex].supported16[pixelFormatGroup][i]) {
                result = SendMessageA(setupCombos[0], CB_ADDSTRING, 0, (LPARAM)displayResolutionLabels[i]);
                if (pixelFormatIndex == i) {
                    resolutionSelection = available;
                }
                available++;
            }
        } else {
            if (displayDrivers[videoModeIndex].supported8[pixelFormatGroup][i]) {
                result = SendMessageA(setupCombos[0], CB_ADDSTRING, 0, (LPARAM)displayResolutionLabels[i]);
                if (pixelFormatIndex == i) {
                    resolutionSelection = available;
                }
                available++;
            }
        }
    }
    result = SendMessageA(setupCombos[0], CB_SETCURSEL, resolutionSelection, 0);

    result = SendMessageA(connectionListBox, CB_RESETCONTENT, 0, 0);
    connectionCount = 0;
    connectionResult = gstEnumProvidersGPI(AddConnectionListEntry);
    if (connectionResult < 1) {
        gstDebugPrintf("gstEnumProvidersGPI: error: %d [%s]", connectionResult, gstErrorString(connectionResult));
    }
    result = SendMessageA(connectionListBox, CB_SETCURSEL, gstProviderChoice, 0);
    labelResult = SetWindowTextA(setupLabels[0],
                                 displayDrivers[videoModeIndex].devices[pixelFormatGroup].name);
}
#endif


#ifdef EQUIV
/* Box-filters texture page page into its target surface: 16-bit targets get packed RGB(A) values,
   8-bit targets get the nearest palette entry. */
#ifndef NETPLAY
void ConvertTexturePage(short page)
{
    DDSURFACEDESC sourceDesc;
    DDSURFACEDESC targetDesc;
    DDPIXELFORMAT pixelFormat;
    PixelChannels channels;
    unsigned char entry, nearest;
    short index;
    unsigned short distance, bestDistance;
    short scale, area;
    short x, y, sx, sy, i, j;
    IDirectDrawSurface2 *source;
    unsigned short pixel;
    IDirectDrawSurface2 *target;
    unsigned char *sourcePixels16, *targetPixels16;
    unsigned short redMask, greenMask, blueMask, alphaMask;
    unsigned short red, green, blue, alpha;
    unsigned char *sourcePixels8, *targetPixels8;

    scale = 1;
    pixelFormat.dwSize = sizeof(pixelFormat);
    ddResult = IDirectDrawSurface_GetPixelFormat(textureRecords[page].surfaces[0], &pixelFormat);
    if (texture16Bit) {
        GetPixelChannels(&channels, pixelFormat);
    }
    sourceDesc.dwSize = sizeof(sourceDesc);
    IDirectDrawSurface_GetSurfaceDesc(textureRecords[page].surfaces[0], &sourceDesc);
    targetDesc.dwSize = sizeof(targetDesc);
    IDirectDrawSurface_GetSurfaceDesc(textureRecords[page].surfaces[1], &targetDesc);
    redMask = (1 << channels.redBits) - 1;
    greenMask = (1 << channels.greenBits) - 1;
    blueMask = (1 << channels.blueBits) - 1;
    alphaMask = (1 << channels.alphaBits) - 1;

    if (textureRecords[page].memorySize == 0x1000) {
        scale = 4;
    } else if (textureRecords[page].memorySize == 0x4000) {
        scale = 2;
    }
    area = scale * scale;

    ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[0], &IID_IDirectDrawSurface2, (void **)&source);
#ifdef NETPLAY
    dxErrorLine = 0x19c5;
#else
    dxErrorLine = 0x167f;
#endif
    LogDXError();
    ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[1], &IID_IDirectDrawSurface2, (void **)&target);
#ifdef NETPLAY
    dxErrorLine = 0x19c6;
#else
    dxErrorLine = 0x1680;
#endif
    LogDXError();
    sourceDesc.dwSize = sizeof(sourceDesc);
    ddResult = IDirectDrawSurface2_Lock(source, NULL, &sourceDesc, DDLOCK_WAIT, NULL);
#ifdef NETPLAY
    dxErrorLine = 0x19c9;
#else
    dxErrorLine = 0x1683;
#endif
    LogDXError();
    targetDesc.dwSize = sizeof(targetDesc);
    ddResult = IDirectDrawSurface2_Lock(target, NULL, &targetDesc, DDLOCK_WAIT, NULL);
#ifdef NETPLAY
    dxErrorLine = 0x19cc;
#else
    dxErrorLine = 0x1686;
#endif
    LogDXError();

    if (texture16Bit) {
        sourcePixels16 = sourceDesc.lpSurface;
        targetPixels16 = targetDesc.lpSurface;
    } else {
        sourcePixels8 = sourceDesc.lpSurface;
        targetPixels8 = targetDesc.lpSurface;
    }
    for (y = 0; y < textureRecords[page].height; y++) {
        for (x = 0; x < textureRecords[page].width; x++) {
            red = green = blue = alpha = 0;
            sy = y * scale;
            for (j = 0; j < scale; j++) {
                sx = x * scale;
                for (i = 0; i < scale; i++) {
                    if (texture16Bit) {
                        pixel = *(unsigned short *)(sourcePixels16 + sy * (sourceDesc.lPitch & ~1) + sx * 2);
                        red += (pixel >> channels.redShift) & redMask;
                        green += (pixel >> channels.greenShift) & greenMask;
                        blue += (pixel >> channels.blueShift) & blueMask;
                        if (pixelFormat.dwRGBAlphaBitMask) {
                            alpha += (pixel >> channels.alphaShift) & alphaMask;
                        }
                    } else {
                        index = sourcePixels8[sy * sourceDesc.lPitch + sx];
                        red += savedPalette[index].peRed;
                        green += savedPalette[index].peGreen;
                        blue += savedPalette[index].peBlue;
                    }
                    sx++;
                }
                sy++;
            }
            red /= area;
            green /= area;
            blue /= area;
            if (pixelFormat.dwRGBAlphaBitMask) {
                alpha /= area;
            }
            if (texture16Bit) {
                *(unsigned short *)(targetPixels16 + (targetDesc.lPitch & ~1) * y + x * 2) = red << channels.redShift;
                *(unsigned short *)(targetPixels16 + (targetDesc.lPitch & ~1) * y + x * 2) |= green << channels.greenShift;
                *(unsigned short *)(targetPixels16 + (targetDesc.lPitch & ~1) * y + x * 2) |= blue << channels.blueShift;
                if (pixelFormat.dwRGBAlphaBitMask) {
                    *(unsigned short *)(targetPixels16 + (targetDesc.lPitch & ~1) * y + x * 2) |= alpha << channels.alphaShift;
                }
            } else {
                /* entry is a byte, so this search never terminates in an 8-bit format (as in the original) */
                bestDistance = 0xffff;
                for (entry = 0; entry < 256; entry++) {
                    distance = abs(savedPalette[entry].peBlue - blue) + abs(savedPalette[entry].peGreen - green)
                        + abs(savedPalette[entry].peRed - red);
                    if (bestDistance > distance) {
                        bestDistance = distance;
                        nearest = entry;
                    }
                }
                targetPixels8[y * targetDesc.lPitch + x] = nearest;
            }
        }
    }
    ddResult = IDirectDrawSurface2_Unlock(source, NULL);
#ifdef NETPLAY
    dxErrorLine = 0x1a2e;
#else
    dxErrorLine = 0x16e8;
#endif
    LogDXError();
    ddResult = IDirectDrawSurface2_Unlock(target, NULL);
#ifdef NETPLAY
    dxErrorLine = 0x1a2f;
#else
    dxErrorLine = 0x16e9;
#endif
    LogDXError();
}
#endif
#endif

#ifdef EQUIV
#ifdef NETPLAY
#pragma intrinsic(abs)
void ConvertTexturePage(short page)
{
    unsigned short *pixels16;
    unsigned short alpha;
    PixelChannels rgbChannels;
    unsigned short maskRed;
    short sourceXPos;
    short sourceLine;
    short sampleCount;
    unsigned char colorEntry;
    short sampleY;
    short factor;
    DDSURFACEDESC inputDesc;
    unsigned short blueChannel;
    unsigned char *dest8;
    short sourcePaletteIndex;
    unsigned short greenChannelMask;
    unsigned short blueMask;
    unsigned short alphaChannelMask;
    short textureX;
    unsigned short minDistance;
    unsigned short colourDistance;
    short outputLine;
    DDPIXELFORMAT ddFormat;
    unsigned short *outputWords;
    short sampleColumn;
    unsigned char *sourceIndexedPixels;
    unsigned short redTotal;
    unsigned short green;
    DDSURFACEDESC targetSurface;
    IDirectDrawSurface2 *sourceSurface2;
    IDirectDrawSurface2 *target;
    unsigned char nearest;

    factor = 1;
    maskRed = 0;
    greenChannelMask = 0;
    blueMask = 0;
    alphaChannelMask = 0;
    sourceXPos = 0;
    sourceLine = 0;
    sampleCount = 0;
    sourcePaletteIndex = 0;
    minDistance = 0xffff;
    ddFormat.dwSize = sizeof(ddFormat);
    ddResult = IDirectDrawSurface_GetPixelFormat(textureRecords[page].surfaces[0], &ddFormat);
    if (texture16Bit) {
        GetPixelChannels(&rgbChannels, ddFormat);
    }
    inputDesc.dwSize = sizeof(inputDesc);
    IDirectDrawSurface_GetSurfaceDesc(textureRecords[page].surfaces[0], &inputDesc);
    targetSurface.dwSize = sizeof(targetSurface);
    IDirectDrawSurface_GetSurfaceDesc(textureRecords[page].surfaces[1], &targetSurface);
    maskRed = (1 << rgbChannels.redBits) - 1;
    greenChannelMask = (1 << rgbChannels.greenBits) - 1;
    blueMask = (1 << rgbChannels.blueBits) - 1;
    alphaChannelMask = (1 << rgbChannels.alphaBits) - 1;

    switch (textureRecords[page].memorySize) {
    case 0x4000:
        factor = 2;
        break;
    case 0x1000:
        factor = 4;
        break;
    }
    sampleCount = factor * factor;

    ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[0], &IID_IDirectDrawSurface2, (void **)&sourceSurface2);
    dxErrorLine = 0x19c5;
    LogDXError();
    ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[1], &IID_IDirectDrawSurface2, (void **)&target);
    dxErrorLine = 0x19c6;
    LogDXError();
    inputDesc.dwSize = sizeof(inputDesc);
    ddResult = IDirectDrawSurface2_Lock(sourceSurface2, NULL, &inputDesc, DDLOCK_WAIT, NULL);
    dxErrorLine = 0x19c9;
    LogDXError();
    targetSurface.dwSize = sizeof(targetSurface);
    ddResult = IDirectDrawSurface2_Lock(target, NULL, &targetSurface, DDLOCK_WAIT, NULL);
    dxErrorLine = 0x19cc;
    LogDXError();

    if (texture16Bit) {
        pixels16 = inputDesc.lpSurface;
        outputWords = targetSurface.lpSurface;
    } else {
        sourceIndexedPixels = inputDesc.lpSurface;
        dest8 = targetSurface.lpSurface;
    }
    for (outputLine = 0; outputLine < textureRecords[page].height; outputLine++) {
        for (textureX = 0; textureX < textureRecords[page].width; textureX++) {
            sourceLine = outputLine * factor;
            redTotal = 0;
            green = 0;
            blueChannel = 0;
            alpha = 0;
            for (sampleY = 0; sampleY < factor; sampleY++) {
                sourceXPos = textureX * factor;
                for (sampleColumn = 0; sampleColumn < factor; sampleColumn++) {
                    if (texture16Bit) {
                        redTotal += (pixels16[(inputDesc.lPitch >> 1) * sourceLine + sourceXPos] >> rgbChannels.redShift) & maskRed;
                        green += (pixels16[(inputDesc.lPitch >> 1) * sourceLine + sourceXPos] >> rgbChannels.greenShift) & greenChannelMask;
                        blueChannel += (pixels16[(inputDesc.lPitch >> 1) * sourceLine + sourceXPos] >> rgbChannels.blueShift) & blueMask;
                        if (ddFormat.dwRGBAlphaBitMask) {
                            alpha += (pixels16[(inputDesc.lPitch >> 1) * sourceLine + sourceXPos] >> rgbChannels.alphaShift) & alphaChannelMask;
                        }
                    } else {
                        sourcePaletteIndex = sourceIndexedPixels[sourceLine * inputDesc.lPitch + sourceXPos];
                        redTotal += savedPalette[sourcePaletteIndex].peRed;
                        green += savedPalette[sourcePaletteIndex].peGreen;
                        blueChannel += savedPalette[sourcePaletteIndex].peBlue;
                    }
                    sourceXPos++;
                }
                sourceLine++;
            }
            redTotal /= sampleCount;
            green /= sampleCount;
            blueChannel /= sampleCount;
            if (ddFormat.dwRGBAlphaBitMask) {
                alpha /= sampleCount;
            }
            if (texture16Bit) {
                outputWords[(targetSurface.lPitch >> 1) * outputLine + textureX] = redTotal << rgbChannels.redShift;
                outputWords[(targetSurface.lPitch >> 1) * outputLine + textureX] |= green << rgbChannels.greenShift;
                outputWords[(targetSurface.lPitch >> 1) * outputLine + textureX] |= blueChannel << rgbChannels.blueShift;
                if (ddFormat.dwRGBAlphaBitMask) {
                    outputWords[(targetSurface.lPitch >> 1) * outputLine + textureX] |= alpha << rgbChannels.alphaShift;
                }
            } else {
                /* colorEntry is a byte, so this search never terminates in an 8-bit format (as in the original) */
                minDistance = 0xffff;
                for (colorEntry = 0; colorEntry < 256; colorEntry++) {
                    colourDistance = abs(savedPalette[colorEntry].peBlue - blueChannel) + abs(savedPalette[colorEntry].peGreen - green)
                        + abs(savedPalette[colorEntry].peRed - redTotal);
                    if (minDistance > colourDistance) {
                        minDistance = colourDistance;
                        nearest = colorEntry;
                    }
                }
                dest8[outputLine * targetSurface.lPitch + textureX] = nearest;
            }
        }
    }
    ddResult = IDirectDrawSurface2_Unlock(sourceSurface2, NULL);
    dxErrorLine = 0x1a2e;
    LogDXError();
    ddResult = IDirectDrawSurface2_Unlock(target, NULL);
    dxErrorLine = 0x1a2f;
    LogDXError();
}
#pragma function(abs)
#endif
#endif

/* Gives the primary surface and the back buffer the default palette. */
void SetDefaultPalette(void)
{
    ddResult = IDirectDrawSurface_SetPalette(primarySurface, defaultPalette);
#ifdef NETPLAY
    dxErrorLine = 0x1a3a;
#else
    dxErrorLine = 0x16f4;
#endif
    LogDXError();
    ddResult = IDirectDrawSurface_SetPalette(backBuffer, defaultPalette);
#ifdef NETPLAY
    dxErrorLine = 0x1a3b;
#else
    dxErrorLine = 0x16f5;
#endif
    LogDXError();
}

/* Draws a bitmap at a client-coordinate position using the current mapping mode. */
void DrawWindowBitmap(HDC dc, HBITMAP image, short x, short y)
{
    HDC memoryDC;
    POINT origin;
    POINT dims;
    BITMAP bm;

    memoryDC = CreateCompatibleDC(dc);
    SelectObject(memoryDC, image);
    SetMapMode(memoryDC, GetMapMode(dc));
    GetObjectA(image, sizeof(BITMAP), &bm);
    dims.x = bm.bmWidth;
    dims.y = bm.bmHeight;
    DPtoLP(dc, &dims, 1);
    origin.x = 0;
    origin.y = 0;
    DPtoLP(memoryDC, &origin, 1);
    BitBlt(dc, x, y, dims.x, dims.y, memoryDC, origin.x, origin.y, SRCCOPY);
    DeleteDC(memoryDC);
}

/* Averages the colours with byte wraparound, then returns the first palette entry nearest to the average. */
#ifdef NETPLAY
#pragma intrinsic(abs)
unsigned char FindNearestColourIndex(const PALETTEENTRY *colours, short count)
{
    unsigned short distance = 0;
    unsigned short minDistance = 0xffff;
    unsigned char nearest = 0;
    PALETTEENTRY mean;
    unsigned short i;

    mean.peRed = 0;
    mean.peGreen = 0;
    mean.peBlue = 0;
    for (i = 0; i < count; i++) {
        mean.peRed += colours[i].peRed;
        mean.peGreen += colours[i].peGreen;
        mean.peBlue += colours[i].peBlue;
    }
    mean.peRed /= count;
    mean.peGreen /= count;
    mean.peBlue /= count;
    for (i = 0; i < 256; i++) {
        distance = abs(savedPalette[i].peBlue - mean.peBlue) + abs(savedPalette[i].peRed - mean.peRed) + abs(savedPalette[i].peGreen - mean.peGreen);
        if (minDistance > distance) {
            minDistance = distance;
            nearest = (unsigned char)i;
        }
    }
    return nearest;
}
#pragma function(abs)
#else
unsigned char FindNearestColourIndex(const PALETTEENTRY *colours, short count)
{
    const PALETTEENTRY *colour;
    PALETTEENTRY average;
    unsigned char red = 0, green = 0, blue = 0;
    unsigned char nearest = 0;
    unsigned short distance;
    unsigned short bestDistance = 0xffff;
    unsigned short i;

    for (i = 0; i < count; i++) {
        colour = &colours[i];
        red += colour->peRed;
        green += colour->peGreen;
        blue += colour->peBlue;
    }
    /* The original divides even for a zero count. */
    average.peRed = red / count;
    average.peGreen = green / count;
    average.peBlue = blue / count;
    for (i = 0; i < 256; i++) {
        distance = abs(savedPalette[i].peRed - average.peRed) + abs(savedPalette[i].peGreen - average.peGreen)
            + abs(savedPalette[i].peBlue - average.peBlue);
        if (distance < bestDistance) {
            bestDistance = distance;
            nearest = (unsigned char)i;
        }
    }
    return nearest;
}
#endif

/* Profiling is disabled in this build. */
double GetProfileTime(void)
{
    return 0.0;
}

/* Times ten magnitude evaluations; cycle profiling is disabled in this build. */
#ifdef NETPLAY
void BenchmarkMagnitude(void)
{
    char message[256];
    float x;
    float y;
    float z;
    int i;
    int magnitude;
    double start;
    double end;
    int cycles;

    x = 112.0f;
    y = -234.0f;
    z = 93.5f;
    start = GetProfileTime();
    for (i = 0; i < 10; i++)
        magnitude = GetMagnitude(x, y, z);
    end = GetProfileTime();
    cycles = (int)(end - start);
    wsprintfA(message, "Timed at %d cycles \n", cycles);
    OutputDebugStringA(message);
}
#else
void BenchmarkMagnitude(void)
{
    char message[256];
    int i;

    for (i = 0; i < 10; i++) {
        GetMagnitude(112.0f, -234.0f, 93.5f);
    }
    wsprintfA(message, "Timed at %d cycles \n", 0);
    OutputDebugStringA(message);
}
#endif

#ifdef EQUIV

#ifndef NETPLAY
/* Recreates the DirectDraw surfaces and Direct3D textures of texture pages 5 to 10. */
int CreateTexturePages(void)
{
    DDSURFACEDESC plainDesc;
    DDSURFACEDESC textureDesc;
    DDSURFACEDESC sourceDesc;
    DDCOLORKEY colourKey;
    DDSCAPS caps;
    DWORD totalMemory;
    DWORD freeMemory;

    {
        int i = 0;
        int page = 0;
        short pageTypes[8][6] = {
            {1, 0, 0, 1, 1, 1}, {0, 0, 1, 0, 0, 0}, {0, 0, 1, 0, 1, 1}, {0, 0, 0, 0, 1, 0},
            {0, 0, 0, 0, 1, 0}, {0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 0, 0},
        };

        for (page = 5; page < 11; page++) {
            if (textureRecords[page].surfaces[0]) {
                surfaceTextures0[page]->lpVtbl->Release(surfaceTextures0[page]);
                IDirectDrawSurface_Release(textureRecords[page].surfaces[0]);
            }
            if (textureRecords[page].surfaces[1]) {
                surfaceTextures1[page]->lpVtbl->Release(surfaceTextures1[page]);
                IDirectDrawSurface_Release(textureRecords[page].surfaces[1]);
            }
            if (textureRecords[page].surfaces[2]) {
                surfaceTextures2[page]->lpVtbl->Release(surfaceTextures2[page]);
                IDirectDrawSurface_Release(textureRecords[page].surfaces[2]);
            }
            {
                int track = gameData->trackNum;

                textureRecords[page].type = 3;
                if (pageTypes[track][i] != 1) {
                    textureRecords[page].type = 2;
                }
            }
            textureRecords[page].typeVariant = 0;
            i++;
        }
    }
    if (gameData->trackNum == 0) {
        textureRecords[9].typeVariant = 1;
        textureRecords[10].typeVariant = 1;
    }

    memset(&sourceDesc, 0, sizeof(sourceDesc));
    memset(&textureDesc, 0, sizeof(textureDesc));
    memset(&plainDesc, 0, sizeof(plainDesc));
    plainDesc.dwSize = sizeof(plainDesc);
    textureDesc.dwSize = sizeof(textureDesc);
    sourceDesc.dwSize = sizeof(sourceDesc);
    plainDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    textureDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    plainDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    sourceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    textureDesc.dwHeight = texturePageSize;
    sourceDesc.dwHeight = texturePageSize;
    textureDesc.dwWidth = texturePageSize;
    sourceDesc.dwWidth = texturePageSize;
    textureDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
    if (nameDrawFirst == 0) {
        textureDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    }
    textureDesc.ddsCaps.dwCaps |= DDSCAPS_ALLOCONLOAD;
    plainDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    textureDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    sourceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    sourceDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    memset(&colourKey, 0, sizeof(colourKey));

    {
        int page;

        for (page = 5; page < 11; page++) {
            caps.dwCaps = DDSCAPS_TEXTURE;
            if (nameDrawFirst) {
                ddResult = IDirectDraw2_GetAvailableVidMem(lpDD2, &caps, &totalMemory, &freeMemory);
#ifdef NETPLAY
                dxErrorLine = 0x1b23;
#else
                dxErrorLine = 0x17dd;
#endif
                LogDXError();
                wsprintfA(g_cdMessage, "Memory - Total:%ld, Free:%ld\n", totalMemory, freeMemory);
                OutputDebugStringA(g_cdMessage);
            }
            if (textureRecords[page].type == 3) {
                textureDesc.ddpfPixelFormat = textureFormats[argb1555Format].pixelFormat;
                plainDesc.ddpfPixelFormat = ppmPixelFormat;
                sourceDesc.ddpfPixelFormat = textureFormats[argb1555Format].pixelFormat;
            } else {
                textureDesc.ddpfPixelFormat = textureFormats[bestTextureFormat].pixelFormat;
                plainDesc.ddpfPixelFormat = ppmPixelFormat;
                sourceDesc.ddpfPixelFormat = textureFormats[bestTextureFormat].pixelFormat;
            }
            wsprintfA(g_cdMessage, "Making texture page %d\n", page);
            OutputDebugStringA(g_cdMessage);

            plainDesc.dwHeight = texturePageSize;
            sourceDesc.dwHeight = texturePageSize;
            plainDesc.dwWidth = texturePageSize;
            sourceDesc.dwWidth = texturePageSize;
            ddResult = IDirectDraw_CreateSurface(lpDD, &sourceDesc, &textureRecords[page].surfaces[0], NULL);
#ifdef NETPLAY
            dxErrorLine = 0x1b3b;
#else
            dxErrorLine = 0x17f5;
#endif
            LogDXError();
            if (!texture16Bit) {
                ddResult = IDirectDrawSurface_SetPalette(textureRecords[page].surfaces[0], windowPalette);
#ifdef NETPLAY
                dxErrorLine = 0x1b3e;
#else
                dxErrorLine = 0x17f8;
#endif
                LogDXError();
            }

            sourceDesc.dwHeight = textureRecords[page].height;
            sourceDesc.dwWidth = textureRecords[page].width;
            textureDesc.dwHeight = textureRecords[page].height;
            textureDesc.dwWidth = textureRecords[page].width;
            ddResult = IDirectDraw_CreateSurface(lpDD, &sourceDesc, &textureRecords[page].surfaces[1], NULL);
#ifdef NETPLAY
            dxErrorLine = 0x1b45;
#else
            dxErrorLine = 0x17ff;
#endif
            LogDXError();
            if (!texture16Bit) {
                ddResult = IDirectDrawSurface_SetPalette(textureRecords[page].surfaces[1], windowPalette);
#ifdef NETPLAY
                dxErrorLine = 0x1b47;
#else
                dxErrorLine = 0x1801;
#endif
                LogDXError();
            }
            ddResult = IDirectDraw_CreateSurface(lpDD, &textureDesc, &textureRecords[page].surfaces[2], NULL);
#ifdef NETPLAY
            dxErrorLine = 0x1b49;
#else
            dxErrorLine = 0x1803;
#endif
            LogDXError();
            if (!texture16Bit) {
                ddResult = IDirectDrawSurface_SetPalette(textureRecords[page].surfaces[2], windowPalette);
#ifdef NETPLAY
                dxErrorLine = 0x1b4b;
#else
                dxErrorLine = 0x1805;
#endif
                LogDXError();
            }
            ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[0], &IID_IDirect3DTexture, (void **)&surfaceTextures0[page]);
#ifdef NETPLAY
            dxErrorLine = 0x1b4d;
#else
            dxErrorLine = 0x1807;
#endif
            LogDXError();
            ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[1], &IID_IDirect3DTexture, (void **)&surfaceTextures1[page]);
#ifdef NETPLAY
            dxErrorLine = 0x1b4e;
#else
            dxErrorLine = 0x1808;
#endif
            LogDXError();
            ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[2], &IID_IDirect3DTexture, (void **)&surfaceTextures2[page]);
#ifdef NETPLAY
            dxErrorLine = 0x1b4f;
#else
            dxErrorLine = 0x1809;
#endif
            LogDXError();
        }
    }
    return 0;
}
#endif
#endif

#ifdef NETPLAY
int CreateTexturePages(void)
{
    DDSCAPS memoryFlags;
    DDSURFACEDESC plainDesc;
    DDCOLORKEY key;
    DDSURFACEDESC sourceSurface;
    DWORD videoMemoryFree;
    int page = 0;
    int i = 0;
    /* The original initializes these bookkeeping locals but never reads them. */
    int j = 0;
    DWORD memoryNeeded = 0;
    DWORD videoMemoryTotal;
    DDSURFACEDESC textureSurfaceDesc;
    short trackTextureTypes[8][6] = {
        {1, 0, 0, 1, 1, 1}, {0, 0, 1, 0, 0, 0}, {0, 0, 1, 0, 1, 1}, {0, 0, 0, 0, 1, 0},
        {0, 0, 0, 0, 1, 0}, {0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 0, 0},
    };

    for (i = 5; i < 11; i++) {
        if (textureRecords[i].surfaces[0]) {
            surfaceTextures0[i]->lpVtbl->Release(surfaceTextures0[i]);
            IDirectDrawSurface_Release(textureRecords[i].surfaces[0]);
        }
        if (textureRecords[i].surfaces[1]) {
            surfaceTextures1[i]->lpVtbl->Release(surfaceTextures1[i]);
            IDirectDrawSurface_Release(textureRecords[i].surfaces[1]);
        }
        if (textureRecords[i].surfaces[2]) {
            surfaceTextures2[i]->lpVtbl->Release(surfaceTextures2[i]);
            IDirectDrawSurface_Release(textureRecords[i].surfaces[2]);
        }
        if (trackTextureTypes[gameData->trackNum][page] == 1) {
            textureRecords[i].type = 3;
        } else {
            textureRecords[i].type = 2;
        }
        textureRecords[i].typeVariant = 0;
        page++;
    }
    if (gameData->trackNum == 0) {
        textureRecords[9].typeVariant = 1;
        textureRecords[10].typeVariant = 1;
    }

    memset(&sourceSurface, 0, sizeof(sourceSurface));
    memset(&textureSurfaceDesc, 0, sizeof(textureSurfaceDesc));
    memset(&plainDesc, 0, sizeof(plainDesc));
    sourceSurface.dwSize = textureSurfaceDesc.dwSize = plainDesc.dwSize = sizeof(plainDesc);
    sourceSurface.dwFlags = textureSurfaceDesc.dwFlags = plainDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    sourceSurface.dwHeight = textureSurfaceDesc.dwHeight = texturePageSize;
    sourceSurface.dwWidth = textureSurfaceDesc.dwWidth = texturePageSize;
    sourceSurface.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    textureSurfaceDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    plainDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    if (nameDrawFirst) {
        textureSurfaceDesc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
    } else {
        textureSurfaceDesc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    }
    sourceSurface.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    sourceSurface.ddpfPixelFormat.dwSize = textureSurfaceDesc.ddpfPixelFormat.dwSize = plainDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    textureSurfaceDesc.ddsCaps.dwCaps |= DDSCAPS_ALLOCONLOAD;
    memoryFlags.dwCaps = DDSCAPS_TEXTURE;
    memset(&key, 0, sizeof(key));

    for (page = 5; page < 11; page++) {
        memoryFlags.dwCaps = DDSCAPS_TEXTURE;
        if (nameDrawFirst) {
            ddResult = IDirectDraw2_GetAvailableVidMem(lpDD2, &memoryFlags, &videoMemoryTotal, &videoMemoryFree);
            dxErrorLine = 0x1b23;
            LogDXError();
            wsprintfA(g_cdMessage, "Memory - Total:%ld, Free:%ld\n", videoMemoryTotal, videoMemoryFree);
            OutputDebugStringA(g_cdMessage);
        }
        if (textureRecords[page].type == 3) {
            textureSurfaceDesc.ddpfPixelFormat = textureFormats[argb1555Format].pixelFormat;
            plainDesc.ddpfPixelFormat = ppmPixelFormat;
            sourceSurface.ddpfPixelFormat = textureFormats[argb1555Format].pixelFormat;
        } else {
            textureSurfaceDesc.ddpfPixelFormat = textureFormats[bestTextureFormat].pixelFormat;
            plainDesc.ddpfPixelFormat = ppmPixelFormat;
            sourceSurface.ddpfPixelFormat = textureFormats[bestTextureFormat].pixelFormat;
        }
        wsprintfA(g_cdMessage, "Making texture page %d\n", page);
        OutputDebugStringA(g_cdMessage);

        sourceSurface.dwHeight = plainDesc.dwHeight = texturePageSize;
        sourceSurface.dwWidth = plainDesc.dwWidth = texturePageSize;
        ddResult = IDirectDraw_CreateSurface(lpDD, &sourceSurface, &textureRecords[page].surfaces[0], NULL);
        dxErrorLine = 0x1b3b;
        LogDXError();
        if (!texture16Bit) {
            ddResult = IDirectDrawSurface_SetPalette(textureRecords[page].surfaces[0], windowPalette);
            dxErrorLine = 0x1b3e;
            LogDXError();
        }

        sourceSurface.dwHeight = textureRecords[page].height;
        sourceSurface.dwWidth = textureRecords[page].width;
        textureSurfaceDesc.dwHeight = textureRecords[page].height;
        textureSurfaceDesc.dwWidth = textureRecords[page].width;
        ddResult = IDirectDraw_CreateSurface(lpDD, &sourceSurface, &textureRecords[page].surfaces[1], NULL);
        dxErrorLine = 0x1b45;
        LogDXError();
        if (!texture16Bit) {
            ddResult = IDirectDrawSurface_SetPalette(textureRecords[page].surfaces[1], windowPalette);
            dxErrorLine = 0x1b47;
            LogDXError();
        }
        ddResult = IDirectDraw_CreateSurface(lpDD, &textureSurfaceDesc, &textureRecords[page].surfaces[2], NULL);
        dxErrorLine = 0x1b49;
        LogDXError();
        if (!texture16Bit) {
            ddResult = IDirectDrawSurface_SetPalette(textureRecords[page].surfaces[2], windowPalette);
            dxErrorLine = 0x1b4b;
            LogDXError();
        }
        ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[0], &IID_IDirect3DTexture, (void **)&surfaceTextures0[page]);
        dxErrorLine = 0x1b4d;
        LogDXError();
        ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[1], &IID_IDirect3DTexture, (void **)&surfaceTextures1[page]);
        dxErrorLine = 0x1b4e;
        LogDXError();
        ddResult = IDirectDrawSurface_QueryInterface(textureRecords[page].surfaces[2], &IID_IDirect3DTexture, (void **)&surfaceTextures2[page]);
        dxErrorLine = 0x1b4f;
        LogDXError();
    }
    return 0;
}
#endif

/* Clears the back buffer with the current fade color. */
void ClearBackBufferWithFade(void)
{
    RECT rect;
    DDBLTFX effects;
#ifndef NETPLAY
    int fadeColor;
#endif

    memset(&effects, 0, sizeof(effects));
    effects.dwSize = sizeof(effects);
    SetRect(&rect, 0, 0, (int)screenScaleX, (int)screenScaleY);
    if (fadeAmount > 180) {
#ifdef NETPLAY
        effects.dwFillColor = ((unsigned char)fadeAmount << 8) |
                              ((unsigned char)fadeAmount << 16) |
                              (unsigned char)fadeAmount;
#else
        fadeColor = (unsigned char)fadeAmount;
        effects.dwFillColor = fadeColor | (fadeColor << 8) | (fadeColor << 16);
#endif
    }
    ddResult = IDirectDrawSurface_Blt(backBuffer, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &effects);
#ifdef NETPLAY
    dxErrorLine = 0x1b66;
#else
    dxErrorLine = 0x1820;
#endif
    LogDXError();
}

/* Initializes the black polygon used behind the setup screen. */
void InitSetupBackground(void)
{
    SetPolyF4(&setupBackground);
    setupBackground.r0 = 0;
    setupBackground.g0 = 0;
    setupBackground.b0 = 0;
    setupBackground.x0 = 0;
    setupBackground.y0 = 0;
    setupBackground.x1 = 320;
    setupBackground.y1 = 0;
    setupBackground.x2 = 0;
    setupBackground.y2 = 240;
    setupBackground.x3 = 320;
    setupBackground.y3 = 240;
}

/* Tests whether the driver accepts the PowerVR command texture format. */
#ifdef NETPLAY
int CheckPVRTextureSupport(LPDIRECTDRAW directDraw)
{
    PvrCommandData *pixelData;
    IDirect3DTexture *pvrTexture;
    DDSURFACEDESC surfaceDesc;
    LPDIRECTDRAWSURFACE lockedSurfacePointer;
    D3DTEXTUREHANDLE surfaceHandle;

    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);
    surfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    surfaceDesc.dwHeight = 2;
    surfaceDesc.dwWidth = 2;
    surfaceDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surfaceDesc.ddpfPixelFormat.dwSize = sizeof(surfaceDesc.ddpfPixelFormat);
    surfaceDesc.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    surfaceDesc.ddpfPixelFormat.dwFourCC = mmioFOURCC('P', 'V', 'R', 'C');

    if (IDirectDraw_CreateSurface(directDraw, &surfaceDesc, &lockedSurfacePointer, NULL) != DD_OK)
        return 0;
    if (IDirectDrawSurface2_Lock((LPDIRECTDRAWSURFACE2)lockedSurfacePointer, NULL, &surfaceDesc, DDLOCK_WAIT, NULL) != DD_OK) {
        if (lockedSurfacePointer) {
            IDirectDrawSurface2_Release((LPDIRECTDRAWSURFACE2)lockedSurfacePointer);
            lockedSurfacePointer = NULL;
        }
        return 0;
    }
    pixelData = surfaceDesc.lpSurface;
    IDirectDrawSurface2_Unlock((LPDIRECTDRAWSURFACE2)lockedSurfacePointer, &surfaceDesc);
    if (IDirectDrawSurface2_QueryInterface((LPDIRECTDRAWSURFACE2)lockedSurfacePointer, &IID_IDirect3DTexture, (void **)&pvrTexture) != DD_OK) {
        if (lockedSurfacePointer) {
            IDirectDrawSurface2_Release((LPDIRECTDRAWSURFACE2)lockedSurfacePointer);
            lockedSurfacePointer = NULL;
        }
        return 0;
    }
    pvrTexture->lpVtbl->GetHandle(pvrTexture, lpD3DDevice, &surfaceHandle);
    if (pvrTexture) {
        pvrTexture->lpVtbl->Release(pvrTexture);
        pvrTexture = NULL;
    }
    if (lockedSurfacePointer) {
        IDirectDrawSurface2_Release((LPDIRECTDRAWSURFACE2)lockedSurfacePointer);
        lockedSurfacePointer = NULL;
    }
    if (pixelData->result == 0)
        return 0;
    return 1;
}
#else
int CheckPVRTextureSupport(LPDIRECTDRAW directDraw)
{
    DDSURFACEDESC surfaceDesc;
    LPDIRECTDRAWSURFACE surface;
    IDirect3DTexture *texture;
    D3DTEXTUREHANDLE textureHandle;
    PvrCommandData *surfaceData;

    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);
    surfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    surfaceDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surfaceDesc.dwHeight = 2;
    surfaceDesc.dwWidth = 2;
    surfaceDesc.ddpfPixelFormat.dwSize = sizeof(surfaceDesc.ddpfPixelFormat);
    surfaceDesc.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    surfaceDesc.ddpfPixelFormat.dwFourCC = mmioFOURCC('P', 'V', 'R', 'C');

    if (IDirectDraw_CreateSurface(directDraw, &surfaceDesc, &surface, NULL) != DD_OK) {
        return 0;
    }
    if (IDirectDrawSurface_Lock(surface, NULL, &surfaceDesc, DDLOCK_WAIT, NULL) != DD_OK) {
        if (surface) {
            IDirectDrawSurface_Release(surface);
        }
        return 0;
    }
    surfaceData = surfaceDesc.lpSurface;
    /* The original passes the descriptor instead of the locked pixel pointer. */
    IDirectDrawSurface_Unlock(surface, &surfaceDesc);
    if (IDirectDrawSurface_QueryInterface(surface, &IID_IDirect3DTexture, (void **)&texture) != DD_OK) {
        if (surface) {
            IDirectDrawSurface_Release(surface);
        }
        return 0;
    }
    texture->lpVtbl->GetHandle(texture, lpD3DDevice, &textureHandle);
    if (texture) {
        texture->lpVtbl->Release(texture);
        texture = NULL;
    }
    if (surface) {
        IDirectDrawSurface_Release(surface);
        surface = NULL;
    }
    /* The original reads the response after releasing both interface references. */
    return surfaceData->result >= 1;
}
#endif

/* Clears the ready flag in the PowerVR command record. */
int SetPvrCommandBusy(void)
{
    HRESULT lockRes;
    DDSURFACEDESC surfaceDesc;

    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);
    surfaceDesc.dwFlags = 0;
    lockRes = IDirectDrawSurface_Lock(pvrCommandSurface, NULL, &surfaceDesc, DDLOCK_WAIT, NULL);
    pvrCommandData->flags &= ~0x20;
    IDirectDrawSurface_Unlock(pvrCommandSurface, &surfaceDesc);
    return 0;
}

/* Sets the ready flag in the PowerVR command record. */
int SetPvrCommandReady(void)
{
    HRESULT lockRes;
    DDSURFACEDESC surfaceDesc;

    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);
    surfaceDesc.dwFlags = 0;
    lockRes = IDirectDrawSurface_Lock(pvrCommandSurface, NULL, &surfaceDesc, DDLOCK_WAIT, NULL);
    pvrCommandData->flags |= 0x20;
    IDirectDrawSurface_Unlock(pvrCommandSurface, &surfaceDesc);
    return 0;
}

/* Releases the PowerVR command texture and its surface, then clears the handle. */
int ReleasePvrCommandSurface(void)
{
    pvrCommandTexture->lpVtbl->Release(pvrCommandTexture);
    IDirectDrawSurface_Release(pvrCommandSurface);
    pvrCommandHandle = 0;
    pvrCommandTexture = NULL;
    pvrCommandSurface = NULL;
    return 1;
}

#ifdef NETPLAY
/* Changes to the DirectX working directory used by the setup tool. */
short SetDirectXDirectory(void)
{
    char path[256];

    strcpy(path, wobbleDir1);
    strcat(path, "directx");
    if (_chdir(path))
        return 0;
    return 1;
}
#endif

int controlKeyDown;

/* Blocks Alt+Tab, Ctrl+Esc, Alt+Esc and Alt+F4 while the game window is active. */
LRESULT CALLBACK KeyboardHookProc(int code, WORD wParam, LPARAM lParam)
{
    if (wParam == VK_CONTROL) {
        if (HIWORD(lParam) & KF_UP)
            controlKeyDown = 0;
        else
            controlKeyDown = 1;
    }
    if (wParam == VK_TAB && (HIWORD(lParam) & KF_ALTDOWN)) {
        return 1;
    } else if (wParam == VK_ESCAPE && (controlKeyDown || (HIWORD(lParam) & KF_ALTDOWN))) {
        return 1;
    } else if (wParam == VK_F4 && (HIWORD(lParam) & KF_ALTDOWN)) {
        return 1;
    }
    return CallNextHookEx(windowHook, code, wParam, lParam);
}

/* Reports whether a Pentium-class processor advertises MMX through CPUID. */
int HasMMX(void)
{
    SYSTEM_INFO systemInfo;
    int hasMMX = 0;

    GetSystemInfo(&systemInfo);
    if (systemInfo.dwProcessorType == PROCESSOR_INTEL_PENTIUM) {
        /* The original reads CPUID with inline assembly; leaf 1 returns the feature flags in edx, bit 23 is MMX. */
        __asm {
            mov eax, 1
            _emit 0x0f
            _emit 0xa2
            test edx, 0800000h
            jne found
            jmp done
        found:
            mov hasMMX, 1
        done:
        }
    } else {
        return 0;
    }
    return hasMMX;
}

#include "pc_sfxds_types.h"
#include "movie_sound.h"
extern LPDIRECTDRAWSURFACE mpegOffscreenSurface;
extern int mpegPlaybackState;
extern LPDIRECTSOUNDBUFFER mpegIntroAudioBuffer;
extern HRESULT mpegIntroAudioLockResult;
extern int mpegPlayStream(LPDIRECTDRAWSURFACE surface, LPDIRECTSOUNDBUFFER soundBuffer,
                          const char *path, void *memory);

/* Plays a movie using a temporary surface and optional playback buffer. */
#pragma function(memset)
#pragma optimize("", off)
int PlayMpegMovie(HWND window, char *name, unsigned char *buffer)
{
    /* Keep the playback state together in the original frame order. */
    struct {
        DDBLTFX effects;
        DDSURFACEDESC movieDesc;
        DDSURFACEDESC primaryDesc;
        int allocated;
        void *audioPart2;
        void *audioPart1;
        DWORD audioPart2Length;
        RECT rect;
        DWORD audioPart1Length;
        int result;
    } movie;

    movie.allocated = 0;
    movie.primaryDesc.dwSize = sizeof(movie.primaryDesc);
    IDirectDrawSurface_GetSurfaceDesc(primarySurface, &movie.primaryDesc);
    movie.movieDesc.dwSize = sizeof(movie.movieDesc);
    movie.movieDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    movie.movieDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    movie.movieDesc.dwHeight = movie.primaryDesc.dwHeight;
    movie.movieDesc.dwWidth = movie.primaryDesc.dwWidth;
    memmove(&movie.movieDesc.ddpfPixelFormat, &movie.primaryDesc.ddpfPixelFormat, sizeof(DDPIXELFORMAT));
    IDirectDraw_CreateSurface(lpDD, &movie.movieDesc, &mpegOffscreenSurface, NULL);
    memset(&movie.effects, 0, sizeof(movie.effects));
    movie.effects.dwSize = sizeof(movie.effects);
    SetRect(&movie.rect, 0, 0, movie.primaryDesc.dwWidth, movie.primaryDesc.dwHeight);
    movie.effects.dwFillColor = 0;
    IDirectDrawSurface_Blt(mpegOffscreenSurface, &movie.rect, NULL, NULL,
                           DDBLT_COLORFILL | DDBLT_WAIT, &movie.effects);
    mpegPlaybackState = 0;
    mpegIntroAudioBuffer = NULL;
    if (introSoundBuffer) {
        mpegIntroAudioLockResult = IDirectSoundBuffer_Lock(introSoundBuffer, 0, 0x10000,
                                                            &movie.audioPart1, &movie.audioPart1Length,
                                                            &movie.audioPart2, &movie.audioPart2Length, 0);
        memset(movie.audioPart1, 0, movie.audioPart1Length);
        memset(movie.audioPart2, 0, movie.audioPart2Length);
        mpegIntroAudioLockResult = IDirectSoundBuffer_Unlock(introSoundBuffer, movie.audioPart1,
                                                              movie.audioPart1Length, movie.audioPart2,
                                                              movie.audioPart2Length);
    }
    mpegIntroAudioBuffer = introSoundBuffer;
    if (buffer == NULL) {
        buffer = malloc(0x410900);
        movie.allocated = 1;
    }
    if (buffer == NULL) {
        return 1;
    }
    movie.result = mpegPlayStream(primarySurface, mpegIntroAudioBuffer, name, buffer);
    if (movie.allocated) {
        free(buffer);
    }
    IDirectDrawSurface_Release(mpegOffscreenSurface);
    return movie.result;
}
#pragma optimize("", on)
#pragma intrinsic(memset)
