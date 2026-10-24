#ifndef PC_SFX_TYPES_H
#define PC_SFX_TYPES_H

/* One playing or queued sample in the software mixer's list. */
typedef struct SfxSample {
    struct SfxSample *next;
    struct SfxSample *prev;
    struct SfxSample *chain;
    unsigned char *data;
    unsigned char *rightData;
    unsigned int offset;        /* byte offset in sample data (0x14) */
    int frontOffset;         /* Sample offset for the front channel in four-channel mixes. */
    unsigned int length;
    unsigned short id;
    unsigned short flags;
    unsigned short rate;
    unsigned short pitch;
    unsigned short posFrac;
    unsigned short rearPosFrac;  /* Fractional sample position for the rear mix. */
    unsigned char volume;
    unsigned char pan;
    unsigned char fbPan;
} SfxSample;

#define SFX_ACTIVE   0x0001
#define SFX_LOOP     0x0002
#define SFX_CHAINED  0x0008
#define SFX_ENDED    0x8000

#endif
