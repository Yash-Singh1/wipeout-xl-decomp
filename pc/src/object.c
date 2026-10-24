#include "functions.h"
#include "mathinline.h"
#ifdef NETPLAY
#include "hud.h"
#include <string.h>
#pragma intrinsic(sqrt, sin, cos, memcpy)
/* Network build: each trig step of SetCameraDirection sits behind this switch; the empty if/else leaves the jump the network build has there. */
#define NET_TRIG_STEP 1
/* Network build: the texture set step of DrawEnvMap sits behind this switch; the empty else leaves the jump the network build has there. */
#define NET_ENV_STEP 1
#endif
#ifdef NON_MATCHING
#include "pc_particles.h"
#include "hud.h"
#include "d3dpoly.h"
#endif

void OuterProduct12(VECTOR *left, VECTOR *right, VECTOR *crossProduct);

extern MATRIX cameraMatrix;
extern VECTOR cameraOffset;

void SetCameraPosition(long x, long y, long z)
{
    cameraOffset.vx = 0;
    cameraOffset.vy = 0;
    cameraOffset.vz = 0;
    cameraMatrix.t[0] = x;
    cameraMatrix.t[1] = y;
    cameraMatrix.t[2] = z;
    SetTransMatrix(&cameraMatrix);
}

void SetCameraDirection(short x, short y, short z)
{
    short sinPit;
    short sy;
    short sz;
    short cosPit;
    short cy;
    short cosRzAng;

#ifdef NETPLAY
    if (NET_TRIG_STEP) { } else { }
    sinPit = (short)(sin((x / fixedPointScale) * angleScale) * fixedPointScale);
    if (NET_TRIG_STEP) { } else { }
    sy = (short)(sin((y / fixedPointScale) * angleScale) * fixedPointScale);
    if (NET_TRIG_STEP) { } else { }
    sz = (short)(sin((z / fixedPointScale) * angleScale) * fixedPointScale);
    if (NET_TRIG_STEP) { } else { }
    cosPit = (short)(cos((x / fixedPointScale) * angleScale) * fixedPointScale);
    if (NET_TRIG_STEP) { } else { }
    cy = (short)(cos((y / fixedPointScale) * angleScale) * fixedPointScale);
    if (NET_TRIG_STEP) { } else { }
    cosRzAng = (short)(cos((z / fixedPointScale) * angleScale) * fixedPointScale);
#else
    sinPit = rsin(x);
    sy = rsin(y);
    sz = rsin(z);
    cosPit = rcos(x);
    cy = rcos(y);
    cosRzAng = rcos(z);
#endif

    cameraMatrix.m[0][0] = (cy * cosRzAng) >> 12;
    cameraMatrix.m[0][1] = ((-cy) * sz) >> 12;
    cameraMatrix.m[0][2] = -sy;

    cameraMatrix.m[1][0] = ((cosPit * sz) >> 12) - ((((sinPit * sy) >> 12) * cosRzAng) >> 12);
    cameraMatrix.m[1][1] = ((cosPit * cosRzAng) >> 12) + ((((sinPit * sy) >> 12) * sz) >> 12);
    cameraMatrix.m[1][2] = ((-sinPit) * cy) >> 12;

    cameraMatrix.m[2][0] = ((sinPit * sz) >> 12) + ((((cosPit * sy) >> 12) * cosRzAng) >> 12);
    cameraMatrix.m[2][1] = ((sinPit * cosRzAng) >> 12) - ((((cosPit * sy) >> 12) * sz) >> 12);
    cameraMatrix.m[2][2] = (cosPit * cy) >> 12;

    SetRotMatrix(&cameraMatrix);
}

void InitialiseAmbientColour(void)
{
    SetBackColor(0x00, 0x00, 0xff);
}

void InitialiseColourMatrix(void)
{
    MATRIX m;

    m.m[0][0] = 0x1000;
    m.m[1][0] = 0;
    m.m[2][0] = 0;
    m.m[0][1] = 0;
    m.m[1][1] = 0x1000;
    m.m[2][1] = 0;
    m.m[0][2] = 0;
    m.m[1][2] = 0;
    m.m[2][2] = 0x1000;
    SetColorMatrix(&m);
}

