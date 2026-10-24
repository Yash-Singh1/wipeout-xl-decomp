#ifndef PC_MESH_H
#define PC_MESH_H

#include "wipeout.h"

struct D3DTRIANGLE;

/* LoadPrmCommon allocates Object.normals as 23 count/pointer groups, one per texture set.
   Group 14 holds the shield's saved set or a start-droid triangle group. */
#define COPIED_NORMAL_GROUP 14

/* Normal sets of a PC object: frame[i] holds the count and the vectors or triangles of texture set i. */
typedef struct PCObjectMesh {
    struct {
        long count;
        union {
            SVECTOR *data;
            struct D3DTRIANGLE *triangles;
        };
    } frame[23];
} PCObjectMesh;

/* Highest active normal set in the current render frame. */
extern unsigned short normalSetNo;

#endif
