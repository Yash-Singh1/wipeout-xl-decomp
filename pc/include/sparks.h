#ifndef SPARKS_H
#define SPARKS_H

#include "wipeout.h"

/* One entry of the particle table (256 entries of 0x2C bytes) shared by sparks, smoke, rain and snow. */
struct SParticle {
    char    type;
    char    flags;
    short   pad02;
    Vector  vel;            /* 0x04 */
    Vector  pos;            /* 0x10 */
    Vector  cameraPos;
    long    life;           /* 0x28 */
};

#define PARTICLE_COUNT 256

/* A billboard explosion (table of 32). */
typedef struct Explosion {
    long   x, y, z;
    u_char size;            /* sprite size is halved per step */
    u_char r, g, b;
    short  flash;           /* 1: plays the flash frames instead of the fireball */
    short  blend;           /* semi-transparency mode of the sprite */
    short  life;            /* free when <= 0 */
    short  ship;            /* position is relative to this ship when >= 0 */
    long   palette;
} Explosion;

#define EXPLOSION_COUNT 32

/* An expanding shock ring (table of 8). Only the position and the life counter are
   used; the other words are never touched and just keep the 0x1C entry stride. */
typedef struct Ring {
    long  x, y, z;
    long  pad0C;
    long  pad10;
    short life;
    short pad16;
    long  pad18;
} Ring;

#define RING_COUNT 8

/* One bird of the flock (table of 32). */
typedef struct Bird {
    long x, y, z;
    long vx, vy, vz;
    long frame;
    long accel;
} Bird;

#define BIRD_COUNT 32

/* An electric bolt (table of 64): a start and an end point, then three line segments,
   each double buffered. */
typedef struct Electric {
    VECTOR  start;
    VECTOR  end;
    long    life;           /* -1 when unused */
    LINE_G2 line1[2];
    LINE_G2 line2[2];
    LINE_G2 line3[2];
    u_char  r1, g1, b1;
    u_char  r2, g2, b2;
    short   padA2;
    long    jitter[6];
} Electric;

#define ELECTRIC_COUNT 64


void UpdateBirds(void);

#ifdef NON_MATCHING
void AsmDrawParticles(struct SParticle *particles, Skeleton *camera, int unused);
#endif

#endif
