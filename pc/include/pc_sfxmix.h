#ifndef PC_SFXMIX_H
#define PC_SFXMIX_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>

#include "pc_sfx_types.h"

/* Shared state used by the software mixers and their lookup-table kernels. */
extern WAVEFORMATEX waveFormat;
extern int *quantTable;
extern unsigned short lutMode;
extern unsigned char *lutRows;
extern unsigned char *lutPairsA;
extern unsigned char *lutPairsB;
extern unsigned char *lutQuads;
extern void (*sfxMixDispatch[])(void);
extern int *mixScratch;
extern char *lutOut;
extern unsigned short lutAcc;
extern unsigned short lutRowSingle;
extern unsigned char lutRowA;
extern unsigned char lutRowB;
extern unsigned short lutStep;
extern int lutCount;
extern unsigned int sfxMaxActive;
extern unsigned int mixBufFrames;
extern unsigned int mixBlockWords;
extern int *mixSrc;
extern unsigned int mixCount;
extern void *mixClearDst;
extern unsigned int mixClearBits;

/* Delay and reverb state shared with the assembly mixer kernels. */
extern int *delayBuf;
extern int *workBuf;
extern int *comb1Buf;
extern int *comb2Buf;
extern int *comb3Buf;
extern int *allPassBuf;
extern int delayLen;
extern int comb1Len;
extern int comb2Len;
extern int comb3Len;
extern int allPassLen;
extern int *delayPos;
extern int *comb1Pos;
extern int *comb2Pos;
extern int *comb3Pos;
extern int *allPassPos;
extern int sndWordsLeft;
extern int sndRunLeft;
extern int sndSamples;
extern int sndCarry;
extern void *sndEndE;
extern void *sndEndB;
extern void *sndEndC;
extern void *sndEndD;
extern unsigned char reverbType;
extern unsigned char reverbSet;
extern unsigned char reverbDepth;
extern unsigned char delayCoeff;

extern unsigned char mixPanSide[2];
extern void (*doReverb)(void);

/* One channel of the sound effect mixer: sample offset, two lookup rows and dispatch flags. */
typedef struct SfxMixChannel {
    unsigned int sampleOffset;
    unsigned short rowA;
    unsigned short rowB;
    unsigned short dispatchFlags;
    unsigned short reserved;
} SfxMixChannel;

extern SfxMixChannel sfxMixChannels[3];

void LutClearRow(void);
void ClearLutPairs(void);
void LutStoreRow(void);
void LutStoreRowStep(void);
void LutAddRow(void);
void LutAddRowStep(void);
void LutStoreDual(void);
void LutStoreDualStep(void);
void LutAddDual(void);
void LutAddDualStep(void);
void LutStoreNeg(void);
void LutStoreNegStep(void);
void LutAddNeg(void);
void LutAddNegStep(void);
void LutStoreDup(void);
void LutStoreDupStep(void);
void LutAddDup(void);
void LutAddDupStep(void);

#endif
