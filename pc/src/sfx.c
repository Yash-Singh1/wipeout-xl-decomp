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

#include "pc_sfx_types.h"

/* One slot of the playing-effect table. */
typedef struct SfxSlot {
    SfxSample *sample;
    u_short state;              /* pending list change: 1 add, 4 kill, 8 unlink */
    u_short pad;
} SfxSlot;

/* A sample converted for the software mixer. */
typedef struct SfxCacheEntry {
    u_char *data;
    u_char *rightData;
    u_int length;
    u_short rate;
    u_short flags;
    int pad10;
} SfxCacheEntry;

#include "pc_wave.h"


extern u_char sfxActive;

extern u_char reverbOn;
extern u_char sfxMuteState;
extern u_char sfxCallbackOn;
extern u_char sfxMixMode;
extern u_int sfxListUpdate;
extern SfxSlot sfxSlots[32];
extern SfxCacheEntry sfxCache[128];
extern SfxSample *sfxListHead;
extern SfxSample *sfxListTail;
extern u_char savedVolume[];
extern SfxSample *sfxIter;
extern SfxSample *sfxIter2;
extern SfxSample *sfxItem;
extern HRESULT mixResult;
extern LPDIRECTSOUNDBUFFER mixBuffer;
/* Declaration order decides the load order of these in the /Od mixer code (SfxDoSfx). */
extern u_int mixBufferBytes;
extern u_int mixLeadBytes;
extern u_int mixPos;
extern u_int mixChunkBytes, mixHalfBytes, mixQuarterBytes;
extern u_int mixScratchBytes, mixUnused0;
#include "sample_output.h"
extern u_short (*sfxDoMix)(SfxSample **list);

#include "reverb.h"
void UnInitReverb(void);
void ClearReverbBuffers(void);
void SetReverbDepth(u_char depth);
void AddReverb(void);
void SfxFreeMixTables(void);
#include "pc_sfx.h"
#include "pc_sfxmix.h"

void AddItem(SfxSample *item);

#ifdef NETPLAY
static void RemoveItem(SfxSample *item);
#else
/* Takes a sample out of the playing list. */
static __inline void RemoveItem(SfxSample *item)
{
    if (item->prev) {
        item->prev->next = item->next;
    } else {
        sfxListHead = item->next;
        if (sfxListHead)
            sfxListHead->prev = 0;
    }
    if (item->next) {
        item->next->prev = item->prev;
    } else {
        sfxListTail = item->next;
        if (sfxListTail)
            sfxListTail->next = 0;
    }
    item->prev = 0;
    item->next = 0;
}
#endif

#ifdef NETPLAY
/* The NET build expands this memset inline (rep stosd) as the exe does. */
#pragma intrinsic(memset)
#endif

/* Creates the DirectSound buffer used by the software mixer. */
int SfxSoftMixInit(LPDIRECTSOUND sound, u_int mode, u_int rate)
{
    void *ptr1;
    DWORD len2;
    DWORD bytes1;
    DSBUFFERDESC bufDesc;
    u_int n;
    void *ptr2;
#ifndef NETPLAY
    LPDIRECTSOUND currentSound = sound;
#endif

    sfxActive = 0;
    memset(&bufDesc, 0, sizeof(DSBUFFERDESC));
    bufDesc.dwSize = sizeof(DSBUFFERDESC);
    bufDesc.dwFlags = 0x100e0;
    bufDesc.dwBufferBytes = 0x8000;
    bufDesc.lpwfxFormat = &waveFormat;
#ifdef NETPLAY
    mixResult = IDirectSound_CreateSoundBuffer(sound, &bufDesc, &mixBuffer, NULL);
#else
    mixResult = IDirectSound_CreateSoundBuffer(currentSound, &bufDesc, &mixBuffer, NULL);
#endif
    if (mixResult != DS_OK) {
#ifdef NETPLAY
        IDirectSound_Release(sound);
        sound = 0;
#else
        IDirectSound_Release(currentSound);
#endif
        return 1;
    }
    mixUnused0 = 0x400;
#ifdef NETPLAY
    mixChunkBytes = (waveFormat.nChannels == 2) ? mixUnused0 * 2 : mixUnused0;
    mixLeadBytes = (waveFormat.wBitsPerSample == 16) ? mixChunkBytes * 2 : mixChunkBytes;
    mixBufferBytes = 0x8000;
    mixHalfBytes = (waveFormat.wBitsPerSample == 16) ? mixBufferBytes >> 1 : mixBufferBytes;
    mixQuarterBytes = (waveFormat.nChannels == 2) ? mixHalfBytes >> 1 : mixHalfBytes;
    mixBufFrames = mixUnused0;
    mixBlockWords = mixBufFrames * 2;
    mixScratchBytes = mixBlockWords * 4;
    mixResult = IDirectSoundBuffer_Lock(mixBuffer, 0, mixBufferBytes, &ptr1, &bytes1, &ptr2, &len2, 0);
#else
    mixChunkBytes = (waveFormat.nChannels == 2) ? 0x800 : 0x400;
    mixLeadBytes = (waveFormat.wBitsPerSample == 16) ? mixChunkBytes * 2 : mixChunkBytes;
    mixBufferBytes = 0x8000;
    mixHalfBytes = (waveFormat.wBitsPerSample == 16) ? 0x4000 : 0x8000;
    mixQuarterBytes = (waveFormat.nChannels == 2) ? mixHalfBytes >> 1 : mixHalfBytes;
    mixBufFrames = 0x400;
    mixBlockWords = 0x800;
    mixScratchBytes = 0x2000;
    mixResult = IDirectSoundBuffer_Lock(mixBuffer, 0, 0x8000, &ptr1, &bytes1, &ptr2, &len2, 0);
#endif
    if (mixResult == DS_OK) {
        if (waveFormat.wBitsPerSample == 16)
            memset(ptr1, 0, bytes1);
        else
            memset(ptr1, 0x80, bytes1);
        mixResult = IDirectSoundBuffer_Unlock(mixBuffer, ptr1, bytes1, ptr2, len2);
    }
    mixScratch = GlobalAlloc(GPTR, mixScratchBytes);
    if (!mixScratch) {
#ifdef NETPLAY
        IDirectSound_Release(sound);
        sound = 0;
#else
        IDirectSound_Release(currentSound);
#endif
        return 1;
    }
    sfxListHead = 0;
    sfxListTail = 0;
    for (n = 0; n < 128; n++) {
        sfxCache[n].length = 0;
        sfxCache[n].data = 0;
    }
    for (n = 0; n < 32; n++) {
        sfxSlots[n].sample = 0;
        sfxSlots[n].state = 0;
    }
    sfxMaxActive = 16;
    switch (mode) {
    case 1:
        sfxDoMix = SfxMixBasic;
        break;
    case 2:
        sfxDoMix = SfxMixerLut;
        break;
    case 3:
        sfxDoMix = MixStereoSfxSamples;
        break;
    case 4:
        sfxDoMix = SfxMixerLut;
        break;
    case 5:
        sfxDoMix = SfxMixFourChannel;
        break;
    }
    if (SfxBuildMixTables(sfxMixMode))
        return 1;
    if (waveFormat.wBitsPerSample == 16) {
        if (waveFormat.nChannels == 2)
            sfxDoMixCopy = PackMixStereo16;
        else
            sfxDoMixCopy = PackMixMono16;
    } else {
#ifdef NETPLAY
        if (waveFormat.nChannels == 2)
            sfxDoMixCopy = PackMixStereo8;
        else
            sfxDoMixCopy = PackMixMono8;
#else
        sfxDoMixCopy = PackMixStereo8;
        if (waveFormat.nChannels != 2)
            sfxDoMixCopy = PackMixMono8;
#endif
    }
    mixResult = IDirectSoundBuffer_Play(mixBuffer, 0, 0, DSBPLAY_LOOPING);
    sfxCallbackOn = 0;
    sfxListUpdate = 0;
    mixPos = 0;
    sfxActive = 1;
    return 0;
}

