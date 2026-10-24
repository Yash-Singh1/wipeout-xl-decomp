#ifndef PC_DDRAW_H
#define PC_DDRAW_H

/* DirectDraw interfaces used by the PC renderer and movie player. */
typedef struct IDirectDrawSurface DDSurface;
typedef struct IDirectDrawPalette DDPalette;

typedef struct DDPixelFormat {
    unsigned long size;
    unsigned long flags;
    unsigned long fourCC;
    unsigned long rgbBitCount;
    unsigned long redMask;
    unsigned long greenMask;
    unsigned long blueMask;
    unsigned long alphaMask;
} DDPixelFormat;

typedef struct DDColorKeyValue {
    unsigned long lowValue;
    unsigned long highValue;
} DDColorKeyValue;

typedef struct DDSurfaceDesc {
    unsigned long size;
    unsigned long flags;
    unsigned long height;
    unsigned long width;
    long pitch;                    /* 0x10 lPitch */
    unsigned long backBufferCount;
    unsigned long mipMapCount;
    unsigned long alphaBitDepth;
    unsigned long reserved;
    void *surface;                 /* 0x24 lpSurface */
    DDColorKeyValue destinationOverlayColorKey;
    DDColorKeyValue destinationBltColorKey;
    DDColorKeyValue sourceOverlayColorKey;
    DDColorKeyValue sourceBltColorKey;
    DDPixelFormat pixelFormat;     /* 0x48 */
    unsigned long caps;            /* 0x68 */
} DDSurfaceDesc;

typedef struct DDBltFx {
    unsigned long size;
    unsigned long ddrawEffects;
    unsigned long rasterOperation;
    unsigned long ddrawRasterOperation;
    unsigned long rotationAngle;
    unsigned long zBufferOperation;
    unsigned long zBufferLow;
    unsigned long zBufferHigh;
    unsigned long zBufferBaseDestination;
    unsigned long zDestinationBitDepth;
    unsigned long zDestinationConstant;
    unsigned long zSourceBitDepth;
    unsigned long zSourceConstant;
    unsigned long alphaEdgeBitDepth;
    unsigned long alphaEdgeBlend;
    unsigned long reserved;
    unsigned long alphaDestinationBitDepth;
    unsigned long alphaDestinationConstant;
    unsigned long alphaSourceBitDepth;
    unsigned long alphaSourceConstant;
    union {
        unsigned long fillColor;
        unsigned long fillDepth;
        DDSurface *pattern;
    };
    DDColorKeyValue destinationColorKey;
    DDColorKeyValue sourceColorKey;
} DDBltFx;

typedef struct MpegRect {
    long left, top, right, bottom;
} MpegRect;

typedef struct IDirectDrawSurfaceVtbl {
    void *QueryInterface;
    void *AddRef;
    unsigned long (__stdcall *Release)(DDSurface *);
    void *AddAttachedSurface;
    void *AddOverlayDirtyRect;
    long (__stdcall *Blt)(DDSurface *, MpegRect *, DDSurface *, MpegRect *, unsigned long, DDBltFx *);
    void *BltBatch;
    void *BltFast;
    void *DeleteAttachedSurface;
    void *EnumAttachedSurfaces;
    void *EnumOverlayZOrders;
    long (__stdcall *Flip)(DDSurface *, DDSurface *, unsigned long);
    void *GetAttachedSurface;
    void *GetBltStatus;
    void *GetCaps;
    void *GetClipper;
    void *GetColorKey;
    void *GetDC;
    void *GetFlipStatus;
    void *GetOverlayPosition;
    long (__stdcall *GetPalette)(DDSurface *, DDPalette **);
    void *GetPixelFormat;
    long (__stdcall *GetSurfaceDesc)(DDSurface *, DDSurfaceDesc *);
    void *Initialize;
    void *IsLost;
    long (__stdcall *Lock)(DDSurface *, MpegRect *, DDSurfaceDesc *, unsigned long, void *);
    void *ReleaseDC;
    void *Restore;
    void *SetClipper;
    void *SetColorKey;
    void *SetOverlayPosition;
    long (__stdcall *SetPalette)(DDSurface *, DDPalette *);
    long (__stdcall *Unlock)(DDSurface *, void *);
} DDSurfaceVtbl;

struct IDirectDrawSurface {
    const DDSurfaceVtbl *lpVtbl;
};

typedef struct IDirectDrawPaletteVtbl {
    void *QueryInterface;
    void *AddRef;
    void *Release;
    void *GetCaps;
    void *GetEntries;
    void *Initialize;
    long (__stdcall *SetEntries)(DDPalette *, unsigned long, unsigned long, unsigned long, void *);
} DDPaletteVtbl;

struct IDirectDrawPalette {
    const DDPaletteVtbl *lpVtbl;
};

extern DDSurface *primarySurface;
extern DDSurface *backBuffer;
extern DDSurface *copyrightSurface;
extern DDSurface *textureSurfaces[];
extern DDSurface *pressSurface;
extern DDPalette *gamePalette;

DDSurface *LoadPPMSurface(char *name, int index);
DDSurface *LoadBMPSurface(char *name, int index, int usePalette);

#endif
