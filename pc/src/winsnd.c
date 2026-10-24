#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifdef NETPLAY
/* The NET build expands these library calls inline. */
#pragma intrinsic(memset, log, pow)
#endif
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;

/* Effect ids: bit 0x400 marks a DirectSound buffer, the low byte is the slot. */
#define SFX_DIRECT_FLAG 0x400
#define SFX_INDEX_MASK 0xff


#include "pc_wave.h"
#include "pc_sound.h"

extern u_char sfxInit;
extern u_char sfxOn;
#include "sfx_started.h"
#include "sound_state.h"
extern u_char sfxDirectMix;
extern u_char sfxMixMode;
extern LPDIRECTSOUND lpDS;
extern LPDIRECTSOUNDBUFFER primaryBuffer;
extern DSCAPS dsCaps;
extern HRESULT dsResult;
extern UINT timerId;
extern int cdAuxDevice;
extern int sfxTimerBusy;

#include "pc_sfx.h"
#include "pc_sfxmix.h"
#include "pc_sfxds.h"

void CALLBACK SfxTimerProc(UINT id, UINT msg, DWORD user, DWORD dw1, DWORD dw2);

/* Opens DirectSound, creates the primary buffer and starts the mixer and the mixing timer. */
#ifdef NETPLAY
/* NET-WOXL: the CD aux-device search is FindCdAuxDevice, and the 16-bit test sets the format directly. */
int SfxInit(HWND hwnd, u_int mode, u_int rate)
{
    TIMECAPS timeCaps;
    u_int delay;
    u_int timerResolution;
    DSBUFFERDESC dsbDesc;
    DSBCAPS bufferCaps;

    sfxInit = 0;
    sfxStarted = 0;
    memset(&waveFormat, 0, sizeof(WAVEFORMATEX));
    if (mode < 1 || mode > 0x82)
        return 1;
    dsResult = DirectSoundCreate(NULL, &lpDS, NULL);
    if (dsResult != DS_OK)
        return 1;
    dsResult = IDirectSound_SetCooperativeLevel(lpDS, hwnd, DSSCL_EXCLUSIVE);
    if (dsResult != DS_OK) {
        IDirectSound_Release(lpDS);
        lpDS = NULL;
        return 1;
    }
    dsCaps.dwSize = sizeof(DSCAPS);
    dsResult = IDirectSound_GetCaps(lpDS, &dsCaps);
    if (dsCaps.dwFlags & DSCAPS_EMULDRIVER)
        mode |= 0x80;
    if (rate == 22050 || rate == 11025)
        waveFormat.nSamplesPerSec = rate;
    if (dsCaps.dwFlags & DSCAPS_PRIMARY16BIT)
        waveFormat.wBitsPerSample = 16;
    else
        waveFormat.wBitsPerSample = 8;
    if (mode & 0x80)
        sfxDirectMix = 1;
    else
        sfxDirectMix = 0;
    if (mode > 1 && (dsCaps.dwFlags & DSCAPS_PRIMARYSTEREO)) {
        waveFormat.nChannels = 2;
    } else {
        waveFormat.nChannels = 1;
        mode = (mode & 0xf0) | 1;
    }
    memset(&dsbDesc, 0, sizeof(DSBUFFERDESC));
    dsbDesc.dwSize = sizeof(DSBUFFERDESC);
    dsbDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
    dsbDesc.dwBufferBytes = 0;
    dsbDesc.lpwfxFormat = NULL;
    dsResult = IDirectSound_CreateSoundBuffer(lpDS, &dsbDesc, &primaryBuffer, NULL);
    if (dsResult != DS_OK) {
        IDirectSound_Release(lpDS);
        lpDS = NULL;
        return 1;
    }
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;
    dsResult = IDirectSoundBuffer_SetFormat(primaryBuffer, &waveFormat);
    dsResult = IDirectSoundBuffer_GetFormat(primaryBuffer, &waveFormat, sizeof(WAVEFORMATEX), NULL);
    bufferCaps.dwSize = sizeof(DSBCAPS);
    dsResult = IDirectSoundBuffer_GetCaps(primaryBuffer, &bufferCaps);
    sfxMixMode = mode;
    if (!sfxDirectMix)
        SfxSoftMixInit(lpDS, mode & 0xf, rate);
    SfxDirectInit(lpDS, mode & 0xf, rate);
    delay = 34;
    if (timeGetDevCaps(&timeCaps, sizeof(TIMECAPS)) != 0) {
        IDirectSound_Release(lpDS);
        lpDS = NULL;
        return 1;
    }
    timerResolution = 10;
    timerId = timeSetEvent(delay, timerResolution, SfxTimerProc, 0, TIME_PERIODIC);
    if (timerId == 0) {
        IDirectSound_Release(lpDS);
        lpDS = NULL;
        return 1;
    }
    cdAuxDevice = FindCdAuxDevice();
    sfxInit = 1;
    sfxStarted = 1;
    sfxOn = 0;
    return 0;
}
#else
int SfxInit(HWND hwnd, u_int mode, u_int rate)
{
    AUXCAPS aux;
    UINT numDevs;
    UINT i;
    DSBCAPS caps;
    TIMECAPS tc;
    DSBUFFERDESC desc;

    sfxInit = 0;
    sfxStarted = 0;
    memset(&waveFormat, 0, sizeof(WAVEFORMATEX));
    if (mode < 1 || mode > 0x82)
        return 1;
    dsResult = DirectSoundCreate(NULL, &lpDS, NULL);
    if (dsResult != DS_OK)
        return 1;
    dsResult = IDirectSound_SetCooperativeLevel(lpDS, hwnd, DSSCL_EXCLUSIVE);
    if (dsResult != DS_OK) {
        IDirectSound_Release(lpDS);
        lpDS = NULL;
        return 1;
    }
    dsCaps.dwSize = sizeof(DSCAPS);
    dsResult = IDirectSound_GetCaps(lpDS, &dsCaps);
    if (dsCaps.dwFlags & DSCAPS_EMULDRIVER)
        mode |= 0x80;
    if (rate == 22050 || rate == 11025)
        waveFormat.nSamplesPerSec = rate;
    waveFormat.wBitsPerSample = 16;
    if (!(dsCaps.dwFlags & DSCAPS_PRIMARY16BIT))
        waveFormat.wBitsPerSample = 8;
    if (mode & 0x80)
        sfxDirectMix = 1;
    else
        sfxDirectMix = 0;
    if (mode > 1 && (dsCaps.dwFlags & DSCAPS_PRIMARYSTEREO)) {
        waveFormat.nChannels = 2;
    } else {
        waveFormat.nChannels = 1;
        mode = (mode & 0xf0) | 1;
    }
    memset(&desc, 0, sizeof(DSBUFFERDESC));
    desc.dwBufferBytes = 0;
    desc.lpwfxFormat = NULL;
    desc.dwSize = sizeof(DSBUFFERDESC);
    desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
    dsResult = IDirectSound_CreateSoundBuffer(lpDS, &desc, &primaryBuffer, NULL);
    if (dsResult != DS_OK) {
        IDirectSound_Release(lpDS);
        lpDS = NULL;
        return 1;
    }
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;
    dsResult = IDirectSoundBuffer_SetFormat(primaryBuffer, &waveFormat);
    dsResult = IDirectSoundBuffer_GetFormat(primaryBuffer, &waveFormat, sizeof(WAVEFORMATEX), NULL);
    caps.dwSize = sizeof(DSBCAPS);
    dsResult = IDirectSoundBuffer_GetCaps(primaryBuffer, &caps);
    sfxMixMode = mode;
    if (!sfxDirectMix)
        SfxSoftMixInit(lpDS, mode & 0xf, rate);
    SfxDirectInit(lpDS, mode & 0xf, rate);
    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != 0) {
        IDirectSound_Release(lpDS);
        lpDS = NULL;
        return 1;
    }
    timerId = timeSetEvent(34, 10, SfxTimerProc, 0, TIME_PERIODIC);
    if (timerId == 0) {
        IDirectSound_Release(lpDS);
        lpDS = NULL;
        return 1;
    }
    numDevs = auxGetNumDevs();
    if (numDevs) {
        i = 0;
        do {
            auxGetDevCapsA(i, &aux, sizeof(AUXCAPS));
            if (aux.wTechnology & AUXCAPS_CDAUDIO) {
                cdAuxDevice = i;
                /* Skip the fallback so success initialization stays shared. */
                goto done;
            }
        } while (++i <= numDevs);
    }
    cdAuxDevice = -1;
