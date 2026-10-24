#ifndef PC_SFXDS_TYPES_H
#define PC_SFXDS_TYPES_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;

/* Playing voice. status bit 0 marks the slot as taken, bit 1 asks for the
   end-of-sample callback and bit 2 marks the voice as still running. */
typedef struct DirectSoundVoice {
    unsigned int status;
    int sample;
    int channel;
} DirectSoundVoice;

/* A sample owns up to eight buffer channels and remembers the voice on each. */
typedef struct DirectSoundSample {
    LPDIRECTSOUNDBUFFER buffers[8];
    int voices[8];
    WAVEFORMATEX format;
    int numBuffers;
    unsigned int channelMask;
} DirectSoundSample;

extern WAVEFORMATEX waveFormat;
extern unsigned char sfxSoundReady;
extern unsigned char sfxCallbackActive;
extern void (*dsCallback)(unsigned int id);
extern int sfxVolumeTable[256];
extern int sfxPanTable[30];
extern HRESULT dsBufferResult;
extern DirectSoundSample dsSoundSamples[128];
extern LPDIRECTSOUND dsSoundDevice;
extern DirectSoundVoice dsSoundSlots[32];

#endif