/* Initializes the low-level reverb buffers and settings. */
int SfxInitReverbLow(int type, u_int delay)
{
    int result;

    if (!sfxActive)
        return 1;
    if (delay > 2000)
        delay = 2000;
    result = InitReverb(type, waveFormat.nChannels, delay) & 0xffff;
    if (result != 0)
        return 1;
    reverbSet = 1;
    reverbOn = 0;
    return 0;
}

/* Enables reverb in the software mixer. */
void SfxReverbOnLow(void)
{
    if (reverbSet)
        reverbOn = 1;
}

/* Disables reverb in the software mixer. */
void SfxReverbOffLow(void)
{
    if (reverbOn) {
        reverbOn = 0;
        ClearReverbBuffers();
    }
}

/* Sets the software mixer's reverb depth. */
void SfxSetReverbDepthLow(int depth)
{
    if (reverbSet)
        SetReverbDepth(depth);
}

/* Releases the low-level reverb buffers. */
void SfxUnInitReverbLow(void)
{
    if (reverbSet) {
        reverbOn = 0;
        reverbSet = 0;
        UnInitReverb();
    }
}

/* Sets the maximum number of effects in the software mixer. */
u_int SfxSetMaxEffectsLow(u_int num)
{
    if (!sfxActive)
        return 0;
    if (num <= 16)
        sfxMaxActive = num;
    return sfxMaxActive;
}

/* Converts a 22 kHz 16-bit mono WAVE image to the mixer's sample format and puts it in the cache. */
int SfxSoftTransEffect(WavFile *left, WavFile *right, u_int flags)
{
    int i;
    u_int convFlags;
    u_int size;
    int rate;

    if (!sfxActive)
        return -1;
    i = 0;
    while (sfxCache[i].flags & 0x200) {
        i++;
        if (i > 128)
            return -5;
    }
    if (waveFormat.wBitsPerSample == 16)
        convFlags = 4;
    else
        convFlags = 2;
    rate = 11025;
    if (waveFormat.nSamplesPerSec == 11025) {
        size = left->dataSize >> 2;
        convFlags |= 1;
    } else {
        size = left->dataSize >> 1;
        rate = 22050;
    }
    sfxCache[i].data = GlobalAlloc(GPTR, size);
    if (!sfxCache[i].data)
        return -6;
    sfxCache[i].length = size;
    sfxCache[i].rate = rate;
    sfxCache[i].flags = 0x280;
    SfxConvertSamples(sfxCache[i].data, (short *)left->data, left->dataSize, convFlags);
    if (sfxMixMode == 4 && right) {
        sfxCache[i].rightData = GlobalAlloc(GPTR, size);
        if (!sfxCache[i].rightData)
            return i;
        SfxConvertSamples(sfxCache[i].rightData, (short *)right->data, right->dataSize, convFlags);
        sfxCache[i].flags |= 0x20;
    }
    return i;
}

