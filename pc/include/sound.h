#ifndef SOUND_H
#define SOUND_H

#include "wipeout.h"
#include "pc_config.h"
#include "pc_sound.h"
#include "wincd.h"
#include <stdio.h>

WINAPI_IMPORT void * __stdcall GlobalAlloc(unsigned int flags, unsigned long size);
WINAPI_IMPORT void * __stdcall GlobalFree(void *memory);

extern int sampleHandle[128];
extern char *sampleBuffer;
extern char wobbleDir6[];
#include "sound_state.h"
extern short remoteEngineVoice2, remoteEngineVoice1, turbulenceVoice, playerEngineVoice;
extern long previousSoundCamera[3];

extern void *g_hwnd;
void SfxSetVolume(unsigned char volume);
void SfxSetCdVolume(unsigned char volume);
void SfxOn(void);
void SfxReverbOn(void);
void InitSpuSound(void);
void InitSpuSamples(void);

void LoadSample(int sampleId, char *name);
char *ReadSample(char *name);
long SampleFileLength(FILE *file);
void StartControlledSound(void);
void UpdateNotes(int paused);
void KillNote(void);
void KillNotes(void);
void KillFadeNotes(void);
void PlayNote(int sampleId, short volume, int lrPan,
              int fbPan, int pitch);
int KillControlledNote(short voice);
void SetPitch(short voice, int pitch);
void SetNoteVolume(short voice, unsigned char volume);
void KillControlledSound(void);
void MuteControlledSound(void);
void TerminateSound(void);

void FileAccessError(char *name);
void SfxStop(int voice);

extern short shieldHumPlaying, shieldHumVoice;
#include "shield_hum.h"

extern short droidSoundPlaying, droidVoice;
void DroidSound(short enabled);

extern short pitAmbientPlaying, pitAmbientVoice;
void SetPitAmbient(short enabled);

extern short crowdSoundPlaying, crowdVoice;
void CrowdSound(short enabled);
void SetAmbientLoop3(short enabled);

extern short scrapeSoundPlaying, scrapeVoice;
void ScrapeSound(short enabled);

extern TrackCamera *cameraPtr;
int GetMagnitude(float x, float y, float z);
int ang(int angle);
short PlayDirectionalNote(short sampleId, short pitch, short duration, VECTOR pos);

extern int cdMusicCount;
extern char msgCrowdPrefix[], msgCdMusicStop[];
int InitCD(CdlLOC *locs, int unused);
int InitCrowdSound(Object **prm, short prmCount, Object **destination, int count);
void CdMusicPlay(int track, int unused);
void CdMusicStop(void);

extern ShipData *shipIndex;
extern short ownShip;
extern unsigned char linkRaceDone;
extern CameraUpdateFunc CameraUpdate, oldcameraupdate;
void SfxEffectLRPan(int voice, unsigned int pan);
void AdjustDirectionalNote(short voice, short unused1, short unused2, VECTOR pos);
void AdjustDopplerEngine(int shipNo, short channel, int unused);
void AdjustRemoteEngineSound(int ship1, int ship2, int extra2, int extra3);

void AdjustCrowdSound(Object **objects, int count);

extern int testPitchStep;
void PlayPitchSequence(void);
extern short scrapeVol;
void SfxEffectFBPan(int voice, unsigned char pan);
void AdjustEngineSound(ShipData *ship, TrackCamera *camera);

#endif