done:
    sfxInit = 1;
    sfxStarted = 1;
    sfxOn = 0;
    return 0;
}
#endif

/* Creates a DirectSound buffer with the requested format and size. */
int SfxCreateBuffer(LPDIRECTSOUNDBUFFER *buffer, u_int bytes, WAVEFORMATEX *format)
{
    DSBUFFERDESC desc;

    memset(&desc, 0, sizeof(DSBUFFERDESC));
    desc.dwSize = sizeof(DSBUFFERDESC);
    desc.dwFlags = DSBCAPS_CTRLDEFAULT;
    desc.dwBufferBytes = bytes;
    desc.lpwfxFormat = format;
    dsResult = IDirectSound_CreateSoundBuffer(lpDS, &desc, buffer, NULL);
#ifdef NETPLAY
    if (dsResult != DS_OK)
        return 1;
    return 0;
#else
    return dsResult != DS_OK;
#endif
}

/* Initializes the reverb used by the active sound backend. */
int SfxInitReverb(int type, u_int delay)
{
    SfxInitReverbLow(type, delay);
    return 0;
}

/* Enables reverb in the active sound backend. */
void SfxReverbOn(void)
{
    SfxReverbOnLow();
}

/* Disables reverb in the active sound backend. */
void SfxReverbOff(void)
{
    SfxReverbOffLow();
}

