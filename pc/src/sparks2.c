#include "wipeout.h"
void SetSkeletonTransform(Skeleton *skeleton);
#include "sparks.h"
typedef struct SParticle SParticle;

#include "camera.h"
#include "particle_palette.h"

u_char FindNearestColourIndex(const struct tagPALETTEENTRY *colours, short count);
#ifdef NETPLAY
extern float D_b3c540;
#endif

void InitParticleColours(void)
{
    CVECTOR colour;

    if (rgbTextures != 0) {
#ifdef NETPLAY
        particleOrangeColour = 0;
        particleDarkGreyColour = 0;
        D_b3c540 = 0;
#else
        particleWhiteColour = 0;
        particleOrangeColour = 0;
        particleDarkGreyColour = 0;
#endif
        particleBlueColour = 0;
        particleGreyColour = 0;
        return;
    }
    colour.r = 255;
    colour.g = 255;
    colour.b = 255;
#ifdef NETPLAY
    particleOrangeColour = (float)FindNearestColourIndex(&colour, 1) / 255.0f;
#else
    particleWhiteColour = (float)FindNearestColourIndex(&colour, 1) / 255.0f;
#endif
    colour.r = 255;
    colour.g = 63;
    colour.b = 0;
#ifdef NETPLAY
    particleDarkGreyColour = (float)FindNearestColourIndex(&colour, 1) / 255.0f;
#else
    particleOrangeColour = (float)FindNearestColourIndex(&colour, 1) / 255.0f;
#endif
    colour.r = 48;
    colour.g = 48;
    colour.b = 48;
#ifdef NETPLAY
    D_b3c540 = (float)FindNearestColourIndex(&colour, 1) / 255.0f;
#else
    particleDarkGreyColour = (float)FindNearestColourIndex(&colour, 1) / 255.0f;
#endif
    colour.r = 63;
    colour.g = 63;
    colour.b = 255;
    particleBlueColour = (float)FindNearestColourIndex(&colour, 1) / 255.0f;
    colour.r = 96;
    colour.g = 96;
    colour.b = 96;
    particleGreyColour = (float)FindNearestColourIndex(&colour, 1) / 255.0f;
}

#ifdef EQUIV
#include "pc_vertex.h"
#include "d3d.h"
#include <string.h>

#include "pc_particles.h"

extern const float particleCoordinateScale;
extern const float particleThinWidth;
extern const float particleWideWidth;
extern const float particleSnowWidth;


