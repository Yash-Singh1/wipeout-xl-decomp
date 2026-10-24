#ifndef PC_TEXTURES_H
#define PC_TEXTURES_H

#include <windows.h>
#include <ddraw.h>
#include "d3d.h"
#include "ship_models.h"

/* Texture record, 0x30 bytes, one per texture page. */
typedef struct TextureRecord {
    unsigned long memorySize;
    short level;
    short width;
    short height;
    short reserved16;
#ifdef NETPLAY
    short colourBase0;
    short reservedColourBase0;
    short colourBase1;
    short reservedColourBase1;
#else
    long colourBase[2];
#endif
    unsigned char reserved1[10];
    short reserved2a;
    short type;
    short typeVariant;
    LPDIRECTDRAWSURFACE surfaces[3];
} TextureRecord;

struct IDirect3DTexture;
typedef struct IDirect3DTextureVtbl {
    D3DHRESULT (__stdcall *QueryInterface)(struct IDirect3DTexture *, const void *, void **);
    unsigned long (__stdcall *AddRef)(struct IDirect3DTexture *);
    unsigned long (__stdcall *Release)(struct IDirect3DTexture *);
    D3DHRESULT (__stdcall *Initialize)(struct IDirect3DTexture *, IDirect3DDevice *, LPDIRECTDRAWSURFACE);
    D3DHRESULT (__stdcall *GetHandle)(struct IDirect3DTexture *, IDirect3DDevice *, D3DTEXTUREHANDLE *);
    D3DHRESULT (__stdcall *PaletteChanged)(struct IDirect3DTexture *, unsigned long, unsigned long);
    D3DHRESULT (__stdcall *Load)(struct IDirect3DTexture *, struct IDirect3DTexture *);
    D3DHRESULT (__stdcall *Unload)(struct IDirect3DTexture *);
} IDirect3DTextureVtbl;
typedef struct IDirect3DTexture {
    IDirect3DTextureVtbl *lpVtbl;
} IDirect3DTexture;

extern const IID IID_IDirect3DTexture;
extern TextureRecord textureRecords[];
extern IDirect3DTexture *surfaceTextures0[], *surfaceTextures1[], *surfaceTextures2[];
struct IDirect3D;
extern struct IDirect3D *direct3D;
void ReleaseSurfaces(void);
int CheckPVRTextureSupport(LPDIRECTDRAW directDraw);
short GetTextureColour(short page, unsigned char colour, short alternate);
void ReleaseRendererObjects(void);

/* PowerVR command record at the start of pvrCommandSurface: a command word, a result word and a
   flags word. Bit 0x20 of flags is the ready bit that SetPvrCommandReady sets and
   SetPvrCommandBusy clears. */
typedef struct PvrCommandData {
    unsigned long command;
    unsigned long result;
    unsigned long flags;
} PvrCommandData;

extern IDirect3DTexture *pvrCommandTexture;
extern LPDIRECTDRAWSURFACE pvrCommandSurface;
extern D3DTEXTUREHANDLE pvrCommandHandle;
extern PvrCommandData *pvrCommandData;
#ifdef POWERVR
extern int pvrCommandEnabled;
#endif
int CreatePvrCommandSurface(LPDIRECTDRAW dd, IDirect3DDevice *device);
int SetPvrCommandBusy(void);
int SetPvrCommandReady(void);
int ReleasePvrCommandSurface(void);

#endif
