#ifndef TRAILS_H
#define TRAILS_H

/* How much a trail segment's colour fades each frame. */
#define TRAIL_FADE REGION(27, 23)

/* One quad of a ship's exhaust trail. */
typedef struct TrailSegment {
    char   active;
    char   pad;
    short  r, g, b;         /* fades a little every frame */
    VECTOR left;
    VECTOR right;
    POLY_G4 leftHalf[2];    /* the two halves of the quad, each double buffered */
    POLY_G4 rightHalf[2];
    DR_MODE drawMode[2];    /* double buffered */
} TrailSegment;

/* The Japanese release keeps two segments fewer per trail. */
#ifdef VERSION_JP
#define TRAIL_SEGMENTS 9
#else
#define TRAIL_SEGMENTS 11
#endif

/* A ship's trail: a ring of segments, newest at index current. */
struct Trail {
    u_char       colour[3][3];  /* normal, the occasional bright flash, white */
    char         pad9[3];
    int          current;
    TrailSegment segment[TRAIL_SEGMENTS];
};

#endif
