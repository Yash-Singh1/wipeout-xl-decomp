#include "effects.h"
#include "race_globals.h"
#include <string.h>
#include "weapon.h"
#include "pc_math.h"
#include "pc_projection.h"
#ifdef NETPLAY
/* NET-WOXL expands strcpy and strcat inline; the PC build calls them. */
#pragma intrinsic(strcpy, strcat, abs, sin)
#endif

extern SParticle *particleSlots;
extern int cameraDroidHeight[];

extern u_short waterfallBuf[16];
extern short advertAngle;
extern short fanAngle;
extern short radarAngle;
extern int shakeAmount;

#ifdef NETPLAY
void AddEffects(WeaponData *weapons, ShipData *ships)
{
    /* The original resets this counter without reading it. */
    short effectCount = 0;
    int i;
    int j;

    for (i = 0; i < 15; i++) {
        if ((ships[i].attr & ELECTROED) && !(ships[i].combatAttr & DESTROYED)) {
            for (j = 0; j < 2; j++) {
                AddElectric(&ships[i].ppivot, 0);
                AddElectric(&ships[i].ppivot, 1);
            }
        }
    }
    for (i = 0; i < 20; i++) {
        if (weapons[i].type == 5 && weapons[i].update == UpdateElecBolt && weapons[i].count > 0) {
            AddEboltParticles(particleSlots, 8, &weapons[i].pos);
            for (j = 0; j < 3; j++) {
                AddElectric(&weapons[i].pos, 0);
                AddElectric(&weapons[i].pos, 1);
            }
        }
        effectCount = 0;
        if (weapons[i].count == -1 || weapons[i].count == -2) {
            if (weapons[i].count == -1)
                AddRing(weapons[i].pos.vx, weapons[i].pos.vy, weapons[i].pos.vz, 12);
            if (weapons[i].type == 13)
                AddMinigunExplode(weapons[i].pos.vx, weapons[i].pos.vy, weapons[i].pos.vz, -1);
            else
                AddExplode(weapons[i].pos.vx, weapons[i].pos.vy, weapons[i].pos.vz, -1);
            weapons[i].count = 0;
        }
    }
}
#else
void AddEffects(WeaponData *weapons, ShipData *ships)
{
    int i;
    int j;

    for (i = 0; i < 15; i++) {
        if ((ships[i].attr & ELECTROED) && !(ships[i].combatAttr & DESTROYED)) {
            for (j = 0; j < 2; j++) {
                AddElectric(&ships[i].ppivot, 0);
                AddElectric(&ships[i].ppivot, 1);
            }
        }
    }
    for (i = 0; i < 20; i++) {
        if (weapons[i].type == 5 && weapons[i].update == UpdateElecBolt && weapons[i].count > 0) {
            AddEboltParticles(particleSlots, 8, &weapons[i].pos);
            for (j = 0; j < 3; j++) {
                AddElectric(&weapons[i].pos, 0);
                AddElectric(&weapons[i].pos, 1);
            }
        }
        if (weapons[i].count == -1 || weapons[i].count == -2) {
            if (weapons[i].count == -1)
                AddRing(weapons[i].pos.vx, weapons[i].pos.vy, weapons[i].pos.vz, 12);
            if (weapons[i].type == 13)
                AddMinigunExplode(weapons[i].pos.vx, weapons[i].pos.vy, weapons[i].pos.vz, -1);
            else
                AddExplode(weapons[i].pos.vx, weapons[i].pos.vy, weapons[i].pos.vz, -1);
            weapons[i].count = 0;
        }
    }
}
#endif

#ifdef NETPLAY
void DrawEffects(WeaponData *weapons, ShipData *ships)
{
    /* The original resets this counter without reading it. */
    short effectCount = 0;
    int i;
    int j;

    for (i = 0; i < 15; i++) {
        if ((ships[i].attr & ELECTROED) && !(ships[i].combatAttr & DESTROYED)) {
            for (j = 0; j < 2; j++) {
                AddElectricStreak(&ships[i].ppivot, 0);
                AddElectricStreak(&ships[i].ppivot, 1);
            }
        }
    }
    for (i = 0; i < 20; i++) {
        if (weapons[i].type == 5 && weapons[i].update == UpdateElecBolt && weapons[i].count > 0) {
            for (j = 0; j < 3; j++) {
                AddElectricStreak(&weapons[i].pos, 0);
                AddElectricStreak(&weapons[i].pos, 1);
            }
        }
        if (weapons[i].type == 2 && weapons[i].update == UpdateMissile && weapons[i].count > 0)
            DrawMissileFlare(weapons[i].pos.vx, weapons[i].pos.vy, weapons[i].pos.vz);
        if (weapons[i].type == 1 && weapons[i].update == UpdateRocket && weapons[i].count > 0)
            DrawRocketFlare(weapons[i].pos.vx, weapons[i].pos.vy, weapons[i].pos.vz);
    }
}
#else
void DrawEffects(WeaponData *weapons, ShipData *ships)
{
    int i;
    int j;
    int k;

    for (i = 0; i < 15; i++) {
        if ((ships[i].attr & ELECTROED) && !(ships[i].combatAttr & DESTROYED)) {
            for (j = 0; j < 2; j++) {
                AddElectricStreak(&ships[i].ppivot, 0);
                AddElectricStreak(&ships[i].ppivot, 1);
            }
        }
    }
    for (i = 0; i < 20; i++) {
        if (weapons[i].type == 5 && weapons[i].update == UpdateElecBolt && weapons[i].count > 0) {
            for (k = 0; k < 3; k++) {
                AddElectricStreak(&weapons[i].pos, 0);
                AddElectricStreak(&weapons[i].pos, 1);
            }
        }
        if (weapons[i].type == 2 && weapons[i].update == UpdateMissile && weapons[i].count > 0)
            DrawMissileFlare(weapons[i].pos.vx, weapons[i].pos.vy, weapons[i].pos.vz);
        if (weapons[i].type == 1 && weapons[i].update == UpdateRocket && weapons[i].count > 0)
            DrawRocketFlare(weapons[i].pos.vx, weapons[i].pos.vy, weapons[i].pos.vz);
    }
}
#endif

void ResetStartBoom(void)
{
}

#include "pc_vertex.h"
extern int trainLightSeed;
extern int neonFrame;

void InitTrainLights(Object **unusedObjects, short unusedCount, Object **objects, int mode)
{
    int i;
    int j;
    int colour;
    int vertex;
    Poly poly;
    PCVertex *vertices;

    colour = 0;
    for (i = 0; i < 6; i++) {
        poly.prim = objects[i]->primitives;
        vertices = (PCVertex *)objects[i]->bspTree;
        vertex = 0;
        for (j = 0; j < 12; j++) {
            switch (poly.prim->type) {
            case TypeG3:
                vertices[vertex].b = vertices[vertex].g = vertices[vertex].r = 255;
                vertices[vertex + 1].b = vertices[vertex + 1].g = vertices[vertex + 1].r = 255;
                vertices[vertex + 2].b = vertices[vertex + 2].g = vertices[vertex + 2].r = 255;
                poly.g3++;
                vertex += 3;
                break;
            case TypeG4:
                vertices[vertex].b = vertices[vertex].g = vertices[vertex].r = colour;
                vertices[vertex + 1].b = vertices[vertex + 1].g = vertices[vertex + 1].r = colour - 50;
                vertices[vertex + 2].b = vertices[vertex + 2].g = vertices[vertex + 2].r = colour - 50;
                vertices[vertex + 3].b = vertices[vertex + 3].g = vertices[vertex + 3].r = colour;
                poly.g4++;
                vertex += 4;
                break;
            }
        }
        if (mode == -1) {
            trainLightSeed += newRand() % 5;
            if (trainLightSeed > 128)
                trainLightSeed = 0;
        }
        colour = 255 - trainLightSeed;
    }
}

#ifdef EQUIV
/* Loop is indexed rather than pointer-stepped; register choice differs. */
#ifdef NETPLAY
void DoNeonSign(Object **objects, short count)
{
    int i;
    int primitiveType;
    PCVertex *vertices;
    Primitive *primitive;
    Object **obj;

    for (i = 7; i < count + 7; i++) {
        obj = objects + i;
        primitive = obj[0]->primitives;
        vertices = (PCVertex *)obj[0]->floatVerts;
        primitiveType = primitive->type;
        switch (primitiveType) {
        case TypeGT4:
            if (neonFrame % 30 < 15) {
                vertices[0].b = vertices[0].g = vertices[0].r = 255;
                vertices[1].b = vertices[1].g = vertices[1].r = 255;
                vertices[2].b = vertices[2].g = vertices[2].r = 255;
                vertices[3].b = vertices[3].g = vertices[3].r = 255;
            } else {
                vertices[0].b = vertices[0].g = vertices[0].r = 128;
                vertices[1].b = vertices[1].g = vertices[1].r = 128;
                vertices[2].b = vertices[2].g = vertices[2].r = 128;
                vertices[3].b = vertices[3].g = vertices[3].r = 128;
            }
            break;
        }
    }
    neonFrame++;
}
#else
void DoNeonSign(Object **objects, short count)
{
    int i;
    int primitiveType;
    PCVertex *vertices;
    Primitive *primitive;
    Object **obj;

    for (i = 7; i < count + 7; i++) {
        obj = objects + i;
        primitive = obj[0]->primitives;
        vertices = (PCVertex *)obj[0]->floatVerts;
        primitiveType = primitive->type;
        switch (primitiveType) {
        case TypeGT4:
            if (neonFrame % 30 < 15) {
                vertices[0].b = vertices[0].g = vertices[0].r = 255;
                vertices[1].b = vertices[1].g = vertices[1].r = 255;
                vertices[2].b = vertices[2].g = vertices[2].r = 255;
                vertices[3].b = vertices[3].g = vertices[3].r = 255;
            } else {
                vertices[0].b = vertices[0].g = vertices[0].r = 128;
                vertices[1].b = vertices[1].g = vertices[1].r = 128;
                vertices[2].b = vertices[2].g = vertices[2].r = 128;
                vertices[3].b = vertices[3].g = vertices[3].r = 128;
            }
            break;
        }
    }
    neonFrame++;
}
#endif
#endif

#include "d3d.h"
extern Texture *textureTable[];
extern const float textureCoordinateScale;

