#include "mpeg.h"
#include <string.h>

#define MPEG_CODE_BUFFER_BYTES 0x80
#define MPEG_STREAM_BUFFER_BYTES 0x5000
#define MPEG_FILE_BINARY_FLAG 0x8000
#define MPEG_STREAM_BUFFER_PREFIX 0x1000
#define MPEG_STREAM_READ_BYTES 0x4000

int _open(const char *path, int flags);
int _close(int fd);
int _read(int fd, void *buf, unsigned count);


/* Allocates a video stream and its initial input/code buffers. */
VidStream *mpegOpenVideoStream(const char *path, void *heap)
{
    VidStream *vs;
    void *buf;

    buf = mpegHeapAlloc(&heap, MPEG_CODE_BUFFER_BYTES);
    vs = mpegHeapAlloc(&heap, sizeof(VidStream));
    memset(vs, 0, sizeof(VidStream));
    vs->heap = heap;
    vs->codeBuf = buf;
    mpegFile = _open(path, MPEG_FILE_BINARY_FLAG);
    if (mpegFile == -1)
        return 0;
    vs->buffer = mpegHeapAlloc(&vs->heap, MPEG_STREAM_BUFFER_BYTES);
    vs->bufEnd = vs->buffer;
    vs->bufStart = vs->buffer;
    return vs;
}

/* The caller passes its stream, but the original closes only the global file handle. */
void mpegCloseVideoStream(VidStream *unusedStream)
{
    _close(mpegFile);
}

/* Reads the next input chunk into the stream's buffered data area. */
void mpegReadVideoStreamChunk(VidStream *vs)
{
    int n;

    n = _read(mpegFile, vs->buffer + MPEG_STREAM_BUFFER_PREFIX, MPEG_STREAM_READ_BYTES);
    if (n != MPEG_STREAM_READ_BYTES)
        vs->eof = 1;
    if (n == 0)
        mpegDebugMessage("Attempt to read beyond stream");
    vs->bufStart = vs->buffer + MPEG_STREAM_BUFFER_PREFIX;
    vs->bufEnd = vs->buffer + n + MPEG_STREAM_BUFFER_PREFIX;
}

/* Compacts unread input and refills the video stream buffer. */
void mpegRefillVideoStreamBuffer(VidStream *vs)
{
    int left;

    left = vs->bufEnd - vs->bufStart;
    if (vs->eof)
        return;
    if (left != 0)
        memmove(vs->buffer + MPEG_STREAM_BUFFER_PREFIX - left, vs->bufStart, left);
    mpegReadVideoStreamChunk(vs);
    vs->bufStart = vs->buffer + (MPEG_STREAM_BUFFER_PREFIX - left);
}
