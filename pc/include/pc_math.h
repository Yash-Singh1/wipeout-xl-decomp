#ifndef PC_MATH_H
#define PC_MATH_H

#include <math.h>

/* PC equivalents of the fixed-point trigonometry helpers. */
static __inline long rsin(long angle)
{
    return (long)(sin(((double)angle / 4096.0) * 6.28318530718) * 4096.0);
}
static __inline long rcos(long angle)
{
    return (long)(cos(((double)angle / 4096.0) * 6.28318530718) * 4096.0);
}
static __inline long SquareRoot0(long value)
{
    return (long)sqrt((double)value);
}

#endif