#ifdef NETPLAY
/* NET keeps the texture-page clamp switched on; the /Od build keeps the jump over the empty else. */
#define NET_CLAMP_TEXPAGE 1
void AnimateTexture(Object **objects, short objectCount, short period,
                    short firstTexture, short textureCount, CVECTOR colour,
                    int *frame, int *texCounter, short texStep)
{
    short texOffset = 0;
    Poly polygon;
    int objIdx;
    PCVertex *vertices;
    Texture *texData;
    int materialSet;

    if (*frame % period == 0)
        *texCounter += texStep;
    texOffset = *texCounter % textureCount;
    texData = textureTable[texOffset + firstTexture];
    if (NET_CLAMP_TEXPAGE) {
        int texPage = (((texData->textureY & 0x100) >> 4) + (texData->textureX >> 6)) & 31;
        if (texPage > 15 && texPage < 100)
            texPage -= 10;
        else if (texPage < 16)
            texPage -= 5;
        materialSet = texPage;
    } else {
    }
    if (materialSet > normalSetNo)
        materialSet = normalSetNo;
    for (objIdx = 0; objIdx < objectCount; objIdx++) {
        ((FT4 *)objects[objIdx]->primitives)->flag |= Translucent;
        polygon.prim = objects[objIdx]->primitives;
        vertices = (PCVertex *)objects[objIdx]->floatVerts;
        if (((PCObjectMesh *)objects[objIdx]->normals)->frame[8].count != 0) {
            objects[objIdx]->normalCount = -12345;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[8].count = 0;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[9].count = 0;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[10].count = 0;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[9].triangles = DAlloc(heap, 2 * sizeof(D3DTRIANGLE));
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[10].triangles = DAlloc(heap, 2 * sizeof(D3DTRIANGLE));
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[9].triangles[0].v1 = 0;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[9].triangles[0].v2 = 1;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[9].triangles[0].v3 = 3;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[9].triangles[1].v1 = 0;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[9].triangles[1].v2 = 3;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[9].triangles[1].v3 = 2;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[10].triangles[0].v1 = 0;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[10].triangles[0].v2 = 1;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[10].triangles[0].v3 = 3;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[10].triangles[1].v1 = 0;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[10].triangles[1].v2 = 3;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[10].triangles[1].v3 = 2;
        }
        if (materialSet == 9) {
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[9].count = 2;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[10].count = 0;
        } else {
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[9].count = 0;
            ((PCObjectMesh *)objects[objIdx]->normals)->frame[10].count = 2;
        }
        switch (polygon.prim->type) {
        case TypeGT4:
            vertices[0].u = (unsigned long)texData->u0 / 255.0f;
            vertices[0].v = (unsigned long)texData->v0 / 255.0f;
            vertices[1].u = (unsigned long)texData->u1 / 255.0f;
            vertices[1].v = (unsigned long)texData->v1 / 255.0f;
            vertices[2].u = (unsigned long)texData->u2 / 255.0f;
            vertices[2].v = (unsigned long)texData->v2 / 255.0f;
            vertices[3].u = (unsigned long)texData->u3 / 255.0f;
            vertices[3].v = (unsigned long)texData->v3 / 255.0f;
            vertices[0].r = colour.r;
            vertices[0].g = colour.g;
            vertices[0].b = colour.b;
            vertices[1].r = colour.r;
            vertices[1].g = colour.g;
            vertices[1].b = colour.b;
            vertices[2].r = colour.r;
            vertices[2].g = colour.g;
            vertices[2].b = colour.b;
            vertices[3].r = colour.r;
            vertices[3].g = colour.g;
            vertices[3].b = colour.b;
            break;
        case TypeFT4:
            polygon.ft4->u0 = texData->u0;
            polygon.ft4->v0 = texData->v0;
            polygon.ft4->u1 = texData->u1;
            polygon.ft4->v1 = texData->v1;
            polygon.ft4->u2 = texData->u2;
            polygon.ft4->v2 = texData->v2;
            polygon.ft4->u3 = texData->u3;
            polygon.ft4->v3 = texData->v3;
            polygon.ft4->cba = texData->cba;
            polygon.ft4->tsb = texData->tsb;
            polygon.ft4++;
            break;
        }
    }
    (*frame)++;
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* The material clamp goes through a local index so the stack store follows the clamp as in the exe;
   the stack slot layout of the locals still differs (86%). */
void AnimateTexture(Object **objects, short objectCount, short period,
                    short firstTexture, short textureCount, CVECTOR colour,
                    int *frame, int *texCounter, short texStep)
{
    Texture *tex;
    PCVertex *vertices;
    int material;
    int i;
    Poly poly;

    if (*frame % period == 0)
        *texCounter += texStep;
    tex = textureTable[firstTexture + (short)(*texCounter % textureCount)];
    {
        int texturePage = (((short)(tex->textureY & 0xff0f) >> 4) + (tex->textureX >> 6)) & 31;
        if (texturePage > 15 && texturePage < 100)
            texturePage -= 10;
        else if (texturePage < 16)
            texturePage -= 5;
        material = texturePage;
    }
    if (material > normalSetNo)
        material = normalSetNo;
    for (i = 0; i < objectCount; i++, objects++) {
        poly.prim = objects[0]->primitives;
        vertices = (PCVertex *)objects[0]->floatVerts;
        poly.ft4->flag |= Translucent;
        if (((PCObjectMesh *)objects[0]->normals)->frame[8].count != 0) {
            objects[0]->normalCount = -12345;
            ((PCObjectMesh *)objects[0]->normals)->frame[8].count = 0;
            ((PCObjectMesh *)objects[0]->normals)->frame[9].count = 0;
            ((PCObjectMesh *)objects[0]->normals)->frame[10].count = 0;
            ((PCObjectMesh *)objects[0]->normals)->frame[9].triangles = DAlloc(heap, 2 * sizeof(D3DTRIANGLE));
            ((PCObjectMesh *)objects[0]->normals)->frame[10].triangles = DAlloc(heap, 2 * sizeof(D3DTRIANGLE));
            ((PCObjectMesh *)objects[0]->normals)->frame[9].triangles[0].v1 = 0;
            ((PCObjectMesh *)objects[0]->normals)->frame[9].triangles[0].v2 = 1;
            ((PCObjectMesh *)objects[0]->normals)->frame[9].triangles[0].v3 = 3;
            ((PCObjectMesh *)objects[0]->normals)->frame[9].triangles[1].v1 = 0;
            ((PCObjectMesh *)objects[0]->normals)->frame[9].triangles[1].v2 = 3;
            ((PCObjectMesh *)objects[0]->normals)->frame[9].triangles[1].v3 = 2;
            ((PCObjectMesh *)objects[0]->normals)->frame[10].triangles[0].v1 = 0;
            ((PCObjectMesh *)objects[0]->normals)->frame[10].triangles[0].v2 = 1;
            ((PCObjectMesh *)objects[0]->normals)->frame[10].triangles[0].v3 = 3;
            ((PCObjectMesh *)objects[0]->normals)->frame[10].triangles[1].v1 = 0;
            ((PCObjectMesh *)objects[0]->normals)->frame[10].triangles[1].v2 = 3;
            ((PCObjectMesh *)objects[0]->normals)->frame[10].triangles[1].v3 = 2;
        }
        if (material == 9) {
            ((PCObjectMesh *)objects[0]->normals)->frame[9].count = 2;
            ((PCObjectMesh *)objects[0]->normals)->frame[10].count = 0;
        } else {
            ((PCObjectMesh *)objects[0]->normals)->frame[9].count = 0;
            ((PCObjectMesh *)objects[0]->normals)->frame[10].count = 2;
        }
        switch (poly.prim->type) {
        case TypeFT4:
            poly.ft4->u0 = tex->u0;
            poly.ft4->v0 = tex->v0;
            poly.ft4->u1 = tex->u1;
            poly.ft4->v1 = tex->v1;
            poly.ft4->u2 = tex->u2;
            poly.ft4->v2 = tex->v2;
            poly.ft4->u3 = tex->u3;
            poly.ft4->v3 = tex->v3;
            poly.ft4->cba = tex->cba;
            poly.ft4->tsb = tex->tsb;
            break;
        case TypeGT4:
            vertices[0].u = (unsigned long)tex->u0 * textureCoordinateScale;
            vertices[0].v = (unsigned long)tex->v0 * textureCoordinateScale;
            vertices[1].u = (unsigned long)tex->u1 * textureCoordinateScale;
            vertices[1].v = (unsigned long)tex->v1 * textureCoordinateScale;
            vertices[2].u = (unsigned long)tex->u2 * textureCoordinateScale;
            vertices[2].v = (unsigned long)tex->v2 * textureCoordinateScale;
            vertices[3].u = (unsigned long)tex->u3 * textureCoordinateScale;
            vertices[3].v = (unsigned long)tex->v3 * textureCoordinateScale;
            vertices[0].r = colour.r;
            vertices[0].g = colour.g;
            vertices[0].b = colour.b;
            vertices[1].r = colour.r;
            vertices[1].g = colour.g;
            vertices[1].b = colour.b;
            vertices[2].r = colour.r;
            vertices[2].g = colour.g;
            vertices[2].b = colour.b;
            vertices[3].r = colour.r;
            vertices[3].g = colour.g;
            vertices[3].b = colour.b;
            break;
        }
    }
    (*frame)++;
}
#endif
#endif

void WaterFall(void)
{
    RECT rect;
    int count;
    u_short first;

    rect.x = 0x370;
    rect.y = 0xf1;
    rect.w = 16;
    rect.h = 1;
    first = waterfallBuf[15];
    for (count = 14; count >= 4; count--)
        waterfallBuf[count + 1] = waterfallBuf[count];
    waterfallBuf[4] = first;
    LoadImageStub(&rect, (u_long *)waterfallBuf);
}

void InitWaterFall(void)
{
    RECT rect;

    rect.x = 0x370;
    rect.y = 0xf1;
    rect.w = 16;
    rect.h = 1;
    StoreImage(&rect, (u_long *)waterfallBuf);
    DrawSync(0);
}

#ifdef NETPLAY
extern short droidLightPhase;
/* NET-WOXL: empty constant tests the NET source keeps (the exe jumps over an empty else at each one). */
#define NET_DROID_LIGHT_TEST 1

void ColourDroidLights(Object *droid)
{
    short r;
    short b;
    short greenFade;
    Poly cursor;
    short g;
    int vertex;
    int i;
    short blueFade;
    short redFade;
    PCVertex *pcVerts;

    if (NET_DROID_LIGHT_TEST) {
        droidLightPhase += 140;
    } else {
    }
    redFade = ((long)(sin(((double)droidLightPhase / 4096.0) * 6.28318530718) * 4096.0) >> 5) + 128;
    if (redFade > 255)
        redFade = 255;
    if (NET_DROID_LIGHT_TEST) {
    } else {
    }
    greenFade = ((long)(sin(((double)(droidLightPhase + 75) / 4096.0) * 6.28318530718) * 4096.0) >> 5) + 128;
    if (greenFade > 255)
        greenFade = 255;
    if (NET_DROID_LIGHT_TEST) {
    } else {
    }
    blueFade = ((long)(sin(((double)((droidLightPhase >> 1) + 32) / 4096.0) * 6.28318530718) * 4096.0) >> 5) + 128;
    if (blueFade > 255)
        blueFade = 255;
    cursor.prim = droid->primitives;
    pcVerts = (PCVertex *)droid->floatVerts;
    vertex = 0;
    for (i = 0; i < 11; i++) {
        if (i < 2) {
            r = 40;
            g = greenFade;
            b = 40;
        } else if (i < 6) {
            r = blueFade >> 1;
            b = blueFade;
            g = blueFade >> 1;
        } else {
            r = redFade;
            b = 40;
            g = 40;
        }
        switch (cursor.prim->type) {
        case TypeGT3:
            pcVerts[vertex].r = r;
            pcVerts[vertex].g = g;
            pcVerts[vertex].b = b;
            pcVerts[vertex + 1].r = r;
            pcVerts[vertex + 1].g = g;
            pcVerts[vertex + 1].b = b;
            pcVerts[vertex + 2].r = r;
            pcVerts[vertex + 2].g = g;
            pcVerts[vertex + 2].b = b;
            cursor.gt3++;
            vertex += 3;
            break;
        case TypeGT4:
            pcVerts[vertex].r = r;
            pcVerts[vertex].g = g;
            pcVerts[vertex].b = b;
            pcVerts[vertex + 1].r = r;
            pcVerts[vertex + 1].g = g;
            pcVerts[vertex + 1].b = b;
            pcVerts[vertex + 2].r = r;
            pcVerts[vertex + 2].g = g;
            pcVerts[vertex + 2].b = b;
            pcVerts[vertex + 3].r = 40;
            pcVerts[vertex + 3].g = 40;
            pcVerts[vertex + 3].b = 40;
            cursor.gt4++;
            vertex += 4;
            break;
        }
    }
}


#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Colour channel allocation differs. */
extern short droidLightPhase;
void ColourDroidLights(Object *droid)
{
    short r, g, b;
    short rf, gf, bf;
    short phase;
    int i, vertex;
    Poly poly;
    PCVertex *vertices;
    PCVertex *vert;

    phase = droidLightPhase + 140;
    rf = (rsin(phase) >> 5) + 128;
    if (rf > 255)
        rf = 255;
    gf = (rsin(phase + 75) >> 5) + 128;
    if (gf > 255)
        gf = 255;
    droidLightPhase = phase;
    bf = (rsin((phase >> 1) + 32) >> 5) + 128;
    if (bf > 255)
        bf = 255;
    poly.prim = droid->primitives;
    vertices = (PCVertex *)droid->floatVerts;
    vertex = 0;
    for (i = 0; i < 11; i++) {
        if (i < 2) {
            r = 40;
            g = gf;
            b = 40;
        } else if (i < 6) {
            r = bf >> 1;
            b = bf;
            g = bf >> 1;
        } else {
            r = rf;
            b = 40;
            g = 40;
        }
        switch (poly.prim->type) {
        case TypeGT3:
            vert = &vertices[vertex];
            vertex += 3;
            vert[0].r = r;
            vert[0].g = g;
            vert[0].b = b;
            vert[1].r = r;
            vert[1].g = g;
            vert[1].b = b;
            vert[2].r = r;
            vert[2].g = g;
            vert[2].b = b;
            poly.gt3++;
            break;
        case TypeGT4:
            vert = &vertices[vertex];
            vertex += 4;
            vert[0].r = r;
            vert[0].g = g;
            vert[0].b = b;
            vert[1].r = r;
            vert[1].g = g;
            vert[1].b = b;
            vert[2].r = r;
            vert[2].g = g;
            vert[2].b = b;
            vert[3].r = 40;
            vert[3].g = 40;
            vert[3].b = 40;
            poly.gt4++;
            break;
        }
    }
}


#endif
#endif

#ifdef NETPLAY
void InitObject(Object **prm, short prmCount, Object **found, char *name, short len, short *count)
{
    int i;
    Object *model;

    *count = 0;
    for (i = 0; i < prmCount; i++) {
        model = prm[i];
        while (model) {
            if (!strncmp(model->name, name, len)) {
                found[*count] = model;
                (*count)++;
            }
            model = model->next;
        }
    }
}
#else
void InitObject(Object **prm, short prmCount, Object **found, char *name, short len, short *count)
{
    int i;
    Object *object;

    *count = 0;
    for (i = 0; i < prmCount; i++) {
        object = prm[i];
        while (object) {
            if (!strncmp(object->name, name, len)) {
                found[*count] = object;
                (*count)++;
            }
            object = object->next;
        }
    }
}
#endif

#ifdef NETPLAY
void MoveAdvert(Object **objects, int count)
{
    Object *model;
    int i;

    for (i = 0; i < count; i++) {
        model = objects[i];
        SetSkeletonDirection(model->skeleton, 0, advertAngle, 0);
    }
    advertAngle += 20;
}
#else
void MoveAdvert(Object **objects, int count)
{
    int i;

    for (i = 0; i < count; i++)
        SetSkeletonDirection(objects[i]->skeleton, 0, advertAngle, 0);
    advertAngle += 20;
}
#endif

#ifdef NETPLAY
void MoveFan(Object **objects, int count)
{
    Object *model;
    int i;

    for (i = 0; i < count; i++) {
        model = objects[i];
        SetSkeletonDirection(model->skeleton, 0, 0, fanAngle);
    }
    fanAngle += 100;
}
#else
void MoveFan(Object **objects, int count)
{
    int i;

    for (i = 0; i < count; i++)
        SetSkeletonDirection(objects[i]->skeleton, 0, 0, fanAngle);
    fanAngle += 100;
}
#endif

#ifdef NETPLAY
void InitMonoData(Object **objects, short count, MonoRailData *mono)
{
    VECTOR last;
    VECTOR first;
    Object *marker;
    int i;
    int duration = 200;

    /* The original leaves object uninitialised when count is zero. */
    for (i = 0; i < count; i++) {
        marker = objects[i];
        if (!strncmp(marker->name, "pylon16", 7))
            break;
    }
    first.vx = marker->skeleton->relative.t[0];
    first.vy = marker->skeleton->relative.t[1];
    first.vz = marker->skeleton->relative.t[2];
    for (i = 0; i < count; i++) {
        marker = objects[i];
        if (!strncmp(marker->name, "pylon0", 6))
            break;
    }
    last.vx = marker->skeleton->relative.t[0];
    last.vy = marker->skeleton->relative.t[1];
    last.vz = marker->skeleton->relative.t[2];
    mono->totalX = last.vx - first.vx;
    mono->totalY = last.vy - first.vy;
    mono->totalZ = last.vz - first.vz;
    mono->stepX = mono->totalX / duration;
    mono->stepY = mono->totalY / duration;
    mono->stepZ = mono->totalZ / duration;
    mono->index = 0;
}
#else
void InitMonoData(Object **objects, short count, MonoRailData *mono)
{
    VECTOR first;
    VECTOR last;
    Object *object;
    int i;
    int j;
    int duration;

    /* The original leaves object uninitialised when count is zero. */
    for (i = 0; i < count; i++) {
        object = objects[i];
        if (!strncmp(object->name, "pylon16", 7))
            break;
    }
    first.vx = object->skeleton->relative.t[0];
    first.vy = object->skeleton->relative.t[1];
    first.vz = object->skeleton->relative.t[2];
    for (j = 0; j < count; j++) {
        object = objects[j];
        if (!strncmp(object->name, "pylon0", 6))
            break;
    }
    last.vx = object->skeleton->relative.t[0];
    last.vy = object->skeleton->relative.t[1];
    last.vz = object->skeleton->relative.t[2];
    duration = 200;
    mono->totalX = last.vx - first.vx;
    mono->totalY = last.vy - first.vy;
    mono->totalZ = last.vz - first.vz;
    mono->stepX = mono->totalX / duration;
    mono->stepY = mono->totalY / duration;
    mono->stepZ = mono->totalZ / duration;
    mono->index = 0;
}
#endif

void ResetAnimatedObjects(void)
{
    ResetMonoRail();
    ResetZeppelin();
}

#ifdef NETPLAY
void ResetMonoRail(void)
{
    Object *model;
    int i;

    for (i = 0; i < 6; i++) {
        model = monoRailObjs[i];
        model->skeleton->relative.t[0] = posMonoRail[i].vx;
        model->skeleton->relative.t[1] = posMonoRail[i].vy;
        model->skeleton->relative.t[2] = posMonoRail[i].vz;
    }
    monoRail.index = 0;
}
#else
void ResetMonoRail(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        Object *object = monoRailObjs[i];
        object->skeleton->relative.t[0] = posMonoRail[i].vx;
        object->skeleton->relative.t[1] = posMonoRail[i].vy;
        object->skeleton->relative.t[2] = posMonoRail[i].vz;
    }
    monoRail.index = 0;
}
#endif

void ResetZeppelin(void)
{
    zeppelinObj[0]->skeleton->relative.t[0] = zeppelinBase.vx;
    zeppelinObj[0]->skeleton->relative.t[1] = zeppelinBase.vy;
    zeppelinObj[0]->skeleton->relative.t[2] = zeppelinBase.vz;
}

