#ifndef EFFECTS_H
#define EFFECTS_H

#include "functions.h"
#include "tim.h"

/* Mono-rail carriage: the step index, the per-frame delta along the rail and the rail's
   full length. clearedX/Y/Z are zeroed once when the mono-rail track loads and never read. */
struct MonoRailData {
    short index;                  /* 0x00 */
    short pad02;                  /* 0x02 */
    long  stepX, stepY, stepZ;    /* 0x04 */
    long  pad10;                  /* 0x10 */
    long  clearedX, clearedY, clearedZ; /* 0x14 */
    long  pad20;                  /* 0x20 */
    long  totalX, totalY, totalZ; /* 0x24 */
};

extern MonoRailData monoRail;
extern VECTOR zeppelinBase;

/* Start-grid droid: the checkpoint section it stands on, that section's bonus seconds
   (non-zero once the cell is live), and the cell's done flag. This is the same six-byte
   record as `CheckCell`. */
struct DroidData {
    short       section;
    short       bonusSeconds;
    signed char done;
};

/* Checkpoint on the race line: section number, seconds allowed and lap. */
struct Checkpoint {
    short       section;
    short       seconds;
    signed char lap;
    signed char pad;
};

/* Horizontal orientation of the start-droid sign texture for each track. */
typedef struct DroidTextureOrientation {
    short unmirrored[8];
} DroidTextureOrientation;

/* Rectangle with unsigned fields, as the video code stores them. */
typedef struct Rect {
    u_short x, y, w, h;
} Rect;


/* One train-light entry: the colour bytes of a G3/G4 primitive, as the lighting code addresses them. */
typedef struct TrainLight {
    unsigned char pad[0xC];
    unsigned char c1[4];
    unsigned char c2[4];
    unsigned char c3[4];
    unsigned char c4[3];
} TrainLight;

/* Shadow quads for one ship: quad[16][2], 16 quads with one copy per screen buffer. */
struct Shadow {
    POLY_FT4 quad[16][2];
};

/* Header entry at the start of an object's primitive list: the type word and a flag word. */
typedef struct PrimHeader {
    short   type;
    u_short flag;
} PrimHeader;

/* Texture fields of a textured quad (GT4 or FT4) from its tsb word on. The clut word (cba) sits
   just before tsb. Colours are only present in GT4 records. */
typedef struct TexFields {
    short   tsb;
    u_char  u0, v0, u1, v1, u2, v2, u3, v3;
    short   pad;
    CVECTOR colour[4];
} TexFields;

/* Scene objects and their saved positions, shared with race setup. */
extern Object *monoRailObjs[6];
extern VECTOR posMonoRail[6];
extern Object *zeppelinObj[];
extern Object *lightGroups[3];
extern unsigned char droidPaletteColour[4];
extern unsigned char droidBlinkColour[4];

void DrawStartDroid(Object **objects, ShipData *ships, DroidData *droids);
void DrawEffects(WeaponData *weapons, ShipData *ships);
void DrawMonoRail(Object **objects, short count);
void AddRenderObject(Object *object, int mode, u_char *colour);

void AddElectricStreak(VECTOR *position, int mode);

#endif
