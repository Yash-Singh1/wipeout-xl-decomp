#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <math.h>

#include "pc_sfx.h"
#include "pc_sfx_types.h"
#include "pc_sfxmix.h"
#include "mpeg.h"

#define COPY 0x00
#define MIX 0x01
#define NORMAL 0x00
#define PITCH 0x02
#define DOLBY 0x08
#define ONE_CHNL 0x10
#define SKIP_MIX 0xFFFF

#define SURROUND 0x10
#define QSND_IMAGE 0x20

#define VOLUME_LEVELS 32
#define MIN(x, y) ((x) < (y) ? (x) : (y))

static unsigned char steepNibbles[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 15, 17, 19, 22, 25
};
static unsigned char linearNibbles[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};
static unsigned char pairGain[16] = {
    0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7
};
static float quadGains[16][4] = {
    { 1.0000f, 0.0000f, 0.0000f, 1.0000f },
    { 1.0000f, 0.0000f, 0.0000f, 0.8981f },
    { 1.0000f, 0.0000f, 0.0000f, 0.7466f },
    { 1.0000f, 0.0000f, 0.0000f, 0.6235f },
    { 1.0000f, 0.0000f, 0.0000f, 0.4649f },
    { 1.0000f, 0.0000f, 0.0000f, 0.2614f },
    { 1.0000f, 0.0000f, 0.0000f, 0.1416f },
    { 1.0000f, 0.0999f, 0.0000f, 0.0310f },
    { 0.9500f, 0.2000f, 0.0000f, 0.0109f },
    { 0.9500f, 0.2999f, 0.0000f, 0.0052f },
    { 0.9000f, 0.4000f, 0.0000f, 0.0026f },
    { 0.9000f, 0.5000f, 0.0000f, 0.0013f },
    { 0.8499f, 0.5499f, 0.0000f, 0.0006f },
    { 0.8499f, 0.5999f, 0.0000f, 0.0003f },
    { 0.7999f, 0.6500f, 0.0000f, 0.0000f },
    { 0.7000f, 0.7000f, 0.0000f, 0.0000f }
};

#ifdef EQUIV
#ifdef NETPLAY
extern unsigned short SfxBuildQuantTable16(void);
extern unsigned short SfxBuildQuantTable8(void);
extern unsigned short SfxBuildPanTable(void);
extern unsigned short func_46c7ab(void);
extern unsigned short func_46c8ce(void);

unsigned short SfxBuildMixTables(unsigned short mode)
{
    switch (mode) {
    case 3:
        sfxMixDispatch[DOLBY | COPY | NORMAL] = LutStoreNeg;
        sfxMixDispatch[DOLBY | COPY | PITCH] = LutStoreNegStep;
        sfxMixDispatch[DOLBY | MIX | NORMAL] = LutAddNeg;
        sfxMixDispatch[DOLBY | MIX | PITCH] = LutAddNegStep;
        /* fall through */
    case 1:
        sfxMixDispatch[COPY | NORMAL] = LutStoreRow;
        sfxMixDispatch[COPY | PITCH] = LutStoreRowStep;
        sfxMixDispatch[MIX | NORMAL] = LutAddRow;
        sfxMixDispatch[MIX | PITCH] = LutAddRowStep;
        break;
    case 2:
    case 4:
    case 5:
        sfxMixDispatch[COPY | NORMAL] = LutStoreDual;
        sfxMixDispatch[COPY | PITCH] = LutStoreDualStep;
        sfxMixDispatch[MIX | NORMAL] = LutAddDual;
        sfxMixDispatch[MIX | PITCH] = LutAddDualStep;
        sfxMixDispatch[DOLBY | COPY | NORMAL] = LutStoreNeg;
        sfxMixDispatch[DOLBY | COPY | PITCH] = LutStoreNegStep;
        sfxMixDispatch[DOLBY | MIX | NORMAL] = LutAddNeg;
        sfxMixDispatch[DOLBY | MIX | PITCH] = LutAddNegStep;
        sfxMixDispatch[ONE_CHNL | COPY | NORMAL] = LutStoreRow;
        sfxMixDispatch[ONE_CHNL | COPY | PITCH] = LutStoreRowStep;
        sfxMixDispatch[ONE_CHNL | MIX | NORMAL] = LutAddRow;
        sfxMixDispatch[ONE_CHNL | MIX | PITCH] = LutAddRowStep;
        sfxMixDispatch[QSND_IMAGE | COPY | NORMAL] = LutStoreDup;
        sfxMixDispatch[QSND_IMAGE | COPY | PITCH] = LutStoreDupStep;
        sfxMixDispatch[QSND_IMAGE | MIX | NORMAL] = LutAddDup;
        sfxMixDispatch[QSND_IMAGE | MIX | PITCH] = LutAddDupStep;
        break;
    }

    lutMode = mode;
    if (waveFormat.wBitsPerSample == 16) {
        if (SfxBuildQuantTable16())
            return 1;
    } else if (SfxBuildQuantTable8()) {
        return 1;
    }
    if (mode >= 2 && SfxBuildPanTable())
        return 1;
    if (mode == 3 && func_46c7ab())
        return 1;
    if (mode == 4 && func_46c8ce())
        return 1;
    return 0;
}
#else
/* Builds the sample quantisation table and the pan/pair/quad mixing tables for the given
   output mode. The original republishes each table pointer to its global while filling it,
   and reads each pair gain into a byte before storing it; both are kept below. */
