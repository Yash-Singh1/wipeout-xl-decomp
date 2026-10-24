#ifndef TRACK_H
#define TRACK_H

/* TrackSection.flags */
#define TRACK_JUMP             0x01
#define TRACK_JUNCTION_END     0x08
#define TRACK_JUNCTION_START   0x10

/* Face.flags: the pit lane repairs ship energy. */
#define PIT_LANE 0x40

#include "psyq.h"

/* Automesh output: the automeshed polygons, then the quad and tri counters. */
struct AutoMeshData {
    char  primitives[0x9000];
    short quadCount;
    short triCount;
    short attemptedAutos;
};

/* Per-face colour copy, kept beside the track faces. */
typedef struct Colors {
    u_char red;
    u_char green;
    u_char blue;
} Colors;

#endif
