#include "functions.h"
#include <math.h>
#include <string.h>

#ifdef NETPLAY
#pragma intrinsic(abs)
#pragma intrinsic(atan2)
#pragma intrinsic(log10)
#pragma intrinsic(sin, cos, pow)
#pragma intrinsic(sqrt)
#endif

/* Geometry-engine emulation (the PS1 libgte, written in plain C for the PC build).
   Angles are 4096 to a turn; rsin and rcos index SineTable, which holds one whole turn. */

typedef struct {
    long r, g, b, cd;
} GTECVECTOR;

#define TableSize 4096

extern MATRIX LCM;
extern MATRIX LLM;
extern MATRIX RTM;
extern SVECTOR LLV[3];
extern GTECVECTOR grgb[3];
extern GTECVECTOR BK;
extern struct { unsigned long r, g, b, cd; } FC;
extern long FFAR;
extern long FNEAR;
extern long xOffset;
extern long yOffset;
extern long screenZ;
extern long SCR2;
extern long stackPtr;
extern MATRIX stack[20];
extern long gsxy[3];
extern long gsz[4];
extern long MaxZValue;
extern long HalfZValue;
extern double SineTable[TableSize];

#define SHIFT 12
#define SHIFT2 24
#define ONE_FLOAT 4096.0
#define ONE 4096

long ratan2(long y, long x)
{
    /* 652 is 4096 / (2 * pi): the radian result becomes 4096 units to a turn. */
    return ((long)((atan2((double)y, (double)x)) * 652));
}

void CreateSineTable(void)
{
    long angle;
#ifdef NETPLAY
    for (angle = 0; angle < TableSize; angle++)
        SineTable[angle] = (double)rsin(angle);
#else
    long index, result;

    for (angle = 0; angle < TableSize; angle++) {
        index = angle;
        if (abs(index) > TableSize)
            index %= TableSize;
        if (index < 0)
            index = -index;
        if (angle < 0.0)
            result = -SineTable[index];
        else
            result = SineTable[index];
        SineTable[angle] = result;
    }
#endif
}

#ifndef NETPLAY
long rsin(long a)
{
    long index, result;

    index = a;
    if (abs(index) > TableSize)
        index %= TableSize;
    if (index < 0)
        index = -index;
    if (a < 0.0)
        result = -SineTable[index];
    else
        result = SineTable[index];

    return result;
}
#endif
#ifdef NETPLAY
long rsin(long a)
{
    long tableIndex, trigValue;

    tableIndex = a;
    if (abs(tableIndex) > TableSize)
        tableIndex %= TableSize;
    if (tableIndex < 0)
        tableIndex = -tableIndex;
    if (a < 0.0)
        trigValue = -SineTable[tableIndex];
    else
        trigValue = SineTable[tableIndex];

    return trigValue;
}
#endif

#ifndef NETPLAY
long rcos(long a)
{
    long index, result;

    index = a + 1024; /* a quarter turn: cos(a) is sin(a + 90 degrees) */
    if (abs(index) > TableSize)
        index %= TableSize;
    if (index < 0) {
        index = -index;
        result = -SineTable[index];
    } else
        result = SineTable[index];

    return result;
}
#endif
#ifdef NETPLAY
long rcos(long a)
{
    long tableIndex, trigValue;

    tableIndex = a + 1024; /* a quarter turn: cos(a) is sin(a + 90 degrees) */
    if (abs(tableIndex) > TableSize)
        tableIndex %= TableSize;
    if (tableIndex < 0) {
        tableIndex = -tableIndex;
        trigValue = -SineTable[tableIndex];
    } else
        trigValue = SineTable[tableIndex];

    return trigValue;
}
#endif

long lnFOG(long a)
{
    double u;

    u = (double)(a - FNEAR) / (double)(FFAR - FNEAR) * 256.0;

    if (u < 1)
        return 0;

    if (u > 256)
        return ONE;

    /* 1700.827 is 4096 / log10(256), so the 1..256 log range maps onto 0..ONE. */
    return (long)(log10(u) * 1700.827);
}

#ifndef NETPLAY
VECTOR *ApplyMatrixCC(MATRIX *m, SVECTOR *v0, VECTOR *v1)
{
    long xt, yt, zt;
    long stage1, stage2, stage3;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;

    stage1 = (xt * m->m[0][0]) >> SHIFT;
    stage2 = (yt * m->m[0][1]) >> SHIFT;
    stage3 = (zt * m->m[0][2]) >> SHIFT;
    v1->vx = stage1 + stage2 + stage3 + m->t[0];

    stage1 = (xt * m->m[1][0]) >> SHIFT;
    stage2 = (yt * m->m[1][1]) >> SHIFT;
    stage3 = (zt * m->m[1][2]) >> SHIFT;
    v1->vy = stage1 + stage2 + stage3 + m->t[1];

    stage1 = (xt * m->m[2][0]) >> SHIFT;
    stage2 = (yt * m->m[2][1]) >> SHIFT;
    stage3 = (zt * m->m[2][2]) >> SHIFT;
    v1->vz = stage1 + stage2 + stage3 + m->t[2];

    return v1;
}
#endif
#ifdef NETPLAY
VECTOR *ApplyMatrixCC(MATRIX *m, SVECTOR *v0, VECTOR *v1)
{
    long xValue, term1, term2, term3, zValue, yValue;

    xValue = v0->vx;
    yValue = v0->vy;
    zValue = v0->vz;

    term1 = (xValue * m->m[0][0]) >> SHIFT;
    term2 = (yValue * m->m[0][1]) >> SHIFT;
    term3 = (zValue * m->m[0][2]) >> SHIFT;
    v1->vx = term1 + term2 + term3 + m->t[0];

    term1 = (xValue * m->m[1][0]) >> SHIFT;
    term2 = (yValue * m->m[1][1]) >> SHIFT;
    term3 = (zValue * m->m[1][2]) >> SHIFT;
    v1->vy = term1 + term2 + term3 + m->t[1];

    term1 = (xValue * m->m[2][0]) >> SHIFT;
    term2 = (yValue * m->m[2][1]) >> SHIFT;
    term3 = (zValue * m->m[2][2]) >> SHIFT;
    v1->vz = term1 + term2 + term3 + m->t[2];

    return v1;
}
#endif

long AverageZ3CC(long sz0, long sz1, long sz2)
{
    long average, term;

    average = (sz0 + sz1 + sz2) >> 2;
    term = average;
    term >>= 2;
    average += term;
    term >>= 2;
    average += term;
    term >>= 2;
    average += term;
    term >>= 2;
    average += term;
    return (average >> 2);
}

long AverageZ4CC(long sz0, long sz1, long sz2, long sz3)
{
#ifdef NETPLAY
    return (sz3 + sz2 + sz1 + sz0) / 16;
#else
    return (sz0 + sz1 + sz2 + sz3) / 16;
#endif
}

void ColorCol(VECTOR *v0, CVECTOR *v1, CVECTOR *v2)
{
    long xt, yt, zt;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;

    grgb[2].r = (((xt * LCM.m[0][0]) + (zt * LCM.m[0][2]) + (yt * LCM.m[0][1])) >> SHIFT) + (BK.r);
    grgb[2].g = (((zt * LCM.m[1][2]) + (xt * LCM.m[1][0]) + (yt * LCM.m[1][1])) >> SHIFT) + (BK.g);
    grgb[2].b = (((zt * LCM.m[2][2]) + (xt * LCM.m[2][0]) + (yt * LCM.m[2][1])) >> SHIFT) + (BK.b);

    v2->r = grgb[2].r = (v1->r * grgb[2].r) >> SHIFT;
    v2->g = grgb[2].g = (v1->g * grgb[2].g) >> SHIFT;
    v2->b = grgb[2].b = (v1->b * grgb[2].b) >> SHIFT;
}

void ColorDpq(VECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2)
{
    long xt, yt, zt;
    long p2 = ONE - p;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;

    grgb[2].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    grgb[2].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    grgb[2].b = (((zt * LCM.m[2][2]) + (yt * LCM.m[2][1]) + (xt * LCM.m[2][0])) >> SHIFT) + (BK.b);

    v2->r = ((p2 * v1->r * grgb[2].r) + (p * FC.r)) >> SHIFT;
    v2->g = ((p2 * v1->g * grgb[2].g) + (p * FC.g)) >> SHIFT;
    v2->b = ((p2 * v1->b * grgb[2].b) + (p * FC.b)) >> SHIFT;
}