#ifdef NETPLAY
/* NET adds the Y and Z products before X to preserve its evaluation order. */
void MatrixMultiply(MATRIX *leftMatrix, MATRIX *rightMatrix, MATRIX *resultMatrix)
{
    resultMatrix->t[0] = (((rightMatrix->m[0][1] * leftMatrix->t[1] +
                            rightMatrix->m[0][2] * leftMatrix->t[2]) +
                           rightMatrix->m[0][0] * leftMatrix->t[0]) >> 12) + rightMatrix->t[0];
    resultMatrix->t[1] = (((rightMatrix->m[1][1] * leftMatrix->t[1] +
                            rightMatrix->m[1][2] * leftMatrix->t[2]) +
                           rightMatrix->m[1][0] * leftMatrix->t[0]) >> 12) + rightMatrix->t[1];
    resultMatrix->t[2] = (((rightMatrix->m[2][1] * leftMatrix->t[1] +
                            rightMatrix->m[2][2] * leftMatrix->t[2]) +
                           rightMatrix->m[2][0] * leftMatrix->t[0]) >> 12) + rightMatrix->t[2];
    MulMatrix0(leftMatrix, rightMatrix, resultMatrix);
}
#else
void MatrixMultiply(MATRIX *m1, MATRIX *m2, MATRIX *m3)
{
    m3->t[0] = m2->t[0] + ((m1->t[0] * m2->m[0][0] + m1->t[1] * m2->m[0][1] + m1->t[2] * m2->m[0][2]) >> 12);
    m3->t[1] = m2->t[1] + ((m1->t[0] * m2->m[1][0] + m1->t[1] * m2->m[1][1] + m1->t[2] * m2->m[1][2]) >> 12);
    m3->t[2] = m2->t[2] + ((m1->t[0] * m2->m[2][0] + m1->t[1] * m2->m[2][1] + m1->t[2] * m2->m[2][2]) >> 12);
    MulMatrix0(m1, m2, m3);
}
#endif

void InitMatrix(MATRIX *m)
{
    m->m[0][0] = 0x1000;
    m->m[1][0] = 0;
    m->m[2][0] = 0;
    m->m[0][1] = 0;
    m->m[1][1] = 0x1000;
    m->m[2][1] = 0;
    m->m[0][2] = 0;
    m->m[1][2] = 0;
    m->m[2][2] = 0x1000;
    m->t[0] = 0;
    m->t[1] = 0;
    m->t[2] = 0;
}

void InitialiseLightMatrix(void)
{
    MATRIX lm;

    lm.m[0][0] = 4096;
    lm.m[0][1] = 0;
    lm.m[0][2] = 0;
    lm.m[1][0] = 0;
    lm.m[1][1] = 4096;
    lm.m[1][2] = 0;
    lm.m[2][0] = 0;
    lm.m[2][1] = 0;
    lm.m[2][2] = 4096;
    SetLightMatrix(&lm);
}

void Normalise(VECTOR *vector)
{
    long magnitude;

#ifdef NETPLAY
    magnitude = ((vector->vz >> 6) * (vector->vz >> 6)) +
                ((vector->vx >> 6) * (vector->vx >> 6)) +
                ((vector->vy >> 6) * (vector->vy >> 6));
    magnitude = (long)sqrt((double)magnitude);
#else
    magnitude = (long)sqrt((double)DotProduct(vector, vector));
#endif
    if (magnitude == 0)
        magnitude = 1;
    vector->vx = (vector->vx << 6) / magnitude;
    vector->vy = (vector->vy << 6) / magnitude;
    vector->vz = (vector->vz << 6) / magnitude;
}

int DotProduct(VECTOR *first, VECTOR *second)
{
    return ((second->vy >> 6) * (first->vy >> 6)) + ((second->vz >> 6) * (first->vz >> 6)) + ((second->vx >> 6) * (first->vx >> 6));
}

void MyCrossProduct(VECTOR *first, VECTOR *second)
{
    OuterProduct12(first, second, first);
}