extern char wobbleDir2[];
extern short TextureTableCount;
void FreePrm(Object *object);

#ifdef NETPLAY
void LoadTrain(Object **objects, char *name, Skeleton *parent, VECTOR *positions)
{
    int count;
    Object *part;
    short textureBase;
    int i;
    TIMlist *texList;

    textureBase = 0;
    count = 0;
    {
        char fileName[256];

        strcpy(fileName, wobbleDir2);
        strcat(fileName, name);
        strcat(fileName, ".cmp");
        textureBase = TextureTableCount;
        texList = LoadCompressedTextureSequence(fileName);
        LoadCmpFiles(texList);
        strcpy(fileName, wobbleDir2);
        strcat(fileName, name);
        strcat(fileName, ".prm");
        FreePrm(objects[0]);
        objects[0] = LoadPrm(fileName, textureBase);
    }
    part = objects[0];
    while (part != 0) {
        part->skeleton->super = parent;
        objects[count] = part;
        count++;
        part = part->next;
    }
    for (i = 0; i < count; i++) {
        positions[i].vx = objects[i]->skeleton->relative.t[0];
        positions[i].vy = objects[i]->skeleton->relative.t[1];
        positions[i].vz = objects[i]->skeleton->relative.t[2];
    }
}
#else
void LoadTrain(Object **objects, char *name, Skeleton *parent, VECTOR *positions)
{
    char fileName[256];
    short textureBase;
    Object *object;
    Object **nextObject;
    int count;
    int i;

    count = 0;
    strcpy(fileName, wobbleDir2);
    strcat(fileName, name);
    strcat(fileName, ".cmp");
    textureBase = TextureTableCount;
    LoadCmpFiles(LoadCompressedTextureSequence(fileName));
    strcpy(fileName, wobbleDir2);
    strcat(fileName, name);
    strcat(fileName, ".prm");
    FreePrm(objects[0]);
    objects[0] = LoadPrm(fileName, textureBase);
    object = objects[0];
    if (object != 0) {
        nextObject = objects;
        do {
            object->skeleton->super = parent;
            nextObject[0] = object;
            nextObject++;
            count++;
            object = object->next;
        } while (object != 0);
    }
    for (i = 0; i < count; i++) {
        positions[i].vx = objects[i]->skeleton->relative.t[0];
        positions[i].vy = objects[i]->skeleton->relative.t[1];
        positions[i].vz = objects[i]->skeleton->relative.t[2];
    }
}
#endif

extern RaceLine raceLines[];
extern short startDroidIndex;
extern TrackSection *droidStartSections[6];
extern TrackSection *droidPickSection;
extern signed char classLaps[];
extern short startLinePos[];
extern int droidBlinkTick;
extern int paletteDirty;
extern const float textureCoordinateScale;
extern Texture *textureTable[];
#include "d3d.h"

/* NET-WOXL: the locals follow the /Od frame layout (tools/odslots.py). The NET build divides the texture
   coordinates by 255.0f inline, stores vector.vy (unused by the heading), takes the loop index through
   explicit branches, and advances the primitive pointer after each case. */
#ifdef NETPLAY
void UpdateStartDroid(Object **shapes, ShipData *ships, DroidData *droids,
                      short textureBase, ConfigData *game, int player)
{
    TrackSection *previous;
    Poly polygon;
    Object *shapeObj;
    short texCell = 0;
    short raceDistance = 0;
    TrackSection *section;
    short i;
    short yaw;
    Texture *uvs;
    VECTOR vector;
    short gap = 0;
    short droidDist = 0;
    DroidTextureOrientation texMirror = {{1, 0, 0, 0, 1, 1, 0, 1}};
    PCVertex *vertexPtr;

    raceDistance = raceLines[ships[ownShip].nearSecNo].distance;
    for (i = 0; i < 6; i++) {
        if (droids[i].section != -1 && droids[i].bonusSeconds > 0) {
            droidDist = raceLines[droids[i].section].distance;
            gap = raceDistance - droidDist;
            if (gap < 0)
                gap += numTrackSections;
            if (gap > numTrackSections >> 1)
                gap -= numTrackSections;
            if (abs(gap) <= 160)
                break;
        }
    }
    startDroidIndex = i;
    if (i == 6)
        i = 1;
    else
        i = 0;
    while (i < 2) {
        if (i == 0 && droids[startDroidIndex].bonusSeconds == 0)
            i++;
        if (i == 0)
            section = droidStartSections[startDroidIndex];
        else
            section = droidPickSection;
        previous = section->prevSection.ptr;
        shapeObj = shapes[i];
        vector.vx = section->centre.vx - previous->centre.vx;
        vector.vy = section->centre.vy - previous->centre.vy;
        vector.vz = section->centre.vz - previous->centre.vz;
        yaw = ratan2(vector.vx, vector.vz);
        yaw = 4096 - yaw;
        SetSkeletonDirectionHPR(shapeObj->skeleton, (short)yaw, 0, 0);
        SetSkeletonPosition(shapeObj->skeleton, section->centre.vx,
                            section->centre.vy - 3000, section->centre.vz);
        if (i == 0) {
            if (ships[ownShip].lap == 0)
                texCell = 0;
            else if (ships[ownShip].lap == classLaps[game->shipClass] &&
                     droids[startDroidIndex].section == startLinePos[game->actTrackNum])
                texCell = 1;
            else
                texCell = 2;
        } else
            texCell = 3;
        uvs = textureTable[texCell + textureBase];
        polygon.prim = shapeObj->primitives;
        vertexPtr = (PCVertex *)shapeObj->floatVerts;
        if (((PCObjectMesh *)shapeObj->normals)->frame[12].count == 0) {
            ((PCObjectMesh *)shapeObj->normals)->frame[12].count = 2;
            ((PCObjectMesh *)shapeObj->normals)->frame[12].triangles = DAlloc(heap, 2 * sizeof(D3DTRIANGLE));
        }
        switch (polygon.prim->type) {

        case TypeGT4:
            if (rgbTextures) {
                if (droidBlinkTick % 20 < 10) {
                    vertexPtr[0].r = 255;
                    vertexPtr[0].g = 255;
                    vertexPtr[0].b = 255;
                    vertexPtr[1].r = 255;
                    vertexPtr[1].g = 255;
                    vertexPtr[1].b = 255;
                    vertexPtr[2].r = 255;
                    vertexPtr[2].g = 255;
                    vertexPtr[2].b = 255;
                    vertexPtr[3].r = 255;
                    vertexPtr[3].g = 255;
                    vertexPtr[3].b = 255;
                } else {
                    vertexPtr[0].r = 128;
                    vertexPtr[0].g = 128;
                    vertexPtr[0].b = 128;
                    vertexPtr[1].r = 128;
                    vertexPtr[1].g = 128;
                    vertexPtr[1].b = 128;
                    vertexPtr[2].r = 128;
                    vertexPtr[2].g = 128;
                    vertexPtr[2].b = 128;
                    vertexPtr[3].r = 128;
                    vertexPtr[3].g = 128;
                    vertexPtr[3].b = 128;
                }
            } else {
                if (droidBlinkTick % 20 < 10) {
                    droidBlinkColour[0] = droidPaletteColour[0] - 65;
                    droidBlinkColour[1] = droidPaletteColour[1] - 65;
                    droidBlinkColour[2] = droidPaletteColour[2] - 65;
                } else {
                    droidBlinkColour[0] = droidPaletteColour[0];
                    droidBlinkColour[1] = droidPaletteColour[1];
                    droidBlinkColour[2] = droidPaletteColour[2];
                }
                paletteDirty |= 1;
            }
            if (i == 0) {
                ((PCObjectMesh *)shapeObj->normals)->frame[12].triangles[0].v1 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[12].triangles[0].v2 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[12].triangles[0].v3 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[12].triangles[1].v1 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[12].triangles[1].v2 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[12].triangles[1].v3 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[COPIED_NORMAL_GROUP].triangles[0].v1 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[COPIED_NORMAL_GROUP].triangles[0].v2 = 1;
                ((PCObjectMesh *)shapeObj->normals)->frame[COPIED_NORMAL_GROUP].triangles[0].v3 = 3;
                ((PCObjectMesh *)shapeObj->normals)->frame[COPIED_NORMAL_GROUP].triangles[1].v1 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[COPIED_NORMAL_GROUP].triangles[1].v2 = 3;
                ((PCObjectMesh *)shapeObj->normals)->frame[COPIED_NORMAL_GROUP].triangles[1].v3 = 2;
                vertexPtr[0].u = (unsigned long)uvs->u0 / 255.0f;
                vertexPtr[0].v = (unsigned long)uvs->v0 / 255.0f;
                vertexPtr[1].u = (unsigned long)uvs->u1 / 255.0f;
                vertexPtr[1].v = (unsigned long)uvs->v1 / 255.0f;
                vertexPtr[2].u = (unsigned long)uvs->u2 / 255.0f;
                vertexPtr[2].v = (unsigned long)uvs->v2 / 255.0f;
                vertexPtr[3].u = (unsigned long)uvs->u3 / 255.0f;
                vertexPtr[3].v = (unsigned long)uvs->v3 / 255.0f;
            } else {
                ((PCObjectMesh *)shapeObj->normals)->frame[COPIED_NORMAL_GROUP].triangles[0].v1 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[COPIED_NORMAL_GROUP].triangles[0].v2 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[COPIED_NORMAL_GROUP].triangles[0].v3 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[COPIED_NORMAL_GROUP].triangles[1].v1 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[COPIED_NORMAL_GROUP].triangles[1].v2 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[COPIED_NORMAL_GROUP].triangles[1].v3 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[12].triangles[0].v1 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[12].triangles[0].v2 = 1;
                ((PCObjectMesh *)shapeObj->normals)->frame[12].triangles[0].v3 = 3;
                ((PCObjectMesh *)shapeObj->normals)->frame[12].triangles[1].v1 = 0;
                ((PCObjectMesh *)shapeObj->normals)->frame[12].triangles[1].v2 = 3;
                ((PCObjectMesh *)shapeObj->normals)->frame[12].triangles[1].v3 = 2;
                if (texMirror.unmirrored[game->trackNum] == 0) {
                    vertexPtr[0].u = (unsigned long)uvs->u1 / 255.0f;
                    vertexPtr[0].v = (unsigned long)uvs->v0 / 255.0f;
                    vertexPtr[1].u = (unsigned long)uvs->u0 / 255.0f;
                    vertexPtr[1].v = (unsigned long)uvs->v1 / 255.0f;
                    vertexPtr[2].u = (unsigned long)uvs->u3 / 255.0f;
                    vertexPtr[2].v = (unsigned long)uvs->v2 / 255.0f;
                    vertexPtr[3].u = (unsigned long)uvs->u2 / 255.0f;
                    vertexPtr[3].v = (unsigned long)uvs->v3 / 255.0f;
                } else {
                    vertexPtr[0].u = (unsigned long)uvs->u0 / 255.0f;
                    vertexPtr[0].v = (unsigned long)uvs->v0 / 255.0f;
                    vertexPtr[1].u = (unsigned long)uvs->u1 / 255.0f;
                    vertexPtr[1].v = (unsigned long)uvs->v1 / 255.0f;
                    vertexPtr[2].u = (unsigned long)uvs->u2 / 255.0f;
                    vertexPtr[2].v = (unsigned long)uvs->v2 / 255.0f;
                    vertexPtr[3].u = (unsigned long)uvs->u3 / 255.0f;
                    vertexPtr[3].v = (unsigned long)uvs->v3 / 255.0f;
                }
            }
            polygon.gt4++;
            break;
        case TypeFT4:
            if (rgbTextures) {
                if (droidBlinkTick % 20 < 10) {
                    vertexPtr[0].r = 255;
                    vertexPtr[0].g = 255;
                    vertexPtr[0].b = 255;
                    vertexPtr[1].r = 255;
                    vertexPtr[1].g = 255;
                    vertexPtr[1].b = 255;
                    vertexPtr[2].r = 255;
                    vertexPtr[2].g = 255;
                    vertexPtr[2].b = 255;
                    vertexPtr[3].r = 255;
                    vertexPtr[3].g = 255;
                    vertexPtr[3].b = 255;
                } else {
                    vertexPtr[0].r = 128;
                    vertexPtr[0].g = 128;
                    vertexPtr[0].b = 128;
                    vertexPtr[1].r = 128;
                    vertexPtr[1].g = 128;
                    vertexPtr[1].b = 128;
                    vertexPtr[2].r = 128;
                    vertexPtr[2].g = 128;
                    vertexPtr[2].b = 128;
                    vertexPtr[3].r = 128;
                    vertexPtr[3].g = 128;
                    vertexPtr[3].b = 128;
                }
            } else {
                if (droidBlinkTick % 20 < 10) {
                    droidBlinkColour[0] = droidPaletteColour[0] - 65;
                    droidBlinkColour[1] = droidPaletteColour[1] - 65;
                    droidBlinkColour[2] = droidPaletteColour[2] - 65;
                } else {
                    droidBlinkColour[0] = droidPaletteColour[0];
                    droidBlinkColour[1] = droidPaletteColour[1];
                    droidBlinkColour[2] = droidPaletteColour[2];
                }
                paletteDirty |= 1;
            }
            vertexPtr[0].u = (unsigned long)uvs->u0 / 255.0f;
            vertexPtr[0].v = (unsigned long)uvs->v0 / 255.0f;
            vertexPtr[1].u = (unsigned long)uvs->u1 / 255.0f;
            vertexPtr[1].v = (unsigned long)uvs->v1 / 255.0f;
            vertexPtr[2].u = (unsigned long)uvs->u2 / 255.0f;
            vertexPtr[2].v = (unsigned long)uvs->v2 / 255.0f;
            vertexPtr[3].u = (unsigned long)uvs->u3 / 255.0f;
            vertexPtr[3].v = (unsigned long)uvs->v3 / 255.0f;
            polygon.ft4++;
            break;
        }
        i++;
    }
    if (player == -1)
        droidBlinkTick++;
}
#else
void UpdateStartDroid(Object **shapes, ShipData *ships, DroidData *droids,
                      short textureBase, ConfigData *game, int player)
{
    VECTOR direction;
    DroidTextureOrientation textureOrientation = {{1, 0, 0, 0, 1, 1, 0, 1}};
    short texture;
    short i;
    short ownSection;
    short distance;
    short angle;
    TrackSection *section;
    TrackSection *previous;
    Texture *tex;
    Object *shape;
    Poly poly;
    PCVertex *vertices;

    ownSection = raceLines[ships[ownShip].nearSecNo].distance;
    for (i = 0; i < 6; i++) {
        if (droids[i].section != -1 && droids[i].bonusSeconds > 0) {
            distance = ownSection - raceLines[droids[i].section].distance;
            if (distance < 0)
                distance += numTrackSections;
            if (distance > numTrackSections >> 1)
                distance -= numTrackSections;
            if (abs(distance) <= 160)
                break;
        }
    }
    startDroidIndex = i;
    for (i = (startDroidIndex == 6); i < 2; i++) {
        if (i == 0 && droids[startDroidIndex].bonusSeconds == 0)
            i++;
        if (i == 0)
            section = droidStartSections[startDroidIndex];
        else
            section = droidPickSection;
        previous = section->prevSection.ptr;
        shape = shapes[i];
        direction.vx = section->centre.vx - previous->centre.vx;
        direction.vz = section->centre.vz - previous->centre.vz;
        angle = ratan2(direction.vx, direction.vz);
        angle = 4096 - angle;
        SetSkeletonDirectionHPR(shape->skeleton, (short)angle, 0, 0);
        SetSkeletonPosition(shape->skeleton, section->centre.vx,
                            section->centre.vy - 3000, section->centre.vz);
        if (i == 0) {
            if (ships[ownShip].lap == 0)
                texture = 0;
            else if (ships[ownShip].lap == classLaps[game->shipClass] &&
                     droids[startDroidIndex].section == startLinePos[game->actTrackNum])
                texture = 1;
            else
                texture = 2;
        } else
            texture = 3;
        tex = textureTable[texture + textureBase];
        vertices = (PCVertex *)shape->floatVerts;
        poly.prim = shape->primitives;
        if (((PCObjectMesh *)shape->normals)->frame[12].count == 0) {
            ((PCObjectMesh *)shape->normals)->frame[12].count = 2;
            ((PCObjectMesh *)shape->normals)->frame[12].triangles = DAlloc(heap, 2 * sizeof(D3DTRIANGLE));
        }
        switch (poly.prim->type) {

        case TypeFT4:
            if (rgbTextures) {
                if (droidBlinkTick % 20 < 10) {
                    vertices[0].r = 255;
                    vertices[0].g = 255;
                    vertices[0].b = 255;
                    vertices[1].r = 255;
                    vertices[1].g = 255;
                    vertices[1].b = 255;
                    vertices[2].r = 255;
                    vertices[2].g = 255;
                    vertices[2].b = 255;
                    vertices[3].r = 255;
                    vertices[3].g = 255;
                    vertices[3].b = 255;
                } else {
                    vertices[0].r = 128;
                    vertices[0].g = 128;
                    vertices[0].b = 128;
                    vertices[1].r = 128;
                    vertices[1].g = 128;
                    vertices[1].b = 128;
                    vertices[2].r = 128;
                    vertices[2].g = 128;
                    vertices[2].b = 128;
                    vertices[3].r = 128;
                    vertices[3].g = 128;
                    vertices[3].b = 128;
                }
            } else {
                if (droidBlinkTick % 20 < 10) {
                    droidBlinkColour[0] = droidPaletteColour[0] - 65;
                    droidBlinkColour[1] = droidPaletteColour[1] - 65;
                    droidBlinkColour[2] = droidPaletteColour[2] - 65;
                } else {
                    droidBlinkColour[0] = droidPaletteColour[0];
                    droidBlinkColour[1] = droidPaletteColour[1];
                    droidBlinkColour[2] = droidPaletteColour[2];
                }
                paletteDirty |= 1;
            }
            vertices[0].u = (unsigned long)tex->u0 * textureCoordinateScale;
            vertices[0].v = (unsigned long)tex->v0 * textureCoordinateScale;
            vertices[1].u = (unsigned long)tex->u1 * textureCoordinateScale;
            vertices[1].v = (unsigned long)tex->v1 * textureCoordinateScale;
            vertices[2].u = (unsigned long)tex->u2 * textureCoordinateScale;
            vertices[2].v = (unsigned long)tex->v2 * textureCoordinateScale;
            vertices[3].u = (unsigned long)tex->u3 * textureCoordinateScale;
            vertices[3].v = (unsigned long)tex->v3 * textureCoordinateScale;
            break;
        case TypeGT4:
            if (rgbTextures) {
                if (droidBlinkTick % 20 < 10) {
                    vertices[0].r = 255;
                    vertices[0].g = 255;
                    vertices[0].b = 255;
                    vertices[1].r = 255;
                    vertices[1].g = 255;
                    vertices[1].b = 255;
                    vertices[2].r = 255;
                    vertices[2].g = 255;
                    vertices[2].b = 255;
                    vertices[3].r = 255;
                    vertices[3].g = 255;
                    vertices[3].b = 255;
                } else {
                    vertices[0].r = 128;
                    vertices[0].g = 128;
                    vertices[0].b = 128;
                    vertices[1].r = 128;
                    vertices[1].g = 128;
                    vertices[1].b = 128;
                    vertices[2].r = 128;
                    vertices[2].g = 128;
                    vertices[2].b = 128;
                    vertices[3].r = 128;
                    vertices[3].g = 128;
                    vertices[3].b = 128;
                }
            } else {
                if (droidBlinkTick % 20 < 10) {
                    droidBlinkColour[0] = droidPaletteColour[0] - 65;
                    droidBlinkColour[1] = droidPaletteColour[1] - 65;
                    droidBlinkColour[2] = droidPaletteColour[2] - 65;
                } else {
                    droidBlinkColour[0] = droidPaletteColour[0];
                    droidBlinkColour[1] = droidPaletteColour[1];
                    droidBlinkColour[2] = droidPaletteColour[2];
                }
                paletteDirty |= 1;
            }
            if (i == 0) {
                ((PCObjectMesh *)shape->normals)->frame[12].triangles[0].v1 = 0;
                ((PCObjectMesh *)shape->normals)->frame[12].triangles[0].v2 = 0;
                ((PCObjectMesh *)shape->normals)->frame[12].triangles[0].v3 = 0;
                ((PCObjectMesh *)shape->normals)->frame[12].triangles[1].v1 = 0;
                ((PCObjectMesh *)shape->normals)->frame[12].triangles[1].v2 = 0;
                ((PCObjectMesh *)shape->normals)->frame[12].triangles[1].v3 = 0;
                ((PCObjectMesh *)shape->normals)->frame[COPIED_NORMAL_GROUP].triangles[0].v1 = 0;
                ((PCObjectMesh *)shape->normals)->frame[COPIED_NORMAL_GROUP].triangles[0].v2 = 1;
                ((PCObjectMesh *)shape->normals)->frame[COPIED_NORMAL_GROUP].triangles[0].v3 = 3;
                ((PCObjectMesh *)shape->normals)->frame[COPIED_NORMAL_GROUP].triangles[1].v1 = 0;
                ((PCObjectMesh *)shape->normals)->frame[COPIED_NORMAL_GROUP].triangles[1].v2 = 3;
                ((PCObjectMesh *)shape->normals)->frame[COPIED_NORMAL_GROUP].triangles[1].v3 = 2;
                vertices[0].u = (unsigned long)tex->u0 * textureCoordinateScale;
                vertices[0].v = (unsigned long)tex->v0 * textureCoordinateScale;
                vertices[1].u = (unsigned long)tex->u1 * textureCoordinateScale;
                vertices[1].v = (unsigned long)tex->v1 * textureCoordinateScale;
                vertices[2].u = (unsigned long)tex->u2 * textureCoordinateScale;
                vertices[2].v = (unsigned long)tex->v2 * textureCoordinateScale;
                vertices[3].u = (unsigned long)tex->u3 * textureCoordinateScale;
                vertices[3].v = (unsigned long)tex->v3 * textureCoordinateScale;
            } else {
                struct D3DTRIANGLE *copiedTris;

                copiedTris = ((PCObjectMesh *)shape->normals)->frame[COPIED_NORMAL_GROUP].triangles;
                ((PCObjectMesh *)shape->normals)->frame[12].triangles[0].v2 = 1;
                ((PCObjectMesh *)shape->normals)->frame[12].triangles[0].v3 = 3;
                ((PCObjectMesh *)shape->normals)->frame[12].triangles[1].v2 = 3;
                ((PCObjectMesh *)shape->normals)->frame[12].triangles[1].v3 = 2;
                copiedTris[0].v1 = 0;
                copiedTris[0].v2 = 0;
                copiedTris[0].v3 = 0;
                copiedTris[1].v1 = 0;
                copiedTris[1].v2 = 0;
                copiedTris[1].v3 = 0;
                ((PCObjectMesh *)shape->normals)->frame[12].triangles[0].v1 = 0;
                ((PCObjectMesh *)shape->normals)->frame[12].triangles[1].v1 = 0;
                if (textureOrientation.unmirrored[game->trackNum] == 0) {
                    vertices[0].u = (unsigned long)tex->u1 * textureCoordinateScale;
                    vertices[0].v = (unsigned long)tex->v0 * textureCoordinateScale;
                    vertices[1].u = (unsigned long)tex->u0 * textureCoordinateScale;
                    vertices[1].v = (unsigned long)tex->v1 * textureCoordinateScale;
                    vertices[2].u = (unsigned long)tex->u3 * textureCoordinateScale;
                    vertices[2].v = (unsigned long)tex->v2 * textureCoordinateScale;
                    vertices[3].u = (unsigned long)tex->u2 * textureCoordinateScale;
                    vertices[3].v = (unsigned long)tex->v3 * textureCoordinateScale;
                } else {
                    vertices[0].u = (unsigned long)tex->u0 * textureCoordinateScale;
                    vertices[0].v = (unsigned long)tex->v0 * textureCoordinateScale;
                    vertices[1].u = (unsigned long)tex->u1 * textureCoordinateScale;
                    vertices[1].v = (unsigned long)tex->v1 * textureCoordinateScale;
                    vertices[2].u = (unsigned long)tex->u2 * textureCoordinateScale;
                    vertices[2].v = (unsigned long)tex->v2 * textureCoordinateScale;
                    vertices[3].u = (unsigned long)tex->u3 * textureCoordinateScale;
                    vertices[3].v = (unsigned long)tex->v3 * textureCoordinateScale;
                }
            }
            break;
        }
    }
    if (player == -1)
        droidBlinkTick++;
}
#endif

