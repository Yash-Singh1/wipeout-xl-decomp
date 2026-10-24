#ifndef PC_WINDOW_H
#define PC_WINDOW_H

#include <windows.h>
#include <ddraw.h>
#include "d3d.h"
#include "config.h"
#include "pc_config.h"
#include "poly_f4.h"
#include "cd.h"

#include "pc_display.h"

#ifndef DDSCAPS_3DDEVICE
#define DDSCAPS_3DDEVICE 0x00002000l
#endif

#ifndef DDSCAPS_ALLOCONLOAD
#define DDSCAPS_ALLOCONLOAD 0x04000000l
#endif

extern HHOOK windowHook;
extern int controlKeyDown;
LRESULT CALLBACK KeyboardHookProc(int code, WORD wParam, LPARAM lParam);
int HasMMX(void);
#ifdef NETPLAY
int ClearResultAndReturnOne(int *handled, LRESULT *result, HWND hwnd, UINT msg,
                            WPARAM wParam, LPARAM lParam);
#endif

/* Shift and width of each colour channel, left to right then down: shifts up, widths, 8 minus widths. */
typedef struct PixelFormatShifts {
    int redUp, greenUp, blueUp, alphaUp;
    int redBits, greenBits, blueBits, alphaBits;
    int redDown, greenDown, blueDown, alphaDown;
} PixelFormatShifts;

extern short displayDriverCount;
extern short videoModeIndex;
extern int pixelFormatGroup;
extern int pixelFormatIndex;
extern int formatOverride;
extern int displayBits;
extern int screenshotNumber;
extern int resolutionSelection;
extern int setupWindowActive;
extern int gameWindowActive;
extern int startGameRequested;
extern HBRUSH windowBrush;
extern HBITMAP setupLogo;
extern HBITMAP setupBanner;
extern HWND setupWindow;
extern HWND setupCombos[3];
extern HWND modeComboBox;
extern HWND formatComboBox;
extern HWND driverNoteWnd;
extern HWND g_hwnd;
extern HINSTANCE g_hInstance;
extern int g_cdValidating;
extern LPDIRECTDRAW lpDD;
extern LPDIRECTDRAW2 lpDD2;
extern LPDIRECTDRAWSURFACE primarySurface;
extern LPDIRECTDRAWSURFACE backBuffer;
extern LPDIRECTDRAWPALETTE defaultPalette;
extern LPDIRECTDRAWPALETTE windowPalette;
extern LPDIRECTDRAWSURFACE zBufferSurface;
extern PALETTEENTRY systemPalette[256];
extern DWORD activeDeviceCaps[43];
extern float viewScaleX, viewScaleY;
extern int fullscreenDisplay;
extern int usingSystemPalette;
/* Non-zero when the CPU has MMX (HasMMX, set at display setup): selects the MMX software blit path. */
extern int mmxAvailable;
extern unsigned long displayDeviceFlags;
/* Texture page used by HUD and menu quads (see hud.h). */
extern int hudTexturePage;
extern HRESULT ddResult;
extern int dxErrorLine;
struct HeapBlock;
extern struct HeapBlock *heap;
#include "heap_state.h"
#include "sfx_started.h"

extern ConfigData *gameData;
extern ConfigData initialGameData;
extern short g_cdTrackError;
extern int cdMusicTrack;
extern short cdTrackChanged;
extern char menuTextureB[];
extern char menuTextureA[];
extern int setupQuitRequested;
extern int setupTimeout;
extern short g_keyboardCode;
extern int acceleratorAvailable;
#include "cd_track_buffer.h"
extern POLY_F4 setupBackground;
extern unsigned long setupLoopTick;
extern int framesThisSecond;
extern long lastFpsSecond;
extern MSG windowMessage;
typedef struct DisplayResolution {
    unsigned short width, height;
} DisplayResolution;
extern DisplayResolution displayResolutions[];
extern char *displayResolutionLabels[];
extern int screenWidth, screenHeight;
extern float screenScaleX, screenScaleY;
extern int doubleSourceRects;
extern char wobbleDir4[];
extern char wobbleDir0[];
extern char wobbleDir1[];
#include "scale_tables.h"
extern SIZE renderSize;
extern HWND setupButtons[4], setupLabels[6];
extern int hardwareVerified;
extern char g_cdMessage[];

int InitCD(CdlLOC *locs, int unused);
void CdValidate(void);
void SetPolyF4(POLY_F4 *p);

