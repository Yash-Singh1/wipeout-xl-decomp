#ifndef PC_PARTICLES_H
#define PC_PARTICLES_H

#include "wipeout.h"
#include "pc_vertex.h"
#include "d3d.h"
#include "render.h"

/* One material's triangle batch in the PC particle renderer. */
typedef TrackBucket ParticleTriangleBuffer;

typedef TriList ParticleTriangleList;

#define trailIndexLists quadBuckets
extern PCVertex trailStage[];
extern unsigned short normalSetNo;
extern unsigned char particleAlpha;

void AddRenderSkeleton(Skeleton *skeleton, PCVertex *vertices, int vertexCount,
                       ParticleTriangleList *triangles, void *billboards, int billboardCount, int mode);

#endif
