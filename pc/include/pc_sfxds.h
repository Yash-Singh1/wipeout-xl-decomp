#ifndef PC_SFXDS_H
#define PC_SFXDS_H

struct IDirectSound;
struct WavFile;

#ifdef NETPLAY
void SfxDirectInit(struct IDirectSound *ds, unsigned int mode, unsigned int rate);
#else
struct IDirectSound *SfxDirectInit(struct IDirectSound *ds, unsigned int mode, unsigned int rate);
#endif
int SfxDirectLoadSample(struct WavFile *wav, unsigned int flags);
int SfxDirectFreeEffect(int id);
void SfxDirectSetCallback(void (*callback)(unsigned int id));
short SfxDirectRejectRequest(int flags, int id);
void SfxDirectStop(int id);
void SfxDirectSetVolume(int id, unsigned char volume);
void SfxDirectSetPitch(int id, unsigned int pitch);
void SfxDirectSetPan(int id, unsigned int pan);
void SfxDirectSetFlag(int id, unsigned short option, unsigned char set);
void SfxDirectPlay(int id, unsigned char volume, unsigned char lrPan, unsigned char fbPan, unsigned int pitch, unsigned char flags);
int SfxDirectPlayId(int id, unsigned char volume, unsigned char lrPan, unsigned char fbPan, unsigned int pitch, unsigned char flags);

void SfxDirectOff(void);
void SfxDirectService(void);
void SfxDirectUnInit(void);

#endif