/* Sets the reverb depth in the active sound backend. */
void SfxSetReverbDepth(int depth)
{
    SfxSetReverbDepthLow(depth);
}

/* Releases reverb state in the active sound backend. */
void SfxUnInitReverb(void)
{
    SfxUnInitReverbLow();
}

/* Sets the maximum number of simultaneous effects. */
u_int SfxSetMaxEffects(u_int num)
{
    return (u_short)SfxSetMaxEffectsLow(num);
}

/* Loads a WAVE image; returns the sample id, or a negative error. Bit 0x400 marks the DirectSound buffer path. */
int SfxTransEffect(WavFile *left, WavFile *right, u_int flags)
{
#ifdef NETPLAY
    int sfxId;
    WavFile *sample;
    WavFile *r;

    if (!sfxInit)
        return -1;
    if (!left)
        return -2;
    sample = left;
    r = 0;
    if (sample->wave != mmioFOURCC('W', 'A', 'V', 'E'))
        return -3;
    if (sample->channels != 1)
        return -4;
    if (sample->sampleRate != 22050)
        return -4;
    if (sample->bits != 16)
        return -4;
    if (sfxMixMode == 4 && right) {
        r = right;
        if (r->wave != mmioFOURCC('W', 'A', 'V', 'E'))
            r = 0;
        if (r->channels != 1)
            r = 0;
        if (r->sampleRate != 22050)
            r = 0;
        if (r->bits != 16)
            r = 0;
        if (r->dataSize != sample->dataSize)
            r = 0;
    }
    if (sfxDirectMix || (flags & 2)) {
        sfxId = (short)SfxDirectLoadSample(sample, flags);
        sfxId |= SFX_DIRECT_FLAG;
    } else if (SfxHwMemAvailable(sample->dataSize) & flags & 1) {
        sfxId = (short)SfxDirectLoadSample(sample, flags);
        sfxId |= SFX_DIRECT_FLAG;
    } else {
        sfxId = (short)SfxSoftTransEffect(sample, r, flags);
    }
    return sfxId;
#else
    WavFile *r;
    u_int size;

    if (!sfxInit)
        return -1;
    if (!left)
        return -2;
    r = 0;
    if (left->wave != mmioFOURCC('W', 'A', 'V', 'E'))
        return -3;
    if (left->channels != 1)
        return -4;
    if (left->sampleRate != 22050)
        return -4;
    if (left->bits != 16)
        return -4;
    if (sfxMixMode == 4 && right) {
        r = right;
        if (r->wave != mmioFOURCC('W', 'A', 'V', 'E'))
            r = 0;
        if (r->channels != 1)
            r = 0;
        if (r->sampleRate != 22050)
            r = 0;
        if (r->bits != 16)
            r = 0;
        if (left->dataSize != r->dataSize)
            r = 0;
    }
    if (!sfxDirectMix && !(flags & 2)) {
        size = left->dataSize;
        dsResult = IDirectSound_GetCaps(lpDS, &dsCaps);
        if (dsCaps.dwFreeHwMixingStaticBuffers && size <= dsCaps.dwFreeHwMemBytes && (flags & 1)) {
            int id = (short)SfxDirectLoadSample(left, flags);
            return id | SFX_DIRECT_FLAG;
        }
        return (short)SfxSoftTransEffect(left, r, flags);
    }
    {
        int id = (short)SfxDirectLoadSample(left, flags);
        return id | SFX_DIRECT_FLAG;
    }
#endif
}