#ifndef NETPLAY
void PreCalcNormals(Object *object, SVECTOR *normals)
{
    VECTOR   d1;   /* first edge vector of the face (edge1 in the original) */
    short    primitiveCount;
    int      vIndex;
    short    vertTotal;
    int      faceIndex;
    VECTOR   e2;   /* second edge vector of the face (edge2 in the original) */
    SVECTOR *vertexBuf;
    VECTOR   faceNorms[256];
    Poly     cur;
    int      n;

    vertexBuf = object->vertices;
    vertTotal = object->vertexCount;
    primitiveCount = object->primitiveCount;
    cur.prim = object->primitives;
    for (faceIndex = 0; faceIndex < primitiveCount; faceIndex++) {
        d1.vx = (vertexBuf[cur.f3->coords[1]].vx - vertexBuf[cur.f3->coords[0]].vx) << 12;
        d1.vy = (vertexBuf[cur.f3->coords[1]].vy - vertexBuf[cur.f3->coords[0]].vy) << 12;
        d1.vz = (vertexBuf[cur.f3->coords[1]].vz - vertexBuf[cur.f3->coords[0]].vz) << 12;
        e2.vx = (vertexBuf[cur.f3->coords[2]].vx - vertexBuf[cur.f3->coords[0]].vx) << 12;
        e2.vy = (vertexBuf[cur.f3->coords[2]].vy - vertexBuf[cur.f3->coords[0]].vy) << 12;
        e2.vz = (vertexBuf[cur.f3->coords[2]].vz - vertexBuf[cur.f3->coords[0]].vz) << 12;
        Normalise(&d1);
        Normalise(&e2);
        MyCrossProduct(&d1, &e2);
        Normalise(&d1);
        faceNorms[faceIndex].vx = d1.vx;
        faceNorms[faceIndex].vy = d1.vy;
        faceNorms[faceIndex].vz = d1.vz;
        switch (cur.f3->type) {
        case TypeG3:
            cur.g3++;
            break;
        case TypeG4:
            cur.g4++;
            break;
        case TypeF3:
            cur.f3++;
            break;
        case TypeF4:
            cur.f4++;
            break;
        }
    }
    for (vIndex = 0; vIndex < vertTotal; vIndex++) {
        d1.vx = 0;
        d1.vy = 0;
        d1.vz = 0;
        n = 0;
        cur.prim = object->primitives;
        for (faceIndex = 0; faceIndex < primitiveCount; faceIndex++) {
            switch (cur.f3->type) {
            case TypeF3:
                if (cur.f3->coords[0] == vIndex || cur.f3->coords[1] == vIndex || cur.f3->coords[2] == vIndex) {
                    n++;
                    d1.vx += faceNorms[faceIndex].vx;
                    d1.vy += faceNorms[faceIndex].vy;
                    d1.vz += faceNorms[faceIndex].vz;
                }
                cur.f3++;
                break;
            case TypeF4:
                if (cur.f4->coords[0] == vIndex || cur.f4->coords[1] == vIndex || cur.f4->coords[2] == vIndex || cur.f4->coords[3] == vIndex) {
                    n++;
                    d1.vx += faceNorms[faceIndex].vx;
                    d1.vy += faceNorms[faceIndex].vy;
                    d1.vz += faceNorms[faceIndex].vz;
                }
                cur.f4 = (F4 *)(cur.f3 + 1);
                break;
            }
        }
        if (n > 0) {
            d1.vx /= n;
            d1.vy /= n;
            d1.vz /= n;
            Normalise(&d1);
            normals[vIndex].vx = d1.vx;
            normals[vIndex].vy = d1.vy;
            normals[vIndex].vz = d1.vz;
        }
    }
}
#endif
#ifdef EQUIV
#ifdef NETPLAY
/* The network build's /Od frame needs these local names; the D3D copy above keeps the same ones. */
void PreCalcNormals(Object *object, SVECTOR *normals)
{
    VECTOR   d1;   /* first edge vector of the face (edge1 in the original) */
    short    primitiveCount;
    int      vIndex;
    short    vertTotal;
    int      faceIndex;
    VECTOR   e2;   /* second edge vector of the face (edge2 in the original) */
    SVECTOR *vertexBuf;
    VECTOR   faceNorms[256];
    Poly     cur;
    int      n;

    vertexBuf = object->vertices;
    vertTotal = object->vertexCount;
    primitiveCount = object->primitiveCount;
    cur.prim = object->primitives;
    for (faceIndex = 0; faceIndex < primitiveCount; faceIndex++) {
        d1.vx = (vertexBuf[cur.f3->coords[1]].vx - vertexBuf[cur.f3->coords[0]].vx) << 12;
        d1.vy = (vertexBuf[cur.f3->coords[1]].vy - vertexBuf[cur.f3->coords[0]].vy) << 12;
        d1.vz = (vertexBuf[cur.f3->coords[1]].vz - vertexBuf[cur.f3->coords[0]].vz) << 12;
        e2.vx = (vertexBuf[cur.f3->coords[2]].vx - vertexBuf[cur.f3->coords[0]].vx) << 12;
        e2.vy = (vertexBuf[cur.f3->coords[2]].vy - vertexBuf[cur.f3->coords[0]].vy) << 12;
        e2.vz = (vertexBuf[cur.f3->coords[2]].vz - vertexBuf[cur.f3->coords[0]].vz) << 12;
        Normalise(&d1);
        Normalise(&e2);
        MyCrossProduct(&d1, &e2);
        Normalise(&d1);
        faceNorms[faceIndex].vx = d1.vx;
        faceNorms[faceIndex].vy = d1.vy;
        faceNorms[faceIndex].vz = d1.vz;
        switch (cur.f3->type) {
        case TypeG3:
            cur.g3++;
            break;
        case TypeG4:
            cur.g4++;
            break;
        case TypeF3:
            cur.f3++;
            break;
        case TypeF4:
            cur.f4++;
            break;
        }
    }
    for (vIndex = 0; vIndex < vertTotal; vIndex++) {
        d1.vx = 0;
        d1.vy = 0;
        d1.vz = 0;
        n = 0;
        cur.prim = object->primitives;
        for (faceIndex = 0; faceIndex < primitiveCount; faceIndex++) {
            switch (cur.f3->type) {
            case TypeF3:
                if (cur.f3->coords[0] == vIndex || cur.f3->coords[1] == vIndex || cur.f3->coords[2] == vIndex) {
                    n++;
                    d1.vx += faceNorms[faceIndex].vx;
                    d1.vy += faceNorms[faceIndex].vy;
                    d1.vz += faceNorms[faceIndex].vz;
                }
                cur.f3++;
                break;
            case TypeF4:
                if (cur.f4->coords[0] == vIndex || cur.f4->coords[1] == vIndex || cur.f4->coords[2] == vIndex || cur.f4->coords[3] == vIndex) {
                    n++;
                    d1.vx += faceNorms[faceIndex].vx;
                    d1.vy += faceNorms[faceIndex].vy;
                    d1.vz += faceNorms[faceIndex].vz;
                }
                cur.f4 = (F4 *)(cur.f3 + 1);
                break;
            }
        }
        if (n > 0) {
            d1.vx /= n;
            d1.vy /= n;
            d1.vz /= n;
            Normalise(&d1);
            normals[vIndex].vx = d1.vx;
            normals[vIndex].vy = d1.vy;
            normals[vIndex].vz = d1.vz;
        }
    }
}
#endif
#endif


