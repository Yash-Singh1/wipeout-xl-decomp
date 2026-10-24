#ifndef MPEG_H
#define MPEG_H

/* MPEG-1 video stream decoder (bit reader, header parsing, table setup). */

enum {
    MPEG_CODE_PICTURE_START = 0x00,
    MPEG_CODE_SEQUENCE_HEADER = 0xB3,
    MPEG_CODE_EXTENSION = 0xB5,
    MPEG_CODE_USER_DATA = 0xB2,
    MPEG_CODE_GROUP_START = 0xB8,
    MPEG_CODE_SEQUENCE_END = 0xB7,
    MPEG_AUDIO_DATA_SIGNATURE_0 = 0xBE,
    MPEG_AUDIO_DATA_SIGNATURE_1 = 0xEF
};

/* Picture header state; one per frame buffer in use. */
typedef struct Picture {
    unsigned char inUse;
    char pad01;
    short temporalRef;
    unsigned char type;           /* 1 = I, 2 = P, 3 = B */
    char pad05;
    short vbvDelay;
    unsigned char fullPelForward;
    char pad09;
    short forwardRSize;
    unsigned char forwardF;
    unsigned char fullPelBackward;
    short backwardRSize;
    unsigned char backwardF;
    char pad11[3];
    unsigned char *y;             /* 0x14 luma plane */
    unsigned char *cb;
    unsigned char *cr;
    char pad20[8];
    short lastMacroblock;         /* 0x28, last decoded macroblock index */
    char pad2a[2];
} Picture;

/* One entry of the decoded-frame ring: display count (0 = empty) and the frame buffer. */
typedef struct FrameSlot {
    int count;
    Picture *frame;
} FrameSlot;

/* Decoder state for one MPEG-1 stream: bit reader, quantiser tables, the picture headers, the
   current macroblock and the frame ring that holds decoded frames waiting to be shown. */
typedef struct VidStream {
    unsigned char *bufStart;  /* 0x000 */
    unsigned char *bufEnd;  /* 0x004 */
    unsigned long curBits;  /* 0x008 */
    int bitOffset;  /* 0x00c */
    int macroblockStride;  /* 0x010, 0x80 bytes per macroblock column */
    short *curIntraQ;  /* 0x014 */
    short *curNonIntraQ;  /* 0x018 */
    int cbp;  /* 0x01c */
    int yPastDelta;  /* 0x020 */
    char pad024[0x4];
    int yFutureDelta;  /* 0x028 */
    char pad02c[0x14];
    int yOffset;  /* 0x040 */
    int dcPredY;  /* 0x044 */
    int yStride;  /* 0x048 */
    int yBase;  /* 0x04c */
    char pad050[0x10];
    int cbOffset;  /* 0x060 */
    int dcPredCb;  /* 0x064 */
    int cbStride;  /* 0x068 */
    int cbBase;  /* 0x06c */
    char pad070[0x10];
    int crOffset;  /* 0x080 */
    int dcPredCr;  /* 0x084 */
    int crStride;  /* 0x088 */
    int crBase;  /* 0x08c */
    char pad090[0x10];
    void *codeBuf;  /* 0x0a0 */
    char pad0a4[0x7c];
    unsigned long decodeBlock[32];  /* 0x120, scratch written by the generated block decoder */
    Picture *curPic;  /* 0x1a0 */
    Picture *pastPic;  /* 0x1a4 */
    Picture *futurePic;  /* 0x1a8 */
    unsigned char isMpeg1;  /* 0x1ac, set when no sequence extension follows */
    char pad1ad[0x3];
    unsigned char *buffer;  /* 0x1b0 */
    char pad1b4[0x4];
    unsigned char eof;  /* 0x1b8 */
    char pad1b9[0x3];
    unsigned short hSize;  /* 0x1bc */
    unsigned short vSize;  /* 0x1be */
    unsigned char aspectRatio;  /* 0x1c0 */
    unsigned char pictureRate;  /* 0x1c1 */
    char pad1c2[0x2];
    int bitRate;  /* 0x1c4 */
    int vbvBufferSize;  /* 0x1c8 */
    unsigned char constrainedParams;  /* 0x1cc */
    char pad1cd[0x1];
    short intraQuant[2048];  /* 0x1ce */
    short nonIntraQuant[2048];  /* 0x11ce */
    char pad21ce[0x2];
    unsigned char gopDropFlag;  /* 0x21d0 */
    unsigned char gopHours;  /* 0x21d1 */
    unsigned char gopMinutes;  /* 0x21d2 */
    unsigned char gopSeconds;  /* 0x21d3 */
    unsigned char gopPictures;  /* 0x21d4 */
    unsigned char gopClosed;  /* 0x21d5 */
    unsigned char gopBrokenLink;  /* 0x21d6 */
    char pad21d7[0x1];
    Picture pics[24];  /* 0x21d8 */
    short sliceRow;  /* 0x25f8 */
    unsigned char sliceExtra;  /* 0x25fa */
    unsigned char sliceExtra2;  /* 0x25fb */
    short fwdPredH;  /* 0x25fc: forward motion predictor, horizontal */
    short fwdPredV;  /* 0x25fe: forward motion predictor, vertical */
    short bwdPredH;  /* 0x2600: backward motion predictor, horizontal */
    short bwdPredV;  /* 0x2602: backward motion predictor, vertical */
    short mbIndex;  /* 0x2604 */
    short mbType;  /* 0x2606: macroblock type flags (1 quant, 2 forward, 4 backward, 8 pattern, 0x10 intra) */
    short motionHCode;  /* 0x2608 */
    short motionVCode;  /* 0x260a */
    short motionHR;  /* 0x260c */
    short motionVR;  /* 0x260e */
    short backHCode;  /* 0x2610 */
    short backVCode;  /* 0x2612 */
    short backHR;  /* 0x2614 */
    short backVR;  /* 0x2616 */
    char pad2618[0x4];
    short mbWidth;  /* 0x261c */
    short mbHeight;  /* 0x261e */
    short mbPixelX;  /* 0x2620: pixel x of the current macroblock */
    short mbPixelY;  /* 0x2622: pixel y of the current macroblock row */
    short tempQuant[64];  /* 0x2624 */
    FrameSlot ring[64];  /* 0x26a4 */
    int ringHead;  /* 0x28a4 */
    int ringTail;  /* 0x28a8 */
    int pictureCount;  /* 0x28ac */
    char pad28b0[0x4];
    unsigned char frameTimerStarted;  /* 0x28b4, set after the playback timer starts */
    char pad28b5[0x3];
    int ringLimit;  /* 0x28b8 */
    char pad28bc[0x4];
    void *heap;  /* 0x28c0 */
} VidStream;

