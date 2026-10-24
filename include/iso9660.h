#ifndef ISO9660_H
#define ISO9660_H

#include "psyq.h"

/* Position and length of a file on the disc, from its directory record. */
typedef struct IsoFileInfo {
    long loc;
    long size;
} IsoFileInfo;

/* One slot of the open-file table. fd is 0 while the slot is free. */
typedef struct IsoFile {
    long        fd;
    IsoFileInfo fi;
    u_long      fp;
} IsoFile;

#endif