unsigned short SfxBuildMixTables(unsigned short mode)
{
    unsigned short wIndx;
    unsigned short wVol;
    unsigned short wSamp;
    unsigned short pwMaxVols[VOLUME_LEVELS];

    switch (mode) {
    case 3:
        sfxMixDispatch[DOLBY | COPY | NORMAL] = LutStoreNeg;
        sfxMixDispatch[DOLBY | COPY | PITCH] = LutStoreNegStep;
        sfxMixDispatch[DOLBY | MIX | NORMAL] = LutAddNeg;
        sfxMixDispatch[DOLBY | MIX | PITCH] = LutAddNegStep;
        /* fall through */
    case 1:
        sfxMixDispatch[COPY | NORMAL] = LutStoreRow;
        sfxMixDispatch[COPY | PITCH] = LutStoreRowStep;
        sfxMixDispatch[MIX | NORMAL] = LutAddRow;
        sfxMixDispatch[MIX | PITCH] = LutAddRowStep;
        break;
    case 2:
    case 4:
    case 5:
        sfxMixDispatch[COPY | NORMAL] = LutStoreDual;
        sfxMixDispatch[COPY | PITCH] = LutStoreDualStep;
        sfxMixDispatch[MIX | NORMAL] = LutAddDual;
        sfxMixDispatch[MIX | PITCH] = LutAddDualStep;
        sfxMixDispatch[DOLBY | COPY | NORMAL] = LutStoreNeg;
        sfxMixDispatch[DOLBY | COPY | PITCH] = LutStoreNegStep;
        sfxMixDispatch[DOLBY | MIX | NORMAL] = LutAddNeg;
        sfxMixDispatch[DOLBY | MIX | PITCH] = LutAddNegStep;
        sfxMixDispatch[ONE_CHNL | COPY | NORMAL] = LutStoreRow;
        sfxMixDispatch[ONE_CHNL | COPY | PITCH] = LutStoreRowStep;
        sfxMixDispatch[ONE_CHNL | MIX | NORMAL] = LutAddRow;
        sfxMixDispatch[ONE_CHNL | MIX | PITCH] = LutAddRowStep;
        sfxMixDispatch[QSND_IMAGE | COPY | NORMAL] = LutStoreDup;
        sfxMixDispatch[QSND_IMAGE | COPY | PITCH] = LutStoreDupStep;
        sfxMixDispatch[QSND_IMAGE | MIX | NORMAL] = LutAddDup;
        sfxMixDispatch[QSND_IMAGE | MIX | PITCH] = LutAddDupStep;
        break;
    }

    lutMode = mode;

    if (waveFormat.wBitsPerSample == 16) {
        int *table;
        double fScaled;
        float fInput;
        float fMaxSamp;
        float fLog;
        int fSign;
        double fExpo;
        float fInterm;

        for (wIndx = 0; wIndx < VOLUME_LEVELS; wIndx++) {
            fScaled = (double)wIndx * 32767.0;
            pwMaxVols[wIndx] = (unsigned short)(fScaled / (VOLUME_LEVELS - 1));
        }

        table = GlobalAlloc(GMEM_ZEROINIT, VOLUME_LEVELS * 256 * sizeof(int));
        quantTable = table;
        if (table == NULL)
            return 1;

        fLog = log(88.6);
        wIndx = 0;
        for (wVol = 0; wVol < VOLUME_LEVELS; wVol++) {
            fMaxSamp = pwMaxVols[wVol];
            for (wSamp = 0; wSamp < 256; wSamp++) {
                fInput = ((float)wSamp - 128.0f) / 128.0f;
                fSign = fInput >= 0.0f ? 1 : -1;
                fExpo = (fLog * fInput) / fSign;
                fInterm = (exp(fExpo) - 1) / 87.6f;
                table[wIndx] = fSign * (int)(fInterm * fMaxSamp);
                wIndx++;
            }
            quantTable = table;
        }
    } else {
        int *table;
        double fScaled;
        float fMaxSamp;
        float fStep;
        float fSample;

        for (wIndx = 0; wIndx < VOLUME_LEVELS; wIndx++) {
            fScaled = (double)wIndx * 127.0;
            pwMaxVols[wIndx] = (unsigned short)(fScaled / (VOLUME_LEVELS - 1));
        }

        table = GlobalAlloc(GMEM_ZEROINIT, VOLUME_LEVELS * 256 * sizeof(int));
        quantTable = table;
        if (table == NULL)
            return 1;

        wIndx = 0;
        for (wVol = 0; wVol < VOLUME_LEVELS; wVol++) {
            fMaxSamp = pwMaxVols[wVol];
            fStep = ((2.0f * fMaxSamp) + 1.0f) / 256.0f;
            fSample = -(fMaxSamp + 1.0f);
            for (wSamp = 0; wSamp < 256; wSamp++) {
                table[wIndx] = (int)fSample;
                fSample += fStep;
                wIndx++;
            }
            quantTable = table;
        }
    }

    if (mode >= 2) {
        unsigned short wPan;
        unsigned char *rows;
        float fOffset;
        float fStep;
        float pfSteps[16];

        rows = GlobalAlloc(GMEM_ZEROINIT, VOLUME_LEVELS * 16);
        lutRows = rows;
        if (rows == NULL)
            return 1;

        for (wPan = 0; wPan < 16; wPan++)
            pfSteps[wPan] = (16.0f - (float)wPan) / 16.0f;

        for (wPan = 0; wPan < 16; wPan++) {
            fStep = pfSteps[wPan];
            fOffset = 0;
            for (wVol = 0; wVol < VOLUME_LEVELS; wVol++) {
                rows[(wPan << 5) + wVol] = (unsigned char)fOffset;
                fOffset += fStep;
            }
            lutRows = rows;
        }
    }

    if (mode == 3) {
        unsigned char *pairsA;
        unsigned char *pairsB;

        lutPairsA = GlobalAlloc(GMEM_ZEROINIT, 16 * 2);
        pairsB = GlobalAlloc(GMEM_ZEROINIT, 16 * 2);
        pairsA = lutPairsA;
        lutPairsB = pairsB;
        if (pairsA == NULL || pairsB == NULL)
            return 1;

        for (wIndx = 0; wIndx < 16; wIndx++) {
            pairsA[wIndx * 2] = steepNibbles[wIndx];
            {
                unsigned char gain = pairGain[wIndx];
                pairsA[wIndx * 2 + 1] = gain;
            }
        }
        for (wIndx = 0; wIndx < 16; wIndx++) {
            lutPairsB = pairsB;
            pairsB[wIndx * 2] = linearNibbles[wIndx];
            {
                unsigned char gain = pairGain[wIndx];
                pairsB[wIndx * 2 + 1] = gain;
            }
        }
    }

    if (mode == 4) {
        unsigned short wPan;

        lutQuads = GlobalAlloc(GMEM_ZEROINIT, 16 * VOLUME_LEVELS * 4);
        if (lutQuads == NULL)
            return 1;

        for (wPan = 0; wPan < 16; wPan++) {
            float *firstGain = &quadGains[wPan][0];

            for (wVol = 0; wVol < VOLUME_LEVELS; wVol++) {
                unsigned char *row = lutQuads + (((wPan << 5) + wVol) << 2);
                double volume = wVol;
                double scaledSample = *firstGain * volume;
                unsigned char sample = (unsigned char)scaledSample;

                scaledSample = quadGains[wPan][1] * volume;
                row[0] = sample;
                sample = (unsigned char)scaledSample;
                scaledSample = quadGains[wPan][2] * volume;
                row[1] = sample;
                sample = (unsigned char)scaledSample;
                scaledSample = quadGains[wPan][3] * volume;
                row[2] = sample;
                row[3] = (unsigned char)scaledSample;
            }
        }
    }

    return 0;
}
#endif
#endif

#ifdef EQUIV
#ifdef NETPLAY
unsigned short SfxMixBasic(SfxSample **list)
{
    unsigned int input;
    SfxSample *currentSample;
    unsigned short dispatchMode;
    unsigned short numEnded;
    unsigned char *data;
    unsigned short sampleVolume;
    unsigned int samples;
    unsigned short activeIndex;
    unsigned char isFinished;

    dispatchMode = 0;
    activeIndex = 0;
    numEnded = 0;
    currentSample = *list;
    dispatchMode = COPY | NORMAL;
    while (currentSample && activeIndex < sfxMaxActive) {
        activeIndex++;
        isFinished = 0;
        data = currentSample->data;
        input = currentSample->offset;
        sampleVolume = currentSample->volume;
        lutRowSingle = sampleVolume << 8;
        lutOut = mixScratch;
        decodeSrc = data + input;
        if (currentSample->pitch == 256) {
            lutCount = MIN(currentSample->length - input, mixBufFrames);
            samples = lutCount;
        } else {
            dispatchMode |= PITCH;
            lutAcc = currentSample->posFrac;
            lutStep = currentSample->pitch;
            lutCount = MIN(((currentSample->length - input) << 8) / lutStep, mixBufFrames);
            samples = lutCount;
        }
        if ((unsigned int)lutCount > 0) {
            sfxMixDispatch[dispatchMode]();
            input = decodeSrc - (unsigned long)data;
        }
        if (mixBufFrames > samples) {
            isFinished = 1;
            currentSample->flags |= SFX_ENDED;
            if (currentSample->flags & SFX_LOOP) {
                input = 0;
                if (currentSample->pitch == 256) {
                    lutCount = mixBufFrames - samples;
                    samples = lutCount;
                } else {
                    dispatchMode |= PITCH;
                    lutCount = mixBufFrames - samples;
                    samples = lutCount;
                }
                if ((unsigned int)lutCount > 0) {
                    decodeSrc = (unsigned long)data;
                    sfxMixDispatch[dispatchMode]();
                    input = decodeSrc - (unsigned long)data;
                }
            } else if (activeIndex == 1) {
                lutCount = mixBufFrames - samples;
                if ((unsigned int)lutCount > 0)
                    LutClearRow();
            }
            if (isFinished == 1 && !(currentSample->flags & SFX_LOOP)) {
                currentSample->flags &= ~SFX_ACTIVE;
                numEnded++;
            }
        }
        currentSample->offset = input;
        currentSample->posFrac = lutAcc;
        currentSample = currentSample->next;
        dispatchMode = MIX;
    }
    *list = currentSample;
    return numEnded;
}
#else
unsigned short SfxMixBasic(SfxSample **list)
{
    SfxSample *s;
    unsigned char *data;
    unsigned int offset;
    unsigned int count;
    unsigned int frames;
    unsigned short volume;
    unsigned short pitch;
    unsigned short dispatchMode;
    unsigned short index;
    unsigned short finished;
    unsigned short posFrac;

    index = 0;
    finished = 0;
    s = *list;

    if (s) {
        dispatchMode = COPY | NORMAL;
        frames = mixBufFrames;
        while (s) {
            if (index >= sfxMaxActive)
                break;

            pitch = s->pitch;
            data = s->data;
            volume = s->volume;
            lutOut = mixScratch;
            offset = s->offset;
            lutRowSingle = (unsigned short)(volume << 8);
            index++;
            /* The decoder uses a 32-bit unsigned address. */
            decodeSrc = (unsigned long)data + offset;

            if (pitch == 256) {
                count = MIN(s->length - offset, frames);
            } else {
                dispatchMode |= PITCH;
                lutAcc = s->posFrac;
                /* Read the sample length before publishing the pitch step. */
                count = s->length - offset;
                lutStep = pitch;
                count = MIN((count << 8) / pitch, frames);
            }
            lutCount = count;
            if (count) {
                sfxMixDispatch[dispatchMode]();
                offset = (unsigned int)(decodeSrc - (unsigned long)data);
                frames = mixBufFrames;
            }

            if (frames > count) {
                unsigned int remaining = frames - count;

                s->flags |= SFX_ENDED;
                if (s->flags & SFX_LOOP) {
                    offset = 0;
                    if (s->pitch != 256)
                        dispatchMode |= PITCH;
                    lutCount = remaining;
                    if (remaining) {
                        decodeSrc = (unsigned long)data;
                        sfxMixDispatch[dispatchMode]();
                        offset = (unsigned int)(decodeSrc - (unsigned long)data);
                        frames = mixBufFrames;
                    }
                } else if (index == 1) {
                    lutCount = frames - count;
                    if (frames != count) {
                        LutClearRow();
                        frames = mixBufFrames;
                    }
                }
                if (!(s->flags & SFX_LOOP)) {
                    s->flags &= ~SFX_ACTIVE;
                    finished++;
                }
            }

            posFrac = lutAcc;
            s->offset = offset;
            s->posFrac = posFrac;
            dispatchMode = MIX | NORMAL;
            s = s->next;
        }
    }

    *list = s;
    return finished;
}
#endif
#endif

