#include "mpeg.h"
#include "mpegbits.h"
#include "pc.h"
#include <stdio.h>
#include <string.h>

/* Screen-centre offsets, shared with the libgte transform helpers that read them. */
extern long xOffset;
extern long yOffset;

/* Win32 message, key and blit constants used by the movie player. */
#define WM_KEYDOWN 0x0100
#define VK_RETURN 0x0D
#define VK_SPACE 0x20
#define VK_ESCAPE 0x1B
#define DDBLT_COLORFILL 0x00000400
#define DDBLT_WAIT 0x01000000
#define TIME_PERIODIC 1
#define KEY_PREVIOUSLY_DOWN 0x40000000

typedef struct MpegMsg {
    void *hwnd;
    unsigned int message;
    unsigned int wParam;
    long lParam;
    unsigned long time;
    long x;
    long y;
} MpegMsg;

__declspec(dllimport) int __stdcall PeekMessageA(MpegMsg *msg, void *hwnd, unsigned min, unsigned max, unsigned remove);
__declspec(dllimport) int __stdcall GetMessageA(MpegMsg *msg, void *hwnd, unsigned min, unsigned max);
__declspec(dllimport) int __stdcall TranslateMessage(const MpegMsg *msg);
__declspec(dllimport) long __stdcall DispatchMessageA(const MpegMsg *msg);
__declspec(dllimport) int __stdcall SetRect(MpegRect *rc, int left, int top, int right, int bottom);
__declspec(dllimport) unsigned __stdcall timeKillEvent(unsigned id);
__declspec(dllimport) unsigned long __stdcall GetTickCount(void);
int PollJoystick(void);

typedef void (__stdcall *TimerCallback)(unsigned, unsigned, unsigned long, unsigned long, unsigned long);
__declspec(dllimport) unsigned __stdcall timeSetEvent(unsigned delay, unsigned resolution, TimerCallback proc, unsigned long user, unsigned flags);

/* Stops the movie when the operator presses escape, space or enter, or a joystick button.
   The flag store lives in mpegAbortPlayback, which the player loop reads, so this is a thin
   wrapper over it. */
void mpegStopPlayback(void)
{
    mpegAbortPlayback();
}

/* Starts one stream: takes the target surface and the sound buffer, clears the player state,
   carves the picture buffers out of mem and opens path. Returns 1 on failure. */
int mpegPlayStream(DDSurface *surface, DSBuffer *soundBuf, const char *path, void *mem)
{
    void *heapStart;
    int i;

    mpegSurface = surface;
    mpegSoundBuf = soundBuf;
    if (mpegSurface == 0)
        return 1;
    memset(mem, 0, 0x2dc6c0);
    mpegHeapUsed = 0;
    mpegStreamResetValue = 0;
    mpegVideo = 0;
    mpegPicWidth = 0;
    mpegPicHeight = 0;
    mpegLastFrame = 0;
    mpegFirstFrame = mpegLastFrame;
    mpegPaced = 0x28;
    mpegStreamResetFlag = 0;
    heapStart = (void *)(((unsigned long)mem + 15) & ~15UL);
    mpegVideo = mpegOpenVideoStream(path, heapStart);
    if (mpegVideo == 0)
        return 1;
    mpegInitializeVlcTables(mpegVideo);
    mpegBuildClipScaleTables(mpegVideo);
    mpegBuildDecodeJumpTable(mpegVideo, (unsigned long)mpegScaleTable);
    mpegInitVideoStream(mpegVideo);
    mpegInitOutputFormat();
    mpegPicWidth = mpegVideo->mbWidth << 4;
    mpegPicHeight = mpegVideo->mbHeight << 4;
    mpegPicPixels = mpegPicWidth * mpegPicHeight;
    for (i = 0; i < 24; i++) {
        mpegVideo->pics[i].y = mpegHeapAlloc(&mpegVideo->heap, mpegPicPixels * 3 / 2);
        mpegVideo->pics[i].cr = mpegVideo->pics[i].y + mpegPicPixels;
        mpegVideo->pics[i].cb = mpegVideo->pics[i].cr + mpegPicPixels / 4;
    }
    mpegDecodeStream(mpegVideo);
    mpegCloseVideoStream(mpegVideo);
    mpegRestoreCodedBlockPatternCodes();
    if (mpegHeapPeak < mpegHeapUsed)
        mpegHeapPeak = mpegHeapUsed;
    return 0;
}

/* Reads the target surface's pixel format and sets mpegDrawMode to the copy path for it. */
void mpegInitOutputFormat(void)
{
    DDSurfaceDesc desc;

    desc.size = sizeof(DDSurfaceDesc);
    mpegPaletteResult = mpegSurface->lpVtbl->GetSurfaceDesc(mpegSurface, &desc);
    if (desc.pixelFormat.rgbBitCount == 0x10) {
        if (desc.width == 0x140)
            mpegDrawMode = 2;
        else
            mpegDrawMode = 3;
    }
    if (desc.pixelFormat.rgbBitCount == 0x18)
        mpegDrawMode = 4;
    if (desc.pixelFormat.rgbBitCount == 0x20)
        mpegDrawMode = 5;
    if (desc.pixelFormat.rgbBitCount == 8) {
        mpegPaletteResult = mpegSurface->lpVtbl->GetPalette(mpegSurface, &mpegPalette);
        mpegBuildDisplayPalette(mpegPalette);
        if (desc.width < 0x280)
            mpegDrawMode = 0;
        else
            mpegDrawMode = 6;
    }
    mpegInitColourTables(&desc.pixelFormat);
}

/* Copies one decoded frame into the target surface, centred or doubled according to
   mpegDrawMode. In mode 0 it then blits the offscreen surface to the back buffer and flips.
   The frame's inUse flag is cleared at the end.

   The local names are chosen so MSVC's /Od frame hash puts each one on the slot the
   exe uses; changing any of them moves the frame. The trailing `return` is real: the
   original jumps to the shared epilogue instead of falling into it. */
void mpegDrawFrame(Picture *frame, unsigned int n)
{
    unsigned int surfHeight;
    unsigned int destW;
    MpegRect dstBox;
    int srcHeight;
    int picW;
    unsigned long blitTicks;
    DDSurfaceDesc sd;
    DDBltFx bltStruct;
    char timingText[0x50];

    /* The original clears the frame number argument here and never reads it again. */
    n = 0;
    if (mpegDrawMode != 0) {
        sd.size = sizeof(DDSurfaceDesc);
        mpegLockSurface(mpegSurface, &sd);
    } else {
        mpegSurface = mpegOffscreenSurface;
        sd.size = sizeof(DDSurfaceDesc);
        mpegLockSurface(mpegSurface, &sd);
    }
    picW = mpegVideo->mbWidth << 4;
    srcHeight = mpegVideo->mbHeight << 4;
    surfHeight = sd.height;
    destW = sd.width;
    mpegDestPitch = sd.pitch;
    mpegDestBase = (unsigned long)sd.surface;
    mpegPlaneY = frame->y;
    mpegPlaneCb = frame->cb;
    mpegPlaneCr = frame->cr;
    mpegDestX0 = (destW - 2 * picW) >> 1;
    mpegDestX1 = 2 * picW + mpegDestX0 - 1;
    mpegDestY0 = (surfHeight - 2 * srcHeight) >> 1;
    mpegDestY1 = 2 * srcHeight + mpegDestY0 - 1;
    mpegSrcWidth = picW;
    mpegSrcHalfWidth = picW / 2;
    blitTicks = GetTickCount();
    switch (mpegDrawMode) {
    case 0:
        mpegDestX0 = (destW - picW) >> 1;
        mpegDestX1 = mpegDestX0 + picW - 1;
        mpegDestY0 = (surfHeight - srcHeight) >> 1;
        mpegDestY1 = mpegDestY0 + srcHeight - 1;
        mpegDestBase += mpegDestX0 + mpegDestY0 * mpegDestPitch;
        mpegDestRows = mpegDestY1 - mpegDestY0 + 1;
        mpegDestCols = mpegDestX1 - mpegDestX0 + 1;
        MpegDitherToPalette();
        break;
    case 6:
        mpegDestBase += mpegDestX0 + mpegDestY0 * mpegDestPitch;
        mpegDestRows = mpegDestY1 - mpegDestY0 + 1;
        mpegDestCols = mpegDestX1 - mpegDestX0 + 1;
        mpegDrawDithered();
        break;
    case 2:
        mpegDestX0 = (destW - picW) >> 1;
        mpegDestX1 = mpegDestX0 + picW - 1;
        mpegDestY0 = (surfHeight - srcHeight) >> 1;
        mpegDestY1 = mpegDestY0 + srcHeight - 1;
        mpegDestBase += mpegDestX0 + mpegDestY0 * mpegDestPitch;
        mpegDestRows = mpegDestY1 - mpegDestY0 + 1;
        mpegDestCols = mpegDestX1 - mpegDestX0 + 1;
        mpegDrawFrameRGB16Native();
        break;
    case 3:
        mpegDestBase += mpegDestX0 + mpegDestY0 * mpegDestPitch;
        mpegDestRows = mpegDestY1 - mpegDestY0 + 1;
        mpegDestCols = mpegDestX1 - mpegDestX0 + 1;
        mpegDrawFrameRGB16Doubled();
        break;
    case 4:
        mpegDestBase += mpegDestX0 + mpegDestY0 * mpegDestPitch;
        mpegDestRows = mpegDestY1 - mpegDestY0 + 1;
        mpegDestCols = mpegDestX1 - mpegDestX0 + 1;
        mpegDrawFrameRGB24Doubled();
        break;
    case 5:
        mpegDestBase += mpegDestX0 + mpegDestY0 * mpegDestPitch;
        mpegDestRows = mpegDestY1 - mpegDestY0 + 1;
        mpegDestCols = mpegDestX1 - mpegDestX0 + 1;
        mpegDrawFrameRGB32Doubled();
        break;
    }
    blitTicks = GetTickCount() - blitTicks;
    sprintf(timingText, "Draw time: %ld", blitTicks);
    mpegUnlockSurface(mpegSurface, &sd);
    if (mpegDrawMode == 0) {
        dstBox.top = 0;
        dstBox.bottom = sd.height;
        dstBox.left = 0;
        dstBox.right = sd.width;
        memset(&bltStruct, 0, sizeof(DDBltFx));
        bltStruct.size = sizeof(DDBltFx);
        backBuffer->lpVtbl->Blt(backBuffer, &dstBox, mpegOffscreenSurface, &dstBox, 1, &bltStruct);
        primarySurface->lpVtbl->Flip(primarySurface, 0, 1);
    }
    frame->inUse &= 0xfe;
    return;
}