/* Releases the sample buffers owned by the effect id. */
int SfxFreeEffect(u_int id)
{
    if (!sfxInit)
        return 1;
    /* The original exposes only the low 16 bits of the free-effect result. */
    if (id & SFX_DIRECT_FLAG)
        return (u_short)SfxDirectFreeEffect(id & SFX_INDEX_MASK);
#ifdef NETPLAY
    else
#endif
        return (u_short)SfxSoftFreeEffect(id & SFX_INDEX_MASK);
}

/* Sets the volume of the sound effects channel. */
void SfxSetVolume(u_char volume)
{
    if (sfxInit) {
        if (volume == 0)
            dsResult = IDirectSoundBuffer_SetVolume(primaryBuffer, -10000);
        else
            dsResult = IDirectSoundBuffer_SetVolume(primaryBuffer, (volume * 3 - 765) * 2);
    }
}

/* Sets the volume of the CD audio channel. */
void SfxSetCdVolume(u_char volume)
{
    long level;

    if (cdAuxDevice < 0)
        return;
    level = (long)((log((u_int)volume / 255.0 + 1.0) * 65535.0) / log(2.0));
    level |= level << 16;
    auxSetVolume(cdAuxDevice, level);
}

/* Returns the current CD audio volume. */
u_char SfxGetCdVolume(void)
{
    DWORD volume;

    if (cdAuxDevice < 0)
        return 255;
    auxGetVolume(cdAuxDevice, &volume);
    return (long)((pow(2.71, log(2.0) * (volume & 0xffff) / 65535.0) - 1.0) * 255.0);
}

/* Stores the callback invoked when an effect finishes. */
void SfxSetCallBack(void (*callback)(u_int id))
{
#ifdef NETPLAY
    if (!sfxInit)
        return;
    SfxSetCallBackLow(callback);
    SfxDirectSetCallback(callback);
#else
    if (sfxInit) {
        SfxSetCallBackLow(callback);
        SfxDirectSetCallback(callback);
    }
#endif
}