#ifdef EQUIV
#ifdef NETPLAY
unsigned short MixStereoSfxSamples(SfxSample **list)
{
    unsigned char chan;
    unsigned short panOffset;
    SfxSample *playingSample;
    unsigned int offset;
    unsigned char *dataBuffer;
    unsigned short finishedTotal;
    unsigned short dispatch;
    unsigned short activeIndexToMix;
    unsigned char wasFinishedLocal;
    unsigned int sampleFramesNow;
    unsigned short sampleGain;

    activeIndexToMix = 0;
    finishedTotal = 0;
    playingSample = *list;
    dispatch = COPY;
    while (playingSample && activeIndexToMix < sfxMaxActive) {
        activeIndexToMix++;
        wasFinishedLocal = 0;
        sampleGain = playingSample->volume;
        panOffset = playingSample->pan;
        if (panOffset <= 15) {
            mixPanSide[0] = 0;
            mixPanSide[1] = 1;
            panOffset = 15 - panOffset;
        } else {
            mixPanSide[0] = 1;
            mixPanSide[1] = 0;
            panOffset -= 15;
        }
        for (chan = 0; chan < 2; chan++) {
            dataBuffer = playingSample->data;
            offset = playingSample->offset;
            if (chan == 0) {
                if (playingSample->rate == 0x5622)
                    offset += lutPairsA[panOffset * 2];
                else
                    offset += lutPairsB[panOffset * 2];
                if (offset > playingSample->length)
                    offset -= playingSample->length;
                lutRowSingle = sampleGain << 8;
            } else {
                lutRowSingle = lutRows[(lutPairsA[panOffset * 2 + 1] << 5) + sampleGain] << 8;
            }
            lutOut = (char *)mixScratch + (mixPanSide[chan] << 2);
            decodeSrc = (unsigned long)(dataBuffer + offset);
            lutRowA = (unsigned char)(lutRowSingle >> 8);
            if (playingSample->pitch == 256) {
                lutCount = MIN(playingSample->length - offset, mixBufFrames);
                sampleFramesNow = lutCount;
            } else {
                dispatch |= PITCH;
                lutAcc = playingSample->posFrac;
                lutStep = playingSample->pitch;
                lutCount = MIN(((playingSample->length - offset) << 8) / lutStep, mixBufFrames);
                sampleFramesNow = lutCount;
            }
            if (playingSample->flags & SURROUND) {
                dispatch |= DOLBY;
                lutOut = mixScratch;
                offset = playingSample->offset;
                decodeSrc = (unsigned long)(dataBuffer + offset);
                chan++;
            }
            if ((unsigned int)lutCount > 0) {
                sfxMixDispatch[dispatch]();
                offset = (unsigned int)(decodeSrc - (unsigned long)dataBuffer);
            }
            if (sampleFramesNow < mixBufFrames) {
                if (chan == 1) {
                    wasFinishedLocal = 1;
                    playingSample->flags |= SFX_ENDED;
                }
                if (playingSample->flags & SFX_LOOP) {
                    offset = 0;
                    lutCount = mixBufFrames - sampleFramesNow;
                    if ((unsigned int)lutCount > 0) {
                        decodeSrc = (unsigned long)dataBuffer;
                        sfxMixDispatch[dispatch]();
                        offset = (unsigned int)(decodeSrc - (unsigned long)dataBuffer);
                    }
                } else if (activeIndexToMix == 1) {
                    lutCount = mixBufFrames - sampleFramesNow;
                    if ((unsigned int)lutCount > 0)
                        LutClearRow();
                }
            }
        }
        if (wasFinishedLocal == 1 && !(playingSample->flags & SFX_LOOP)) {
            playingSample->flags &= ~SFX_ACTIVE;
            finishedTotal++;
        }
        playingSample->offset = offset;
        playingSample->posFrac = lutAcc;
        playingSample = playingSample->next;
        dispatch = MIX;
    }
    *list = playingSample;
    return finishedTotal;
}
#else
unsigned short MixStereoSfxSamples(SfxSample **list)
{
    SfxSample *s;
    unsigned char *data;
    unsigned int offset;
    unsigned int count;
    unsigned short finished;
    unsigned short posFrac;
    unsigned short index;
    unsigned short volume;
    unsigned char row;
    unsigned short pan;
    unsigned short mode;
    unsigned short pitch;
    unsigned char complete;
    unsigned char channel;

    index = 0;
    finished = 0;
    s = *list;
    mode = COPY;

    while (s && index < sfxMaxActive) {
        index++;
        complete = 0;
        volume = s->volume;
        pan = s->pan;

        if (pan <= 15) {
            mixPanSide[0] = 0;
            mixPanSide[1] = 1;
            pan = 15 - pan;
        } else {
            mixPanSide[0] = 1;
            mixPanSide[1] = 0;
            pan -= 15;
        }

        for (channel = 0; channel < 2; channel++) {
            data = s->data;
            offset = s->offset;

            if (channel == 0) {
                if (s->rate == 0x5622)
                    offset += lutPairsA[pan * 2];
                else
                    offset += lutPairsB[pan * 2];
                if (offset > s->length)
                    offset -= s->length;
                row = (unsigned char)volume;
            } else {
                row = lutRows[(lutPairsA[pan * 2 + 1] << 5) + volume];
            }

            lutRowSingle = (unsigned short)(row << 8);
            lutRowA = row;
            lutOut = mixScratch + mixPanSide[channel];
            decodeSrc = (unsigned long)(data + offset);

            pitch = s->pitch;
            if (pitch == 256) {
                count = MIN(s->length - offset, mixBufFrames);
            } else {
                mode |= PITCH;
                lutAcc = s->posFrac;
                /* Read the sample length before publishing the pitch step. */
                count = s->length - offset;
                lutStep = pitch;
                count = MIN((count << 8) / pitch, mixBufFrames);
            }

            if (s->flags & SURROUND) {
                offset = s->offset;
                lutOut = mixScratch;
                decodeSrc = (unsigned long)(data + offset);
                channel++;
                mode |= DOLBY;
            }

            lutCount = count;
            if (count != 0) {
                sfxMixDispatch[mode]();
                offset = (unsigned int)(decodeSrc - (unsigned long)data);
            }

            if (count < mixBufFrames) {
                if (channel == 1) {
                    complete = 1;
                    s->flags |= SFX_ENDED;
                }
                if (s->flags & SFX_LOOP) {
                    offset = 0;
                    lutCount = mixBufFrames - count;
                    if (lutCount != 0) {
                        decodeSrc = (unsigned long)data;
                        sfxMixDispatch[mode]();
                        offset = (unsigned int)(decodeSrc - (unsigned long)data);
                    }
                } else if (index == 1) {
                    lutCount = mixBufFrames - count;
                    if (lutCount != 0)
                        LutClearRow();
                }
            }
        }

        if (complete == 1 && !(s->flags & SFX_LOOP)) {
            s->flags &= ~SFX_ACTIVE;
            finished++;
        }

        posFrac = lutAcc;
        s->offset = offset;
        s->posFrac = posFrac;
        s = s->next;
        mode = MIX;
    }

    *list = s;
    return finished;
}
#endif
#endif