#ifdef EQUIV
#ifdef NETPLAY
/* NET's /Od body initializes the loop index through two explicit branches. */
void DrawStartDroid(Object **objects, ShipData *ships, DroidData *droids)
{
    short i;
    short distance;
    short ownSection;
    short droidSection;
    Object *object;

    i = startDroidIndex;
    if (i == 6)
        i = 1;
    else
        i = 0;
    while (i < 2) {
        ownSection = raceLines[ships[ownShip].nearSecNo].distance;
        if (i == 0)
            droidSection = raceLines[droids[startDroidIndex].section].distance;
        else
            droidSection = raceLines[droids[6].section].distance;
        distance = ownSection - droidSection;
        if (distance < 0)
            distance += numTrackSections;
        if (distance > numTrackSections >> 1)
            distance -= numTrackSections;
        if (i == 0 && droids[startDroidIndex].bonusSeconds == 0)
            i++;
        object = objects[i];
        object->skeleton->update = 1;
        if (abs(distance) <= 160)
            AddRenderObject(object, 0, 0);
        i++;
    }
}
#endif
#endif
#ifdef NETPLAY
#ifndef EQUIV
/* NET's /Od body initializes the loop index through two explicit branches. */
void DrawStartDroid(Object **objects, ShipData *ships, DroidData *droids)
{
    short i;
    short distance;
    short ownSection;
    short droidSection;
    Object *object;

    i = startDroidIndex;
    if (i == 6)
        i = 1;
    else
        i = 0;
    while (i < 2) {
        ownSection = raceLines[ships[ownShip].nearSecNo].distance;
        if (i == 0)
            droidSection = raceLines[droids[startDroidIndex].section].distance;
        else
            droidSection = raceLines[droids[6].section].distance;
        distance = ownSection - droidSection;
        if (distance < 0)
            distance += numTrackSections;
        if (distance > numTrackSections >> 1)
            distance -= numTrackSections;
        if (i == 0 && droids[startDroidIndex].bonusSeconds == 0)
            i++;
        object = objects[i];
        object->skeleton->update = 1;
        if (abs(distance) <= 160)
            AddRenderObject(object, 0, 0);
        i++;
    }
}
#endif
#else
/* Start-grid droids are drawn only near the player's race-line position. */
void DrawStartDroid(Object **objects, ShipData *ships, DroidData *droids)
{
    short i;
    short distance;
    short ownSection;
    short droidSection;
    Object *object;

    i = startDroidIndex == 6;
    for (; i < 2; i++) {
        ownSection = raceLines[ships[ownShip].nearSecNo].distance;
        if (i == 0)
            droidSection = raceLines[droids[startDroidIndex].section].distance;
        else
            droidSection = raceLines[droids[6].section].distance;
        distance = ownSection - droidSection;
        if (distance < 0)
            distance += numTrackSections;
        if (distance > numTrackSections >> 1)
            distance -= numTrackSections;
        if (i == 0 && droids[startDroidIndex].bonusSeconds == 0)
            i++;
        object = objects[i];
        object->skeleton->update = 1;
        if (abs(distance) <= 160)
            AddRenderObject(object, 0, 0);
    }
}
#endif

extern char wobbleDir2[];
extern short TextureTableCount;
void FreePrm(Object *object);

#ifdef NETPLAY
void LoadStartDroid(Object **objects, char *name, Skeleton *parent)
{
    int count;
    Object *part;
    short textureBase;
    TIMlist *texList;
    char fullPath[256];

    textureBase = 0;
    count = 0;
    strcpy(fullPath, wobbleDir2);
    strcat(fullPath, name);
    strcat(fullPath, ".cmp");
    textureBase = TextureTableCount;
    texList = LoadCompressedTextureSequence(fullPath);
    LoadCmpFiles(texList);
    strcpy(fullPath, wobbleDir2);
    strcat(fullPath, name);
    strcat(fullPath, ".prm");
    FreePrm(objects[0]);
    objects[0] = LoadPrm(fullPath, textureBase);
    part = objects[0];
    while (part != 0) {
        part->skeleton->super = parent;
        objects[count] = part;
        count++;
        part = part->next;
    }
}
#else
void LoadStartDroid(Object **objects, char *name, Skeleton *parent)
{
    char fileName[256];
    short textureBase;
    Object *object;
    Object **nextObject;

    strcpy(fileName, wobbleDir2);
    strcat(fileName, name);
    strcat(fileName, ".cmp");
    textureBase = TextureTableCount;
    LoadCmpFiles(LoadCompressedTextureSequence(fileName));
    strcpy(fileName, wobbleDir2);
    strcat(fileName, name);
    strcat(fileName, ".prm");
    FreePrm(objects[0]);
    object = LoadPrm(fileName, textureBase);
    objects[0] = object;
    if (object != 0) {
        nextObject = objects;
        do {
            nextObject[0] = object;
            object->skeleton->super = parent;
            nextObject++;
            object = object->next;
        } while (object != 0);
    }
}
#endif

extern int lightUpdate;
extern unsigned char lightStartColour[4];
extern TrackSection *lightSections[3];
extern int gameType;
extern short monoRailSettle;