#ifdef EQUIV
void ColorMatCol(SVECTOR *v0, CVECTOR *v1, CVECTOR *v2, long matc)
{
    long xt, yt, zt;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;

    LLV[2].vx = (((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT);
    LLV[2].vy = (((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT);
    LLV[2].vz = (((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT);

    LLV[2].vx = pow(LLV[2].vx, (pow(2, matc)));
    LLV[2].vy = pow(LLV[2].vy, (pow(2, matc)));
    LLV[2].vz = pow(LLV[2].vz, (pow(2, matc)));

    xt = (long)(LLV[2].vx);
    yt = (long)(LLV[2].vy);
    zt = (long)(LLV[2].vz);

    grgb[2].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    grgb[2].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    grgb[2].b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + (BK.b);

    xt = (long)(grgb[2].r);
    yt = (long)(grgb[2].g);
    zt = (long)(grgb[2].b);

    v2->r = ((xt * v1->r) + (yt * v1->r) + (zt * v1->r)) >> SHIFT;
    v2->g = ((xt * v1->g) + (yt * v1->g) + (zt * v1->g)) >> SHIFT;
    v2->b = ((xt * v1->b) + (yt * v1->b) + (zt * v1->b)) >> SHIFT;
}
#else
void ColorMatCol(SVECTOR *v0, CVECTOR *v1, CVECTOR *v2, long matc)
{
    long xt, yt, zt;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;

    LLV[2].vx = (((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT);
    LLV[2].vy = (((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT);
    LLV[2].vz = (((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT);

    LLV[2].vx = pow(LLV[2].vx, (pow(2, matc)));
    LLV[2].vy = pow(LLV[2].vy, (pow(2, matc)));
    LLV[2].vz = pow(LLV[2].vz, (pow(2, matc)));

    xt = (long)(LLV[2].vx);
    yt = (long)(LLV[2].vy);
    zt = (long)(LLV[2].vz);

    grgb[2].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    grgb[2].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    grgb[2].b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + (BK.b);

    xt = (long)(grgb[2].r);
    yt = (long)(grgb[2].g);
    zt = (long)(grgb[2].b);

    v2->r = ((xt * v1->r) + (yt * v1->r) + (zt * v1->r)) >> SHIFT;
    v2->g = ((xt * v1->g) + (yt * v1->g) + (zt * v1->g)) >> SHIFT;
    v2->b = ((xt * v1->b) + (yt * v1->b) + (zt * v1->b)) >> SHIFT;
}
#endif
#ifdef EQUIV
#ifdef NETPLAY
void ColorMatDpq(SVECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2, long matc)
{
    long xt, yt, zt;
    long p2 = ONE - p;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;

    LLV[2].vx = ((zt * LLM.m[0][2]) + (yt * LLM.m[0][1]) + (xt * LLM.m[0][0])) >> SHIFT;
    LLV[2].vy = ((zt * LLM.m[1][2]) + (xt * LLM.m[1][0]) + (yt * LLM.m[1][1])) >> SHIFT;
    LLV[2].vz = ((zt * LLM.m[2][2]) + (xt * LLM.m[2][0]) + (yt * LLM.m[2][1])) >> SHIFT;

    LLV[2].vx = pow(LLV[2].vx, (pow(2, matc)));
    LLV[2].vy = pow(LLV[2].vy, (pow(2, matc)));
    LLV[2].vz = pow(LLV[2].vz, (pow(2, matc)));

    xt = (long)(LLV[2].vx);
    yt = (long)(LLV[2].vy);
    zt = (long)(LLV[2].vz);

    grgb[2].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    grgb[2].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    grgb[2].b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + (BK.b);

    v2->r = grgb[2].r = ((p2 * v1->r * grgb[2].r) + (p * FC.r)) >> SHIFT2;
    v2->g = grgb[2].g = ((p2 * v1->g * grgb[2].g) + (p * FC.g)) >> SHIFT2;
    v2->b = grgb[2].b = ((p2 * v1->b * grgb[2].b) + (p * FC.b)) >> SHIFT2;
}
#else
void ColorMatDpq(SVECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2, long matc)
{
    long xt, yt, zt;
    long p2 = ONE - p;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;

    LLV[2].vx = ((zt * LLM.m[0][2]) + (yt * LLM.m[0][1]) + (xt * LLM.m[0][0])) >> SHIFT;
    LLV[2].vy = ((zt * LLM.m[1][2]) + (xt * LLM.m[1][0]) + (yt * LLM.m[1][1])) >> SHIFT;
    LLV[2].vz = ((zt * LLM.m[2][2]) + (xt * LLM.m[2][0]) + (yt * LLM.m[2][1])) >> SHIFT;

    LLV[2].vx = pow(LLV[2].vx, (pow(2, matc)));
    LLV[2].vy = pow(LLV[2].vy, (pow(2, matc)));
    LLV[2].vz = pow(LLV[2].vz, (pow(2, matc)));

    xt = (long)(LLV[2].vx);
    yt = (long)(LLV[2].vy);
    zt = (long)(LLV[2].vz);

    grgb[2].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    grgb[2].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    grgb[2].b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + (BK.b);

    v2->r = grgb[2].r = ((p2 * v1->r * grgb[2].r) + (p * FC.r)) >> SHIFT2;
    v2->g = grgb[2].g = ((p2 * v1->g * grgb[2].g) + (p * FC.g)) >> SHIFT2;
    v2->b = grgb[2].b = ((p2 * v1->b * grgb[2].b) + (p * FC.b)) >> SHIFT2;
}
#endif
#endif

long csqrt(long a)
{
    unsigned long radicand;

    radicand = (unsigned long)a;
    return (long)(sqrt((double)radicand) * 64);
}

void DpqColor(CVECTOR *v0, long p, CVECTOR *v1)
{
    long p2 = ONE - p;

    v1->r = ((p2 * v0->r) + (p * FC.r)) >> SHIFT;
    v1->g = ((p2 * v0->g) + (p * FC.g)) >> SHIFT;
    v1->b = ((p2 * v0->b) + (p * FC.b)) >> SHIFT;
}

void DpqColor3(CVECTOR *v0, CVECTOR *v1, CVECTOR *v2, long p, CVECTOR *v3, CVECTOR *v4, CVECTOR *v5)
{
    long p2 = ONE - p;

    v3->r = grgb[2].r = ((p2 * v0->r) + (p * FC.r)) >> SHIFT;
    v3->g = grgb[2].g = ((p2 * v0->g) + (p * FC.g)) >> SHIFT;
    v3->b = grgb[2].b = ((p2 * v0->b) + (p * FC.b)) >> SHIFT;

    v4->r = grgb[2].r = ((p2 * v1->r) + (p * FC.r)) >> SHIFT;
    v4->g = grgb[2].g = ((p2 * v1->g) + (p * FC.g)) >> SHIFT;
    v4->b = grgb[2].b = ((p2 * v1->b) + (p * FC.b)) >> SHIFT;

    v5->r = grgb[2].r = ((p2 * v2->r) + (p * FC.r)) >> SHIFT;
    v5->g = grgb[2].g = ((p2 * v2->g) + (p * FC.g)) >> SHIFT;
    v5->b = grgb[2].b = ((p2 * v2->b) + (p * FC.b)) >> SHIFT;
}

void DpqColorLight(VECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2)
{
    long p2 = ONE - p;

    v2->r = grgb[2].r = ((p2 * v1->r * v1->r) + (p * FC.r)) >> SHIFT2;
    v2->g = grgb[2].g = ((p2 * v1->g * v1->g) + (p * FC.g)) >> SHIFT2;
    v2->b = grgb[2].b = ((p2 * v1->b * v1->b) + (p * FC.b)) >> SHIFT2;
}

void FlipRotMatrixX(void)
{
    MATRIX RMat;

#ifdef NETPLAY
    ReadRotMatrix(&RMat);
#else
    RMat = RTM;
#endif

    RMat.m[0][0] = -RMat.m[0][0];
    RMat.m[0][1] = -RMat.m[0][1];
    RMat.m[0][2] = -RMat.m[0][2];

#ifdef NETPLAY
    SetRotMatrix(&RMat);
#else
    memcpy(&(RTM.m), &(RMat.m), sizeof(short) * 9);
#endif
}

void FlipTRX(void)
{
    MATRIX RMat;

#ifdef NETPLAY
    ReadRotMatrix(&RMat);
#else
    RMat = RTM;
#endif
    RMat.t[0] = -RMat.t[0];
#ifdef NETPLAY
    SetTransMatrix(&RMat);
#else
    memcpy(&(RTM.t), &(RMat.t), sizeof(long) * 3);
#endif
}

void gteMIMefunc(SVECTOR *otp, SVECTOR *dfp, long n, long p)
{
    int i;

    for (i = 0; i < n; i++) {
        (otp + i)->vx += ((int)((dfp + i)->vx) * p) >> SHIFT;
        (otp + i)->vy += ((int)((dfp + i)->vy) * p) >> SHIFT;
        (otp + i)->vz += ((int)((dfp + i)->vz) * p) >> SHIFT;
    }
}

void InitGeom(void)
{
    stackPtr = 0;

    RTM.m[0][0] = 16652;
    RTM.m[0][1] = -31805;
    RTM.m[0][2] = -30657;
    RTM.m[1][0] = 17100;
    RTM.m[1][1] = 26345;
    RTM.m[1][2] = 25272;
    RTM.m[2][0] = 6882;
    RTM.m[2][1] = -11327;
    RTM.m[2][2] = 20672;

    RTM.t[0] = 0x40000401;
    RTM.t[1] = 0x40000401;
    RTM.t[2] = 0x40000401;

    LCM.m[0][0] = ONE;
    LCM.m[0][1] = 0;
    LCM.m[0][2] = 0;
    LCM.m[1][0] = 0;
    LCM.m[1][1] = ONE;
    LCM.m[1][2] = 0;
    LCM.m[2][0] = 0;
    LCM.m[2][1] = 0;
    LCM.m[2][2] = ONE;

    LLM.m[0][0] = -28320;
    LLM.m[0][1] = -29804;
    LLM.m[0][2] = 9801;
    LLM.m[1][0] = 782;
    LLM.m[1][1] = -23358;
    LLM.m[1][2] = -29799;
    LLM.m[2][0] = 21952;
    LLM.m[2][1] = -22357;
    LLM.m[2][2] = 31393;

    xOffset = 0;
    yOffset = 0;
    screenZ = 0x3e8;
    SCR2 = screenZ >> 1;

    BK.r = 0;
    BK.g = 0;
    BK.b = 255;

    FC.r = 0;
    FC.g = 255;
    FC.b = 0;

    MaxZValue = 65535;
    HalfZValue = MaxZValue >> 1;
}

void Intpl(VECTOR *v0, long p, CVECTOR *v1)
{
    long p2 = ONE - p;

    v1->r = grgb[2].r = ((p2 * v0->vx) + (p * FC.r)) >> SHIFT;
    v1->g = grgb[2].g = ((p2 * v0->vy) + (p * FC.g)) >> SHIFT;
    v1->b = grgb[2].b = ((p2 * v0->vz) + (p * FC.b)) >> SHIFT;
}

void InterpolShort(SVECTOR *vi0, SVECTOR *vi1, long p0, SVECTOR *vt1)
{
    float p1 = (ONE - (float)p0) / ONE;

    vt1->vx = ((vi1->vx - vi0->vx) * p1) + vi0->vx;
    vt1->vy = ((vi1->vy - vi0->vy) * p1) + vi0->vy;
    vt1->vz = ((vi1->vz - vi0->vz) * p1) + vi0->vz;
}

void InvSquareRoot(long a, long *b, long *c)
{
    double mantissa;
    int exponent;

    mantissa = frexp(ONE / (sqrt(a)), &exponent);
    *b = (long)(mantissa);
    *c = (long)(exponent);
}

#ifdef NETPLAY
void LightColor(VECTOR *v0, VECTOR *v1)
{
    long x, y, z;

    x = v0->vx;
    y = v0->vy;
    z = v0->vz;

    v1->vx = (((z * LCM.m[0][2]) + (y * LCM.m[0][1])) + (x * LCM.m[0][0])) >> SHIFT;
    v1->vy = (((z * LCM.m[1][2]) + (y * LCM.m[1][1])) + (x * LCM.m[1][0])) >> SHIFT;
    v1->vz = (((z * LCM.m[2][2]) + (y * LCM.m[2][1])) + (x * LCM.m[2][0])) >> SHIFT;

    if (v1->vx < 0)
        v1->vx = 0;
    if (v1->vy < 0)
        v1->vy = 0;
    if (v1->vz < 0)
        v1->vz = 0;
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
void LightColor(VECTOR *v0, VECTOR *v1)
{
    long xt, yt, zt;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;

    v1->vx = (((zt * LCM.m[0][2]) + (yt * LCM.m[0][1])) + (xt * LCM.m[0][0])) >> SHIFT;
    v1->vy = (((zt * LCM.m[1][2]) + (yt * LCM.m[1][1])) + (xt * LCM.m[1][0])) >> SHIFT;
    v1->vz = (((zt * LCM.m[2][2]) + (yt * LCM.m[2][1])) + (xt * LCM.m[2][0])) >> SHIFT;

    if (v1->vx < 0)
        v1->vx = 0;
    if (v1->vy < 0)
        v1->vy = 0;
    if (v1->vz < 0)
        v1->vz = 0;
}
#endif
#endif

void LoadAverage0(VECTOR *v0, VECTOR *v1, long p0, long p1, VECTOR *v2)
{
    v2->vx = (v0->vx * p0) + (v1->vx * p1);
    v2->vy = (v0->vy * p0) + (v1->vy * p1);
    v2->vz = (v0->vz * p0) + (v1->vz * p1);
}

void LoadAverage12(VECTOR *v0, VECTOR *v1, long p0, long p1, VECTOR *v2)
{
    v2->vx = (((v0->vx) * p0) + ((v1->vx) * p1)) >> SHIFT;
    v2->vy = (((v0->vy) * p0) + ((v1->vy) * p1)) >> SHIFT;
    v2->vz = (((v0->vz) * p0) + ((v1->vz) * p1)) >> SHIFT;
}

void LoadAverageShort0(SVECTOR *v0, SVECTOR *v1, long p0, long p1, SVECTOR *v2)
{
    v2->vx = (v0->vx * p0) + (v1->vx * p1);
    v2->vy = (v0->vy * p0) + (v1->vy * p1);
    v2->vz = (v0->vz * p0) + (v1->vz * p1);
}

void LoadAverageShort12(SVECTOR *v0, SVECTOR *v1, long p0, long p1, SVECTOR *v2)
{
    v2->vx = (short)((((v0->vx) * p0) + ((v1->vx) * p1)) >> SHIFT);
    v2->vy = (short)((((v0->vy) * p0) + ((v1->vy) * p1)) >> SHIFT);
    v2->vz = (short)((((v0->vz) * p0) + ((v1->vz) * p1)) >> SHIFT);
}

#ifdef NETPLAY
void LocalLight(SVECTOR *v0, VECTOR *v1)
{
    long x, y, z;

    x = v0->vx;
    y = v0->vy;
    z = v0->vz;

    v1->vx = (((z * LLM.m[0][2]) + (y * LLM.m[0][1])) + (x * LLM.m[0][0])) >> SHIFT;
    v1->vy = ((z * LLM.m[1][2]) + ((x * LLM.m[1][0]) + (y * LLM.m[1][1]))) >> SHIFT;
    v1->vz = ((z * LLM.m[2][2]) + ((x * LLM.m[2][0]) + (y * LLM.m[2][1]))) >> SHIFT;

    if (v1->vx < 0)
        v1->vx = 0;
    if (v1->vy < 0)
        v1->vy = 0;
    if (v1->vz < 0)
        v1->vz = 0;
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
void LocalLight(SVECTOR *v0, VECTOR *v1)
{
    long xt, yt, zt;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;

    v1->vx = (((zt * LLM.m[0][2]) + (yt * LLM.m[0][1])) + (xt * LLM.m[0][0])) >> SHIFT;
    v1->vy = (((zt * LLM.m[1][2]) + (yt * LLM.m[1][1])) + (xt * LLM.m[1][0])) >> SHIFT;
    v1->vz = (((zt * LLM.m[2][2]) + (yt * LLM.m[2][1])) + (xt * LLM.m[2][0])) >> SHIFT;

    if (v1->vx < 0)
        v1->vx = 0;
    if (v1->vy < 0)
        v1->vy = 0;
    if (v1->vz < 0)
        v1->vz = 0;
}
#endif
#endif

#ifndef NETPLAY
long Lzc(long data)
{
    u_long shifted;
    long cnt = 0;

    if (data < 0)
        shifted = (u_long)(-data);
    else
        shifted = (u_long)(+data);

    while (shifted <= (u_long)0x7FFFFFFF) {
        shifted <<= 1;
        cnt++;
    }
    return cnt;
}
#endif
#ifdef NETPLAY
long Lzc(long data)
{
    u_long value;
    long count = 0;

    if (data < 0)
        value = (u_long)(-data);
    else
        value = (u_long)(+data);

    while (value <= (u_long)0x7FFFFFFF) {
        value <<= 1;
        count++;
    }
    return count;
}
#endif

MATRIX *MulMatrix(MATRIX *m0, MATRIX *m1)
{
    RTM.m[0][0] = ((m0->m[0][0] * m1->m[0][0]) + (m0->m[0][1] * m1->m[1][0]) + (m0->m[0][2] * m1->m[2][0])) >> SHIFT;
    RTM.m[0][1] = ((m0->m[0][0] * m1->m[0][1]) + (m0->m[0][1] * m1->m[1][1]) + (m0->m[0][2] * m1->m[2][1])) >> SHIFT;
    RTM.m[0][2] = ((m0->m[0][0] * m1->m[0][2]) + (m0->m[0][1] * m1->m[1][2]) + (m0->m[0][2] * m1->m[2][2])) >> SHIFT;
    RTM.m[1][0] = ((m0->m[1][0] * m1->m[0][0]) + (m0->m[1][1] * m1->m[1][0]) + (m0->m[1][2] * m1->m[2][0])) >> SHIFT;
    RTM.m[1][1] = ((m0->m[1][0] * m1->m[0][1]) + (m0->m[1][1] * m1->m[1][1]) + (m0->m[1][2] * m1->m[2][1])) >> SHIFT;
    RTM.m[1][2] = ((m0->m[1][0] * m1->m[0][2]) + (m0->m[1][1] * m1->m[1][2]) + (m0->m[1][2] * m1->m[2][2])) >> SHIFT;
    RTM.m[2][0] = ((m0->m[2][0] * m1->m[0][0]) + (m0->m[2][1] * m1->m[1][0]) + (m0->m[2][2] * m1->m[2][0])) >> SHIFT;
    RTM.m[2][1] = ((m0->m[2][0] * m1->m[0][1]) + (m0->m[2][1] * m1->m[1][1]) + (m0->m[2][2] * m1->m[2][1])) >> SHIFT;
    RTM.m[2][2] = ((m0->m[2][0] * m1->m[0][2]) + (m0->m[2][1] * m1->m[1][2]) + (m0->m[2][2] * m1->m[2][2])) >> SHIFT;
    RTM.t[0] = m0->t[0];
    RTM.t[1] = m0->t[1];
    RTM.t[2] = m0->t[2];

    (*m0) = RTM;
    return m0;
}

MATRIX *MulMatrix0(MATRIX *m0, MATRIX *m1, MATRIX *m2)
{
    RTM.m[0][0] = ((m0->m[0][0] * m1->m[0][0]) + (m0->m[0][1] * m1->m[1][0]) + (m0->m[0][2] * m1->m[2][0])) >> SHIFT;
    RTM.m[0][1] = ((m0->m[0][0] * m1->m[0][1]) + (m0->m[0][1] * m1->m[1][1]) + (m0->m[0][2] * m1->m[2][1])) >> SHIFT;
    RTM.m[0][2] = ((m0->m[0][0] * m1->m[0][2]) + (m0->m[0][1] * m1->m[1][2]) + (m0->m[0][2] * m1->m[2][2])) >> SHIFT;
    RTM.m[1][0] = ((m0->m[1][0] * m1->m[0][0]) + (m0->m[1][1] * m1->m[1][0]) + (m0->m[1][2] * m1->m[2][0])) >> SHIFT;
    RTM.m[1][1] = ((m0->m[1][0] * m1->m[0][1]) + (m0->m[1][1] * m1->m[1][1]) + (m0->m[1][2] * m1->m[2][1])) >> SHIFT;
    RTM.m[1][2] = ((m0->m[1][0] * m1->m[0][2]) + (m0->m[1][1] * m1->m[1][2]) + (m0->m[1][2] * m1->m[2][2])) >> SHIFT;
    RTM.m[2][0] = ((m0->m[2][0] * m1->m[0][0]) + (m0->m[2][1] * m1->m[1][0]) + (m0->m[2][2] * m1->m[2][0])) >> SHIFT;
    RTM.m[2][1] = ((m0->m[2][0] * m1->m[0][1]) + (m0->m[2][1] * m1->m[1][1]) + (m0->m[2][2] * m1->m[2][1])) >> SHIFT;
    RTM.m[2][2] = ((m0->m[2][0] * m1->m[0][2]) + (m0->m[2][1] * m1->m[1][2]) + (m0->m[2][2] * m1->m[2][2])) >> SHIFT;
    RTM.t[0] = m2->t[0];
    RTM.t[1] = m2->t[1];
    RTM.t[2] = m2->t[2];

    (*m2) = RTM;
    return m2;
}

MATRIX *MulMatrix2(MATRIX *m0, MATRIX *m1)
{
    RTM.m[0][0] = ((m0->m[0][0] * m1->m[0][0]) + (m0->m[0][1] * m1->m[1][0]) + (m0->m[0][2] * m1->m[2][0])) >> SHIFT;
    RTM.m[0][1] = ((m0->m[0][0] * m1->m[0][1]) + (m0->m[0][1] * m1->m[1][1]) + (m0->m[0][2] * m1->m[2][1])) >> SHIFT;
    RTM.m[0][2] = ((m0->m[0][0] * m1->m[0][2]) + (m0->m[0][1] * m1->m[1][2]) + (m0->m[0][2] * m1->m[2][2])) >> SHIFT;
    RTM.m[1][0] = ((m0->m[1][0] * m1->m[0][0]) + (m0->m[1][1] * m1->m[1][0]) + (m0->m[1][2] * m1->m[2][0])) >> SHIFT;
    RTM.m[1][1] = ((m0->m[1][0] * m1->m[0][1]) + (m0->m[1][1] * m1->m[1][1]) + (m0->m[1][2] * m1->m[2][1])) >> SHIFT;
    RTM.m[1][2] = ((m0->m[1][0] * m1->m[0][2]) + (m0->m[1][1] * m1->m[1][2]) + (m0->m[1][2] * m1->m[2][2])) >> SHIFT;
    RTM.m[2][0] = ((m0->m[2][0] * m1->m[0][0]) + (m0->m[2][1] * m1->m[1][0]) + (m0->m[2][2] * m1->m[2][0])) >> SHIFT;
    RTM.m[2][1] = ((m0->m[2][0] * m1->m[0][1]) + (m0->m[2][1] * m1->m[1][1]) + (m0->m[2][2] * m1->m[2][1])) >> SHIFT;
    RTM.m[2][2] = ((m0->m[2][0] * m1->m[0][2]) + (m0->m[2][1] * m1->m[1][2]) + (m0->m[2][2] * m1->m[2][2])) >> SHIFT;
    RTM.t[0] = m1->t[0];
    RTM.t[1] = m1->t[1];
    RTM.t[2] = m1->t[2];

    (*m1) = RTM;
    return m1;
}

long NormalClip(long sxy0, long sxy1, long sxy2)
{
    long sx0, sy0, dx1, dy1, dx2, dy2;
    DVECTOR *xy0, *xy1, *xy2;

    xy0 = (DVECTOR *)&sxy0;
    xy1 = (DVECTOR *)&sxy1;
    xy2 = (DVECTOR *)&sxy2;

    dx1 = xy1->vx - xy0->vx;
    dy1 = xy1->vy - xy0->vy;
    dx2 = xy2->vx - xy0->vx;
    dy2 = xy2->vy - xy0->vy;

    return ((dx1 * dy2) - (dy1 * dx2));
}

/* Empty, as in the original. */
void NormalColor(SVECTOR *v0, CVECTOR *v1)
{
}

#ifdef EQUIV
#ifdef NETPLAY
void NormalColor3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4, CVECTOR *v5)
{
    long xt, yt, zt;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;

    LLV[0].vx = (((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT);
    LLV[0].vy = (((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT);
    LLV[0].vz = (((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT);

    xt = v1->vx;
    yt = v1->vy;
    zt = v1->vz;

    LLV[1].vx = (((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT);
    LLV[1].vy = (((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT);
    LLV[1].vz = (((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT);

    xt = v2->vx;
    yt = v2->vy;
    zt = v2->vz;

    LLV[2].vx = (((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT);
    LLV[2].vy = (((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT);
    LLV[2].vz = (((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT);

    xt = (long)(LLV[0].vx);
    yt = (long)(LLV[0].vy);
    zt = (long)(LLV[0].vz);

    v3->r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    v3->g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    v3->b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + (BK.b);

    xt = (long)(LLV[1].vx);
    yt = (long)(LLV[1].vy);
    zt = (long)(LLV[1].vz);

    v4->r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    v4->g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    v4->b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + (BK.b);

    xt = (long)(LLV[2].vx);
    yt = (long)(LLV[2].vy);
    zt = (long)(LLV[2].vz);

    v5->r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    v5->g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    v5->b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + (BK.b);
}
#else
void NormalColor3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4, CVECTOR *v5)
{
    long xt, yt, zt;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;

    LLV[0].vx = (((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT);
    LLV[0].vy = (((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT);
    LLV[0].vz = (((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT);

    xt = v1->vx;
    yt = v1->vy;
    zt = v1->vz;

    LLV[1].vx = (((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT);
    LLV[1].vy = (((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT);
    LLV[1].vz = (((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT);

    xt = v2->vx;
    yt = v2->vy;
    zt = v2->vz;

    LLV[2].vx = (((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT);
    LLV[2].vy = (((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT);
    LLV[2].vz = (((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT);

    xt = (long)(LLV[0].vx);
    yt = (long)(LLV[0].vy);
    zt = (long)(LLV[0].vz);

    v3->r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    v3->g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    v3->b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + (BK.b);

    xt = (long)(LLV[1].vx);
    yt = (long)(LLV[1].vy);
    zt = (long)(LLV[1].vz);

    v4->r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    v4->g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    v4->b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + (BK.b);

    xt = (long)(LLV[2].vx);
    yt = (long)(LLV[2].vy);
    zt = (long)(LLV[2].vz);

    v5->r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    v5->g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    v5->b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + (BK.b);
}
#endif
#endif

void NormalColorCol(SVECTOR *v0, CVECTOR *v1, CVECTOR *v2)
{
    long xt, yt, zt;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;

    LLV[2].vx = (((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT);
    LLV[2].vy = (((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT);
    LLV[2].vz = (((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT);

    xt = (long)(LLV[2].vx);
    yt = (long)(LLV[2].vy);
    zt = (long)(LLV[2].vz);

    grgb[2].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    grgb[2].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    grgb[2].b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + (BK.b);

    v2->r = grgb[2].r = (v1->r * grgb[2].r) >> SHIFT;
    v2->g = grgb[2].g = (v1->g * grgb[2].g) >> SHIFT;
    v2->b = grgb[2].b = (v1->b * grgb[2].b) >> SHIFT;
}

void NormalColorDpq(SVECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2)
{
    long xt, yt, zt;
    long p2 = ONE - p;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;

    LLV[2].vx = ((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT;
    LLV[2].vy = ((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT;
    LLV[2].vz = ((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT;

    xt = (long)(LLV[2].vx);
    yt = (long)(LLV[2].vy);
    zt = (long)(LLV[2].vz);

    grgb[2].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    grgb[2].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    grgb[2].b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + (BK.b);

    v2->r = ((p2 * v1->r * grgb[2].r) + (p * FC.r)) >> SHIFT;
    v2->g = ((p2 * v1->g * grgb[2].g) + (p * FC.g)) >> SHIFT;
    v2->b = ((p2 * v1->b * grgb[2].b) + (p * FC.b)) >> SHIFT;
}

#ifdef EQUIV
void OuterProduct0(VECTOR *v0, VECTOR *v1, VECTOR *v2)
{
    v2->vx = ((v0->vy * v1->vz) - (v0->vz * v1->vy));
    v2->vy = ((v0->vz * v1->vx) - (v0->vx * v1->vz));
    v2->vz = ((v0->vx * v1->vy) - (v0->vy * v1->vx));
}
#else
void OuterProduct0(VECTOR *v0, VECTOR *v1, VECTOR *v2)
{
    v2->vx = ((v0->vy * v1->vz) - (v0->vz * v1->vy));
    v2->vy = ((v0->vz * v1->vx) - (v0->vx * v1->vz));
    v2->vz = ((v0->vx * v1->vy) - (v0->vy * v1->vx));
}
#endif
#ifdef EQUIV
void OuterProduct12(VECTOR *v0, VECTOR *v1, VECTOR *v2)
{
    v2->vx = ((v0->vy * v1->vz) - (v0->vz * v1->vy)) >> SHIFT;
    v2->vy = ((v0->vz * v1->vx) - (v0->vx * v1->vz)) >> SHIFT;
    v2->vz = ((v0->vx * v1->vy) - (v0->vy * v1->vx)) >> SHIFT;
}
#else
void OuterProduct12(VECTOR *v0, VECTOR *v1, VECTOR *v2)
{
    v2->vx = ((v0->vy * v1->vz) - (v0->vz * v1->vy)) >> SHIFT;
    v2->vy = ((v0->vz * v1->vx) - (v0->vx * v1->vz)) >> SHIFT;
    v2->vz = ((v0->vx * v1->vy) - (v0->vy * v1->vx)) >> SHIFT;
}
#endif

#ifndef NETPLAY
void PopMatrix(void)
{
    if (stackPtr != 0) {
        stackPtr--;
        RTM = stack[stackPtr];
    }
}
#endif
#ifdef NETPLAY
void PopMatrix(void)
{
    if (stackPtr == 0)
        return;
    stackPtr--;
    RTM = stack[stackPtr];
}
#endif

#ifndef NETPLAY
void PushMatrix(void)
{
    if (stackPtr != 20) {
        stack[stackPtr] = RTM;
        stackPtr++;
    }
}
#endif
#ifdef NETPLAY
void PushMatrix(void)
{
    if (stackPtr == 20)
        return;
    stack[stackPtr] = RTM;
    stackPtr++;
}
#endif

void ReadColorMatrix(MATRIX *m)
{
    *m = LCM;
}

void ReadLightMatrix(MATRIX *m)
{
    /* PowerVR stores the light-matrix read through RTM. */
#ifdef POWERVR
    *m = RTM;
#else
    *m = LLM;
#endif
}

void ReadRGBfifo(CVECTOR *v0, CVECTOR *v1, CVECTOR *v2)
{
    v0->r = grgb[0].r;
    v0->g = grgb[0].g;
    v0->b = grgb[0].b;
    v1->r = grgb[1].r;
    v1->g = grgb[1].g;
    v1->b = grgb[1].b;
    v2->r = grgb[2].r;
    v2->g = grgb[2].g;
    v2->b = grgb[2].b;
}

void ReadRotMatrix(MATRIX *m)
{
    /* PowerVR stores the rotation-matrix read through LLM. */
#ifdef POWERVR
    *m = LLM;
#else
    *m = RTM;
#endif
}

void ReadSXSYfifo(long *sxy0, long *sxy1, long *sxy2)
{
    *sxy0 = gsxy[0];
    *sxy1 = gsxy[1];
    *sxy2 = gsxy[2];
}

void ReadSZfifo3(long *sz0, long *sz1, long *sz2)
{
    *sz0 = gsz[1];
    *sz1 = gsz[2];
    *sz2 = gsz[3];
}

/* Faithful to the original: sz2 reads gsz[2], the same slot as sz1, not gsz[3]. */
void ReadSZfifo4(long *sz, long *sz0, long *sz1, long *sz2)
{
    *sz = gsz[0];
    *sz0 = gsz[1];
    *sz1 = gsz[2];
    *sz2 = gsz[2];
}

/* Original inline assembly: the ApplyMatrix block repeated in RotTrans and
   the projection wrappers, with word loads and SHL/SAR sign extension. */
static __inline void ApplyMatrixInline(MATRIX *m, SVECTOR *vIn, VECTOR *r)
{
    long xt, yt, zt;

    __asm {
        mov eax, m
        mov ebx, vIn
        mov ecx, r
        mov edi, eax
        mov edx, ecx
        mov cx, [ebx]
        mov ax, [ebx + 2]
        mov bx, [ebx + 4]
        shl ecx, 16
        shl eax, 16
        shl ebx, 16
        sar ecx, 16
        sar eax, 16
        sar ebx, 16
        mov xt, ecx
        mov yt, eax
        mov zt, ebx
        mov cx, [edi]
        mov ax, [edi + 2]
        mov bx, [edi + 4]
        shl ecx, 16
        shl eax, 16
        shl ebx, 16
        sar ecx, 16
        sar eax, 16
        sar ebx, 16
        imul ecx, xt
        imul eax, yt
        imul ebx, zt
        sar ecx, 12
        sar eax, 12
        sar ebx, 12
        add ecx, eax
        add ecx, ebx
        add ecx, [edi + 20]
        mov [edx], ecx
        mov cx, [edi + 6]
        mov ax, [edi + 8]
        mov bx, [edi + 10]
        shl ecx, 16
        shl eax, 16
        shl ebx, 16
        sar ecx, 16
        sar eax, 16
        sar ebx, 16
        imul ecx, xt
        imul eax, yt
        imul ebx, zt
        sar ecx, 12
        sar eax, 12
        sar ebx, 12
        add ecx, eax
        add ecx, ebx
        add ecx, [edi + 24]
        mov [edx + 4], ecx
        mov cx, [edi + 12]
        mov ax, [edi + 14]
        mov bx, [edi + 16]
        shl ecx, 16
        shl eax, 16
        shl ebx, 16
        sar ecx, 16
        sar eax, 16
        sar ebx, 16
        imul ecx, xt
        imul eax, yt
        imul ebx, zt
        sar ecx, 12
        sar eax, 12
        sar ebx, 12
        add ecx, eax
        add ecx, ebx
        add ecx, [edi + 28]
        mov [edx + 8], ecx
    }
}

/* Original inline assembly: the RotTransPersV projection block repeated in
   the wrappers; one-operand IMUL preserves EDX:EAX for IDIV without CDQ. */
static __inline void PerspectiveInline(long *sxy, VECTOR *r0, long *flagOut)
{
    long sxyAddr;
    VECTOR *r0Addr;
    long *flagAddr;
    long flag;
    long x, y, z;

    __asm {
        mov eax, sxy
        mov ebx, r0
        mov ecx, flagOut
        mov sxyAddr, eax
        mov r0Addr, ebx
        mov flagAddr, ecx
        mov flag, 0
        mov eax, [ebx]
        mov x, eax
        mov eax, [ebx + 4]
        mov y, eax
        mov eax, [ebx + 8]
        mov z, eax
        cmp z, 0
        jge nonegz
        or flag, 0x00040000
    nonegz:
        mov ebx, [HalfZValue]
        cmp z, ebx
        jle nobigz
        or flag, 0x00400000
    nobigz:
        mov ebx, [SCR2]
        cmp z, ebx
        jge noscr2clip
        mov z, ebx
        or flag, 0x00020000
    noscr2clip:
        mov ebx, [MaxZValue]
        cmp z, ebx
        jle nomaxzclip
        mov z, ebx
    nomaxzclip:
        mov ebx, [FNEAR]
        cmp z, ebx
        jge nozfnear
        or flag, 0x00001000
        jmp nozffar
    nozfnear:
        mov ebx, [FFAR]
        cmp z, ebx
        jle nozffar
        or flag, 0x00001000
    nozffar:
        cmp x, 32767
        jle nomaxxclip
        mov x, 32767
        or flag, 0x81000000
    nomaxxclip:
        cmp y, 32767
        jle nomaxyclip
        mov y, 32767
        or flag, 0x80800000
    nomaxyclip:
        cmp x, -32767
        jge nominxclip
        mov x, -32767
        or flag, 0x81000000
    nominxclip:
        cmp y, -32767
        jge nominyclip
        mov y, -32767
        or flag, 0x80800000
    nominyclip:
        mov eax, [screenZ]
        imul x
        idiv z
        add eax, [xOffset]
        mov ebx, eax
        mov eax, [screenZ]
        imul y
        idiv z
        add eax, [yOffset]
        mov ecx, eax
        cmp ebx, -1024
        jge nominxt0clip
        mov ebx, -1024
        or flag, 0x80004000
    nominxt0clip:
        cmp ebx, 1023
        jle nomaxxt0clip
        mov ebx, 1023
        or flag, 0x80004000
    nomaxxt0clip:
        cmp ecx, -1024
        jge nominyt0clip
        mov ecx, -1024
        or flag, 0x80002000
    nominyt0clip:
        cmp ecx, 1023
        jle nomaxyt0clip
        mov ecx, 1023
        or flag, 0x80002000
    nomaxyt0clip:
        and ebx, 0x0000FFFF
        shl ecx, 16
        or ebx, ecx
        mov edx, sxyAddr
        mov [edx], ebx
        mov ebx, flag
        mov edx, flagAddr
        mov [edx], ebx
    }
}

#ifdef EQUIV
/* Three-normal color path, proven equivalent to the executable. */
#ifdef NETPLAY
void NormalColorCol3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4, CVECTOR *v5, CVECTOR *v6)
{
    long vx, vy, vz;

    vx = v0->vx;
    vy = v0->vy;
    vz = v0->vz;
    LLV[0].vx = ((vx * LLM.m[0][0]) + (vy * LLM.m[0][1]) + (vz * LLM.m[0][2])) >> SHIFT;
    LLV[0].vy = ((vx * LLM.m[1][0]) + (vy * LLM.m[1][1]) + (vz * LLM.m[1][2])) >> SHIFT;
    LLV[0].vz = ((vx * LLM.m[2][0]) + (vy * LLM.m[2][1]) + (vz * LLM.m[2][2])) >> SHIFT;

    vx = v1->vx;
    vy = v1->vy;
    vz = v1->vz;
    LLV[1].vx = ((vx * LLM.m[0][0]) + (vy * LLM.m[0][1]) + (vz * LLM.m[0][2])) >> SHIFT;
    LLV[1].vy = ((vx * LLM.m[1][0]) + (vy * LLM.m[1][1]) + (vz * LLM.m[1][2])) >> SHIFT;
    LLV[1].vz = ((vx * LLM.m[2][0]) + (vy * LLM.m[2][1]) + (vz * LLM.m[2][2])) >> SHIFT;

    vx = v2->vx;
    vy = v2->vy;
    vz = v2->vz;
    LLV[2].vx = ((vx * LLM.m[0][0]) + (vy * LLM.m[0][1]) + (vz * LLM.m[0][2])) >> SHIFT;
    LLV[2].vy = ((vx * LLM.m[1][0]) + (vy * LLM.m[1][1]) + (vz * LLM.m[1][2])) >> SHIFT;
    LLV[2].vz = ((vx * LLM.m[2][0]) + (vy * LLM.m[2][1]) + (vz * LLM.m[2][2])) >> SHIFT;

    vx = LLV[0].vx;
    vy = LLV[0].vy;
    vz = LLV[0].vz;
    grgb[0].r = (((vx * LCM.m[0][0]) + (vy * LCM.m[0][1]) + (vz * LCM.m[0][2])) >> SHIFT) + BK.r;
    grgb[0].g = (((vx * LCM.m[1][0]) + (vy * LCM.m[1][1]) + (vz * LCM.m[1][2])) >> SHIFT) + BK.g;
    grgb[0].b = (((vx * LCM.m[2][0]) + (vy * LCM.m[2][1]) + (vz * LCM.m[2][2])) >> SHIFT) + BK.b;

    vx = LLV[1].vx;
    vy = LLV[1].vy;
    vz = LLV[1].vz;
    grgb[1].r = (((vx * LCM.m[0][0]) + (vy * LCM.m[0][1]) + (vz * LCM.m[0][2])) >> SHIFT) + BK.r;
    grgb[1].g = (((vx * LCM.m[1][0]) + (vy * LCM.m[1][1]) + (vz * LCM.m[1][2])) >> SHIFT) + BK.g;
    grgb[1].b = (((vx * LCM.m[2][0]) + (vy * LCM.m[2][1]) + (vz * LCM.m[2][2])) >> SHIFT) + BK.b;

    vx = LLV[2].vx;
    vy = LLV[2].vy;
    vz = LLV[2].vz;
    grgb[2].r = (((vx * LCM.m[0][0]) + (vy * LCM.m[0][1]) + (vz * LCM.m[0][2])) >> SHIFT) + BK.r;
    grgb[2].g = (((vx * LCM.m[1][0]) + (vy * LCM.m[1][1]) + (vz * LCM.m[1][2])) >> SHIFT) + BK.g;
    grgb[2].b = (((vx * LCM.m[2][0]) + (vy * LCM.m[2][1]) + (vz * LCM.m[2][2])) >> SHIFT) + BK.b;

    v4->r = grgb[0].r = (v3->r * grgb[0].r) >> SHIFT;
    v4->g = grgb[0].g = (v3->g * grgb[0].g) >> SHIFT;
    v4->b = grgb[0].b = (v3->b * grgb[0].b) >> SHIFT;
    v5->r = grgb[1].r = (v3->r * grgb[1].r) >> SHIFT;
    v5->g = grgb[1].g = (v3->g * grgb[1].g) >> SHIFT;
    v5->b = grgb[1].b = (v3->b * grgb[1].b) >> SHIFT;
    v6->r = grgb[2].r = (v3->r * grgb[2].r) >> SHIFT;
    v6->g = grgb[2].g = (v3->g * grgb[2].g) >> SHIFT;
    v6->b = grgb[2].b = (v3->b * grgb[2].b) >> SHIFT;
}
#else
void NormalColorCol3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4, CVECTOR *v5, CVECTOR *v6)
{
    long xt, yt, zt;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;
    LLV[0].vx = ((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT;
    LLV[0].vy = ((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT;
    LLV[0].vz = ((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT;

    xt = v1->vx;
    yt = v1->vy;
    zt = v1->vz;
    LLV[1].vx = ((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT;
    LLV[1].vy = ((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT;
    LLV[1].vz = ((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT;

    xt = v2->vx;
    yt = v2->vy;
    zt = v2->vz;
    LLV[2].vx = ((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT;
    LLV[2].vy = ((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT;
    LLV[2].vz = ((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT;

    xt = LLV[0].vx;
    yt = LLV[0].vy;
    zt = LLV[0].vz;
    grgb[0].r = ((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT;
    grgb[0].g = ((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT;
    grgb[0].b = ((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT;
    grgb[0].r += BK.r;
    grgb[0].g += BK.g;
    grgb[0].b += BK.b;

    xt = LLV[1].vx;
    yt = LLV[1].vy;
    zt = LLV[1].vz;
    grgb[1].r = ((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT;
    grgb[1].g = ((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT;
    grgb[1].b = ((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT;
    grgb[1].r += BK.r;
    grgb[1].g += BK.g;
    grgb[1].b += BK.b;

    xt = LLV[2].vx;
    yt = LLV[2].vy;
    zt = LLV[2].vz;
    grgb[2].r = ((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT;
    grgb[2].g = ((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT;
    grgb[2].b = ((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT;
    grgb[2].r += BK.r;
    grgb[2].g += BK.g;
    grgb[2].b += BK.b;

    v4->r = grgb[0].r = (v3->r * grgb[0].r) >> SHIFT;
    v4->g = grgb[0].g = (v3->g * grgb[0].g) >> SHIFT;
    v4->b = grgb[0].b = (v3->b * grgb[0].b) >> SHIFT;
    v5->r = grgb[1].r = (v3->r * grgb[1].r) >> SHIFT;
    v5->g = grgb[1].g = (v3->g * grgb[1].g) >> SHIFT;
    v5->b = grgb[1].b = (v3->b * grgb[1].b) >> SHIFT;
    v6->r = grgb[2].r = (v3->r * grgb[2].r) >> SHIFT;
    v6->g = grgb[2].g = (v3->g * grgb[2].g) >> SHIFT;
    v6->b = grgb[2].b = (v3->b * grgb[2].b) >> SHIFT;
}
#endif
#endif

/* Three normal vectors receive matrix lighting and depth cueing. */
#ifdef EQUIV
void NormalColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, long p, CVECTOR *v4, CVECTOR *v5, CVECTOR *v6)
{
    long xt, yt, zt;
    long p2 = ONE - p;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;
    LLV[0].vx = ((zt * LLM.m[0][2]) + (yt * LLM.m[0][1]) + (xt * LLM.m[0][0])) >> SHIFT;
    LLV[0].vy = ((zt * LLM.m[1][2]) + (yt * LLM.m[1][1]) + (xt * LLM.m[1][0])) >> SHIFT;
    LLV[0].vz = ((zt * LLM.m[2][2]) + (yt * LLM.m[2][1]) + (xt * LLM.m[2][0])) >> SHIFT;

    xt = v1->vx;
    yt = v1->vy;
    zt = v1->vz;
    LLV[1].vx = ((zt * LLM.m[0][2]) + (yt * LLM.m[0][1]) + (xt * LLM.m[0][0])) >> SHIFT;
    LLV[1].vy = ((zt * LLM.m[1][2]) + (yt * LLM.m[1][1]) + (xt * LLM.m[1][0])) >> SHIFT;
    LLV[1].vz = ((zt * LLM.m[2][2]) + (yt * LLM.m[2][1]) + (xt * LLM.m[2][0])) >> SHIFT;

    xt = v2->vx;
    yt = v2->vy;
    zt = v2->vz;
    LLV[2].vx = ((zt * LLM.m[0][2]) + (yt * LLM.m[0][1]) + (xt * LLM.m[0][0])) >> SHIFT;
    LLV[2].vy = ((zt * LLM.m[1][2]) + (yt * LLM.m[1][1]) + (xt * LLM.m[1][0])) >> SHIFT;
    LLV[2].vz = ((zt * LLM.m[2][2]) + (yt * LLM.m[2][1]) + (xt * LLM.m[2][0])) >> SHIFT;

    xt = LLV[0].vx;
    yt = LLV[0].vy;
    zt = LLV[0].vz;
    grgb[0].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + BK.r;
    grgb[0].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + BK.g;
    grgb[0].b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + BK.b;

    xt = LLV[1].vx;
    yt = LLV[1].vy;
    zt = LLV[1].vz;
    grgb[1].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + BK.r;
    grgb[1].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + BK.g;
    grgb[1].b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + BK.b;

    xt = LLV[2].vx;
    yt = LLV[2].vy;
    zt = LLV[2].vz;
    grgb[2].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + BK.r;
    grgb[2].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + BK.g;
    grgb[2].b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + BK.b;

    v4->r = grgb[0].r = ((grgb[0].r * (p2 * v3->r)) + (p * FC.r)) >> SHIFT;
    v4->g = grgb[0].g = ((grgb[0].g * (p2 * v3->g)) + (p * FC.g)) >> SHIFT;
    v4->b = grgb[0].b = ((grgb[0].b * (p2 * v3->b)) + (p * FC.b)) >> SHIFT;
    v5->r = grgb[1].r = ((grgb[1].r * (p2 * v3->r)) + (p * FC.r)) >> SHIFT;
    v5->g = grgb[1].g = ((grgb[1].g * (p2 * v3->g)) + (p * FC.g)) >> SHIFT;
    v5->b = grgb[1].b = ((grgb[1].b * (p2 * v3->b)) + (p * FC.b)) >> SHIFT;
    v6->r = grgb[2].r = ((grgb[2].r * (p2 * v3->r)) + (p * FC.r)) >> SHIFT;
    v6->g = grgb[2].g = ((grgb[2].g * (p2 * v3->g)) + (p * FC.g)) >> SHIFT;
    v6->b = grgb[2].b = ((grgb[2].b * (p2 * v3->b)) + (p * FC.b)) >> SHIFT;
}
#else
void NormalColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, long p, CVECTOR *v4, CVECTOR *v5, CVECTOR *v6)
{
    long xt, yt, zt;
    long p2 = ONE - p;

    xt = v0->vx;
    yt = v0->vy;
    zt = v0->vz;
    LLV[0].vx = ((zt * LLM.m[0][2]) + (yt * LLM.m[0][1]) + (xt * LLM.m[0][0])) >> SHIFT;
    LLV[0].vy = ((zt * LLM.m[1][2]) + (yt * LLM.m[1][1]) + (xt * LLM.m[1][0])) >> SHIFT;
    LLV[0].vz = ((zt * LLM.m[2][2]) + (yt * LLM.m[2][1]) + (xt * LLM.m[2][0])) >> SHIFT;

    xt = v1->vx;
    yt = v1->vy;
    zt = v1->vz;
    LLV[1].vx = ((zt * LLM.m[0][2]) + (yt * LLM.m[0][1]) + (xt * LLM.m[0][0])) >> SHIFT;
    LLV[1].vy = ((zt * LLM.m[1][2]) + (yt * LLM.m[1][1]) + (xt * LLM.m[1][0])) >> SHIFT;
    LLV[1].vz = ((zt * LLM.m[2][2]) + (yt * LLM.m[2][1]) + (xt * LLM.m[2][0])) >> SHIFT;

    xt = v2->vx;
    yt = v2->vy;
    zt = v2->vz;
    LLV[2].vx = ((zt * LLM.m[0][2]) + (yt * LLM.m[0][1]) + (xt * LLM.m[0][0])) >> SHIFT;
    LLV[2].vy = ((zt * LLM.m[1][2]) + (yt * LLM.m[1][1]) + (xt * LLM.m[1][0])) >> SHIFT;
    LLV[2].vz = ((zt * LLM.m[2][2]) + (yt * LLM.m[2][1]) + (xt * LLM.m[2][0])) >> SHIFT;

    xt = LLV[0].vx;
    yt = LLV[0].vy;
    zt = LLV[0].vz;
    grgb[0].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + BK.r;
    grgb[0].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + BK.g;
    grgb[0].b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + BK.b;

    xt = LLV[1].vx;
    yt = LLV[1].vy;
    zt = LLV[1].vz;
    grgb[1].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + BK.r;
    grgb[1].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + BK.g;
    grgb[1].b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + BK.b;

    xt = LLV[2].vx;
    yt = LLV[2].vy;
    zt = LLV[2].vz;
    grgb[2].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + BK.r;
    grgb[2].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + BK.g;
    grgb[2].b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + BK.b;

    v4->r = grgb[0].r = ((grgb[0].r * (p2 * v3->r)) + (p * FC.r)) >> SHIFT;
    v4->g = grgb[0].g = ((grgb[0].g * (p2 * v3->g)) + (p * FC.g)) >> SHIFT;
    v4->b = grgb[0].b = ((grgb[0].b * (p2 * v3->b)) + (p * FC.b)) >> SHIFT;
    v5->r = grgb[1].r = ((grgb[1].r * (p2 * v3->r)) + (p * FC.r)) >> SHIFT;
    v5->g = grgb[1].g = ((grgb[1].g * (p2 * v3->g)) + (p * FC.g)) >> SHIFT;
    v5->b = grgb[1].b = ((grgb[1].b * (p2 * v3->b)) + (p * FC.b)) >> SHIFT;
    v6->r = grgb[2].r = ((grgb[2].r * (p2 * v3->r)) + (p * FC.r)) >> SHIFT;
    v6->g = grgb[2].g = ((grgb[2].g * (p2 * v3->g)) + (p * FC.g)) >> SHIFT;
    v6->b = grgb[2].b = ((grgb[2].b * (p2 * v3->b)) + (p * FC.b)) >> SHIFT;
}
#endif
#ifdef EQUIV
/* Equivalent under the file's /Oa contract (the exe keeps SCR2 live across the branch). */
#ifdef NETPLAY
long RotTransPers3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2, long *p, long *flag)
{
    long z;
    long xt0, yt0;
    long xt1, yt1;
    long xt2, yt2;
    VECTOR r0, r1, r2;

    *flag = 0;

    ApplyMatrix(&RTM, v0, &r0);
    ApplyMatrix(&RTM, v1, &r1);
    ApplyMatrix(&RTM, v2, &r2);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);

    return r2.vz >> 2;
}
#else
long RotTransPers3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2, long *p, long *flag)
{
    long z;
    long xt0, yt0;
    long xt1, yt1;
    long xt2, yt2;
    VECTOR r0, r1, r2;

    *flag = 0;

    ApplyMatrixInline(&RTM, v0, &r0);
    ApplyMatrixInline(&RTM, v1, &r1);
    ApplyMatrixInline(&RTM, v2, &r2);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);

    return r2.vz >> 2;
}
#endif
#endif

/* Built without /Oa: the exe reloads *sxy after the colour call. */
#pragma optimize("a", off)
#ifdef EQUIV
#ifdef NETPLAY
long RotAverageNclipColorCol3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4, SVECTOR *v5, CVECTOR *v6, long *sxy0, long *sxy1, long *sxy2, CVECTOR *v7, CVECTOR *v8, CVECTOR *v9, long *otz, long *flag)
{
    long yt0, xt0;
    long fogLevel;
    long yt1, xt1;
    long yt2, xt2;
    VECTOR rot0;
    VECTOR rot1;
    VECTOR rot2;
    long depthZ;

    *flag = 0;

    ApplyMatrix(&RTM, v0, &rot0);
    ApplyMatrix(&RTM, v1, &rot1);
    ApplyMatrix(&RTM, v2, &rot2);

    depthZ = rot0.vz;
    if (depthZ < 0)
        *flag |= 0x00040000;
    if (depthZ > 32768)
        *flag |= 0x00400000;
    if (depthZ < SCR2) {
        *flag |= 0x00020000;
        depthZ = SCR2;
    }
    if (depthZ > 65536)
        depthZ = 65536;
    if ((FNEAR > depthZ) || (FFAR < depthZ))
        *flag |= 0x00001000;
    if (rot0.vx > 32767) {
        *flag |= 0x81000000;
        rot0.vx = 32767;
    }
    if (rot0.vy > 32767) {
        *flag |= 0x80800000;
        rot0.vy = 32767;
    }
    if (rot0.vx < -32767) {
        *flag |= 0x81000000;
        rot0.vx = -32767;
    }
    if (rot0.vy < -32767) {
        *flag |= 0x80800000;
        rot0.vy = -32767;
    }
    xt0 = ((screenZ * rot0.vx) / depthZ) + xOffset;
    yt0 = ((screenZ * rot0.vy) / depthZ) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    depthZ = rot1.vz;
    if (depthZ > 32768)
        *flag |= 0x00400000;
    if (depthZ < 0)
        *flag |= 0x00040000;
    if (depthZ < SCR2) {
        *flag |= 0x00020000;
        depthZ = SCR2;
    }
    if (depthZ > 65536)
        depthZ = 65536;
    if ((FNEAR > depthZ) || (FFAR < depthZ))
        *flag |= 0x00001000;
    if (rot1.vx > 32767) {
        *flag |= 0x81000000;
        rot1.vx = 32767;
    }
    if (rot1.vy > 32767) {
        *flag |= 0x80800000;
        rot1.vy = 32767;
    }
    if (rot1.vx < -32767) {
        *flag |= 0x81000000;
        rot1.vx = -32767;
    }
    if (rot1.vy < -32767) {
        *flag |= 0x80800000;
        rot1.vy = -32767;
    }
    xt1 = ((screenZ * rot1.vx) / depthZ) + xOffset;
    yt1 = ((screenZ * rot1.vy) / depthZ) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    depthZ = rot2.vz;
    if (depthZ < 0)
        *flag |= 0x00040000;
    if (depthZ > 32768)
        *flag |= 0x00400000;
    if (depthZ < SCR2) {
        *flag |= 0x00020000;
        depthZ = SCR2;
    }
    if (depthZ > 65536)
        depthZ = 65536;
    if ((FNEAR > depthZ) || (FFAR < depthZ))
        *flag |= 0x00001000;
    if (rot2.vx > 32767) {
        *flag |= 0x01000000;
        rot2.vx = 32767;
    }
    if (rot2.vy > 32767) {
        *flag |= 0x00800000;
        rot2.vy = 32767;
    }
    if (rot2.vx < -32767) {
        *flag |= 0x81000000;
        rot2.vx = -32767;
    }
    if (rot2.vy < -32767) {
        *flag |= 0x80800000;
        rot2.vy = -32767;
    }
    xt2 = ((screenZ * rot2.vx) / depthZ) + xOffset;
    yt2 = ((screenZ * rot2.vy) / depthZ) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    *sxy0 = (xt0 & 0x0000FFFF) | (yt0 << 16);
    *sxy1 = (xt1 & 0x0000FFFF) | (yt1 << 16);
    *sxy2 = (xt2 & 0x0000FFFF) | (yt2 << 16);

    *otz = (rot0.vz + rot1.vz + rot2.vz) / 12;

    NormalColorCol3(v3, v4, v5, v6, v7, v8, v9);

    return NormalClip(*sxy0, *sxy1, *sxy2);
}
#else
long RotAverageNclipColorCol3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4, SVECTOR *v5, CVECTOR *v6, long *sxy0, long *sxy1, long *sxy2, CVECTOR *v7, CVECTOR *v8, CVECTOR *v9, long *otz, long *flag)
{
    long scr2;
    long z;
    long xt0, yt0;
    long xt1, yt1;
    long xt2, yt2;
    VECTOR r0;
    VECTOR r1;
    VECTOR r2;

    *flag = 0;

#ifdef NETPLAY
    ApplyMatrix(&RTM, v0, &r0);
    ApplyMatrix(&RTM, v1, &r1);
    ApplyMatrix(&RTM, v2, &r2);
#else
    ApplyMatrixInline(&RTM, v0, &r0);
    ApplyMatrixInline(&RTM, v1, &r1);
    ApplyMatrixInline(&RTM, v2, &r2);
#endif

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    scr2 = SCR2;
    if (z < scr2) {
        *flag |= 0x00020000;
        z = scr2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < scr2) {
        *flag |= 0x00020000;
        z = scr2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < scr2) {
        *flag |= 0x00020000;
        z = scr2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);

    *otz = (r0.vz + r1.vz + r2.vz) / 12;

    NormalColorCol3(v3, v4, v5, v6, v7, v8, v9);

    return NormalClip(*sxy0, *sxy1, *sxy2);
}
#endif
#else
long RotAverageNclipColorCol3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4, SVECTOR *v5, CVECTOR *v6, long *sxy0, long *sxy1, long *sxy2, CVECTOR *v7, CVECTOR *v8, CVECTOR *v9, long *otz, long *flag)
{
    long scr2;
    long z;
    long xt0, yt0;
    long xt1, yt1;
    long xt2, yt2;
    VECTOR r0;
    VECTOR r1;
    VECTOR r2;

    *flag = 0;

#ifdef NETPLAY
    ApplyMatrix(&RTM, v0, &r0);
    ApplyMatrix(&RTM, v1, &r1);
    ApplyMatrix(&RTM, v2, &r2);
#else
    ApplyMatrixInline(&RTM, v0, &r0);
    ApplyMatrixInline(&RTM, v1, &r1);
    ApplyMatrixInline(&RTM, v2, &r2);
#endif

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    scr2 = SCR2;
    if (z < scr2) {
        *flag |= 0x00020000;
        z = scr2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < scr2) {
        *flag |= 0x00020000;
        z = scr2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < scr2) {
        *flag |= 0x00020000;
        z = scr2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);

    *otz = (r0.vz + r1.vz + r2.vz) / 12;

    NormalColorCol3(v3, v4, v5, v6, v7, v8, v9);

    return NormalClip(*sxy0, *sxy1, *sxy2);
}
#endif
#pragma optimize("a", on)

#ifdef EQUIV
/* Built without /Oa: the exe reloads *sxy after the colour call. */
#pragma optimize("a", off)
#ifdef NETPLAY
long RotAverageNclipColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4, SVECTOR *v5, CVECTOR *v6, long *sxy0, long *sxy1, long *sxy2, CVECTOR *v7, CVECTOR *v8, CVECTOR *v9, long *otz, long *flag)
{
    long z;
    long xt1, yt1;
    long xt2, yt2;
    long xt0, yt0;
    VECTOR r0, r1, r2;
    long p;

    *flag = 0;

    ApplyMatrix(&RTM, v0, &r0);
    ApplyMatrix(&RTM, v1, &r1);
    ApplyMatrix(&RTM, v2, &r2);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);

    *otz = (r0.vz + r1.vz + r2.vz) / 12;

    if ((FNEAR < SCR2) && (z < SCR2))
        p = lnFOG(SCR2);
    else
        p = lnFOG(z);

    NormalColorDpq3(v3, v4, v5, v6, p, v7, v8, v9);

    return NormalClip(*sxy0, *sxy1, *sxy2);
}
#else
long RotAverageNclipColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4, SVECTOR *v5, CVECTOR *v6, long *sxy0, long *sxy1, long *sxy2, CVECTOR *v7, CVECTOR *v8, CVECTOR *v9, long *otz, long *flag)
{
    long z;
    long xt1, yt1;
    long xt2, yt2;
    long xt0, yt0;
    VECTOR r0, r1, r2;
    long p;

    *flag = 0;

    ApplyMatrixInline(&RTM, v0, &r0);
    ApplyMatrixInline(&RTM, v1, &r1);
    ApplyMatrixInline(&RTM, v2, &r2);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);

    *otz = (r0.vz + r1.vz + r2.vz) / 12;

    if ((FNEAR < SCR2) && (z < SCR2))
        p = lnFOG(SCR2);
    else
        p = lnFOG(z);

    NormalColorDpq3(v3, v4, v5, v6, p, v7, v8, v9);

    return NormalClip(*sxy0, *sxy1, *sxy2);
}
#endif
#pragma optimize("a", on)
#endif

#ifdef EQUIV
#ifdef NETPLAY
long RotColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4, SVECTOR *v5, CVECTOR *v6, long *sxy0, long *sxy1, long *sxy2, CVECTOR *v7, CVECTOR *v8, CVECTOR *v9, long *flag)
{
    long z;
    long p;
    long xt0, yt0;
    long xt1, yt1;
    long xt2, yt2;
    VECTOR r0, r1, r2;

    *flag = 0;

    ApplyMatrix(&RTM, v0, &r0);
    ApplyMatrix(&RTM, v1, &r1);
    ApplyMatrix(&RTM, v2, &r2);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);

    if ((FNEAR < SCR2) && (z < SCR2))
        p = lnFOG(SCR2);
    else
        p = lnFOG(z);

    NormalColorDpq3(v3, v4, v5, v6, p, v7, v8, v9);

    return ((r0.vz + r1.vz + r2.vz) / 12) >> 2;
}
#endif
#endif
#ifndef NETPLAY
long RotColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4, SVECTOR *v5, CVECTOR *v6, long *sxy0, long *sxy1, long *sxy2, CVECTOR *v7, CVECTOR *v8, CVECTOR *v9, long *flag)
{
    long z;
    long p;
    long xt0, yt0;
    long xt1, yt1;
    long xt2, yt2;
    VECTOR r0, r1, r2;

    *flag = 0;

    ApplyMatrixInline(&RTM, v0, &r0);
    ApplyMatrixInline(&RTM, v1, &r1);
    ApplyMatrixInline(&RTM, v2, &r2);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);

    if ((FNEAR < SCR2) && (z < SCR2))
        p = lnFOG(SCR2);
    else
        p = lnFOG(z);

    NormalColorDpq3(v3, v4, v5, v6, p, v7, v8, v9);

    return ((r0.vz + r1.vz + r2.vz) / 12) >> 2;
}
#endif

#ifdef EQUIV
#ifdef NETPLAY
long RotColorDpq(SVECTOR *v0, SVECTOR *v1, CVECTOR *v2, long *sxy, CVECTOR *v3, long *flag)
{
    long p, p2, xt;
    long rotZ;
    VECTOR ztoff;
    long zclip;
    VECTOR vtmp2;
    long ys;
    VECTOR pos;

    *flag = 0;

    ApplyMatrix(&RTM, v0, &pos);

    zclip = pos.vz;
    if (zclip < 0)
        *flag |= 0x00040000;
    if (zclip > 32768)
        *flag |= 0x00400000;
    if (zclip < SCR2) {
        *flag |= 0x00020000;
        zclip = SCR2;
    }
    if (zclip > 65536)
        zclip = 65536;
    if ((FNEAR > zclip) || (FFAR < zclip))
        *flag |= 0x00001000;
    if (pos.vx > 32767) {
        *flag |= 0x81000000;
        pos.vx = 32767;
    }
    if (pos.vy > 32767) {
        *flag |= 0x80800000;
        pos.vy = 32767;
    }
    if (pos.vx < -32767) {
        *flag |= 0x81000000;
        pos.vx = -32767;
    }
    if (pos.vy < -32767) {
        *flag |= 0x80800000;
        pos.vy = -32767;
    }
    xt = ((screenZ * pos.vx) / zclip) + xOffset;
    ys = ((screenZ * pos.vy) / zclip) + yOffset;
    if (xt < -1024) {
        *flag |= 0x80004000;
        xt = -1024;
    }
    if (xt > 1023) {
        *flag |= 0x80004000;
        xt = 1023;
    }
    if (ys < -1024) {
        *flag |= 0x80002000;
        ys = -1024;
    }
    if (ys > 1023) {
        *flag |= 0x80002000;
        ys = 1023;
    }

    *sxy = (xt & 0x0000FFFF) | (ys << 16);

    if ((FNEAR < SCR2) && (zclip < SCR2))
        p = lnFOG(SCR2);
    else
        p = lnFOG(zclip);

    p2 = ONE - p;

    xt = v1->vx;
    ys = v1->vy;
    rotZ = v1->vz;
    LLV[2].vx = ((xt * LLM.m[0][0]) + (ys * LLM.m[0][1]) + (rotZ * LLM.m[0][2])) >> SHIFT;
    LLV[2].vy = ((xt * LLM.m[1][0]) + (ys * LLM.m[1][1]) + (rotZ * LLM.m[1][2])) >> SHIFT;
    LLV[2].vz = ((xt * LLM.m[2][0]) + (ys * LLM.m[2][1]) + (rotZ * LLM.m[2][2])) >> SHIFT;

    xt = LLV[2].vx;
    ys = LLV[2].vy;
    rotZ = LLV[2].vz;
    grgb[2].r = (((xt * LCM.m[0][0]) + (ys * LCM.m[0][1]) + (rotZ * LCM.m[0][2])) >> SHIFT) + BK.r;
    grgb[2].g = (((xt * LCM.m[1][0]) + (ys * LCM.m[1][1]) + (rotZ * LCM.m[1][2])) >> SHIFT) + BK.g;
    grgb[2].b = (((xt * LCM.m[2][0]) + (ys * LCM.m[2][1]) + (rotZ * LCM.m[2][2])) >> SHIFT) + BK.b;

    v3->r = grgb[2].r = ((p2 * v2->r * grgb[2].r) + (p * FC.r)) >> SHIFT2;
    v3->g = grgb[2].g = ((p2 * v2->g * grgb[2].g) + (p * FC.g)) >> SHIFT2;
    v3->b = grgb[2].b = ((p2 * v2->b * grgb[2].b) + (p * FC.b)) >> SHIFT2;

    return pos.vz >> 2;
}
#else
long RotColorDpq(SVECTOR *v0, SVECTOR *v1, CVECTOR *v2, long *sxy, CVECTOR *v3, long *flag)
{
    long xt, yt, zt;
    VECTOR r0;
    long z, p, p2;
    long scr2;

    *flag = 0;

    ApplyMatrixInline(&RTM, v0, &r0);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    scr2 = SCR2;
    if (z < scr2) {
        *flag |= 0x00020000;
        z = scr2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt = ((screenZ * r0.vx) / z) + xOffset;
    yt = ((screenZ * r0.vy) / z) + yOffset;
    if (xt < -1024) {
        *flag |= 0x80004000;
        xt = -1024;
    }
    if (xt > 1023) {
        *flag |= 0x80004000;
        xt = 1023;
    }
    if (yt < -1024) {
        *flag |= 0x80002000;
        yt = -1024;
    }
    if (yt > 1023) {
        *flag |= 0x80002000;
        yt = 1023;
    }

    *sxy = (yt << 16) | (xt & 0x0000FFFF);

    if ((FNEAR < scr2) && (z < scr2))
        p = lnFOG(scr2);
    else
        p = lnFOG(z);

    p2 = ONE - p;

    xt = v1->vx;
    yt = v1->vy;
    zt = v1->vz;
    LLV[2].vx = ((xt * LLM.m[0][0]) + (yt * LLM.m[0][1]) + (zt * LLM.m[0][2])) >> SHIFT;
    LLV[2].vy = ((xt * LLM.m[1][0]) + (yt * LLM.m[1][1]) + (zt * LLM.m[1][2])) >> SHIFT;
    LLV[2].vz = ((xt * LLM.m[2][0]) + (yt * LLM.m[2][1]) + (zt * LLM.m[2][2])) >> SHIFT;

    xt = LLV[2].vx;
    yt = LLV[2].vy;
    zt = LLV[2].vz;
    grgb[2].r = ((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT;
    grgb[2].g = ((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT;
    grgb[2].b = ((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT;
    grgb[2].r += BK.r;
    grgb[2].g += BK.g;
    grgb[2].b += BK.b;

    v3->r = grgb[2].r = ((p2 * v2->r * grgb[2].r) + (p * FC.r)) >> SHIFT2;
    v3->g = grgb[2].g = ((p2 * v2->g * grgb[2].g) + (p * FC.g)) >> SHIFT2;
    v3->b = grgb[2].b = ((p2 * v2->b * grgb[2].b) + (p * FC.b)) >> SHIFT2;

    return r0.vz >> 2;
}
#endif
#endif

#ifdef EQUIV
#ifdef NETPLAY
long RotColorMatDpq(SVECTOR *v0, SVECTOR *v1, CVECTOR *v2, long *sxy, CVECTOR *v3, long matc, long *flag)
{
    long p, p2, xt;
    long rotZ;
    VECTOR ztoff;
    long zclip;
    VECTOR vtmp2;
    long ys;
    VECTOR pos;

    *flag = 0;

    ApplyMatrix(&RTM, v0, &pos);

    zclip = pos.vz;
    if (zclip < 0)
        *flag |= 0x00040000;
    if (zclip > 32768)
        *flag |= 0x00400000;
    if (zclip < SCR2) {
        *flag |= 0x00020000;
        zclip = SCR2;
    }
    if (zclip > 65536)
        zclip = 65536;
    if ((FNEAR > zclip) || (FFAR < zclip))
        *flag |= 0x00001000;
    if (pos.vx > 32767) {
        *flag |= 0x81000000;
        pos.vx = 32767;
    }
    if (pos.vy > 32767) {
        *flag |= 0x80800000;
        pos.vy = 32767;
    }
    if (pos.vx < -32767) {
        *flag |= 0x81000000;
        pos.vx = -32767;
    }
    if (pos.vy < -32767) {
        *flag |= 0x80800000;
        pos.vy = -32767;
    }
    xt = ((screenZ * pos.vx) / zclip) + xOffset;
    ys = ((screenZ * pos.vy) / zclip) + yOffset;
    if (xt < -1024) {
        *flag |= 0x80004000;
        xt = -1024;
    }
    if (xt > 1023) {
        *flag |= 0x80004000;
        xt = 1023;
    }
    if (ys < -1024) {
        *flag |= 0x80002000;
        ys = -1024;
    }
    if (ys > 1023) {
        *flag |= 0x80002000;
        ys = 1023;
    }

    *sxy = (xt & 0x0000FFFF) | (ys << 16);

    if ((FNEAR < SCR2) && (zclip < SCR2))
        p = lnFOG(SCR2);
    else
        p = lnFOG(zclip);

    p2 = ONE - p;

    xt = v1->vx;
    ys = v1->vy;
    rotZ = v1->vz;

    LLV[2].vx = ((rotZ * LLM.m[0][2]) + (ys * LLM.m[0][1]) + (xt * LLM.m[0][0])) >> SHIFT;
    LLV[2].vy = ((rotZ * LLM.m[1][2]) + (xt * LLM.m[1][0]) + (ys * LLM.m[1][1])) >> SHIFT;
    LLV[2].vz = ((rotZ * LLM.m[2][2]) + (xt * LLM.m[2][0]) + (ys * LLM.m[2][1])) >> SHIFT;

    LLV[2].vx = pow(LLV[2].vx, (pow(2, matc)));
    LLV[2].vy = pow(LLV[2].vy, (pow(2, matc)));
    LLV[2].vz = pow(LLV[2].vz, (pow(2, matc)));

    xt = (long)(LLV[2].vx);
    ys = (long)(LLV[2].vy);
    rotZ = (long)(LLV[2].vz);

    grgb[2].r = (((xt * LCM.m[0][0]) + (ys * LCM.m[0][1]) + (rotZ * LCM.m[0][2])) >> SHIFT) + (BK.r);
    grgb[2].g = (((xt * LCM.m[1][0]) + (ys * LCM.m[1][1]) + (rotZ * LCM.m[1][2])) >> SHIFT) + (BK.g);
    grgb[2].b = (((xt * LCM.m[2][0]) + (ys * LCM.m[2][1]) + (rotZ * LCM.m[2][2])) >> SHIFT) + (BK.b);

    v3->r = grgb[2].r = ((p2 * v2->r * grgb[2].r) + (p * FC.r)) >> SHIFT2;
    v3->g = grgb[2].g = ((p2 * v2->g * grgb[2].g) + (p * FC.g)) >> SHIFT2;
    v3->b = grgb[2].b = ((p2 * v2->b * grgb[2].b) + (p * FC.b)) >> SHIFT2;

    return pos.vz >> 2;
}
#else
long RotColorMatDpq(SVECTOR *v0, SVECTOR *v1, CVECTOR *v2, long *sxy, CVECTOR *v3, long matc, long *flag)
{
    long z;
    long p;
    long scr2;
    long xt0, yt0;
    long xt, yt, zt;
    long p2;
    VECTOR r0;

    *flag = 0;

    ApplyMatrixInline(&RTM, v0, &r0);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    scr2 = SCR2;
    if (z < scr2) {
        *flag |= 0x00020000;
        z = scr2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    *sxy = (yt0 << 16) | (xt0 & 0x0000FFFF);

    if ((FNEAR < scr2) && (z < scr2))
        p = lnFOG(scr2);
    else
        p = lnFOG(z);

    p2 = ONE - p;

    xt = v1->vx;
    yt = v1->vy;
    zt = v1->vz;

    LLV[2].vx = ((zt * LLM.m[0][2]) + (yt * LLM.m[0][1]) + (xt * LLM.m[0][0])) >> SHIFT;
    LLV[2].vy = ((zt * LLM.m[1][2]) + (xt * LLM.m[1][0]) + (yt * LLM.m[1][1])) >> SHIFT;
    LLV[2].vz = ((zt * LLM.m[2][2]) + (xt * LLM.m[2][0]) + (yt * LLM.m[2][1])) >> SHIFT;

    LLV[2].vx = pow(LLV[2].vx, (pow(2, matc)));
    LLV[2].vy = pow(LLV[2].vy, (pow(2, matc)));
    LLV[2].vz = pow(LLV[2].vz, (pow(2, matc)));

    xt = (long)(LLV[2].vx);
    yt = (long)(LLV[2].vy);
    zt = (long)(LLV[2].vz);

    grgb[2].r = (((xt * LCM.m[0][0]) + (yt * LCM.m[0][1]) + (zt * LCM.m[0][2])) >> SHIFT) + (BK.r);
    grgb[2].g = (((xt * LCM.m[1][0]) + (yt * LCM.m[1][1]) + (zt * LCM.m[1][2])) >> SHIFT) + (BK.g);
    grgb[2].b = (((xt * LCM.m[2][0]) + (yt * LCM.m[2][1]) + (zt * LCM.m[2][2])) >> SHIFT) + (BK.b);

    v3->r = grgb[2].r = ((p2 * v2->r * grgb[2].r) + (p * FC.r)) >> SHIFT2;
    v3->g = grgb[2].g = ((p2 * v2->g * grgb[2].g) + (p * FC.g)) >> SHIFT2;
    v3->b = grgb[2].b = ((p2 * v2->b * grgb[2].b) + (p * FC.b)) >> SHIFT2;

    return r0.vz >> 2;
}
#endif
#endif

#ifdef EQUIV
#ifdef NETPLAY
long RotAverage3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2, long *p, long *flag)
{
    long z, ave;
    VECTOR r0, r1, r2;
    long xt0, yt0;
    long xt1, yt1;
    long xt2, yt2;

    *flag = 0;

    ApplyMatrix(&RTM, v0, &r0);
    ApplyMatrix(&RTM, v1, &r1);
    ApplyMatrix(&RTM, v2, &r2);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);

    ave = (r0.vz + r1.vz + r2.vz) / 12;

    if ((FNEAR < SCR2) && (z < SCR2))
        *p = lnFOG(SCR2);
    else
        *p = lnFOG(z);

    return ave;
}
#else
long RotAverage3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2, long *p, long *flag)
{
    long z, ave;
    VECTOR r0, r1, r2;
    long xt0, yt0;
    long xt1, yt1;
    long xt2, yt2;

    *flag = 0;

#ifdef NETPLAY
    ApplyMatrix(&RTM, v0, &r0);
    ApplyMatrix(&RTM, v1, &r1);
    ApplyMatrix(&RTM, v2, &r2);
#else
    ApplyMatrixInline(&RTM, v0, &r0);
    ApplyMatrixInline(&RTM, v1, &r1);
    ApplyMatrixInline(&RTM, v2, &r2);
#endif

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);

    ave = (r0.vz + r1.vz + r2.vz) / 12;

    if ((FNEAR < SCR2) && (z < SCR2))
        *p = lnFOG(SCR2);
    else
        *p = lnFOG(z);

    return ave;
}
#endif
#endif
#ifdef EQUIV
#ifdef NETPLAY
long RotAverage4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, long *sxy0, long *sxy1, long *sxy2, long *sxy3, long *p, long *flag)
{
    long z, ave;
    long xt0, yt0;
    long xt1, yt1;
    long yt2, xt2;
    long xt3, yt3;
    VECTOR r0, r1, r2, r3;

#ifdef NETPLAY
    ApplyMatrix(&RTM, v0, &r0);
    ApplyMatrix(&RTM, v1, &r1);
    ApplyMatrix(&RTM, v2, &r2);
    ApplyMatrix(&RTM, v3, &r3);
#else
    ApplyMatrixInline(&RTM, v0, &r0);
    ApplyMatrixInline(&RTM, v1, &r1);
    ApplyMatrixInline(&RTM, v2, &r2);
    ApplyMatrixInline(&RTM, v3, &r3);
#endif

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x81000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x80800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    z = r3.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r3.vx > 32767) {
        *flag |= 0x81000000;
        r3.vx = 32767;
    }
    if (r3.vy > 32767) {
        *flag |= 0x80800000;
        r3.vy = 32767;
    }
    if (r3.vx < -32767) {
        *flag |= 0x81000000;
        r3.vx = -32767;
    }
    if (r3.vy < -32767) {
        *flag |= 0x80800000;
        r3.vy = -32767;
    }
    xt3 = ((screenZ * r3.vx) / z) + xOffset;
    yt3 = ((screenZ * r3.vy) / z) + yOffset;
    if (xt3 < -1024) {
        *flag |= 0x80004000;
        xt3 = -1024;
    }
    if (xt3 > 1023) {
        *flag |= 0x80004000;
        xt3 = 1023;
    }
    if (yt3 < -1024) {
        *flag |= 0x80002000;
        yt3 = -1024;
    }
    if (yt3 > 1023) {
        *flag |= 0x80002000;
        yt3 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);
    *sxy3 = (yt3 << 16) | (xt3 & 0x0000FFFF);

    ave = (r0.vz + r1.vz + r2.vz + r3.vz) >> 4;

    if ((FNEAR < SCR2) && (z < SCR2))
        *p = lnFOG(SCR2);
    else
        *p = lnFOG(z);

    return ave;
}
#else
long RotAverage4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, long *sxy0, long *sxy1, long *sxy2, long *sxy3, long *p, long *flag)
{
    long z, ave;
    long xt0, yt0;
    long xt1, yt1;
    long yt2, xt2;
    long xt3, yt3;
    VECTOR r0, r1, r2, r3;

    ApplyMatrixInline(&RTM, v0, &r0);
    ApplyMatrixInline(&RTM, v1, &r1);
    ApplyMatrixInline(&RTM, v2, &r2);
    ApplyMatrixInline(&RTM, v3, &r3);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x81000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x80800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    z = r3.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r3.vx > 32767) {
        *flag |= 0x81000000;
        r3.vx = 32767;
    }
    if (r3.vy > 32767) {
        *flag |= 0x80800000;
        r3.vy = 32767;
    }
    if (r3.vx < -32767) {
        *flag |= 0x81000000;
        r3.vx = -32767;
    }
    if (r3.vy < -32767) {
        *flag |= 0x80800000;
        r3.vy = -32767;
    }
    xt3 = ((screenZ * r3.vx) / z) + xOffset;
    yt3 = ((screenZ * r3.vy) / z) + yOffset;
    if (xt3 < -1024) {
        *flag |= 0x80004000;
        xt3 = -1024;
    }
    if (xt3 > 1023) {
        *flag |= 0x80004000;
        xt3 = 1023;
    }
    if (yt3 < -1024) {
        *flag |= 0x80002000;
        yt3 = -1024;
    }
    if (yt3 > 1023) {
        *flag |= 0x80002000;
        yt3 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);
    *sxy3 = (yt3 << 16) | (xt3 & 0x0000FFFF);

    ave = (r0.vz + r1.vz + r2.vz + r3.vz) >> 4;

    if ((FNEAR < SCR2) && (z < SCR2))
        *p = lnFOG(SCR2);
    else
        *p = lnFOG(z);

    return ave;
}
#endif

#endif
#ifdef EQUIV
#ifdef NETPLAY
long RotAverageNclip3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2, long *p, long *otz, long *flag)
{
    long z;
    long xt0, yt0;
    long xt1, yt1;
    long xt2, yt2;
    VECTOR r0, r1, r2;

    *flag = 0;

    ApplyMatrix(&RTM, v0, &r0);
    ApplyMatrix(&RTM, v1, &r1);
    ApplyMatrix(&RTM, v2, &r2);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);

    *otz = (r0.vz + r1.vz + r2.vz) / 12;

    if ((FNEAR < SCR2) && (z < SCR2))
        *p = lnFOG(SCR2);
    else
        *p = lnFOG(z);

    return NormalClip(*sxy0, *sxy1, *sxy2);
}
#else
long RotAverageNclip3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2, long *p, long *otz, long *flag)
{
    long z;
    long xt0, yt0;
    long xt1, yt1;
    long xt2, yt2;
    VECTOR r0, r1, r2;

    *flag = 0;

    ApplyMatrixInline(&RTM, v0, &r0);
    ApplyMatrixInline(&RTM, v1, &r1);
    ApplyMatrixInline(&RTM, v2, &r2);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);

    *otz = (r0.vz + r1.vz + r2.vz) / 12;

    if ((FNEAR < SCR2) && (z < SCR2))
        *p = lnFOG(SCR2);
    else
        *p = lnFOG(z);

    return NormalClip(*sxy0, *sxy1, *sxy2);
}
#endif
#endif
#ifdef EQUIV
/* Same computation as the exe; it loads the depth values on the other side of the *sxy3 store. */
#ifdef NETPLAY
long RotAverageNclip4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, long *sxy0, long *sxy1, long *sxy2, long *sxy3, long *p, long *otz, long *flag)
{
    long xt0;
    long yt0;
    /* Never read: the NET build keeps this 4-byte local in the frame ([ebp-4]). */
    long fogLevel;
    long xt1;
    long yt1;
    long xt2;
    long yt2;
    VECTOR rot0;
    long xt3;
    long yt3;
    VECTOR rot1;
    VECTOR rot2;
    VECTOR rot3;
    long depthZ;

    *flag = 0;

    ApplyMatrix(&RTM, v0, &rot0);
    ApplyMatrix(&RTM, v1, &rot1);
    ApplyMatrix(&RTM, v2, &rot2);
    ApplyMatrix(&RTM, v3, &rot3);

    depthZ = rot0.vz;
    if (depthZ < 0)
        *flag |= 0x00040000;
    if (depthZ > 32768)
        *flag |= 0x00400000;
    if (depthZ < SCR2) {
        *flag |= 0x00020000;
        depthZ = SCR2;
    }
    if (depthZ > 65536)
        depthZ = 65536;
    if ((FNEAR > depthZ) || (FFAR < depthZ))
        *flag |= 0x00001000;
    if (rot0.vx > 32767) {
        *flag |= 0x81000000;
        rot0.vx = 32767;
    }
    if (rot0.vy > 32767) {
        *flag |= 0x80800000;
        rot0.vy = 32767;
    }
    if (rot0.vx < -32767) {
        *flag |= 0x81000000;
        rot0.vx = -32767;
    }
    if (rot0.vy < -32767) {
        *flag |= 0x80800000;
        rot0.vy = -32767;
    }
    xt0 = ((screenZ * rot0.vx) / depthZ) + xOffset;
    yt0 = ((screenZ * rot0.vy) / depthZ) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    depthZ = rot1.vz;
    if (depthZ > 32768)
        *flag |= 0x00400000;
    if (depthZ < 0)
        *flag |= 0x00040000;
    if (depthZ < SCR2) {
        *flag |= 0x00020000;
        depthZ = SCR2;
    }
    if (depthZ > 65536)
        depthZ = 65536;
    if ((FNEAR > depthZ) || (FFAR < depthZ))
        *flag |= 0x00001000;
    if (rot1.vx > 32767) {
        *flag |= 0x81000000;
        rot1.vx = 32767;
    }
    if (rot1.vy > 32767) {
        *flag |= 0x80800000;
        rot1.vy = 32767;
    }
    if (rot1.vx < -32767) {
        *flag |= 0x81000000;
        rot1.vx = -32767;
    }
    if (rot1.vy < -32767) {
        *flag |= 0x80800000;
        rot1.vy = -32767;
    }
    xt1 = ((screenZ * rot1.vx) / depthZ) + xOffset;
    yt1 = ((screenZ * rot1.vy) / depthZ) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    depthZ = rot2.vz;
    if (depthZ < 0)
        *flag |= 0x00040000;
    if (depthZ > 32768)
        *flag |= 0x00400000;
    if (depthZ < SCR2) {
        *flag |= 0x00020000;
        depthZ = SCR2;
    }
    if (depthZ > 65536)
        depthZ = 65536;
    if ((FNEAR > depthZ) || (FFAR < depthZ))
        *flag |= 0x00001000;
    if (rot2.vx > 32767) {
        *flag |= 0x01000000;
        rot2.vx = 32767;
    }
    if (rot2.vy > 32767) {
        *flag |= 0x00800000;
        rot2.vy = 32767;
    }
    if (rot2.vx < -32767) {
        *flag |= 0x81000000;
        rot2.vx = -32767;
    }
    if (rot2.vy < -32767) {
        *flag |= 0x80800000;
        rot2.vy = -32767;
    }
    xt2 = ((screenZ * rot2.vx) / depthZ) + xOffset;
    yt2 = ((screenZ * rot2.vy) / depthZ) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    depthZ = rot3.vz;
    if (depthZ < 0)
        *flag |= 0x00040000;
    if (depthZ > 32768)
        *flag |= 0x00400000;
    if (depthZ < SCR2) {
        *flag |= 0x00020000;
        depthZ = SCR2;
    }
    if (depthZ > 65536)
        depthZ = 65536;
    if ((FNEAR > depthZ) || (FFAR < depthZ))
        *flag |= 0x00001000;
    if (rot3.vx > 32767) {
        *flag |= 0x81000000;
        rot3.vx = 32767;
    }
    if (rot3.vy > 32767) {
        *flag |= 0x80800000;
        rot3.vy = 32767;
    }
    if (rot3.vx < -32767) {
        *flag |= 0x81000000;
        rot3.vx = -32767;
    }
    if (rot3.vy < -32767) {
        *flag |= 0x80800000;
        rot3.vy = -32767;
    }
    xt3 = ((screenZ * rot3.vx) / depthZ) + xOffset;
    yt3 = ((screenZ * rot3.vy) / depthZ) + yOffset;
    if (xt3 < -1024) {
        *flag |= 0x80004000;
        xt3 = -1024;
    }
    if (xt3 > 1023) {
        *flag |= 0x80004000;
        xt3 = 1023;
    }
    if (yt3 < -1024) {
        *flag |= 0x80002000;
        yt3 = -1024;
    }
    if (yt3 > 1023) {
        *flag |= 0x80002000;
        yt3 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);
    *sxy3 = (yt3 << 16) | (xt3 & 0x0000FFFF);

    *otz = (rot0.vz + rot1.vz + rot2.vz + rot3.vz) >> 4;

    if ((FNEAR < SCR2) && (depthZ < SCR2))
        *p = lnFOG(SCR2);
    else
        *p = lnFOG(depthZ);

    return NormalClip(*sxy0, *sxy1, *sxy2);
}
#else
long RotAverageNclip4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, long *sxy0, long *sxy1, long *sxy2, long *sxy3, long *p, long *otz, long *flag)
{
    long z;
    long xt0, yt0;
    long xt1, yt1;
    long xt2, yt2;
    long xt3, yt3;
    /* Keep the transformed vectors in the original stack order. */
    VECTOR r3, r0, r1, r2;

    *flag = 0;

    ApplyMatrixInline(&RTM, v0, &r0);
    ApplyMatrixInline(&RTM, v1, &r1);
    ApplyMatrixInline(&RTM, v2, &r2);
    ApplyMatrixInline(&RTM, v3, &r3);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    z = r3.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r3.vx > 32767) {
        *flag |= 0x81000000;
        r3.vx = 32767;
    }
    if (r3.vy > 32767) {
        *flag |= 0x80800000;
        r3.vy = 32767;
    }
    if (r3.vx < -32767) {
        *flag |= 0x81000000;
        r3.vx = -32767;
    }
    if (r3.vy < -32767) {
        *flag |= 0x80800000;
        r3.vy = -32767;
    }
    xt3 = ((screenZ * r3.vx) / z) + xOffset;
    yt3 = ((screenZ * r3.vy) / z) + yOffset;
    if (xt3 < -1024) {
        *flag |= 0x80004000;
        xt3 = -1024;
    }
    if (xt3 > 1023) {
        *flag |= 0x80004000;
        xt3 = 1023;
    }
    if (yt3 < -1024) {
        *flag |= 0x80002000;
        yt3 = -1024;
    }
    if (yt3 > 1023) {
        *flag |= 0x80002000;
        yt3 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);
    *sxy3 = (yt3 << 16) | (xt3 & 0x0000FFFF);

    *otz = (r0.vz + r1.vz + r2.vz + r3.vz) >> 4;

    if ((FNEAR < SCR2) && (z < SCR2))
        *p = lnFOG(SCR2);
    else
        *p = lnFOG(z);

    return NormalClip(*sxy0, *sxy1, *sxy2);
}
#endif
#endif

#ifdef EQUIV
#ifdef NETPLAY
long RotTransPers4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, long *sxy0, long *sxy1, long *sxy2, long *sxy3, long *p, long *flag)
{
    long z;
    long xt0, yt0;
    long xt1, yt1;
    long xt2, yt2;
    long xt3, yt3;
    VECTOR r0, r1, r2, r3;

    *flag = 0;

    ApplyMatrix(&RTM, v0, &r0);
    ApplyMatrix(&RTM, v1, &r1);
    ApplyMatrix(&RTM, v2, &r2);
    ApplyMatrix(&RTM, v3, &r3);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    z = r3.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r3.vx > 32767) {
        *flag |= 0x81000000;
        r3.vx = 32767;
    }
    if (r3.vy > 32767) {
        *flag |= 0x80800000;
        r3.vy = 32767;
    }
    if (r3.vx < -32767) {
        *flag |= 0x81000000;
        r3.vx = -32767;
    }
    if (r3.vy < -32767) {
        *flag |= 0x80800000;
        r3.vy = -32767;
    }
    xt3 = ((screenZ * r3.vx) / z) + xOffset;
    yt3 = ((screenZ * r3.vy) / z) + yOffset;
    if (xt3 < -1024) {
        *flag |= 0x80004000;
        xt3 = -1024;
    }
    if (xt3 > 1023) {
        *flag |= 0x80004000;
        xt3 = 1023;
    }
    if (yt3 < -1024) {
        *flag |= 0x80002000;
        yt3 = -1024;
    }
    if (yt3 > 1023) {
        *flag |= 0x80002000;
        yt3 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);
    *sxy3 = (yt3 << 16) | (xt3 & 0x0000FFFF);

    if ((FNEAR < SCR2) && (z < SCR2))
        *p = lnFOG(SCR2);
    else
        *p = lnFOG(z);

    return r3.vz >> 2;
}
#else
long RotTransPers4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, long *sxy0, long *sxy1, long *sxy2, long *sxy3, long *p, long *flag)
{
    long z;
    long xt0, yt0;
    long xt1, yt1;
    long xt2, yt2;
    long xt3, yt3;
    VECTOR r0, r1, r2, r3;

    *flag = 0;

    ApplyMatrixInline(&RTM, v0, &r0);
    ApplyMatrixInline(&RTM, v1, &r1);
    ApplyMatrixInline(&RTM, v2, &r2);
    ApplyMatrixInline(&RTM, v3, &r3);

    z = r0.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r0.vx > 32767) {
        *flag |= 0x81000000;
        r0.vx = 32767;
    }
    if (r0.vy > 32767) {
        *flag |= 0x80800000;
        r0.vy = 32767;
    }
    if (r0.vx < -32767) {
        *flag |= 0x81000000;
        r0.vx = -32767;
    }
    if (r0.vy < -32767) {
        *flag |= 0x80800000;
        r0.vy = -32767;
    }
    xt0 = ((screenZ * r0.vx) / z) + xOffset;
    yt0 = ((screenZ * r0.vy) / z) + yOffset;
    if (xt0 < -1024) {
        *flag |= 0x80004000;
        xt0 = -1024;
    }
    if (xt0 > 1023) {
        *flag |= 0x80004000;
        xt0 = 1023;
    }
    if (yt0 < -1024) {
        *flag |= 0x80002000;
        yt0 = -1024;
    }
    if (yt0 > 1023) {
        *flag |= 0x80002000;
        yt0 = 1023;
    }

    z = r1.vz;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < 0)
        *flag |= 0x00040000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r1.vx > 32767) {
        *flag |= 0x81000000;
        r1.vx = 32767;
    }
    if (r1.vy > 32767) {
        *flag |= 0x80800000;
        r1.vy = 32767;
    }
    if (r1.vx < -32767) {
        *flag |= 0x81000000;
        r1.vx = -32767;
    }
    if (r1.vy < -32767) {
        *flag |= 0x80800000;
        r1.vy = -32767;
    }
    xt1 = ((screenZ * r1.vx) / z) + xOffset;
    yt1 = ((screenZ * r1.vy) / z) + yOffset;
    if (xt1 < -1024) {
        *flag |= 0x80004000;
        xt1 = -1024;
    }
    if (xt1 > 1023) {
        *flag |= 0x80004000;
        xt1 = 1023;
    }
    if (yt1 < -1024) {
        *flag |= 0x80002000;
        yt1 = -1024;
    }
    if (yt1 > 1023) {
        *flag |= 0x80002000;
        yt1 = 1023;
    }

    z = r2.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r2.vx > 32767) {
        *flag |= 0x01000000;
        r2.vx = 32767;
    }
    if (r2.vy > 32767) {
        *flag |= 0x00800000;
        r2.vy = 32767;
    }
    if (r2.vx < -32767) {
        *flag |= 0x81000000;
        r2.vx = -32767;
    }
    if (r2.vy < -32767) {
        *flag |= 0x80800000;
        r2.vy = -32767;
    }
    xt2 = ((screenZ * r2.vx) / z) + xOffset;
    yt2 = ((screenZ * r2.vy) / z) + yOffset;
    if (xt2 < -1024) {
        *flag |= 0x80004000;
        xt2 = -1024;
    }
    if (xt2 > 1023) {
        *flag |= 0x80004000;
        xt2 = 1023;
    }
    if (yt2 < -1024) {
        *flag |= 0x80002000;
        yt2 = -1024;
    }
    if (yt2 > 1023) {
        *flag |= 0x80002000;
        yt2 = 1023;
    }

    z = r3.vz;
    if (z < 0)
        *flag |= 0x00040000;
    if (z > 32768)
        *flag |= 0x00400000;
    if (z < SCR2) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 65536)
        z = 65536;
    if ((FNEAR > z) || (FFAR < z))
        *flag |= 0x00001000;
    if (r3.vx > 32767) {
        *flag |= 0x81000000;
        r3.vx = 32767;
    }
    if (r3.vy > 32767) {
        *flag |= 0x80800000;
        r3.vy = 32767;
    }
    if (r3.vx < -32767) {
        *flag |= 0x81000000;
        r3.vx = -32767;
    }
    if (r3.vy < -32767) {
        *flag |= 0x80800000;
        r3.vy = -32767;
    }
    xt3 = ((screenZ * r3.vx) / z) + xOffset;
    yt3 = ((screenZ * r3.vy) / z) + yOffset;
    if (xt3 < -1024) {
        *flag |= 0x80004000;
        xt3 = -1024;
    }
    if (xt3 > 1023) {
        *flag |= 0x80004000;
        xt3 = 1023;
    }
    if (yt3 < -1024) {
        *flag |= 0x80002000;
        yt3 = -1024;
    }
    if (yt3 > 1023) {
        *flag |= 0x80002000;
        yt3 = 1023;
    }

    *sxy0 = (yt0 << 16) | (xt0 & 0x0000FFFF);
    *sxy1 = (yt1 << 16) | (xt1 & 0x0000FFFF);
    *sxy2 = (yt2 << 16) | (xt2 & 0x0000FFFF);
    *sxy3 = (yt3 << 16) | (xt3 & 0x0000FFFF);

    if ((FNEAR < SCR2) && (z < SCR2))
        *p = lnFOG(SCR2);
    else
        *p = lnFOG(z);

    return r3.vz >> 2;
}
#endif
#endif


#define ROT_MULT(a, b) (((a) * (b)) >> SHIFT)

#ifdef NETPLAY
/* Configuration: compute the rotation sin/cos inline in double precision (NET build). */
#define NET_INLINE_TRIG 1
#endif

/* Sine and cosine of a 4096-per-turn angle in double precision; the originals
   were inlined into RotMatrix only. */
static long slow_rsin(long a)
{
    return (long)(sin(((double)a / ONE_FLOAT) * 6.28318530718) * ONE_FLOAT);
}

static long slow_rcos(long a)
{
    return (long)(cos(((double)a / ONE_FLOAT) * 6.28318530718) * ONE_FLOAT);
}

MATRIX *RotMatrix(SVECTOR *r, MATRIX *m)
{
    long sineX, sineY, cosX, sineZ, cosY, cosZ;

#ifdef NETPLAY
    /* Each sin/cos is a constant-condition if with an empty else: the inline
       trig path is the one the NET build keeps. */
    if (NET_INLINE_TRIG) {
        sineX = (long)(sin(((double)r->vx / ONE_FLOAT) * 6.28318530718) * ONE_FLOAT);
    } else {
    }
    if (NET_INLINE_TRIG) {
        cosX = (long)(cos(((double)r->vx / ONE_FLOAT) * 6.28318530718) * ONE_FLOAT);
    } else {
    }
    if (NET_INLINE_TRIG) {
        sineY = (long)(sin(((double)r->vy / ONE_FLOAT) * 6.28318530718) * ONE_FLOAT);
    } else {
    }
    if (NET_INLINE_TRIG) {
        cosY = (long)(cos(((double)r->vy / ONE_FLOAT) * 6.28318530718) * ONE_FLOAT);
    } else {
    }
    if (NET_INLINE_TRIG) {
        sineZ = (long)(sin(((double)r->vz / ONE_FLOAT) * 6.28318530718) * ONE_FLOAT);
    } else {
    }
    if (NET_INLINE_TRIG) {
        cosZ = (long)(cos(((double)r->vz / ONE_FLOAT) * 6.28318530718) * ONE_FLOAT);
    } else {
    }
#else
    sineX = slow_rsin(r->vx);
    cosX = slow_rcos(r->vx);
    sineY = slow_rsin(r->vy);
    cosY = slow_rcos(r->vy);
    sineZ = slow_rsin(r->vz);
    cosZ = slow_rcos(r->vz);
#endif

    m->m[0][0] = ROT_MULT(cosZ, cosY);
    m->m[0][1] = -ROT_MULT(sineZ, cosY);
    m->m[0][2] = sineY;
    m->m[1][0] = ((sineZ * cosX) + (ROT_MULT(sineY, sineX) * cosZ)) >> SHIFT;
    m->m[1][1] = ((cosX * cosZ) - (ROT_MULT(sineY, sineX) * sineZ)) >> SHIFT;
    m->m[1][2] = -ROT_MULT(cosY, sineX);
    m->m[2][0] = ((sineZ * sineX) - (ROT_MULT(cosX, cosZ) * sineY)) >> SHIFT;
    m->m[2][1] = ((sineX * cosZ) + (ROT_MULT(cosX, sineZ) * sineY)) >> SHIFT;
    m->m[2][2] = ROT_MULT(cosY, cosX);

    return m;
}

long RotAverageNclip3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2, long *p, long *otz, long *flag);
long RotAverageNclip4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, long *sxy0, long *sxy1, long *sxy2, long *sxy3, long *p, long *otz, long *flag);

/* The original RotMatrixC simply forwards to RotMatrix. */
MATRIX *RotMatrixC(SVECTOR *r, MATRIX *m)
{
    return RotMatrix(r, m);
}

long RotNClip3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2, long *p, long *otz, long *flag)
{
    long result;

    result = RotAverageNclip3(v0, v1, v2, sxy0, sxy1, sxy2, p, otz, flag);
    return result;
}

long RotNClip4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, long *sxy0, long *sxy1, long *sxy2, long *sxy3, long *p, long *otz, long *flag)
{
    long result;

    result = RotAverageNclip4(v0, v1, v2, v3, sxy0, sxy1, sxy2, sxy3, p, otz, flag);
    return result;
}

#ifndef NETPLAY
void RotTrans(SVECTOR *v0, VECTOR *v1, long *flag)
{
    register long z;

#ifdef NETPLAY
    ApplyMatrix(&RTM, v0, v1);
#else
    ApplyMatrixInline(&RTM, v0, v1);
#endif
    z = v1->vz;
    if (z < 0) {
        *flag |= 0x00040000;
    }
    if (z > 0x8000) {
        *flag |= 0x00400000;
    }
    if (SCR2 > z) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 0x10000) {
        z = 0x10000;
    }
    if ((FNEAR > z) || (FFAR < z)) {
        *flag |= 0x00001000;
    }
    if (v1->vy > 0x7fff) {
        v1->vy = 0x7fff;
        *flag |= 0x80800000;
    }
    if (v1->vy < -0x7fff) {
        v1->vy = -0x7fff;
        *flag |= 0x80800000;
    }
}
#endif
#ifdef NETPLAY
void RotTrans(SVECTOR *v0, VECTOR *v1, long *flag)
{
    register long z;

    ApplyMatrix(&RTM, v0, v1);
    z = v1->vz;
    if (z < 0) {
        *flag |= 0x00040000;
    }
    if (z > 0x8000) {
        *flag |= 0x00400000;
    }
    if (SCR2 > z) {
        *flag |= 0x00020000;
        z = SCR2;
    }
    if (z > 0x10000) {
        z = 0x10000;
    }
    if ((FNEAR > z) || (FFAR < z)) {
        *flag |= 0x00001000;
    }
    if (v1->vy > 0x7fff) {
        *flag |= 0x80800000;
        v1->vy = 0x7fff;
    }
    if (v1->vy < -0x7fff) {
        *flag |= 0x80800000;
        v1->vy = -0x7fff;
    }
}
#endif

long RotTransPersForN(SVECTOR *v0, long *sxy, long *p, long *flag)
{
    VECTOR r0;

#ifdef NETPLAY
    ApplyMatrix(&RTM, v0, &r0);
    RotTransPersV(sxy, &r0, flag);
#else
    ApplyMatrixInline(&RTM, v0, &r0);
    PerspectiveInline(sxy, &r0, flag);
#endif
    if (FNEAR > r0.vz) {
        *p = 0;
    } else if (FFAR < r0.vz) {
        *p = 4095;
    } else {
        *p = (long)(((r0.vz - FNEAR) * 4095.0) / (float)(FFAR - FNEAR));
    }
    return r0.vz;
}

#ifndef NETPLAY
void RotTransPersN(SVECTOR *v0, DVECTOR *v1, u_short *sz, u_short *flag, long n)
{
    int loop;
    SVECTOR *ip;
    DVECTOR *op;
#ifdef NETPLAY
    long depthProjection;
#else
    VECTOR r0;
#endif
    long long_flag;

    long_flag = 0;
    ip = v0;
    op = v1;
#ifdef NETPLAY
    depthProjection = 0;
#endif
    for (loop = 0; loop < n; loop++) {
#ifdef NETPLAY
        *sz++ = (u_short)RotTransPersForN(ip++, (long *)op, &depthProjection, &long_flag);
#else
        ApplyMatrixInline(&RTM, ip++, &r0);
        PerspectiveInline((long *)op, &r0, &long_flag);
        *sz++ = (u_short)r0.vz;
#endif
        op++;
        *flag++ = (u_short)(long_flag >> 12);
    }
}
#endif
#ifdef NETPLAY
void RotTransPersN(SVECTOR *v0, DVECTOR *v1, u_short *sz, u_short *flag, long n)
{
    long flags;
    SVECTOR *input;
    long p;
    DVECTOR *op;
    int loop;

    flags = 0;
    input = v0;
    op = v1;
    p = 0;
    for (loop = 0; loop < n; loop++) {
        *sz++ = (u_short)RotTransPersForN(input++, (long *)op, &p, &flags);
        *flag++ = (u_short)(flags >> 12);
        op++;
    }
}
#endif

long RotTransPers(SVECTOR *v0, long *sxy, long *p, long *flag)
{
    VECTOR r0;

#ifdef NETPLAY
    ApplyMatrix(&RTM, v0, &r0);
    RotTransPersV(sxy, &r0, flag);
#else
    ApplyMatrixInline(&RTM, v0, &r0);
    PerspectiveInline(sxy, &r0, flag);
#endif
    if (FNEAR > r0.vz) {
        *p = 0;
    } else if (FFAR < r0.vz) {
        *p = 4095;
    } else {
        *p = (long)(((r0.vz - FNEAR) * 4095.0) / (float)(FFAR - FNEAR));
    }
    return r0.vz >> 2;
}

void SetBackColor(long rbk, long gbk, long bbk)
{
    BK.r = rbk;
    BK.g = gbk;
    BK.b = bbk;
}

void SetColorMatrix(MATRIX *m)
{
    LCM = *m;
}

void SetFarColor(long rfc, long gfc, long bfc)
{
    FC.r = rfc;
    FC.g = gfc;
    FC.b = bfc;
}

void SetFogFar(long a, long h)
{
    FFAR = a;
    FNEAR = a / 5;
}

void SetFogNear(long a, long h)
{
#ifdef NETPLAY
    FNEAR = a;
    FFAR = a * 5;
#else
    FFAR = a * 5;
    FNEAR = a;
#endif
}

void SetGeomOffset(long ofx, long ofy)
{
    xOffset = ofx;
    yOffset = ofy;
}

void SetGeomScreen(long h)
{
    screenZ = h;
    SCR2 = h >> 1;
}

void SetLightMatrix(MATRIX *m)
{
    LLM = *m;
}

#ifdef NETPLAY
#pragma intrinsic(memcpy)
#endif
void SetRotMatrix(MATRIX *m)
{
    memcpy(&(RTM.m), &(m->m), sizeof(short) * 9);
}

void SetTransMatrix(MATRIX *m)
{
    memcpy(&(RTM.t), &(m->t), sizeof(long) * 3);
}

VECTOR *Square0(VECTOR *v0, VECTOR *v1)
{
    v1->vx = (v0->vx * v0->vx);
    v1->vy = (v0->vy * v0->vy);
    v1->vz = (v0->vz * v0->vz);

    return v1;
}

VECTOR *Square12(VECTOR *v0, VECTOR *v1)
{
    v1->vx = (v0->vx * v0->vx) >> SHIFT;
    v1->vy = (v0->vy * v0->vy) >> SHIFT;
    v1->vz = (v0->vz * v0->vz) >> SHIFT;

    return v1;
}

long SquareRoot0(long a)
{
    unsigned long radicand;

    radicand = (unsigned long)a;
    return sqrt(radicand);
}

long SquareRoot12(long a)
{
    unsigned long radicand;

    radicand = (unsigned long)a;
    return (long)(sqrt((double)radicand / ONE_FLOAT) * ONE_FLOAT);
}

#ifdef NETPLAY
/* Empty, as in the NET original. */
void SubPol3(void *p, void *sp, int ndiv)
{
}

/* Empty, as in the NET original. */
void SubPol4(void *p, void *sp, int ndiv)
{
}
#else
/* Empty, as in the D3D original. */
void SubPol3(void *p, void *sp, int ndiv)
{
}

/* Empty, as in the D3D original. */
void SubPol4(void *p, void *sp, int ndiv)
{
}
#endif

MATRIX *TransMatrix(MATRIX *m, VECTOR *v)
{
    m->t[0] = v->vx;
    m->t[1] = v->vy;
    m->t[2] = v->vz;

    return m;
}

void VectorNormal(VECTOR *v0, VECTOR *v1)
{
    long xSquare, ySquare, zSquare;
#ifdef NETPLAY
    float length;
#else
    double length;
#endif

    xSquare = v0->vx * v0->vx;
    ySquare = v0->vy * v0->vy;
    zSquare = v0->vz * v0->vz;

    length = sqrt(xSquare + ySquare + zSquare) / ONE_FLOAT;

    v1->vx = v0->vx / length;
    v1->vy = v0->vy / length;
    v1->vz = v0->vz / length;
}