#ifdef EQUIV
/* Image bytes are intentionally uninitialized when the QSND image branch is skipped. */
#ifdef NETPLAY
unsigned short SfxMixerLut(SfxSample **list)
{
    unsigned int sampleCount;
    SfxSample *voice;
    unsigned short sampleDone;
    unsigned char rightImgByte;
    unsigned char *data;
    unsigned int startOffset;
    unsigned char leftImgAux;
    unsigned short mixVolume;
    unsigned short leftBalance;
    unsigned short mixer;
    unsigned int imageBase;
    unsigned char rightPair;
    unsigned char endMarker;
    unsigned int heldFrames;
    unsigned short spanPan;
    unsigned char leftRowIndex;
    void *outBuf;
    unsigned short sampleLimit;

    sampleLimit = 0;
    sampleDone = 0;
    voice = *list;
    mixer = COPY;

    while (voice && sampleLimit < sfxMaxActive) {
        sampleLimit++;
        endMarker = 0;
        data = voice->data;
        startOffset = voice->offset;
        imageBase = startOffset;

        mixVolume = voice->volume;
        spanPan = voice->pan;
        leftBalance = spanPan;

        if (voice->flags & SURROUND) {
            lutRowA = lutRows[mixVolume];
            lutRowB = lutRows[mixVolume];
        } else if (voice->flags & QSND_IMAGE) {
            if (spanPan < 16) {
                spanPan <<= 7;
                mixVolume <<= 2;
                lutRowA = lutQuads[spanPan + mixVolume];
                lutRowB = lutQuads[spanPan + mixVolume + 1];
                leftImgAux = lutQuads[spanPan + mixVolume + 2];
                rightImgByte = lutQuads[spanPan + mixVolume + 3];
            } else {
                spanPan -= 15;
                spanPan = 15 - spanPan;
                spanPan <<= 7;
                mixVolume <<= 2;
                lutRowA = lutQuads[spanPan + mixVolume + 1];
                lutRowB = lutQuads[spanPan + mixVolume];
                leftImgAux = lutQuads[spanPan + mixVolume + 3];
                rightImgByte = lutQuads[spanPan + mixVolume + 2];
            }
        } else {
            if (spanPan < 15) {
                spanPan = 0;
                leftBalance = 15 - leftBalance;
            } else if (spanPan > 15) {
                leftBalance = 0;
                spanPan -= 15;
            } else {
                spanPan = 0;
                leftBalance = 0;
            }
            spanPan <<= 5;
            leftBalance <<= 5;
            lutRowA = lutRows[spanPan + mixVolume];
            lutRowB = lutRows[leftBalance + mixVolume];
        }

        leftRowIndex = lutRowA;
        rightPair = lutRowB;
        lutOut = mixScratch;
        decodeSrc = startOffset + (unsigned long)data;

        if (voice->pitch == 256) {
            sampleCount = lutCount = MIN(voice->length - startOffset, mixBufFrames);
            heldFrames = lutCount;
        } else {
            mixer |= PITCH;
            lutAcc = voice->posFrac;
            lutStep = voice->pitch;
            sampleCount = lutCount = MIN(((voice->length - startOffset) << 8) / lutStep, mixBufFrames);
            heldFrames = lutCount;
        }

        if (voice->flags & SURROUND) {
            mixer |= DOLBY;
        } else if (lutRowB == lutRowA) {
            mixer |= 0x20;
        }

        if ((unsigned int)lutCount > 0) {
            if ((voice->flags & QSND_IMAGE) && !(voice->flags & SURROUND)) {
                if ((lutRowA > 0 && lutRowB > 0)
                    || (lutRowA > 0 && rightImgByte == 0)
                    || (lutRowB > 0 && leftImgAux == 0)
                    || (lutRowA == 0 && lutRowB == 0)) {
                    sfxMixDispatch[mixer]();
                } else if (lutRowA > 0) {
                    lutRowSingle = lutRowA << 8;
                    sfxMixDispatch[mixer | ONE_CHNL]();
                } else {
                    lutRowSingle = lutRowB << 8;
                    lutOut += 4;
                    sfxMixDispatch[mixer | ONE_CHNL]();
                    lutOut -= 4;
                }
            } else {
                sfxMixDispatch[mixer]();
            }

            startOffset = (unsigned int)(decodeSrc - (unsigned long)data);

            if ((voice->flags & QSND_IMAGE) && !(voice->flags & SURROUND)) {
                lutRowA = leftImgAux;
                lutRowB = rightImgByte;
                lutAcc = voice->posFrac;
                lutStep = voice->pitch;
                lutCount = sampleCount;
                if (lutRowA != 0 || lutRowB != 0) {
                    decodeSrc = (unsigned long)voice->rightData + imageBase;
                    if (lutRowA == 0) {
                        lutRowSingle = lutRowB << 8;
                        lutOut = mixScratch;
                        lutOut += 4;
                        sfxMixDispatch[mixer | ONE_CHNL]();
                        lutOut -= 4;
                    }
                    if (lutRowB == 0) {
                        lutRowSingle = lutRowA << 8;
                        lutOut = mixScratch;
                        sfxMixDispatch[mixer | ONE_CHNL]();
                    }
                }
            }
        }

        outBuf = lutOut;

        if (heldFrames < mixBufFrames) {
            endMarker = 1;
            voice->flags |= SFX_ENDED;
            if (voice->flags & SFX_LOOP) {
                startOffset = 0;
                sampleCount = lutCount = mixBufFrames - heldFrames;
                heldFrames = lutCount;
                if ((unsigned int)lutCount > 0) {
                    lutRowA = leftRowIndex;
                    lutRowB = rightPair;
                    decodeSrc = (unsigned long)voice->data;
                    if ((voice->flags & QSND_IMAGE) && !(voice->flags & SURROUND)) {
                        if ((lutRowA > 0 && lutRowB > 0)
                            || (lutRowA > 0 && rightImgByte == 0)
                            || (lutRowB > 0 && leftImgAux == 0)
                            || (lutRowA == 0 && lutRowB == 0)) {
                            sfxMixDispatch[mixer]();
                        } else if (lutRowA > 0) {
                            lutRowSingle = lutRowA << 8;
                            sfxMixDispatch[mixer | ONE_CHNL]();
                        } else {
                            lutRowSingle = lutRowB << 8;
                            lutOut += 4;
                            sfxMixDispatch[mixer | ONE_CHNL]();
                            lutOut -= 4;
                        }
                    } else {
                        sfxMixDispatch[mixer]();
                    }

                    startOffset = (unsigned int)(decodeSrc - (unsigned long)data);

                    if ((voice->flags & QSND_IMAGE) && !(voice->flags & SURROUND)) {
                        lutRowA = leftImgAux;
                        lutRowB = rightImgByte;
                        lutAcc = voice->posFrac;
                        lutStep = voice->pitch;
                        lutCount = sampleCount;
                        if (lutRowA != 0 || lutRowB != 0) {
                            decodeSrc = (unsigned long)voice->rightData;
                            if (lutRowA == 0) {
                                lutRowSingle = lutRowB << 8;
                                lutOut = outBuf;
                                lutOut += 4;
                                sfxMixDispatch[mixer | ONE_CHNL | MIX]();
                                lutOut -= 4;
                            }
                            if (lutRowB == 0) {
                                lutRowSingle = lutRowA << 8;
                                lutOut = outBuf;
                                sfxMixDispatch[mixer | ONE_CHNL | MIX]();
                            }
                        }
                    }
                }
            } else if (sampleLimit == 1) {
                lutCount = mixBufFrames - heldFrames;
                if ((unsigned int)lutCount > 0)
                    ClearLutPairs();
            }

            if (endMarker == 1 && !(voice->flags & SFX_LOOP)) {
                voice->flags &= ~SFX_ACTIVE;
                sampleDone++;
            }
        }

        voice->offset = startOffset;
        voice->posFrac = lutAcc;
        voice = voice->next;
        mixer = MIX;
    }

    *list = voice;
    return sampleDone;
}
#else
unsigned short SfxMixerLut(SfxSample **list)
{
    register unsigned short mode;
    SfxSample *s;
    struct { unsigned char leftStore; unsigned char rightStore; unsigned char leftImg; unsigned char rightImg; } rows;
    /* The original keeps these per-channel values together in one stack record. */
    struct {
        unsigned short finished;
        unsigned short index;
        unsigned int offset;
        union { unsigned int imgOffset; void *outStore; } cursor;
        unsigned int countStore;
        unsigned short surround;
        unsigned char *data;
    } saved;
    unsigned int count;
    unsigned char *leftRow;
    unsigned short pitch;
    unsigned short leftPan;
    unsigned short volume;

    saved.index = 0;
    saved.finished = 0;
    s = *list;
    mode = COPY;

    while (s) {
        if (saved.index >= sfxMaxActive)
            break;
        saved.index++;
        leftRow = &rows.leftStore;
        saved.data = s->data;
        saved.offset = s->offset;
        saved.cursor.imgOffset = saved.offset;

        volume = s->volume;
        leftPan = s->pan;

        saved.surround = s->flags & SURROUND;
        if (saved.surround) {
            *leftRow = lutRows[volume];
            lutRowB = lutRows[volume];
        } else if (s->flags & QSND_IMAGE) {
            if (leftPan < 16) {
                leftPan <<= 7;
                volume <<= 2;
                lutRowB = lutQuads[leftPan + volume + 1];
                rows.leftImg = lutQuads[leftPan + volume + 2];
                rows.leftStore = lutQuads[leftPan + volume];
                rows.rightImg = lutQuads[leftPan + volume + 3];
            } else {
                leftPan -= 15;
                leftPan = 15 - leftPan;
                leftPan <<= 7;
                volume <<= 2;
                *leftRow = lutQuads[leftPan + volume + 1];
                lutRowB = lutQuads[leftPan + volume];
                rows.leftImg = lutQuads[leftPan + volume + 3];
                rows.rightImg = lutQuads[leftPan + volume + 2];
            }
        } else {
            unsigned short rightPan = leftPan;
            if (leftPan < 15) {
                leftPan = 0;
                rightPan = 15 - rightPan;
            } else if (leftPan > 15) {
                rightPan = 0;
                leftPan -= 15;
            } else {
                leftPan = 0;
                rightPan = 0;
            }
            leftPan <<= 5;
            rightPan <<= 5;
            *leftRow = lutRows[leftPan + volume];
            lutRowB = lutRows[rightPan + volume];
        }

        pitch = s->pitch;
        rows.rightStore = lutRowB;
        lutOut = mixScratch;
        decodeSrc = (unsigned long)saved.data + saved.offset;

        if (pitch == 256) {
            count = MIN(s->length - saved.offset, mixBufFrames);
            saved.countStore = count;
        } else {
            /* Flag bits are set in mode's low byte only; its high byte carries into the dispatch index. */
            mode = (mode & 0xff00) | ((unsigned char)mode | PITCH);
            lutAcc = s->posFrac;
            lutStep = pitch;
            count = MIN(((s->length - saved.offset) << 8) / pitch, mixBufFrames);
            saved.countStore = count;
        }

        if (saved.surround) {
            mode |= DOLBY;
        } else if (*leftRow == lutRowB) {
            mode = (mode & 0xff00) | ((unsigned char)mode | 0x20);
        }

        lutCount = count;
        lutRowA = *leftRow;
        if (lutCount != 0) {
            if ((s->flags & QSND_IMAGE) && !saved.surround) {
                /* lutRowA is republished before each image test, as in the original. */
                if ((*leftRow > 0 && lutRowB > 0)
                    || (*leftRow > 0 && (lutRowA = *leftRow, rows.rightImg == 0))
                    || (lutRowB > 0 && (lutRowA = *leftRow, rows.leftImg == 0))
                    || (*leftRow == 0 && (lutRowA = *leftRow, lutRowB == 0))) {
                    sfxMixDispatch[mode]();
                } else if ((lutRowA = *leftRow) > 0) {
                    lutRowSingle = lutRowA << 8;
                    sfxMixDispatch[mode | ONE_CHNL]();
                } else {
                    lutRowSingle = lutRowB << 8;
                    lutOut = (char *)lutOut + 4;
                    sfxMixDispatch[mode | ONE_CHNL]();
                    lutOut = (char *)lutOut - 4;
                }
            } else {
                sfxMixDispatch[mode]();
            }

            saved.offset = (unsigned int)(decodeSrc - (unsigned long)saved.data);

            if ((s->flags & QSND_IMAGE) && !(s->flags & SURROUND)) {
                lutAcc = s->posFrac;
                lutRowA = rows.leftImg;
                lutRowB = rows.rightImg;
                lutStep = s->pitch;
                lutCount = saved.countStore;
                if (lutRowA != 0 || lutRowB != 0) {
                    decodeSrc = (unsigned long)s->rightData + saved.cursor.imgOffset;
                    if (lutRowA == 0) {
                        lutRowSingle = lutRowB << 8;
                        lutOut = mixScratch;
                        lutOut = (char *)lutOut + 4;
                        sfxMixDispatch[mode | ONE_CHNL]();
                        lutOut = (char *)lutOut - 4;
                    }
                    if (lutRowB == 0) {
                        lutRowSingle = lutRowA << 8;
                        lutOut = mixScratch;
                        sfxMixDispatch[mode | ONE_CHNL]();
                    }
                }
            }
        }

        saved.cursor.outStore = lutOut;

        {
            unsigned int frames = mixBufFrames;
            if (count < frames) {
                unsigned short flags = s->flags | SFX_ENDED;
                s->flags = flags;
                if (flags & SFX_LOOP) {
                    saved.offset = 0;
                    lutCount = frames - count;
                    saved.countStore = lutCount;
                    count = lutCount;
                    if (lutCount != 0) {
                        lutRowA = *leftRow;
                        lutRowB = rows.rightStore;
                        decodeSrc = (unsigned long)s->data;
                        if ((flags & QSND_IMAGE) && !(flags & SURROUND)) {
                            if ((*leftRow > 0 && rows.rightStore > 0)
                                || (*leftRow > 0 && rows.rightImg == 0)
                                || (rows.rightStore > 0 && rows.leftImg == 0)
                                || (*leftRow == 0 && rows.rightStore == 0)) {
                                sfxMixDispatch[mode]();
                            } else if (rows.leftStore > 0) {
                                lutRowSingle = rows.leftStore << 8;
                                sfxMixDispatch[mode | ONE_CHNL]();
                            } else {
                                lutRowSingle = rows.rightStore << 8;
                                lutOut = (char *)lutOut + 4;
                                sfxMixDispatch[mode | ONE_CHNL]();
                                lutOut = (char *)lutOut - 4;
                            }
                        } else {
                            sfxMixDispatch[mode]();
                        }

                        saved.offset = (unsigned int)(decodeSrc - (unsigned long)saved.data);

                        if ((s->flags & QSND_IMAGE) && !(s->flags & SURROUND)) {
                            lutAcc = s->posFrac;
                            lutRowA = rows.leftImg;
                            lutRowB = rows.rightImg;
                            lutStep = s->pitch;
                            lutCount = saved.countStore;
                            if (lutRowA != 0 || lutRowB != 0) {
                                decodeSrc = (unsigned long)s->rightData;
                                if (lutRowA == 0) {
                                    lutRowSingle = lutRowB << 8;
                                    lutOut = (char *)saved.cursor.outStore + 4;
                                    sfxMixDispatch[mode | ONE_CHNL | MIX]();
                                    lutOut = (char *)lutOut - 4;
                                }
                                if (lutRowB == 0) {
                                    lutRowSingle = lutRowA << 8;
                                    lutOut = saved.cursor.outStore;
                                    mode = (mode & 0xff00) | ((unsigned char)mode | ONE_CHNL | MIX);
                                    sfxMixDispatch[mode]();
                                }
                            }
                        }
                    }
                } else if (saved.index == 1) {
                    lutCount = mixBufFrames - count;
                    if (lutCount != 0)
                        ClearLutPairs();
                }

                if (!(s->flags & SFX_LOOP)) {
                    s->flags &= ~SFX_ACTIVE;
                    saved.finished++;
                }
            }
        }

        s->posFrac = lutAcc;
        s->offset = saved.offset;
        s = s->next;
        mode = MIX;
    }

    *list = s;
    return saved.finished;
}
#endif
#endif

