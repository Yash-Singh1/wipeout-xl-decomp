#include <math.h>
#include <string.h>

#ifdef NETPLAY
#pragma intrinsic(log10)
#endif

#include "pc_sfxds_types.h"
#include "pc_sfxds.h"
#include "pc_sfx.h"
#include "pc_wave.h"

#ifdef NETPLAY
int SfxDirectFindFreeSlot(void);
#endif

#ifdef NETPLAY
/* The NET-WOXL build returns nothing (its callers ignore the device; the D3D build returns it). */
void SfxDirectInit(LPDIRECTSOUND ds, u_int mode, u_int rate)
{
    int logValue;
    u_int sampleNo;
    u_int panPosition;
    u_int j;
    u_int volumeIndex;

    sfxSoundReady = 0;
    for (sampleNo = 0; sampleNo < 128; sampleNo++) {
        for (j = 0; j < 8; j++)
            dsSoundSamples[sampleNo].buffers[j] = 0;
        dsSoundSamples[sampleNo].numBuffers = 0;
        dsSoundSamples[sampleNo].channelMask = 0;
    }
    for (volumeIndex = 0; volumeIndex <= 255; volumeIndex++)
        sfxVolumeTable[volumeIndex] = (int)((log10(volumeIndex + 1) * 5000.0) / log10(256.0) - 5000.0);
    for (panPosition = 0; panPosition < 16; panPosition++) {
        logValue = (int)((log10(16 - panPosition) * 5000.0) / log10(16.0) - 5000.0);
        sfxPanTable[15 - panPosition] = logValue;
        sfxPanTable[15 + panPosition] = -logValue;
    }
    dsSoundDevice = ds;
    sfxSoundReady = 1;
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* Clears the DirectSound sample table, builds the volume and pan tables, keeps the
   device for the sample loader and marks the direct backend ready.
   Equivalent: only the two register moves that initialise the pan cursors differ. */
LPDIRECTSOUND SfxDirectInit(LPDIRECTSOUND ds, u_int mode, u_int rate)
{
    u_int i;
    int value;

    for (i = 0; i < 128; i++) {
        memset(dsSoundSamples[i].buffers, 0, sizeof(dsSoundSamples[i].buffers));
        dsSoundSamples[i].numBuffers = 0;
        dsSoundSamples[i].channelMask = 0;
    }
    for (i = 0; i <= 255; i++)
        sfxVolumeTable[i] = (int)((log10(i + 1) * 5000.0) / log10(256.0) - 5000.0);
    {
        u_int panIndex;

        /* The original loop also writes index 30, just past the pan table. */
        for (panIndex = 0; panIndex < 16; panIndex++) {
            value = (int)((log10(16 - panIndex) * 5000.0) / log10(16.0) - 5000.0);
            sfxPanTable[15 - panIndex] = value;
            sfxPanTable[15 + panIndex] = -value;
        }
    }
    sfxSoundReady = 1;
    dsSoundDevice = ds;
    return ds;
}
#endif
#endif

#ifdef NETPLAY
/* Creates a static DirectSound buffer for a 22050 Hz mono WAVE image and fills it.
   Returns the sample slot, with bit 0x800 set when the buffer lives in hardware. */
int SfxDirectLoadSample(WavFile *wav, u_int flags)
{
    u_int size;
    void *data1;
    WAVEFORMATEX format;
    DWORD length2;
    void *data2;
    DWORD size1;
    WavFile *src;
    u_int conversion;
    LPDIRECTSOUNDBUFFER buffer;
    int index;
    DSBCAPS bufferCapabilities;
    DSBUFFERDESC bufDesc;

    if (!sfxSoundReady)
        return -1;
    src = wav;
    index = 0;
    while (dsSoundSamples[index].buffers[0]) {
        if (++index == 128)
            return -3;
    }
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    conversion = 0;
    size = src->dataSize;
    if (waveFormat.wBitsPerSample != 16) {
        conversion |= 2;
        size >>= 1;
        format.wBitsPerSample = 8;
    } else {
        format.wBitsPerSample = 16;
    }
    if (waveFormat.nSamplesPerSec == 11025) {
        size >>= 1;
        conversion |= 1;
        format.nSamplesPerSec = 11025;
    } else {
        size = src->dataSize;
        format.nSamplesPerSec = 22050;
    }
    format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
    format.nAvgBytesPerSec = format.nBlockAlign * format.nSamplesPerSec;
    memset(&bufDesc, 0, sizeof(DSBUFFERDESC));
    bufDesc.dwSize = sizeof(DSBUFFERDESC);
    bufDesc.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME;
    if (waveFormat.nChannels == 2)
        bufDesc.dwFlags |= DSBCAPS_CTRLPAN;
    bufDesc.dwBufferBytes = size;
    bufDesc.lpwfxFormat = &format;
    dsBufferResult = IDirectSound_CreateSoundBuffer(dsSoundDevice, &bufDesc, &dsSoundSamples[index].buffers[0], NULL);
    if (dsBufferResult != DS_OK)
        return -1;
    dsSoundSamples[index].numBuffers = 1;
    buffer = dsSoundSamples[index].buffers[0];
    dsSoundSamples[index].format = format;
    dsBufferResult = IDirectSoundBuffer_Lock(buffer, 0, size, &data1, &size1, &data2, &length2, 0);
    if (dsBufferResult != DS_OK) {
        dsSoundSamples[index].buffers[0] = NULL;
        return -1;
    }
    SfxConvertSamples(data1, (short *)src->data, src->dataSize, conversion);
    dsBufferResult = IDirectSoundBuffer_Unlock(buffer, data1, size1, data2, length2);
    bufferCapabilities.dwSize = sizeof(DSBCAPS);
    dsBufferResult = IDirectSoundBuffer_GetCaps(buffer, &bufferCapabilities);
    if (bufferCapabilities.dwFlags & DSBCAPS_LOCHARDWARE)
        index |= 0x800;
    return index;
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* Equivalent: the exe loads the byte count once, before the bit-depth test, and this build loads
   it in each branch; the 22050 path reloads wav->dataSize from the parameter, as the exe does. */
/* Creates a static DirectSound buffer for a 22050 Hz mono WAVE image and fills it.
   Returns the sample slot, with bit 0x800 set when the buffer lives in hardware. */
int SfxDirectLoadSample(WavFile *wav, u_int flags)
{
    int index;
    u_int convertFlags;
    u_int bytes;
    WAVEFORMATEX format;
    DSBUFFERDESC desc;
    DSBCAPS caps;
    LPDIRECTSOUNDBUFFER buffer;
    void *ptr1;
    void *ptr2;
    DWORD len1;
    DWORD len2;

    if (!sfxSoundReady)
        return -1;
    index = 0;
    while (dsSoundSamples[index].buffers[0]) {
        if (++index == 128)
            return -3;
    }
    convertFlags = 0;
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    if (waveFormat.wBitsPerSample != 16) {
        bytes = wav->dataSize >> 1;
        convertFlags = 2;
        format.wBitsPerSample = 8;
    } else {
        bytes = wav->dataSize;
        format.wBitsPerSample = 16;
    }
    if (waveFormat.nSamplesPerSec == 11025) {
        bytes >>= 1;
        convertFlags |= 1;
        format.nSamplesPerSec = 11025;
    } else {
        bytes = wav->dataSize;
        format.nSamplesPerSec = 22050;
    }
    format.nBlockAlign = format.wBitsPerSample >> 3;
    format.nAvgBytesPerSec = format.nBlockAlign * format.nSamplesPerSec;
    memset(&desc, 0, sizeof(DSBUFFERDESC));
    desc.dwSize = sizeof(DSBUFFERDESC);
    desc.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME;
    if (waveFormat.nChannels == 2)
        desc.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN;
    desc.dwBufferBytes = bytes;
    desc.lpwfxFormat = &format;
    dsBufferResult = IDirectSound_CreateSoundBuffer(dsSoundDevice, &desc, &dsSoundSamples[index].buffers[0], NULL);
    if (dsBufferResult != DS_OK)
        return -1;
    dsSoundSamples[index].numBuffers = 1;
    buffer = dsSoundSamples[index].buffers[0];
    dsSoundSamples[index].format = format;
    dsBufferResult = IDirectSoundBuffer_Lock(buffer, 0, bytes, &ptr1, &len1, &ptr2, &len2, 0);
    if (dsBufferResult != DS_OK) {
        dsSoundSamples[index].buffers[0] = NULL;
        return -1;
    }
    SfxConvertSamples(ptr1, (short *)wav->data, wav->dataSize, convertFlags);
    dsBufferResult = IDirectSoundBuffer_Unlock(buffer, ptr1, len1, ptr2, len2);
    caps.dwSize = sizeof(DSBCAPS);
    dsBufferResult = IDirectSoundBuffer_GetCaps(buffer, &caps);
    if (caps.dwFlags & DSBCAPS_LOCHARDWARE)
        index |= 0x800;
    return index;
}
#endif
#endif

/* Releases the DirectSound buffers owned by a sample slot. */
int SfxDirectFreeEffect(int id)
{
#ifdef NETPLAY
    LPDIRECTSOUNDBUFFER voiceBuffer;
    DWORD playState;
    int slot;
    u_int bufferIndex;

    if (!sfxSoundReady || id < 0)
        return -1;
    if (!dsSoundSamples[id].buffers[0])
        return -1;
    for (bufferIndex = 0; bufferIndex < dsSoundSamples[id].numBuffers; bufferIndex++) {
        voiceBuffer = dsSoundSamples[id].buffers[bufferIndex];
        dsBufferResult = IDirectSoundBuffer_GetStatus(voiceBuffer, &playState);
        if (playState == DSBSTATUS_PLAYING || playState == DSBSTATUS_LOOPING) {
            dsBufferResult = IDirectSoundBuffer_Stop(voiceBuffer);
            slot = dsSoundSamples[id].voices[bufferIndex];
            dsSoundSlots[slot].status = 0;
        }
        dsBufferResult = IDirectSoundBuffer_Release(voiceBuffer);
        dsSoundSamples[id].buffers[bufferIndex] = 0;
    }
    dsSoundSamples[id].numBuffers = 0;
    dsSoundSamples[id].channelMask = 0;
    return 0;
#else
    LPDIRECTSOUNDBUFFER buffer;
    DWORD status;
    u_int i;

    if (sfxSoundReady && id >= 0) {
        if (!dsSoundSamples[id].buffers[0])
            return -1;
        for (i = 0; i < dsSoundSamples[id].numBuffers; i++) {
            buffer = dsSoundSamples[id].buffers[i];
            dsBufferResult = IDirectSoundBuffer_GetStatus(buffer, &status);
            if (status == DSBSTATUS_PLAYING || status == DSBSTATUS_LOOPING) {
                dsBufferResult = IDirectSoundBuffer_Stop(buffer);
                dsSoundSlots[dsSoundSamples[id].voices[i]].status = 0;
            }
            dsBufferResult = IDirectSoundBuffer_Release(buffer);
            dsSoundSamples[id].buffers[i] = 0;
        }
        dsSoundSamples[id].numBuffers = 0;
        dsSoundSamples[id].channelMask = 0;
        return 0;
    }
    return -1;
#endif
}

/* Stores the callback used when a DirectSound sample finishes. */
void SfxDirectSetCallback(void (*callback)(u_int id))
{
#ifdef NETPLAY
    if (!sfxSoundReady)
        return;
    dsCallback = callback;
    sfxCallbackActive = 1;
#else
    if (sfxSoundReady) {
        dsCallback = callback;
        sfxCallbackActive = 1;
    }
#endif
}

/* Differs from the exe in register use: the exe keeps the voice in edi and the
   buffer in esi, this build swaps them and loads the buffer pointer earlier. */
#if defined(EQUIV) || defined(NON_MATCHING)
/* Returns an unused DirectSound voice index, or -1 if all are busy. */
static __inline int FindFreeVoice(void)
{
    int voice;

    for (voice = 0; voice != 32; voice++) {
        if (!(dsSoundSlots[voice].status & 1))
            return voice;
    }
    return -1;
}

#endif

#ifdef NETPLAY
/* Starts the sample on a free voice and a free duplicate buffer. The loop flag is ignored here. */
void SfxDirectPlay(int id, u_char volume, u_char lrPan, u_char fbPan, u_int pitch, u_char flags)
{
    DWORD playState;
    int slot;
    LPDIRECTSOUNDBUFFER buffer;
    DWORD rateHz;
    int channel;

    if (!sfxSoundReady || id < 0)
        return;
    if (!dsSoundSamples[id].buffers[0])
        return;
    slot = SfxDirectFindFreeSlot();
    if (slot == -1)
        return;
    channel = 0;
    while (dsSoundSamples[id].channelMask & (1 << channel)) {
        if (++channel == 8)
            return;
    }
    if (!dsSoundSamples[id].buffers[channel]) {
        dsBufferResult = IDirectSound_DuplicateSoundBuffer(
            dsSoundDevice, dsSoundSamples[id].buffers[0], &dsSoundSamples[id].buffers[channel]);
        dsSoundSamples[id].numBuffers++;
    }
    buffer = dsSoundSamples[id].buffers[channel];
    dsBufferResult = IDirectSoundBuffer_GetStatus(buffer, &playState);
    if (playState == DSBSTATUS_BUFFERLOST)
        dsBufferResult = IDirectSoundBuffer_Restore(buffer);
    dsBufferResult = IDirectSoundBuffer_SetPan(buffer, sfxPanTable[lrPan]);
    dsBufferResult = IDirectSoundBuffer_SetVolume(buffer, sfxVolumeTable[volume]);
    rateHz = dsSoundSamples[id].format.nSamplesPerSec * (u_short)pitch >> 8;
    dsBufferResult = IDirectSoundBuffer_SetFrequency(buffer, rateHz);
    dsBufferResult = IDirectSoundBuffer_Play(buffer, 0, 0, 0);
    dsSoundSamples[id].channelMask |= 1 << channel;
    dsSoundSlots[slot].sample = id;
    dsSoundSlots[slot].channel = channel;
    dsSoundSlots[slot].status = 5;
    dsSoundSamples[id].voices[channel] = slot;
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* Starts the sample on a free voice and a free duplicate buffer. The loop flag is ignored here. */
void SfxDirectPlay(int id, u_char volume, u_char lrPan, u_char fbPan, u_int pitch, u_char flags)
{
    int channel;
    int voice;
    LPDIRECTSOUNDBUFFER buffer;
    DWORD status;

    if (sfxSoundReady && id >= 0 && dsSoundSamples[id].buffers[0]) {
        voice = FindFreeVoice();
        if (voice == -1)
            return;
        channel = 0;
        while (dsSoundSamples[id].channelMask & (1 << channel)) {
            if (++channel == 8)
                return;
        }
        if (!dsSoundSamples[id].buffers[channel]) {
            dsBufferResult = IDirectSound_DuplicateSoundBuffer(
                dsSoundDevice, dsSoundSamples[id].buffers[0], &dsSoundSamples[id].buffers[channel]);
            dsSoundSamples[id].numBuffers++;
        }
        buffer = dsSoundSamples[id].buffers[channel];
        dsBufferResult = IDirectSoundBuffer_GetStatus(buffer, &status);
        if (status == DSBSTATUS_BUFFERLOST)
            dsBufferResult = IDirectSoundBuffer_Restore(buffer);
        dsBufferResult = IDirectSoundBuffer_SetPan(buffer, sfxPanTable[lrPan]);
        dsBufferResult = IDirectSoundBuffer_SetVolume(buffer, sfxVolumeTable[volume]);
        dsBufferResult = IDirectSoundBuffer_SetFrequency(
            buffer, dsSoundSamples[id].format.nSamplesPerSec * (u_short)pitch >> 8);
        dsBufferResult = IDirectSoundBuffer_Play(buffer, 0, 0, 0);
        dsSoundSamples[id].channelMask |= 1 << channel;
        dsSoundSlots[voice].sample = id;
        dsSoundSlots[voice].channel = channel;
        dsSoundSamples[id].voices[channel] = voice;
        dsSoundSlots[voice].status = 5;
    }
}
#endif
#endif

#ifdef NETPLAY
/* Starts a sample, honours the loop flag and returns its voice slot, or -1. */
int SfxDirectPlayId(int id, u_char volume, u_char lrPan, u_char fbPan, u_int pitch, u_char flags)
{
    DWORD playState;
    int slot;
    LPDIRECTSOUNDBUFFER buffer;
    DWORD rateHz;
    int channel;

    if (!sfxSoundReady || id < 0)
        return -1;
    if (!dsSoundSamples[id].buffers[0])
        return -1;
    slot = SfxDirectFindFreeSlot();
    if (slot == -1)
        return -1;
    channel = 0;
    while (dsSoundSamples[id].channelMask & (1 << channel)) {
        if (++channel == 8)
            return -1;
    }
    if (!dsSoundSamples[id].buffers[channel]) {
        dsBufferResult = IDirectSound_DuplicateSoundBuffer(
            dsSoundDevice, dsSoundSamples[id].buffers[0], &dsSoundSamples[id].buffers[channel]);
        dsSoundSamples[id].numBuffers++;
    }
    buffer = dsSoundSamples[id].buffers[channel];
    dsBufferResult = IDirectSoundBuffer_GetStatus(buffer, &playState);
    if (playState == DSBSTATUS_BUFFERLOST)
        dsBufferResult = IDirectSoundBuffer_Restore(buffer);
    dsBufferResult = IDirectSoundBuffer_SetPan(buffer, sfxPanTable[lrPan]);
    dsBufferResult = IDirectSoundBuffer_SetVolume(buffer, sfxVolumeTable[volume]);
    rateHz = dsSoundSamples[id].format.nSamplesPerSec * (u_short)pitch >> 8;
    dsBufferResult = IDirectSoundBuffer_SetFrequency(buffer, rateHz);
    if (flags & 1)
        dsBufferResult = IDirectSoundBuffer_Play(buffer, 0, 0, DSBPLAY_LOOPING);
    else
        dsBufferResult = IDirectSoundBuffer_Play(buffer, 0, 0, 0);
    dsSoundSamples[id].channelMask |= 1 << channel;
    dsSoundSlots[slot].sample = id;
    dsSoundSlots[slot].channel = channel;
    dsSoundSlots[slot].status = 7;
    dsSoundSamples[id].voices[channel] = slot;
    return slot;
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* Same as SfxDirectPlay, but honours the loop flag and returns the voice slot, or -1.
   Differs from the exe in the same register swap, and the exe compiles the loop
   flag to a branch around two pushes where this build uses an and. */
int SfxDirectPlayId(int id, u_char volume, u_char lrPan, u_char fbPan, u_int pitch, u_char flags)
{
    int channel;
    LPDIRECTSOUNDBUFFER buffer;
    int voice;
    DWORD status;

    if (sfxSoundReady && id >= 0) {
        if (!dsSoundSamples[id].buffers[0])
            return -1;
        voice = FindFreeVoice();
        if (voice == -1)
            return -1;
        channel = 0;
        while (dsSoundSamples[id].channelMask & (1 << channel)) {
            if (++channel == 8)
                return -1;
        }
        if (!dsSoundSamples[id].buffers[channel]) {
            dsBufferResult = IDirectSound_DuplicateSoundBuffer(
                dsSoundDevice, dsSoundSamples[id].buffers[0], &dsSoundSamples[id].buffers[channel]);
            dsSoundSamples[id].numBuffers++;
        }
        buffer = dsSoundSamples[id].buffers[channel];
        dsBufferResult = IDirectSoundBuffer_GetStatus(buffer, &status);
        if (status == DSBSTATUS_BUFFERLOST)
            dsBufferResult = IDirectSoundBuffer_Restore(buffer);
        dsBufferResult = IDirectSoundBuffer_SetPan(buffer, sfxPanTable[lrPan]);
        dsBufferResult = IDirectSoundBuffer_SetVolume(buffer, sfxVolumeTable[volume]);
        dsBufferResult = IDirectSoundBuffer_SetFrequency(
            buffer, dsSoundSamples[id].format.nSamplesPerSec * (u_short)pitch >> 8);
        if (flags & 1)
            dsBufferResult = IDirectSoundBuffer_Play(buffer, 0, 0, DSBPLAY_LOOPING);
        else
            dsBufferResult = IDirectSoundBuffer_Play(buffer, 0, 0, 0);
        dsSoundSamples[id].channelMask |= 1 << channel;
        dsSoundSlots[voice].sample = id;
        dsSoundSlots[voice].channel = channel;
        dsSoundSamples[id].voices[channel] = voice;
        dsSoundSlots[voice].status = 7;
        return voice;
    }
    return -1;
}
#endif
#endif

/* Checks whether the requested sound can replace its current sample. */
short SfxDirectRejectRequest(int flags, int id)
{
#ifdef NETPLAY
    if (!sfxSoundReady || id < 0)
        return -1;
    return -1;
#else
    if (sfxSoundReady && id >= 0)
        return -1;
    return -1;
#endif
}

/* Stops the DirectSound voice for a sample slot. */
void SfxDirectStop(int id)
{
#ifdef NETPLAY
    LPDIRECTSOUNDBUFFER voiceBuffer;
    int voiceChannel;
    int sampleIndex;

    if (!sfxSoundReady)
        return;
    if (id < 0)
        return;
    sampleIndex = dsSoundSlots[id].sample;
    voiceChannel = dsSoundSlots[id].channel;
    dsSoundSamples[sampleIndex].channelMask &= ~(1 << voiceChannel);
    dsSoundSlots[id].status = 0;
    voiceBuffer = dsSoundSamples[sampleIndex].buffers[voiceChannel];
    if (!voiceBuffer)
        return;
    dsBufferResult = IDirectSoundBuffer_Stop(voiceBuffer);
    dsBufferResult = IDirectSoundBuffer_SetCurrentPosition(voiceBuffer, 0);
#else
    int sample;
    int channel;
    LPDIRECTSOUNDBUFFER buffer;

    if (sfxSoundReady && id >= 0) {
        sample = dsSoundSlots[id].sample;
        channel = dsSoundSlots[id].channel;
        dsSoundSamples[sample].channelMask &= ~(1 << channel);
        dsSoundSlots[id].status = 0;
        buffer = dsSoundSamples[sample].buffers[channel];
        if (buffer) {
            dsBufferResult = IDirectSoundBuffer_Stop(buffer);
            dsBufferResult = IDirectSoundBuffer_SetCurrentPosition(buffer, 0);
        }
    }
#endif
}

/* Sets the volume of a DirectSound sample slot. */
void SfxDirectSetVolume(int id, u_char volume)
{
#ifdef NETPLAY
    LPDIRECTSOUNDBUFFER voiceBuffer;
    int voiceChannel;
    int sampleIndex;

    if (!sfxSoundReady)
        return;
    if (id < 0)
        return;
    sampleIndex = dsSoundSlots[id].sample;
    voiceChannel = dsSoundSlots[id].channel;
    voiceBuffer = dsSoundSamples[sampleIndex].buffers[voiceChannel];
    if (!voiceBuffer)
        return;
    dsBufferResult = IDirectSoundBuffer_SetVolume(voiceBuffer, sfxVolumeTable[volume]);
#else
    if (sfxSoundReady && id >= 0) {
        LPDIRECTSOUNDBUFFER buffer =
            dsSoundSamples[dsSoundSlots[id].sample].buffers[dsSoundSlots[id].channel];

        if (buffer)
            dsBufferResult = IDirectSoundBuffer_SetVolume(buffer, sfxVolumeTable[volume]);
    }
#endif
}

/* Sets the playback frequency of a DirectSound sample slot. */
void SfxDirectSetPitch(int id, u_int pitch)
{
#ifdef NETPLAY
    LPDIRECTSOUNDBUFFER voiceBuffer;
    int voiceChannel;
    int sampleRate;
    int sampleIndex;

    if (!sfxSoundReady)
        return;
    if (id < 0)
        return;
    sampleIndex = dsSoundSlots[id].sample;
    voiceChannel = dsSoundSlots[id].channel;
    voiceBuffer = dsSoundSamples[sampleIndex].buffers[voiceChannel];
    if (!voiceBuffer)
        return;
    sampleRate = dsSoundSamples[sampleIndex].format.nSamplesPerSec * pitch >> 8;
    dsBufferResult = IDirectSoundBuffer_SetFrequency(voiceBuffer, sampleRate);
#else
    int sample;
    int channel;
    int frequency;
    LPDIRECTSOUNDBUFFER buffer;

    if (sfxSoundReady && id >= 0) {
        sample = dsSoundSlots[id].sample;
        channel = dsSoundSlots[id].channel;
        buffer = dsSoundSamples[sample].buffers[channel];
        if (buffer) {
            frequency = dsSoundSamples[sample].format.nSamplesPerSec * pitch >> 8;
            dsBufferResult = IDirectSoundBuffer_SetFrequency(buffer, frequency);
        }
    }
#endif
}

/* Sets the stereo pan of a DirectSound sample slot. */
void SfxDirectSetPan(int id, u_int pan)
{
#ifdef NETPLAY
    LPDIRECTSOUNDBUFFER voiceBuffer;
    int voiceChannel;
    int sampleIndex;

    if (!sfxSoundReady)
        return;
    if (id < 0)
        return;
    sampleIndex = dsSoundSlots[id].sample;
    voiceChannel = dsSoundSlots[id].channel;
    voiceBuffer = dsSoundSamples[sampleIndex].buffers[voiceChannel];
    if (!voiceBuffer)
        return;
    dsBufferResult = IDirectSoundBuffer_SetPan(voiceBuffer, sfxPanTable[(u_char)pan]);
#else
    if (sfxSoundReady && id >= 0) {
        LPDIRECTSOUNDBUFFER buffer =
            dsSoundSamples[dsSoundSlots[id].sample].buffers[dsSoundSlots[id].channel];

        if (buffer)
            dsBufferResult = IDirectSoundBuffer_SetPan(buffer, sfxPanTable[(u_char)pan]);
    }
#endif
}

/* Enables or disables a DirectSound sample option. */
void SfxDirectSetFlag(int id, u_short option, u_char set)
{
#ifdef NETPLAY
    LPDIRECTSOUNDBUFFER voiceBuffer;
    int voiceChannel;
    int sampleIndex;

    if (!sfxSoundReady)
        return;
    if (id < 0)
        return;
    sampleIndex = dsSoundSlots[id].sample;
    voiceChannel = dsSoundSlots[id].channel;
    voiceBuffer = dsSoundSamples[sampleIndex].buffers[voiceChannel];
    if (!voiceBuffer)
        return;
    if (option == 1) {
        if (set == 1)
            dsBufferResult = IDirectSoundBuffer_Play(voiceBuffer, 0, 0, DSBPLAY_LOOPING);
        else
            dsBufferResult = IDirectSoundBuffer_Play(voiceBuffer, 0, 0, 0);
    }
#else
    DirectSoundSample *sample;
    LPDIRECTSOUNDBUFFER buffer;

    if (sfxSoundReady && id >= 0) {
        sample = &dsSoundSamples[dsSoundSlots[id].sample];
        buffer = sample->buffers[dsSoundSlots[id].channel];
        if (buffer && option == 1) {
            if (set == 1)
                dsBufferResult = IDirectSoundBuffer_Play(buffer, 0, 0, DSBPLAY_LOOPING);
            else
                dsBufferResult = IDirectSoundBuffer_Play(buffer, 0, 0, 0);
        }
    }
#endif
}


#ifdef NETPLAY
/* The binary retains this readiness test although both paths have no effect. */
void func_46df32(void)
{
    if (!sfxSoundReady)
        return;
}

void func_46e020(void) {}
void func_46e036(void) {}
void func_46e041(void) {}
#endif

/* The hardware-off hook is empty in this build. */
void SfxDirectOff(void)
{
}

#ifdef NETPLAY
/* Polls every playing voice once per frame. A voice whose buffer has stopped is freed,
   rewound and, when it asked for it, reported through the end-of-sample callback. */
void SfxDirectService(void)
{
    LPDIRECTSOUNDBUFFER voiceBuffer;
    DWORD playState;
    int sampleIndex;
    u_int voiceNo;
    int channel;

    if (!sfxSoundReady)
        return;
    for (voiceNo = 0; voiceNo < 32; voiceNo++) {
        if (dsSoundSlots[voiceNo].status & 4) {
            sampleIndex = dsSoundSlots[voiceNo].sample;
            channel = dsSoundSlots[voiceNo].channel;
            voiceBuffer = dsSoundSamples[sampleIndex].buffers[channel];
            dsBufferResult = IDirectSoundBuffer_GetStatus(voiceBuffer, &playState);
            if (playState != DSBSTATUS_LOOPING && playState != DSBSTATUS_PLAYING) {
                dsSoundSamples[sampleIndex].channelMask &= ~(1 << channel);
                dsSoundSlots[voiceNo].status &= ~4;
                if (!(dsSoundSlots[voiceNo].status & 2))
                    dsSoundSlots[voiceNo].status = 0;
                dsBufferResult = IDirectSoundBuffer_SetCurrentPosition(voiceBuffer, 0);
                if (sfxCallbackActive && (dsSoundSlots[voiceNo].status & 2))
                    dsCallback(voiceNo | 0x400);
            }
        }
    }
}
#endif
#ifdef EQUIV
#ifdef POWERVR
/* Polls every playing voice once per frame. A voice whose buffer has stopped is freed,
   rewound and, when it asked for it, reported through the end-of-sample callback. */
void SfxDirectService(void)
{
    DirectSoundVoice *voice;
    LPDIRECTSOUNDBUFFER buffer;
    u_int index;
    int sample;
    int channel;
    u_int voiceStatus;
    DWORD status;

    if (!sfxSoundReady)
        return;
    index = 0;
    for (voice = dsSoundSlots; voice < dsSoundSlots + 32; voice++, index++) {
        if (!(voice->status & 4))
            continue;
        sample = voice->sample;
        channel = voice->channel;
        buffer = dsSoundSamples[sample].buffers[channel];
        dsBufferResult = IDirectSoundBuffer_GetStatus(buffer, &status);
        if (status == DSBSTATUS_LOOPING || status == DSBSTATUS_PLAYING)
            continue;
        voiceStatus = voice->status & ~4;
        dsSoundSamples[sample].channelMask &= ~(1 << channel);
        voice->status = voiceStatus;
        if (!(voiceStatus & 2))
            voice->status = 0;
        dsBufferResult = IDirectSoundBuffer_SetCurrentPosition(buffer, 0);
        if (sfxCallbackActive && (voice->status & 2))
            dsCallback(index | 0x400);
    }
}
#endif
#endif

#if !defined(NETPLAY) && !defined(POWERVR)
void SfxDirectService(void)
{
    int channel;
    u_int index;
    DWORD status;
    LPDIRECTSOUNDBUFFER buffer;
    int sample;

    if (!sfxSoundReady)
        return;
    for (index = 0; index < 32; index++) {
        if (!(dsSoundSlots[index].status & 4))
            continue;
        sample = dsSoundSlots[index].sample;
        channel = dsSoundSlots[index].channel;
        buffer = dsSoundSamples[sample].buffers[channel];
        dsBufferResult = IDirectSoundBuffer_GetStatus(buffer, &status);
        if (status == DSBSTATUS_LOOPING || status == DSBSTATUS_PLAYING)
            continue;
        dsSoundSamples[sample].channelMask &= ~(1 << channel);
        dsSoundSlots[index].status &= ~4;
        if (!(dsSoundSlots[index].status & 2))
            dsSoundSlots[index].status = 0;
        dsBufferResult = IDirectSoundBuffer_SetCurrentPosition(buffer, 0);
        if (sfxCallbackActive && (dsSoundSlots[index].status & 2))
            dsCallback(index | 0x400);
    }
}
#endif

/* Stops and releases every buffer still held, then marks the backend not ready.
   Returns early, leaving the flag set, when the backend was never ready. */
#ifdef NETPLAY
void SfxDirectUnInit(void)
{
    LPDIRECTSOUNDBUFFER voiceBuffer;
    DWORD playState;
    short slot;
    u_int sampleIndex;
    u_int channel;

    if (!sfxSoundReady)
        return;
    for (sampleIndex = 0; sampleIndex < 128; sampleIndex++) {
        if (dsSoundSamples[sampleIndex].buffers[0]) {
            for (channel = 0; channel < dsSoundSamples[sampleIndex].numBuffers; channel++) {
                voiceBuffer = dsSoundSamples[sampleIndex].buffers[channel];
                dsBufferResult = IDirectSoundBuffer_GetStatus(voiceBuffer, &playState);
                if (playState == DSBSTATUS_PLAYING || playState == DSBSTATUS_LOOPING) {
                    dsBufferResult = IDirectSoundBuffer_Stop(voiceBuffer);
                    slot = (short)dsSoundSamples[sampleIndex].voices[channel];
                    dsSoundSlots[slot].status = 0;
                }
                dsBufferResult = IDirectSoundBuffer_Release(voiceBuffer);
                dsSoundSamples[sampleIndex].buffers[channel] = NULL;
            }
            dsSoundSamples[sampleIndex].numBuffers = 0;
            dsSoundSamples[sampleIndex].channelMask = 0;
        }
    }
    sfxSoundReady = 0;
}
#else
void SfxDirectUnInit(void)
{
    u_int sampleIndex;
    LPDIRECTSOUNDBUFFER buffer;
    u_int channel;
    DWORD status;

    if (!sfxSoundReady)
        return;
    for (sampleIndex = 0; sampleIndex < 128; sampleIndex++) {
        if (dsSoundSamples[sampleIndex].buffers[0]) {
            channel = 0;
            if (dsSoundSamples[sampleIndex].numBuffers != channel) {
                do {
                    buffer = dsSoundSamples[sampleIndex].buffers[channel];
                    dsBufferResult = IDirectSoundBuffer_GetStatus(buffer, &status);
                    if (status == DSBSTATUS_PLAYING || status == DSBSTATUS_LOOPING) {
                        dsBufferResult = IDirectSoundBuffer_Stop(buffer);
                        dsSoundSlots[(short)dsSoundSamples[sampleIndex].voices[channel]].status = 0;
                    }
                    dsBufferResult = IDirectSoundBuffer_Release(buffer);
                    dsSoundSamples[sampleIndex].buffers[channel] = NULL;
                    channel++;
                } while (channel < dsSoundSamples[sampleIndex].numBuffers);
            }
            dsSoundSamples[sampleIndex].numBuffers = 0;
            dsSoundSamples[sampleIndex].channelMask = 0;
        }
    }
    sfxSoundReady = 0;
}
#endif

#ifdef NETPLAY
/* Returns the first voice slot whose active bit is clear. */
int SfxDirectFindFreeSlot(void)
{
    int index;

    index = 0;
    while (dsSoundSlots[index].status & 1) {
        index++;
        if (index == 32)
            return -1;
    }
    return index;
}
#endif
