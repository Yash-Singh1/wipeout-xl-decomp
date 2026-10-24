#ifndef TRAILS_H
#define TRAILS_H

#include "psyq.h"

/* One quad of a ship's exhaust trail. The PC build keeps only the corners and
   the fading colour per segment; the quads are drawn from these each frame. */
typedef struct TrailSegment {
    char   active;
    char   pad;
    short  r, g, b;         /* fades a little every frame */
    VECTOR left;
    VECTOR right;
} TrailSegment;

#define TRAIL_SEGMENTS 30

/* A ship's trail: a ring of segments, newest at index current. */
struct Trail {
    u_char       colour[3][3];  /* normal, the occasional bright flash, white */
    char         pad9[3];
    int          current;
    TrailSegment segment[TRAIL_SEGMENTS];
};

#endif
