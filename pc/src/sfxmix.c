#include "reverb.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;

#include "pc_sfx.h"
#include "pc_sfxmix.h"

#ifdef NETPLAY
#pragma intrinsic(memcpy)
#endif

void MonoEcho(void);
void StereoEcho(void);
void MonoDelay(void);
void StereoDelay(void);
void MonoReverb(void);
void ClearReverbBuffers(void);

#ifdef NETPLAY
unsigned int SfxDownmixSamples(short *dst, short *src, unsigned int size);
unsigned int SfxConvertSamplesTo8Bit(u_char *dst, short *src, unsigned int size);
unsigned int SfxConvertSamplesToCompressed8Bit(u_char *dst, short *src, unsigned int size);
#endif

/* Converts the decoded sample buffer to the output format: optional stereo downmix,
   then 8-bit shift, 8-bit compression through the quantisation table, or a raw copy. */
void SfxConvertSamples(u_char *dst, short *src, u_int size, u_int flags)
{
#ifdef NETPLAY
    if (flags & 1) {
        SfxDownmixSamples(src, src, size);
        size >>= 1;
    }
    if (flags & 2) {
        SfxConvertSamplesTo8Bit(dst, src, size);
    } else if (flags & 4) {
        SfxConvertSamplesToCompressed8Bit(dst, src, size);
    } else {
        memcpy(dst, src, size);
    }
}

unsigned int SfxDownmixSamples(short *dst, short *src, unsigned int size)
{
    u_int count;
    u_int i;
    u_int j;

    count = size >> 1;
    i = 0;
    j = 0;
    for (; i < count; i += 2, j++)
        dst[j] = (src[i] + src[i + 1]) >> 1;
    return count;
}

unsigned int SfxConvertSamplesTo8Bit(u_char *dst, short *src, unsigned int size)
{
    unsigned int count;
    unsigned int i;

    count = size >> 1;
    for (i = 0; i < count; i++)
        dst[i] = (src[i] >> 8) + 0x80;
    return count;
}

unsigned int SfxConvertSamplesToCompressed8Bit(u_char *dst, short *src, unsigned int size)
{
    u_short level;
    u_short bitMask;
    unsigned int numOut;
    unsigned int i;
    int *quant;

    quant = quantTable + 0x1f00;
    numOut = size >> 1;
    for (i = 0; i < numOut; i++) {
        level = 0;
        for (bitMask = 0x80; bitMask > 0; bitMask >>= 1) {
            level |= bitMask;
            if (quant[level] > src[i])
                level &= ~bitMask;
        }
        dst[i] = level;
    }
    return numOut;
}
#else
    u_short bit;
    u_int i;
    int *table;
    u_short level;
    short *mixOut;
    u_int count;
    short *mixIn;

    if (flags & 1) {
        count = size >> 1;
        if (count > 0) {
            mixIn = src + 1;
            mixOut = src;
            i = (count + 1) >> 1;
            do {
                *mixOut++ = (mixIn[-1] + mixIn[0]) >> 1;
                mixIn += 2;
            } while (--i);
        }
        size = count;
    }
    if (flags & 2) {
        int samples = size >> 1;

        for (i = 0; i < samples; i++)
            dst[i] = (src[i] >> 8) + 0x80;
    } else if (flags & 4) {
        table = quantTable + 0x1f00;
        count = size >> 1;
        for (i = 0; count > i; i++) {
            level = 0;
            for (bit = 0x80; bit; bit >>= 1) {
                level |= bit;
                if (table[level] > src[i])
                    level &= ~bit;
            }
            dst[i] = level;
        }
    } else {
        memcpy(dst, src, size);
    }
}
#endif

