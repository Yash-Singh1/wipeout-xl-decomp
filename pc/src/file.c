#include "functions.h"
#include <stdio.h>

long ReadFileBytes(int handle, void *buffer, long size);
int iso_close(int handle);
long SeekFile(int handle, long offset, int whence);
void FileAccessError(char *fileName);
#ifdef NETPLAY
int _rmdir(char *path);
#endif

/* Returns the length of an ISO file, or zero if it cannot be opened. */
#ifndef NETPLAY
long FileLength(char *name)
{
    int fd;
    long length;

    fd = iso_open(name, 0, 0);
    if (fd == -1)
        return 0;
    length = SeekFile(fd, 0, SEEK_END);
    iso_close(fd);
    return length;
}
#endif
#ifdef NETPLAY
long FileLength(char *name)
{
    int handle;
    long length;

    handle = iso_open(name, 0, 0);
    if (handle == -1)
        return 0;
    length = SeekFile(handle, 0, SEEK_END);
    iso_close(handle);
    return length;
}
#endif

/* Reads the complete ISO file into the supplied buffer. */
#ifndef NETPLAY
long LoadFile(char *name, char *buffer)
{
    int fd;
    long length;

    fd = iso_open(name, 0, 0);
    if (fd == -1) {
        FileAccessError(name);
        return 0;
    }
    length = SeekFile(fd, 0, SEEK_END);
    SeekFile(fd, 0, SEEK_SET);
    if (ReadFileBytes(fd, buffer, length) != length) {
        iso_close(fd);
        return 0;
    }
    iso_close(fd);
    return length;
}
#endif
#ifdef NETPLAY
long LoadFile(char *name, char *buffer)
{
    int handle;
    long length;

    handle = iso_open(name, 0, 0);
    if (handle == -1) {
        FileAccessError(name);
        return 0;
    }
    length = SeekFile(handle, 0, SEEK_END);
    SeekFile(handle, 0, SEEK_SET);
    if (ReadFileBytes(handle, buffer, length) != length) {
        iso_close(handle);
        return 0;
    }
    iso_close(handle);
    return length;
}
#endif

/* Replaces a disk file with the supplied byte range. */
void SaveFile(char *name, char *buffer, long length)
{
    FILE *fp;

#ifdef NETPLAY
    _rmdir(name);
#else
    remove(name);
#endif
    fp = fopen(name, "a+");
    if (fp) {
        fwrite(buffer, 1, length, fp);
        fclose(fp);
    }
}
