#include "mpeg.h"

#define MPEG_SCALE_TABLE_BYTES 0x40000
#define MPEG_CLIP_TABLE_ENTRIES 0x800
#define MPEG_CLIP_MID_RANGE 0x100
#define MPEG_CLIP_TABLE_BIAS 0x200
#define MPEG_CLIP_HIGH_CLAMP_START 0x300
#define MPEG_SCALE_COEFFICIENT_SPAN 0x1ff8
#define MPEG_SCALE_HALF_TABLE_STRIDE 0x2000
#define MPEG_SCALE_SIGN_THRESHOLD 0x4000
#define MPEG_SCALE_VALUE_MASK 0x7fff

/* Builds the byte clamp table (0x800 entries) and the 0x40000-byte scale table. */
void mpegBuildClipScaleTables(VidStream *vs)
{
    int idx;
    int a;
    int s;
    unsigned long s2;
    unsigned long d;
    int i;
    short j;
    short m;

    mpegScaleTable = mpegHeapAlloc(&vs->heap, MPEG_SCALE_TABLE_BYTES);
    mpegClipTable = mpegHeapAlloc(&vs->heap, MPEG_CLIP_TABLE_ENTRIES);
    for (idx = 0; idx < MPEG_CLIP_TABLE_BIAS; idx++)
        mpegClipTable[idx] = 0;
    for (idx = 0; idx < MPEG_CLIP_MID_RANGE; idx++)
        mpegClipTable[idx + MPEG_CLIP_TABLE_BIAS] = idx;
    for (idx = MPEG_CLIP_HIGH_CLAMP_START; idx < MPEG_CLIP_TABLE_ENTRIES; idx++)
        mpegClipTable[idx] = 0xff;
    for (idx = 0; idx < 8; idx++) {
        for (s = 0; s < 8; s++) {
            s2 = scaleBase[idx][s];
            d = scaleStep[idx][s];
            i = s + (idx << 15) / 2;
            for (a = MPEG_SCALE_COEFFICIENT_SPAN; a >= 0; a -= 8) {
                j = (s2 >> 16) & MPEG_SCALE_VALUE_MASK;
                if (j >= MPEG_SCALE_SIGN_THRESHOLD)
                    j++;
                mpegScaleTable[i + a] = j;
                s2 -= d;
            }
            i += MPEG_SCALE_HALF_TABLE_STRIDE;
            for (a = MPEG_SCALE_COEFFICIENT_SPAN; a >= 0; a -= 8) {
                m = (s2 >> 16) & MPEG_SCALE_VALUE_MASK;
                if (m >= MPEG_SCALE_SIGN_THRESHOLD)
                    m++;
                mpegScaleTable[i + a] = m;
                s2 -= d;
            }
        }
    }
}
