#include "mpeg.h"

#define MPEG_AUDIO_BLOCK_LIMIT 0x80
#define MPEG_AUDIO_WAVEFORMAT_SIZE 0x12
#define MPEG_AUDIO_SCRATCH_SIZE 0x80000
#define MPEG_AUDIO_TIMER_INTERVAL 0x22
#define MPEG_AUDIO_TIMER_RESOLUTION 8
#define MPEG_AUDIO_WRITE_CHUNK 0x4000
#define MPEG_AUDIO_FRAMES_PER_CHUNK 0x100
#define MPEG_AUDIO_VOLUME_STEPS 0x100
#define MPEG_AUDIO_SAMPLE_RATE 44100
#define MPEG_AUDIO_BITS_PER_SAMPLE 16
#define MPEG_AUDIO_CHANNELS 2
#define DSBPLAY_LOOPING 1
#define TIME_PERIODIC 1

typedef void (__stdcall *TimerCallback)(unsigned, unsigned, unsigned long, unsigned long, unsigned long);
__declspec(dllimport) unsigned __stdcall timeSetEvent(unsigned delay, unsigned resolution, TimerCallback proc, unsigned long user, unsigned flags);
__declspec(dllimport) unsigned __stdcall timeKillEvent(unsigned id);

void mpegDecodeAudioSamples(void);
double pow(double x, double y);

/* Validates the DirectSound format and initializes the audio block queue. */
int mpegInitAudioQueue(unsigned long numBlocks, unsigned long blockSize, void *mem)
{
    unsigned char *p;
    WaveFormat wfx;
    DSBufferCaps caps;

    sndActive = 0;
    sndTimer = 0;
    if (numBlocks > MPEG_AUDIO_BLOCK_LIMIT)
        return 1;
    mpegSndResult = mpegSoundBuf->lpVtbl->GetFormat(mpegSoundBuf, &wfx, MPEG_AUDIO_WAVEFORMAT_SIZE, 0);
    if (wfx.samplesPerSec != MPEG_AUDIO_SAMPLE_RATE)
        return 1;
    if (wfx.bitsPerSample != MPEG_AUDIO_BITS_PER_SAMPLE)
        return 1;
    if (wfx.channels != MPEG_AUDIO_CHANNELS)
        return 1;
    caps.size = sizeof(DSBufferCaps);
    mpegSndResult = mpegSoundBuf->lpVtbl->GetCaps(mpegSoundBuf, &caps);
    sndBufferBytes = caps.bufferBytes;
    sndHalfBytes = sndBufferBytes >> 1;
    sndQuarterBytes = sndHalfBytes >> 1;
    sndNumBlocks = numBlocks;
    p = mem;
    for (sndCurBlock = 0; sndNumBlocks > sndCurBlock; sndCurBlock++) {
        sndBlocks[sndCurBlock].data = p;
        p += blockSize + SND_FRAME_BYTES;
        if (sndBlocks[sndCurBlock].data == 0)
            return 1;
        sndBlocks[sndCurBlock].fill = 0;
        sndBlocks[sndCurBlock].frames = 0;
        sndBlocks[sndCurBlock].ready = 0;
    }
    mpegBuildVolumeTable(p);
    p += MPEG_AUDIO_SCRATCH_SIZE;
    sndUnused1 = 0;
    sndUnused2 = 0;
    sndTimer = timeSetEvent(MPEG_AUDIO_TIMER_INTERVAL, MPEG_AUDIO_TIMER_RESOLUTION, mpegAudioTimerCallback, 0, TIME_PERIODIC);
    sndCallbackSet = 0;
    sndCurBlock = 0;
    sndLastBlock = -1;
    sndPartial = 0;
    sndActive = 1;
    sndPlaying = 0;
    sndStreamPos = 0;
    return 0;
}

/* Stops audio playback and clears every queued block. */
void mpegResetAudioBlockQueue(void)
{
    if (!sndActive)
        return;
    sndActive = 0;
    sndPlaying = 0;
    for (sndCurBlock = 0; sndNumBlocks > sndCurBlock; sndCurBlock++) {
        sndBlocks[sndCurBlock].fill = 0;
        sndBlocks[sndCurBlock].frames = 0;
        sndBlocks[sndCurBlock].ready = 0;
    }
    if (sndTimer)
        timeKillEvent(sndTimer);
    sndTimer = 0;
}

/* Registers the callback run when an audio block has been consumed. */
void mpegSetAudioBlockCallback(void (*callback)(void))
{
    if (!sndActive)
        return;
    sndCallback = callback;
    sndCallbackSet = 1;
}

/* Queues decoded MPEG audio bytes, completing any partial frame first. */
void mpegQueueAudioData(unsigned char *data, unsigned long len)
{
    unsigned long *s4;
    unsigned long left;
    unsigned long *dp;
    unsigned long j;
    unsigned char *src;
    unsigned char *dst;

    if (!sndActive)
        return;
    src = data;
    if (sndPartial) {
        dst = sndBlocks[sndLastBlock].data + sndBlocks[sndLastBlock].fill;
        left = SND_FRAME_BYTES - sndPartial;
        for (j = 0; j < left; j++) {
            *dst = *src;
            src++;
            dst++;
        }
        sndBlocks[sndLastBlock].fill += left;
        sndBlocks[sndLastBlock].frames++;
        sndBlocks[sndLastBlock].ready = 1;
        len -= left;
    }
    sndLastBlock = (sndLastBlock + 1) % sndNumBlocks;
    dp = (unsigned long *)sndBlocks[sndLastBlock].data;
    s4 = (unsigned long *)src;
    for (j = 0; j < len; j += 4) {
        *dp = *s4;
        s4++;
        dp++;
    }
    sndBlocks[sndLastBlock].fill = len;
    sndBlocks[sndLastBlock].frames = len / SND_FRAME_BYTES;
    sndPartial = len % SND_FRAME_BYTES;
    if (sndPartial == 0)
        sndBlocks[sndLastBlock].ready = 1;
}

