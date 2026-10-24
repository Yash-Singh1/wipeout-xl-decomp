#include "functions.h"
#include "gpu_stubs.h"

/* The PC build reads game files straight from disk with stdio, so the CD-ROM layer
   is a thin wrapper around FILE streams. */
typedef struct _iobuf FILE;
FILE *fopen(const char *name, const char *mode);
int fclose(FILE *stream);
int fseek(FILE *stream, long offset, int origin);
long ftell(FILE *stream);
unsigned int fread(void *buffer, unsigned int size, unsigned int count, FILE *stream);
unsigned int fwrite(const void *buffer, unsigned int size, unsigned int count, FILE *stream);

/* The stdio-backed PC file layer requires no heap initialization. */
int iso_init(HeapBlock *heap)
{
    return 0;
}

/* Opens a file for reading ("rb"), writing ("wb") or update ("wb+"), returning the stream or -1. */
#ifndef NETPLAY
long iso_open(char *pathname, long flags, int unusedMode)
{
    FILE *stream;

    switch (flags) {
    case 0:
        stream = fopen(pathname, "rb");
        break;
    case 1:
        stream = fopen(pathname, "wb");
        break;
    case 2:
        stream = fopen(pathname, "wb+");
        break;
    }
    if (stream == 0) {
        return -1;
    }
    return (long)stream;
}
#endif
#ifdef NETPLAY
long iso_open(char *pathname, long flags, int unusedMode)
{
    FILE *stream;

    switch (flags) {
    case 0:
        stream = fopen(pathname, "rb");
        break;
    case 1:
        stream = fopen(pathname, "wb");
        break;
    case 2:
        stream = fopen(pathname, "wb+");
        break;
    }
    if (stream == 0) {
        return -1;
    } else {
        return (long)stream;
    }
}
#endif

/* Moves the stream and returns its new position. */
long SeekFile(int fd, long offset, int origin)
{
    fseek((FILE *)fd, offset, origin);
    return ftell((FILE *)fd);
}

/* Reads bytes from a stdio stream. */
long ReadFileBytes(int fd, void *buffer, long count)
{
    return fread(buffer, 1, count, (FILE *)fd);
}

/* Writes bytes to a stdio stream. */
long WriteFileBytes(int fd, void *buffer, long count)
{
    return fwrite(buffer, 1, count, (FILE *)fd);
}

/* Closes the stdio stream associated with the file descriptor. */
int iso_close(int fd)
{
    fclose((FILE *)fd);
    return 0;
}

/* The disc directory and sector queries have no meaning for stdio files: they report nothing. */
long iso_read(void *buffer, long nsec, long fd)
{
    return 0;
}

/* The PC disc layer has no operation at this compatibility entry point. */
void IsoDiscNoOp(void)
{
}

/* No table of contents is exposed by the stdio-backed PC file layer. */
u_char *iso_toc(void)
{
    return 0;
}

/* Disc-table size queries are unsupported by the stdio file layer. */
int iso_filesize(int fileNo)
{
    return 0;
}

/* Disc-table location queries are unsupported by the stdio file layer. */
int iso_fileloc(int fileNo)
{
    return 0;
}

/* The CD-ROM error callback is unused by the PC file layer. */
void my_error_handler(void)
{
}

/* Compatibility implementation of the display-environment setter stub. */
DISPENV *PutDispEnvStub(DISPENV *value)
{
    return value;
}

/* Compatibility implementation of the drawing-environment setter stub. */
DRAWENV *PutDrawEnvStub(DRAWENV *value)
{
    return value;
}

/* Primitive submission is provided by Direct3D elsewhere in the PC renderer. */
void DrawPrim(void *primitive)
{
}

#ifdef POWERVR
int IsoTellStub(void)
{
    return 0;
}

long IsoWrite(int fd, void *buffer, long count)
{
    return fwrite(buffer, 1, count, (FILE *)fd);
}

typedef struct IsoIoctlDevice {
    short type;
    short flags;
    long state[17];
    void (*ioctl)(int request, int command, struct IsoIoctlDevice *device,
                  struct IsoIoctlDevice *devices);
} IsoIoctlDevice;

void IsoIoctl(int request, int command, IsoIoctlDevice *devices)
{
    IsoIoctlDevice *device;
    int count;

    device = devices;
    count = 20;
    while (count > 0) {
        if (device->type > 0 && device->ioctl) {
            device->ioctl(request, command, device, devices);
        }
        device++;
        count--;
    }
}
#endif