/* Initializes the selected reverb mode and its delay buffers. */
u_short InitReverb(u_short type, u_char channels, u_short ms)
{
    u_int count;

    reverbType = type;
    switch (type) {
    case 0:
    case 1:
    case 2:
        if (type == 1)
            delayCoeff = 2;
        else
            delayCoeff = 1;
        count = (waveFormat.nSamplesPerSec * ms) / 1000;
        delayBuf = GlobalAlloc(GPTR, count * sizeof(int));
        if (delayBuf == NULL)
            return 1;
        delayPos = delayBuf;
        delayLen = count;
        break;
    case 3:
        workBuf = GlobalAlloc(GPTR, sizeof(int) * mixBufFrames);
        if (workBuf == NULL)
            return 1;
        count = (waveFormat.nSamplesPerSec * 51) / 1000;
        comb1Buf = GlobalAlloc(GPTR, count * sizeof(int));
        if (comb1Buf == NULL)
            return 1;
        comb1Pos = comb1Buf;
        comb1Len = count;
        count = (waveFormat.nSamplesPerSec * 27) / 1000;
        comb2Buf = GlobalAlloc(GPTR, count * sizeof(int));
        if (comb2Buf == NULL)
            return 1;
        comb2Pos = comb2Buf;
        comb2Len = count;
        count = (waveFormat.nSamplesPerSec * 43) / 1000;
        comb3Buf = GlobalAlloc(GPTR, count * sizeof(int));
        if (comb3Buf == NULL)
            return 1;
        comb3Pos = comb3Buf;
        comb3Len = count;
        count = (waveFormat.nSamplesPerSec * 7) / 1000;
        allPassBuf = GlobalAlloc(GPTR, count * sizeof(int));
        if (allPassBuf == NULL)
            return 1;
        allPassPos = allPassBuf;
        allPassLen = count;
        break;
    default:
        return 1;
    }
    ClearReverbBuffers();
    type <<= 1;
    type += (channels == 2 ? 1 : 0);
    switch (type) {
    case 0:
        doReverb = MonoEcho;
        break;
    case 1:
        doReverb = StereoEcho;
        break;
    case 2:
    case 4:
        doReverb = MonoDelay;
        break;
    case 3:
    case 5:
        doReverb = StereoDelay;
        break;
    case 6:
        doReverb = MonoReverb;
        break;
    case 7:
        doReverb = StereoReverb;
        break;
    }
    return 0;
}

/* Releases reverb delay buffers and resets mixer state. */
void UnInitReverb(void)
{
    switch (reverbType) {
    case 0:
    case 1:
    case 2:
        GlobalFree(delayBuf);
        break;
    case 3:
        GlobalFree(workBuf);
        GlobalFree(comb1Buf);
        GlobalFree(comb2Buf);
        GlobalFree(comb3Buf);
        GlobalFree(allPassBuf);
        break;
    }
}

/* Updates the volume applied to the reverb output. */
void SetReverbDepth(u_char depth)
{
#ifdef NETPLAY
    u_int maskedDepth;

    maskedDepth = depth & 0x7f;
    reverbDepth = 3 - (maskedDepth >> 5);
#else
    reverbDepth = 3 - ((depth & 0x60) >> 5);
#endif
}

/* Mixes the current buffer through the active reverb effect. */
void AddReverb(void)
{
    doReverb();
}

/* Clears every delay buffer used by the reverb effects. */
void ClearReverbBuffers(void)
{
    u_int count;
    int *p;

    switch (reverbType) {
    case 0:
    case 1:
    case 2:
        p = delayBuf;
        count = delayLen;
        do {
            *p = 0;
            p++;
            count--;
        } while (count);
        break;
    case 3:
        p = comb1Buf;
        count = comb1Len;
        do {
            *p = 0;
            p++;
            count--;
        } while (count);
        p = comb2Buf;
        count = comb2Len;
        do {
            *p = 0;
            p++;
            count--;
        } while (count);
        p = comb3Buf;
        count = comb3Len;
        do {
            *p = 0;
            p++;
            count--;
        } while (count);
        p = allPassBuf;
        count = allPassLen;
        do {
            *p = 0;
            p++;
            count--;
        } while (count);
        break;
    }
}