#ifdef NETPLAY
/* Clamps playback settings and selects the sound backend. */
void SfxPlay(u_int id, u_char volume, u_char lrPan, u_char fbPan, u_int pitch, int flags)
{
    if (!sfxOn || !sfxInit)
        return;
    lrPan = lrPan < 30 ? lrPan : 30;
    fbPan = fbPan < 30 ? fbPan : 30;
    if (pitch < 16)
        pitch = 16;
    else
        pitch = pitch < 1024 ? pitch : 1024;
    if (id & SFX_DIRECT_FLAG)
        SfxDirectPlay(id & SFX_INDEX_MASK, volume, lrPan, fbPan, pitch, flags);
    else
        SfxSoftPlay(id & SFX_INDEX_MASK, volume, lrPan, fbPan, pitch, flags);
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* EQUIV: the executable loads id into a register before the pitch clamp; this version tests the
   id argument in memory for the direct/soft choice. */
void SfxPlay(u_int id, u_char volume, u_char lrPan, u_char fbPan, u_int pitch, int flags)
{
    if (sfxOn && sfxInit) {
        if (lrPan >= 30)
            lrPan = 30;
        if (fbPan >= 30)
            fbPan = 30;
        if (pitch < 16)
            pitch = 16;
        else if (pitch >= 1024)
            pitch = 1024;
        if (id & SFX_DIRECT_FLAG)
            SfxDirectPlay(id & SFX_INDEX_MASK, volume, lrPan, fbPan, pitch, flags);
        else
            SfxSoftPlay(id & SFX_INDEX_MASK, volume, lrPan, fbPan, pitch, flags);
    }
}
#endif
#endif

#ifdef NETPLAY
/* Starts an effect and returns its voice id, including the backend flag. */
int SfxPlayId(u_int id, u_char volume, u_char lrPan, u_char fbPan, u_int pitch, int flags)
{
    int result;

    if (!sfxOn || !sfxInit)
        return -1;
    lrPan = lrPan < 30 ? lrPan : 30;
    fbPan = fbPan < 30 ? fbPan : 30;
    if (pitch < 16)
        pitch = 16;
    else
        pitch = pitch < 1024 ? pitch : 1024;
    if (id & SFX_DIRECT_FLAG) {
        result = (short)SfxDirectPlayId(id & SFX_INDEX_MASK, volume, lrPan, fbPan, pitch, flags);
        result |= SFX_DIRECT_FLAG;
    } else {
        result = (short)SfxSoftPlayId(id & SFX_INDEX_MASK, volume, lrPan, fbPan, pitch, flags);
    }
    return result;
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* Matches the executable up to register allocation (EQUIV). */
int SfxPlayId(u_int id, u_char volume, u_char lrPan, u_char fbPan, u_int pitch, int flags)
{
    int result;

    if (sfxOn && sfxInit) {
        if (lrPan >= 30)
            lrPan = 30;
        if (fbPan >= 30)
            fbPan = 30;
        if (pitch < 16)
            pitch = 16;
        else if (pitch >= 1024)
            pitch = 1024;
        if (id & SFX_DIRECT_FLAG) {
            result = (short)SfxDirectPlayId(id & SFX_INDEX_MASK, volume, lrPan, fbPan, pitch, flags);
            return result | SFX_DIRECT_FLAG;
        }
        result = (short)SfxSoftPlayId(id & SFX_INDEX_MASK, volume, lrPan, fbPan, pitch, flags);
        return result;
    }
    return -1;
}
#endif
#endif

/* Stops the effect identified by id. */
void SfxStop(int id)
{
#ifdef NETPLAY
    if (!sfxInit || id < 0)
        return;
    if (id & SFX_DIRECT_FLAG)
        SfxDirectStop(id & SFX_INDEX_MASK);
    else
        SfxEffectKill(id & SFX_INDEX_MASK);
#else
    if (sfxInit) {
        if (id >= 0) {
            if (id & SFX_DIRECT_FLAG)
                SfxDirectStop(id & SFX_INDEX_MASK);
            else
                SfxEffectKill(id & SFX_INDEX_MASK);
        }
    }
#endif
}

/* Changes the volume of an active effect. */
void SfxEffectVol(int id, u_char volume)
{
#ifdef NETPLAY
    if (!sfxInit || id < 0)
        return;
    if (id & SFX_DIRECT_FLAG) {
        SfxDirectSetVolume(id & SFX_INDEX_MASK, volume);
        return;
    }
    SfxEffectVolLow(id, volume);
#else
    if (sfxInit) {
        if (id >= 0) {
            if (id & SFX_DIRECT_FLAG)
                SfxDirectSetVolume(id & SFX_INDEX_MASK, volume);
            else
                SfxEffectVolLow(id, volume);
        }
    }
#endif
}

/* Changes the pitch of an active effect. */
void SfxEffectPitch(int id, u_int pitch)
{
#ifdef NETPLAY
    if (!sfxInit || id < 0)
        return;
    if (id & SFX_DIRECT_FLAG) {
        SfxDirectSetPitch(id & SFX_INDEX_MASK, pitch);
        return;
    }
    SfxEffectPitchLow(id & SFX_INDEX_MASK, pitch);
#else
    if (sfxInit) {
        if (id >= 0) {
            if (id & SFX_DIRECT_FLAG)
                SfxDirectSetPitch(id & SFX_INDEX_MASK, pitch);
            else
                SfxEffectPitchLow(id & SFX_INDEX_MASK, pitch);
        }
    }
#endif
}

/* Changes the left-right pan of an active effect. */
void SfxEffectLRPan(int id, u_int pan)
{
#ifdef NETPLAY
    if (!soundEnabled)
        return;
    if (!sfxInit || id < 0)
        return;
    if (id & SFX_DIRECT_FLAG)
        SfxDirectSetPan(id & SFX_INDEX_MASK, pan);
    else
        SfxEffectLRPanLow(id & SFX_INDEX_MASK, pan);
#else
    if (soundEnabled && sfxInit) {
        if (id >= 0) {
            if (id & SFX_DIRECT_FLAG)
                SfxDirectSetPan(id & SFX_INDEX_MASK, pan);
            else
                SfxEffectLRPanLow(id & SFX_INDEX_MASK, pan);
        }
    }
#endif
}

/* Changes the front-back pan of an active effect. */
void SfxEffectFBPan(int id, u_char pan)
{
#ifdef NETPLAY
    if (!soundEnabled)
        return;
    if (!sfxInit || id < 0)
        return;
    pan = pan < 30 ? pan : 30;
    if (id & SFX_DIRECT_FLAG)
        /* The original front/back pan path calls the pitch setter. */
        SfxDirectSetPitch(id & SFX_INDEX_MASK, pan);
    else
        SfxEffectFBPanLow(id & SFX_INDEX_MASK, pan);
#else
    if (soundEnabled && sfxInit) {
        if (id >= 0) {
            if (pan >= 30)
                pan = 30;
            if (id & SFX_DIRECT_FLAG)
                /* The original front/back pan path calls the pitch setter. */
                SfxDirectSetPitch(id & SFX_INDEX_MASK, pan);
            else
                SfxEffectFBPanLow(id & SFX_INDEX_MASK, pan);
        }
    }
#endif
}

#ifdef NETPLAY
/* Updates an active effect's backend flags. */
void SfxEffectFlagSet(int id, u_int option, u_int set)
{
    if (!sfxInit || id < 0)
        return;
    if (id & SFX_DIRECT_FLAG) {
        SfxDirectSetFlag(id & SFX_INDEX_MASK, option, set);
        return;
    }
    SfxEffectFlag(id & SFX_INDEX_MASK, option, set);
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* Matches the executable up to register allocation (EQUIV). */
void SfxEffectFlagSet(int id, u_int option, u_int set)
{
    if (sfxInit) {
        if (id >= 0) {
            if (id & SFX_DIRECT_FLAG)
                SfxDirectSetFlag(id & SFX_INDEX_MASK, option, set);
            else
                SfxEffectFlag(id & SFX_INDEX_MASK, option, set);
        }
    }
}
#endif
#endif

/* Enables sound effects after sound initialization. */
void SfxOn(void)
{
    if (sfxInit)
        sfxOn = 1;
}

/* Disables sound effects and stops active playback. */
void SfxOff(void)
{
    sfxOn = 0;
    SfxFreeAll();
    SfxDirectOff();
}

/* Empty compatibility stub for the DirectSound backend. */
void SfxBackendNoOpA(void)
{
}

/* Empty compatibility stub for the DirectSound backend. */
void SfxBackendNoOpB(void)
{
}

/* Called every 34 ms by the multimedia timer. */
#ifdef NETPLAY
void func_432273(void) {}
void func_43227e(void) {}
#endif

void CALLBACK SfxTimerProc(UINT id, UINT msg, DWORD user, DWORD dw1, DWORD dw2)
{
    if (sfxTimerBusy == 1)
        return;
#ifdef NETPLAY
    else
#endif
        sfxTimerBusy = 1;
    if (!sfxInit || !sfxOn) {
        sfxTimerBusy = 0;
        return;
    }
    SfxDoSfx();
    SfxDirectService();
    sfxTimerBusy = 0;
}

/* Stops the timer and releases sound backend resources. */
void SfxUnInit(void)
{
#ifdef NETPLAY
    if (!sfxInit)
        return;
    sfxInit = 0;
    sfxOn = 0;
    SfxUnInitLow();
    SfxDirectUnInit();
    if (timerId)
        timeKillEvent(timerId);
    timerId = 0;
    IDirectSound_Release(lpDS);
    lpDS = 0;
#else
    if (sfxInit) {
        sfxInit = 0;
        sfxOn = 0;
        SfxUnInitLow();
        SfxDirectUnInit();
        if (timerId)
            timeKillEvent(timerId);
        timerId = 0;
        IDirectSound_Release(lpDS);
        lpDS = 0;
    }
#endif
}

/* Loads a whole file into a new global block; returns 0 if it cannot be opened or allocated. */
#ifdef NETPLAY
long SfxFileLength(FILE *fp);
#endif
#ifdef NETPLAY
void *LoadFileAlloc(char *name)
{
    void *fileData;
    long length;
    FILE *fp;

    fp = fopen(name, "rb");
    if (!fp)
        return 0;
    length = SfxFileLength(fp);
    fileData = GlobalAlloc(GPTR, length);
    if (!fileData) {
        fclose(fp);
        return 0;
    }
    fread(fileData, 1, length, fp);
    fclose(fp);
    return fileData;
}
#else
void *LoadFileAlloc(char *name)
{
    FILE *fp;
    long start;
    long size;
    void *buffer;

    fp = fopen(name, "rb");
    if (!fp)
        return 0;
    start = ftell(fp);
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, start, SEEK_SET);
    buffer = GlobalAlloc(GPTR, size);
    if (!buffer) {
        fclose(fp);
        return 0;
    }
    fread(buffer, 1, size, fp);
    fclose(fp);
    return buffer;
}
#endif

#ifdef NETPLAY
/* Returns the file length and restores the stream to its original position. */
long SfxFileLength(FILE *fp)
{
    long length;
    long position;

    position = ftell(fp);
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, position, SEEK_SET);
    return length;
}
#endif

/* Finds the auxiliary device used to control CD audio volume. */
#ifdef NETPLAY
int FindCdAuxDevice(void)
{
    UINT count;
    UINT deviceIndex;
    AUXCAPS capabilities;

    count = auxGetNumDevs();
    if (count == 0)
        return -1;
    for (deviceIndex = 0; deviceIndex <= count; deviceIndex++) {
        auxGetDevCapsA(deviceIndex, &capabilities, sizeof(AUXCAPS));
        if (capabilities.wTechnology & AUXCAPS_CDAUDIO)
            return deviceIndex;
    }
    return -1;
}
#else
int FindCdAuxDevice(void)
{
    UINT numDevs;
    UINT i;
    AUXCAPS caps;

    numDevs = auxGetNumDevs();
    if (numDevs == 0)
        return -1;
    for (i = 0; i <= numDevs; i++) {
        auxGetDevCapsA(i, &caps, sizeof(AUXCAPS));
        if (caps.wTechnology & AUXCAPS_CDAUDIO)
            return i;
    }
    return -1;
}
#endif

/* Reports whether DirectSound can allocate a buffer of the requested size. */
int SfxHwMemAvailable(u_int size)
{
    dsResult = IDirectSound_GetCaps(lpDS, &dsCaps);
#ifdef NETPLAY
    if (dsCaps.dwFreeHwMixingStaticBuffers > 0 && dsCaps.dwFreeHwMemBytes >= size)
#else
    if (dsCaps.dwFreeHwMixingStaticBuffers && dsCaps.dwFreeHwMemBytes >= size)
#endif
        return 1;
    return 0;
}

