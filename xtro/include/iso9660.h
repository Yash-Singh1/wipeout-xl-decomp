#ifndef ISO9660_H
#define ISO9660_H

#include "common.h"
#include "malloc.h"

#define DIR_CACHE_SIZE 8192
#define ISO_SECTOR_BYTES 2048
#define CD_SECTORS_PER_SECOND 75
#define CD_SECTORS_PER_MINUTE (60 * CD_SECTORS_PER_SECOND)
#define CD_SEEK_LOGICAL 0x15
#define CD_DOUBLE_SPEED 0x80
#define ISO_PRIMARY_VOLUME 1
#define ISO_DIRECTORY 2
#define ISO_OPEN_FILES 16

int printf(const char *format, ...);

/* Minute/second/sector/track as the CD drive reports a position. */
typedef struct CdlLOC {
    u_char minute;
    u_char second;
    u_char sector;
    u_char track;
} CdlLOC;

/* Position and length of a file on the disc, from its directory record. */
typedef struct IsoFileInfo {
    long loc;
    long size;
} IsoFileInfo;

/* One slot of the open-file table. fd is 0 while the slot is free. */
typedef struct IsoFile {
    long fd;
    IsoFileInfo info;
    u_long fp;
} IsoFile;

/* Read the little-endian halves of an extent or file length. */
#define ISO_EXTENT(record) ((record)->extent[0] | ((record)->extent[1] << 16))
#define ISO_FILE_SIZE(record) ((record)->size[0] | ((record)->size[1] << 16))

/* A variable-length entry in the little-endian path table. */
typedef struct IsoPathRecord {
    u_char nameLen;
    u_char extAttrLength;
    u_short extent[2];
    u_short parent;
    char name[1];
} IsoPathRecord;

extern u_char toc_loc[];

/* Primary volume descriptor, the first sector iso_init reads. Only the fields the player
   uses are named; the path table size and location are their little-endian halves. */
typedef struct PrimaryVolDesc {
    u_char type;
    u_char standardId[5];
    u_char version;
    u_char unused0[0x7D];
    u_long pathTableSize;
    u_long pathTableSizeBE;
    u_long pathTableLoc;
} PrimaryVolDesc;

/* ISO 9660 directory record; the first two halfwords of each extent and size are little-endian. */
typedef struct DirRecord {
    u_char length;
    u_char extAttrLength;
    u_short extent[4];
    u_short size[4];
    u_char date[7];
    u_char flags;
    u_char unitSize;
    u_char interleaveGap;
    u_char volSeq[4];
    u_char nameLen;
    char name[1];
} DirRecord;

#endif /* ISO9660_H */
