#ifndef PC_SOUND_H
#define PC_SOUND_H

/* PCM format passed to the PC sound-buffer allocator, including its extra-format-size word. */
#ifndef _WAVEFORMATEX_
#pragma pack(push, 2)
typedef struct tWAVEFORMATEX {
    unsigned short wFormatTag, nChannels;
    unsigned long nSamplesPerSec, nAvgBytesPerSec;
    unsigned short nBlockAlign, wBitsPerSample, cbSize;
} PcWaveFormat;
#pragma pack(pop)
#else
typedef WAVEFORMATEX PcWaveFormat;
#endif

struct IDirectSoundBuffer;
int SfxCreateBuffer(struct IDirectSoundBuffer **buffer, unsigned int size, PcWaveFormat *format);
void SfxEffectVol(int id, unsigned char volume);
void SfxEffectPitch(int id, unsigned int pitch);
int SfxInitReverb(int type, unsigned int delay);
void SfxSetReverbDepth(int depth);

struct WavFile;
int SfxInit(void *window, unsigned int mode, unsigned int rate);
int SfxTransEffect(struct WavFile *left, struct WavFile *right, unsigned int flags);

void SfxPlay(unsigned int id, unsigned char volume, unsigned char lrPan,
             unsigned char fbPan, unsigned int pitch, int flags);
int SfxPlayId(unsigned int id, unsigned char volume, unsigned char lrPan,
              unsigned char fbPan, unsigned int pitch, int flags);
void SfxEffectFlagSet(int id, unsigned int option, unsigned int set);

#endif