/* Counts audio blocks ready for playback. */
int mpegCountReadyAudioBlocks(void)
{
    int count;
    unsigned long i;

    count = 0;
    for (i = 0; i < sndNumBlocks; i++) {
        if (sndBlocks[i].ready == 1)
            count++;
    }
    return count;
}

/* Starts the DirectSound buffer and marks audio as playing. */
void mpegStartAudioPlayback(void)
{
    if (sndActive)
        sndPlaying = 1;
    mpegSndResult = mpegSoundBuf->lpVtbl->Play(mpegSoundBuf, 0, 0, DSBPLAY_LOOPING);
}

/* Stops the DirectSound buffer and clears the playing state. */
void mpegStopAudioPlayback(void)
{
    sndPlaying = 0;
    mpegSndResult = mpegSoundBuf->lpVtbl->Stop(mpegSoundBuf);
}

/* Feeds the next audio chunk to DirectSound on each timer tick. The seven locals keep their
   names and order, which set the executable's stack layout. */
void __stdcall mpegAudioTimerCallback(unsigned id, unsigned msg, unsigned long user, unsigned long dw1, unsigned long dw2)
{
    void *p1; /* first locked region */
    unsigned long sum; /* play cursor */
    unsigned long s; /* buffer status */
    unsigned long hv; /* write cursor */
    void *y0; /* second locked region */
    unsigned long s1; /* first locked size */
    unsigned long new; /* second locked size */

    if (!sndPlaying)
        return;
    mpegSndResult = mpegSoundBuf->lpVtbl->GetStatus(mpegSoundBuf, &s);
    if (!(s & 1)) {
        mpegSndResult = mpegSoundBuf->lpVtbl->Restore(mpegSoundBuf);
        mpegSndResult = mpegSoundBuf->lpVtbl->Play(mpegSoundBuf, 0, 0, DSBPLAY_LOOPING);
    }
    mpegSndResult = mpegSoundBuf->lpVtbl->GetCurrentPosition(mpegSoundBuf, &sum, &hv);
    if ((hv + MPEG_AUDIO_WRITE_CHUNK) % sndBufferBytes < hv) {
        if (hv > sndStreamPos && (hv + MPEG_AUDIO_WRITE_CHUNK) % sndBufferBytes < sndStreamPos)
            return;
    } else {
        if (hv > sndStreamPos || hv + MPEG_AUDIO_WRITE_CHUNK < sndStreamPos)
            return;
    }
    mpegSndResult = mpegSoundBuf->lpVtbl->Lock(mpegSoundBuf, sndStreamPos, MPEG_AUDIO_WRITE_CHUNK, &p1, &s1, &y0, &new, 0);
    mpegDecodeAudioFrames(p1);
    sndStreamPos += MPEG_AUDIO_WRITE_CHUNK;
    if (sndStreamPos >= sndBufferBytes)
        sndStreamPos = 0;
    mpegSndResult = mpegSoundBuf->lpVtbl->Unlock(mpegSoundBuf, p1, s1, y0, new);
}

/* Decodes queued audio frames into a locked DirectSound buffer region. */
void mpegDecodeAudioFrames(unsigned char *dst)
{
    unsigned char *src;
    unsigned long rem;
    unsigned char *out;
    unsigned long cnt;
    unsigned long c;

    out = dst;
    src = sndBlocks[sndCurBlock].data + sndFrameOffset * SND_FRAME_BYTES;
    rem = sndBlocks[sndCurBlock].frames - sndFrameOffset;
    c = MPEG_AUDIO_FRAMES_PER_CHUNK;
    do {
        if (!sndBlocks[sndCurBlock].ready)
            break;
        cnt = c < rem ? c : rem;
        c -= cnt;
        rem -= cnt;
        sndFrameOffset += cnt;
        decodeDst = out;
        decodeSrc = (unsigned long)src;
        decodeCount = cnt;
        mpegDecodeAudioSamples();
        out += (cnt << 6) & ~1;
        if (rem == 0) {
            sndBlocks[sndCurBlock].fill = 0;
            sndBlocks[sndCurBlock].frames = 0;
            sndBlocks[sndCurBlock].ready = 0;
            sndCurBlock = (sndCurBlock + 1) % sndNumBlocks;
            src = sndBlocks[sndCurBlock].data;
            rem = sndBlocks[sndCurBlock].frames;
            sndFrameOffset = 0;
            if (sndCallbackSet)
                sndCallback();
        }
    } while (c != 0);
}

/* Builds the volume/pan table: for each of 256 volume steps, 16x16 left/right gains. */
int mpegBuildVolumeTable(void *table)
{
    int amp;
    unsigned int step;
    short right;
    short l;

    sndVolumeTable = table;
    if (sndVolumeTable == 0)
        return 1;
    for (step = 0; step < MPEG_AUDIO_VOLUME_STEPS; step++) {
        amp = (int)(pow(11.0, -(255.0f - step) / 65.161) * 65535.0);
        for (l = 0; l < 16; l++) {
            for (right = 0; right < 16; right++) {
                sndVolumeTable[(l * 16 + step * 256 + right) * 2] = (int)(panTable[l] * amp);
                sndVolumeTable[(l * 16 + step * 256 + right) * 2 + 1] = (int)(panTable[right] * amp);
            }
        }
    }
    return 0;
}
