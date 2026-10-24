#ifndef RACEINIT_H
#define RACEINIT_H

#include "hud.h"

/* The pause-menu fields reset when a race is (re)started. */
struct RaceData {
    int         selection;
    int         musicPaused;
    int         command;
    signed char pad0cLow;
    signed char pad0cHigh;
    signed char showOptions;
};

/* The pointers that InitRaceResetData records for a restart of the race. */
struct ResetData {
    ShipData   *ships;
    RaceConfig *config;
    struct WeaponData *weapons;
    struct WeaponShape *weaponShapes;
    RescueData *rescue;
    Object    **shipShapes;
    struct CheckPoints *checkPoints;
};

#endif