#ifdef EQUIV
#ifndef NETPLAY
/* Same computation; the exe keeps the world X and Z in the other two registers. */
void AsmDrawParticles(SParticle *particles, Skeleton *camera, int unused)
{
    VECTOR position;
    PCVertex *vertices;
    ParticleTriangleList *triangleLists;
    char *cursor;
    int vertexCount;
    int material;
    int i;
    int count;
    float x, y, z;

    SetSkeletonTransform(camera->super);
    position.vx = cameraPtr->camPos->relative.t[0];
    position.vy = cameraPtr->camPos->relative.t[1];
    position.vz = cameraPtr->camPos->relative.t[2];
    vertexCount = 0;
    for (i = 0; i <= normalSetNo; i++)
        trailIndexLists[i].count = 0;
    if (rgbTextures)
        material = normalSetNo;
    else
        material = 11;
    for (i = 0; i < PARTICLE_COUNT; i++) {
        if (particles[i].life > 0) {
            int worldX = particles[i].pos.vx + position.vx;
            int worldY = particles[i].pos.vy + position.vy;
            int worldZ = particles[i].pos.vz + position.vz;

            if (rgbTextures) {
                trailStage[vertexCount].v = 0;
                trailStage[vertexCount + 1].v = 0;
                trailStage[vertexCount + 2].v = 0;
            } else {
                trailStage[vertexCount].v = 110.0f / 255.0f;
                trailStage[vertexCount + 1].v = 110.0f / 255.0f;
                trailStage[vertexCount + 2].v = 110.0f / 255.0f;
            }
            trailStage[vertexCount].a = particleAlpha;
            trailStage[vertexCount + 1].a = particleAlpha;
            trailStage[vertexCount + 2].a = particleAlpha;
            trailStage[vertexCount].x = particles[i].cameraPos.vx * particleCoordinateScale;
            trailStage[vertexCount].y = particles[i].cameraPos.vy * particleCoordinateScale;
            trailStage[vertexCount].z = particles[i].cameraPos.vz * particleCoordinateScale;
            particles[i].cameraPos.vx = worldX;
            particles[i].cameraPos.vy = worldY;
            particles[i].cameraPos.vz = worldZ;
            if (particles[i].flags) {
                x = worldX * particleCoordinateScale;
                trailStage[vertexCount + 1].x = x;
                trailStage[vertexCount + 2].x = x;
                y = worldY * particleCoordinateScale;
                trailStage[vertexCount + 1].y = y;
                trailStage[vertexCount + 2].y = y;
                z = worldZ * particleCoordinateScale;
                trailStage[vertexCount + 1].z = z;
                trailStage[vertexCount + 2].z = z;
                switch (particles[i].type) {
                case 1:
                    if (rgbTextures) {
                        trailStage[vertexCount].r = 127;
                        trailStage[vertexCount].g = 127;
                        trailStage[vertexCount].b = 127;
                    } else {
                        trailStage[vertexCount].r = 255;
                        trailStage[vertexCount].g = 255;
                        trailStage[vertexCount].b = 255;
                    }
                    trailStage[vertexCount + 1].r = 255;
                    trailStage[vertexCount + 1].g = 255;
                    trailStage[vertexCount + 1].b = 255;
                    trailStage[vertexCount + 2].r = 255;
                    trailStage[vertexCount + 2].g = 255;
                    trailStage[vertexCount + 2].b = 255;
                    trailStage[vertexCount + 2].y = y + particleThinWidth;
                    trailStage[vertexCount].u = particleWhiteColour;
                    trailStage[vertexCount + 1].u = particleWhiteColour;
                    trailStage[vertexCount + 2].u = particleWhiteColour;
                    break;
                case 2:
                    if (rgbTextures) {
                        trailStage[vertexCount].r = 63;
                        trailStage[vertexCount].g = 0;
                    } else {
                        trailStage[vertexCount].r = 255;
                        trailStage[vertexCount].g = 63;
                    }
                    trailStage[vertexCount].b = 0;
                    trailStage[vertexCount + 1].r = 255;
                    trailStage[vertexCount + 2].r = 255;
                    trailStage[vertexCount + 1].g = 63;
                    trailStage[vertexCount + 2].g = 63;
                    trailStage[vertexCount + 1].b = 0;
                    trailStage[vertexCount + 2].b = 0;
                    trailStage[vertexCount + 2].x = x + particleWideWidth;
                    trailStage[vertexCount].u = particleOrangeColour;
                    trailStage[vertexCount + 1].u = particleOrangeColour;
                    trailStage[vertexCount + 2].u = particleOrangeColour;
                    break;
                case 3:
                    if (rgbTextures) {
                        trailStage[vertexCount].r = 12;
                        trailStage[vertexCount].g = 12;
                        trailStage[vertexCount].b = 12;
                    } else {
                        trailStage[vertexCount].r = 48;
                        trailStage[vertexCount].g = 48;
                        trailStage[vertexCount].b = 48;
                    }
                    trailStage[vertexCount + 1].r = 48;
                    trailStage[vertexCount + 1].g = 48;
                    trailStage[vertexCount + 1].b = 48;
                    trailStage[vertexCount + 2].r = 48;
                    trailStage[vertexCount + 2].g = 48;
                    trailStage[vertexCount + 2].b = 48;
                    trailStage[vertexCount + 2].x = x + particleThinWidth;
                    trailStage[vertexCount].u = particleDarkGreyColour;
                    trailStage[vertexCount + 1].u = particleDarkGreyColour;
                    trailStage[vertexCount + 2].u = particleDarkGreyColour;
                    break;
                case 4:
                    if (rgbTextures) {
                        trailStage[vertexCount].r = 0;
                        trailStage[vertexCount].g = 0;
                        trailStage[vertexCount].b = 63;
                    } else {
                        trailStage[vertexCount].r = 63;
                        trailStage[vertexCount].g = 63;
                        trailStage[vertexCount].b = 255;
                    }
                    trailStage[vertexCount + 1].r = 63;
                    trailStage[vertexCount + 2].r = 63;
                    trailStage[vertexCount + 1].g = 63;
                    trailStage[vertexCount + 2].g = 63;
                    trailStage[vertexCount + 1].b = 255;
                    trailStage[vertexCount + 2].b = 255;
                    trailStage[vertexCount + 2].y = y + particleThinWidth;
                    trailStage[vertexCount].u = particleBlueColour;
                    trailStage[vertexCount + 1].u = particleBlueColour;
                    trailStage[vertexCount + 2].u = particleBlueColour;
                    break;
                case 5:
                    trailStage[vertexCount].r = 96;
                    trailStage[vertexCount].g = 96;
                    trailStage[vertexCount].b = 96;
                    trailStage[vertexCount + 1].r = 96;
                    trailStage[vertexCount + 1].g = 96;
                    trailStage[vertexCount + 1].b = 96;
                    trailStage[vertexCount + 2].r = 96;
                    trailStage[vertexCount + 2].g = 96;
                    trailStage[vertexCount + 2].b = 96;
                    trailStage[vertexCount + 1].x = x - particleSnowWidth;
                    trailStage[vertexCount + 2].x = x + particleSnowWidth;
                    trailStage[vertexCount].u = particleGreyColour;
                    trailStage[vertexCount + 1].u = particleGreyColour;
                    trailStage[vertexCount + 2].u = particleGreyColour;
                    break;
                }
                trailIndexLists[material].triangles[trailIndexLists[material].count].v1 = vertexCount;
                trailIndexLists[material].triangles[trailIndexLists[material].count].v2 = vertexCount + 1;
                trailIndexLists[material].triangles[trailIndexLists[material].count].v3 = vertexCount + 2;
                trailIndexLists[material].count++;
                vertexCount += 3;
            }
        }
    }
    vertices = (PCVertex *)trackVertexCursor;
    triangleLists = (ParticleTriangleList *)(vertices + vertexCount);
    memcpy(vertices, trailStage, vertexCount * sizeof(PCVertex));
    cursor = (char *)(triangleLists + 23);
    trackVertexCursor = cursor;
    for (i = 0; i <= normalSetNo; i++) {
        count = trailIndexLists[i].count;
        if (count) {
            triangleLists[i].tris = (D3DTRIANGLE *)cursor;
            triangleLists[i].count = count;
            /* Read again after the header stores, as the original does. */
            count = trailIndexLists[i].count;
            memcpy(cursor, trailIndexLists[i].triangles,
                   count * sizeof(D3DTRIANGLE));
            cursor += count * sizeof(D3DTRIANGLE);
        } else {
            triangleLists[i].count = 0;
            triangleLists[i].tris = 0;
        }
        trackVertexCursor = cursor;
    }
    AddRenderSkeleton(camera->super, vertices, vertexCount, triangleLists, 0, 0, 1);
}
#endif
#endif
#ifdef EQUIV
#ifdef NETPLAY
#pragma intrinsic(memcpy)
/* NET: a transcription of the /Od code. NET's colour globals hold other colours than the D3D
   names: white is particleOrangeColour, orange particleDarkGreyColour, dark grey D_b3c540. */