/* Bump allocator for the decoder: returns `size` bytes rounded up to a 16-byte boundary
   from the heap pointer *heap, advances the pointer past them and counts the hand-out in
   mpegHeapUsed. */
void *mpegHeapAlloc(void **heap, int size)
{
    char *p = (char *)*heap;

    size = (size + 0xf) & ~0xf;
    *heap = (char *)*heap + size;
    mpegHeapUsed += size;
    return p;
}

/* Converts one YCbCr sample to RGB with 16.16 fixed-point coefficients. Each channel is
   summed in full, then divided by 65536 with signed division toward zero. Nothing is
   clamped here. The local names are short on purpose: MSVC's stack slot order for these
   three ints follows the names, and ar/ag/ab give the exe's frame layout. */
void mpegYCbCrToRGB(int y, int cr, int cb, int *r, int *g, int *b)
{
    int ar;
    int ag;
    int ab;

    ar = (y << 16) + cr * 91881;
    ag = (y << 16) - cb * 22551 - cr * 46802;
    ab = (y << 16) + cb * 116129;
    *r = ar / 65536;
    *g = ag / 65536;
    *b = ab / 65536;
}

/* Builds the YCbCr conversion tables selected by the DirectDraw pixel format.
   Local names pick the /Od slots. */
void mpegInitColourTables(DDPixelFormat *format)
{
    unsigned int a;
    int b;
    int c;
    int d;
    unsigned int e;
    unsigned char f;
    int g;
    int h;
    unsigned int i;
    unsigned int j;
    int k;
    int l;
    int m;
    int n;
    int o;
    int p;
    unsigned int q;
    unsigned int r;
    unsigned char s;
    int t;
    unsigned char u;
    int v;

    b = 0;
    if (format->flags & 0x40) {
        switch (format->rgbBitCount) {
        case 32:
            j = format->redMask;
            q = format->greenMask;
            i = format->blueMask;
        case 24:
            mpegRedContribution = mpegHeapAlloc(&mpegVideo->heap, 0x100);
            mpegGreenCombination = mpegHeapAlloc(&mpegVideo->heap, 0x10000);
            mpegBlueContribution = mpegHeapAlloc(&mpegVideo->heap, 0x100);
            mpegClampTable = mpegHeapAlloc(&mpegVideo->heap, 0x10000);

            for (g = 0; g < 256; g++) {
                o = ((g - 128) * 91881) >> 16;
                n = ((g - 128) * 116129) >> 16;
                if (o > 127) o = 127;
                if (o < -128) o = -128;
                if (n > 127) n = 127;
                if (n < -128) n = -128;
                mpegRedContribution[g] = o + 128;
                mpegBlueContribution[g] = n + 128;
                for (h = 0; h < 256; h++) {
                    m = (((h - 128) * 22551) + ((g - 128) * 46802)) >> 16;
                    if (m > 127) m = 127;
                    if (m < -128) m = -128;
                    mpegGreenCombination[(g << 8) + h] = 128 - m;
                }
            }
            for (g = 0; g < 256; g++) {
                for (h = 0; h < 256; h++) {
                    k = g + h - 128;
                    if (k < 0) k = 0;
                    if (k > 255) k = 255;
                    mpegClampTable[(g << 8) + h] = k;
                }
            }
            break;
        case 16:
            mpegRGB16Table = mpegHeapAlloc(&mpegVideo->heap, 0x20000);
            j = format->redMask;
            p = 0;
            while (!(j & 1)) {
                j >>= 1;
                p++;
            }
            d = 0;
            while (j & 1) {
                j >>= 1;
                d++;
            }
            q = format->greenMask;
            t = 0;
            while (!(q & 1)) {
                q >>= 1;
                t++;
            }
            v = 0;
            while (q & 1) {
                q >>= 1;
                v++;
            }
            i = format->blueMask;
            l = 0;
            while (!(i & 1)) {
                i >>= 1;
                l++;
            }
            c = 0;
            while (i & 1) {
                i >>= 1;
                c++;
            }
            for (s = 0; s < 32; s++) {
                for (u = 0; u < 32; u++) {
                    for (f = 0; f < 64; f++) {
                        mpegYCbCrToClampedRGB(f << 2, (s << 3) - 124, (u << 3) - 124, &r, &e, &a);
                        r >>= 8 - d;
                        e >>= 8 - v;
                        a >>= 8 - c;
                        mpegRGB16Table[(s << 5) + (f << 10) + u] = (e << t) | (r << p) | (a << l);
                    }
                }
            }
            break;
        case 8:
            break;
        }
    }
}

/* Converts luma and chroma to RGB and clamps each channel to 8 bits. The short local names
   matter: MSVC's stack slot order for the three ints follows them, and ar/ag/ab match the exe. */
void mpegYCbCrToClampedRGB(int y, int cr, int cb,
                           unsigned int *r, unsigned int *g, unsigned int *b)
{
    int ar;
    int ag;
    int ab;

    ar = (y << 16) + cr * 91881;
    ag = (y << 16) - cb * 22551 - cr * 46802;
    ab = (y << 16) + cb * 116129;

    if (ar < 0)
        ar = 0;
    if (ar > 0xff0000)
        ar = 0xff0000;
    if (ag < 0)
        ag = 0;
    if (ag > 0xff0000)
        ag = 0xff0000;
    if (ab < 0)
        ab = 0;
    if (ab > 0xff0000)
        ab = 0xff0000;

    *r = ar >> 16;
    *g = ag >> 16;
    *b = ab >> 16;
}

/* Builds the 256-entry display palette: 4 luma bands, 8 Cr bands and 8 Cb bands, indexed
   (y << 6) | (cr << 3) | cb, each converted to clamped RGB. The darkest entry swaps places
   with entry 0, which is then forced to black. Finally the palette is set on the surface
   and saved to mpegPaletteEntries. The original clears and sizes a 0x64-byte descriptor it
   never uses, and resets mpegPaletteBlackIndex to 0 afterwards. Local names pick the
   /Od slots: a=message, b=total, c=blue, d=red, i=cr, j=cb, k=green, m=y, n=index,
   o=descriptor, px=minimum. */
void mpegBuildDisplayPalette(DDPalette *palette)
{
    char a[0x100];
    short b;
    unsigned int c;
    unsigned int d;
    int e;
    int f;
    int g;
    int h;
    int i;
    int j;
    unsigned int k;
    char l[0x6c];
    int m;
    int n;
    struct {
        unsigned long size;
        char pad[0x4c];
        unsigned long fourCC;
        char tail[0x10];
    } o;
    short px;

    b = 0;
    px = 9999;
    n = 0;
    for (j = 0; j < 8; j++) {
        for (i = 0; i < 8; i++) {
            for (m = 0; m < 4; m++) {
                mpegYCbCrToClampedRGB(m * 64 + 32, i * 32 - 112, j * 32 - 112, &d, &k, &c);
                n = (m << 6) | (i << 3) | j;
                mpegDisplayPalette[n].peRed = d;
                mpegDisplayPalette[n].peGreen = k;
                mpegDisplayPalette[n].peBlue = c;
                mpegDisplayPalette[n].peFlags = 0;
            }
        }
    }
    for (n = 0; n < 256; n++) {
        b = mpegDisplayPalette[n].peBlue + mpegDisplayPalette[n].peRed +
            mpegDisplayPalette[n].peGreen;
        if (px > b) {
            px = b;
            mpegPaletteBlackIndex = n;
        }
    }
    mpegDisplayPalette[mpegPaletteBlackIndex].peRed = mpegDisplayPalette[0].peRed;
    mpegDisplayPalette[mpegPaletteBlackIndex].peGreen = mpegDisplayPalette[0].peGreen;
    mpegDisplayPalette[mpegPaletteBlackIndex].peBlue = mpegDisplayPalette[0].peBlue;
    mpegDisplayPalette[0].peRed = 0;
    mpegDisplayPalette[0].peGreen = 0;
    mpegDisplayPalette[0].peBlue = 0;
    mpegPaletteResult = palette->lpVtbl->SetEntries(palette, 0, 0, 256, mpegDisplayPalette);
    wsprintfA(a, "Palette black value = %d\n", mpegPaletteBlackIndex);
    OutputDebugStringA(a);
    memset(&o, 0, 0x64);
    o.size = 0x64;
    o.fourCC = 0;
    mpegPaletteBlackIndex = 0;
#ifdef NETPLAY
    memcpy(mpegPaletteEntries, mpegDisplayPalette, sizeof(mpegPaletteEntries));
#else
    memmove(mpegPaletteEntries, mpegDisplayPalette, sizeof(mpegPaletteEntries));
#endif
}

/* Locks the primary surface, has DirectDraw fill desc with its descriptor, then records
   the offset that centres a 640x480 frame on screen. Returns 1 when the Lock call fails
   and 0 when it succeeds. */
int mpegLockSurface(DDSurface *surf, DDSurfaceDesc *desc)
{
    char pad[256]; /* the original reserves this frame and never reads it */

    desc->size = sizeof(DDSurfaceDesc);
    mpegPaletteResult = surf->lpVtbl->Lock(surf, 0, desc, 1, 0);
    xOffset = (desc->width - 0x280) >> 1;
    yOffset = (desc->height - 0x1e0) >> 1;
    if (mpegPaletteResult)
        return 1;
    return 0;
}

/* Unlocks the surface mpegLockSurface locked. The HRESULT is kept in mpegPaletteResult,
   so the function itself just returns 0. */
int mpegUnlockSurface(DDSurface *surf, DDSurfaceDesc *desc)
{
    mpegPaletteResult = surf->lpVtbl->Unlock(surf, 0);
    return 0;
}

/* Dither full-size planar Y/Cr/Cb into the movie palette. Palette entries
   zero and 27 are exchanged by the original converter. Local names are chosen
   so /Od places each slot on the exe's frame. */