/* Frees a loaded effect, stopping the voices that play it. */
#ifdef NETPLAY
int SfxSoftFreeEffect(u_int id)
{
#ifndef NETPLAY
    u_short flags;
    int i;
#else
    SfxSample *sample;
    u_int slotIndex;
#endif

#ifdef NETPLAY
    if (!sfxActive)
        return 1;
    if (!(sfxCache[id].flags & 0x200))
        return 1;
    if (!(sfxCache[id].flags & 0x80))
        return 1;
    for (slotIndex = 0; slotIndex < 32; slotIndex++) {
        if (sfxSlots[slotIndex].sample) {
            sample = sfxSlots[slotIndex].sample;
            if (sample->data == sfxCache[id].data) {
                sfxSlots[slotIndex].state = 4;
                sfxListUpdate++;
            }
        }
    }
    if (sfxCache[id].flags & 0x80) {
        GlobalFree(sfxCache[id].data);
        if (sfxCache[id].flags & 0x20)
            GlobalFree(sfxCache[id].rightData);
        sfxCache[id].flags = 0;
    }
    sfxCache[id].length = 0;
    sfxCache[id].data = 0;
    return 0;
#else
    if (!sfxActive)
        return 1;
    flags = sfxCache[id].flags;
    if (!(flags & 0x200))
        return 1;
    flags &= 0x80;
    if (!flags)
        return 1;
    for (i = 0; i < 32; i++) {
        if (sfxSlots[i].sample && sfxCache[id].data == sfxSlots[i].sample->data) {
            sfxSlots[i].state = 4;
            sfxListUpdate++;
        }
    }
    if (flags) {
        GlobalFree(sfxCache[id].data);
        if (sfxCache[id].flags & 0x20)
            GlobalFree(sfxCache[id].rightData);
        sfxCache[id].flags = 0;
    }
    sfxCache[id].length = 0;
    sfxCache[id].data = 0;
    return 0;
#endif
}
#else
int SfxSoftFreeEffect(u_int id)
{
#ifndef NETPLAY
    u_short flags;
    int i;
#else
    u_int i;
    SfxSample *s;
#endif

#ifdef NETPLAY
    if (!sfxActive)
        return 1;
    if (!(sfxCache[id].flags & 0x200))
        return 1;
    if (!(sfxCache[id].flags & 0x80))
        return 1;
    for (i = 0; i < 32; i++) {
        if (sfxSlots[i].sample) {
            s = sfxSlots[i].sample;
            if (s->data == sfxCache[id].data) {
                sfxSlots[i].state = 4;
                sfxListUpdate++;
            }
        }
    }
    if (sfxCache[id].flags & 0x80) {
        GlobalFree(sfxCache[id].data);
        if (sfxCache[id].flags & 0x20)
            GlobalFree(sfxCache[id].rightData);
        sfxCache[id].flags = 0;
    }
    sfxCache[id].length = 0;
    sfxCache[id].data = 0;
    return 0;
#else
    if (!sfxActive)
        return 1;
    flags = sfxCache[id].flags;
    if (!(flags & 0x200))
        return 1;
    flags &= 0x80;
    if (!flags)
        return 1;
    for (i = 0; i < 32; i++) {
        if (sfxSlots[i].sample && sfxCache[id].data == sfxSlots[i].sample->data) {
            sfxSlots[i].state = 4;
            sfxListUpdate++;
        }
    }
    if (flags) {
        GlobalFree(sfxCache[id].data);
        if (sfxCache[id].flags & 0x20)
            GlobalFree(sfxCache[id].rightData);
        sfxCache[id].flags = 0;
    }
    sfxCache[id].length = 0;
    sfxCache[id].data = 0;
    return 0;
#endif
}
#endif

/* Stores the callback used when a software effect finishes. */
void SfxSetCallBackLow(void (*callback)(u_int id))
{
    /* The empty arm preserves the executable's jump around the update block. */
    if (!sfxActive) {
    } else {
        sfxCallback = callback;
        sfxCallbackOn = 1;
    }
}

/* Starts a software-mixed effect with the requested settings. */
void SfxSoftPlay(int id, u_char volume, u_char lrPan, u_char fbPan, u_int pitch, u_char flags)
{
    int playIndex;
    SfxSample *sample;

    if (!sfxActive)
        return;
    if (id < 0 || id > 128)
        return;
    if (!sfxCache[id].data)
        return;
    playIndex = 0;
    while (sfxSlots[playIndex].sample) {
        playIndex++;
        if (playIndex == 32)
            return;
    }
    sample = GlobalAlloc(GPTR, sizeof(SfxSample));
    sample->flags = SFX_ACTIVE;
    if (flags & 2)
        sample->flags |= 0x10;
    if (flags & 4)
        sample->flags |= 0x40;
    if (flags & 8)
        sample->flags |= 0x400;
    sample->volume = volume >> 3;
    sample->pan = lrPan;
    sample->fbPan = fbPan;
    sample->pitch = pitch;
    sample->posFrac = 0;
    sample->rearPosFrac = 0;
    sample->data = sfxCache[id].data;
    sample->length = sfxCache[id].length;
    sample->offset = 0;
    sample->frontOffset = 0;
    sample->rate = sfxCache[id].rate;
    sample->next = 0;
    sample->id = playIndex;
    sample->prev = 0;
    sample->chain = 0;
    sample->rightData = 0;
    if (sfxCache[id].flags & 0x20) {
        sample->rightData = sfxCache[id].rightData;
        sample->flags |= 0x20;
    }
    sfxSlots[playIndex].sample = sample;
    sfxListUpdate++;
    sfxSlots[playIndex].state = 1;
}