#ifdef EQUIV
/* Four-channel mixing keeps the original cached flag and sample-length reads. */
#ifdef NETPLAY
/* Equivalent with a reversed unsigned comparison. The +0 in the rear gain product
   preserves the original /Od operand load order. */
unsigned short SfxMixFourChannel(SfxSample **list)
{
    unsigned char *loopSource;
    SfxSample *currentSample;
    short frontPos;
    unsigned int offsetInSample;
    unsigned short completedCount;
    unsigned short dispatchSlot;
    unsigned short channelOut;
    unsigned short baseVolume;
    unsigned short maxIndex;
    unsigned char activeMarker;
    unsigned int remainingPan;
    unsigned short rightGainA;
    unsigned short surroundRearPan;
    unsigned int frameNum;

    maxIndex = 0;
    completedCount = 0;
    currentSample = *list;
    sfxMixChannels[0].dispatchFlags = 0;
    sfxMixChannels[1].dispatchFlags = 1;

    while (currentSample && maxIndex < sfxMaxActive) {
        maxIndex++;
        activeMarker = 0;
        baseVolume = currentSample->volume;

        if (currentSample->flags & 0x40) {
            surroundRearPan = currentSample->fbPan;
            frontPos = currentSample->pan;
            if ((short)surroundRearPan < 15) {
                rightGainA = baseVolume;
                baseVolume = (unsigned short)((unsigned int)(((short)surroundRearPan + 0) * (int)baseVolume) / 15);
            } else if ((short)surroundRearPan > 15) {
                rightGainA = (unsigned short)((unsigned int)((31 - (short)surroundRearPan) * (int)baseVolume) / 15);
            } else {
                rightGainA = baseVolume;
            }

            if ((short)frontPos < 15) {
                sfxMixChannels[1].rowA = baseVolume;
                sfxMixChannels[1].rowB = baseVolume + ((15 - frontPos) << 5);
            } else if ((short)frontPos > 15) {
                sfxMixChannels[1].rowB = baseVolume;
                sfxMixChannels[1].rowA = baseVolume + ((frontPos - 15) << 5);
            } else {
                sfxMixChannels[1].rowA = baseVolume;
                sfxMixChannels[1].rowB = baseVolume;
            }
            sfxMixChannels[0].dispatchFlags |= DOLBY;
            sfxMixChannels[0].rowA = rightGainA;
            sfxMixChannels[0].rowB = rightGainA;
            sfxMixChannels[0].sampleOffset = currentSample->frontOffset;
            sfxMixChannels[1].sampleOffset = currentSample->offset;
        } else if (currentSample->flags & SURROUND) {
            sfxMixChannels[0].rowA = baseVolume;
            sfxMixChannels[0].rowB = baseVolume;
            sfxMixChannels[0].dispatchFlags |= DOLBY;
            sfxMixChannels[0].sampleOffset = currentSample->offset;
            sfxMixChannels[1].dispatchFlags = SKIP_MIX;
        } else {
            frontPos = currentSample->pan;
            if ((short)frontPos < 15) {
                sfxMixChannels[0].rowA = baseVolume;
                sfxMixChannels[0].rowB = baseVolume + ((15 - frontPos) << 5);
            } else if ((short)frontPos > 15) {
                sfxMixChannels[0].rowB = baseVolume;
                sfxMixChannels[0].rowA = baseVolume + ((frontPos - 15) << 5);
            } else {
                sfxMixChannels[0].rowA = baseVolume;
                sfxMixChannels[0].rowB = baseVolume;
            }
            sfxMixChannels[0].sampleOffset = currentSample->offset;
            sfxMixChannels[1].dispatchFlags = SKIP_MIX;
        }

        for (channelOut = 0; channelOut < 2; channelOut++) {
            if (sfxMixChannels[channelOut].dispatchFlags != SKIP_MIX) {
                loopSource = currentSample->data;
                offsetInSample = sfxMixChannels[channelOut].sampleOffset;
                lutOut = mixScratch;
                decodeSrc = (unsigned long)(loopSource + offsetInSample);
                frameNum = mixBufFrames;
                lutAcc = channelOut == 0 ? currentSample->posFrac : currentSample->rearPosFrac;

                lutStep = currentSample->pitch;
                if (channelOut == 1) {
                    if (!(currentSample->flags & 0x4000)) {
                        sfxMixChannels[0].sampleOffset = sfxMixChannels[1].sampleOffset;
                        lutOut += 0x200;
                        frameNum -= 0x80;
                        currentSample->flags |= 0x4000;
                    }
                }

                if (currentSample->pitch == 256) {
                    lutCount = MIN(currentSample->length - offsetInSample, frameNum);
                    remainingPan = lutCount;
                } else {
                    sfxMixChannels[channelOut].dispatchFlags |= PITCH;
                    lutCount = MIN(((currentSample->length - offsetInSample) << 8) / lutStep, frameNum);
                    remainingPan = lutCount;
                }

                lutRowA = lutRows[sfxMixChannels[channelOut].rowA];
                lutRowB = lutRows[sfxMixChannels[channelOut].rowB];
                if ((unsigned int)lutCount > 1) {
                    dispatchSlot = sfxMixChannels[channelOut].dispatchFlags;
                    sfxMixDispatch[dispatchSlot]();
                    offsetInSample = (unsigned int)(decodeSrc - (unsigned long)loopSource);
                }

                if (remainingPan < (frameNum + 0)) {
                    activeMarker = 1;
                    currentSample->flags |= SFX_ENDED;
                    if (currentSample->flags & SFX_LOOP) {
                        offsetInSample = 0;
                        /* Both arms are identical in the original (a leftover from a per-pitch version). */
                        if (currentSample->pitch == 256) {
                            remainingPan = lutCount = frameNum - remainingPan;
                        } else {
                            remainingPan = lutCount = frameNum - remainingPan;
                        }
                        if ((unsigned int)lutCount > 1) {
                            decodeSrc = (unsigned long)loopSource;
                            sfxMixDispatch[sfxMixChannels[channelOut].dispatchFlags]();
                            offsetInSample = (unsigned int)(decodeSrc - (unsigned long)loopSource);
                        }
                    } else if (maxIndex == 1 && channelOut == 0) {
                        lutCount = frameNum - remainingPan;
                        if ((unsigned int)lutCount > 0)
                            ClearLutPairs();
                    }
                    if (activeMarker == 1 && !(currentSample->flags & SFX_LOOP) && channelOut == 0) {
                        currentSample->flags &= ~SFX_ACTIVE;
                        completedCount++;
                    }
                }

                sfxMixChannels[channelOut].sampleOffset = offsetInSample;
                if (channelOut == 0)
                    currentSample->posFrac = lutAcc;
                else
                    currentSample->rearPosFrac = lutAcc;
            }
        }

        if (currentSample->flags & 0x40) {
            currentSample->frontOffset = sfxMixChannels[0].sampleOffset;
            currentSample->offset = sfxMixChannels[1].sampleOffset;
        } else if (currentSample->flags & 0x10) {
            currentSample->offset = sfxMixChannels[0].sampleOffset;
        } else {
            /* Both arms are identical in the original. */
            currentSample->offset = sfxMixChannels[0].sampleOffset;
        }

        currentSample = currentSample->next;
        sfxMixChannels[0].dispatchFlags = MIX;
        sfxMixChannels[1].dispatchFlags = MIX;
    }

    *list = currentSample;
    return completedCount;
}
#else
unsigned short SfxMixFourChannel(SfxSample **list)
{
    SfxSample *s;
    unsigned short finished;
    unsigned short index;
    unsigned short channel;
    unsigned short flags;
    unsigned short volume;
    unsigned short pan;
    unsigned short fbPan;
    unsigned short frontGain;
    unsigned short backGain;

    finished = 0;
    index = 0;
    sfxMixChannels[0].dispatchFlags = 0;
    sfxMixChannels[1].dispatchFlags = 1;
    s = *list;

    while (s && index < sfxMaxActive) {
        flags = s->flags;
        volume = s->volume;
        index++;

        if (flags & 0x40) {
            int frontSampleOffset;
            unsigned int channelSampleOffset;

            fbPan = s->fbPan;
            pan = s->pan;
            if ((short)fbPan < 15) {
                frontGain = volume;
                backGain = (unsigned short)((unsigned int)((short)fbPan * (int)volume) / 15);
            } else if ((short)fbPan > 15) {
                backGain = volume;
                frontGain = (unsigned short)((unsigned int)((31 - (short)fbPan) * (int)volume) / 15);
            } else {
                frontGain = volume;
                backGain = volume;
            }

            if ((short)pan < 15) {
                sfxMixChannels[1].rowA = backGain;
                sfxMixChannels[1].rowB = backGain + ((15 - pan) << 5);
            } else if ((short)pan > 15) {
                sfxMixChannels[1].rowB = backGain;
                sfxMixChannels[1].rowA = backGain + ((pan - 15) << 5);
            } else {
                sfxMixChannels[1].rowA = backGain;
                sfxMixChannels[1].rowB = backGain;
            }
            sfxMixChannels[0].rowA = frontGain;
            frontSampleOffset = s->frontOffset;
            sfxMixChannels[0].dispatchFlags |= DOLBY;
            sfxMixChannels[0].rowB = frontGain;
            channelSampleOffset = s->offset;
            sfxMixChannels[0].sampleOffset = frontSampleOffset;
            sfxMixChannels[1].sampleOffset = channelSampleOffset;
        } else if (flags & SURROUND) {
            sfxMixChannels[0].rowA = volume;
            sfxMixChannels[0].rowB = volume;
            sfxMixChannels[0].dispatchFlags |= DOLBY;
            sfxMixChannels[1].dispatchFlags = SKIP_MIX;
            sfxMixChannels[0].sampleOffset = s->offset;
        } else {
            pan = s->pan;
            if ((short)pan < 15) {
                sfxMixChannels[0].rowA = volume;
                sfxMixChannels[0].rowB = volume + ((15 - pan) << 5);
            } else if ((short)pan > 15) {
                sfxMixChannels[0].rowB = volume;
                sfxMixChannels[0].rowA = volume + ((pan - 15) << 5);
            } else {
                sfxMixChannels[0].rowA = volume;
                sfxMixChannels[0].rowB = volume;
            }
            sfxMixChannels[1].dispatchFlags = SKIP_MIX;
            sfxMixChannels[0].sampleOffset = s->offset;
        }

        for (channel = 0; channel < 2; channel++) {
            unsigned int frames;
            unsigned int remaining;
            unsigned int sourceOffset;
            unsigned char *data;
            unsigned short pitch;

            if (sfxMixChannels[channel].dispatchFlags == SKIP_MIX)
                continue;

            data = s->data;
            sourceOffset = sfxMixChannels[channel].sampleOffset;
            frames = mixBufFrames;
            lutOut = mixScratch;
            decodeSrc = (unsigned long)(data + sourceOffset);
            pitch = s->pitch;
            lutAcc = channel == 0 ? s->posFrac : s->rearPosFrac;

            if (channel == 1) {
                flags = s->flags;
                if (!(flags & 0x4000)) {
                    frames = mixBufFrames;
                    sfxMixChannels[0].sampleOffset = sfxMixChannels[1].sampleOffset;
                    flags |= 0x4000;
                    s->flags = flags;
                    lutOut = (char *)mixScratch + 0x200;
                    frames -= 0x80;
                }
            }

            lutStep = pitch;
            if (pitch == 256) {
                remaining = s->length - sourceOffset;
                if (remaining >= frames)
                    remaining = frames;
            } else {
                /* The original writes lutStep again on the pitched path. */
                lutStep = pitch;
                remaining = s->length - sourceOffset;
                sfxMixChannels[channel].dispatchFlags |= PITCH;
                remaining = (remaining << 8) / pitch;
                if (remaining >= frames)
                    remaining = frames;
            }

            {
                unsigned char leftRow = lutRows[sfxMixChannels[channel].rowA];
                unsigned short rightRow = sfxMixChannels[channel].rowB;
                lutRowA = leftRow;
                lutCount = remaining;
                lutRowB = lutRows[rightRow];
            }
            if (remaining > 1) {
                sfxMixDispatch[sfxMixChannels[channel].dispatchFlags]();
                sourceOffset = (unsigned int)(decodeSrc - (unsigned long)data);
            }

            if (remaining < frames) {
                s->flags |= SFX_ENDED;
                if (s->flags & SFX_LOOP) {
                    sourceOffset = 0;
                    remaining = frames - remaining;
                    lutCount = remaining;
                    if (remaining > 1) {
                        decodeSrc = (unsigned long)data;
                        sfxMixDispatch[sfxMixChannels[channel].dispatchFlags]();
                        sourceOffset = (unsigned int)(decodeSrc - (unsigned long)data);
                    }
                } else if (index == 1 && channel == 0) {
                    remaining = frames - remaining;
                    lutCount = remaining;
                    if (remaining)
                        ClearLutPairs();
                }
                if (!(s->flags & SFX_LOOP) && channel == 0) {
                    s->flags &= ~SFX_ACTIVE;
                    finished++;
                }
            }

            {
                unsigned short posFrac = lutAcc;
                sfxMixChannels[channel].sampleOffset = sourceOffset;
                if (channel == 0)
                    s->posFrac = posFrac;
                else
                    s->rearPosFrac = posFrac;
            }
        }

        if (s->flags & 0x40) {
            unsigned int frontOffset = sfxMixChannels[0].sampleOffset;
            unsigned int backOffset = sfxMixChannels[1].sampleOffset;
            s->frontOffset = frontOffset;
            s->offset = backOffset;
        } else {
            s->offset = sfxMixChannels[0].sampleOffset;
        }

        sfxMixChannels[0].dispatchFlags = MIX;
        sfxMixChannels[1].dispatchFlags = MIX;
        s = s->next;
    }

    *list = s;
    return finished;
}
#endif
#endif

