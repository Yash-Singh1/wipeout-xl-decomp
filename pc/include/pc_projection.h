#ifndef PC_PROJECTION_H
#define PC_PROJECTION_H

#include "wipeout.h"
#include "d3d.h"

/* Translation entries m[3][0] and m[3][1] of viewMatrixFlipped. */
extern D3DMATRIX viewMatrixFlipped;
extern D3DMATRIX projMatrix;
/* Camera shake offsets, set by ShakeScreen (effects.c) and added to projected x and y in
   trails.c. */
extern float cameraShakeX, cameraShakeY;
void ConvertMatrix(float *out, MATRIX *matrix);

void RotTransPersN(SVECTOR *vertices, DVECTOR *screen, unsigned short *depth,
                   unsigned short *flags, long count);

#endif
