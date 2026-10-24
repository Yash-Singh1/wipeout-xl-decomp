#ifndef PC_ENVMAP_H
#define PC_ENVMAP_H

#include "wipeout.h"

/* Per-vertex scratch for environment mapping: screen position, depth and flags from RotTransPersN,
   rotated normals (StaticRVert), and the 6-bit U and V lookup coordinates (envMapU, envMapV). */
extern DVECTOR StaticScreen[];
extern unsigned short StaticDepth[], StaticFlags[];
extern VECTOR StaticRVert[];
extern signed char envMapU[], envMapV[];

#endif