#ifdef EQUIV
/* Network build: the reflection dot is DotProduct against (0, 0, 4096) and the u index reuses the dot
   local; the divisors are written inline, and the texture column is converted to unsigned short
   before the offset is added (the load order the network build uses).
   The byte-to-word cast retains both masks emitted by the original compiler. */
#ifdef NETPLAY
/* Adding zero to the second index preserves the original /Od operand loads. */
#define ENV_EDGE(a, b, originalA, originalB, adjustedA, adjustedB) \
    reflect.vx = (StaticRVert[b + 0].vx + StaticRVert[a].vx) >> 1; \
    reflect.vy = (StaticRVert[b + 0].vy + StaticRVert[a].vy) >> 1; \
    reflect.vz = (StaticRVert[b + 0].vz + StaticRVert[a].vz) >> 1; \
    Normalise(&reflect); \
    envMapNormalDot = DotProduct(&reflect, &localViewAxis); \
    reflect.vx = ((reflect.vx * envMapNormalDot) >> 11) - localViewAxis.vx; \
    reflect.vz = ((reflect.vz * envMapNormalDot) >> 11) - localViewAxis.vz; \
    envMapNormalDot = (ratan2(reflect.vx, reflect.vz) >> 6) & 63; \
    if (originalB < originalA) { \
        if (envMapNormalDot < originalB || originalA < envMapNormalDot) adjustedB = originalB + 64; \
    } else { \
        if (envMapNormalDot < originalA || originalB < envMapNormalDot) adjustedA = originalA + 64; \
    }

#define ENV_VERTEX(slot, vertex, texU) \
    trailStage[outputCount + slot].x = currentVertexData[vertex].vx / 4096.0f; \
    trailStage[outputCount + slot].y = currentVertexData[vertex].vy / 4096.0f; \
    trailStage[outputCount + slot].z = currentVertexData[vertex].vz / 4096.0f; \
    trailStage[outputCount + slot].u = (((unsigned short)(unsigned char)(texU & 63)) + uOffset) / 255.0f; \
    trailStage[outputCount + slot].v = ((envMapV[vertex] & 63) + envRowOffset) / 255.0f; \
    trailStage[outputCount + slot].a = opaqueAlpha; \
    trailStage[outputCount + slot].r = envmapr; \
    trailStage[outputCount + slot].g = envmapg; \
    trailStage[outputCount + slot].b = envmapb;