void MpegDitherToPalette(void)
{
    int ye;
    int yerror;
    unsigned int out;
    int crLevel;
    unsigned char *acc;
    int v;
    unsigned char *red;
    int cbOff;
    int bao;
    unsigned char *pOut;
    unsigned char *lum;
    int index;

    lum = mpegPlaneY;
    yerror = 0;
    ye = 0;
    cbOff = 0;
    do {
        pOut = (unsigned char *)mpegDestBase;
        acc = mpegPlaneCr;
        red = mpegPlaneCb;
        out = (unsigned int)mpegDestCols >> 1;
        do {
            bao = *lum + yerror;
            if (bao > 255)
                bao = 255;
            bao &= 0xc0;
            yerror += *lum - bao;
            yerror &= 0x7f;
            crLevel = *acc + ye;
            if (crLevel > 255)
                crLevel = 255;
            crLevel &= 0xe0;
            ye += *acc - crLevel;
            v = *red + cbOff;
            if (v > 255)
                v = 255;
            v &= 0xe0;
            cbOff += *red - v;
            index = (crLevel >> 2) | (v >> 5) | bao;
            if (index == 0)
                index = 27;
            else if (index == 27)
                index = 0;
            *pOut = (unsigned char)index;
            pOut++;
            lum++;
            bao = *lum + yerror;
            if (bao > 255)
                bao = 255;
            bao &= 0xc0;
            yerror += *lum - bao;
            yerror &= 0x7f;
            index = (crLevel >> 2) | (v >> 5) | bao;
            if (index == 0)
                index = 27;
            else if (index == 27)
                index = 0;
            *pOut = (unsigned char)index;
            pOut++;
            lum++;
            acc++;
            red++;
            out--;
        } while (out != 0);
        mpegDestBase += mpegDestPitch;
        if (mpegDestRows & 1) {
            mpegPlaneCr += mpegSrcHalfWidth;
            mpegPlaneCb += mpegSrcHalfWidth;
        }
        mpegDestRows--;
    } while (mpegDestRows != 0);
}

/* Packs two Y bits, three Cr bits and three Cb bits with error diffusion.
   Each pixel is a DWORD store two bytes after the previous one, so the next store overwrites
   the high half of the last. Only every other destination row is drawn. Same local names
   as MpegDitherToPalette so /Od builds the same frame. */
void mpegDrawDithered(void)
{
    int ye;
    int yerror;
    unsigned int out;
    int crLevel;
    unsigned char *acc;
    int v;
    unsigned char *red;
    int cbOff;
    int bao;
    unsigned char *pOut;
    unsigned char *lum;
    int index;

    lum = mpegPlaneY;
    yerror = 0;
    ye = 0;
    cbOff = 0;
    do {
        pOut = (unsigned char *)mpegDestBase;
        acc = mpegPlaneCr;
        red = mpegPlaneCb;
        out = (unsigned int)mpegDestCols >> 2;
        do {
            bao = *lum + yerror;
            if (bao > 0xff)
                bao = 0xff;
            bao &= 0xc0;
            yerror += *lum - bao;
            yerror &= 0x7f;

            crLevel = *acc + ye;
            if (crLevel > 0xff)
                crLevel = 0xff;
            crLevel &= 0xe0;
            ye += *acc - crLevel;

            v = *red + cbOff;
            if (v > 0xff)
                v = 0xff;
            v &= 0xe0;
            cbOff += *red - v;

            index = (crLevel >> 2) | (v >> 5) | bao;
            *(unsigned long *)pOut = index;
            pOut += 2;
            lum++;

            bao = *lum + yerror;
            if (bao > 0xff)
                bao = 0xff;
            bao &= 0xc0;
            yerror += *lum - bao;
            yerror &= 0x7f;

            index = (crLevel >> 2) | (v >> 5) | bao;
            *(unsigned long *)pOut = index;
            pOut += 2;
            lum++;
            acc++;
            red++;
            out--;
        } while (out != 0);
        mpegDestBase += mpegDestPitch * 2;
        if (mpegDestRows & 2) {
            mpegPlaneCr += mpegSrcHalfWidth;
            mpegPlaneCb += mpegSrcHalfWidth;
        }
        mpegDestRows -= 2;
    } while (mpegDestRows != 0);
}

/* Asks the movie player to stop; mpegDecodeStream watches mpegAbort in its loop. */
void mpegAbortPlayback(void)
{
    mpegAbort = 1;
}

/* Reads the sequence header and derives the coded macroblock dimensions and strides. */
void mpegInitVideoStream(VidStream *vs)
{
    int nextStartCode;
    int code;

    if (vs->bufEnd - vs->bufStart < 8)
        mpegRefillVideoStreamBuffer(vs);
    while (vs->bitOffset & 7) {
        GET_BIT1(vs, code);
        if (code != 0)
            mpegDebugMessage("Expected zero bit");
    }
    for (;;) {
        if (vs->bufEnd - vs->bufStart < 8)
            mpegRefillVideoStreamBuffer(vs);
        NEED_BITS_LOOP(vs, 24);
        PEEK_BITS(vs, 24, code);
        if (code == 1)
            break;
        GET_BITS3(vs, 8, code);
        if (code != 0)
            mpegDebugMessage("Expected zero byte");
    }
    NEED_BITS_LOOP(vs, 24);
    TAKE_BITS(vs, 24, code);
    mpegReadSequenceHeader(vs);
    if (vs->bufEnd - vs->bufStart < 8)
        mpegRefillVideoStreamBuffer(vs);
    NEED_BITS3(vs, 8);
    PEEK_BITS(vs, 8, nextStartCode);
    if (nextStartCode != MPEG_CODE_EXTENSION)
        vs->isMpeg1 = 1;
    else
        vs->isMpeg1 = 0;
    vs->mbWidth = (vs->hSize + 15) >> 4;
    vs->mbHeight = (vs->vSize + 15) >> 4;
    vs->futurePic = 0;
    vs->pastPic = vs->futurePic;
    vs->yStride = (vs->mbWidth << 4) - 8;
    vs->cbStride = (vs->mbWidth << 3) - 8;
    vs->crStride = (vs->mbWidth << 3) - 8;
    vs->yBase = vs->mbWidth * 240 + 16;
    vs->cbBase = vs->mbWidth * 56 + 8;
    vs->crBase = vs->mbWidth * 56 + 8;
    vs->macroblockStride = vs->mbWidth << 7;
}

/* Plays the stream: pumps messages, decodes pictures until the end code, then blanks the screen. */
void mpegDecodeStream(VidStream *vs)
{
    unsigned int prev; /* next start code */
    int word; /* loop flag, set when the player should stop */
    MpegMsg skip; /* window message */
    DDSurfaceDesc qq; /* primary surface description */
    DDBltFx b0; /* colour fill descriptor */
    MpegRect coef; /* screen rectangle */
    unsigned char c0[0x400]; /* all-black palette entries */

    mpegDropCount = 0;
    mpegTicks = 0;
    mpegFrameCount = 0;
    mpegAudioPos = mpegFrameCount;
    word = 0;
    mpegStartAudioQueue();
    mpegAbort = 0;
    do {
        mpegReadExtensionsAndUserData(vs, 0);
        if (vs->bufEnd - vs->bufStart < 8)
            mpegRefillVideoStreamBuffer(vs);
        NEED_BITS3(vs, 8);
        PEEK_BITS(vs, 8, prev);
        do {
            if (PeekMessageA(&skip, 0, 0, 0, 0)) {
                if (!GetMessageA(&skip, 0, 0, 0))
                    word = 1;
                if (skip.message == WM_KEYDOWN && !(skip.lParam & KEY_PREVIOUSLY_DOWN)) {
                    if (skip.wParam == VK_ESCAPE || skip.wParam == VK_SPACE || skip.wParam == VK_RETURN)
                        mpegStopPlayback();
                }
                TranslateMessage(&skip);
                DispatchMessageA(&skip);
            } else {
                if (mpegAbort) {
                    word = 1;
                    break;
                }
                if (prev == MPEG_CODE_GROUP_START) {
                    mpegReadGroupHeader(vs);
                    mpegReadExtensionsAndUserData(vs, 1);
                }
                mpegReadPictureHeader(vs);
                mpegReadExtensionsAndUserData(vs, 2);
                mpegDecodePicture(vs);
                if (vs->bufEnd - vs->bufStart < 8)
                    mpegRefillVideoStreamBuffer(vs);
                NEED_BITS3(vs, 8);
                PEEK_BITS(vs, 8, prev);
            }
            if (PollJoystick()) {
                mpegStopPlayback();
                word = 1;
                break;
            }
        } while (prev == MPEG_CODE_PICTURE_START || prev == MPEG_CODE_GROUP_START);
        if (word)
            break;
        if (prev != MPEG_CODE_SEQUENCE_END) {
            mpegReadSequenceHeader(vs);
            if (vs->bufEnd - vs->bufStart < 8)
                mpegRefillVideoStreamBuffer(vs);
            NEED_BITS3(vs, 8);
            PEEK_BITS(vs, 8, prev);
        }
    } while (prev != MPEG_CODE_SEQUENCE_END && !word);
    qq.size = sizeof(DDSurfaceDesc);
    primarySurface->lpVtbl->GetSurfaceDesc(primarySurface, &qq);
    if (qq.pixelFormat.rgbBitCount == 8) {
        memset(c0, 0, sizeof(c0));
        mpegPaletteResult = mpegPalette->lpVtbl->SetEntries(mpegPalette, 0, 0, 0x100, c0);
        memset(&b0, 0, sizeof(DDBltFx));
        b0.size = sizeof(DDBltFx);
        b0.fillColor = 0;
        SetRect(&coef, 0, 0, qq.width, qq.height);
        primarySurface->lpVtbl->Blt(primarySurface, &coef, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &b0);
    }
    if (!word) {
        while (vs->ringHead != vs->ringTail || vs->ring[vs->ringHead].count != 0)
            mpegPresentQueuedFrame(vs);
    }
    if (mpegFrameTimer)
        timeKillEvent(mpegFrameTimer);
    mpegFrameTimer = 0;
    mpegStopAudioQueue();
}