/* Frees the tables that SfxBuildMixTables allocated for the current lutMode. */
void SfxFreeMixTables(void)
{
    GlobalFree(quantTable);
    if (lutMode >= 2)
        GlobalFree(lutRows);
    if (lutMode == 3) {
        GlobalFree(lutPairsA);
        GlobalFree(lutPairsB);
    }
    if (lutMode == 4)
        GlobalFree(lutQuads);
}

#ifdef NETPLAY
#pragma intrinsic(log)
#pragma intrinsic(exp)
/* Builds the NET 16-bit sample quantisation table. */
unsigned short SfxBuildQuantTable16(void)
{
    unsigned short volumes[VOLUME_LEVELS];
    float intermediate;
    unsigned short sample;
    unsigned short volume;
    float maxSamp;
    float volumeInput;
    float expValue;
    unsigned short volumeIndex;
    float rangeLog;

    for (volume = 0; volume < VOLUME_LEVELS; volume++)
        volumes[volume] = (unsigned short)(((double)volume * 32767.0) / (VOLUME_LEVELS - 1));

    quantTable = GlobalAlloc(GMEM_ZEROINIT, VOLUME_LEVELS * 256 * sizeof(int));
    if (quantTable == NULL)
        return 1;

    rangeLog = log(88.6);
    volume = 0;
    for (volumeIndex = 0; volumeIndex < VOLUME_LEVELS; volumeIndex++) {
        maxSamp = volumes[volumeIndex];
        for (sample = 0; sample < 256; sample++) {
            volumeInput = ((float)sample - 128.0f) / 128.0f;
            expValue = (rangeLog * volumeInput) / (volumeInput >= 0.0f ? 1 : -1);
            intermediate = (exp(expValue) - 1) / 87.6f;
            quantTable[volume] = (volumeInput >= 0.0f ? 1 : -1) * (int)(intermediate * maxSamp);
            volume++;
        }
    }
    return 0;
}