void AsmDrawParticles(SParticle *particles, Skeleton *camera, int unused)
{
    PCVertex *vertices;
    ParticleTriangleList *triangleLists;
    int material;
    int index;
    int vertexTotal;
    VECTOR cameraPos;

    SetSkeletonTransform(camera->super);
    cameraPos.vx = cameraPtr->camPos->relative.t[0];
    cameraPos.vy = cameraPtr->camPos->relative.t[1];
    cameraPos.vz = cameraPtr->camPos->relative.t[2];
    vertexTotal = 0;
    for (index = 0; index <= normalSetNo; index++)
        trailIndexLists[index].count = 0;
    if (rgbTextures)
        material = normalSetNo;
    else
        material = 11;
    for (index = 0; index < PARTICLE_COUNT; index++) {
        if (particles[index].life > 0) {
            int worldXpos = particles[index].pos.vx + cameraPos.vx;
            int worldYpos = particles[index].pos.vy + cameraPos.vy;
            int worldZpos = particles[index].pos.vz + cameraPos.vz;
            int tmp; /* declared by the original and never read */

            if (rgbTextures) {
                trailStage[vertexTotal].v = 0;
                trailStage[vertexTotal + 1].v = 0;
                trailStage[vertexTotal + 2].v = 0;
            } else {
                trailStage[vertexTotal].v = 110.0f / 255.0f;
                trailStage[vertexTotal + 1].v = 110.0f / 255.0f;
                trailStage[vertexTotal + 2].v = 110.0f / 255.0f;
            }
            trailStage[vertexTotal].a = particleAlpha;
            trailStage[vertexTotal + 1].a = particleAlpha;
            trailStage[vertexTotal + 2].a = particleAlpha;
            trailStage[vertexTotal].x = particles[index].cameraPos.vx / 4096.0f;
            trailStage[vertexTotal].y = particles[index].cameraPos.vy / 4096.0f;
            trailStage[vertexTotal].z = particles[index].cameraPos.vz / 4096.0f;
            particles[index].cameraPos.vx = worldXpos;
            particles[index].cameraPos.vy = worldYpos;
            particles[index].cameraPos.vz = worldZpos;
            if (particles[index].flags) {
                trailStage[vertexTotal + 1].x = worldXpos / 4096.0f;
                trailStage[vertexTotal + 1].y = worldYpos / 4096.0f;
                trailStage[vertexTotal + 1].z = worldZpos / 4096.0f;
                trailStage[vertexTotal + 2].x = worldXpos / 4096.0f;
                trailStage[vertexTotal + 2].y = worldYpos / 4096.0f;
                trailStage[vertexTotal + 2].z = worldZpos / 4096.0f;
                switch (particles[index].type) {
                case 1:
                    if (rgbTextures) {
                        trailStage[vertexTotal].r = 127;
                        trailStage[vertexTotal].g = 127;
                        trailStage[vertexTotal].b = 127;
                    } else {
                        trailStage[vertexTotal].r = 255;
                        trailStage[vertexTotal].g = 255;
                        trailStage[vertexTotal].b = 255;
                    }
                    trailStage[vertexTotal + 1].r = 255;
                    trailStage[vertexTotal + 1].g = 255;
                    trailStage[vertexTotal + 1].b = 255;
                    trailStage[vertexTotal + 2].r = 255;
                    trailStage[vertexTotal + 2].g = 255;
                    trailStage[vertexTotal + 2].b = 255;
                    trailStage[vertexTotal + 2].y = worldYpos / 4096.0f + 0.003f;
                    trailStage[vertexTotal].u = particleOrangeColour;
                    trailStage[vertexTotal + 1].u = particleOrangeColour;
                    trailStage[vertexTotal + 2].u = particleOrangeColour;
                    break;
                case 2:
                    if (rgbTextures) {
                        trailStage[vertexTotal].r = 63;
                        trailStage[vertexTotal].g = 0;
                        trailStage[vertexTotal].b = 0;
                    } else {
                        trailStage[vertexTotal].r = 255;
                        trailStage[vertexTotal].g = 63;
                        trailStage[vertexTotal].b = 0;
                    }
                    trailStage[vertexTotal + 1].r = 255;
                    trailStage[vertexTotal + 1].g = 63;
                    trailStage[vertexTotal + 1].b = 0;
                    trailStage[vertexTotal + 2].r = 255;
                    trailStage[vertexTotal + 2].g = 63;
                    trailStage[vertexTotal + 2].b = 0;
                    trailStage[vertexTotal + 2].x = worldXpos / 4096.0f + 0.005f;
                    trailStage[vertexTotal].u = particleDarkGreyColour;
                    trailStage[vertexTotal + 1].u = particleDarkGreyColour;
                    trailStage[vertexTotal + 2].u = particleDarkGreyColour;
                    break;
                case 3:
                    if (rgbTextures) {
                        trailStage[vertexTotal].r = 12;
                        trailStage[vertexTotal].g = 12;
                        trailStage[vertexTotal].b = 12;
                    } else {
                        trailStage[vertexTotal].r = 48;
                        trailStage[vertexTotal].g = 48;
                        trailStage[vertexTotal].b = 48;
                    }
                    trailStage[vertexTotal + 1].r = 48;
                    trailStage[vertexTotal + 1].g = 48;
                    trailStage[vertexTotal + 1].b = 48;
                    trailStage[vertexTotal + 2].r = 48;
                    trailStage[vertexTotal + 2].g = 48;
                    trailStage[vertexTotal + 2].b = 48;
                    trailStage[vertexTotal + 2].x = worldXpos / 4096.0f + 0.003f;
                    trailStage[vertexTotal].u = D_b3c540;
                    trailStage[vertexTotal + 1].u = D_b3c540;
                    trailStage[vertexTotal + 2].u = D_b3c540;
                    break;
                case 4:
                    if (rgbTextures) {
                        trailStage[vertexTotal].r = 0;
                        trailStage[vertexTotal].g = 0;
                        trailStage[vertexTotal].b = 63;
                    } else {
                        trailStage[vertexTotal].r = 63;
                        trailStage[vertexTotal].g = 63;
                        trailStage[vertexTotal].b = 255;
                    }
                    trailStage[vertexTotal + 1].r = 63;
                    trailStage[vertexTotal + 1].g = 63;
                    trailStage[vertexTotal + 1].b = 255;
                    trailStage[vertexTotal + 2].r = 63;
                    trailStage[vertexTotal + 2].g = 63;
                    trailStage[vertexTotal + 2].b = 255;
                    trailStage[vertexTotal + 2].y = worldYpos / 4096.0f + 0.003f;
                    trailStage[vertexTotal].u = particleBlueColour;
                    trailStage[vertexTotal + 1].u = particleBlueColour;
                    trailStage[vertexTotal + 2].u = particleBlueColour;
                    break;
                case 5:
                    trailStage[vertexTotal].r = 96;
                    trailStage[vertexTotal].g = 96;
                    trailStage[vertexTotal].b = 96;
                    trailStage[vertexTotal + 1].r = 96;
                    trailStage[vertexTotal + 1].g = 96;
                    trailStage[vertexTotal + 1].b = 96;
                    trailStage[vertexTotal + 2].r = 96;
                    trailStage[vertexTotal + 2].g = 96;
                    trailStage[vertexTotal + 2].b = 96;
                    trailStage[vertexTotal + 1].x = worldXpos / 4096.0f - particleSnowWidth;
                    trailStage[vertexTotal + 2].x = worldXpos / 4096.0f + particleSnowWidth;
                    trailStage[vertexTotal].u = particleGreyColour;
                    trailStage[vertexTotal + 1].u = particleGreyColour;
                    trailStage[vertexTotal + 2].u = particleGreyColour;
                    break;
                }
                trailIndexLists[material].triangles[trailIndexLists[material].count].v1 = vertexTotal;
                trailIndexLists[material].triangles[trailIndexLists[material].count].v2 = vertexTotal + 1;
                trailIndexLists[material].triangles[trailIndexLists[material].count].v3 = vertexTotal + 2;
                trailIndexLists[material].count++;
                vertexTotal += 3;
            } else {
                /* the original keeps the jump over this empty else */
            }
        }
    }
    vertices = (PCVertex *)trackVertexCursor;
    trackVertexCursor += vertexTotal * sizeof(PCVertex);
    memcpy(vertices, trailStage, vertexTotal * sizeof(PCVertex));
    triangleLists = (ParticleTriangleList *)trackVertexCursor;
    trackVertexCursor += 23 * sizeof(ParticleTriangleList);
    for (index = 0; index <= normalSetNo; index++) {
        if (trailIndexLists[index].count != 0) {
            triangleLists[index].count = trailIndexLists[index].count;
            triangleLists[index].tris = (D3DTRIANGLE *)trackVertexCursor;
            trackVertexCursor += trailIndexLists[index].count * sizeof(D3DTRIANGLE);
            memcpy(triangleLists[index].tris, trailIndexLists[index].triangles,
                   trailIndexLists[index].count * sizeof(D3DTRIANGLE));
        } else {
            triangleLists[index].count = 0;
            triangleLists[index].tris = 0;
        }
    }
    AddRenderSkeleton(camera->super, vertices, vertexTotal, triangleLists, 0, 0, 1);
}
#endif
#endif
#endif

void UpdateParticles(SParticle *particles, Skeleton *camera, int unused)
{
    int i;
    VECTOR position;

    SetSkeletonTransform(camera->super);
    i = 0;
    position.vx = cameraPtr->camPos->relative.t[0];
    position.vy = cameraPtr->camPos->relative.t[1];
    position.vz = cameraPtr->camPos->relative.t[2];
    for (; i < PARTICLE_COUNT; i++) {
        if (particles[i].life > 0) {
            particles[i].pos.vx += particles[i].vel.vx;
            particles[i].pos.vy += particles[i].vel.vy;
            particles[i].pos.vz += particles[i].vel.vz;
            particles[i].life--;
            if (particles[i].flags == 0) {
                particles[i].flags = 1;
                particles[i].cameraPos.vx = particles[i].pos.vx + position.vx;
                particles[i].cameraPos.vy = particles[i].pos.vy + position.vy;
                particles[i].cameraPos.vz = particles[i].pos.vz + position.vz;
            }
        }
    }
}
