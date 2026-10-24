#ifndef SHIPS_H
#define SHIPS_H

#include "wipeout.h"
#include "ship_models.h"

/* One entry per track section, indexed by section number. */
/* Race order table: position along the lap and the ship it belongs to. */
typedef struct ShipRank {
    short position;
    short shipNo;
} ShipRank;

#ifdef NETPLAY
extern ShipRank shipRank[];
extern short activeShipCount;
#endif

/* Texture table entry: CLUT and texture page VRAM coordinates. */
/* Ghost CLUT position (texture page relative). */
typedef struct GhostClut {
    u_short clutX;
    u_short clutY;
} GhostClut;


/* XL ship record (0xF0 bytes). Offsets are taken from the code; the names of the
   16-bit block at 0x90..0xB0 and of 0xC0..0xC7 are provisional (see NOTES-ships.md). */

#endif