#else
#define ENV_EDGE(a, b, originalA, originalB, adjustedA, adjustedB) \
    reflect.vx = (StaticRVert[a].vx + StaticRVert[b].vx) >> 1; \
    reflect.vy = (StaticRVert[a].vy + StaticRVert[b].vy) >> 1; \
    reflect.vz = (StaticRVert[a].vz + StaticRVert[b].vz) >> 1; \
    Normalise(&reflect); \
    dot = reflect.vz & ~63; \
    reflect.vx = (dot * reflect.vx) >> 11; \
    reflect.vz = ((dot * reflect.vz) >> 11) - 4096; \
    u = (ratan2(reflect.vx, reflect.vz) >> 6) & 63; \
    if (originalB < originalA) { \
        if (u < originalB || originalA < u) adjustedB = originalB + 64; \
    } else { \
        if (u < originalA || originalB < u) adjustedA = originalA + 64; \
    }

#define ENV_VERTEX(slot, vertex, texU) \
    trailStage[index + slot].x = vertices[vertex].vx * (1.0f / 4096.0f); \
    trailStage[index + slot].y = vertices[vertex].vy * (1.0f / 4096.0f); \
    trailStage[index + slot].z = vertices[vertex].vz * (1.0f / 4096.0f); \
    trailStage[index + slot].u = ((texU & 63) + uOffset) * (1.0f / 255.0f); \
    trailStage[index + slot].v = (envMapV[vertex] & 63) * (1.0f / 255.0f); \
    trailStage[index + slot].a = opaqueAlpha; \
    trailStage[index + slot].r = envmapr; \
    trailStage[index + slot].g = envmapg; \
    trailStage[index + slot].b = envmapb;

#endif
#ifdef NETPLAY
#define ENV_SET() \
    { \
        int textureSet; \
        textureSet = GetTPage(1, 0, 320, 256) & 31; \
        if (textureSet > 15 && textureSet < 100) textureSet -= 10; \
        else if (textureSet < 16) textureSet -= 5; \
        if (NET_ENV_STEP) set = textureSet; else { } \
    } \
    if (set > normalSetNo) set = normalSetNo;
#else
#define ENV_SET() \
    { \
        int textureSet; \
        textureSet = GetTPage(1, 0, 320, 256) & 31; \
        if (textureSet > 15 && textureSet < 100) textureSet -= 10; \
        else if (textureSet < 16) textureSet -= 5; \
        set = textureSet; \
    } \
    if (set > normalSetNo) set = normalSetNo;
#endif