/* Sequence header. */
void mpegReadSequenceHeader(VidStream *vs)
{
    short *q;
    int sc;
    int l;
    int m;
    int loadMatrix;

    if (vs->bufEnd - vs->bufStart < 0x100)
        mpegRefillVideoStreamBuffer(vs);
    GET_BITS3(vs, 8, sc);
    if (sc != MPEG_CODE_SEQUENCE_HEADER)
        mpegDebugMessage("Expected sequence header code");
    GET_BITS2(vs, 12, vs->hSize);
    GET_BITS2(vs, 12, vs->vSize);
    GET_BITS3(vs, 4, vs->aspectRatio);
    GET_BITS3(vs, 4, vs->pictureRate);
    GET_BITS_LOOP(vs, 18, vs->bitRate);
    SKIP_BIT1(vs);
    GET_BITS2(vs, 10, vs->vbvBufferSize);
    GET_BIT1(vs, vs->constrainedParams);
    GET_BIT1(vs, sc);
    if (sc) {
        q = vs->tempQuant;
        for (l = 0; l < 64; l++) {
            GET_BITS3(vs, 8, vs->tempQuant[l]);
        }
    } else {
        q = defaultIntraQuant;
    }
    for (l = 0; l < 32; l++) {
        for (m = 0; m < 64; m++)
            vs->intraQuant[l * 64 + m] = q[zigzag[m]] * l;
    }
    GET_BIT1(vs, sc);
    if (sc) {
        q = vs->tempQuant;
        for (l = 0; l < 64; l++) {
            GET_BITS3(vs, 8, vs->tempQuant[l]);
        }
    } else {
        q = defaultNonIntraQuant;
    }
    for (l = 0; l < 32; l++) {
        for (m = 0; m < 64; m++)
            vs->nonIntraQuant[l * 64 + m] = q[zigzag[m]] * l;
    }
    if (vs->bufEnd - vs->bufStart < 8)
        mpegRefillVideoStreamBuffer(vs);
    while (vs->bitOffset & 7) {
        GET_BIT1(vs, loadMatrix);
        if (loadMatrix)
            mpegDebugMessage("Expected zero bit");
    }
    for (;;) {
        if (vs->bufEnd - vs->bufStart < 8)
            mpegRefillVideoStreamBuffer(vs);
        NEED_BITS_LOOP(vs, 24);
        PEEK_BITS(vs, 24, loadMatrix);
        if (loadMatrix == 1)
            break;
        GET_BITS3(vs, 8, loadMatrix);
        if (loadMatrix)
            mpegDebugMessage("Expected zero byte");
    }
    NEED_BITS_LOOP(vs, 24);
    TAKE_BITS(vs, 24, loadMatrix);
}

/* Extension and user data. In mode 2 the user data carries audio: after a 0xbe 0xef signature
   come length-prefixed blocks that are handed to the sound queue. */
void mpegReadExtensionsAndUserData(VidStream *vs, int mode)
{
    int size;
    int flag;
    int cnt;
    int tmp;

    if (vs->bufEnd - vs->bufStart < 8)
        mpegRefillVideoStreamBuffer(vs);
    NEED_BITS3(vs, 8);
    PEEK_BITS(vs, 8, tmp);
    while (tmp == MPEG_CODE_EXTENSION || tmp == MPEG_CODE_USER_DATA) {
        if (mode != 1 && tmp == MPEG_CODE_EXTENSION)
            mpegDebugMessage("Can't do extensions");
        if (tmp == MPEG_CODE_USER_DATA) {
            if (vs->bufEnd - vs->bufStart < 8)
                mpegRefillVideoStreamBuffer(vs);
            if (mode == 2) {
                NEED_BITS3(vs, 8);
                SKIP_BITS(vs, 8);
                GET_BITS3(vs, 8, tmp);
                if (tmp != MPEG_AUDIO_DATA_SIGNATURE_0)
                    mpegDebugMessage("Wrong signature");
                GET_BITS3(vs, 8, tmp);
                if (tmp != MPEG_AUDIO_DATA_SIGNATURE_1)
                    mpegDebugMessage("Wrong signature");
                do {
                    if (vs->bufEnd - vs->bufStart < 0x7d8)
                        mpegRefillVideoStreamBuffer(vs);
                    GET_BITS3(vs, 8, size);
                    GET_BITS3(vs, 8, tmp);
                    size += tmp << 8;
                    cnt = vs->bitOffset >> 3;
                    switch (cnt) {
                    case 1:
                        vs->bufStart[-1] = vs->curBits;
                        break;
                    case 2:
                        vs->bufStart[-2] = vs->curBits >> 8;
                        vs->bufStart[-1] = vs->curBits;
                        break;
                    case 3:
                        vs->bufStart[-3] = vs->curBits >> 16;
                        vs->bufStart[-2] = vs->curBits >> 8;
                        vs->bufStart[-1] = vs->curBits;
                        break;
                    case 4:
                        vs->bufStart[-4] = vs->curBits >> 24;
                        vs->bufStart[-3] = vs->curBits >> 16;
                        vs->bufStart[-2] = vs->curBits >> 8;
                        vs->bufStart[-1] = vs->curBits;
                        break;
                    }
                    mpegQueueAudioData(vs->bufStart - cnt, size);
                    vs->bufStart += size - (vs->bitOffset >> 3);
                    vs->curBits = 0;
                    vs->bitOffset = 0;
                    NEED_BITS_LOOP(vs, 24);
                    PEEK_BITS(vs, 24, tmp);
                } while (tmp != 1);
            } else {
                while (tmp != 1) {
                    if (vs->bufEnd - vs->bufStart < 8)
                        mpegRefillVideoStreamBuffer(vs);
                    SKIP_BITS(vs, 8);
                    NEED_BITS_LOOP(vs, 24);
                    PEEK_BITS(vs, 24, tmp);
                }
            }
            if (vs->bufEnd - vs->bufStart < 8)
                mpegRefillVideoStreamBuffer(vs);
            while (vs->bitOffset & 7) {
                GET_BIT1(vs, flag);
                if (flag)
                    mpegDebugMessage("Expected zero bit");
            }
            for (;;) {
                if (vs->bufEnd - vs->bufStart < 8)
                    mpegRefillVideoStreamBuffer(vs);
                NEED_BITS_LOOP(vs, 24);
                PEEK_BITS(vs, 24, flag);
                if (flag == 1)
                    break;
                GET_BITS3(vs, 8, flag);
                if (flag)
                    mpegDebugMessage("Expected zero byte");
            }
            NEED_BITS_LOOP(vs, 24);
            TAKE_BITS(vs, 24, flag);
        }
        if (vs->bufEnd - vs->bufStart < 8)
            mpegRefillVideoStreamBuffer(vs);
        NEED_BITS3(vs, 8);
        PEEK_BITS(vs, 8, tmp);
    }
}

/* Group of pictures header. */
void mpegReadGroupHeader(VidStream *vs)
{
    int tmp;
    int flag;

    if (vs->bufEnd - vs->bufStart < 0x10)
        mpegRefillVideoStreamBuffer(vs);
    GET_BITS3(vs, 8, tmp);
    if (tmp != MPEG_CODE_GROUP_START)
        mpegDebugMessage("Expected group start code");
    GET_BIT1(vs, vs->gopDropFlag);
    GET_BITS3(vs, 5, vs->gopHours);
    GET_BITS3(vs, 6, vs->gopMinutes);
    SKIP_BIT1(vs);
    GET_BITS3(vs, 6, vs->gopSeconds);
    GET_BITS3(vs, 6, vs->gopPictures);
    GET_BIT1(vs, vs->gopClosed);
    GET_BIT1(vs, vs->gopBrokenLink);
    if (vs->bufEnd - vs->bufStart < 8)
        mpegRefillVideoStreamBuffer(vs);
    while (vs->bitOffset & 7) {
        GET_BIT1(vs, flag);
        if (flag)
            mpegDebugMessage("Expected zero bit");
    }
    for (;;) {
        if (vs->bufEnd - vs->bufStart < 8)
            mpegRefillVideoStreamBuffer(vs);
        NEED_BITS_LOOP(vs, 24);
        PEEK_BITS(vs, 24, flag);
        if (flag == 1)
            break;
        GET_BITS3(vs, 8, flag);
        if (flag)
            mpegDebugMessage("Expected zero byte");
    }
    NEED_BITS_LOOP(vs, 24);
    TAKE_BITS(vs, 24, flag);
}

/* Picture header. */
void mpegReadPictureHeader(VidStream *vs)
{
    int val; /* start code / extra bits */
    int nd; /* picture slot index */
    int f; /* free slot found */
    int bit; /* padding bit */

    if (vs->bufEnd - vs->bufStart < 0x40)
        mpegRefillVideoStreamBuffer(vs);
    GET_BITS3(vs, 8, val);
    if (val != MPEG_CODE_PICTURE_START)
        mpegDebugMessage("Expected picture start code");
    vs->curPic = 0;
    vs->pictureCount++;
    f = 0;
    do {
        for (nd = 0; nd < 24; nd++) {
            if (!vs->pics[nd].inUse) {
                vs->curPic = &vs->pics[nd];
                f = 1;
                break;
            }
        }
        if (vs->ringHead == vs->ringTail && vs->ring[vs->ringHead].count != 0)
            f = 0;
        if (!f) {
            if (!vs->frameTimerStarted) {
                if (mpegAudioActive) {
                    mpegStartAudioPlayback();
                    mpegAudioStarted = 1;
                } else {
                    mpegAudioStarted = 0;
                }
                mpegFrameTimer = timeSetEvent(0x28, 1, mpegFrameTimerCallback, 0, TIME_PERIODIC);
                if (mpegFrameTimer)
                    vs->frameTimerStarted = 1;
            }
            mpegPresentQueuedFrame(vs);
        }
    } while (!f);
    vs->curPic->inUse = 1;
    GET_BITS2(vs, 10, vs->curPic->temporalRef);
    GET_BITS3(vs, 3, vs->curPic->type);
    GET_BITS2(vs, 16, vs->curPic->vbvDelay);
    if (vs->curPic->type == 2 || vs->curPic->type == 3) {
        GET_BIT1(vs, vs->curPic->fullPelForward);
        GET_BITS3(vs, 3, val);
        vs->curPic->forwardRSize = val - 1;
        vs->curPic->forwardF = 1 << vs->curPic->forwardRSize;
    }
    if (vs->curPic->type == 3) {
        GET_BIT1(vs, vs->curPic->fullPelBackward);
        GET_BITS3(vs, 3, val);
        vs->curPic->backwardRSize = val - 1;
        vs->curPic->backwardF = 1 << vs->curPic->backwardRSize;
        vs->mbType = 0;
    }
    GET_BIT1(vs, val);
    while (val) {
        if (vs->bufEnd - vs->bufStart < 4)
            mpegRefillVideoStreamBuffer(vs);
        NEED_BITS3(vs, 8);
        SKIP_BITS(vs, 8);
        GET_BIT1(vs, val);
    }
    vs->curPic->lastMacroblock = -1;
    if (vs->bufEnd - vs->bufStart < 8)
        mpegRefillVideoStreamBuffer(vs);
    while (vs->bitOffset & 7) {
        GET_BIT1(vs, bit);
        if (bit)
            mpegDebugMessage("Expected zero bit");
    }
    for (;;) {
        if (vs->bufEnd - vs->bufStart < 8)
            mpegRefillVideoStreamBuffer(vs);
        NEED_BITS_LOOP(vs, 24);
        PEEK_BITS(vs, 24, bit);
        if (bit == 1)
            break;
        GET_BITS3(vs, 8, bit);
        if (bit)
            mpegDebugMessage("Expected zero byte");
    }
    NEED_BITS_LOOP(vs, 24);
    TAKE_BITS(vs, 24, bit);
}