/* Starts an effect and returns its allocated playback id. */
int SfxSoftPlayId(int id, u_char volume, u_char lrPan, u_char fbPan, u_int pitch, u_char flags)
{
    int playIndex;
    SfxSample *sample;

    if (!sfxActive || id < 0 || id > 128)
        return -1;
    if (!sfxCache[id].data)
        return -3;
    playIndex = 0;
    while (sfxSlots[playIndex].sample) {
        playIndex++;
        if (playIndex == 32)
            return -2;
    }
    sample = GlobalAlloc(GPTR, sizeof(SfxSample));
    sample->flags = SFX_ACTIVE | 4;
    if (flags & 1)
        sample->flags |= SFX_LOOP;
    if (flags & 2)
        sample->flags |= 0x10;
    if (flags & 4)
        sample->flags |= 0x40;
    if (flags & 8)
        sample->flags |= 0x400;
    sample->volume = volume >> 3;
    sample->pan = lrPan;
    sample->fbPan = fbPan;
    sample->pitch = pitch;
    sample->posFrac = 0;
    sample->rearPosFrac = 0;
    sample->data = sfxCache[id].data;
    sample->length = sfxCache[id].length;
    sample->rate = sfxCache[id].rate;
    sample->offset = 0;
    sample->id = playIndex;
    sample->frontOffset = 0;
    sample->next = 0;
    sample->prev = 0;
    sample->chain = 0;
    sample->rightData = 0;
    if (sfxCache[id].flags & 0x20) {
        sample->rightData = sfxCache[id].rightData;
        sample->flags |= 0x20;
    }
    sfxSlots[playIndex].sample = sample;
    sfxSlots[playIndex].state = 1;
    sfxListUpdate++;
    return playIndex;
}

/* Starts an effect after the previous playback id finishes. */
short SfxSoftChainId(int prev, int id, u_char volume, u_char lrPan, u_char fbPan, u_int pitch, u_char flags)
{
    int newSlot;
    SfxSample *previousSample;
    SfxSample *newSample;
#ifndef NETPLAY
    u_int inc;
#endif

    if (!sfxActive || id < 0 || id > 128)
        return -1;
    if (prev < 0 || prev > 32)
        return -3;
    if (!sfxSlots[prev].sample)
        return -3;
    if (!sfxCache[id].data)
        return -3;
    newSlot = 0;
    while (sfxSlots[newSlot].sample) {
        newSlot++;
        if (newSlot == 32)
            return -2;
    }
    lrPan = (lrPan >= 30) ? 30 : lrPan;
    fbPan = (fbPan >= 30) ? 30 : fbPan;
#ifdef NETPLAY
    if (pitch < 16)
        pitch = 16;
    else if (pitch >= 1024)
        pitch = 1024;
#else
    inc = pitch;
    if (inc < 16)
        inc = 16;
    else if (inc >= 1024)
        inc = 1024;
#endif
    newSample = GlobalAlloc(GPTR, sizeof(SfxSample));
    newSample->flags = SFX_ACTIVE | 4;
    if (flags & 1)
        newSample->flags |= SFX_LOOP;
    if (flags & 2)
        newSample->flags |= 0x10;
    if (flags & 4)
        newSample->flags |= 0x40;
    if (flags & 8)
        newSample->flags |= 0x400;
    newSample->volume = volume >> 3;
    newSample->pan = lrPan;
    newSample->fbPan = fbPan;
#ifdef NETPLAY
    newSample->pitch = pitch;
#else
    newSample->pitch = inc;
#endif
    newSample->posFrac = 0;
    newSample->rearPosFrac = 0;
    newSample->data = sfxCache[id].data;
    newSample->length = sfxCache[id].length;
    newSample->rate = sfxCache[id].rate;
    newSample->offset = 0;
    newSample->frontOffset = 0;
    newSample->id = newSlot;
    newSample->next = 0;
    newSample->prev = 0;
    newSample->chain = 0;
    newSample->rightData = 0;
    if (sfxCache[id].flags & 0x20) {
        newSample->rightData = sfxCache[id].rightData;
        newSample->flags |= 0x20;
    }
    previousSample = sfxSlots[prev].sample;
    previousSample->chain = newSample;
    previousSample->flags |= SFX_CHAINED;
#ifdef NETPLAY
    sfxSlots[newSlot].sample = newSample;
    sfxSlots[newSlot].state = 0;
    sfxListUpdate++;
#else
    sfxListUpdate++;
    sfxSlots[newSlot].sample = newSample;
    sfxSlots[newSlot].state = 0;
#endif
    return newSlot;
}

/* Stops the active playback identified by id. */
void SfxEffectKill(u_int id)
{
    /* The empty arms preserve the executable's jumps around the state update. */
    if (!sfxActive) {
    } else if (!sfxSlots[id].sample) {
    } else {
        sfxSlots[id].state = 4;
        sfxListUpdate++;
    }
}

/* Changes the volume of an active software effect. */
#ifdef EQUIV
void SfxEffectVolLow(u_int id, u_char volume)
{
    SfxSample *s;

    if (!sfxActive)
        return;
    if (!sfxSlots[id].sample)
        return;
    s = sfxSlots[id].sample;
    /* These empty arms preserve the executable's branch layout. */
    if (s->flags & 4) {
        if (s->flags & SFX_ACTIVE) {
            s->volume = volume >> 3;
        } else {
        }
    } else {
    }
}
#else
void SfxEffectVolLow(u_int id, u_char volume)
{
    SfxSample *s;

    if (sfxActive) {
        s = sfxSlots[id].sample;
        if (s) {
            if ((s->flags & 4) && (s->flags & SFX_ACTIVE))
                s->volume = volume >> 3;
        }
    }
}
#endif

