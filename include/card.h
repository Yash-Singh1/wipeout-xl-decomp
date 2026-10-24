#ifndef CARD_H
#define CARD_H

#include "wipeout.h"

/* Memory card directory entry (libapi DIRENTRY layout). */
typedef struct DIRENTRY {
    char            name[20];
    long            attr;
    long            size;
    struct DIRENTRY *next;
    char            system[8];
} DIRENTRY;

DIRENTRY *firstfile(char *name, DIRENTRY *dir);
DIRENTRY *nextfile(DIRENTRY *dir);

/* Icon rectangles of the save-icon image, in VRAM coordinates. */
typedef struct CardImage {
    u_char reserved[6];
    RECT   image;
    u_short cx, cy, cw, ch;
} CardImage;

typedef struct Label6 {
    signed char bytes[6];
} Label6;

typedef struct Label8 {
    signed char bytes[8];
} Label8;

typedef struct Label5 {
    signed char bytes[5];
} Label5;

/* One file shown in the card browser. */
typedef struct CardEntry {
    char      name[9];
    u_char    alignment;
    u_char    used;
    char      filename[32];
#ifdef VERSION_EU /* "WO2097 " does not fit in six bytes */
    Label8    label;
    char      reserved[24];
#else
    Label6    label;
    char      reserved[26];
#endif
    u_char    type;
#ifndef VERSION_EU /* the European card browser has no multi-block files */
    u_char    first;
    u_char    multi;
#endif
    u_char    icon[8];
    u_short   x, y;
} CardEntry;

typedef struct CardSlot {
    CardEntry entry[15];
} CardSlot;

void LoadCardFiles(CardSlot *cards, int slot);

/* Name buffer built for a card file. The union keeps the 4-byte alignment of the original local. */
typedef union CardFilename {
    char bytes[40];
    long words[10];
} CardFilename;

/* The game's product code: the memory card directory name of its saves. */
#define CARD_GAME_CODE REGION_STR("BASCUS-94351", "BESLES-00327")

/* Size of the header block at the start of a save file; the European card format uses half. */
#define CARD_HEADER_SIZE REGION(0x200, 0x100)

/* Offset in the save header where the file's own name starts, after the game title. */
#define CARD_NAME_OFFSET REGION(14, 18)

/* Device prefix written before a file name ("bu00" style). */
typedef union CardDevice {
    char bytes[6];
    long align;
} CardDevice;

/* Key block: 2 key sets of 9 pad keys and 2 sets of 6 keys. */
typedef struct CardKeys {
    u_short pad[2][9];
    u_short reserved;
    u_short other[2][6];
} CardKeys;

/* A whole card file as read by LoadCardInitData: header, key/key-block and settings. */
typedef struct CardFileBuffer {
    u_char   header[CARD_HEADER_SIZE];
    char     name[32];
    u_short  data[128];
} CardFileBuffer;

#endif
