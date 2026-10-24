#ifndef MATHINLINE_H
#define MATHINLINE_H

/* Some game files expand the libgte trigonometry inline instead of calling it. */

#include <math.h>

#undef rsin
#undef rcos
#undef SquareRoot0
#define rsin(a) ((long)(sin(((a) / 4096.0) * 6.28318530718) * 4096.0))
#define rcos(a) ((long)(cos(((a) / 4096.0) * 6.28318530718) * 4096.0))
#define SquareRoot0(a) ((long)sqrt(a))

#endif
