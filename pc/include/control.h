#ifndef CONTROL_H
#define CONTROL_H

#include "wipeout.h"

#ifdef NETPLAY
void UpdateRacingShip(ShipData *ships, void *shapes, short shipNo, void *weapons);
#endif

typedef void (*ShipUpdateFunc)(ShipData *, Object **, short, void *);

/* Controller state as the ship update code reads it (one per player / ghost / remote). */
typedef struct ControlInput {
    u_short head;
    u_short buttons;
    u_short pressed;
    u_char  pad6[2];
    u_char  thrust;
    u_char  steering;
} ControlInput;

/* Four 16-bit values, one per brake mode. */
typedef struct BrakeModes {
    short value[4];
} BrakeModes;

/* Checkpoint bonus cell: track section, bonus seconds and a done flag. Cells are indexed by
   trackNum * 4 + shipClass, then by cell number. */
typedef struct CheckCell {
    short section;
    short bonusSeconds;
    char  done;
} CheckCell;

#endif