/* Decodes the slices of the current picture, then queues the finished frame for display. */
void mpegDecodePicture(VidStream *vs)
{
    int skip;
    Picture *p;
    unsigned int code;
    int left;

    skip = 0;
    p = 0;
    vs->yOffset = (int)vs->curPic->y - vs->yBase;
    vs->cbOffset = (int)vs->curPic->cb - vs->cbBase;
    vs->crOffset = (int)vs->curPic->cr - vs->crBase;
    if (vs->pastPic)
        vs->yPastDelta = vs->curPic->y - vs->pastPic->y;
    if (vs->futurePic)
        vs->yFutureDelta = vs->curPic->y - vs->futurePic->y;
    vs->mbPixelX = (vs->mbWidth - 1) << 4;
    vs->mbPixelY = -16;
    vs->ringLimit = 0x40;
    if (vs->curPic->type == 3 && vs->frameTimerStarted) {
        if (vs->pictureCount <= mpegTicks) {
            skip = 1;
        } else {
            switch (vs->pictureCount - mpegTicks) {
            case 0:
                skip = 1;
                break;
            case 1:
            case 2:
                vs->ringLimit = 0x14;
                break;
            case 3:
                vs->ringLimit = 0x18;
                break;
            case 4:
                vs->ringLimit = 0x20;
                break;
            case 5:
                vs->ringLimit = 0x30;
                break;
            }
        }
    }
    if (!mpegPaced) {
        vs->ringLimit = 0x40;
        skip = 0;
    }
    do {
        if (skip) {
            left = 1;
            for (;;) {
                if (--left == 0) {
                    left = 0x78;
                    if (vs->bufEnd - vs->bufStart < 0x80)
                        mpegRefillVideoStreamBuffer(vs);
                }
                NEED_BITS_LOOP(vs, 24);
                PEEK_BITS(vs, 24, code);
                if (code == 1)
                    break;
                SKIP_BITS(vs, 8);
            }
            NEED_BITS_LOOP(vs, 24);
            SKIP_BITS(vs, 24);
        } else {
            mpegDecodeSlice(vs);
        }
        if (vs->bufEnd - vs->bufStart < 8)
            mpegRefillVideoStreamBuffer(vs);
        NEED_BITS3(vs, 8);
        PEEK_BITS(vs, 8, code);
    } while (code >= 1 && code <= 0xaf);
    if (vs->curPic->type == 1 || vs->curPic->type == 2) {
        if (vs->futurePic)
            vs->futurePic->inUse &= ~4;
        vs->futurePic = vs->pastPic;
        p = vs->futurePic;
        vs->pastPic = vs->curPic;
        vs->pastPic->inUse |= 4;
    } else {
        p = vs->curPic;
    }
    if (skip) {
        p->inUse = 0;
        p = 0;
    }
    vs->ring[vs->ringTail].frame = p;
    vs->ring[vs->ringTail].count = vs->pictureCount;
    vs->ringTail = (vs->ringTail + 1) % 64;
}

/* Slice header, then the macroblocks of the slice by picture type. */
void mpegDecodeSlice(VidStream *vs)
{
    unsigned int c;
    short qscale;
    int z;

    if (vs->bufEnd - vs->bufStart < 0x20)
        mpegRefillVideoStreamBuffer(vs);
    GET_BITS3(vs, 8, c);
    vs->sliceRow = c - 1;
    GET_BITS3(vs, 5, qscale);
    vs->curIntraQ = vs->intraQuant + (qscale << 6);
    vs->curNonIntraQ = vs->nonIntraQuant + (qscale << 6);
    GET_BIT1(vs, vs->sliceExtra);
    if (vs->sliceExtra) {
        GET_BIT1(vs, vs->sliceExtra2);
        NEED_BITS3(vs, 7);
        SKIP_BITS(vs, 7);
        NEED_BIT1(vs);
        PEEK_BITS(vs, 1, c);
        while (c) {
            if (vs->bufEnd - vs->bufStart < 8)
                mpegRefillVideoStreamBuffer(vs);
            SKIP_BITS(vs, 1);
            NEED_BITS3(vs, 8);
            SKIP_BITS(vs, 8);
            NEED_BIT1(vs);
            PEEK_BITS(vs, 1, c);
        }
        SKIP_BITS(vs, 1);
    }
    vs->mbIndex = (short)(vs->sliceRow * vs->mbWidth) - 1;
    vs->dcPredY = vs->dcPredCb = vs->dcPredCr = 0x80;
    switch (vs->curPic->type) {
    case 1:
        do {
            mpegDecodeIntraMacroblock(vs);
            if (vs->bufEnd - vs->bufStart < 8)
                mpegRefillVideoStreamBuffer(vs);
            NEED_BITS_LOOP(vs, 23);
            PEEK_BITS(vs, 23, c);
            mpegPresentQueuedFrame(vs);
        } while (c != 0);
        break;
    case 2:
        vs->fwdPredH = vs->fwdPredV = 0;
        do {
            mpegDecodePredictedMacroblock(vs);
            if (vs->bufEnd - vs->bufStart < 8)
                mpegRefillVideoStreamBuffer(vs);
            NEED_BITS_LOOP(vs, 23);
            PEEK_BITS(vs, 23, c);
            mpegPresentQueuedFrame(vs);
        } while (c != 0);
        break;
    case 3:
        vs->fwdPredH = vs->fwdPredV = 0;
        vs->bwdPredH = vs->bwdPredV = 0;
        vs->mbType = 0;
        do {
            mpegDecodeBidirectionalMacroblock(vs);
            if (vs->bufEnd - vs->bufStart < 8)
                mpegRefillVideoStreamBuffer(vs);
            NEED_BITS_LOOP(vs, 23);
            PEEK_BITS(vs, 23, c);
            mpegPresentQueuedFrame(vs);
        } while (c != 0);
        break;
    default:
        mpegDebugMessage("Unknown frame type");
    }
    if (vs->bufEnd - vs->bufStart < 8)
        mpegRefillVideoStreamBuffer(vs);
    while (vs->bitOffset & 7) {
        GET_BIT1(vs, z);
        if (z)
            mpegDebugMessage("Expected zero bit");
    }
    for (;;) {
        if (vs->bufEnd - vs->bufStart < 8)
            mpegRefillVideoStreamBuffer(vs);
        NEED_BITS_LOOP(vs, 24);
        PEEK_BITS(vs, 24, z);
        if (z == 1)
            break;
        GET_BITS3(vs, 8, z);
        if (z)
            mpegDebugMessage("Expected zero byte");
    }
    NEED_BITS_LOOP(vs, 24);
    TAKE_BITS(vs, 24, z);
}

/* Macroblocks of an intra picture. */
void mpegDecodeIntraMacroblock(VidStream *vs)
{
    unsigned int idx;
    short inc;
    int qscale;
    int status;

    do {
        if (vs->bufEnd - vs->bufStart < 8)
            mpegRefillVideoStreamBuffer(vs);
        NEED_BITS2_LOOP(vs, 11);
        PEEK_BITS(vs, 11, idx);
        inc = mbAddrIncTable[idx].value;
        vs->bitOffset -= mbAddrIncTable[idx].numBits;
        vs->curBits &= bitMask[vs->bitOffset];
        if (inc == 0x22) {
            vs->mbIndex += 33;
            inc = 0x23;
        }
    } while (inc == 0x23);
    if (vs->bufEnd - vs->bufStart < 0x20)
        mpegRefillVideoStreamBuffer(vs);
    vs->mbIndex += inc;
    if (vs->curPic->lastMacroblock + 1 < vs->mbIndex)
        mpegDebugMessage("Skipping in intra");
    vs->curPic->lastMacroblock = vs->mbIndex;
    vs->mbPixelX += 16;
    if ((vs->mbWidth << 4) == vs->mbPixelX) {
        vs->yOffset += vs->yBase;
        vs->cbOffset += vs->cbBase;
        vs->crOffset += vs->crBase;
        vs->mbPixelX = 0;
        vs->mbPixelY += 16;
    } else {
        vs->yOffset += 16;
        vs->cbOffset += 8;
        vs->crOffset += 8;
    }
    NEED_BITS3(vs, 2);
    PEEK_BITS(vs, 2, idx);
    switch (idx) {
    case 2:
    case 3:
        vs->mbType = 0;
        SKIP_BITS(vs, 1);
        break;
    case 1:
        vs->mbType = 1;
        SKIP_BITS(vs, 2);
        break;
    default:
        mpegDebugMessage("Invalid I frame vlc");
        break;
    }
    if (vs->mbType == 1) {
        GET_BITS3(vs, 5, qscale);
        vs->curIntraQ = vs->intraQuant + (qscale << 6);
        vs->curNonIntraQ = vs->nonIntraQuant + (qscale << 6);
    }
    if (vs->bufEnd - vs->bufStart < 0x200)
        mpegRefillVideoStreamBuffer(vs);
    mpegStream = vs;
    mpegRingLimit = vs->ringLimit;
    status = mpegDecodeIntraBlocks();
}