#ifdef EQUIV
/* register choice differs */

/* Changes the pitch of an active software effect. */
#ifdef NETPLAY
void SfxEffectPitchLow(u_int id, u_int pitch)
{
    SfxSample *s;

    /* The empty arms preserve the executable's jumps around the pitch update. */
    if (!sfxActive) {
    } else if (!sfxSlots[id].sample) {
    } else {
        s = sfxSlots[id].sample;
        if (!(s->flags & 4)) {
        } else if (!(s->flags & SFX_ACTIVE)) {
        } else {
            if (pitch < 16) pitch = 16;
            else if (pitch >= 1024) pitch = 1024;
            s->pitch = pitch;
        }
    }
}
#else
void SfxEffectPitchLow(u_int id, u_int pitch)
{
    SfxSample *s;

    if (sfxActive) {
        s = sfxSlots[id].sample;
        if (s) {
            if ((s->flags & 4) && (s->flags & SFX_ACTIVE)) {
                if (pitch < 16) pitch = 16;
                else if (pitch >= 1024) pitch = 1024;
                s->pitch = pitch;
            }
        }
    }
}
#endif
#endif

/* Changes the left-right pan of an active software effect. */
#ifdef EQUIV
void SfxEffectLRPanLow(u_int id, u_char pan)
{
    SfxSample *s;

    /* The empty arms preserve the executable's jumps around the pan update. */
    if (!sfxActive) {
    } else if (!sfxSlots[id].sample) {
    } else {
        s = sfxSlots[id].sample;
        if (!(s->flags & 4)) {
        } else if (!(s->flags & SFX_ACTIVE)) {
        } else {
            if (pan >= 30)
                pan = 30;
            s->pan = pan;
        }
    }
}
#else
void SfxEffectLRPanLow(u_int id, u_char pan)
{
    SfxSample *s;

    if (sfxActive) {
        s = sfxSlots[id].sample;
        if (s) {
            if ((s->flags & 4) && (s->flags & SFX_ACTIVE)) {
                if (pan >= 30)
                    pan = 30;
                s->pan = pan;
            }
        }
    }
}
#endif

/* Changes the front-back pan of an active software effect. */
#ifdef EQUIV
void SfxEffectFBPanLow(u_int id, u_char pan)
{
    SfxSample *s;

    /* The empty arms preserve the executable's jumps around the pan update. */
    if (!sfxActive) {
    } else if (!sfxSlots[id].sample) {
    } else {
        s = sfxSlots[id].sample;
        if (!(s->flags & 4)) {
        } else if (!(s->flags & SFX_ACTIVE)) {
        } else {
            s->fbPan = pan;
        }
    }
}
#else
void SfxEffectFBPanLow(u_int id, u_char pan)
{
    SfxSample *s;

    if (sfxActive) {
        s = sfxSlots[id].sample;
        if (s) {
            if ((s->flags & 4) && (s->flags & SFX_ACTIVE))
                s->fbPan = pan;
        }
    }
}
#endif

/* Changes one playback flag for an active software effect. */
#ifdef EQUIV
void SfxEffectFlag(u_int id, u_short option, u_char set)
{
    SfxSample *s;
    u_short bit;

    /* The empty arms preserve the executable's jumps around the flag update. */
    if (!sfxActive) {
    } else if (!sfxSlots[id].sample) {
    } else {
        s = sfxSlots[id].sample;
        if (!(s->flags & 4)) {
        } else if (!(s->flags & SFX_ACTIVE)) {
        } else {
            switch (option) {
            case 1:
                bit = 2;
                break;
            case 2:
                bit = 0x10;
                break;
#ifdef NETPLAY
            case 4:
#else
            case 3:
#endif
                bit = 0x40;
                s->flags &= 0xbfff;
                break;
            case 8:
                bit = 0x400;
                break;
            default:
                return;
            }
            if (set == 0)
                s->flags &= ~bit;
            else
                s->flags |= bit;
        }
    }
}
#else
void SfxEffectFlag(u_int id, u_short option, u_char set)
{
    SfxSample *s;
    u_short bit;

    if (sfxActive) {
        s = sfxSlots[id].sample;
        if (s) {
            if ((s->flags & 4) && (s->flags & SFX_ACTIVE)) {
                switch (option) {
                case 1:
                    bit = 2;
                    break;
                case 2:
                    bit = 0x10;
                    break;
#ifdef NETPLAY
                case 4:
#else
                case 3:
#endif
                    bit = 0x40;
                    s->flags &= 0xbfff;
                    break;
                case 8:
                    bit = 0x400;
                    break;
                default:
                    return;
                }
                if (set == 0)
                    s->flags &= ~bit;
                else
                    s->flags |= bit;
            }
        }
    }
}
#endif

#ifndef NETPLAY
/* Stops all software effects and clears their playback records. */
void SfxFreeAll(void)
{
    int i;
    SfxSample *sfxSample;

    if (!sfxActive)
        return;
    for (i = 0; i < 32; i++) {
        if (sfxSlots[i].sample) {
            sfxSample = sfxSlots[i].sample;
            sfxSlots[i].sample = 0;
            sfxSlots[i].state = 0;
            RemoveItem(sfxSample);
            GlobalFree(sfxSample);
        }
    }
    sfxListHead = 0;
    sfxListTail = 0;
}
#endif
#ifdef EQUIV
#ifdef NETPLAY
/* NET-WOXL: checked in the EQUIV build, where SfxDoSfx precedes it (its RemoveItem call is a
   near call whose displacement depends on the code compiled before it). */