/* One variable-length code of a table: bit string ("0", "1", " " separators) and its value. */
typedef struct VlcSource {
    const char *code;
    short value;
} VlcSource;

/* Decode table entry: value is -1 for unused slots. */
typedef struct VlcEntry {
    short value;
    short numBits;
} VlcEntry;

extern unsigned long bitMask[];
extern short defaultIntraQuant[64];
extern short defaultNonIntraQuant[64];
extern short zigzag[64];

void mpegRefillVideoStreamBuffer(VidStream *vs);
void mpegQueueAudioData(unsigned char *data, unsigned long len);
extern int mpegAudioActive;
extern int mpegAudioStarted;
extern unsigned int mpegFrameTimer;
extern int mpegPaced;
extern int mpegTicks;

#include "pc_ddraw.h"
#include "pc_palette.h"

extern VidStream *mpegVideo;
extern unsigned int mpegFirstFrame;
extern unsigned int mpegLastFrame;
void mpegDrawFrame(Picture *frame, unsigned int n);
void mpegInitOutputFormat(void);
extern DDPalette *mpegPalette;
extern long mpegPaletteResult;
extern DDSurface *mpegSurface;
extern DDSurface *mpegOffscreenSurface;
extern int mpegDrawMode;
extern int mpegHeapPeak;
extern int mpegPicWidth;
extern int mpegPicHeight;
extern int mpegPicPixels;
extern int mpegStreamResetValue;
extern unsigned char mpegStreamResetFlag;
extern int mpegSrcWidth;
extern int mpegSrcHalfWidth;
extern unsigned char *mpegPlaneY;
extern unsigned char *mpegPlaneCb;
extern unsigned char *mpegPlaneCr;
extern int mpegDestX0;
extern int mpegDestX1;
extern int mpegDestY0;
extern int mpegDestY1;
extern int mpegDestPitch;
extern int mpegDestRows;
extern int mpegDestCols;
VidStream *mpegOpenVideoStream(const char *path, void *heap);
void mpegCloseVideoStream(VidStream *unusedStream);
void mpegReadVideoStreamChunk(VidStream *vs);
void mpegInitVideoStream(VidStream *vs);
void MpegDitherToPalette(void);
void mpegDrawDithered(void);
void mpegDrawFrameRGB16Native(void);
void mpegDrawFrameRGB16Doubled(void);
void mpegDrawFrameRGB24Doubled(void);
void mpegDrawFrameRGB32Doubled(void);
void mpegBuildDisplayPalette(DDPalette *palette);
void mpegInitColourTables(DDPixelFormat *pixelFormat);
void mpegYCbCrToClampedRGB(int y, int cr, int cb,
                           unsigned int *r, unsigned int *g, unsigned int *b);
