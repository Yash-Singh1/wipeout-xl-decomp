#include "wipeout.h"
#include "utils.h"

#include <math.h>
#ifdef NETPLAY
#pragma intrinsic(sqrt)
#endif

extern short arccos_tab[];
#ifdef NETPLAY
extern int randomSeed;
extern int localRandomSeed;
extern u_char localRandomOn;
#else
extern int attractShip;
#endif

/* Returns the fixed-angle difference between two vectors. */
int GetAng(VECTOR vec1, VECTOR vec2)
{
#ifdef NETPLAY
    int angle;
    int dotProduct;
    int magnitude;

    dotProduct = ScalarProduct(vec1, vec2);
    magnitude = (GetMagnitude((float)vec2.vx, (float)vec2.vy, (float)vec2.vz) *
                 GetMagnitude((float)vec1.vx, (float)vec1.vy, (float)vec1.vz)) >> 12;
    if (magnitude == 0)
        dotProduct = 4095;
    else
        dotProduct = dotProduct / magnitude;
    if (dotProduct > 4095)
        dotProduct = 4095;
    else if (dotProduct < -4095)
        dotProduct = -4095;
    angle = arccos(dotProduct);
    return angle;
#else
    int product;
    int mag;
#ifdef POWERVR
    int firstMagnitude;
    int secondMagnitude;
#endif

    product = vec2.vx * vec1.vx + vec2.vy * vec1.vy + vec2.vz * vec1.vz;
#ifdef POWERVR
    /* Preserve the squared-length addition order and convert vec1 first. */
    firstMagnitude = GetMagnitude((float)vec1.vz, (float)vec1.vy, (float)vec1.vx);
    secondMagnitude = GetMagnitude((float)vec2.vz, (float)vec2.vy, (float)vec2.vx);
    mag = (firstMagnitude * secondMagnitude) >> 12;
#else
    mag = (GetMagnitude((float)vec2.vx, (float)vec2.vz, (float)vec2.vy) *
           GetMagnitude((float)vec1.vx, (float)vec1.vz, (float)vec1.vy)) >> 12;
#endif
    if (mag == 0)
        product = 4095;
    else
        product = product / mag;
    if (product > 4095)
        product = 4095;
    else if (product < -4095)
        product = -4095;
    return arccos(product);
#endif
}



/* Returns the dot product of two vectors as an integer. */
int scalar_product(const VECTOR *a, const VECTOR *b)
{
    int result;
    result = a->vx * b->vx + (a->vy * b->vy + a->vz * b->vz);
    return result;
}


/* Returns the dot product of two vectors. */
int ScalarProduct(VECTOR v, VECTOR w)
{
#ifdef NETPLAY
    int product;

    product = v.vx * w.vx + (v.vy * w.vy + v.vz * w.vz);
    return product;
#else
    int xy;
    int z;

    xy = v.vx * w.vx + v.vy * w.vy;
    z = v.vz * w.vz;
    return xy + z;
#endif
}

/* Stores the cross product of two vectors. */
void CrossProduct(VECTOR *out, VECTOR a, VECTOR b)
{
    out->vx = (a.vy * b.vz - a.vz * b.vy) >> 9;
    out->vy = (a.vz * b.vx - a.vx * b.vz) >> 9;
    out->vz = (a.vx * b.vy - a.vy * b.vx) >> 9;
}


/* Stores the difference between two vectors. */
void MinusVector(VECTOR v1, VECTOR v2, VECTOR *result)
{
    result->vx = v2.vx - v1.vx;
    result->vy = v2.vy - v1.vy;
    result->vz = v2.vz - v1.vz;
}

/* Returns the length of a vector from its three coordinates. */
int GetMagnitude(float x, float y, float z)
{
#ifdef NETPLAY
    float sum;

    sum = (z * z + x * x) + y * y;
    sum = sqrt(sum);
    return (int)sum;
#else
    float sum;

#ifdef POWERVR
    sum = y * y;
    sum += x * x;
    sum += z * z;
#else
    sum = z * z;
    sum += y * y;
    sum += x * x;
#endif
    return sqrt(sum);
#endif
}


/* Converts a cosine value to the game's fixed-angle scale. */
int arccos(int value)
{
    int angle;
    int absValue;

    if (value < 0)
        absValue = -value;
    else
        absValue = value;
    angle = arccos_tab[absValue];
    if (value < 0)
        angle = 8192 + (8192 - angle);
    return angle;
}

/* Performs the game's signed right shift operation. */
int sar(int a, int n)
{
    if (a > 0)
        return a >> n;
    else {
        a = (-a) >> n;
        return -a;
    }
}

/* Wraps an angle into the game's full-turn range. */
int ang(int angle)
{
    angle &= 4095;
    if (angle > 2048)
        return angle - 4096;
    return angle;
}

/* Copies a vector into the destination. */
void assign(VECTOR *out, VECTOR v)
{
    out->vx = v.vx;
    out->vy = v.vy;
    out->vz = v.vz;
}

#ifdef NETPLAY
/* Selects the local (unsynchronised) random sequence for newRand while on. */
void SetLocalRandom(u_char enable)
{
    if (enable)
        localRandomOn = 1;
    else
        localRandomOn = 0;
}
#endif

/* Returns the next value from the game's random number sequence. */
int newRand(void)
{
#ifdef NETPLAY
    int seed;

    if (!localRandomOn) {
        seed = (randomSeed >> 1) ^ (randomSeed * 8);
        randomSeed = seed;
        return randomSeed & 0x7fff;
    }
    return (localRandomSeed * 501) & 0x7fff;
#else
    int seed;

    seed = (attractShip >> 1) ^ (attractShip * 8);
    attractShip = seed;
    return seed & 0x7fff;
#endif
}