/* Builds the NET table. */
unsigned short SfxBuildQuantTable8(void)
{
    float delta;
    float value;
    unsigned short maxVols[VOLUME_LEVELS];
    unsigned short sampleIndex;
    unsigned short index;
    unsigned short volumeIndex;
    float maximum;

    for (index = 0; index < VOLUME_LEVELS; index++)
        maxVols[index] = (unsigned short)(((double)index * 127.0) / (VOLUME_LEVELS - 1));

    quantTable = GlobalAlloc(GMEM_ZEROINIT, VOLUME_LEVELS * 256 * sizeof(int));
    if (quantTable == NULL)
        return 1;

    index = 0;
    for (volumeIndex = 0; volumeIndex < VOLUME_LEVELS; volumeIndex++) {
        maximum = maxVols[volumeIndex];
        delta = ((2.0f * maximum) + 1.0f) / 256.0f;
        value = -(maximum + 1.0f);
        for (sampleIndex = 0; sampleIndex < 256; sampleIndex++) {
            quantTable[index] = (int)value;
            value = value + delta;
            index++;
        }
    }
    return 0;
}

unsigned short SfxBuildPanTable(void)
{
    float speed;
    float curve;
    unsigned short pan;
    unsigned short column;
    float panSteps[16];

    lutRows = GlobalAlloc(GMEM_ZEROINIT, 0x200);
    if (lutRows == NULL)
        return 1;

    for (pan = 0; pan < 16; pan++)
        panSteps[pan] = (16.0f - (__int64)pan) / 16.0f;

    for (pan = 0; pan < 16; pan++) {
        speed = panSteps[pan];
        curve = 0.0f;
        for (column = 0; column < 32; column++) {
            lutRows[(pan << 5) + column] = (unsigned char)curve;
            curve = curve + speed;
        }
    }
    return 0;
}