void SfxFreeAll(void)
{
    int i;
    SfxSample *sfxSample;

    if (!sfxActive)
        return;
    for (i = 0; i < 32; i++) {
        if (sfxSlots[i].sample) {
            sfxSample = sfxSlots[i].sample;
            sfxSlots[i].sample = 0;
            sfxSlots[i].state = 0;
            RemoveItem(sfxSample);
            GlobalFree(sfxSample);
        }
    }
    sfxListHead = 0;
    sfxListTail = 0;
}
#endif
#endif

/* Pauses the software mixer and optionally all active effects. */
#ifdef NETPLAY
void SfxPauseLow(u_char all)
{
    if (!sfxActive)
        return;
    if (all == 0) {
        sfxMuteState = 1;
        return;
    }
    sfxMuteState = 2;
    sfxIter = sfxListHead;
    while (sfxIter) {
        if (sfxIter->flags & SFX_LOOP) {
            savedVolume[sfxIter->id] = sfxIter->volume;
            sfxIter->volume = 0;
        }
        sfxIter = sfxIter->next;
    }
}
#else
void SfxPauseLow(u_char all)
{
    if (sfxActive) {
        if (all == 0) {
            sfxMuteState = 1;
            return;
        }
        sfxMuteState = 2;
        sfxIter = sfxListHead;
        while (sfxIter) {
            if (sfxIter->flags & SFX_LOOP) {
                savedVolume[sfxIter->id] = sfxIter->volume;
                sfxIter->volume = 0;
            }
            sfxIter = sfxIter->next;
        }
    }
}
#endif

/* Resumes software effects paused by SfxPauseLow. */
#ifdef NETPLAY
void SfxContinueLow(void)
{
    if (!sfxActive)
        return;
    if (sfxMuteState == 1) {
        sfxMuteState = 0;
        return;
    }
    if (sfxMuteState == 2) {
        sfxMuteState = 0;
        sfxIter2 = sfxListHead;
        while (sfxIter2) {
            if (sfxIter2->flags & SFX_LOOP)
                sfxIter2->volume = savedVolume[sfxIter2->id];
            sfxIter2 = sfxIter2->next;
        }
    }
}
#else
void SfxContinueLow(void)
{
    if (sfxActive) {
        if (sfxMuteState == 1) {
            sfxMuteState = 0;
            return;
        }
        if (sfxMuteState == 2) {
            sfxMuteState = 0;
            sfxIter2 = sfxListHead;
            while (sfxIter2) {
                if (sfxIter2->flags & SFX_LOOP)
                    sfxIter2->volume = savedVolume[sfxIter2->id];
                sfxIter2 = sfxIter2->next;
            }
        }
    }
}
#endif

#ifdef EQUIV
#ifdef NETPLAY
/* NET-WOXL: every DirectSound result goes through one local (hres) as in the original;
   the empty default: and the empty else after the sfxMaxActive test keep the jumps
   the original's switch and if/else emit. */
void SfxDoSfx(void)
{
    DWORD readPos;
    DWORD statusFlags;
    DWORD count1;
    u_int slot;
    DWORD count2;
    void *data1;
    DWORD writePosition;
    SfxSample *chainedSample;
    void *data2;
    HRESULT hres;
    u_int i;
    SfxSample *next;

    if (!sfxActive)
        return;
    hres = IDirectSoundBuffer_GetStatus(mixBuffer, &statusFlags);
    if (!(statusFlags & DSBSTATUS_PLAYING)) {
        hres = IDirectSoundBuffer_Restore(mixBuffer);
        mixResult = IDirectSoundBuffer_Play(mixBuffer, 0, 0, DSBPLAY_LOOPING);
    }
    readPos = writePosition = 0;
    hres = IDirectSoundBuffer_GetCurrentPosition(mixBuffer, &readPos, &writePosition);
    if ((writePosition + mixLeadBytes) % mixBufferBytes < writePosition) {
        if (mixPos < writePosition && (mixLeadBytes + writePosition) % mixBufferBytes < mixPos)
            return;
    } else {
        if (mixPos < writePosition || mixLeadBytes + writePosition < mixPos)
            return;
    }
    for (i = 0; i < 32; i++) {
        if (sfxSlots[i].sample && sfxSlots[i].state) {
            switch (sfxSlots[i].state) {
            case 1:
                AddItem(sfxSlots[i].sample);
                sfxSlots[i].state &= ~1;
                break;
            case 8:
                RemoveItem(sfxSlots[i].sample);
                sfxSlots[i].state &= ~8;
                break;
            case 4:
                if (sfxSlots[i].sample->flags & SFX_ACTIVE)
                    RemoveItem(sfxSlots[i].sample);
                GlobalFree(sfxSlots[i].sample);
                sfxSlots[i].sample = 0;
                sfxSlots[i].state &= ~4;
                break;
            default:
                ;   /* the original's empty default leaves the dead jmp after case 4 */
            }
            sfxListUpdate--;
        }
    }
    hres = IDirectSoundBuffer_Lock(mixBuffer, mixPos, mixLeadBytes, &data1, &count1, &data2, &count2, 0);
    if (hres != DS_OK)
        return;
    if (sfxListHead == 0) {
        if (reverbOn) {
            ClearMixScratch();
            AddReverb();
            decodeDst = data1;
            mixCount = count1;
            mixSrc = mixScratch;
            sfxDoMixCopy();
            mixPos += count1;
            if (mixPos >= mixBufferBytes)
                mixPos = 0;
            if (data2) {
                decodeDst = data2;
                mixCount = count2;
                mixSrc = mixScratch + count1;
                sfxDoMixCopy();
                mixPos = count2;
            }
        } else {
            mixClearDst = data1;
            mixCount = count1;
            mixClearBits = waveFormat.wBitsPerSample;
            ClearMixOutput();
            mixPos += count1;
            if (mixPos >= mixBufferBytes)
                mixPos = 0;
            if (data2) {
                mixClearDst = data2;
                mixCount = count2;
                mixClearBits = waveFormat.wBitsPerSample;
                ClearMixOutput();
                mixPos = count2;
            }
        }
        hres = IDirectSoundBuffer_Unlock(mixBuffer, data1, count1, data2, count2);
        return;
    }
    sfxItem = sfxListHead;
    sfxListUpdate = 0;
    if (sfxMaxActive > 0)
        sfxListUpdate = sfxDoMix(&sfxItem);
    else
        ;
    if (reverbOn)
        AddReverb();
    decodeDst = data1;
    mixCount = count1;
    mixSrc = mixScratch;
    sfxDoMixCopy();
    mixPos += count1;
    if (mixPos >= mixBufferBytes)
        mixPos = 0;
    if (data2) {
        decodeDst = data2;
        mixCount = count2;
        mixSrc = mixScratch + count1;
        sfxDoMixCopy();
        mixPos = count2;
    }
    hres = IDirectSoundBuffer_Unlock(mixBuffer, data1, count1, data2, count2);
    if (sfxCallbackOn) {
        sfxItem = sfxListHead;
        while (sfxItem) {
            if (sfxItem->flags & SFX_ENDED) {
                sfxCallback(sfxItem->id);
                sfxItem->flags &= ~SFX_ENDED;
            }
            sfxItem = sfxItem->next;
        }
    }
    sfxItem = sfxListHead;
    while (sfxListUpdate > 0) {
        next = sfxItem->next;
        if (!(sfxItem->flags & SFX_ACTIVE)) {
            if (sfxItem->flags & SFX_CHAINED) {
                chainedSample = sfxItem->chain;
                AddItem(chainedSample);
            }
            if (!(sfxItem->flags & 4)) {
                slot = sfxItem->id;
                RemoveItem(sfxItem);
                GlobalFree(sfxItem);
                sfxSlots[slot].sample = 0;
                sfxSlots[slot].state = 0;
            } else {
                RemoveItem(sfxItem);
            }
            sfxListUpdate--;
        }
        sfxItem = next;
    }
}
#else
/* Mixes the playing effects into the DirectSound buffer; the register allocation of the
   slot loop and the position test differ. */
