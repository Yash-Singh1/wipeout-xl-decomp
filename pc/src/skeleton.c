#include <math.h>
#include "functions.h"

/* sin/cos of a 4096-per-turn angle, scaled by 4096. */
#undef rsin
#undef rcos
#define rsin(a) ((long)(sin(((double)(a) / 4096.0) * 6.28318530718) * 4096.0))
#define rcos(a) ((long)(cos(((double)(a) / 4096.0) * 6.28318530718) * 4096.0))

Skeleton *NewSkeleton(void)
{
    Skeleton *skeleton;

    skeleton = DAlloc(heap, sizeof(Skeleton));
    if (!skeleton) {
        OutputDebugString("Skeleton.c:SkeletonRoot(): Failed to allocate skeleton\n");
        return 0;
    }

    skeleton->super = 0;
    skeleton->sub = 0;
    skeleton->next = 0;

    skeleton->relative.t[0] = 0;
    skeleton->relative.t[1] = 0;
    skeleton->relative.t[2] = 0;

    skeleton->relative.m[0][0] = 4096;
    skeleton->relative.m[0][1] = 0;
    skeleton->relative.m[0][2] = 0;

    skeleton->relative.m[1][0] = 0;
    skeleton->relative.m[1][1] = 4096;
    skeleton->relative.m[1][2] = 0;

    /* The third row's 4096 sits in m[2][1], not m[2][2]. */
    skeleton->relative.m[2][0] = 0;
    skeleton->relative.m[2][1] = 4096;
    skeleton->relative.m[2][2] = 0;

    skeleton->update = 1;

    return skeleton;
}

/* Sets the skeleton's translation relative to its parent. */
void SetSkeletonPosition(Skeleton *skeleton, long x, long y, long z)
{
    skeleton->relative.t[0] = x;
    skeleton->relative.t[1] = y;
    skeleton->relative.t[2] = z;
    skeleton->update = 1;
}


/* Builds the local rotation matrix from X, Y, and Z angles. */
void SetSkeletonDirection(Skeleton *skeleton, short xa, short ya, short za)
{
    short sz;
    short cx;
    short sy;
    short cy;
    short cz;
    short sx;

    sx = rsin(xa);
    sy = rsin(ya);
    sz = rsin(za);
    cx = rcos(xa);
    cy = rcos(ya);
    cz = rcos(za);

    skeleton->relative.m[0][0] = (cy * cz) >> 12;
    skeleton->relative.m[0][1] = (-cy * sz) >> 12;
    skeleton->relative.m[0][2] = -sy;

    skeleton->relative.m[1][0] = ((cx * sz) >> 12) - ((((sx * sy) >> 12) * cz) >> 12);
    skeleton->relative.m[1][1] = ((cx * cz) >> 12) + ((((sx * sy) >> 12) * sz) >> 12);
    skeleton->relative.m[1][2] = (-sx * cy) >> 12;

    skeleton->relative.m[2][0] = ((sx * sz) >> 12) + ((((cx * sy) >> 12) * cz) >> 12);
    skeleton->relative.m[2][1] = ((sx * cz) >> 12) - ((((cx * sy) >> 12) * sz) >> 12);
    skeleton->relative.m[2][2] = (cx * cy) >> 12;

    skeleton->update = 1;
}


/* Builds the local rotation matrix from heading, roll, and pitch. */
void SetSkeletonDirectionHRP(Skeleton *skeleton, short hdg, short roll, short pitch)
{
    short sx, sy, sz;
    short cx, cy, cz;

    sy = rsin(hdg);
    sx = rsin(pitch);
    sz = rsin(roll);
    cy = rcos(hdg);
    cx = rcos(pitch);
    cz = rcos(roll);

    skeleton->relative.m[0][0] = (cz * cy) >> 12;
    skeleton->relative.m[0][1] = (((-sy) * sx) >> 12) + ((((cy * sz) >> 12) * cx) >> 12);
    skeleton->relative.m[0][2] = (((-sy) * cx) >> 12) - ((((cy * sz) >> 12) * sx) >> 12);
    skeleton->relative.m[1][0] = -sz;
    skeleton->relative.m[1][1] = (cz * cx) >> 12;
    skeleton->relative.m[1][2] = ((-cz) * sx) >> 12;
    skeleton->relative.m[2][0] = (sy * cz) >> 12;
    skeleton->relative.m[2][1] = ((cy * sx) >> 12) + ((((sy * sz) >> 12) * cx) >> 12);
    skeleton->relative.m[2][2] = ((cx * cy) >> 12) - ((((sy * sz) >> 12) * sx) >> 12);

    skeleton->update = 1;
}


