#ifndef PC_SFX_H
#define PC_SFX_H

struct SfxSample;
struct WavFile;
struct IDirectSound;

extern void (*sfxCallback)(unsigned int id);
extern void (*sfxDoMixCopy)(void);

int SfxInitReverbLow(int type, unsigned int delay);
void SfxReverbOnLow(void);
void SfxReverbOffLow(void);
void SfxSetReverbDepthLow(int depth);
void SfxUnInitReverbLow(void);
unsigned int SfxSetMaxEffectsLow(unsigned int num);
int SfxSoftMixInit(struct IDirectSound *sound, unsigned int mode, unsigned int rate);
int SfxSoftTransEffect(struct WavFile *left, struct WavFile *right, unsigned int flags);
int SfxSoftFreeEffect(unsigned int id);
void SfxSetCallBackLow(void (*callback)(unsigned int id));
void SfxSoftPlay(int id, unsigned char volume, unsigned char lrPan, unsigned char fbPan, unsigned int pitch, unsigned char flags);
int SfxSoftPlayId(int id, unsigned char volume, unsigned char lrPan, unsigned char fbPan, unsigned int pitch, unsigned char flags);
void SfxEffectKill(unsigned int id);
void SfxEffectVolLow(unsigned int id, unsigned char volume);
void SfxEffectPitchLow(unsigned int id, unsigned int pitch);
void SfxEffectLRPanLow(unsigned int id, unsigned char pan);
void SfxEffectFBPanLow(unsigned int id, unsigned char pan);
void SfxEffectFlag(unsigned int id, unsigned short option, unsigned char set);
void SfxFreeAll(void);
void SfxDoSfx(void);
void SfxUnInitLow(void);
void SfxConvertSamples(unsigned char *dst, short *src, unsigned int size, unsigned int flags);
void SfxFreeMixTables(void);
void SfxDumpMixedSamples(void);

/* Software mixers advance the sample list and return the number of completed samples. */
unsigned short SfxMixBasic(struct SfxSample **list);
unsigned short SfxMixerLut(struct SfxSample **list);
unsigned short MixStereoSfxSamples(struct SfxSample **list);
unsigned short SfxMixFourChannel(struct SfxSample **list);
unsigned short SfxBuildMixTables(unsigned short mode);

void ClearMixScratch(void);
void ClearMixOutput(void);
void PackMixMono16(void);
void PackMixStereo16(void);
void PackMixMono8(void);
void PackMixStereo8(void);
void StereoReverb(void);

#endif