#ifdef NETPLAY
extern TrackSection *startDroidSection;
void InitDroidThings(Object **objects, int unused1, int unused2,
                     Checkpoint *points, ConfigData *options, ShipData *ships)
{
    int lightIndex;
    short count;
    short flag;
    short spread;
    TrackSection *current;
    short spacingStep;
    int i, j;
    short candidates[8] = {255, 298, 438, 352, 12, 350, 310, 64};

    flag = 0;
    count = 3;
    spread = 7;
    spacingStep = 7;
    lightIndex = 0;

    if (strncmp(objects[0]->name, "light", 5) == 0) {
        OutputDebugString("creating new instance of light beam \n");
        objects[1] = NewInstance(objects[0]);
        objects[2] = NewInstance(objects[0]);
        lightUpdate = 0;
        current = track->sections;
        for (i = 0; i < track->sectionCount; i++) {
            if (current->secNo == ships[ownShip].nearTrkSect->secNo) {
                startDroidSection = current;
                break;
            }
            current = current->nextSection.ptr;
        }
        if (gameType == 1 || gameType == 99) {
            for (i = 0; i < 3; i++)
                startDroidSection = startDroidSection->prevSection.ptr;
            count = 1;
        }
        if (options->actTrackNum == 6)
            spread = 3;
        for (i = 0; i < count; i++) {
            current = startDroidSection;
            for (j = 0; j < spread; j++)
                current = current->nextSection.ptr;
            lightSections[i] = current;
            spread += spacingStep;
        }
    } else {
        for (i = 0; i < 6; i++) {
            current = track->sections;
            for (j = 0; j < track->sectionCount; j++) {
                if (current->secNo == points[i].section) {
                    droidStartSections[i] = current;
                    break;
                }
                current = current->nextSection.ptr;
            }
        }
        current = track->sections;
        for (j = 0; j < track->sectionCount; j++) {
            if (current->secNo == candidates[options->trackNum]) {
                droidPickSection = current;
                points[6].section = current->secNo;
                break;
            }
            current = current->nextSection.ptr;
        }
    }
    if (strncmp(objects[0]->name, "light", 5) == 0) {
    }
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
extern TrackSection *startDroidSection;
/* The compiler removes the redundant startDroidSection store in the light loop. */
void InitDroidThings(Object **objects, int unused1, int unused2,
                     Checkpoint *points, ConfigData *options, ShipData *ships)
{
    short pickSections[8] = {255, 298, 438, 352, 12, 350, 310, 64};
    short count = 3;
    short spacing = 7;
    TrackSection *next;
    int i, j;
    TrackSection *section;

    if (strncmp(objects[0]->name, "light", 5) == 0) {
        OutputDebugString("creating new instance of light beam \n");
        objects[1] = NewInstance(objects[0]);
        objects[2] = NewInstance(objects[0]);
        lightUpdate = 0;
        section = startDroidSection;
        next = track->sections;
        for (i = 0; i < track->sectionCount; i++) {
            if (next->secNo == ships[ownShip].nearTrkSect->secNo) {
                section = next;
                break;
            }
            next = next->nextSection.ptr;
        }
        if (gameType == 1 || gameType == 99) {
            for (i = 0; i < 3; i++)
                section = section->prevSection.ptr;
            count = 1;
        }
        if (options->actTrackNum == 6)
            spacing = 3;
        startDroidSection = section;
        for (i = 0; i < count; i++) {
            next = section;
            for (j = 0; j < spacing; j++)
                next = next->nextSection.ptr;
            lightSections[i] = next;
            spacing += 7;
            startDroidSection = section;
        }
    } else {
        for (i = 0; i < 6; i++) {
            section = track->sections;
            for (j = 0; j < track->sectionCount; j++) {
                if (section->secNo == points[i].section) {
                    droidStartSections[i] = section;
                    break;
                }
                section = section->nextSection.ptr;
            }
        }
        section = track->sections;
        for (j = 0; j < track->sectionCount; j++) {
            if (section->secNo == pickSections[options->trackNum]) {
                points[6].section = section->secNo;
                droidPickSection = section;
                break;
            }
            section = section->nextSection.ptr;
        }
    }
    strncmp(objects[0]->name, "light", 5);
}
#endif
#endif

void ResetLightStart(void)
{
    short group;
    Poly prim;
    short count;
    short vertex;
    PCVertex *pcVerts;

    lightUpdate = 0;
    for (group = 0; group < 3; group++) {
        prim.ft4 = (FT4 *)lightGroups[group]->primitives;
        pcVerts = (PCVertex *)lightGroups[group]->floatVerts;
        vertex = 0;
        for (count = 0; count < 8; count++) {
            switch (prim.ft4->type) {
            case TypeFT4:
                if (rgbTextures) {
                    pcVerts[vertex + 0].r = 128;
                    pcVerts[vertex + 0].g = 128;
                    pcVerts[vertex + 0].b = 128;
                    pcVerts[vertex + 1].r = 128;
                    pcVerts[vertex + 1].g = 128;
                    pcVerts[vertex + 1].b = 128;
                    pcVerts[vertex + 2].r = 128;
                    pcVerts[vertex + 2].g = 128;
                    pcVerts[vertex + 2].b = 128;
                    pcVerts[vertex + 3].r = 128;
                    pcVerts[vertex + 3].g = 128;
                    pcVerts[vertex + 3].b = 128;
                } else {
                    lightStartColour[0] = 173;
                    lightStartColour[1] = 173;
                    lightStartColour[2] = 181;
                    paletteDirty |= 1;
                }
                prim.ft4++;
                vertex += 4;
                break;
            }
        }
    }
}

#ifdef NETPLAY
#include "start_lights.h"
extern int lightChaseStep, lightUpdateTick;
extern int startCountdown;
void UpdateLightStart(Object **objects, ShipData *unusedShips, CheckCell *unusedCells,
                      short unusedTextureBase, ConfigData *unusedConfig, int mode)
{
    TrackSection *section;
    short flashStep;
    Object *lightObj;
    short glowStep;
    short total;
    short chaseStart;
    Poly primitive;
    short lightStep;
    short i;
    TrackSection *prevLight;
    short chaseSlot;
    short j;
    short yaw;
    short flashCount;
    PCVertex *pcVerts;
    VECTOR dir;
    short base;

    chaseStart = 0;
    glowStep = 0;
    flashStep = 0;
    flashCount = 0;
    lightStep = 0;
    total = 3;
    chaseSlot = 0;
    if (gameType == 1 || gameType == 99)
        total = 1;
    for (i = 0; i < total; i++) {
        lightObj = objects[i];
        section = lightSections[i];
        prevLight = section->prevSection.ptr;
        dir.vx = section->centre.vx - prevLight->centre.vx;
        dir.vy = section->centre.vy - prevLight->centre.vy;
        dir.vz = section->centre.vz - prevLight->centre.vz;
        yaw = ratan2(dir.vx, dir.vz);
        yaw = 4096 - yaw;
        SetSkeletonDirectionHPR(lightObj->skeleton, yaw, 0, 0);
        SetSkeletonPosition(lightObj->skeleton, section->centre.vx,
                            section->centre.vy - 3000, section->centre.vz);
        if (mode == -1) {
            primitive.prim = lightObj->primitives;
            pcVerts = (PCVertex *)lightObj->floatVerts;
            base = 0;
            for (j = 0; j < 8; j++) {
                switch (primitive.prim->type) {
                case TypeFT4:
                    if (lightUpdate < 480) {
                        if (startCountdown <= 66 && startCountdown > 33) {
                            if (rgbTextures) {
                                pcVerts[base].r = 255;
                                pcVerts[base].g = 0;
                                pcVerts[base].b = 0;
                                pcVerts[base + 1].r = 255;
                                pcVerts[base + 1].g = 0;
                                pcVerts[base + 1].b = 0;
                                pcVerts[base + 2].r = 255;
                                pcVerts[base + 2].g = 0;
                                pcVerts[base + 2].b = 0;
                                pcVerts[base + 3].r = 255;
                                pcVerts[base + 3].g = 0;
                                pcVerts[base + 3].b = 0;
                            } else {
                                lightStartColour[0] = 255;
                                lightStartColour[1] = 0;
                                lightStartColour[2] = 0;
                                paletteDirty |= 1;
                            }
                        } else if (startCountdown <= 33 && startCountdown > 0) {
                            if (rgbTextures) {
                                pcVerts[base].r = 255;
                                pcVerts[base].g = 128;
                                pcVerts[base].b = 0;
                                pcVerts[base + 1].r = 255;
                                pcVerts[base + 1].g = 128;
                                pcVerts[base + 1].b = 0;
                                pcVerts[base + 2].r = 255;
                                pcVerts[base + 2].g = 128;
                                pcVerts[base + 2].b = 0;
                                pcVerts[base + 3].r = 255;
                                pcVerts[base + 3].g = 128;
                                pcVerts[base + 3].b = 0;
                            } else {
                                lightStartColour[0] = 255;
                                lightStartColour[1] = 128;
                                lightStartColour[2] = 0;
                                paletteDirty |= 1;
                            }
                        } else if (startCountdown == 0) {
                            if (rgbTextures) {
                                pcVerts[base].r = 0;
                                pcVerts[base].g = 255;
                                pcVerts[base].b = 0;
                                pcVerts[base + 1].r = 0;
                                pcVerts[base + 1].g = 255;
                                pcVerts[base + 1].b = 0;
                                pcVerts[base + 2].r = 0;
                                pcVerts[base + 2].g = 255;
                                pcVerts[base + 2].b = 0;
                                pcVerts[base + 3].r = 0;
                                pcVerts[base + 3].g = 255;
                                pcVerts[base + 3].b = 0;
                            } else {
                                lightStartColour[0] = 0;
                                lightStartColour[1] = 255;
                                lightStartColour[2] = 0;
                                paletteDirty |= 1;
                            }
                            lightUpdate++;
                        }
                    } else {
                        lightUpdate++;
                        if (lightUpdate >= 480) {
                            chaseSlot = (short)(lightChaseStep % 8);
                            if (chaseSlot == j) {
                                pcVerts[base].r = 0;
                                pcVerts[base].g = 255;
                                pcVerts[base].b = 0;
                                pcVerts[base + 1].r = 0;
                                pcVerts[base + 1].g = 255;
                                pcVerts[base + 1].b = 0;
                                pcVerts[base + 2].r = 0;
                                pcVerts[base + 2].g = 255;
                                pcVerts[base + 2].b = 0;
                                pcVerts[base + 3].r = 0;
                                pcVerts[base + 3].g = 255;
                                pcVerts[base + 3].b = 0;
                            } else {
                                pcVerts[base].r = 0;
                                pcVerts[base].g = j << 4;
                                pcVerts[base].b = 0;
                                pcVerts[base + 1].r = 0;
                                pcVerts[base + 1].g = j << 4;
                                pcVerts[base + 1].b = 0;
                                pcVerts[base + 2].r = 0;
                                pcVerts[base + 2].g = j << 4;
                                pcVerts[base + 2].b = 0;
                                pcVerts[base + 3].r = 0;
                                pcVerts[base + 3].g = j << 4;
                                pcVerts[base + 3].b = 0;
                            }
                        }
                    }
                    primitive.ft4++;
                    base += 4;
                    break;
                }
            }
        }
    }
    if (mode == -1) {
        if (lightUpdateTick % 2 == 0)
            lightChaseStep++;
        lightUpdateTick++;
    }
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
#include "start_lights.h"
extern int lightChaseStep, lightUpdateTick;
extern int startCountdown;
void UpdateLightStart(Object **objects, ShipData *unusedShips, CheckCell *unusedCells,
                      short unusedTextureBase, ConfigData *unusedConfig, int mode)
{
    short count = 3;
    short i, j, vertex;
    int white;
    int dirty;
    VECTOR direction;
    short angle;
    TrackSection *section, *previous;
    Object *object;
    Poly poly;
    PCVertex *vertices;

    if (gameType == 1 || gameType == 99)
        count = 1;
    for (i = 0; i < count; i++) {
        section = lightSections[i];
        object = objects[i];
        previous = section->prevSection.ptr;
        direction.vz = section->centre.vz - previous->centre.vz;
        direction.vx = section->centre.vx - previous->centre.vx;
        angle = ratan2(direction.vx, direction.vz);
        angle = 4096 - angle;
        SetSkeletonDirectionHPR(object->skeleton, angle, 0, 0);
        SetSkeletonPosition(object->skeleton, section->centre.vx,
                            section->centre.vy - 3000, section->centre.vz);
        dirty = paletteDirty;
        white = 255;
        if (mode == -1) {
            poly.prim = object->primitives;
            vertices = (PCVertex *)object->floatVerts;
            vertex = 0;
            for (j = 0; j < 8; j++) {
                switch (poly.prim->type) {
                case TypeFT4:
                    if (lightUpdate < 480) {
                        if (startCountdown <= 66 && startCountdown > 33) {
                            if (rgbTextures) {
                                vertices[vertex].r = white;
                                vertices[vertex].g = 0;
                                vertices[vertex].b = 0;
                                vertices[vertex + 1].r = white;
                                vertices[vertex + 1].g = 0;
                                vertices[vertex + 1].b = 0;
                                vertices[vertex + 2].r = white;
                                vertices[vertex + 2].g = 0;
                                vertices[vertex + 2].b = 0;
                                vertices[vertex + 3].r = white;
                                vertices[vertex + 3].g = 0;
                                vertices[vertex + 3].b = 0;
                            } else {
                                dirty |= 1;
                                lightStartColour[0] = white;
                                lightStartColour[1] = 0;
                                lightStartColour[2] = 0;
                            }
                        } else if (startCountdown <= 33 && startCountdown > 0) {
                            if (rgbTextures) {
                                vertices[vertex].r = white;
                                vertices[vertex].g = 128;
                                vertices[vertex].b = 0;
                                vertices[vertex + 1].r = white;
                                vertices[vertex + 1].g = 128;
                                vertices[vertex + 1].b = 0;
                                vertices[vertex + 2].r = white;
                                vertices[vertex + 2].g = 128;
                                vertices[vertex + 2].b = 0;
                                vertices[vertex + 3].r = white;
                                vertices[vertex + 3].g = 128;
                                vertices[vertex + 3].b = 0;
                            } else {
                                dirty |= 1;
                                lightStartColour[0] = white;
                                lightStartColour[1] = 128;
                                lightStartColour[2] = 0;
                            }
                        } else if (startCountdown == 0) {
                            if (rgbTextures) {
                                vertices[vertex].r = 0;
                                vertices[vertex].g = white;
                                vertices[vertex].b = 0;
                                vertices[vertex + 1].r = 0;
                                vertices[vertex + 1].g = white;
                                vertices[vertex + 1].b = 0;
                                vertices[vertex + 2].r = 0;
                                vertices[vertex + 2].g = white;
                                vertices[vertex + 2].b = 0;
                                vertices[vertex + 3].r = 0;
                                vertices[vertex + 3].g = white;
                                vertices[vertex + 3].b = 0;
                            } else {
                                dirty |= 1;
                                lightStartColour[0] = 0;
                                lightStartColour[1] = white;
                                lightStartColour[2] = 0;
                            }
                            lightUpdate++;
                        }
                    } else {
                        lightUpdate++;
                        if (lightUpdate >= 480) {
                            if ((short)(lightChaseStep % 8) == j) {
                                vertices[vertex].r = 0;
                                vertices[vertex].g = white;
                                vertices[vertex].b = 0;
                                vertices[vertex + 1].r = 0;
                                vertices[vertex + 1].g = white;
                                vertices[vertex + 1].b = 0;
                                vertices[vertex + 2].r = 0;
                                vertices[vertex + 2].g = white;
                                vertices[vertex + 2].b = 0;
                                vertices[vertex + 3].r = 0;
                                vertices[vertex + 3].g = white;
                                vertices[vertex + 3].b = 0;
                            } else {
                                vertices[vertex].r = 0;
                                vertices[vertex].g = j << 4;
                                vertices[vertex].b = 0;
                                vertices[vertex + 1].r = 0;
                                vertices[vertex + 1].g = j << 4;
                                vertices[vertex + 1].b = 0;
                                vertices[vertex + 2].r = 0;
                                vertices[vertex + 2].g = j << 4;
                                vertices[vertex + 2].b = 0;
                                vertices[vertex + 3].r = 0;
                                vertices[vertex + 3].g = j << 4;
                                vertices[vertex + 3].b = 0;
                            }
                        }
                    }
                    vertex += 4;
                    poly.ft4++;
                    break;
                }
                paletteDirty = dirty;
            }
        }
    }
    if (mode == -1) {
        if (lightUpdateTick % 2 == 0)
            lightChaseStep++;
        lightUpdateTick++;
    }
}
#endif
#endif

#ifdef EQUIV
#ifdef NETPLAY
void DrawLightStart(Object **objects, ShipData *ships, CheckCell *unusedCells,
                    short unusedTextureBase, ConfigData *unusedConfig, int unusedMode)
{
    short count;
    short i;
    short distance;
    TrackSection *section;
    Object *object;

    count = 3;
    if (gameType == 1 || gameType == 99)
        count = 1;
    /* NET-WOXL selects the first light on either branch of the count test. */
    if (count == 1)
        i = 0;
    else
        i = 0;
    section = lightSections[i];
    distance = raceLines[ships[ownShip].nearSecNo].distance - raceLines[section->secNo].distance;
    if (distance < 0)
        distance += numTrackSections;
    if (distance > numTrackSections >> 1)
        distance -= numTrackSections;
    object = objects[i];
    object->skeleton->update = 1;
    if (abs(distance) <= 160)
        AddRenderObject(object, 0, 0);
}
#endif
#endif

#ifndef NETPLAY
void DrawLightStart(Object **objects, ShipData *ships, CheckCell *unusedCells,
                    short unusedTextureBase, ConfigData *unusedConfig, int unusedMode)
{
    short count;
    short i;
    short distance;
    TrackSection *section;
    Object *object;

    count = 3;
    if (gameType == 1 || gameType == 99)
        count = 1;
    /* The redundant branches preserve the original count setup. */
    if (count == 1)
        i = 0;
    else
        i = 0;
    do {
        section = lightSections[i];
        distance = raceLines[ships[ownShip].nearSecNo].distance - raceLines[section->secNo].distance;
        if (distance < 0)
            distance += numTrackSections;
        if (distance > numTrackSections >> 1)
            distance -= numTrackSections;
        object = objects[i];
        object->skeleton->update = 1;
        if (abs(distance) <= 160)
            AddRenderObject(object, 0, 0);
        return;
    } while (++i < count);
}
#endif

void MoveMonoRail(Object **objects, short count, MonoRailData *mono, ShipData *ships, int mode)
{
    Object *railObject;
#ifndef NETPLAY
    Skeleton *skeleton;
#endif
    int i;

    if (ships[ownShip].nearSecNo == 210) {
        if (mono->index != ships[ownShip].lap) {
#ifdef NETPLAY
            mono->index = ships[ownShip].lap;
            monoRailSettle = 1900;
#else
            monoRailSettle = 1900;
            mono->index = ships[ownShip].lap;
#endif
        }
    }
    for (i = 0; i < count; i++) {
        railObject = objects[i];
        if (mode == -1 && monoRailSettle != 0) {
            if (ships[ownShip].lap % 2 == 0) {
#ifdef NETPLAY
                SetSkeletonPosition(railObject->skeleton, mono->stepX + railObject->skeleton->relative.t[0],
                                    mono->stepY + railObject->skeleton->relative.t[1],
                                    mono->stepZ + railObject->skeleton->relative.t[2]);
#else
                skeleton = railObject->skeleton;
                SetSkeletonPosition(skeleton, mono->stepX + skeleton->relative.t[0],
                                    mono->stepY + skeleton->relative.t[1],
                                    mono->stepZ + skeleton->relative.t[2]);
#endif
            } else {
#ifdef NETPLAY
                SetSkeletonPosition(railObject->skeleton, railObject->skeleton->relative.t[0] - mono->stepX,
                                    railObject->skeleton->relative.t[1] - mono->stepY,
                                    railObject->skeleton->relative.t[2] - mono->stepZ);
#else
                skeleton = railObject->skeleton;
                SetSkeletonPosition(skeleton, skeleton->relative.t[0] - mono->stepX,
                                    skeleton->relative.t[1] - mono->stepY,
                                    skeleton->relative.t[2] - mono->stepZ);
#endif
            }
            monoRailSettle--;
        }
    }
}

extern int zeppelinDrift;

#ifdef NETPLAY
void DrawMonoRail(Object **objects, short count)
{
    int i;
    Object *model;

    for (i = 0; i < count; i++) {
        model = objects[i];
        model->skeleton->update = 1;
        AddRenderObject(model, 0, 0);
    }
}
#else
void DrawMonoRail(Object **objects, short count)
{
    int i;
    Object *object;

    for (i = 0; i < count; i++) {
        object = objects[i];
        object->skeleton->update = 1;
        AddRenderObject(object, 0, 0);
    }
}
#endif

void MoveZeppelin(Object **objects, short unused, ShipData *ships)
{
    Object *object;

    object = objects[0];
    if (ships[ownShip].nearTrkSect->secNo == 150 && ships[ownShip].prevShipSect->secNo == 149)
        zeppelinDrift = 0;
    if (ships[ownShip].nearTrkSect->secNo == 201)
        ResetZeppelin();
    if (ships[ownShip].nearTrkSect->secNo >= 150 && ships[ownShip].nearTrkSect->secNo < 201 && zeppelinDrift < 100000)
        zeppelinDrift += 100;
    SetSkeletonPosition(object->skeleton, zeppelinBase.vx, zeppelinBase.vy, zeppelinBase.vz + zeppelinDrift);
}

#ifdef NETPLAY
#include "mathinline.h"
#pragma intrinsic(sqrt)
/* NET latches the droid heights on the first bob frame; the /Od build keeps the jump after the latch. */
#define NET_LATCH_DROID_HEIGHT 1
void MoveCameraDroid(Object **objects, short count, ShipData *ships, int *bobCount)
{
    short pitchAng;
    int magnitude;
    int index;
    Object *obj;
    short dir;
    VECTOR offsetVec;

    for (index = 0; index < count; index++) {
        obj = objects[index];
        offsetVec.vx = obj->skeleton->relative.t[0] - ships[ownShip].ppivot.vx;
        offsetVec.vy = obj->skeleton->relative.t[1] - ships[ownShip].ppivot.vy;
        offsetVec.vz = obj->skeleton->relative.t[2] - ships[ownShip].ppivot.vz;
        dir = ratan2(offsetVec.vx, offsetVec.vz);
        magnitude = sqrt(offsetVec.vx * offsetVec.vx + offsetVec.vz * offsetVec.vz);
        pitchAng = ratan2(offsetVec.vy, magnitude);
        if (ships[ownShip].attr & DIRECTION) {
            pitchAng = 4096 - pitchAng;
            dir = 4096 - dir;
        } else {
            pitchAng = 0 - pitchAng;
            dir = 0 - dir;
        }
        SetSkeletonDirectionHPR(obj->skeleton, dir, pitchAng, 0);
        if (NET_LATCH_DROID_HEIGHT) {
            if (*bobCount == 0)
                cameraDroidHeight[index] = obj->skeleton->relative.t[1];
        } else {
        }
        SetSkeletonPosition(obj->skeleton, obj->skeleton->relative.t[0],
                            cameraDroidHeight[index] + (rsin(*bobCount) >> 6), obj->skeleton->relative.t[2]);
    }
    *bobCount += 200;
}
#pragma function(sqrt)
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Register allocation differs. */
void MoveCameraDroid(Object **objects, short count, ShipData *ships, int *bobCount)
{
    int i;
    Object *object;
    VECTOR delta;
    short heading;
    short pitch;

    for (i = 0; i < count; i++) {
        object = objects[i];
        delta.vy = object->skeleton->relative.t[1] - ships[ownShip].ppivot.vy;
        delta.vx = object->skeleton->relative.t[0] - ships[ownShip].ppivot.vx;
        delta.vz = object->skeleton->relative.t[2] - ships[ownShip].ppivot.vz;
        heading = ratan2(delta.vx, delta.vz);
        pitch = ratan2(delta.vy, SquareRoot0(delta.vx * delta.vx + delta.vz * delta.vz));
        if (ships[ownShip].attr & DIRECTION) {
            pitch = 4096 - pitch;
            heading = 4096 - heading;
        } else {
            pitch = -pitch;
            heading = -heading;
        }
        SetSkeletonDirectionHPR(object->skeleton, heading, pitch, 0);
        if (*bobCount == 0)
            cameraDroidHeight[i] = object->skeleton->relative.t[1];
        SetSkeletonPosition(object->skeleton, object->skeleton->relative.t[0],
                            cameraDroidHeight[i] + (rsin(*bobCount) >> 6), object->skeleton->relative.t[2]);
    }
    *bobCount += 200;
}
#endif
#endif

#ifdef NETPLAY
void MoveRadar(Object **objects, short count)
{
    Object *model;
    int i;

    for (i = 0; i < count; i++) {
        model = objects[i];
        SetSkeletonDirection(model->skeleton, 0, radarAngle, 0);
    }
    radarAngle += 80;
}
#else
void MoveRadar(Object **objects, short count)
{
    int i;

    for (i = 0; i < count; i++)
        SetSkeletonDirection(objects[i]->skeleton, 0, radarAngle, 0);
    radarAngle += 80;
}
#endif

#ifdef NETPLAY
#define SHADOW_LAST_PANEL_EXITS 1
#include "pc_particles.h"
#include "camera.h"
extern short flyingFlags[15];
extern unsigned char opaqueAlpha;
extern int mmxAvailable;
extern const float shadowCoordinateScale, shadowHeight;

#pragma intrinsic(memcpy)
void DrawShadow(Object **unusedShapes, ShipData *ships)
{
    ParticleTriangleList *triangleList;
    VECTOR frontLeft;
    PCVertex *quadVerts;
    VECTOR cornerB;
    int planeT;
    int vertexNum;
    VECTOR backRight;
    int i;
    Face *floorFace;
    VECTOR backCorner;
    int material;
    VECTOR facePoint;

    vertexNum = 0;
    for (i = 0; i <= normalSetNo; i++)
        trailIndexLists[i].count = 0;
    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    for (i = 0; i < 15; i++) {
        if ((ships[i].prevShipSect->flags & TRACK_BASE) && !(ships[i].nearTrkSect->flags & TRACK_BASE))
            flyingFlags[i] |= FLYING;
        if (((gameType == 1 && ownShip == i) || gameType != 1) &&
            !(ships[i].attr & RESCUE) && (ships[i].attr & VISIBLE) &&
            !(flyingFlags[i] & FLYING) && !(ships[i].combatAttr & SHIP_COLL) && !(ships[i].combatAttr & ELECTROED)) {
            floorFace = &track->faces[ships[i].nearTrkSect->faceStart];
            while (!(floorFace->flags & TRACK_BASE))
                floorFace++;
            frontLeft.vx = ((ships[i].unitVecNose.vx * 4) >> 5) + ships[i].ppivot.vx - (ships[i].unitVecWing.vx >> 4);
            frontLeft.vy = ((ships[i].unitVecNose.vy * 4) >> 5) + ships[i].ppivot.vy - (ships[i].unitVecWing.vy >> 4);
            frontLeft.vz = ((ships[i].unitVecNose.vz * 4) >> 5) + ships[i].ppivot.vz - (ships[i].unitVecWing.vz >> 4);
            cornerB.vx = ((ships[i].unitVecNose.vx * 4) >> 5) + ships[i].ppivot.vx + (ships[i].unitVecWing.vx >> 4);
            cornerB.vy = ((ships[i].unitVecNose.vy * 4) >> 5) + ships[i].ppivot.vy + (ships[i].unitVecWing.vy >> 4);
            cornerB.vz = ((ships[i].unitVecNose.vz * 4) >> 5) + ships[i].ppivot.vz + (ships[i].unitVecWing.vz >> 4);
            backRight.vx = ships[i].ppivot.vx - ((ships[i].unitVecNose.vx * 4) >> 5) - (ships[i].unitVecWing.vx >> 4);
            backRight.vy = ships[i].ppivot.vy - ((ships[i].unitVecNose.vy * 4) >> 5) - (ships[i].unitVecWing.vy >> 4);
            backRight.vz = ships[i].ppivot.vz - ((ships[i].unitVecNose.vz * 4) >> 5) - (ships[i].unitVecWing.vz >> 4);
            backCorner.vx = ships[i].ppivot.vx - ((ships[i].unitVecNose.vx * 4) >> 5) + (ships[i].unitVecWing.vx >> 4);
            backCorner.vy = ships[i].ppivot.vy - ((ships[i].unitVecNose.vy * 4) >> 5) + (ships[i].unitVecWing.vy >> 4);
            backCorner.vz = ships[i].ppivot.vz - ((ships[i].unitVecNose.vz * 4) >> 5) + (ships[i].unitVecWing.vz >> 4);
            facePoint.vx = track->vertices[floorFace->vertex[0]].vx;
            facePoint.vy = track->vertices[floorFace->vertex[0]].vy;
            facePoint.vz = track->vertices[floorFace->vertex[0]].vz;
            planeT = PlaneLineCollision(facePoint, frontLeft, floorFace->normal);
            frontLeft.vx -= (planeT * floorFace->normal.vx) >> 12;
            frontLeft.vy -= (planeT * floorFace->normal.vy) >> 12;
            frontLeft.vz -= (planeT * floorFace->normal.vz) >> 12;
            planeT = PlaneLineCollision(facePoint, cornerB, floorFace->normal);
            cornerB.vx -= (planeT * floorFace->normal.vx) >> 12;
            cornerB.vy -= (planeT * floorFace->normal.vy) >> 12;
            cornerB.vz -= (planeT * floorFace->normal.vz) >> 12;
            planeT = PlaneLineCollision(facePoint, backRight, floorFace->normal);
            backRight.vx -= (planeT * floorFace->normal.vx) >> 12;
            backRight.vy -= (planeT * floorFace->normal.vy) >> 12;
            backRight.vz -= (planeT * floorFace->normal.vz) >> 12;
            planeT = PlaneLineCollision(facePoint, backCorner, floorFace->normal);
            backCorner.vx -= (planeT * floorFace->normal.vx) >> 12;
            backCorner.vy -= (planeT * floorFace->normal.vy) >> 12;
            backCorner.vz -= (planeT * floorFace->normal.vz) >> 12;
            trailStage[vertexNum].x = (frontLeft.vx + cameraPtr->camPos->relative.t[0]) / 4096.0f;
            trailStage[vertexNum].y = (frontLeft.vy + cameraPtr->camPos->relative.t[1]) / 4096.0f - shadowHeight;
            trailStage[vertexNum].z = (frontLeft.vz + cameraPtr->camPos->relative.t[2]) / 4096.0f;
            trailStage[vertexNum].a = opaqueAlpha;
            trailStage[vertexNum].r = 255;
            trailStage[vertexNum].g = 255;
            trailStage[vertexNum].b = 255;
            trailStage[vertexNum + 1].x = (cornerB.vx + cameraPtr->camPos->relative.t[0]) / 4096.0f;
            trailStage[vertexNum + 1].y = (cornerB.vy + cameraPtr->camPos->relative.t[1]) / 4096.0f - shadowHeight;
            trailStage[vertexNum + 1].z = (cornerB.vz + cameraPtr->camPos->relative.t[2]) / 4096.0f;
            trailStage[vertexNum + 1].a = opaqueAlpha;
            trailStage[vertexNum + 1].r = 255;
            trailStage[vertexNum + 1].g = 255;
            trailStage[vertexNum + 1].b = 255;
            trailStage[vertexNum + 2].x = (backRight.vx + cameraPtr->camPos->relative.t[0]) / 4096.0f;
            trailStage[vertexNum + 2].y = (backRight.vy + cameraPtr->camPos->relative.t[1]) / 4096.0f - shadowHeight;
            trailStage[vertexNum + 2].z = (backRight.vz + cameraPtr->camPos->relative.t[2]) / 4096.0f;
            trailStage[vertexNum + 2].a = opaqueAlpha;
            trailStage[vertexNum + 2].r = 255;
            trailStage[vertexNum + 2].g = 255;
            trailStage[vertexNum + 2].b = 255;
            trailStage[vertexNum + 3].x = (backCorner.vx + cameraPtr->camPos->relative.t[0]) / 4096.0f;
            trailStage[vertexNum + 3].y = (backCorner.vy + cameraPtr->camPos->relative.t[1]) / 4096.0f - shadowHeight;
            trailStage[vertexNum + 3].z = (backCorner.vz + cameraPtr->camPos->relative.t[2]) / 4096.0f;
            trailStage[vertexNum + 3].a = opaqueAlpha;
            trailStage[vertexNum + 3].r = 255;
            trailStage[vertexNum + 3].g = 255;
            trailStage[vertexNum + 3].b = 255;
            switch (i / 3) {
            case 0:
                trailStage[vertexNum].u = 0.005f;
                trailStage[vertexNum].v = 0.0f;
                trailStage[vertexNum + 1].u = 0.495f;
                trailStage[vertexNum + 1].v = 0.0f;
                trailStage[vertexNum + 2].u = 0.005f;
                trailStage[vertexNum + 2].v = 0.5f;
                trailStage[vertexNum + 3].u = 0.495f;
                trailStage[vertexNum + 3].v = 0.5f;
                material = 15;
                break;
            case 1:
                trailStage[vertexNum].u = 0.505f;
                trailStage[vertexNum].v = 0.0f;
                trailStage[vertexNum + 1].u = 0.995f;
                trailStage[vertexNum + 1].v = 0.0f;
                trailStage[vertexNum + 2].u = 0.505f;
                trailStage[vertexNum + 2].v = 0.5f;
                trailStage[vertexNum + 3].u = 0.995f;
                trailStage[vertexNum + 3].v = 0.5f;
                material = 15;
                break;
            case 2:
                trailStage[vertexNum].u = 0.005f;
                trailStage[vertexNum].v = 0.5f;
                trailStage[vertexNum + 1].u = 0.495f;
                trailStage[vertexNum + 1].v = 0.5f;
                trailStage[vertexNum + 2].u = 0.005f;
                trailStage[vertexNum + 2].v = 1.0f;
                trailStage[vertexNum + 3].u = 0.495f;
                trailStage[vertexNum + 3].v = 1.0f;
                material = 15;
                break;
            case 3:
                trailStage[vertexNum].u = 0.505f;
                trailStage[vertexNum].v = 0.5f;
                trailStage[vertexNum + 1].u = 0.995f;
                trailStage[vertexNum + 1].v = 0.5f;
                trailStage[vertexNum + 2].u = 0.505f;
                trailStage[vertexNum + 2].v = 1.0f;
                trailStage[vertexNum + 3].u = 0.995f;
                trailStage[vertexNum + 3].v = 1.0f;
                material = 15;
                break;
            case 4:
                trailStage[vertexNum].u = 0.505f;
                trailStage[vertexNum].v = 0.25f;
                trailStage[vertexNum + 1].u = 0.995f;
                trailStage[vertexNum + 1].v = 0.25f;
                trailStage[vertexNum + 2].u = 0.505f;
                trailStage[vertexNum + 2].v = 0.75f;
                trailStage[vertexNum + 3].u = 0.995f;
                trailStage[vertexNum + 3].v = 0.75f;
                material = 16;
                if (SHADOW_LAST_PANEL_EXITS) /* NET-WOXL leaves the last panel through a constant test; /Od keeps both jumps */
                    break;
                break;
            }
            trailIndexLists[material].triangles[trailIndexLists[material].count].v1 = vertexNum;
            trailIndexLists[material].triangles[trailIndexLists[material].count].v2 = vertexNum + 1;
            trailIndexLists[material].triangles[trailIndexLists[material].count].v3 = vertexNum + 3;
            trailIndexLists[material].count++;
            trailIndexLists[material].triangles[trailIndexLists[material].count].v1 = vertexNum;
            trailIndexLists[material].triangles[trailIndexLists[material].count].v2 = vertexNum + 3;
            trailIndexLists[material].triangles[trailIndexLists[material].count].v3 = vertexNum + 2;
            trailIndexLists[material].count++;
            vertexNum += 4;
        }
        flyingFlags[i] = 0;
    }
    if (vertexNum) {
        quadVerts = (PCVertex *)trackVertexCursor;
        trackVertexCursor += vertexNum * sizeof(PCVertex);
        memcpy(quadVerts, trailStage, vertexNum * sizeof(PCVertex));
        triangleList = (ParticleTriangleList *)trackVertexCursor;
        trackVertexCursor += 23 * sizeof(ParticleTriangleList);
        for (i = 0; i <= normalSetNo; i++) {
            if (quadBuckets[i].count) {
                triangleList[i].count = quadBuckets[i].count;
                triangleList[i].tris = (D3DTRIANGLE *)trackVertexCursor;
                trackVertexCursor += quadBuckets[i].count * sizeof(D3DTRIANGLE);
                memcpy(triangleList[i].tris, quadBuckets[i].triangles, quadBuckets[i].count * sizeof(D3DTRIANGLE));
            } else {
                triangleList[i].count = 0;
                triangleList[i].tris = 0;
            }
        }
        if (rgbTextures) {
            if (mmxAvailable)
                AddRenderSkeleton(cameraPtr->camPos->super, quadVerts, vertexNum, triangleList, 0, 0, 1);
            else
                AddRenderSkeleton(cameraPtr->camPos->super, quadVerts, vertexNum, triangleList, 0, 0, 0);
        } else
            AddRenderSkeleton(cameraPtr->camPos->super, quadVerts, vertexNum, triangleList, 0, 0, 1);
    }
}
#pragma function(memcpy)
#endif

#ifdef EQUIV
#include "pc_particles.h"
#include "camera.h"
extern short flyingFlags[15];
extern unsigned char opaqueAlpha;
extern int mmxAvailable;
extern const float shadowCoordinateScale, shadowHeight;

#ifndef NETPLAY
void DrawShadow(Object **unusedShapes, ShipData *ships)
{
    VECTOR cornerA, cornerB, cornerC, cornerD, facePoint;
    int alpha;
    int i, j;
    int vertexCount;
    int material;
    int materialsLeft;
    int count;
    Face *face;
    ShipData *ship;
    PCVertex *vertices;
    ParticleTriangleList *triangleLists;
    ParticleTriangleBuffer *buffer;

    materialsLeft = normalSetNo + 1;
    vertexCount = 0;
    buffer = trailIndexLists;
    do {
        buffer->count = 0;
        buffer++;
    } while (--materialsLeft);
    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    for (i = 0; i < 15; i++) {
        ship = &ships[i];
        if ((ship->prevShipSect->flags & TRACK_BASE) && !(ship->nearTrkSect->flags & TRACK_BASE))
            flyingFlags[i] |= FLYING;
        if ((gameType != 1 || i == ownShip || gameType != 1) &&
            !(ship->attr & RESCUE) && (ship->attr & VISIBLE) &&
            !(flyingFlags[i] & FLYING) && !(ship->combatAttr & (SHIP_COLL | ELECTROED))) {
            face = &track->faces[ship->nearTrkSect->faceStart];
            while (!(face->flags & TRACK_BASE))
                face++;
            cornerA.vx = (((ship->unitVecNose.vx * 4) >> 5) - (ship->unitVecWing.vx >> 4)) + ship->ppivot.vx;
            cornerA.vy = (((ship->unitVecNose.vy * 4) >> 5) - (ship->unitVecWing.vy >> 4)) + ship->ppivot.vy;
            cornerA.vz = (((ship->unitVecNose.vz * 4) >> 5) - (ship->unitVecWing.vz >> 4)) + ship->ppivot.vz;
            cornerB.vx = (((ship->unitVecNose.vx * 4) >> 5) + (ship->unitVecWing.vx >> 4)) + ship->ppivot.vx;
            cornerB.vy = (((ship->unitVecNose.vy * 4) >> 5) + (ship->unitVecWing.vy >> 4)) + ship->ppivot.vy;
            cornerB.vz = (((ship->unitVecNose.vz * 4) >> 5) + (ship->unitVecWing.vz >> 4)) + ship->ppivot.vz;
            cornerC.vx = (ship->ppivot.vx - ((ship->unitVecNose.vx * 4) >> 5)) - (ship->unitVecWing.vx >> 4);
            cornerC.vy = (ship->ppivot.vy - ((ship->unitVecNose.vy * 4) >> 5)) - (ship->unitVecWing.vy >> 4);
            cornerC.vz = (ship->ppivot.vz - ((ship->unitVecNose.vz * 4) >> 5)) - (ship->unitVecWing.vz >> 4);
            cornerD.vx = ((ship->unitVecWing.vx >> 4) - ((ship->unitVecNose.vx * 4) >> 5)) + ship->ppivot.vx;
            cornerD.vy = ((ship->unitVecWing.vy >> 4) - ((ship->unitVecNose.vy * 4) >> 5)) + ship->ppivot.vy;
            cornerD.vz = ((ship->unitVecWing.vz >> 4) - ((ship->unitVecNose.vz * 4) >> 5)) + ship->ppivot.vz;
            facePoint.vx = track->vertices[face->vertex[0]].vx;
            facePoint.vy = track->vertices[face->vertex[0]].vy;
            facePoint.vz = track->vertices[face->vertex[0]].vz;
            alpha = PlaneLineCollision(facePoint, cornerA, face->normal);
            cornerA.vx -= (alpha * face->normal.vx) >> 12;
            cornerA.vy -= (alpha * face->normal.vy) >> 12;
            cornerA.vz -= (alpha * face->normal.vz) >> 12;
            alpha = PlaneLineCollision(facePoint, cornerB, face->normal);
            cornerB.vx -= (alpha * face->normal.vx) >> 12;
            cornerB.vy -= (alpha * face->normal.vy) >> 12;
            cornerB.vz -= (alpha * face->normal.vz) >> 12;
            alpha = PlaneLineCollision(facePoint, cornerC, face->normal);
            cornerC.vx -= (alpha * face->normal.vx) >> 12;
            cornerC.vy -= (alpha * face->normal.vy) >> 12;
            cornerC.vz -= (alpha * face->normal.vz) >> 12;
            alpha = PlaneLineCollision(facePoint, cornerD, face->normal);
            cornerD.vx -= (alpha * face->normal.vx) >> 12;
            cornerD.vy -= (alpha * face->normal.vy) >> 12;
            cornerD.vz -= (alpha * face->normal.vz) >> 12;
            trailStage[vertexCount].x = (cornerA.vx + cameraPtr->camPos->relative.t[0]) * shadowCoordinateScale;
            trailStage[vertexCount].y = (cornerA.vy + cameraPtr->camPos->relative.t[1]) * shadowCoordinateScale - shadowHeight;
            trailStage[vertexCount].z = (cornerA.vz + cameraPtr->camPos->relative.t[2]) * shadowCoordinateScale;
            trailStage[vertexCount].a = opaqueAlpha;
            trailStage[vertexCount].r = 255;
            trailStage[vertexCount].g = 255;
            trailStage[vertexCount].b = 255;
            trailStage[vertexCount + 1].x = (cornerB.vx + cameraPtr->camPos->relative.t[0]) * shadowCoordinateScale;
            trailStage[vertexCount + 1].y = (cornerB.vy + cameraPtr->camPos->relative.t[1]) * shadowCoordinateScale - shadowHeight;
            trailStage[vertexCount + 1].z = (cornerB.vz + cameraPtr->camPos->relative.t[2]) * shadowCoordinateScale;
            trailStage[vertexCount + 1].a = opaqueAlpha;
            trailStage[vertexCount + 1].r = 255;
            trailStage[vertexCount + 1].g = 255;
            trailStage[vertexCount + 1].b = 255;
            trailStage[vertexCount + 2].x = (cornerC.vx + cameraPtr->camPos->relative.t[0]) * shadowCoordinateScale;
            trailStage[vertexCount + 2].y = (cornerC.vy + cameraPtr->camPos->relative.t[1]) * shadowCoordinateScale - shadowHeight;
            trailStage[vertexCount + 2].z = (cornerC.vz + cameraPtr->camPos->relative.t[2]) * shadowCoordinateScale;
            trailStage[vertexCount + 2].a = opaqueAlpha;
            trailStage[vertexCount + 2].r = 255;
            trailStage[vertexCount + 2].g = 255;
            trailStage[vertexCount + 2].b = 255;
            trailStage[vertexCount + 3].x = (cornerD.vx + cameraPtr->camPos->relative.t[0]) * shadowCoordinateScale;
            trailStage[vertexCount + 3].y = (cornerD.vy + cameraPtr->camPos->relative.t[1]) * shadowCoordinateScale - shadowHeight;
            trailStage[vertexCount + 3].z = (cornerD.vz + cameraPtr->camPos->relative.t[2]) * shadowCoordinateScale;
            trailStage[vertexCount + 3].a = opaqueAlpha;
            trailStage[vertexCount + 3].r = 255;
            trailStage[vertexCount + 3].g = 255;
            trailStage[vertexCount + 3].b = 255;
            switch (i / 3) {
            case 0:
                trailStage[vertexCount].u = 0.005f;
                trailStage[vertexCount].v = 0.0f;
                trailStage[vertexCount + 1].u = 0.495f;
                trailStage[vertexCount + 1].v = 0.0f;
                trailStage[vertexCount + 2].u = 0.005f;
                trailStage[vertexCount + 2].v = 0.5f;
                trailStage[vertexCount + 3].u = 0.495f;
                trailStage[vertexCount + 3].v = 0.5f;
                material = 15;
                break;
            case 1:
                trailStage[vertexCount].u = 0.505f;
                trailStage[vertexCount].v = 0.0f;
                trailStage[vertexCount + 1].u = 0.995f;
                trailStage[vertexCount + 1].v = 0.0f;
                trailStage[vertexCount + 2].u = 0.505f;
                trailStage[vertexCount + 2].v = 0.5f;
                trailStage[vertexCount + 3].u = 0.995f;
                trailStage[vertexCount + 3].v = 0.5f;
                material = 15;
                break;
            case 2:
                trailStage[vertexCount].u = 0.005f;
                trailStage[vertexCount].v = 0.5f;
                trailStage[vertexCount + 1].u = 0.495f;
                trailStage[vertexCount + 1].v = 0.5f;
                trailStage[vertexCount + 2].u = 0.005f;
                trailStage[vertexCount + 2].v = 1.0f;
                trailStage[vertexCount + 3].u = 0.495f;
                trailStage[vertexCount + 3].v = 1.0f;
                material = 15;
                break;
            case 3:
                trailStage[vertexCount].u = 0.505f;
                trailStage[vertexCount].v = 0.5f;
                trailStage[vertexCount + 1].u = 0.995f;
                trailStage[vertexCount + 1].v = 0.5f;
                trailStage[vertexCount + 2].u = 0.505f;
                trailStage[vertexCount + 2].v = 1.0f;
                trailStage[vertexCount + 3].u = 0.995f;
                trailStage[vertexCount + 3].v = 1.0f;
                material = 15;
                break;
            case 4:
                trailStage[vertexCount].u = 0.505f;
                trailStage[vertexCount].v = 0.25f;
                trailStage[vertexCount + 1].u = 0.995f;
                trailStage[vertexCount + 1].v = 0.25f;
                trailStage[vertexCount + 2].u = 0.505f;
                trailStage[vertexCount + 2].v = 0.75f;
                trailStage[vertexCount + 3].u = 0.995f;
                trailStage[vertexCount + 3].v = 0.75f;
                material = 16;
                break;
            }
            trailIndexLists[material].triangles[trailIndexLists[material].count].v1 = vertexCount;
            trailIndexLists[material].triangles[trailIndexLists[material].count].v2 = vertexCount + 1;
            trailIndexLists[material].triangles[trailIndexLists[material].count].v3 = vertexCount + 3;
            trailIndexLists[material].count++;
            trailIndexLists[material].triangles[trailIndexLists[material].count].v1 = vertexCount;
            trailIndexLists[material].triangles[trailIndexLists[material].count].v2 = vertexCount + 3;
            trailIndexLists[material].triangles[trailIndexLists[material].count].v3 = vertexCount + 2;
            trailIndexLists[material].count++;
            vertexCount += 4;
        }
        flyingFlags[i] = 0;
    }
    if (vertexCount) {
        char *cursor;
        vertices = (PCVertex *)trackVertexCursor;
        memcpy(vertices, trailStage, vertexCount * sizeof(PCVertex));
        triangleLists = (ParticleTriangleList *)(vertices + vertexCount);
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
                triangleLists[i].tris = 0;
                triangleLists[i].count = 0;
            }
            trackVertexCursor = cursor;
        }
        if (rgbTextures) {
            if (mmxAvailable)
                AddRenderSkeleton(cameraPtr->camPos->super, vertices, vertexCount, triangleLists, 0, 0, 1);
            else
                AddRenderSkeleton(cameraPtr->camPos->super, vertices, vertexCount, triangleLists, 0, 0, 0);
        } else
            AddRenderSkeleton(cameraPtr->camPos->super, vertices, vertexCount, triangleLists, 0, 0, 1);
    }
}
#endif
#endif