/* Builds the local rotation matrix from heading, pitch, and roll. */
void SetSkeletonDirectionHPR(Skeleton *skeleton, short hdg, short pitch, short roll)
{
    short sx, sy, sz;
    short cx, cy, cz;

    sy = rsin(hdg);
    sx = rsin(pitch);
    sz = rsin(roll);
    cy = rcos(hdg);
    cx = rcos(pitch);
    cz = rcos(roll);

    skeleton->relative.m[0][0] = ((cy * cz) >> 12) + ((((sy * sz) >> 12) * sx) >> 12);
    skeleton->relative.m[0][1] = ((cy * sz) >> 12) - ((((sy * sx) >> 12) * cz) >> 12);
    skeleton->relative.m[0][2] = ((-sy) * cx) >> 12;
    skeleton->relative.m[1][0] = (-(sz * cx)) >> 12;
    skeleton->relative.m[1][1] = (cx * cz) >> 12;
    skeleton->relative.m[1][2] = -sx;
    skeleton->relative.m[2][0] = ((sy * cz) >> 12) - ((((cy * sx) >> 12) * sz) >> 12);
    skeleton->relative.m[2][1] = ((sy * sz) >> 12) + ((((cy * sx) >> 12) * cz) >> 12);
    skeleton->relative.m[2][2] = (cx * cy) >> 12;

    skeleton->update = 1;
}


/* Builds the local rotation matrix from roll, pitch, and heading. */
void SetSkeletonDirectionRPH(Skeleton *skeleton, short roll, short pitch, short hdg)
{
    short cy;
    short cz;
    short sx;
    short sy;
    short cx;
    short sz;

    sy = rsin(hdg);
    sx = rsin(pitch);
    sz = rsin(roll);
    cy = rcos(hdg);
    cx = rcos(pitch);
    cz = rcos(roll);

    skeleton->relative.m[0][0] = ((cy * cz) >> 12) - ((((sz * sx) >> 12) * sy) >> 12);
    skeleton->relative.m[0][1] = (sz * cx) >> 12;
    skeleton->relative.m[0][2] = ((-(sy * cz)) >> 12) - ((((sz * sx) >> 12) * cy) >> 12);
    skeleton->relative.m[1][0] = ((-(sz * cy)) >> 12) - ((((sx * cz) >> 12) * sy) >> 12);
    skeleton->relative.m[1][1] = (cx * cz) >> 12;
    skeleton->relative.m[1][2] = ((sz * sy) >> 12) - ((((sx * cz) >> 12) * cy) >> 12);
    skeleton->relative.m[2][0] = (cx * sy) >> 12;
    skeleton->relative.m[2][1] = sx;
    skeleton->relative.m[2][2] = (cx * cy) >> 12;

    skeleton->update = 1;
}


MATRIX *GetSkeletonMatrix(Skeleton *skeleton)
{
    MATRIX *relMatrix;
    MATRIX *superMatrix;
    MATRIX *absMatrix;

    if (!skeleton->super) {
        return &skeleton->relative;
    }
    if (skeleton->update) {
        relMatrix = &skeleton->relative;
        superMatrix = GetSkeletonMatrix(skeleton->super);
        absMatrix = &skeleton->absolute;

        absMatrix->t[0] = superMatrix->t[0] + ((relMatrix->t[0] * superMatrix->m[0][0] + relMatrix->t[1] * superMatrix->m[0][1] + relMatrix->t[2] * superMatrix->m[0][2]) >> 12);
        absMatrix->t[1] = superMatrix->t[1] + ((relMatrix->t[0] * superMatrix->m[1][0] + relMatrix->t[1] * superMatrix->m[1][1] + relMatrix->t[2] * superMatrix->m[1][2]) >> 12);
        absMatrix->t[2] = superMatrix->t[2] + ((relMatrix->t[0] * superMatrix->m[2][0] + relMatrix->t[1] * superMatrix->m[2][1] + relMatrix->t[2] * superMatrix->m[2][2]) >> 12);

        MulMatrix0(superMatrix, relMatrix, absMatrix);

        skeleton->update = 0;
    }
    return &skeleton->absolute;
}


/* Recalculates the skeleton's world transform when it is marked dirty. */
void SetSkeletonTransform(Skeleton *skeleton)
{
    MATRIX *matrix;

    matrix = GetSkeletonMatrix(skeleton);
    SetTransMatrix(matrix);
    SetRotMatrix(matrix);
}
