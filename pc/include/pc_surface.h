#ifndef PC_SURFACE_H
#define PC_SURFACE_H

#include <windows.h>
#include <ddraw.h>

/* Bit layout of a 16-bit pixel format, filled from a DDPIXELFORMAT by GetPixelChannels.
   Each channel's shift is the position of its lowest mask bit; the loss fields are
   8 minus the channel's bit count. Without an alpha mask, alphaShift and alphaBits both take
   the green bit count. 0x30 bytes. */
typedef struct PixelChannels {
    int redShift, greenShift, blueShift, alphaShift;
    int redBits, greenBits, blueBits, alphaBits;
    int redLoss, greenLoss, blueLoss, alphaLoss;
} PixelChannels;

extern LPDIRECTDRAWSURFACE fadeRampSurface;
#ifdef POWERVR
extern LPDIRECTDRAWCLIPPER windowClipper;
#endif

void GetPixelChannels(PixelChannels *channels, DDPIXELFORMAT format);

#endif