unsigned short func_46c7ab(void)
{
    unsigned short i;

    lutPairsA = GlobalAlloc(GMEM_ZEROINIT, 32);
    lutPairsB = GlobalAlloc(GMEM_ZEROINIT, 32);
    if (lutPairsA == NULL || lutPairsB == NULL)
        return 1;

    for (i = 0; i < 16; i++) {
        lutPairsA[i * 2] = steepNibbles[i];
        lutPairsA[i * 2 + 1] = pairGain[i];
    }
    for (i = 0; i < 16; i++) {
        lutPairsB[i * 2] = linearNibbles[i];
        lutPairsB[i * 2 + 1] = pairGain[i];
    }
    return 0;
}

#endif

#ifdef EQUIV
#ifdef NETPLAY
unsigned short func_46c8ce(void)
{
    unsigned short i;
    unsigned short j;

    lutQuads = GlobalAlloc(GMEM_ZEROINIT, 0x800);
    if (lutQuads == NULL)
        return 1;

    for (i = 0; i < 16; i++) {
        for (j = 0; j < 32; j++) {
            lutQuads[j * 4 + (i << 7)] = (unsigned char)((float)j * quadGains[i][0]);
            lutQuads[j * 4 + (i << 7) + 1] = (unsigned char)((float)j * quadGains[i][1]);
            lutQuads[j * 4 + (i << 7) + 2] = (unsigned char)((float)j * quadGains[i][2]);
            lutQuads[j * 4 + (i << 7) + 3] = (unsigned char)((float)j * quadGains[i][3]);
        }
    }
    return 0;
}
#endif
#else
#ifdef NETPLAY
unsigned short func_46c8ce(void)
{
    unsigned short i;
    unsigned short j;

    lutQuads = GlobalAlloc(GMEM_ZEROINIT, 0x800);
    if (lutQuads == NULL)
        return 1;

    for (i = 0; i < 16; i++) {
        for (j = 0; j < 32; j++) {
            lutQuads[j * 4 + (i << 7)] = (unsigned char)((float)j * quadGains[i][0]);
            lutQuads[j * 4 + (i << 7) + 1] = (unsigned char)((float)j * quadGains[i][1]);
            lutQuads[j * 4 + (i << 7) + 2] = (unsigned char)((float)j * quadGains[i][2]);
            lutQuads[j * 4 + (i << 7) + 3] = (unsigned char)((float)j * quadGains[i][3]);
        }
    }
    return 0;
}
#endif
#endif

/* Debug dump: clamps the mixed stereo samples to 16 bits into decodeDst and appends mixCount
   bytes of them to mixout.raw. */
#ifndef NETPLAY
void SfxDumpMixedSamples(void)
{
    unsigned int remaining;
    short *destination;
    short *start;
    int *source;
    int value;
    FILE *file;

    remaining = mixCount;
    destination = (short *)decodeDst;
    start = destination;
    source = mixSrc;
    do {
        value = source[0];
        *destination = (short)value;
        if ((unsigned int)value + 0x8000u > 0x10000u) {
            if (value > 0x7fff)
                *destination = 0x7fff;
            else
                *destination = (short)0x8000;
        }
        destination++;

        value = source[1];
        source++;
        *destination = (short)value;
        if ((unsigned int)value + 0x8000u > 0x10000u) {
            if (value > 0x7fff)
                *destination = 0x7fff;
            else
                *destination = (short)0x8000;
        }
        destination++;
        source++;
    } while (--remaining != 0);

    file = fopen("mixout.raw", "a+");
    if (file != NULL) {
        fwrite(start, mixCount, 1, file);
        fclose(file);
    }
}
#endif
#ifdef EQUIV
#ifdef NETPLAY
void SfxDumpMixedSamples(void)
{
    int *input;
    unsigned int count;
    short *output;
    short *base;
    FILE *file;

    input = mixSrc;
    output = (short *)decodeDst;
    base = (short *)decodeDst;
    count = mixCount;
    do {
        *output = (short)input[0];
        if ((unsigned int)input[0] + 0x8000u > 0x10000u) {
            if (input[0] > 0x7fff)
                *output = 0x7fff;
            else
                *output = (short)0x8000;
        }
        output++;
        input++;

        *output = (short)input[0];
        if ((unsigned int)input[0] + 0x8000u > 0x10000u) {
            if (input[0] > 0x7fff)
                *output = 0x7fff;
            else
                *output = (short)0x8000;
        }
        output++;
        input++;
        count--;
    } while (count != 0);

    file = fopen("mixout.raw", "a+");
    if (file == NULL) {
    } else {
        fwrite(base, mixCount, 1, file);
        fclose(file);
    }
}
#endif
#endif