/* Macroblocks of a predicted picture. */
void mpegDecodePredictedMacroblock(VidStream *vs)
{
    int hv; /* loop row */
    short val; /* macroblock address increment */
    int minv; /* minimum vector range */
    int rh; /* forward vector candidate sum */
    unsigned char bv; /* block lies outside the picture */
    unsigned char k0; /* down half-pel flag */
    unsigned char s; /* right half-pel flag */
    int f; /* maximum vector range */
    short arr; /* skipped macroblock counter */
    short srcp; /* right reconstruction vector */
    int rc; /* decode status */
    int loopStart; /* vertical vector correction */
    unsigned int high; /* table index */
    int x; /* down vector */
    int hc; /* down vector wrapped */
    int ch; /* horizontal vector correction */
    short slice; /* quantiser scale */
    int sx; /* right vector */
    short mbaddr; /* down reconstruction vector */
    unsigned char *min; /* fill pointer */
    int counter; /* right vector wrapped */
    int kk; /* loop column */
    int n; /* decode status */

    do {
        if (vs->bufEnd - vs->bufStart < 8)
            mpegRefillVideoStreamBuffer(vs);
        NEED_BITS2_LOOP(vs, 11);
        PEEK_BITS(vs, 11, high);
        val = mbAddrIncTable[high].value;
        vs->bitOffset -= mbAddrIncTable[high].numBits;
        vs->curBits &= bitMask[vs->bitOffset];
        if (val == 0x22) {
            vs->mbIndex += 33;
            val = 0x23;
        }
    } while (val == 0x23);
    vs->mbIndex += val;
    if (vs->bufEnd - vs->bufStart < 0x20)
        mpegRefillVideoStreamBuffer(vs);
    if (vs->curPic->lastMacroblock + 1 < vs->mbIndex) {
        arr = vs->curPic->lastMacroblock + 1;
        vs->fwdPredH = vs->fwdPredV = 0;
        vs->dcPredY = vs->dcPredCb = vs->dcPredCr = 0x80;
        for (; vs->mbIndex != arr; arr++) {
            vs->mbPixelX += 16;
            if ((vs->mbWidth << 4) == vs->mbPixelX) {
                vs->yOffset += vs->yBase;
                vs->cbOffset += vs->cbBase;
                vs->crOffset += vs->crBase;
                vs->mbPixelX = 0;
                vs->mbPixelY += 16;
            } else {
                vs->yOffset += 16;
                vs->cbOffset += 8;
                vs->crOffset += 8;
            }
            mpegStream = vs;
            mpegRingLimit = vs->yPastDelta;
            mpegCopyMacroblock();
        }
    }
    vs->curPic->lastMacroblock = vs->mbIndex;
    vs->mbPixelX += 16;
    if ((vs->mbWidth << 4) == vs->mbPixelX) {
        vs->yOffset += vs->yBase;
        vs->cbOffset += vs->cbBase;
        vs->crOffset += vs->crBase;
        vs->mbPixelX = 0;
        vs->mbPixelY += 16;
    } else {
        vs->yOffset += 16;
        vs->cbOffset += 8;
        vs->crOffset += 8;
    }
    NEED_BITS3(vs, 6);
    PEEK_BITS(vs, 6, high);
    vs->mbType = mbTypePTable[high].value;
    vs->bitOffset -= mbTypePTable[high].numBits;
    vs->curBits &= bitMask[vs->bitOffset];
    if (vs->mbType & 1) {
        GET_BITS3(vs, 5, slice);
        vs->curIntraQ = vs->intraQuant + (slice << 6);
        vs->curNonIntraQ = vs->nonIntraQuant + (slice << 6);
    }
    if (vs->mbType & 2) {
        NEED_BITS2_LOOP(vs, 11);
        PEEK_BITS(vs, 11, high);
        vs->motionHCode = motionTable[high].value;
        vs->bitOffset -= motionTable[high].numBits;
        vs->curBits &= bitMask[vs->bitOffset];
        if (vs->curPic->forwardF != 1 && vs->motionHCode != 0) {
            while (vs->curPic->forwardRSize > vs->bitOffset) {
                READ_BYTE(vs);
                vs->bitOffset += 8;
            }
            vs->bitOffset -= vs->curPic->forwardRSize;
            vs->motionHR = vs->curBits >> vs->bitOffset;
            vs->curBits &= bitMask[vs->bitOffset];
        }
        NEED_BITS2_LOOP(vs, 11);
        PEEK_BITS(vs, 11, high);
        vs->motionVCode = motionTable[high].value;
        vs->bitOffset -= motionTable[high].numBits;
        vs->curBits &= bitMask[vs->bitOffset];
        if (vs->curPic->forwardF != 0 && vs->motionVCode != 0) {
            while (vs->curPic->forwardRSize > vs->bitOffset) {
                READ_BYTE(vs);
                vs->bitOffset += 8;
            }
            vs->bitOffset -= vs->curPic->forwardRSize;
            vs->motionVR = vs->curBits >> vs->bitOffset;
            vs->curBits &= bitMask[vs->bitOffset];
        }
    }
    if (vs->mbType & 8) {
        NEED_BITS2(vs, 9);
        PEEK_BITS(vs, 9, high);
        vs->cbp = codedBlockPatternTable[high].value;
        vs->bitOffset -= codedBlockPatternTable[high].numBits;
        vs->curBits &= bitMask[vs->bitOffset];
    } else {
        vs->cbp = 0;
    }
    if (vs->mbType & 0x10) {
        if (vs->bufEnd - vs->bufStart < 0x200)
            mpegRefillVideoStreamBuffer(vs);
        mpegStream = vs;
        mpegRingLimit = vs->ringLimit;
        rc = mpegDecodeIntraBlocks();
        vs->fwdPredH = vs->fwdPredV = 0;
        return;
    }
    bv = 0;
    vs->dcPredY = vs->dcPredCb = vs->dcPredCr = 0x80;
    if (vs->mbType & 2) {
        if (vs->curPic->forwardF == 1 || vs->motionHCode == 0)
            ch = 0;
        else
            ch = vs->curPic->forwardF - 1 - vs->motionHR;
        if (vs->curPic->forwardF == 1 || vs->motionVCode == 0)
            loopStart = 0;
        else
            loopStart = vs->curPic->forwardF - 1 - vs->motionVR;
        sx = vs->curPic->forwardF * vs->motionHCode;
        if (sx == 0) {
            counter = 0;
        } else if (sx > 0) {
            sx -= ch;
            counter = sx - (vs->curPic->forwardF << 5);
        } else {
            sx += ch;
            counter = (vs->curPic->forwardF << 5) + sx;
        }
        x = vs->curPic->forwardF * vs->motionVCode;
        if (x == 0) {
            hc = 0;
        } else if (x > 0) {
            x -= loopStart;
            hc = x - (vs->curPic->forwardF << 5);
        } else {
            x += loopStart;
            hc = (vs->curPic->forwardF << 5) + x;
        }
        f = (vs->curPic->forwardF << 4) - 1;
        minv = -(vs->curPic->forwardF << 4);
        rh = vs->fwdPredH + sx;
        if (f >= rh && rh >= minv)
            srcp = vs->fwdPredH + sx;
        else
            srcp = vs->fwdPredH + counter;
        vs->fwdPredH = srcp;
        if (vs->curPic->fullPelForward)
            srcp *= 2;
        rh = vs->fwdPredV + x;
        if (f >= rh && rh >= minv)
            mbaddr = vs->fwdPredV + x;
        else
            mbaddr = vs->fwdPredV + hc;
        vs->fwdPredV = mbaddr;
        if (vs->curPic->fullPelForward)
            mbaddr *= 2;
        s = srcp & 1;
        k0 = mbaddr & 1;
        srcp >>= 1;
        if (srcp < 0) {
            if (vs->mbPixelX + srcp < 0)
                bv = 1;
        } else if (vs->mbPixelX + srcp > (vs->mbWidth << 4) - 16) {
            bv = 1;
        }
        mbaddr >>= 1;
        if (mbaddr < 0) {
            if (vs->mbPixelY + mbaddr < 0)
                bv = 1;
        } else if (vs->mbPixelY + mbaddr > (vs->mbHeight << 4) - 16) {
            bv = 1;
        }
        if (bv) {
            min = (unsigned char *)vs->yOffset;
            for (hv = 0; hv < 16; hv++) {
                for (kk = 0; kk < 16; kk++)
                    *min++ = 0x80;
                min += vs->yStride - 8;
            }
            min = (unsigned char *)vs->cbOffset;
            for (hv = 0; hv < 8; hv++) {
                for (kk = 0; kk < 8; kk++)
                    *min++ = 0x80;
                min += vs->cbStride;
            }
            min = (unsigned char *)vs->crOffset;
            for (hv = 0; hv < 8; hv++) {
                for (kk = 0; kk < 8; kk++)
                    *min++ = 0x80;
                min += vs->crStride;
            }
        } else if (srcp == 0 && mbaddr == 0 && !s && !k0) {
            mpegStream = vs;
            mpegRingLimit = vs->yPastDelta;
            mpegCopyMacroblock();
        } else {
            if (s) {
                if (k0) {
                    mpegStream = vs;
                    mpegRingLimit = vs->yPastDelta - srcp - vs->mbWidth * mbaddr * 16;
                    mpegPredictCopy16HalfXY();
                } else {
                    mpegStream = vs;
                    mpegRingLimit = vs->yPastDelta - srcp - vs->mbWidth * mbaddr * 16;
                    mpegPredictCopy16HalfX();
                }
            } else if (k0) {
                mpegStream = vs;
                mpegRingLimit = vs->yPastDelta - srcp - vs->mbWidth * mbaddr * 16;
                mpegPredictCopy16HalfY();
            } else {
                mpegStream = vs;
                mpegRingLimit = vs->yPastDelta - srcp - vs->mbWidth * mbaddr * 16;
                mpegPredictCopy16();
            }
            s = srcp & 1;
            srcp >>= 1;
            k0 = mbaddr & 1;
            mbaddr >>= 1;
            if (s) {
                if (k0) {
                    mpegStream = vs;
                    mpegRingLimit = vs->yPastDelta - srcp - vs->mbWidth * mbaddr * 8;
                    mpegPredictCopy8HalfXY();
                } else {
                    mpegStream = vs;
                    mpegRingLimit = vs->yPastDelta - srcp - vs->mbWidth * mbaddr * 8;
                    mpegPredictCopy8HalfX();
                }
            } else if (k0) {
                mpegStream = vs;
                mpegRingLimit = vs->yPastDelta - srcp - vs->mbWidth * mbaddr * 8;
                mpegPredictCopy8HalfY();
            } else {
                mpegStream = vs;
                mpegRingLimit = vs->yPastDelta - srcp - vs->mbWidth * mbaddr * 8;
                mpegPredictCopy8();
            }
        }
    } else {
        vs->fwdPredH = vs->fwdPredV = 0;
        mpegStream = vs;
        mpegRingLimit = vs->yPastDelta;
        mpegCopyMacroblock();
    }
    if (vs->cbp) {
        if (vs->bufEnd - vs->bufStart < 0x200)
            mpegRefillVideoStreamBuffer(vs);
        mpegStream = vs;
        mpegRingLimit = vs->ringLimit;
        n = mpegDecodeNonIntraBlocks();
    }
}

