#ifndef WEAPON_H
#define WEAPON_H

#include "wipeout.h"

/* One entry of the weapon shape table: the object used to draw a weapon, and whether it is in use. */
struct WeaponShape {
    Object *shapePtr;
    short   used;
    short   pad;
};

#include "pc_mesh.h"

/* Shield normal sets use the shared PC object mesh layout. */
typedef PCObjectMesh ShieldNormals;

/* One weapon in flight (0x4C bytes). The weapons array (weaponIndex, DAlloc of 0x610 bytes in
   racectrl.c) holds 20 of them; the update loops run over 0 to 19.
   count: frames left while flying; -1 = hit (ring + explosion), -2 = expired, 0 = free.
   type:  1 rocket, 2 missile, 3 mine, 5 electric bolt, 13 minigun.
   target: ship index this weapon has hit or is locked on. */
struct WeaponData {
    short        count;          /* 0x00 */
    short        bumpStep;       /* 0x02: bump step counter used by the quake weapon */
    short        shapeNo;        /* 0x04: index into the WeaponShape table */
    short         pad06;         /* 0x06: alignment padding */
    TrackSection *section;       /* 0x08: track section the weapon is over */
    VECTOR       accel;          /* 0x0C */
    VECTOR       velocity;       /* 0x1C */
    VECTOR       pos;            /* 0x2C */
    short        hdg;            /* 0x3C */
    short        pitch;          /* 0x3E */
    signed char  target;         /* 0x40 */
    signed char   owner;         /* 0x41: index of the firing ship */
#ifdef NETPLAY
    signed char  type;           /* 0x42 */
#else
    unsigned char type;          /* 0x42 */
#endif
    unsigned char pad43;         /* 0x43: alignment padding */
    void        (*update)(ShipData *ships, struct WeaponShape *shapes, struct WeaponData *weapon, struct WeaponData *weapons); /* 0x44 */
    void        (*draw)(ShipData *ships, struct WeaponShape *shapes, struct WeaponData *weapon, struct WeaponData *weapons); /* 0x48: per-frame draw callback (PC only) */
};


/* One weapon grid zone (16 bytes): the same entry as WeaponGrid in dynam.h, with the fields the
   grid animation uses. count: the zone is armed (>= 0) or counting down a hit. */
struct WeaponGridZone {
    TrackSection *section;
#ifdef NETPLAY
    short         attr;
#else
    u_short       attr;
#endif
    short         angle;
    long          count;
    u_char        r, g, b, pad;
};

/* A Gouraud-shaded triangle and quad as written into the primitive buffer (28 and 36 bytes). */
typedef struct ShadedTri {
    unsigned long tag;
    CVECTOR       colour0;
    DVECTOR       xy0;
    CVECTOR       colour1;
    DVECTOR       xy1;
    CVECTOR       colour2;
    DVECTOR       xy2;
} ShadedTri;

typedef struct ShadedQuad {
    unsigned long tag;
    CVECTOR       colour0;
    DVECTOR       xy0;
    CVECTOR       colour1;
    DVECTOR       xy1;
    CVECTOR       colour2;
    DVECTOR       xy2;
    CVECTOR       colour3;
    DVECTOR       xy3;
} ShadedQuad;

typedef union ShadedPtr {
    void       *ptr;
    ShadedTri  *tri;
    ShadedQuad *quad;
} ShadedPtr;

/* Weapon draw callbacks (WeaponData.draw): show the shape, or free its slot when the weapon ends. */
void DrawMissile(ShipData *ships, struct WeaponShape *shapes, struct WeaponData *weapon, struct WeaponData *weapons);
void DrawRocket(ShipData *ships, struct WeaponShape *shapes, struct WeaponData *weapon, struct WeaponData *weapons);
void DrawPlasma(ShipData *ships, struct WeaponShape *shapes, struct WeaponData *weapon, struct WeaponData *weapons);

#endif