void SfxDoSfx(void)
{
    DWORD status;
    DWORD playCursor;
    DWORD writeCursor;
    void *ptr1, *ptr2;
    DWORD bytes1, bytes2;
    u_int target;
    SfxSample *next;
    SfxSample *s;
    u_int id;
    int i;

    if (!sfxActive)
        return;
    IDirectSoundBuffer_GetStatus(mixBuffer, &status);
    if (!(status & DSBSTATUS_PLAYING)) {
        IDirectSoundBuffer_Restore(mixBuffer);
        mixResult = IDirectSoundBuffer_Play(mixBuffer, 0, 0, DSBPLAY_LOOPING);
    }
    writeCursor = 0;
    playCursor = 0;
    IDirectSoundBuffer_GetCurrentPosition(mixBuffer, &playCursor, &writeCursor);
    target = (writeCursor + mixLeadBytes) % mixBufferBytes;
    if (target < writeCursor) {
        if (mixPos < writeCursor && target < mixPos)
            return;
    } else {
        if (mixPos < writeCursor || writeCursor + mixLeadBytes < mixPos)
            return;
    }
    for (i = 0; i < 32; i++) {
        s = sfxSlots[i].sample;
        if (s && sfxSlots[i].state) {
            switch (sfxSlots[i].state) {
            case 1:
                AddItem(s);
                sfxSlots[i].state &= ~1;
                break;
            case 4:
                if (s->flags & SFX_ACTIVE)
                    RemoveItem(s);
                GlobalFree(sfxSlots[i].sample);
                sfxSlots[i].sample = 0;
                sfxSlots[i].state &= ~4;
                break;
            case 8:
                RemoveItem(sfxSlots[i].sample);
                sfxSlots[i].state &= ~8;
                break;
            }
            sfxListUpdate--;
        }
    }
    if (IDirectSoundBuffer_Lock(mixBuffer, mixPos, mixLeadBytes, &ptr1, &bytes1, &ptr2, &bytes2, 0) != DS_OK)
        return;
    if (sfxListHead == 0) {
        if (reverbOn) {
            ClearMixScratch();
            AddReverb();
            decodeDst = ptr1;
            mixSrc = mixScratch;
            mixCount = bytes1;
            sfxDoMixCopy();
            mixPos += bytes1;
            if (mixPos >= mixBufferBytes)
                mixPos = 0;
            if (ptr2) {
                decodeDst = ptr2;
                mixCount = bytes2;
                mixSrc = mixScratch + bytes1;
                sfxDoMixCopy();
                mixPos = bytes2;
            }
        } else {
            mixClearDst = ptr1;
            mixCount = bytes1;
            mixClearBits = waveFormat.wBitsPerSample;
            ClearMixOutput();
            mixPos += bytes1;
            if (mixPos >= mixBufferBytes)
                mixPos = 0;
            if (ptr2) {
                mixClearDst = ptr2;
                mixCount = bytes2;
                mixClearBits = waveFormat.wBitsPerSample;
                ClearMixOutput();
                mixPos = bytes2;
            }
        }
        IDirectSoundBuffer_Unlock(mixBuffer, ptr1, bytes1, ptr2, bytes2);
        return;
    }
    sfxItem = sfxListHead;
    sfxListUpdate = 0;
    if (sfxMaxActive)
        sfxListUpdate = sfxDoMix(&sfxItem);
    if (reverbOn)
        AddReverb();
    decodeDst = ptr1;
    mixSrc = mixScratch;
    mixCount = bytes1;
    sfxDoMixCopy();
    mixPos += bytes1;
    if (mixPos >= mixBufferBytes)
        mixPos = 0;
    if (ptr2) {
        decodeDst = ptr2;
        mixCount = bytes2;
        mixSrc = mixScratch + bytes1;
        sfxDoMixCopy();
        mixPos = bytes2;
    }
    IDirectSoundBuffer_Unlock(mixBuffer, ptr1, bytes1, ptr2, bytes2);
    if (sfxCallbackOn) {
        sfxItem = sfxListHead;
        while (sfxItem) {
            if (sfxItem->flags & SFX_ENDED) {
                sfxCallback(sfxItem->id);
                sfxItem->flags &= ~SFX_ENDED;
            }
            sfxItem = sfxItem->next;
        }
    }
    sfxItem = sfxListHead;
    while (sfxListUpdate > 0) {
        next = sfxItem->next;
        if (!(sfxItem->flags & SFX_ACTIVE)) {
            if (sfxItem->flags & SFX_CHAINED)
                AddItem(sfxItem->chain);
            if (!(sfxItem->flags & 4)) {
                id = sfxItem->id;
                RemoveItem(sfxItem);
                GlobalFree(sfxItem);
                sfxSlots[id].sample = 0;
                sfxSlots[id].state = 0;
            } else {
                RemoveItem(sfxItem);
            }
            sfxListUpdate--;
        }
        sfxItem = next;
    }
}
#endif
#endif

