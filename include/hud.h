#ifndef HUD_H
#define HUD_H

#include "functions.h"

/* AddTime display flags. */
#define TIME_SINGLE_MINUTE_DIGIT 1
#define TIME_HIDE_MINUTES        2

/* One glyph of a bitmap font: its VRAM origin and advance width. */
typedef struct FontGlyph {
    u_short vramX;
    u_short vramY;
    u_short width;
} FontGlyph;

/* The eight quads of the speedo: frame, speed bar, shield bar and thrust marker, one per screen. */
typedef struct Speedo {
    POLY_FT4 frame[2];
    POLY_FT4 speed[2];
    POLY_FT4 shield[2];
    POLY_FT4 marker[2];
} Speedo;

/* One checkpoint of the race: the track section it sits on, the time allowance,
   and whether the player has reached it. The array is indexed directly. */
struct CheckPoints {
    short       section;
    short       time;
    signed char reached;
    char        padding;
};

#include "options.h"

/* Ship records are ShipData; the HUD and the race-start code use the same layout under this name. */
typedef ShipData RaceShip;

#endif
