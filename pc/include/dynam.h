#ifndef DYNAM_H
#define DYNAM_H

#include "wipeout.h"

/* A weapon grid entry: one track section where a weapon zone is active. */
struct WeaponGrid {
    TrackSection *section;
#ifdef NETPLAY
    short         attr;    /* NET-WOXL loads it sign-extended (movsx) */
#else
    u_short       attr;
#endif
    short         angle;
    long          count;
    long          colour;
};

/* A special (pickup) grid entry: the section number and the count of that zone. */
struct SpecialGrid {
    short section;
    short count;
    char  attr;
    char  pad05;
};

#endif