void SetShake(int amount)
{
    shakeAmount = amount;
}

extern DISPENV DispEnv0;
extern int screenWidth, screenHeight;
void ShakeScreen(void)
{
    float scale;

    if (shakeAmount > 1) {
        DispEnv0.screen.x = -(newRand() % shakeAmount) + (shakeAmount >> 1);
        DispEnv0.screen.y = -(newRand() % shakeAmount) + (shakeAmount >> 1);
        shakeAmount--;
    } else if (shakeAmount == 1) {
        DispEnv0.screen.x = 0;
        DispEnv0.screen.y = 0;
        shakeAmount--;
    }
    if (shipIndex[ownShip].attr & VIEW_IN)
        scale = 50.0f;
    else
        scale = 20.0f;
    cameraShakeX = (float)DispEnv0.screen.x / screenWidth * scale;
    cameraShakeY = (float)DispEnv0.screen.y / screenHeight * scale;
}

#ifdef NETPLAY
#include "pc_game.h"
extern PCGameConfig *gameData;
extern short startLinePos[];
extern int trackCycleActive;
extern u_char pitCycleFlag;
extern short pitCycleLength;

void InitPitCycle(void)
{
    TrackSection *section;
    Face *face;
    TrackSection *start;

    {
        int i;

        OutputDebugString("searching for pit lane\n");
        trackCycleActive = 1;
        section = track->sections;
        i = 0;
        for (;;) {
            face = track->faces + section->faceStart;
            while (!(face->flags & TRACK_BASE))
                face++;
            if (!(face->flags & 0x40))
                break;
            section++;
            i++;
            if (track->sectionCount < i) {
                pitCycleFlag = 0;
                break;
            }
        }
        for (;;) {
            face = track->faces + section->faceStart;
            while (!(face->flags & TRACK_BASE))
                face++;
            if (face->flags & 0x40)
                break;
            section++;
            i++;
            if (i >= track->sectionCount) {
                pitCycleFlag = 0;
                OutputDebugString("no pit lane\n");
                return;
            }
        }
        pitCycleFlag = 1;
        start = section;
        for (;;) {
            face = track->faces + start->faceStart;
            while (!(face->flags & TRACK_BASE))
                face++;
            if (!(face->flags & 0x40))
                break;
            start++;
            i++;
            if (i >= track->sectionCount) {
                pitCycleFlag = 0;
                OutputDebugString("no pit lane\n");
                return;
            }
        }
        pitCycleLength = start->secNo - section->secNo;
    }
}
#endif