void SetMenuCDTrack(void *, void *, ConfigData *);
void UpdateSetupDisplayOptions(void);
int CdIsDoorOpen(void);
int LoadInstallPaths(void);
void SetAssetDirectories(void);
void EnumerateDisplayDrivers(void);
void CreateDirectDrawObjects(void);
int CreateDirect3DDevice(void);
int InitDirectInput(void);
void PlayMovie(char *name);
int __stdcall EnumTextureFormatCallback(LPDDSURFACEDESC surface, LPVOID context);

/* One enumerated texture format, 0x54 bytes. kind is 4, 8 or 16 bits per pixel. */
typedef struct TextureFormat {
    DDPIXELFORMAT pixelFormat;
    short kind;
    short reserved;
    PixelFormatShifts shifts;
} TextureFormat;
extern TextureFormat textureFormats[];
extern short textureFormatCount, bestTextureFormat, argb1555Format;
extern int texturePageSize;
extern int bestRedBits, bestGreenBits, bestBlueBits;
extern int bestRedUp, bestGreenUp, bestBlueUp;
extern int bestRedDown, bestGreenDown, bestBlueDown;
extern int executeBuffersCreated;

/* Viewport record set on the render viewport (renderViewportData). */
typedef struct D3DVIEWPORT {
    unsigned long dwSize, dwX, dwY, dwWidth, dwHeight;
    float dvScaleX, dvScaleY, dvMaxX, dvMaxY, dvMinZ, dvMaxZ;
} D3DVIEWPORT;
extern D3DVIEWPORT renderViewportData;
extern int blendByBucket, gameFlags, typeSwitchesMode;
extern unsigned char particleAlpha, translucentAlpha, opaqueAlpha, vertexAlpha;
extern unsigned short normalSetNo;
extern unsigned long vertexBufferSize;
extern D3DEXECUTEBUFFERDESC bufferDescriptionTemplate;

struct IDirect3D;
struct IDirect3DViewport;
typedef struct IDirect3DVtbl {
    long (__stdcall *QueryInterface)(struct IDirect3D *, const void *, void **);
    unsigned long (__stdcall *AddRef)(struct IDirect3D *);
    unsigned long (__stdcall *Release)(struct IDirect3D *);
    long (__stdcall *Initialize)(struct IDirect3D *, const void *);
    long (__stdcall *EnumDevices)(struct IDirect3D *, void *, void *);
    long (__stdcall *CreateLight)(struct IDirect3D *, void **, void *);
    long (__stdcall *CreateMaterial)(struct IDirect3D *, void **, void *);
    long (__stdcall *CreateViewport)(struct IDirect3D *, struct IDirect3DViewport **, void *);
} IDirect3DVtbl;
struct IDirect3D {
    IDirect3DVtbl *lpVtbl;
};
#ifdef NETPLAY
void ClearBackBufferToBlack(int colour);
void ClearBackBufferForTextureMode(void);
void ReleaseDirectDrawObjects(void);
#else
void ClearBackBufferToBlack(void);
#endif
int PlayMpegMovie(HWND window, char *name, unsigned char *buffer);
extern short moviePlaying;
extern int joyButtonNow, joyButtonPrev;
extern unsigned char bmpBuffer[];
void InitCDAudio(ConfigData *config, unsigned char *status);
int main(void);

void TerminateSound(void);
void SetShieldHum(short enabled);
void DroidSound(short enabled);
void SetPitAmbient(short enabled);
void SetAmbientLoop3(short enabled);
void CrowdSound(short enabled);
void ScrapeSound(short enabled);
void SfxUnInitLow(void);
unsigned long CdStop(void);
void CdClose(void);
void ReleaseDirectInput(void);
void LogDXError(void);
#ifdef NETPLAY
void ReportDDError(void);
#endif

LRESULT __stdcall WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL __stdcall DisplayOptionsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL __stdcall AdvancedDisplayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ShutdownGame(void);
void ShowMessage(char *format, ...);

int ProcessSetupMessages(void);
void SetCodeSectionReadWrite(void);
void SaveScreenshot(char *directory, void *pixels, int size);
void InitDirectDrawDriver(int unused, short driverIndex);
void DestroySetupControls(void);
void DestroySetupScreen(void);
#ifdef NON_MATCHING
#include "pc_backbuffer.h"
void InitSetupBackground(void);

#endif

void ClearBackBufferWithFade(void);
void DrawWindowBitmap(HDC dc, HBITMAP image, short x, short y);

void InitSetupBackground(void);

#endif