/* Builds reflected texture coordinates and batches the environment-mapped faces. */
#ifdef NETPLAY
void DrawEnvMap(Object *object, SVECTOR *normals, int x, int y, float scale)
{
    VECTOR reflect;
    VECTOR translation;
    short corner3;
    short vertex2;
    short vertex1;
    int column0;
    int textureRow;
    int set;
    short envVertexIndex;
    SVECTOR *currentVertexData;
    short objectPoint0;
    unsigned char uOffset = 0;
    unsigned char envRowOffset = 0;
    DVECTOR *screenVertices;
    short envBucketIndex;
    short normalCount;
    unsigned short *renderScreenDepths;
    Poly primitive;
    long envClipFlag;
    unsigned short *mappedScreenFlags;
    int baseColumn1;
    int original2;
    int sourceU3;
    int sourceU0;
    int envMapNormalDot;
    MATRIX renderRotation;
    int outputCount;
    int envMapColumn3;
    short faceTotal;
    VECTOR localViewAxis;
    int column2;
    TriList *mappedBatches;
    int currentTextureU1;
    unsigned char *out;

    if (!texture16Bit && !rgbTextures) {
        uOffset = 64;
        if (awardedMedal == 1) uOffset = 0;
        else if (awardedMedal == 3) uOffset = 128;
    }
    outputCount = 0;
    /* Adding zero preserves the original loop comparison load orders. */
    for (envBucketIndex = 0; normalSetNo + 0 >= envBucketIndex; envBucketIndex++)
        quadBuckets[envBucketIndex].count = 0;
    SetSkeletonTransform(object->skeleton);
    localViewAxis.vx = 0;
    localViewAxis.vy = 0;
    localViewAxis.vz = 4096;
    currentVertexData = object->vertices;
    normalCount = object->vertexCount;
    primitive.prim = object->primitives;
    faceTotal = object->primitiveCount;
    screenVertices = StaticScreen;
    renderScreenDepths = StaticDepth;
    mappedScreenFlags = StaticFlags;
    RotTransPersN(currentVertexData, screenVertices, renderScreenDepths, mappedScreenFlags, normalCount);
    translation.vx = 0;
    translation.vy = 0;
    translation.vz = 0;
    TransMatrix(&renderRotation, &translation);
    SetTransMatrix(&renderRotation);
    for (envBucketIndex = 0; envBucketIndex < normalCount + 0; envBucketIndex++)
        RotTrans(&normals[envBucketIndex], &StaticRVert[envBucketIndex], &envClipFlag);
    for (envVertexIndex = 0; envVertexIndex < normalCount + 0; envVertexIndex++) {
        reflect.vx = StaticRVert[envVertexIndex].vx;
        reflect.vy = StaticRVert[envVertexIndex].vy;
        reflect.vz = StaticRVert[envVertexIndex].vz;
        envMapNormalDot = DotProduct(&reflect, &localViewAxis);
        reflect.vx = ((reflect.vx * envMapNormalDot) >> 11) - localViewAxis.vx;
        reflect.vy = ((reflect.vy * envMapNormalDot) >> 11) - localViewAxis.vy;
        reflect.vz = ((reflect.vz * envMapNormalDot) >> 11) - localViewAxis.vz;
        envMapU[envVertexIndex] = (ratan2(reflect.vx, reflect.vz) >> 6) & 63;
        textureRow = 63 - (((reflect.vy << 6) >> 13) + 32);
        if (textureRow < 0) textureRow = 0;
        if (textureRow > 63) textureRow = 63;
        envMapV[envVertexIndex] = textureRow;
    }
    primitive.prim = object->primitives;
    for (envBucketIndex = 0; envBucketIndex < faceTotal + 0; envBucketIndex++) {
        switch (primitive.prim->type) {
        case TypeF3:
            objectPoint0 = primitive.f3->coords[0];
            vertex1 = primitive.f3->coords[1];
            vertex2 = primitive.f3->coords[2];
            ENV_SET();
            sourceU0 = envMapU[objectPoint0];
            baseColumn1 = envMapU[vertex1];
            original2 = envMapU[vertex2];
            column0 = sourceU0;
            currentTextureU1 = baseColumn1;
            column2 = original2;
            ENV_EDGE(objectPoint0, vertex1, sourceU0, baseColumn1, column0, currentTextureU1);
            ENV_EDGE(vertex1, vertex2, baseColumn1, original2, currentTextureU1, column2);
            ENV_EDGE(objectPoint0, vertex2, sourceU0, original2, column0, column2);
            ENV_VERTEX(0, objectPoint0, column0);
            ENV_VERTEX(1, vertex1, currentTextureU1);
            ENV_VERTEX(2, vertex2, column2);
            quadBuckets[set].triangles[quadBuckets[set].count].v1 = outputCount;
            quadBuckets[set].triangles[quadBuckets[set].count].v2 = outputCount + 1;
            quadBuckets[set].triangles[quadBuckets[set].count].v3 = outputCount + 2;
            quadBuckets[set].count++;
            primitive.f3++;
            outputCount += 3;
            break;
        case TypeF4:
            objectPoint0 = primitive.f4->coords[0];
            vertex1 = primitive.f4->coords[1];
            vertex2 = primitive.f4->coords[2];
            corner3 = primitive.f4->coords[3];
            ENV_SET();
            sourceU0 = envMapU[objectPoint0];
            baseColumn1 = envMapU[vertex1];
            original2 = envMapU[vertex2];
            sourceU3 = envMapU[corner3];
            column0 = sourceU0;
            currentTextureU1 = baseColumn1;
            column2 = original2;
            envMapColumn3 = sourceU3;
            ENV_EDGE(objectPoint0, vertex1, sourceU0, baseColumn1, column0, currentTextureU1);
            ENV_EDGE(vertex1, vertex2, baseColumn1, original2, currentTextureU1, column2);
            ENV_EDGE(vertex2, corner3, original2, sourceU3, column2, envMapColumn3);
            ENV_EDGE(objectPoint0, corner3, sourceU0, sourceU3, column0, envMapColumn3);
            ENV_VERTEX(0, objectPoint0, column0);
            ENV_VERTEX(1, vertex1, currentTextureU1);
            ENV_VERTEX(2, vertex2, column2);
            ENV_VERTEX(3, corner3, envMapColumn3);
            quadBuckets[set].triangles[quadBuckets[set].count].v1 = outputCount;
            quadBuckets[set].triangles[quadBuckets[set].count].v2 = outputCount + 1;
            quadBuckets[set].triangles[quadBuckets[set].count].v3 = outputCount + 3;
            quadBuckets[set].count++;
            quadBuckets[set].triangles[quadBuckets[set].count].v1 = outputCount;
            quadBuckets[set].triangles[quadBuckets[set].count].v2 = outputCount + 3;
            quadBuckets[set].triangles[quadBuckets[set].count].v3 = outputCount + 2;
            quadBuckets[set].count++;
            primitive.f4++;
            outputCount += 4;
            break;
        }
    }
    if (outputCount) {
        out = trackVertexCursor;
        trackVertexCursor += outputCount * sizeof(PCVertex);
        memcpy(out, trailStage, outputCount * sizeof(PCVertex));
        mappedBatches = (TriList *)trackVertexCursor;
        trackVertexCursor += 23 * sizeof(TriList);
        for (envBucketIndex = 0; normalSetNo + 0 >= envBucketIndex; envBucketIndex++) {
            if (quadBuckets[envBucketIndex].count) {
                mappedBatches[envBucketIndex].count = quadBuckets[envBucketIndex].count;
                mappedBatches[envBucketIndex].tris = (D3DTRIANGLE *)trackVertexCursor;
                trackVertexCursor += quadBuckets[envBucketIndex].count * sizeof(D3DTRIANGLE);
                memcpy(mappedBatches[envBucketIndex].tris, quadBuckets[envBucketIndex].triangles, quadBuckets[envBucketIndex].count * sizeof(D3DTRIANGLE));
            } else {
                mappedBatches[envBucketIndex].count = 0;
                mappedBatches[envBucketIndex].tris = 0;
            }
        }
        AddRenderSkeleton(object->skeleton, (PCVertex *)out, outputCount, mappedBatches, 0, 0, 0);
    }
}
#else
void DrawEnvMap(Object *object, SVECTOR *normals, int x, int y, float scale)
{
    VECTOR reflect;
    int dot;
    VECTOR translation;
    short vertex0;
    short vertex1;
    short vertex2;
    short vertex3;
    unsigned char uOffset = 0;
    int u0;
    int u1;
    int u2;
    int index;
    int u3;
    int u;
    int vertical;
    int set;
    MATRIX matrix;
    TriList *lists;
    unsigned char *out;
    unsigned char *cursor;
    short vertexCount;
    short primitiveCount;
    short primitivesLeft;
    short i;
    short j;
    Poly primitive;
    long transformFlag;
    int original0;
    int original1;
    int original2;
    int original3;
    SVECTOR *vertices;

    if (!texture16Bit && !rgbTextures) {
        uOffset = 64;
        if (awardedMedal == 1) uOffset = 0;
        else if (awardedMedal == 3) uOffset = 128;
    }
    index = 0;
    j = 0;
    do {
        quadBuckets[j].count = 0;
    } while (++j <= normalSetNo);
    SetSkeletonTransform(object->skeleton);
    vertexCount = object->vertexCount;
    vertices = object->vertices;
    primitiveCount = object->primitiveCount;
    RotTransPersN(vertices, StaticScreen, StaticDepth, StaticFlags, vertexCount);
    translation.vx = 0;
    translation.vy = 0;
    translation.vz = 0;
    TransMatrix(&matrix, &translation);
    SetTransMatrix(&matrix);
    for (i = 0; i < vertexCount; i++)
        RotTrans(&normals[i], &StaticRVert[i], &transformFlag);
    for (i = 0; i < vertexCount; i++) {
        dot = StaticRVert[i].vz & ~63;
        reflect.vx = (dot * StaticRVert[i].vx) >> 11;
        reflect.vy = (dot * StaticRVert[i].vy) >> 11;
        reflect.vz = ((dot * StaticRVert[i].vz) >> 11) - 4096;
        envMapU[i] = (ratan2(reflect.vx, reflect.vz) >> 6) & 63;
        vertical = 31 - ((reflect.vy << 6) >> 13);
        if (vertical < 0) vertical = 0;
        if (vertical > 63) vertical = 63;
        envMapV[i] = vertical;
    }
    primitive.prim = object->primitives;
    if (primitiveCount > 0) {
        primitivesLeft = primitiveCount;
        do {
            switch (primitive.prim->type) {
            case TypeF3:
                vertex0 = primitive.f3->coords[0];
                vertex1 = primitive.f3->coords[1];
                vertex2 = primitive.f3->coords[2];
                ENV_SET();
                original0 = envMapU[vertex0];
                original1 = envMapU[vertex1];
                original2 = envMapU[vertex2];
                u0 = original0;
                u1 = original1;
                u2 = original2;
                ENV_EDGE(vertex0, vertex1, original0, original1, u0, u1);
                ENV_EDGE(vertex1, vertex2, original1, original2, u1, u2);
                ENV_EDGE(vertex0, vertex2, original0, original2, u0, u2);
                ENV_VERTEX(0, vertex0, u0);
                ENV_VERTEX(1, vertex1, u1);
                ENV_VERTEX(2, vertex2, u2);
                quadBuckets[set].triangles[quadBuckets[set].count].v1 = index;
                quadBuckets[set].triangles[quadBuckets[set].count].v2 = index + 1;
                quadBuckets[set].triangles[quadBuckets[set].count].v3 = index + 2;
                quadBuckets[set].count++;
                primitive.f3++;
                index += 3;
                break;
            case TypeF4:
                vertex0 = primitive.f4->coords[0];
                vertex1 = primitive.f4->coords[1];
                vertex2 = primitive.f4->coords[2];
                vertex3 = primitive.f4->coords[3];
                ENV_SET();
                original0 = envMapU[vertex0];
                original1 = envMapU[vertex1];
                original2 = envMapU[vertex2];
                original3 = envMapU[vertex3];
                u0 = original0;
                u1 = original1;
                u2 = original2;
                u3 = original3;
                ENV_EDGE(vertex0, vertex1, original0, original1, u0, u1);
                ENV_EDGE(vertex1, vertex2, original1, original2, u1, u2);
                ENV_EDGE(vertex2, vertex3, original2, original3, u2, u3);
                ENV_EDGE(vertex0, vertex3, original0, original3, u0, u3);
                ENV_VERTEX(0, vertex0, u0);
                ENV_VERTEX(1, vertex1, u1);
                ENV_VERTEX(2, vertex2, u2);
                ENV_VERTEX(3, vertex3, u3);
                quadBuckets[set].triangles[quadBuckets[set].count].v1 = index;
                quadBuckets[set].triangles[quadBuckets[set].count].v2 = index + 1;
                quadBuckets[set].triangles[quadBuckets[set].count].v3 = index + 3;
                quadBuckets[set].count++;
                quadBuckets[set].triangles[quadBuckets[set].count].v1 = index;
                quadBuckets[set].triangles[quadBuckets[set].count].v2 = index + 3;
                quadBuckets[set].triangles[quadBuckets[set].count].v3 = index + 2;
                quadBuckets[set].count++;
                primitive.f4++;
                index += 4;
                break;
            }
        } while (--primitivesLeft);
    }
    if (index) {
        out = trackVertexCursor;
        memcpy(out, trailStage, index * sizeof(PCVertex));
        trackVertexCursor = out + index * sizeof(PCVertex);
        lists = (TriList *)trackVertexCursor;
        trackVertexCursor += 23 * sizeof(TriList);
        cursor = trackVertexCursor;
        j = 0;
        do {
            if (quadBuckets[j].count) {
                lists[j].count = quadBuckets[j].count;
                lists[j].tris = (D3DTRIANGLE *)cursor;
                cursor += quadBuckets[j].count * sizeof(D3DTRIANGLE);
                memcpy(lists[j].tris, quadBuckets[j].triangles, quadBuckets[j].count * sizeof(D3DTRIANGLE));
            } else {
                lists[j].count = 0;
                lists[j].tris = 0;
            }
            trackVertexCursor = cursor;
        } while (++j <= normalSetNo);
        AddRenderSkeleton(object->skeleton, (PCVertex *)out, index, lists, 0, 0, 0);
    }
}
#endif
#undef ENV_EDGE
#undef ENV_VERTEX
#undef ENV_SET
#endif
