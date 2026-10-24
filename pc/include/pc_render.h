#ifndef PC_RENDER_H
#define PC_RENDER_H

#include <windows.h>
#include <ddraw.h>
#include "d3d.h"
#include "render_execute.h"
#include "pixel_shift.h"
#include "pc_palette.h"

/* Rectangle for the viewport Clear call: left, top, right and bottom. */
typedef struct D3DRECT {
    long x1, y1, x2, y2;
} D3DRECT;

typedef struct IDirect3DViewportVtbl {
    D3DHRESULT (__stdcall *QueryInterface)(struct IDirect3DViewport *, const void *, void **);
    unsigned long (__stdcall *AddRef)(struct IDirect3DViewport *);
    unsigned long (__stdcall *Release)(struct IDirect3DViewport *);
    D3DHRESULT (__stdcall *Initialize)(struct IDirect3DViewport *, void *);
    D3DHRESULT (__stdcall *GetViewport)(struct IDirect3DViewport *, void *);
    D3DHRESULT (__stdcall *SetViewport)(struct IDirect3DViewport *, void *);
    D3DHRESULT (__stdcall *TransformVertices)(struct IDirect3DViewport *, unsigned long, void *, unsigned long, unsigned long *);
    D3DHRESULT (__stdcall *LightElements)(struct IDirect3DViewport *, unsigned long, void *);
    D3DHRESULT (__stdcall *SetBackground)(struct IDirect3DViewport *, unsigned long);
    D3DHRESULT (__stdcall *GetBackground)(struct IDirect3DViewport *, unsigned long *, int *);
    D3DHRESULT (__stdcall *SetBackgroundDepth)(struct IDirect3DViewport *, void *);
    D3DHRESULT (__stdcall *GetBackgroundDepth)(struct IDirect3DViewport *, void **, int *);
    D3DHRESULT (__stdcall *Clear)(struct IDirect3DViewport *, unsigned long, D3DRECT *, unsigned long);
} IDirect3DViewportVtbl;
typedef struct IDirect3DViewport {
    IDirect3DViewportVtbl *lpVtbl;
} IDirect3DViewport;

extern unsigned long presentStartTime;
extern double renderProfileTimer, renderProfileTimer2;
extern double flipProfileTime, fillProfileTime, renderLoopProfileTime, renderLoopProfileTime2;
extern int rgbTextures, paletteDirty, fadeAmount;
extern int texture16Bit;
extern short savedPaletteInUse;
#include "palette_state.h"
extern DDPIXELFORMAT ppmPixelFormat;
extern int pixelRedBits, pixelGreenBits, pixelBlueBits;
extern IDirect3DViewport *renderViewport;
extern IDirect3DDevice *lpD3DDevice;
extern int displayStat0, displayStat1;
extern int fpsSampleCount, fpsSampleTotal;
extern short raceFinished;
#include "directinput_state.h"
void ReleaseRenderMatrices(void);

extern short renderQueueCounter0, renderQueueCounter1, renderQueueCounter2, renderQueueCounter3;
extern double queueProfileTime0, queueProfileTime1, queueProfileTime2, queueProfileTime3;
extern int nameDrawFirst;
#ifdef POWERVR
#include "poly_f4.h"
extern int lastSpecialRenderBuffer;
void DrawPolyF4(float depth, POLY_F4 *poly);
extern double executeProfileTime0, executeProfileTime1, executeProfileTime2, executeProfileTime3, executeProfileTime4;
extern double releaseMatricesProfileTime, sceneEndProfileTime;
#endif
void FlushBlitQueue(int queue);
void DrawRenderQueue(void);
void SubmitRenderTriangles(void);
int PresentRenderFrame(void);
void FlushRenderQueues(void);
void FlushGameRenderQueues(void);

double GetProfileTime(void);

#endif