/* Motion vector decoding helpers for bidirectional macroblocks. */
#define MV_COMP(f, code, rbits, comp) \
    if ((f) == 1 || (code) == 0) \
        comp = 0; \
    else \
        comp = (f) - 1 - (rbits);

#define MV_SCALE(prod, comp, newv, f) \
    if ((prod) == 0) { \
        newv = 0; \
    } else if ((prod) > 0) { \
        (prod) -= (comp); \
        newv = (prod) - ((f) << 5); \
    } else { \
        (prod) += (comp); \
        newv = ((f) << 5) + (prod); \
    }

#define MV_RECON(vs, field, prod, newv, maxv, minv, sum, recon, fullpel) \
    sum = (vs)->field + (prod); \
    if (sum <= maxv && sum >= minv) \
        recon = (vs)->field + (prod); \
    else \
        recon = (vs)->field + (newv); \
    (vs)->field = recon; \
    if (fullpel) \
        recon *= 2;

/* Split a vector into half-pel flags and whole pixels, and flag blocks reaching outside the picture. */
#define HALF_AND_CLIP(vs, rr, rd, rh, dh, outside) \
    rh = (rr) & 1; \
    dh = (rd) & 1; \
    (rr) >>= 1; \
    if ((rr) < 0) { \
        if ((vs)->mbPixelX + (rr) < 0) \
            outside = 1; \
    } else if ((vs)->mbPixelX + (rr) > ((vs)->mbWidth << 4) - 16) { \
        outside = 1; \
    } \
    (rd) >>= 1; \
    if ((rd) < 0) { \
        if ((vs)->mbPixelY + (rd) < 0) \
            outside = 1; \
    } else if ((vs)->mbPixelY + (rd) > ((vs)->mbHeight << 4) - 16) { \
        outside = 1; \
    }

/* Predict a block from the reference at a half-pel offset with one of four routines. */
#define PREDICT(vs, base, r, d, mul, rh, dh, f11, f10, f01, f00) \
    if (rh) { \
        if (dh) { \
            mpegStream = (vs); \
            mpegRingLimit = (vs)->base - (r) - (vs)->mbWidth * (d) * (mul); \
            f11(); \
        } else { \
            mpegStream = (vs); \
            mpegRingLimit = (vs)->base - (r) - (vs)->mbWidth * (d) * (mul); \
            f10(); \
        } \
    } else if (dh) { \
        mpegStream = (vs); \
        mpegRingLimit = (vs)->base - (r) - (vs)->mbWidth * (d) * (mul); \
        f01(); \
    } else { \
        mpegStream = (vs); \
        mpegRingLimit = (vs)->base - (r) - (vs)->mbWidth * (d) * (mul); \
        f00(); \
    }

/* Luma then chroma prediction using the copying routines. */
#define PREDICT_COPY(vs, base, r, d, rh, dh) \
    PREDICT(vs, base, r, d, 16, rh, dh, mpegPredictCopy16HalfXY, mpegPredictCopy16HalfX, mpegPredictCopy16HalfY, mpegPredictCopy16) \
    rh = (r) & 1; \
    (r) >>= 1; \
    dh = (d) & 1; \
    (d) >>= 1; \
    PREDICT(vs, base, r, d, 8, rh, dh, mpegPredictCopy8HalfXY, mpegPredictCopy8HalfX, mpegPredictCopy8HalfY, mpegPredictCopy8)

/* Same, averaging into what the forward prediction already wrote. */
#define PREDICT_AVERAGE(vs, base, r, d, rh, dh) \
    PREDICT(vs, base, r, d, 16, rh, dh, mpegPredictAverage16HalfXY, mpegPredictAverage16HalfX, mpegPredictAverage16HalfY, mpegPredictAverage16) \
    rh = (r) & 1; \
    (r) >>= 1; \
    dh = (d) & 1; \
    (d) >>= 1; \
    PREDICT(vs, base, r, d, 8, rh, dh, mpegPredictAverage8HalfXY, mpegPredictAverage8HalfX, mpegPredictAverage8HalfY, mpegPredictAverage8)

#define SKIP_ADVANCE(vs) \
    (vs)->mbPixelX += 16; \
    if (((vs)->mbWidth << 4) == (vs)->mbPixelX) { \
        (vs)->yOffset += (vs)->yBase; \
        (vs)->cbOffset += (vs)->cbBase; \
        (vs)->crOffset += (vs)->crBase; \
        (vs)->mbPixelX = 0; \
        (vs)->mbPixelY += 16; \
    } else { \
        (vs)->yOffset += 16; \
        (vs)->cbOffset += 8; \
        (vs)->crOffset += 8; \
    }