extern unsigned char *mpegRedContribution;
extern unsigned char *mpegBlueContribution;
extern unsigned char *mpegGreenCombination;
extern unsigned char *mpegClampTable;
extern unsigned short *mpegRGB16Table;
extern short mpegPaletteBlackIndex;
int mpegLockSurface(DDSurface *surf, DDSurfaceDesc *desc);
int mpegUnlockSurface(DDSurface *surf, DDSurfaceDesc *desc);
extern int mpegAudioPos;
extern int mpegFrameCount;
extern int mpegDropCount;
extern int mpegAbort;
extern int mpegTickTotal;
extern int mpegLate;
extern short mpegSync;

void mpegStopPlayback(void);
void mpegAbortPlayback(void);
void mpegStartAudioQueue(void);
void mpegStopAudioQueue(void);
void mpegAudioBlockPlayed(void);
void mpegDecodeStream(VidStream *vs);
void mpegStartAudioPlayback(void);
void mpegPresentQueuedFrame(VidStream *vs);
void __stdcall mpegFrameTimerCallback(unsigned id, unsigned msg, unsigned long user, unsigned long dw1, unsigned long dw2);
void mpegReadPictureHeader(VidStream *vs);
void mpegDecodePicture(VidStream *vs);
void mpegDecodeSlice(VidStream *vs);
void mpegDecodeIntraMacroblock(VidStream *vs);
extern VidStream *mpegStream;
extern int mpegRingLimit;
/* Macroblock helpers. They take their stream and offset from the mpegStream and mpegRingLimit
   globals. The Predict routines write the motion-compensated prediction into the picture
   (Copy) or average it with what is already there (Average). HalfX, HalfY and HalfXY give
   the half-pel direction; the plain names are whole-pel. */
int mpegDecodeIntraBlocks(void);
void mpegCopyMacroblock(void);
void mpegPredictAverage16HalfXY(void);
void mpegPredictAverage16HalfX(void);
void mpegPredictAverage16HalfY(void);
void mpegPredictAverage16(void);
void mpegPredictAverage8HalfXY(void);
void mpegPredictAverage8HalfX(void);
void mpegPredictAverage8HalfY(void);
void mpegPredictAverage8(void);
int mpegDecodeNonIntraBlocks(void);
void mpegPredictCopy16HalfXY(void);
void mpegPredictCopy16HalfX(void);
void mpegPredictCopy16HalfY(void);
void mpegPredictCopy16(void);
void mpegPredictCopy8HalfXY(void);
void mpegPredictCopy8HalfX(void);
void mpegPredictCopy8HalfY(void);
void mpegPredictCopy8(void);
extern VlcEntry *mbTypePTable;
void mpegDecodePredictedMacroblock(VidStream *vs);
void mpegDecodeBidirectionalMacroblock(VidStream *vs);
void mpegReadSequenceHeader(VidStream *vs);
void mpegReadExtensionsAndUserData(VidStream *vs, int mode);
void mpegReadGroupHeader(VidStream *vs);

void *mpegHeapAlloc(void **heap, int size);
void mpegYCbCrToRGB(int y, int cr, int cb, int *r, int *g, int *b);
extern int mpegHeapUsed;
void mpegDebugMessage(const char *msg, ...);
void mpegDebugFunctionEnter(int fn);
void mpegDebugFunctionLeave(void);

extern int funcDepth;
extern int funcStack[];
extern short mpegFile;

extern VlcSource mbAddrIncCodes[];
extern VlcSource mbTypePCodes[];
extern VlcSource mbTypeBCodes[];
extern VlcSource codedBlockPatternCodes[];
extern VlcSource motionCodes[];
extern VlcEntry *mbAddrIncTable;
extern VlcEntry *mbTypePTable;
extern VlcEntry *mbTypeBTable;
extern VlcEntry *codedBlockPatternTable;
extern VlcEntry *motionTable;

extern unsigned char *mpegClipTable;
extern short *mpegScaleTable;
extern unsigned long scaleBase[8][8];
extern unsigned long scaleStep[8][8];

extern unsigned char *mpegCodeBase;
extern unsigned char **mpegJumpTable;
short mpegBuildDecodeJumpTable(VidStream *vs, unsigned long base);
void mpegBuildClipScaleTables(VidStream *vs);
void mpegInitializeVlcTables(VidStream *vs);
void mpegBuildVlcLookupTable(VidStream *vs, VlcEntry **table, short count, VlcSource *codes, short bits);
void mpegRestoreCodedBlockPatternCodes(void);

/* Streaming audio output (DirectSound buffer fed from a timer). */

typedef struct IDirectSoundBuffer DSBuffer;

