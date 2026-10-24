#include "functions.h"

#ifdef NON_MATCHING
/* C's double conversion stores an x87 temporary that the original keeps on the x87 stack. */
void VectorMagnitude(VECTOR *vector)
{
    int length;

    length = (int)_ftol(sqrt((double)((vector->vx >> 6) * (vector->vx >> 6)
        + (vector->vy >> 6) * (vector->vy >> 6)
        + (vector->vz >> 6) * (vector->vz >> 6))));
    if (length == 0)
        length = 1;
    vector->vx = (vector->vx * 64) / length;
    vector->vy = (vector->vy * 64) / length;
    vector->vz = (vector->vz * 64) / length;
}
#endif

#ifdef EQUIV
void MultiplyScaledMatrix(MATRIX *transform, MATRIX *source, MATRIX *result)
{
    result->t[0] = ((source->m[0][0] * transform->t[0]
        + source->m[0][1] * transform->t[1]
        + source->m[0][2] * transform->t[2]) >> 12) + source->t[0];
    result->t[1] = ((source->m[1][0] * transform->t[0]
        + source->m[1][1] * transform->t[1]
        + source->m[1][2] * transform->t[2]) >> 12) + source->t[1];
    result->t[2] = ((source->m[2][0] * transform->t[0]
        + source->m[2][1] * transform->t[1]
        + source->m[2][2] * transform->t[2]) >> 12) + source->t[2];
    MulMatrix0(transform, source, result);
}
#endif