/* Macroblocks of a bidirectionally predicted picture. */
void mpegDecodeBidirectionalMacroblock(VidStream *vs)
{
    int reconRight; /* forward down delta */
    int res; /* decode status */
    short begin; /* forward right vector */
    int tt; /* forward right delta */
    short hc; /* forward down vector */
    int loopcount; /* backward right delta */
    unsigned char nr; /* skipped MB: forward right half-pel */
    short back; /* skipped MB: backward down vector */
    unsigned char c; /* skipped MB: backward right half-pel */
    int ctr; /* forward vector minimum */
    short rsize; /* backward down vector */
    unsigned char s; /* skipped MB: backward down half-pel */
    short fx; /* quantiser scale */
    int dst2; /* backward down delta, wrapped */
    unsigned char nz; /* backward right half-pel */
    unsigned char ret; /* forward right half-pel */
    int df; /* backward vector minimum */
    int ch; /* forward vector candidate */
    unsigned char bot; /* forward down half-pel */
    unsigned char nbit; /* skipped MB: forward down half-pel */
    short elapsed; /* macroblock address increment */
    unsigned char fwd; /* block reaches outside the picture */
    int counter; /* backward vector candidate */
    int r0; /* residual decode status */
    int hh; /* backward horizontal correction */
    int chroma; /* forward horizontal correction */
    int imm; /* forward vector maximum */
    unsigned int level; /* table index */
    int y0; /* forward vertical correction */
    short freq; /* skipped MB: forward right vector */
    int u1; /* backward vector maximum */
    int skipRight; /* backward vertical correction */
    unsigned char cv; /* backward down half-pel */
    unsigned char inc; /* skipped MB reaches outside the picture */
    int sx; /* forward down delta, wrapped */
    int name; /* backward down delta */
    short arr; /* skipped macroblock counter */
    int oz; /* forward right delta, wrapped */
    short rd; /* skipped MB: backward right vector */
    short mn; /* backward right vector */
    int kind; /* backward right delta, wrapped */
    short hrecon; /* skipped MB: forward down vector */

    do {
        if (vs->bufEnd - vs->bufStart < 8)
            mpegRefillVideoStreamBuffer(vs);
        NEED_BITS2_LOOP(vs, 11);
        PEEK_BITS(vs, 11, level);
        elapsed = mbAddrIncTable[level].value;
        vs->bitOffset -= mbAddrIncTable[level].numBits;
        vs->curBits &= bitMask[vs->bitOffset];
        if (elapsed == 0x22) {
            vs->mbIndex += 33;
            elapsed = 0x23;
        }
    } while (elapsed == 0x23);
    vs->mbIndex += elapsed;
    if (vs->bufEnd - vs->bufStart < 0x20)
        mpegRefillVideoStreamBuffer(vs);
    if (vs->curPic->lastMacroblock + 1 < vs->mbIndex) {
        inc = 0;
        arr = vs->curPic->lastMacroblock + 1;
        vs->dcPredY = vs->dcPredCb = vs->dcPredCr = 0x80;
        for (; vs->mbIndex != arr; arr++) {
            freq = vs->fwdPredH;
            hrecon = vs->fwdPredV;
            rd = vs->bwdPredH;
            back = vs->bwdPredV;
            SKIP_ADVANCE(vs)
            if (vs->mbType & 2) {
                HALF_AND_CLIP(vs, freq, hrecon, nr, nbit, inc)
            }
            if (vs->mbType & 4) {
                HALF_AND_CLIP(vs, rd, back, c, s, inc)
            }
            if (!inc) {
                if (vs->mbType & 2) {
                    if (freq == 0 && hrecon == 0 && !nr && !nbit) {
                        mpegStream = vs;
                        mpegRingLimit = vs->yFutureDelta;
                        mpegCopyMacroblock();
                    } else {
                        PREDICT_COPY(vs, yFutureDelta, freq, hrecon, nr, nbit)
                    }
                    if (vs->mbType & 4) {
                        PREDICT_AVERAGE(vs, yPastDelta, rd, back, c, s)
                    }
                } else if (vs->mbType & 4) {
                    if (rd == 0 && back == 0 && !c && !s) {
                        mpegStream = vs;
                        mpegRingLimit = vs->yPastDelta;
                        mpegCopyMacroblock();
                    } else {
                        PREDICT_COPY(vs, yPastDelta, rd, back, c, s)
                    }
                } else {
                    mpegDebugMessage("Fish");
                }
            }
        }
    }
    vs->curPic->lastMacroblock = vs->mbIndex;
    SKIP_ADVANCE(vs)
    NEED_BITS3(vs, 6);
    PEEK_BITS(vs, 6, level);
    vs->mbType = mbTypeBTable[level].value;
    vs->bitOffset -= mbTypeBTable[level].numBits;
    vs->curBits &= bitMask[vs->bitOffset];
    if (vs->mbType & 1) {
        GET_BITS3(vs, 5, fx);
        vs->curIntraQ = vs->intraQuant + (fx << 6);
        vs->curNonIntraQ = vs->nonIntraQuant + (fx << 6);
    }
    if (vs->mbType & 2) {
        NEED_BITS2_LOOP(vs, 11);
        PEEK_BITS(vs, 11, level);
        vs->motionHCode = motionTable[level].value;
        vs->bitOffset -= motionTable[level].numBits;
        vs->curBits &= bitMask[vs->bitOffset];
        if (vs->curPic->forwardF != 1 && vs->motionHCode != 0) {
            while (vs->curPic->forwardRSize > vs->bitOffset) {
                READ_BYTE(vs);
                vs->bitOffset += 8;
            }
            vs->bitOffset -= vs->curPic->forwardRSize;
            vs->motionHR = vs->curBits >> vs->bitOffset;
            vs->curBits &= bitMask[vs->bitOffset];
        }
        NEED_BITS2_LOOP(vs, 11);
        PEEK_BITS(vs, 11, level);
        vs->motionVCode = motionTable[level].value;
        vs->bitOffset -= motionTable[level].numBits;
        vs->curBits &= bitMask[vs->bitOffset];
        if (vs->curPic->forwardF != 0 && vs->motionVCode != 0) {
            while (vs->curPic->forwardRSize > vs->bitOffset) {
                READ_BYTE(vs);
                vs->bitOffset += 8;
            }
            vs->bitOffset -= vs->curPic->forwardRSize;
            vs->motionVR = vs->curBits >> vs->bitOffset;
            vs->curBits &= bitMask[vs->bitOffset];
        }
    }
    if (vs->mbType & 4) {
        NEED_BITS2_LOOP(vs, 11);
        PEEK_BITS(vs, 11, level);
        vs->backHCode = motionTable[level].value;
        vs->bitOffset -= motionTable[level].numBits;
        vs->curBits &= bitMask[vs->bitOffset];
        if (vs->curPic->backwardF != 1 && vs->backHCode != 0) {
            while (vs->curPic->backwardRSize > vs->bitOffset) {
                READ_BYTE(vs);
                vs->bitOffset += 8;
            }
            vs->bitOffset -= vs->curPic->backwardRSize;
            vs->backHR = vs->curBits >> vs->bitOffset;
            vs->curBits &= bitMask[vs->bitOffset];
        }
        NEED_BITS2_LOOP(vs, 11);
        PEEK_BITS(vs, 11, level);
        vs->backVCode = motionTable[level].value;
        vs->bitOffset -= motionTable[level].numBits;
        vs->curBits &= bitMask[vs->bitOffset];
        if (vs->curPic->backwardF != 0 && vs->backVCode != 0) {
            while (vs->curPic->backwardRSize > vs->bitOffset) {
                READ_BYTE(vs);
                vs->bitOffset += 8;
            }
            vs->bitOffset -= vs->curPic->backwardRSize;
            vs->backVR = vs->curBits >> vs->bitOffset;
            vs->curBits &= bitMask[vs->bitOffset];
        }
    }
    if (vs->mbType & 8) {
        NEED_BITS2(vs, 9);
        PEEK_BITS(vs, 9, level);
        vs->cbp = codedBlockPatternTable[level].value;
        vs->bitOffset -= codedBlockPatternTable[level].numBits;
        vs->curBits &= bitMask[vs->bitOffset];
    } else {
        vs->cbp = 0;
    }
    if (vs->mbType & 0x10) {
        if (vs->bufEnd - vs->bufStart < 0x200)
            mpegRefillVideoStreamBuffer(vs);
        mpegStream = vs;
        mpegRingLimit = vs->ringLimit;
        res = mpegDecodeIntraBlocks();
        vs->fwdPredH = vs->fwdPredV = 0;
        vs->bwdPredH = vs->bwdPredV = 0;
        return;
    }
    fwd = 0;
    vs->dcPredY = vs->dcPredCb = vs->dcPredCr = 0x80;
    if (vs->mbType & 2) {
        MV_COMP(vs->curPic->forwardF, vs->motionHCode, vs->motionHR, chroma)
        MV_COMP(vs->curPic->forwardF, vs->motionVCode, vs->motionVR, y0)
        tt = vs->curPic->forwardF * vs->motionHCode;
        MV_SCALE(tt, chroma, oz, vs->curPic->forwardF)
        reconRight = vs->curPic->forwardF * vs->motionVCode;
        MV_SCALE(reconRight, y0, sx, vs->curPic->forwardF)
        imm = (vs->curPic->forwardF << 4) - 1;
        ctr = -(vs->curPic->forwardF << 4);
        MV_RECON(vs, fwdPredH, tt, oz, imm, ctr, ch, begin, vs->curPic->fullPelForward)
        MV_RECON(vs, fwdPredV, reconRight, sx, imm, ctr, ch, hc, vs->curPic->fullPelForward)
        HALF_AND_CLIP(vs, begin, hc, ret, bot, fwd)
    }
    if (vs->mbType & 4) {
        MV_COMP(vs->curPic->backwardF, vs->backHCode, vs->backHR, hh)
        MV_COMP(vs->curPic->backwardF, vs->backVCode, vs->backVR, skipRight)
        loopcount = vs->curPic->backwardF * vs->backHCode;
        MV_SCALE(loopcount, hh, kind, vs->curPic->backwardF)
        name = vs->curPic->backwardF * vs->backVCode;
        MV_SCALE(name, skipRight, dst2, vs->curPic->backwardF)
        u1 = (vs->curPic->backwardF << 4) - 1;
        df = -(vs->curPic->backwardF << 4);
        MV_RECON(vs, bwdPredH, loopcount, kind, u1, df, counter, mn, vs->curPic->fullPelBackward)
        MV_RECON(vs, bwdPredV, name, dst2, u1, df, counter, rsize, vs->curPic->fullPelBackward)
        HALF_AND_CLIP(vs, mn, rsize, nz, cv, fwd)
    }
    if (!fwd) {
        if (vs->mbType & 2) {
            if (begin == 0 && hc == 0 && !ret && !bot) {
                mpegStream = vs;
                mpegRingLimit = vs->yFutureDelta;
                mpegCopyMacroblock();
            } else {
                PREDICT_COPY(vs, yFutureDelta, begin, hc, ret, bot)
            }
            if (vs->mbType & 4) {
                PREDICT_AVERAGE(vs, yPastDelta, mn, rsize, nz, cv)
            }
        } else if (vs->mbType & 4) {
            if (mn == 0 && rsize == 0 && !nz && !cv) {
                mpegStream = vs;
                mpegRingLimit = vs->yPastDelta;
                mpegCopyMacroblock();
            } else {
                PREDICT_COPY(vs, yPastDelta, mn, rsize, nz, cv)
            }
        } else {
            mpegDebugMessage("Fish");
        }
    }
    if (vs->cbp) {
        if (vs->bufEnd - vs->bufStart < 0x200)
            mpegRefillVideoStreamBuffer(vs);
        mpegStream = vs;
        mpegRingLimit = vs->ringLimit;
        r0 = mpegDecodeNonIntraBlocks();
    }
}

/* Shows (or drops) the frame at the head of the ring when its time has come. */
void mpegPresentQueuedFrame(VidStream *vs)
{
    unsigned int n;

    if (vs->ring[vs->ringHead].count == 0)
        return;
    if (mpegPaced) {
        if (!vs->frameTimerStarted)
            return;
        if (vs->ring[vs->ringHead].count > mpegTicks)
            return;
    }
    n = vs->ring[vs->ringHead].count - 1;
    if (vs->ring[vs->ringHead].frame != 0) {
        if (mpegFirstFrame <= n && (mpegLastFrame == 0 || mpegLastFrame >= n))
            mpegDrawFrame(vs->ring[vs->ringHead].frame, n);
        else
            vs->ring[vs->ringHead].frame->inUse &= 0xfe;
    }
    vs->ring[vs->ringHead].count = 0;
    vs->ringHead = (vs->ringHead + 1) % 64;
}

/* Sets up the sound queue if there is a sound buffer. */
void mpegStartAudioQueue(void)
{
    void *mem;

    if (mpegSoundBuf == 0) {
        mpegAudioActive = 0;
        return;
    }
    mem = mpegHeapAlloc(&mpegVideo->heap, 0xc0000);
    if (mpegInitAudioQueue(0x40, 0x1000, mem)) {
        mpegAudioActive = 0;
        return;
    }
    mpegSetAudioBlockCallback(mpegAudioBlockPlayed);
    mpegAudioActive = 1;
}

/* Stops MPEG audio playback when its sound queue is active. */
void mpegStopAudioQueue(void)
{
    if (mpegAudioActive) {
        mpegStopAudioPlayback();
        mpegResetAudioBlockQueue();
    }
}

/* Called by the sound queue each time a block has been played. */
void mpegAudioBlockPlayed(void)
{
    mpegAudioPos++;
}

/* Frame timer: keeps the frame clock within a few ticks of the sound clock. */
void __stdcall mpegFrameTimerCallback(unsigned id, unsigned msg, unsigned long user, unsigned long dw1, unsigned long dw2)
{
    mpegTicks++;
    mpegTickTotal++;
    mpegFrameCount++;
    if (mpegLate > 10)
        mpegSync = 0;
    if (mpegSync != 0 && mpegAudioActive) {
        if (mpegAudioPos == 0)
            mpegLate++;
        while (mpegAudioPos + 5 < mpegTicks)
            mpegTicks--;
        while (mpegTicks + 3 < mpegAudioPos)
            mpegTicks++;
    }
}