/* PCM format of the sound buffer, as GetFormat fills it (WAVEFORMATEX fields). */
typedef struct WaveFormat {
    unsigned short formatTag;
    unsigned short channels;
    unsigned long samplesPerSec;
    unsigned long avgBytesPerSec;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
    unsigned short cbSize;
} WaveFormat;

/* Buffer capabilities filled in by GetCaps; bufferBytes gives the size of the sound buffer. */
typedef struct DSBufferCaps {
    unsigned long size;
    unsigned long flags;
    unsigned long bufferBytes;
    unsigned long unlockTransferRate;
    unsigned long playCpuOverhead;
} DSBufferCaps;

/* DirectSound buffer method table, in IDirectSoundBuffer order. */
typedef struct IDirectSoundBufferVtbl {
    long (__stdcall *QueryInterface)(DSBuffer *, const void *, void **);
    unsigned long (__stdcall *AddRef)(DSBuffer *);
    unsigned long (__stdcall *Release)(DSBuffer *);
    long (__stdcall *GetCaps)(DSBuffer *, DSBufferCaps *);
    long (__stdcall *GetCurrentPosition)(DSBuffer *, unsigned long *, unsigned long *);
    long (__stdcall *GetFormat)(DSBuffer *, WaveFormat *, unsigned long, unsigned long *);
    long (__stdcall *GetVolume)(DSBuffer *, long *);
    long (__stdcall *GetPan)(DSBuffer *, long *);
    long (__stdcall *GetFrequency)(DSBuffer *, unsigned long *);
    long (__stdcall *GetStatus)(DSBuffer *, unsigned long *);
    long (__stdcall *Initialize)(DSBuffer *, void *, void *);
    long (__stdcall *Lock)(DSBuffer *, unsigned long, unsigned long, void **, unsigned long *, void **, unsigned long *, unsigned long);
    long (__stdcall *Play)(DSBuffer *, unsigned long, unsigned long, unsigned long);
    long (__stdcall *SetCurrentPosition)(DSBuffer *, unsigned long);
    long (__stdcall *SetFormat)(DSBuffer *, const WaveFormat *);
    long (__stdcall *SetVolume)(DSBuffer *, long);
    long (__stdcall *SetPan)(DSBuffer *, long);
    long (__stdcall *SetFrequency)(DSBuffer *, unsigned long);
    long (__stdcall *Stop)(DSBuffer *);
    long (__stdcall *Unlock)(DSBuffer *, void *, unsigned long, void *, unsigned long);
    long (__stdcall *Restore)(DSBuffer *);
} DSBufferVtbl;

struct IDirectSoundBuffer {
    const DSBufferVtbl *lpVtbl;
};

/* One block of queued audio: a run of 0x12-byte frames waiting to be played. */
typedef struct SndBlock {
    unsigned char *data;
    unsigned long fill;      /* bytes stored */
    unsigned long frames;
    unsigned char ready;
} SndBlock;

#define SND_FRAME_BYTES 0x12

int mpegPlayStream(DDSurface *surface, DSBuffer *soundBuf, const char *path, void *mem);
extern DSBuffer *mpegSoundBuf;
extern unsigned long *sndVolumeTable;
#include "sample_output.h"
extern unsigned long decodeSrc;
extern unsigned long decodeCount;
extern float panTable[];
extern long mpegSndResult;
extern unsigned long sndBufferBytes;
extern unsigned long sndHalfBytes;
extern unsigned long sndQuarterBytes;
extern unsigned long sndNumBlocks;
extern unsigned long sndCurBlock;
extern unsigned long sndLastBlock;
extern unsigned long sndPartial;
extern unsigned char sndPlaying;
extern unsigned char sndActive;
extern unsigned long sndTimer;
extern unsigned long sndStreamPos;
extern unsigned long sndUnused1;
extern unsigned long sndUnused2;
extern unsigned long sndFrameOffset;
extern unsigned char sndCallbackSet;
extern void (*sndCallback)(void);
extern SndBlock sndBlocks[];

int mpegInitAudioQueue(unsigned long numBlocks, unsigned long blockSize, void *mem);
void mpegResetAudioBlockQueue(void);
void mpegSetAudioBlockCallback(void (*callback)(void));
void mpegQueueAudioData(unsigned char *data, unsigned long len);
int mpegCountReadyAudioBlocks(void);
void mpegStartAudioPlayback(void);
void mpegStopAudioPlayback(void);
void __stdcall mpegAudioTimerCallback(unsigned id, unsigned msg, unsigned long user, unsigned long dw1, unsigned long dw2);
void mpegDecodeAudioFrames(unsigned char *dst);
int mpegBuildVolumeTable(void *table);

#endif