#ifndef NETPLAY
#include "pc_game.h"
extern PCGameConfig *gameData;
#endif
#ifdef EQUIV
#ifndef NETPLAY
extern short startLinePos[];
extern int trackCycleActive;
extern u_char pitCycleFlag;
extern short pitCycleLength;

void InitPitCycle(void)
{
    Face *faces;
    int i;
    TrackData *data;
    TrackSection *section;
    TrackSection *start;
    Face *face;

    OutputDebugString("searching for pit lane\n");
    data = track;
    i = 0;
    trackCycleActive = 1;
    faces = data->faces;
    section = data->sections;
    for (;;) {
        face = faces + section->faceStart;
        while (!(face->flags & TRACK_BASE))
            face++;
        if (!(face->flags & 0x40))
            break;
        section++;
        i++;
        if (data->sectionCount < i)
            break;
    }
    for (;;) {
        face = faces + section->faceStart;
        while (!(face->flags & TRACK_BASE))
            face++;
        if (face->flags & 0x40)
            break;
        section++;
        i++;
        if (i >= data->sectionCount) {
            pitCycleFlag = 0;
            OutputDebugString("no pit lane\n");
            return;
        }
    }
    pitCycleFlag = 1;
    start = section;
    for (;;) {
        face = faces + section->faceStart;
        while (!(face->flags & TRACK_BASE))
            face++;
        if (!(face->flags & 0x40))
            break;
        section++;
        i++;
        if (i >= data->sectionCount) {
            pitCycleFlag = 0;
            OutputDebugString("no pit lane\n");
            return;
        }
    }
    pitCycleLength = section->secNo - start->secNo;
}
#endif
#endif