/* Frees every sample and the mixer buffers. */
void SfxUnInitLow(void)
{
#ifdef NETPLAY
    SfxSample *sample;
    int slotIndex;
    u_int cacheIndex;

    if (!sfxActive)
        return;
    sfxActive = 0;
    for (cacheIndex = 0; cacheIndex < (u_int)128; cacheIndex++) {
        if (sfxCache[cacheIndex].flags & 0x80) {
            GlobalFree(sfxCache[cacheIndex].data);
            if (sfxCache[cacheIndex].flags & 0x20)
                GlobalFree(sfxCache[cacheIndex].rightData);
        }
        sfxCache[cacheIndex].flags = 0;
    }
    for (slotIndex = 0; slotIndex < 32; slotIndex++) {
        if (sfxSlots[slotIndex].sample) {
            sample = sfxSlots[slotIndex].sample;
            sfxSlots[slotIndex].sample = 0;
            sfxSlots[slotIndex].state = 0;
            RemoveItem(sample);
            GlobalFree(sample);
        }
    }
    if (reverbSet) {
        reverbOn = 0;
        reverbSet = 0;
        UnInitReverb();
    }
    GlobalFree(mixScratch);
    SfxFreeMixTables();
#else
    SfxSample *s;
    int i;

    if (sfxActive) {
        sfxActive = 0;
        for (i = 0; i < 128; i++) {
            if (sfxCache[i].flags & 0x80) {
                GlobalFree(sfxCache[i].data);
                if (sfxCache[i].flags & 0x20)
                    GlobalFree(sfxCache[i].rightData);
            }
            sfxCache[i].flags = 0;
        }
        for (i = 0; i < 32; i++) {
            if (sfxSlots[i].sample) {
                s = sfxSlots[i].sample;
                sfxSlots[i].sample = 0;
                sfxSlots[i].state = 0;
                RemoveItem(s);
                GlobalFree(s);
            }
        }
        if (reverbSet) {
            reverbOn = 0;
            reverbSet = 0;
            UnInitReverb();
        }
        GlobalFree(mixScratch);
        SfxFreeMixTables();
    }
#endif
}

/* Puts a sample in the list, after those flagged 0x400 and before the first that is not. */
void AddItem(SfxSample *item)
{
    SfxSample *curr;
    SfxSample *old;
    int placed;

    placed = 0;
    if (sfxListHead == 0) {
        item->next = 0;
        item->prev = 0;
        sfxListHead = item;
        sfxListTail = item;
        placed = 1;
    } else {
        curr = sfxListHead;
        old = 0;
        while (curr && !placed) {
            if (curr->flags & 0x400) {
                old = curr;
                curr = curr->next;
            } else {
                if (curr->prev) {
                    curr->prev->next = item;
                    item->next = curr;
                    item->prev = curr->prev;
                    curr->prev = item;
                    placed = 1;
                } else {
                    item->next = curr;
                    item->prev = 0;
                    curr->prev = item;
                    sfxListHead = item;
                    placed = 1;
                }
            }
        }
        if (!placed) {
            old->next = item;
            item->next = 0;
            item->prev = old;
            sfxListTail = item;
        }
    }
}

#ifdef NETPLAY
/* Takes a sample out of the playing list. */
static void RemoveItem(SfxSample *item)
{
    if (item->prev) {
        item->prev->next = item->next;
    } else {
        sfxListHead = item->next;
        if (sfxListHead)
            sfxListHead->prev = 0;
    }
    if (item->next) {
        item->next->prev = item->prev;
    } else {
        sfxListTail = item->next;
        if (sfxListTail)
            sfxListTail->next = 0;
    }
    item->prev = 0;
    item->next = 0;
}
#endif