#ifdef EQUIV
#ifdef NETPLAY
void InitTrackCycle(void)
{
    TrackSection *section;

    section = track->sections;
    for (;;) {
        if (section->secNo == startLinePos[gameData->actTrackNum])
            break;
        section++;
    }
    {
        int i;

        for (i = 0; i < 30; i++)
            section = section->prevSection.ptr;
    }
    InitPitCycle();
}
#endif
#endif
#ifndef NETPLAY
void InitTrackCycle(void)
{
    TrackSection *section;
    int i;

    section = track->sections;
    for (;;) {
        if (section->secNo == startLinePos[gameData->actTrackNum])
            break;
        section++;
    }
    for (i = 0; i < 30; i++)
        section = section->prevSection.ptr;
    InitPitCycle();
}
#endif

#ifdef EQUIV
#ifdef NETPLAY
/* NET-WOXL: the coordinates are divided by 255 per vertex (no shared scale), and the object's primitive list pointer is read and never used. */
void RedBullAnimate(Object **objects, short objectCount, short unusedPeriod,
                    short firstTexture, short unusedTextureCount)
{
    Primitive *prim;
    int i;
    PCVertex *vertexPtr;
    Texture *texture;

    texture = textureTable[firstTexture];
    for (i = 0; i < objectCount; i++) {
        prim = objects[i]->primitives;
        vertexPtr = (PCVertex *)objects[i]->floatVerts;
        vertexPtr[0].u = (unsigned long)texture->u0 / 255.0f;
        vertexPtr[0].v = (unsigned long)texture->v0 / 255.0f;
        vertexPtr[1].u = (texture->u1 - ((texture->textureW * 4) >> 1)) / 255.0f;
        vertexPtr[1].v = (unsigned long)texture->v1 / 255.0f;
        vertexPtr[2].u = (unsigned long)texture->u2 / 255.0f;
        vertexPtr[2].v = (texture->v2 - (texture->textureH >> 1)) / 255.0f;
        vertexPtr[3].u = (texture->u3 - ((texture->textureW * 4) >> 1)) / 255.0f;
        vertexPtr[3].v = (texture->v3 - (texture->textureH >> 1)) / 255.0f;
        vertexPtr[4].u = (texture->u0 + ((texture->textureW * 4) >> 1)) / 255.0f;
        vertexPtr[4].v = (unsigned long)texture->v0 / 255.0f;
        vertexPtr[5].u = (unsigned long)texture->u1 / 255.0f;
        vertexPtr[5].v = (unsigned long)texture->v1 / 255.0f;
        vertexPtr[6].u = (texture->u2 + ((texture->textureW * 4) >> 1)) / 255.0f;
        vertexPtr[6].v = (texture->v2 - (texture->textureH >> 1)) / 255.0f;
        vertexPtr[7].u = (unsigned long)texture->u3 / 255.0f;
        vertexPtr[7].v = (texture->v3 - (texture->textureH >> 1)) / 255.0f;
        vertexPtr[8].u = (unsigned long)texture->u0 / 255.0f;
        vertexPtr[8].v = (texture->v0 + (texture->textureH >> 1)) / 255.0f;
        vertexPtr[9].u = (texture->u1 - ((texture->textureW * 4) >> 1)) / 255.0f;
        vertexPtr[9].v = (texture->v1 + (texture->textureH >> 1)) / 255.0f;
        vertexPtr[10].u = (unsigned long)texture->u2 / 255.0f;
        vertexPtr[10].v = (unsigned long)texture->v2 / 255.0f;
        vertexPtr[11].u = (texture->u3 - ((texture->textureW * 4) >> 1)) / 255.0f;
        vertexPtr[11].v = (unsigned long)texture->v3 / 255.0f;
        vertexPtr[12].u = (texture->u0 + ((texture->textureW * 4) >> 1)) / 255.0f;
        vertexPtr[12].v = (texture->v0 + (texture->textureH >> 1)) / 255.0f;
        vertexPtr[13].u = (unsigned long)texture->u1 / 255.0f;
        vertexPtr[13].v = (texture->v1 + (texture->textureH >> 1)) / 255.0f;
        vertexPtr[14].u = (texture->u2 + ((texture->textureW * 4) >> 1)) / 255.0f;
        vertexPtr[14].v = (unsigned long)texture->v2 / 255.0f;
        vertexPtr[15].u = (unsigned long)texture->u3 / 255.0f;
        vertexPtr[15].v = (unsigned long)texture->v3 / 255.0f;
    }
}
#else
void RedBullAnimate(Object **objects, short objectCount, short unusedPeriod,
                    short firstTexture, short unusedTextureCount)
{
    Texture *texture;
    PCVertex *vertices;
    int count;
    int halfWidth, halfHeight;
    float left0, left1, left2, left3;
    float right0, right1, right2, right3;
    float top0, top1, top2, top3;
    float bottom0, bottom1, bottom2, bottom3;

    texture = textureTable[firstTexture];
    if (objectCount <= 0)
        return;
    halfWidth = (texture->textureW * 4) >> 1;
    halfHeight = (short)(texture->textureH >> 1);
    left0 = (unsigned long)texture->u0 * textureCoordinateScale;
    top0 = (unsigned long)texture->v0 * textureCoordinateScale;
    left1 = (texture->u1 - halfWidth) * textureCoordinateScale;
    top1 = (unsigned long)texture->v1 * textureCoordinateScale;
    left2 = (unsigned long)texture->u2 * textureCoordinateScale;
    top2 = (texture->v2 - halfHeight) * textureCoordinateScale;
    top3 = (texture->v3 - halfHeight) * textureCoordinateScale;
    left3 = (texture->u3 - halfWidth) * textureCoordinateScale;
    right0 = (texture->u0 + halfWidth) * textureCoordinateScale;
    right1 = (unsigned long)texture->u1 * textureCoordinateScale;
    bottom0 = (texture->v0 + halfHeight) * textureCoordinateScale;
    bottom1 = (texture->v1 + halfHeight) * textureCoordinateScale;
    right2 = (texture->u2 + halfWidth) * textureCoordinateScale;
    right3 = (unsigned long)texture->u3 * textureCoordinateScale;
    bottom2 = (unsigned long)texture->v2 * textureCoordinateScale;
    bottom3 = (unsigned long)texture->v3 * textureCoordinateScale;
    count = objectCount;
    do {
        vertices = (PCVertex *)(*objects)->floatVerts;
        vertices[0].u = left0; vertices[0].v = top0;
        vertices[1].u = left1; vertices[1].v = top1;
        vertices[2].u = left2; vertices[2].v = top2;
        vertices[3].u = left3; vertices[3].v = top3;
        vertices[4].u = right0; vertices[4].v = top0;
        vertices[5].u = right1; vertices[5].v = top1;
        vertices[6].u = right2; vertices[6].v = top2;
        vertices[7].u = right3; vertices[7].v = top3;
        vertices[8].u = left0; vertices[8].v = bottom0;
        vertices[9].u = left1; vertices[9].v = bottom1;
        vertices[10].u = left2; vertices[10].v = bottom2;
        vertices[11].u = left3; vertices[11].v = bottom3;
        vertices[12].u = right0; vertices[12].v = bottom0;
        vertices[13].u = right1; vertices[13].v = bottom1;
        vertices[14].u = right2; vertices[14].v = bottom2;
        vertices[15].u = right3; vertices[15].v = bottom3;
        objects++;
    } while (--count);
}
#endif
#endif
