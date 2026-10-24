#include "functions.h"
#include "d3dpoly.h"
#include "race_globals.h"
#include "sparks.h"
#if defined(NON_MATCHING) || defined(EQUIV)
#include <string.h>
#pragma intrinsic(strcpy, strcat)
#endif

extern int nextParticleSlot;
extern Texture *explodeTexture[];
extern POLY_FT4 explodePrims[EXPLOSION_COUNT];
#ifdef NETPLAY
extern char explosionAnimFrame;
#else
extern u_char explosionAnimFrame;
#endif
extern int missileFlareAngle, rocketFlareAngle;
#include "camera.h"
#ifdef NETPLAY
#pragma intrinsic(abs)
#endif
extern Explosion explosions[];
extern Ring rings[];
extern Bird *birds;
extern POLY_FT4 *birdPrims;
extern Texture *birdTexture[];
extern TrackSection *birdSectionA, *birdSectionB;
extern Electric electrics[];
extern int electricSlotIndex, rocketFlareCount, missileFlareCount;

SParticle *InitParticle(void)
{
    SParticle *particles;

    OutputDebugString("trying to alloc mem for particles\n");
    particles = DAlloc(heap, PARTICLE_COUNT * sizeof(SParticle));
    ClearParticleTable(particles);
    InitParticleColours();
    OutputDebugString("init particle done\n");
    return particles;
}

void AddRandomParticles(SParticle *particles, int count, VECTOR *pos, VECTOR *vel)
{
    int i;

#ifdef NETPLAY
    SetLocalRandom(0);
#endif
    while (count > 0) {
        i = nextParticleSlot;
#ifdef NETPLAY
        nextParticleSlot = (nextParticleSlot + 1) % PARTICLE_COUNT;
#else
        nextParticleSlot = (i + 1) % PARTICLE_COUNT;
#endif
        if (i > -1) {
            particles[i].pos.vx = pos->vx;
            particles[i].pos.vy = pos->vy;
            particles[i].pos.vz = pos->vz;
            particles[i].vel.vx = (short)((newRand() & 63) - 32 + vel->vx);
            particles[i].vel.vy = (newRand() & 63) - 32;
            particles[i].vel.vz = (short)((newRand() & 63) - 32 + vel->vz);
            particles[i].life = 7;
            particles[i].type = 1;
            particles[i].flags = 0;
        }
        count--;
    }
#ifdef NETPLAY
    SetLocalRandom(1);
#endif
}

void AddScrapingParticles(SParticle *particles, int count, VECTOR *pos, VECTOR *vel)
{
    int i;

#ifdef NETPLAY
    SetLocalRandom(0);
#endif
    while (count > 0) {
        i = nextParticleSlot;
        nextParticleSlot = (nextParticleSlot + 1) % PARTICLE_COUNT;
        if (i > -1) {
            particles[i].pos.vx = pos->vx;
            particles[i].pos.vy = pos->vy;
            particles[i].pos.vz = pos->vz;
            particles[i].vel.vx = (short)((newRand() & 63) - 32 + vel->vx);
            particles[i].vel.vy = (newRand() & 63) - 32;
            particles[i].vel.vz = (short)((newRand() & 63) - 32 + vel->vz);
            particles[i].life = 7;
            particles[i].type = 2;
            particles[i].flags = 0;
        }
        count--;
    }
#ifdef NETPLAY
    SetLocalRandom(1);
#endif
}

void AddSnow(SParticle *particles, int unused, VECTOR *camera)
{
#ifdef NETPLAY
    TrackSection *section;
    int n;
    int i;

    SetLocalRandom(0);
    section = cameraPtr->section;
    for (n = 0; n < 8; n++) {
        if (section->snow == 0) {
            i = nextParticleSlot;
            nextParticleSlot = (nextParticleSlot + 1) % PARTICLE_COUNT;
            if (i > -1) {
                particles[i].pos.vx = section->centre.vx + (newRand() & 8191) - 4096;
                particles[i].pos.vy = section->centre.vy - ((newRand() - 1000) & 4095);
                particles[i].pos.vz = section->centre.vz + (newRand() & 8191) - 4096;
                particles[i].vel.vx = 4;
                particles[i].vel.vy = 20;
                particles[i].vel.vz = 0;
                particles[i].life = 50;
                particles[i].type = 5;
                particles[i].flags = 0;
            }
        }
        section = section->nextSection.ptr;
    }
    section = cameraPtr->section->prevSection.ptr;
    for (n = 0; n < 7; n++) {
        if (section->snow == 0) {
            i = nextParticleSlot;
            nextParticleSlot = (nextParticleSlot + 1) % PARTICLE_COUNT;
            if (i > -1 && particles[i].type == 0) {
                particles[i].pos.vx = section->centre.vx + (newRand() & 8191) - 4096;
                particles[i].pos.vy = section->centre.vy - ((newRand() - 1) & 4095);
                particles[i].pos.vz = section->centre.vz + (newRand() & 8191) - 4096;
                particles[i].vel.vx = 4;
                particles[i].vel.vy = 20;
                particles[i].vel.vz = 0;
                particles[i].life = 50;
                particles[i].type = 5;
                particles[i].flags = 0;
            }
        }
        section = section->prevSection.ptr;
    }
    SetLocalRandom(1);
#else
    SParticle *particle;
    int i;
    int n;
    TrackSection *section;

#ifdef NETPLAY
    SetLocalRandom(0);
#endif
    section = cameraPtr->section;
    for (n = 8; n > 0; n--) {
        if (section->snow == 0) {
            i = nextParticleSlot;
#ifdef NETPLAY
            nextParticleSlot = (nextParticleSlot + 1) % PARTICLE_COUNT;
#else
            nextParticleSlot = (i + 1) % PARTICLE_COUNT;
#endif
            if (i > -1) {
                particle = &particles[i];
                particle->pos.vx = section->centre.vx + (newRand() & 8191) - 4096;
                particle->pos.vy = section->centre.vy - ((newRand() - 1000) & 4095);
                particle->pos.vz = section->centre.vz + (newRand() & 8191) - 4096;
                particles[i].vel.vx = 4;
                particles[i].vel.vy = 20;
                particles[i].vel.vz = 0;
                particle->life = 50;
                particle->type = 5;
                particles[i].flags = 0;
            }
        }
        section = section->nextSection.ptr;
    }
    section = cameraPtr->section->prevSection.ptr;
    for (n = 7; n > 0; n--) {
        if (section->snow == 0) {
            i = nextParticleSlot;
#ifdef NETPLAY
            nextParticleSlot = (nextParticleSlot + 1) % PARTICLE_COUNT;
#else
            nextParticleSlot = (i + 1) % PARTICLE_COUNT;
#endif
            if (i > -1) {
                particle = &particles[i];
                if (particle->type == 0) {
                    particle->pos.vx = section->centre.vx + (newRand() & 8191) - 4096;
                    particle->pos.vy = section->centre.vy - ((newRand() - 1) & 4095);
                    particle->pos.vz = section->centre.vz + (newRand() & 8191) - 4096;
                    particles[i].vel.vx = 4;
                    particles[i].vel.vy = 20;
                    particles[i].vel.vz = 0;
                    particle->life = 50;
                    particle->type = 5;
                    particles[i].flags = 0;
                }
            }
        }
        section = section->prevSection.ptr;
    }
#endif
}

void AddRain(SParticle *particles, int unused, VECTOR *camera)
{
    int i;
    int n;
    TrackSection *section;
    TrackSection *previous;

#ifdef NETPLAY
    SetLocalRandom(0);
#endif
    section = cameraPtr->section;
    for (n = 0; n < 8; n += 2) {
        if (section->snow == 0) {
            i = nextParticleSlot;
#ifdef NETPLAY
            nextParticleSlot = (nextParticleSlot + 1) % PARTICLE_COUNT;
#else
            nextParticleSlot = (i + 1) % PARTICLE_COUNT;
#endif
            if (i > -1) {
                particles[i].pos.vx = section->centre.vx + (newRand() & 8191) - 4096;
                particles[i].pos.vy = -camera->vy - (newRand() & 4095);
                particles[i].pos.vz = section->centre.vz + (newRand() & 8191) - 4096;
                particles[i].vel.vx = 160;
                particles[i].vel.vy = 480;
                particles[i].vel.vz = 0;
                particles[i].life = 20;
                particles[i].type = 3;
                particles[i].flags = 0;
            }
        }
        section = section->nextSection.ptr;
        section = section->nextSection.ptr;
    }
    previous = cameraPtr->section->prevSection.ptr;
    for (n = 0; n < 7; n += 2) {
        if (previous->snow == 0) {
            i = nextParticleSlot;
#ifdef NETPLAY
            nextParticleSlot = (nextParticleSlot + 1) % PARTICLE_COUNT;
#else
            nextParticleSlot = (i + 1) % PARTICLE_COUNT;
#endif
            if (i > -1) {
                particles[i].pos.vx = previous->centre.vx + (newRand() & 8191) - 4096;
                particles[i].pos.vy = -camera->vy - (newRand() & 4095);
                particles[i].pos.vz = previous->centre.vz + (newRand() & 8191) - 4096;
                particles[i].vel.vx = 160;
                particles[i].vel.vy = 480;
                particles[i].vel.vz = 0;
                particles[i].life = 20;
                particles[i].type = 3;
                particles[i].flags = 0;
            }
        }
        previous = previous->prevSection.ptr;
        previous = previous->prevSection.ptr;
    }
#ifdef NETPLAY
    SetLocalRandom(1);
#endif
}

void AddEboltParticles(SParticle *particles, int count, VECTOR *pos)
{
    int i;

#ifdef NETPLAY
    SetLocalRandom(0);
#endif
    while (count > 0) {
        i = nextParticleSlot;
        nextParticleSlot = (nextParticleSlot + 1) % PARTICLE_COUNT;
        if (i > -1) {
            particles[i].pos.vx = pos->vx;
            particles[i].pos.vy = pos->vy;
            particles[i].pos.vz = pos->vz;
            particles[i].vel.vx = (newRand() & 63) - 31;
            particles[i].vel.vy = (newRand() & 63) - 31;
            particles[i].vel.vz = (newRand() & 63) - 31;
            particles[i].life = 15;
            particles[i].type = 4;
            particles[i].flags = 0;
        }
        count--;
    }
#ifdef NETPLAY
    SetLocalRandom(1);
#endif
}

void AddRisingParticles(SParticle *particles, int count, VECTOR *pos, int fallSpeed)
{
    int i;

#ifdef NETPLAY
    SetLocalRandom(0);
#endif
    while (count > 0) {
        i = nextParticleSlot;
        nextParticleSlot = (nextParticleSlot + 1) % PARTICLE_COUNT;
        if (i > -1) {
            particles[i].pos.vx = pos->vx + newRand() % 1600 - 800;
            particles[i].pos.vy = pos->vy;
            particles[i].pos.vz = pos->vz + newRand() % 1600 - 800;
            particles[i].vel.vx = 0;
            particles[i].vel.vy = -(newRand() & 15) * fallSpeed;
            particles[i].vel.vz = 0;
            particles[i].life = (newRand() & 7) + 8;
            particles[i].type = 2;
            particles[i].flags = 0;
        }
        count--;
    }
#ifdef NETPLAY
    SetLocalRandom(1);
#endif
}

void AddPitParticles(SParticle *particles, int count, VECTOR *pos, int fallSpeed)
{
    int i;

#ifdef NETPLAY
    SetLocalRandom(0);
#endif
    while (count > 0) {
        i = nextParticleSlot;
        nextParticleSlot = (nextParticleSlot + 1) % PARTICLE_COUNT;
        if (i > -1) {
            particles[i].pos.vx = pos->vx + (newRand() & 511) - 256;
            particles[i].pos.vy = pos->vy + 256;
            particles[i].pos.vz = pos->vz + (newRand() & 511) - 256;
            particles[i].vel.vx = 0;
            particles[i].vel.vy = -(newRand() & 7) * fallSpeed;
            particles[i].vel.vz = 0;
            particles[i].life = (newRand() & 7) + 8;
            particles[i].type = 2;
            particles[i].flags = 0;
        }
        count--;
    }
#ifdef NETPLAY
    SetLocalRandom(1);
#endif
}

void ClearParticleTable(SParticle *particles)
{
    int i;

    OutputDebugString("init particle table\n");
    for (i = 0; i < PARTICLE_COUNT; i++) {
        particles[i].pos.vx = 0;
        particles[i].pos.vy = 0;
        particles[i].pos.vz = 0;
        particles[i].vel.vx = 0;
        particles[i].vel.vy = 0;
        particles[i].vel.vz = 0;
        particles[i].life = 0;
        particles[i].type = 0;
        particles[i].flags = 0;
    }
}

extern short TextureTableCount;
extern char wobbleDir3[];
extern Texture *textureTable[];
#include "heap_state.h"

#ifdef NETPLAY
#include <string.h>
#pragma intrinsic(strcpy, strcat)
void LoadExplodeGFX(void)
{
    int i;
    int textureIndex;

    textureIndex = TextureTableCount;
    {
        char fileName[256];
        TIMlist *sequence;

        strcpy(fileName, wobbleDir3);
        strcat(fileName, "explode.cmp");
        sequence = LoadCompressedTextureSequence(fileName);
        LoadCmpFiles(sequence);
    }
    for (i = 0; i < 32; i++) {
        explodeTexture[i] = textureTable[textureIndex];
        textureIndex++;
    }
    OutputDebugString("loaded explosion textures\n");
}
#endif
#ifndef NETPLAY
/* Keep the default build's string declarations here to preserve the earlier functions' compiler layout. */
#ifndef EQUIV
#ifndef NON_MATCHING
#include <string.h>
#pragma intrinsic(strcpy, strcat)
#endif
#endif
void LoadExplodeGFX(void)
{
    char fileName[256];
    int start;
    int i;

    start = TextureTableCount;
    strcpy(fileName, wobbleDir3);
    strcat(fileName, "explode.cmp");
    LoadCmpFiles(LoadCompressedTextureSequence(fileName));
    for (i = 0; i < 32; i++)
        explodeTexture[i] = textureTable[start + i];
    OutputDebugString("loaded explosion textures\n");
}
#endif

void InitBirds(void)
{
    int i;
    int birdIndex;
    int j;

#ifdef NETPLAY
    SetLocalRandom(0);
#endif
    OutputDebugString("doing init birds\n");
    birdSectionA = shipIndex[ownShip].nearTrkSect;
    birdSectionB = birdSectionA;
    for (i = 0; i < 50; i++)
        birdSectionB = birdSectionB->nextSection.ptr;
    birdPrims = DAlloc(reserveHeap, BIRD_COUNT * 2 * sizeof(POLY_FT4));
    birds = DAlloc(reserveHeap, BIRD_COUNT * sizeof(Bird));
    OutputDebugString("malloc ok...\n");
    for (birdIndex = 0; birdIndex < BIRD_COUNT; birdIndex++) {
        birds[birdIndex].x = (newRand() & 0xffff) - 0x8000;
        birds[birdIndex].y = -2048;
        birds[birdIndex].z = (newRand() & 0xffff) - 0x8000;
        birds[birdIndex].vx = 0;
        birds[birdIndex].vy = 0;
        birds[birdIndex].vz = 0;
        birds[birdIndex].accel = (newRand() & 7) + 4;
        birds[birdIndex].frame = newRand() & 31;
        for (j = 0; j < 2; j++) {
            SetPolyFT4(&birdPrims[birdIndex * 2 + j]);
            birdPrims[birdIndex * 2 + j].r0 = 128;
            birdPrims[birdIndex * 2 + j].g0 = 128;
            birdPrims[birdIndex * 2 + j].b0 = 128;
            birdPrims[birdIndex * 2 + j].x0 = 128;
            birdPrims[birdIndex * 2 + j].y0 = 64;
            birdPrims[birdIndex * 2 + j].x1 = 192;
            birdPrims[birdIndex * 2 + j].y1 = 64;
            birdPrims[birdIndex * 2 + j].x2 = 128;
            birdPrims[birdIndex * 2 + j].y2 = 128;
            birdPrims[birdIndex * 2 + j].x3 = 192;
            birdPrims[birdIndex * 2 + j].y3 = 128;
            birdPrims[birdIndex * 2 + j].u0 = birdTexture[0]->u0;
            birdPrims[birdIndex * 2 + j].v0 = birdTexture[0]->v0;
            birdPrims[birdIndex * 2 + j].u1 = birdTexture[0]->u1;
            birdPrims[birdIndex * 2 + j].v1 = birdTexture[0]->v1;
            birdPrims[birdIndex * 2 + j].u2 = birdTexture[0]->u2;
            birdPrims[birdIndex * 2 + j].v2 = birdTexture[0]->v2;
            birdPrims[birdIndex * 2 + j].u3 = birdTexture[0]->u3;
            birdPrims[birdIndex * 2 + j].v3 = birdTexture[0]->v3;
            birdPrims[birdIndex * 2 + j].tpage = GetTPage(0, 0, birdTexture[0]->textureX, birdTexture[0]->textureY);
            birdPrims[birdIndex * 2 + j].clut = birdTexture[0]->cba;
        }
    }
    OutputDebugString("init birds done\n");
#ifdef NETPLAY
    SetLocalRandom(1);
#endif
}

#ifdef NETPLAY
void LoadBirdGFX(void)
{
    int i;
    int textureIndex;

    textureIndex = TextureTableCount;
    {
        char fileName[256];
        TIMlist *sequence;

        strcpy(fileName, wobbleDir3);
        strcat(fileName, "bird.cmp");
        sequence = LoadCompressedTextureSequence(fileName);
        LoadCmpFiles(sequence);
    }
    for (i = 0; i < 32; i++) {
        birdTexture[i] = textureTable[textureIndex];
        textureIndex++;
    }
    OutputDebugString("loaded bird textures\n");
    InitBirds();
}
#endif
#ifndef NETPLAY
void LoadBirdGFX(void)
{
    char fileName[256];
    int start;
    int i;

    start = TextureTableCount;
    strcpy(fileName, wobbleDir3);
    strcat(fileName, "bird.cmp");
    LoadCmpFiles(LoadCompressedTextureSequence(fileName));
    for (i = 0; i < 32; i++)
        birdTexture[i] = textureTable[start + i];
    OutputDebugString("loaded bird textures\n");
    InitBirds();
}
#endif

void InitExplode(void)
{
    int i;

    OutputDebugString("doing init explode\n");
    for (i = 0; i < EXPLOSION_COUNT; i++) {
        explosions[i].x = 0;
        explosions[i].y = 0;
        explosions[i].z = 0;
        explosions[i].r = 128;
        explosions[i].g = 128;
        explosions[i].b = 128;
        explosions[i].flash = 0;
        explosions[i].ship = -1;
        explosions[i].size = 0;
        explosions[i].life = 0;
        explosions[i].palette = 0;
        SetPolyFT4(&explodePrims[i]);
        explodePrims[i].r0 = 128;
        explodePrims[i].g0 = 128;
        explodePrims[i].b0 = 128;
        explodePrims[i].x0 = 128;
        explodePrims[i].y0 = 64;
        explodePrims[i].x1 = 192;
        explodePrims[i].y1 = 64;
        explodePrims[i].x2 = 128;
        explodePrims[i].y2 = 128;
        explodePrims[i].x3 = 192;
        explodePrims[i].y3 = 128;
        explodePrims[i].u0 = explodeTexture[0]->u0;
        explodePrims[i].v0 = explodeTexture[0]->v0;
        explodePrims[i].u1 = explodeTexture[0]->u1;
        explodePrims[i].v1 = explodeTexture[0]->v1;
        explodePrims[i].u2 = explodeTexture[0]->u2;
        explodePrims[i].v2 = explodeTexture[0]->v2;
        explodePrims[i].u3 = explodeTexture[0]->u3;
        explodePrims[i].v3 = explodeTexture[0]->v3;
        explodePrims[i].tpage = GetTPage(1, 1, explodeTexture[0]->textureX, explodeTexture[0]->textureY);
        SetSemiTrans(&explodePrims[i], 1);
        explodePrims[i].clut = explodeTexture[0]->cba;
    }
    OutputDebugString("init explode done\n");
}

void UpdateExplode(void)
{
    int i;

    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    explosionAnimFrame++;
    for (i = 0; i < EXPLOSION_COUNT; i++) {
        if (explosions[i].life > 0) {
            if (explosions[i].blend == 1)
                explosions[i].life--;
            else {
                explosions[i].y -= 60;
                if (!(explosionAnimFrame & 1))
                    explosions[i].life--;
            }
        }
    }
}

#ifdef EQUIV
#include "pc_billboard.h"
#include "pc_particles.h"
extern PCBillboard explodeBillboards[EXPLOSION_COUNT];
extern int mmxAvailable, gameFlags;

#ifdef NETPLAY
#pragma intrinsic(memcpy)
/* NET build: NET_CLAMP_MATERIAL is a constant test kept by the original build (it leaves a jump over the empty else).
   The material index reads bit 8 of textureY only; z is truncated to short before the depth offset. */
#define NET_CLAMP_MATERIAL 1
void DrawExplode(void)
{
    PCBillboard *submitted;
    int frame;
    int count;
    short clamped;
    int i;
    VECTOR camera;
    int material;
    int x;
    int y;
    int z;

    count = 0;
    camera.vx = cameraPtr->camPos->relative.t[0];
    camera.vy = cameraPtr->camPos->relative.t[1];
    camera.vz = cameraPtr->camPos->relative.t[2];
    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    for (i = 0; i < EXPLOSION_COUNT; i++) {
        if (explosions[i].life <= 0)
            continue;
        if ((explosions[i].life >= 15 || explosions[i].flash != 1) &&
            (explosions[i].life >= 17 || explosions[i].flash != 0))
            continue;
        if (explosions[i].flash == 1)
            frame = 29 - explosions[i].life;
        else
            frame = 16 - explosions[i].life;
        if (explosions[i].ship > -1) {
            x = explosions[i].x + shipIndex[explosions[i].ship].ppivot.vx + camera.vx;
            y = explosions[i].y + shipIndex[explosions[i].ship].ppivot.vy + camera.vy;
            z = explosions[i].z + shipIndex[explosions[i].ship].ppivot.vz + camera.vz;
        } else {
            x = explosions[i].x + camera.vx;
            y = explosions[i].y + camera.vy;
            z = explosions[i].z + camera.vz;
        }
        if (abs(x) < 32768 && abs(z) < 32768) {
            material = (((explodeTexture[frame]->textureY & 0x100) >> 4) + (explodeTexture[frame]->textureX >> 6)) & 31;
            if (NET_CLAMP_MATERIAL) {
                if (material > 15 && material < 100)
                    material -= 10;
                else if (material < 16)
                    material -= 5;
            } else {
            }
            clamped = material;
            if (clamped > normalSetNo)
                clamped = normalSetNo;
            explodeBillboards[count].textureSet = clamped;
            explodeBillboards[count].x = x;
            explodeBillboards[count].y = y;
            explodeBillboards[count].z = (short)z - i;
            explodeBillboards[count].r = explosions[i].r;
            explodeBillboards[count].g = explosions[i].g;
            explodeBillboards[count].b = explosions[i].b;
            explodeBillboards[count].u0 = explodeTexture[frame]->u0 & 0xbf;
            explodeBillboards[count].u1 = explodeTexture[frame]->u1 & 0xbf;
            explodeBillboards[count].u2 = explodeTexture[frame]->u2 & 0xbf;
            explodeBillboards[count].u3 = explodeTexture[frame]->u3 & 0xbf;
            explodeBillboards[count].v0 = explodeTexture[frame]->v0;
            explodeBillboards[count].v1 = explodeTexture[frame]->v1;
            explodeBillboards[count].v2 = explodeTexture[frame]->v2;
            explodeBillboards[count].v3 = explodeTexture[frame]->v3;
            if (explosions[i].palette) {
                explodeBillboards[count].u0 += 64;
                explodeBillboards[count].u1 += 64;
                explodeBillboards[count].u2 += 64;
                explodeBillboards[count].u3 += 64;
            }
            explodeBillboards[count].widthScale = 512;
            explodeBillboards[count].heightScale = 2048;
            explodeBillboards[count].textureWidth = explodeTexture[frame]->u3 - explodeTexture[frame]->u0;
            explodeBillboards[count].textureHeight = explodeTexture[frame]->v3 - explodeTexture[frame]->v0;
            count++;
        }
    }
    if (count) {
        submitted = (PCBillboard *)trackVertexCursor;
        trackVertexCursor += count * sizeof(PCBillboard);
        memcpy(submitted, explodeBillboards, count * sizeof(PCBillboard));
        if (mmxAvailable || (rgbTextures && (gameFlags & 2)))
            AddRenderSkeleton(cameraPtr->camPos->super, 0, 0, 0, submitted, count, 1);
        else
            AddRenderSkeleton(cameraPtr->camPos->super, 0, 0, 0, submitted, count, 0);
    }
}
#else
/* Same computation; the exe's frame is 8 bytes larger (0x2c against 0x24). */
void DrawExplode(void)
{
    int frame;
    int count;
    int i;
    VECTOR camera;
    int x, y, z;
    int material;
    short clamped;
    Texture *texture;
    PCBillboard *submitted;

    count = 0;
    camera.vx = cameraPtr->camPos->relative.t[0];
    camera.vy = cameraPtr->camPos->relative.t[1];
    camera.vz = cameraPtr->camPos->relative.t[2];
    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    for (i = 0; i < EXPLOSION_COUNT; i++) {
        if (explosions[i].life <= 0)
            continue;
        if ((explosions[i].life >= 15 || explosions[i].flash != 1) &&
            (explosions[i].life >= 17 || explosions[i].flash != 0))
            continue;
        if (explosions[i].flash == 1)
            frame = 29 - explosions[i].life;
        else
            frame = 16 - explosions[i].life;
        if (explosions[i].ship > -1) {
            x = explosions[i].x + shipIndex[explosions[i].ship].ppivot.vx + camera.vx;
            y = explosions[i].y + shipIndex[explosions[i].ship].ppivot.vy + camera.vy;
            z = explosions[i].z + shipIndex[explosions[i].ship].ppivot.vz + camera.vz;
        } else {
            x = explosions[i].x + camera.vx;
            y = explosions[i].y + camera.vy;
            z = explosions[i].z + camera.vz;
        }
        if (abs(x) < 32768 && abs(z) < 32768) {
            texture = explodeTexture[frame];
            material = ((short)(texture->textureY & 0xff0f) >> 4) + (texture->textureX >> 6);
            material &= 31;
            if (material > 15 && material < 100)
                material -= 10;
            else if (material < 16)
                material -= 5;
            clamped = material;
            if (clamped > normalSetNo)
                clamped = normalSetNo;
            explodeBillboards[count].textureSet = clamped;
            explodeBillboards[count].x = x;
            explodeBillboards[count].y = y;
            explodeBillboards[count].z = z - i;
            explodeBillboards[count].r = explosions[i].r;
            explodeBillboards[count].g = explosions[i].g;
            explodeBillboards[count].b = explosions[i].b;
            explodeBillboards[count].u0 = texture->u0 & 0xbf;
            explodeBillboards[count].u1 = texture->u1 & 0xbf;
            explodeBillboards[count].u2 = texture->u2 & 0xbf;
            explodeBillboards[count].u3 = texture->u3 & 0xbf;
            explodeBillboards[count].v0 = texture->v0;
            explodeBillboards[count].v1 = texture->v1;
            explodeBillboards[count].v2 = texture->v2;
            explodeBillboards[count].v3 = texture->v3;
            if (explosions[i].palette) {
                explodeBillboards[count].u0 += 64;
                explodeBillboards[count].u1 += 64;
                explodeBillboards[count].u2 += 64;
                explodeBillboards[count].u3 += 64;
            }
            explodeBillboards[count].widthScale = 512;
            explodeBillboards[count].heightScale = 2048;
            explodeBillboards[count].textureWidth = texture->u3 - texture->u0;
            explodeBillboards[count].textureHeight = texture->v3 - texture->v0;
            count++;
        }
    }
    if (count) {
        submitted = (PCBillboard *)trackVertexCursor;
        trackVertexCursor += count * sizeof(PCBillboard);
        memcpy(submitted, explodeBillboards, count * sizeof(PCBillboard));
        if (mmxAvailable || (rgbTextures && (gameFlags & 2)))
            AddRenderSkeleton(cameraPtr->camPos->super, 0, 0, 0, submitted, count, 1);
        else
            AddRenderSkeleton(cameraPtr->camPos->super, 0, 0, 0, submitted, count, 0);
    }
}

#endif
#endif

/* Bird motion is updated independently of drawing on PC. */
/* NET build (MSVC /Od): the frame follows the names below. frame holds the bird's frame number while its phase is tested. */
#ifdef NETPLAY
void UpdateBirds(void)
{
    int birdX;
    int frame;
    int birdY, birdZ;
    int bird;

    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    for (bird = 0; bird < BIRD_COUNT; bird++) {
        frame = birds[bird].frame;
        if (frame != 8 || birds[bird].vx <= 0)
            birds[bird].frame = (birds[bird].frame + 2) & 31;
        if (bird < 15) {
            birdX = birdSectionA->centre.vx + 512;
            birdY = birdSectionA->centre.vy - 7400;
            birdZ = birdSectionA->centre.vz - 512;
        } else {
            birdX = birdSectionB->centre.vx + 512;
            birdY = birdSectionB->centre.vy - 7400;
            birdZ = birdSectionB->centre.vz - 512;
        }
        if (birds[bird].x > birdX)
            birds[bird].vx -= birds[bird].accel;
        if (birds[bird].x < birdX)
            birds[bird].vx += birds[bird].accel;
        if (birds[bird].y > birdY)
            birds[bird].vy -= birds[bird].accel * 2;
        if (birds[bird].y < birdY)
            birds[bird].vy += birds[bird].accel * 2;
        if (birds[bird].z > birdZ)
            birds[bird].vz -= birds[bird].accel;
        if (birds[bird].z < birdZ)
            birds[bird].vz += birds[bird].accel;
        if (birds[bird].vx > 320)
            birds[bird].vx = 320;
        if (birds[bird].vx < -320)
            birds[bird].vx = -320;
        /* The test is against 320, but an overshoot is set to 160 (not 320): the original's quirk. */
        if (birds[bird].vy > 320)
            birds[bird].vy = 160;
        if (birds[bird].vy < -320)
            birds[bird].vy = -160;
        if (birds[bird].vz > 320)
            birds[bird].vz = 320;
        if (birds[bird].vz < -320)
            birds[bird].vz = -320;
        birds[bird].x += birds[bird].vx;
        birds[bird].y += birds[bird].vy;
        birds[bird].z += birds[bird].vz;
    }
}
#else
void UpdateBirds(void)
{
    int i;
    int x, y, z;

    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    for (i = 0; i < BIRD_COUNT; i++) {
        if (birds[i].frame != 8 || birds[i].vx <= 0)
            birds[i].frame = (birds[i].frame + 2) & 31;
        if (i < 15) {
            x = birdSectionA->centre.vx + 512;
            y = birdSectionA->centre.vy - 7400;
            z = birdSectionA->centre.vz;
        } else {
            x = birdSectionB->centre.vx + 512;
            y = birdSectionB->centre.vy - 7400;
            z = birdSectionB->centre.vz;
        }
        z -= 512;
        if (birds[i].x > x)
            birds[i].vx -= birds[i].accel;
        if (birds[i].x < x)
            birds[i].vx += birds[i].accel;
        if (birds[i].y > y)
            birds[i].vy -= birds[i].accel * 2;
        if (birds[i].y < y)
            birds[i].vy += birds[i].accel * 2;
        if (birds[i].z > z)
            birds[i].vz -= birds[i].accel;
        if (birds[i].z < z)
            birds[i].vz += birds[i].accel;
        if (birds[i].vx > 320)
            birds[i].vx = 320;
        if (birds[i].vx < -320)
            birds[i].vx = -320;
        /* The test is against 320, but an overshoot is set to 160 (not 320): the original's quirk. */
        if (birds[i].vy > 320)
            birds[i].vy = 160;
        if (birds[i].vy < -320)
            birds[i].vy = -160;
        if (birds[i].vz > 320)
            birds[i].vz = 320;
        if (birds[i].vz < -320)
            birds[i].vz = -320;
        birds[i].x += birds[i].vx;
        birds[i].y += birds[i].vy;
        birds[i].z += birds[i].vz;
    }
}
#endif

extern short CurrentScreen;

#ifdef NETPLAY
/* NET build: the frame follows the names below (MSVC /Od). The polygon is indexed afresh for each store (no quad pointer), and prevX is declared but never used in the original. */
void DrawBirds(void)
{
    int idx;
    int py;
    int sxScreen;
    int frameIndex;
    long depthCue;
    long clipFlag;
    VECTOR camOffset;
    DVECTOR dvec;
    int depth;
    int zPos;
    int prevX;
    int x;
    int vy;
    SVECTOR vertex;
    short extent;

    camOffset.vx = cameraPtr->camPos->relative.t[0];
    camOffset.vy = cameraPtr->camPos->relative.t[1];
    camOffset.vz = cameraPtr->camPos->relative.t[2];
    for (idx = 0; idx < BIRD_COUNT; idx++) {
        frameIndex = birds[idx].frame;
        x = birds[idx].x + camOffset.vx;
        vy = birds[idx].y + camOffset.vy;
        zPos = birds[idx].z + camOffset.vz;
        if (abs(x) < 32768 && abs(zPos) < 32768) {
            vertex.vx = x;
            vertex.vy = vy;
            vertex.vz = zPos;
            depth = RotTransPers(&vertex, (long *)&dvec, &depthCue, &clipFlag);
            sxScreen = dvec.vx;
            py = dvec.vy;
            if (!(clipFlag & 0x80460000)) {
                extent = 51200 / depth;
                if (extent > 280)
                    extent = 280;
                sxScreen -= extent / 2;
                py -= extent / 2;
                birdPrims[idx * 2 + CurrentScreen].x0 = sxScreen;
                birdPrims[idx * 2 + CurrentScreen].y0 = py;
                birdPrims[idx * 2 + CurrentScreen].x1 = sxScreen + extent;
                birdPrims[idx * 2 + CurrentScreen].y1 = py;
                birdPrims[idx * 2 + CurrentScreen].x2 = sxScreen;
                birdPrims[idx * 2 + CurrentScreen].y2 = py + extent;
                birdPrims[idx * 2 + CurrentScreen].x3 = sxScreen + extent;
                birdPrims[idx * 2 + CurrentScreen].y3 = py + extent;
                birdPrims[idx * 2 + CurrentScreen].r0 = 96;
                birdPrims[idx * 2 + CurrentScreen].g0 = 96;
                birdPrims[idx * 2 + CurrentScreen].b0 = 96;
                birdPrims[idx * 2 + CurrentScreen].u0 = birdTexture[frameIndex]->u0;
                birdPrims[idx * 2 + CurrentScreen].v0 = birdTexture[frameIndex]->v0;
                birdPrims[idx * 2 + CurrentScreen].u1 = birdTexture[frameIndex]->u1;
                birdPrims[idx * 2 + CurrentScreen].v1 = birdTexture[frameIndex]->v1;
                birdPrims[idx * 2 + CurrentScreen].u2 = birdTexture[frameIndex]->u2;
                birdPrims[idx * 2 + CurrentScreen].v2 = birdTexture[frameIndex]->v2;
                birdPrims[idx * 2 + CurrentScreen].u3 = birdTexture[frameIndex]->u3;
                birdPrims[idx * 2 + CurrentScreen].v3 = birdTexture[frameIndex]->v3;
                birdPrims[idx * 2 + CurrentScreen].tpage = GetTPage(0, 0, birdTexture[frameIndex]->textureX, birdTexture[frameIndex]->textureY);
                birdPrims[idx * 2 + CurrentScreen].clut = birdTexture[frameIndex]->cba;
                DrawPolyFT4(0.1f, &birdPrims[idx * 2 + CurrentScreen], 1);
            }
        }
    }
}
#else
void DrawBirds(void)
{
    VECTOR camera;
    SVECTOR vertex;
    DVECTOR screen;
    long depthCue, flag;
    int i, frame;
    int x, y, z;
    int sx, sy;
    int depth;
    short size;
    POLY_FT4 *quad;
    Texture *texture;

    camera.vx = cameraPtr->camPos->relative.t[0];
    camera.vy = cameraPtr->camPos->relative.t[1];
    camera.vz = cameraPtr->camPos->relative.t[2];
    for (i = 0; i < BIRD_COUNT; i++) {
        frame = birds[i].frame;
        x = birds[i].x + camera.vx;
        y = birds[i].y + camera.vy;
        z = birds[i].z + camera.vz;
        if (abs(x) < 32768 && abs(z) < 32768) {
            vertex.vx = x;
            vertex.vy = y;
            vertex.vz = z;
            depth = RotTransPers(&vertex, (long *)&screen, &depthCue, &flag);
            sx = screen.vx;
            sy = screen.vy;
            if (!(flag & 0x80460000)) {
                size = 51200 / depth;
                if (size > 280)
                    size = 280;
                sx -= size / 2;
                sy -= size / 2;
                quad = &birdPrims[i * 2 + CurrentScreen];
                quad->x0 = sx;
                quad->y0 = sy;
                quad->x1 = sx + size;
                quad->y1 = sy;
                quad->x2 = sx;
                quad->y2 = sy + size;
                quad->x3 = sx + size;
                quad->y3 = sy + size;
                texture = birdTexture[frame];
                birdPrims[i * 2 + CurrentScreen].r0 = 96;
                birdPrims[i * 2 + CurrentScreen].g0 = 96;
                birdPrims[i * 2 + CurrentScreen].b0 = 96;
                birdPrims[i * 2 + CurrentScreen].u0 = texture->u0;
                birdPrims[i * 2 + CurrentScreen].v0 = texture->v0;
                birdPrims[i * 2 + CurrentScreen].u1 = texture->u1;
                birdPrims[i * 2 + CurrentScreen].v1 = texture->v1;
                birdPrims[i * 2 + CurrentScreen].u2 = texture->u2;
                birdPrims[i * 2 + CurrentScreen].v2 = texture->v2;
                birdPrims[i * 2 + CurrentScreen].u3 = texture->u3;
                birdPrims[i * 2 + CurrentScreen].v3 = texture->v3;
                quad->tpage = GetTPage(0, 0, texture->textureX, texture->textureY);
                quad = &birdPrims[i * 2 + CurrentScreen];
                quad->clut = birdTexture[frame]->cba;
                DrawPolyFT4(0.1f, quad, 1);
            }
        }
    }
}
#endif

extern POLY_G4 ringPrims[16][RING_COUNT];
extern POLY_G3 ringBluePrims[4][RING_COUNT];
extern POLY_G3 ringOrangePrims[4][RING_COUNT];
void InitRing(void)
{
    int i, j;

    for (i = 0; i < RING_COUNT; i++) {
        rings[i].x = 0;
        rings[i].y = 0;
        rings[i].z = 0;
        rings[i].life = 0;
        for (j = 0; j < 16; j++) {
            SetPolyG4(&ringPrims[j][i]);
            if (rgbTextures)
                ringPrims[j][i].r0 = 255;
            else
                ringPrims[j][i].r0 = 7;
            ringPrims[j][i].g0 = 255;
            ringPrims[j][i].b0 = 255;
            if (rgbTextures)
                ringPrims[j][i].r1 = 255;
            else
                ringPrims[j][i].r1 = 7;
            ringPrims[j][i].g1 = 255;
            ringPrims[j][i].b1 = 255;
            if (rgbTextures)
                ringPrims[j][i].r2 = 0;
            else
                ringPrims[j][i].r2 = 7;
            ringPrims[j][i].g2 = 0;
            ringPrims[j][i].b2 = 0;
            if (rgbTextures)
                ringPrims[j][i].r3 = 0;
            else
                ringPrims[j][i].r3 = 7;
            ringPrims[j][i].g3 = 0;
            ringPrims[j][i].b3 = 0;
            SetSemiTrans(&ringPrims[j][i], 1);
        }
    }
    for (i = 0; i < RING_COUNT; i++) {
        for (j = 0; j < 4; j++) {
            SetPolyG3(&ringBluePrims[j][i]);
            if (rgbTextures)
                ringBluePrims[j][i].r0 = 0;
            else
                ringBluePrims[j][i].r0 = 31;
            ringBluePrims[j][i].g0 = 0;
            ringBluePrims[j][i].b0 = 0;
            if (rgbTextures)
                ringBluePrims[j][i].r2 = 0;
            else
                ringBluePrims[j][i].r2 = 31;
            ringBluePrims[j][i].g2 = 0;
            ringBluePrims[j][i].b2 = 0;
            if (rgbTextures)
                ringBluePrims[j][i].r1 = 32;
            else
                ringBluePrims[j][i].r1 = 31;
            ringBluePrims[j][i].g1 = 64;
            ringBluePrims[j][i].b1 = 255;
            SetSemiTrans(&ringBluePrims[j][i], 1);
        }
    }
    for (i = 0; i < RING_COUNT; i++) {
        for (j = 0; j < 4; j++) {
            SetPolyG3(&ringOrangePrims[j][i]);
            if (rgbTextures)
                ringOrangePrims[j][i].r0 = 0;
            else
                ringOrangePrims[j][i].r0 = 35;
            ringOrangePrims[j][i].g0 = 0;
            ringOrangePrims[j][i].b0 = 0;
            if (rgbTextures)
                ringOrangePrims[j][i].r2 = 0;
            else
                ringOrangePrims[j][i].r2 = 35;
            ringOrangePrims[j][i].g2 = 0;
            ringOrangePrims[j][i].b2 = 0;
            if (rgbTextures)
                ringOrangePrims[j][i].r1 = 255;
            else
                ringOrangePrims[j][i].r1 = 35;
            ringOrangePrims[j][i].g1 = 64;
            ringOrangePrims[j][i].b1 = 32;
            SetSemiTrans(&ringOrangePrims[j][i], 1);
        }
    }
}


void UpdateRing(void)
{
    int i;

    missileFlareAngle = (missileFlareAngle + 160) & 4095;
    rocketFlareAngle = (rocketFlareAngle - 96) & 4095;
    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    for (i = 0; i < RING_COUNT; i++) {
        if (rings[i].life > 0) {
            rings[i].life--;
            rings[i].y -= 15;
        }
    }
}

#ifdef EQUIV
#include "pc_math.h"
#include "d3d.h"
#include "pc_projection.h"

extern const float electricCoordinateScale;

#ifdef NETPLAY
#include "mathinline.h"
#pragma intrinsic(sin, cos)
enum { RING_TRIG_TEST = 1 };
/* NET build (byte-exact target). The frame names are chosen so the locals sit in the exe's slots;
   the ring segment loop indexes ringPrims[slice][ring]. */
void DrawRing(void)
{
    float ty;
    int elapsed;
    int level;
    float txVec;
    long depthCue;
    short innerRadiusPos;
    float tz;
    POLY_G4 *q;
    float sxVal;
    float mat[16];
    float denom;
    int ring;
    float screenY;
    VECTOR camPos;
    int wedge;
    int s1;
    int cNext;
    int dist;
    long cullFlags;
    int cos0;
    int sin0;
    SVECTOR point;
    DVECTOR scr;
    short radiusIdx;

    camPos.vx = cameraPtr->camPos->relative.t[0];
    camPos.vy = cameraPtr->camPos->relative.t[1];
    camPos.vz = cameraPtr->camPos->relative.t[2];
    for (ring = 0; ring < RING_COUNT; ring++) {
        if (rings[ring].life > 0 && rings[ring].life <= 16) {
            elapsed = 16 - rings[ring].life;
            point.vx = rings[ring].x + camPos.vx;
            point.vy = rings[ring].y + camPos.vy;
            point.vz = rings[ring].z + camPos.vz;
            if (abs(rings[ring].x + camPos.vx) < 32768 && abs(rings[ring].z + camPos.vz) < 32768) {
                dist = RotTransPers(&point, (long *)&scr, &depthCue, &cullFlags);
                sxVal = point.vx / 4096.0f;
                screenY = point.vy / 4096.0f;
                denom = point.vz / 4096.0f;
                ConvertMatrix(mat, GetSkeletonMatrix(cameraPtr->camPos->super));
                txVec = ((mat[8] * denom) + mat[4] * screenY) + mat[0] * sxVal + mat[12];
                ty = ((mat[9] * denom) + mat[5] * screenY) + mat[1] * sxVal + mat[13];
                tz = ((mat[10] * denom) + mat[6] * screenY) + mat[2] * sxVal + mat[14];
                sxVal = ((viewMatrixFlipped.m[0][0] * txVec) + viewMatrixFlipped.m[2][0] * tz) + viewMatrixFlipped.m[1][0] * ty + viewMatrixFlipped.m[3][0];
                screenY = ((viewMatrixFlipped.m[0][1] * txVec) + viewMatrixFlipped.m[2][1] * tz) + viewMatrixFlipped.m[1][1] * ty + viewMatrixFlipped.m[3][1];
                denom = ((viewMatrixFlipped.m[0][2] * txVec) + viewMatrixFlipped.m[1][2] * ty) + viewMatrixFlipped.m[2][2] * tz + viewMatrixFlipped.m[3][2];
                txVec = projMatrix.m[0][0] * sxVal / denom * 2.0f;
                ty = projMatrix.m[1][1] * screenY / denom * 2.0f;
                tz = projMatrix.m[3][2] / denom + projMatrix.m[2][2];
                sxVal = (txVec + 1.0f) * 160.0f;
                screenY = (-ty + 1.0f) * 120.0f;
                if (!(cullFlags & 0x80460000) && denom > 0.0) {
                    radiusIdx = 51200 / dist;
                    if (radiusIdx > 256)
                        radiusIdx = 256;
                    radiusIdx = (radiusIdx * elapsed) / 2;
                    level = (16 - elapsed) << 4;
                    if (level > 255)
                        level = 255;
                    innerRadiusPos = radiusIdx / 2;
                    for (wedge = 0; wedge < 16; wedge++) {
                        /* Compile-time constant tests in the original build: each folds away, the empty else keeps the jump. */
                        if (RING_TRIG_TEST) {
                            sin0 = rsin(wedge * 256);
                        } else {
                        }
                        if (RING_TRIG_TEST) {
                            cos0 = rcos(wedge * 256);
                        } else {
                        }
                        if (RING_TRIG_TEST) {
                            s1 = rsin((wedge + 1) * 256);
                        } else {
                        }
                        if (RING_TRIG_TEST) {
                            cNext = rcos((wedge + 1) * 256);
                        } else {
                        }
                        q = &ringPrims[wedge][ring];
                        if (rgbTextures) {
                            q->r0 = level;
                            q->r1 = level;
                        } else {
                            q->r0 = 7;
                            q->r1 = 7;
                        }
                        q->g0 = level;
                        q->g1 = level;
                        q->b0 = level;
                        q->b1 = level;
                        q->x1 = ((sin0 * radiusIdx) >> 12) + sxVal;
                        q->y1 = ((cos0 * radiusIdx) >> 12) + screenY;
                        q->x0 = ((s1 * radiusIdx) >> 12) + sxVal;
                        q->y0 = ((cNext * radiusIdx) >> 12) + screenY;
                        q->x3 = ((sin0 * innerRadiusPos) >> 12) + sxVal;
                        q->y3 = ((cos0 * innerRadiusPos) >> 12) + screenY;
                        q->x2 = ((s1 * innerRadiusPos) >> 12) + sxVal;
                        q->y2 = ((cNext * innerRadiusPos) >> 12) + screenY;
                        DrawPolyG4(tz, &ringPrims[wedge][ring], 1);
                    }
                }
            }
        }
    }
}
#else
/* Same computation as the exe; sx, sy, sz hold the scaled camera point and are then reused for
   the screen position, so the frame slots line up with the exe's. */
void DrawRing(void)
{
    VECTOR camera;
    SVECTOR vertex;
    DVECTOR screen;
    long depthCue, flag;
    int i;
    int elapsed;
    int depth;
    int brightness;
    int angle;
    int sin0, cos0, sin1, cos1;
    short size, radius, innerRadius;
    float sx, sy, sz;
    float tx, ty, tz;
    float denominator, projectedDepth;
    float matrix[16];
    POLY_G4 *quad;

    camera.vx = cameraPtr->camPos->relative.t[0];
    camera.vy = cameraPtr->camPos->relative.t[1];
    camera.vz = cameraPtr->camPos->relative.t[2];
    for (i = 0; i < RING_COUNT; i++) {
        if (rings[i].life > 0 && rings[i].life <= 16) {
            elapsed = 16 - rings[i].life;
            vertex.vx = rings[i].x + camera.vx;
            vertex.vy = rings[i].y + camera.vy;
            vertex.vz = rings[i].z + camera.vz;
            if (abs(rings[i].x + camera.vx) < 32768 &&
                abs(rings[i].z + camera.vz) < 32768) {
                depth = RotTransPers(&vertex, (long *)&screen, &depthCue, &flag);
                sx = vertex.vx * electricCoordinateScale;
                sy = vertex.vy * electricCoordinateScale;
                sz = vertex.vz * electricCoordinateScale;
                ConvertMatrix(matrix, GetSkeletonMatrix(cameraPtr->camPos->super));
                tx = matrix[8] * sz + matrix[4] * sy + matrix[0] * sx + matrix[12];
                ty = matrix[9] * sz + matrix[5] * sy + matrix[1] * sx + matrix[13];
                tz = matrix[10] * sz + matrix[6] * sy + matrix[2] * sx + matrix[14];
                sx = viewMatrixFlipped.m[2][0] * tz + viewMatrixFlipped.m[1][0] * ty + viewMatrixFlipped.m[0][0] * tx + viewMatrixFlipped.m[3][0];
                sy = viewMatrixFlipped.m[2][1] * tz + viewMatrixFlipped.m[1][1] * ty + viewMatrixFlipped.m[0][1] * tx + viewMatrixFlipped.m[3][1];
                denominator = viewMatrixFlipped.m[2][2] * tz + viewMatrixFlipped.m[1][2] * ty + viewMatrixFlipped.m[0][2] * tx + viewMatrixFlipped.m[3][2];
                sx = ((sx / denominator * projMatrix.m[0][0]) * 2.0f + 1.0f) * 160.0f;
                projectedDepth = projMatrix.m[3][2] / denominator + projMatrix.m[2][2];
                sy = sy / denominator * projMatrix.m[1][1] * 2.0f;
                sy = (1.0f - sy) * 120.0f;
                if (!(flag & 0x80460000) && denominator > 0.0) {
                    size = 51200 / depth;
                    if (size > 256)
                        size = 256;
                    radius = (size * elapsed) / 2;
                    brightness = (16 - elapsed) << 4;
                    if (brightness > 255)
                        brightness = 255;
                    innerRadius = radius / 2;
                    quad = &ringPrims[0][i];
                    for (angle = 0; angle < 4096; angle += 256) {
                        sin0 = rsin(angle);
                        cos0 = rcos(angle);
                        sin1 = rsin(angle + 256);
                        cos1 = rcos(angle + 256);
                        if (rgbTextures) {
                            quad->r0 = brightness;
                            quad->r1 = brightness;
                        } else {
                            quad->r0 = 7;
                            quad->r1 = 7;
                        }
                        quad->g0 = brightness;
                        quad->g1 = brightness;
                        quad->b0 = brightness;
                        quad->b1 = brightness;
                        quad->x1 = ((sin0 * radius) >> 12) + sx;
                        quad->y1 = ((cos0 * radius) >> 12) + sy;
                        quad->x0 = ((sin1 * radius) >> 12) + sx;
                        quad->y0 = ((cos1 * radius) >> 12) + sy;
                        quad->x3 = ((sin0 * innerRadius) >> 12) + sx;
                        quad->y3 = ((cos0 * innerRadius) >> 12) + sy;
                        quad->x2 = ((sin1 * innerRadius) >> 12) + sx;
                        quad->y2 = ((cos1 * innerRadius) >> 12) + sy;
                        DrawPolyG4(projectedDepth, quad, 1);
                        quad += RING_COUNT;
                    }
                }
            }
        }
    }
}
#endif
#endif

void AddSpurtExplode(int x, int y, int z, int ship)
{
    int i;

    for (i = 0; i < EXPLOSION_COUNT; i++) {
        if (explosions[i].life <= 0) {
            explosions[i].life = 15;
            explosions[i].x = x;
            explosions[i].y = y;
            explosions[i].z = z;
            explosions[i].r = 128;
            explosions[i].g = 128;
            explosions[i].b = 128;
            explosions[i].blend = 1;
            explosions[i].flash = 1;
            explosions[i].ship = ship;
            explosions[i].size = 1;
            explosions[i].palette = 0;
            return;
        }
    }
}

void AddExplode(int x, int y, int z, int ship)
{
    int i;

    for (i = 0; i < EXPLOSION_COUNT; i++) {
        if (explosions[i].life <= 0) {
            explosions[i].life = 17;
            explosions[i].x = x;
            explosions[i].y = y;
            explosions[i].z = z;
            explosions[i].r = 128;
            explosions[i].g = 128;
            explosions[i].b = 128;
            explosions[i].blend = 1;
            explosions[i].flash = 0;
            explosions[i].ship = ship;
            explosions[i].size = 1;
            explosions[i].palette = 0;
            return;
        }
    }
}

void AddMinigunExplode(int x, int y, int z, int unused)
{
    int i;

    for (i = 0; i < EXPLOSION_COUNT; i++) {
        if (explosions[i].life <= 0) {
            explosions[i].life = 17;
            explosions[i].x = x;
            explosions[i].y = y;
            explosions[i].z = z;
            explosions[i].r = 32;
            explosions[i].g = 64;
            explosions[i].b = 192;
            explosions[i].blend = 1;
            explosions[i].ship = -1;
            explosions[i].flash = 1;
            explosions[i].size = 2;
            explosions[i].palette = 0;
            return;
        }
    }
}

void AddSmallSpurtExplode(int x, int y, int z, int ship)
{
    int i;

    for (i = 0; i < EXPLOSION_COUNT; i++) {
        if (explosions[i].life <= 0) {
            explosions[i].life = 17;
            explosions[i].x = x;
            explosions[i].y = y;
            explosions[i].z = z;
            explosions[i].r = 96;
            explosions[i].g = 96;
            explosions[i].b = 96;
            explosions[i].blend = 1;
            explosions[i].ship = ship;
            explosions[i].flash = 1;
            explosions[i].size = 3;
            explosions[i].palette = 0;
            return;
        }
    }
}

void AddSmallSmokeExplode(int x, int y, int z, int ship)
{
    int i;

    for (i = 0; i < EXPLOSION_COUNT; i++) {
        if (explosions[i].life <= 0) {
            explosions[i].life = 17;
            explosions[i].x = x;
            explosions[i].y = y;
            explosions[i].z = z;
            explosions[i].r = 16;
            explosions[i].g = 16;
            explosions[i].b = 32;
            explosions[i].blend = 2;
            explosions[i].ship = ship;
            explosions[i].flash = 0;
            explosions[i].size = 2;
            if (rgbTextures)
                explosions[i].palette = 0;
            else
                explosions[i].palette = 1;
            return;
        }
    }
}

void AddSmokeExplode(int x, int y, int z, int unused)
{
    int i;

    for (i = 0; i < EXPLOSION_COUNT; i++) {
        if (explosions[i].life <= 0) {
            explosions[i].life = 17;
            explosions[i].x = x;
            explosions[i].y = y;
            explosions[i].z = z;
            explosions[i].r = 16;
            explosions[i].g = 16;
            explosions[i].b = 32;
            explosions[i].blend = 2;
            explosions[i].flash = 0;
            explosions[i].ship = -1;
            explosions[i].size = 1;
            if (rgbTextures)
                explosions[i].palette = 0;
            else
                explosions[i].palette = 1;
            return;
        }
    }
}

void AddRing(int x, int y, int z, int unused)
{
    int i;

    for (i = 0; i < RING_COUNT; i++) {
        if (rings[i].life <= 0) {
            rings[i].life = 16;
            rings[i].x = x;
            rings[i].y = y;
            rings[i].z = z;
            return;
        }
    }
}


#ifdef EQUIV
#ifndef NETPLAY
/* Same computation as the exe; register allocation and frame layout differ. */
void DrawMissileFlare(int worldX, int worldY, int worldZ)
{
    VECTOR camera;
    SVECTOR vertex;
    DVECTOR screen;
    long depthCue, flag;
    int pass, depth;
    int savedAngle;
    short radius, halfRadius;
    float x, y, z, ty, tx, tz, sx, sy, denominator, projectedDepth;
    float matrix[16];

    camera.vx = cameraPtr->camPos->relative.t[0];
    camera.vy = cameraPtr->camPos->relative.t[1];
    camera.vz = cameraPtr->camPos->relative.t[2];
    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    savedAngle = missileFlareAngle;
    for (pass = 0; pass < 2; pass++, missileFlareAngle += 256) {
        if (missileFlareCount >= 8)
            return;
        missileFlareCount++;
        vertex.vx = worldX + camera.vx;
        vertex.vy = worldY + camera.vy;
        vertex.vz = worldZ + camera.vz;
        if (abs(worldX + camera.vx) >= 32768 ||
            abs(worldZ + camera.vz) >= 32768)
            continue;
        depth = RotTransPers(&vertex, (long *)&screen, &depthCue, &flag);
        x = vertex.vx * electricCoordinateScale;
        y = vertex.vy * electricCoordinateScale;
        z = vertex.vz * electricCoordinateScale;
        ConvertMatrix(matrix, GetSkeletonMatrix(cameraPtr->camPos->super));
        tx = matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12];
        ty = matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13];
        tz = matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14];
        sx = viewMatrixFlipped.m[2][0] * tz + viewMatrixFlipped.m[1][0] * ty + viewMatrixFlipped.m[0][0] * tx + viewMatrixFlipped.m[3][0];
        sy = viewMatrixFlipped.m[2][1] * tz + viewMatrixFlipped.m[1][1] * ty + viewMatrixFlipped.m[0][1] * tx + viewMatrixFlipped.m[3][1];
        denominator = viewMatrixFlipped.m[2][2] * tz + viewMatrixFlipped.m[1][2] * ty + viewMatrixFlipped.m[0][2] * tx + viewMatrixFlipped.m[3][2];
        projectedDepth = projMatrix.m[3][2] / denominator + projMatrix.m[2][2];
        sx = ((sx / denominator * projMatrix.m[0][0]) * 2.0f + 1.0f) * 160.0f;
        sy = sy * (projMatrix.m[1][1] / denominator) * 2.0f;
        sy = (1.0f - sy) * 120.0f;
        if (denominator > 0.0 && depth != 0) {
            radius = (short)(51200 / depth) >> 1;
            if (radius > 256)
                radius = 256;
            halfRadius = radius >> 1;
            projectedDepth *= projectedDepth;
            ringBluePrims[0][missileFlareCount].x0 = ((rsin(missileFlareAngle) * radius) >> 12) + sx;
            ringBluePrims[0][missileFlareCount].y0 = ((rcos(missileFlareAngle) * radius) >> 12) + sy;
            ringBluePrims[0][missileFlareCount].x2 = ((rsin(missileFlareAngle + 512) * radius) >> 12) + sx;
            ringBluePrims[0][missileFlareCount].y2 = ((rcos(missileFlareAngle + 512) * radius) >> 12) + sy;
            ringBluePrims[0][missileFlareCount].x1 = sx;
            ringBluePrims[0][missileFlareCount].y1 = sy;
            DrawPolyG3(projectedDepth, &ringBluePrims[0][missileFlareCount], 1);
            ringBluePrims[1][missileFlareCount].x0 = ((rsin(missileFlareAngle + 1024) * halfRadius) >> 12) + sx;
            ringBluePrims[1][missileFlareCount].y0 = ((rcos(missileFlareAngle + 1024) * halfRadius) >> 12) + sy;
            ringBluePrims[1][missileFlareCount].x2 = ((rsin(missileFlareAngle + 1536) * halfRadius) >> 12) + sx;
            ringBluePrims[1][missileFlareCount].y2 = ((rcos(missileFlareAngle + 1536) * halfRadius) >> 12) + sy;
            ringBluePrims[1][missileFlareCount].x1 = sx;
            ringBluePrims[1][missileFlareCount].y1 = sy;
            DrawPolyG3(projectedDepth, &ringBluePrims[1][missileFlareCount], 1);
            ringBluePrims[2][missileFlareCount].x0 = ((rsin(missileFlareAngle + 2048) * radius) >> 12) + sx;
            ringBluePrims[2][missileFlareCount].y0 = ((rcos(missileFlareAngle + 2048) * radius) >> 12) + sy;
            ringBluePrims[2][missileFlareCount].x2 = ((rsin(missileFlareAngle + 2560) * radius) >> 12) + sx;
            ringBluePrims[2][missileFlareCount].y2 = ((rcos(missileFlareAngle + 2560) * radius) >> 12) + sy;
            ringBluePrims[2][missileFlareCount].x1 = sx;
            ringBluePrims[2][missileFlareCount].y1 = sy;
            DrawPolyG3(projectedDepth, &ringBluePrims[2][missileFlareCount], 1);
            ringBluePrims[3][missileFlareCount].x0 = ((rsin(missileFlareAngle + 3072) * halfRadius) >> 12) + sx;
            ringBluePrims[3][missileFlareCount].y0 = ((rcos(missileFlareAngle + 3072) * halfRadius) >> 12) + sy;
            ringBluePrims[3][missileFlareCount].x2 = ((rsin(missileFlareAngle + 3584) * halfRadius) >> 12) + sx;
            ringBluePrims[3][missileFlareCount].y2 = ((rcos(missileFlareAngle + 3584) * halfRadius) >> 12) + sy;
            ringBluePrims[3][missileFlareCount].x1 = sx;
            ringBluePrims[3][missileFlareCount].y1 = sy;
            DrawPolyG3(projectedDepth, &ringBluePrims[3][missileFlareCount], 1);
        }
    }
    missileFlareAngle = savedAngle;
}

#endif
#endif
#ifdef NETPLAY
#include "pc_math.h"
#include "d3d.h"
#include "pc_projection.h"
#include "mathinline.h"
/* Tests on compile-time constants in the original build: each folds away, the empty else keeps the jump. */
enum { FLARE_HALF_RADIUS_TEST = 1, FLARE_RING_VERTEX_TEST = 1, FLARE_RING_DRAW_TEST = 1 };
#pragma intrinsic(sin, cos)
/* NET build (byte-exact). The transform sums are grouped as ((a) + b) + c + d, the grouping /Od
   needs for the exe's operand load order. */
void DrawMissileFlare(int worldX, int worldY, int worldZ)
{
    long cue;
    short halfRadius;
    float flareTx, flareY;
    int angle;
    float posY;
    float posX;
    float flareMatrix[16];
    float depthZ;
    VECTOR cam;
    float flareDenom;
    int side;
    long flagBits;
    int flareDepth;
    SVECTOR point;
    DVECTOR scr;
    short radii;

    cam.vx = cameraPtr->camPos->relative.t[0];
    cam.vy = cameraPtr->camPos->relative.t[1];
    cam.vz = cameraPtr->camPos->relative.t[2];
    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    angle = missileFlareAngle;
    for (side = 0; side < 2; side++) {
        if (missileFlareCount >= 8)
            return;
        missileFlareCount++;
        point.vx = worldX + cam.vx;
        point.vy = worldY + cam.vy;
        point.vz = worldZ + cam.vz;
        if (abs(worldX + cam.vx) < 32768 && abs(worldZ + cam.vz) < 32768) {
        flareDepth = RotTransPers(&point, (long *)&scr, &cue, &flagBits);
        posX = point.vx / 4096.0f;
        posY = point.vy / 4096.0f;
        flareDenom = point.vz / 4096.0f;
        ConvertMatrix(flareMatrix, GetSkeletonMatrix(cameraPtr->camPos->super));
        flareTx = flareMatrix[8] * flareDenom + flareMatrix[4] * posY + flareMatrix[0] * posX + flareMatrix[12];
        flareY = flareMatrix[9] * flareDenom + flareMatrix[5] * posY + flareMatrix[1] * posX + flareMatrix[13];
        posX = (depthZ = ((flareMatrix[2] * posX) + flareMatrix[10] * flareDenom) + flareMatrix[6] * posY + flareMatrix[14]) * viewMatrixFlipped.m[2][0] + viewMatrixFlipped.m[1][0] * flareY + viewMatrixFlipped.m[0][0] * flareTx + viewMatrixFlipped.m[3][0];
        posY = ((viewMatrixFlipped.m[2][1] * depthZ) + viewMatrixFlipped.m[1][1] * flareY) + viewMatrixFlipped.m[0][1] * flareTx + viewMatrixFlipped.m[3][1];
        flareDenom = ((viewMatrixFlipped.m[2][2] * depthZ) + viewMatrixFlipped.m[1][2] * flareY) + viewMatrixFlipped.m[0][2] * flareTx + viewMatrixFlipped.m[3][2];
        flareTx = projMatrix.m[0][0] * posX / flareDenom * 2.0f;
        flareY = projMatrix.m[1][1] * posY / flareDenom * 2.0f;
        depthZ = projMatrix.m[3][2] / flareDenom + projMatrix.m[2][2];
        posX = (flareTx + 1.0f) * 160.0f;
        posY = (-flareY + 1.0f) * 120.0f;
        if (flareDenom > 0.0 && flareDepth != 0) {
            radii = (short)(51200 / flareDepth);
            radii = radii >> 1;
            if (radii > 256)
                radii = 256;
            if (FLARE_HALF_RADIUS_TEST) {
                halfRadius = radii >> 1;
            } else {
            }
            if (FLARE_RING_VERTEX_TEST) {
                ringBluePrims[0][missileFlareCount].x0 = ((rsin(missileFlareAngle) * radii) >> 12) + posX;
            } else {
            }
            if (FLARE_RING_VERTEX_TEST) {
                ringBluePrims[0][missileFlareCount].y0 = ((rcos(missileFlareAngle) * radii) >> 12) + posY;
            } else {
            }
            if (FLARE_RING_VERTEX_TEST) {
                ringBluePrims[0][missileFlareCount].x2 = ((rsin(missileFlareAngle + 512) * radii) >> 12) + posX;
            } else {
            }
            ringBluePrims[0][missileFlareCount].y2 = ((rcos(missileFlareAngle + 512) * radii) >> 12) + posY;
            ringBluePrims[0][missileFlareCount].x1 = posX;
            ringBluePrims[0][missileFlareCount].y1 = posY;
            if (FLARE_RING_DRAW_TEST) {
                DrawPolyG3(depthZ * depthZ, &ringBluePrims[0][missileFlareCount], 1);
            } else {
            }
            if (FLARE_RING_VERTEX_TEST) {
                ringBluePrims[1][missileFlareCount].x0 = ((rsin(missileFlareAngle + 1024) * halfRadius) >> 12) + posX;
            } else {
            }
            if (FLARE_RING_VERTEX_TEST) {
                ringBluePrims[1][missileFlareCount].y0 = ((rcos(missileFlareAngle + 1024) * halfRadius) >> 12) + posY;
            } else {
            }
            if (FLARE_RING_VERTEX_TEST) {
                ringBluePrims[1][missileFlareCount].x2 = ((rsin(missileFlareAngle + 1536) * halfRadius) >> 12) + posX;
            } else {
            }
            ringBluePrims[1][missileFlareCount].y2 = ((rcos(missileFlareAngle + 1536) * halfRadius) >> 12) + posY;
            ringBluePrims[1][missileFlareCount].x1 = posX;
            ringBluePrims[1][missileFlareCount].y1 = posY;
            if (FLARE_RING_DRAW_TEST) {
                DrawPolyG3(depthZ * depthZ, &ringBluePrims[1][missileFlareCount], 1);
            } else {
            }
            if (FLARE_RING_VERTEX_TEST) {
                ringBluePrims[2][missileFlareCount].x0 = ((rsin(missileFlareAngle + 2048) * radii) >> 12) + posX;
            } else {
            }
            if (FLARE_RING_VERTEX_TEST) {
                ringBluePrims[2][missileFlareCount].y0 = ((rcos(missileFlareAngle + 2048) * radii) >> 12) + posY;
            } else {
            }
            if (FLARE_RING_VERTEX_TEST) {
                ringBluePrims[2][missileFlareCount].x2 = ((rsin(missileFlareAngle + 2560) * radii) >> 12) + posX;
            } else {
            }
            ringBluePrims[2][missileFlareCount].y2 = ((rcos(missileFlareAngle + 2560) * radii) >> 12) + posY;
            ringBluePrims[2][missileFlareCount].x1 = posX;
            ringBluePrims[2][missileFlareCount].y1 = posY;
            if (FLARE_RING_DRAW_TEST) {
                DrawPolyG3(depthZ * depthZ, &ringBluePrims[2][missileFlareCount], 1);
            } else {
            }
            if (FLARE_RING_VERTEX_TEST) {
                ringBluePrims[3][missileFlareCount].x0 = ((rsin(missileFlareAngle + 3072) * halfRadius) >> 12) + posX;
            } else {
            }
            if (FLARE_RING_VERTEX_TEST) {
                ringBluePrims[3][missileFlareCount].y0 = ((rcos(missileFlareAngle + 3072) * halfRadius) >> 12) + posY;
            } else {
            }
            if (FLARE_RING_VERTEX_TEST) {
                ringBluePrims[3][missileFlareCount].x2 = ((rsin(missileFlareAngle + 3584) * halfRadius) >> 12) + posX;
            } else {
            }
            ringBluePrims[3][missileFlareCount].y2 = ((rcos(missileFlareAngle + 3584) * halfRadius) >> 12) + posY;
            ringBluePrims[3][missileFlareCount].x1 = posX;
            ringBluePrims[3][missileFlareCount].y1 = posY;
            DrawPolyG3(depthZ * depthZ, &ringBluePrims[3][missileFlareCount], 1);
        }
        }
        missileFlareAngle += 256;
    }
    missileFlareAngle = angle;
}
/* NET build (byte-exact). Same shape as the missile body. The rocket's frame names are chosen so
   the locals sit in the exe's slots; viewZ and zScreen are reused for the projected depth and the
   divisor, as the exe reuses their slots. */
void DrawRocketFlare(int worldX, int worldY, int worldZ)
{
    float txCam;
    long cue;
    short halfRadius;
    float flareY;
    float xform[16];
    float viewZ;
    float projX;
    float zScreen;
    int pass;
    float projY;
    long outcode;
    VECTOR camPt;
    int camDepth;
    SVECTOR point;
    DVECTOR scr;
    int angle0;
    short radii;

    camPt.vx = cameraPtr->camPos->relative.t[0];
    camPt.vy = cameraPtr->camPos->relative.t[1];
    camPt.vz = cameraPtr->camPos->relative.t[2];
    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    angle0 = rocketFlareAngle;
    for (pass = 0; pass < 2; pass++) {
        if (rocketFlareCount >= 8)
            return;
        rocketFlareCount++;
        point.vx = camPt.vx + worldX;
        point.vy = camPt.vy + worldY;
        point.vz = camPt.vz + worldZ;
        if (abs(camPt.vx + worldX) < 32768 && abs(camPt.vz + worldZ) < 32768) {
            camDepth = RotTransPers(&point, (long *)&scr, &cue, &outcode);
            projX = point.vx / 4096.0f;
            projY = point.vy / 4096.0f;
            zScreen = point.vz / 4096.0f;
            ConvertMatrix(xform, GetSkeletonMatrix(cameraPtr->camPos->super));
            txCam = xform[8] * zScreen + xform[4] * projY + xform[0] * projX + xform[12];
            flareY = ((xform[9] * zScreen) + xform[5] * projY) + xform[1] * projX + xform[13];
            viewZ = xform[10] * zScreen + xform[6] * projY + xform[2] * projX + xform[14];
            projX = ((viewMatrixFlipped.m[0][0] * txCam) + viewMatrixFlipped.m[2][0] * viewZ) + viewMatrixFlipped.m[1][0] * flareY + viewMatrixFlipped.m[3][0];
            projY = ((viewMatrixFlipped.m[0][1] * txCam) + viewMatrixFlipped.m[2][1] * viewZ) + viewMatrixFlipped.m[1][1] * flareY + viewMatrixFlipped.m[3][1];
            zScreen = ((viewMatrixFlipped.m[0][2] * txCam) + viewMatrixFlipped.m[1][2] * flareY) + viewMatrixFlipped.m[2][2] * viewZ + viewMatrixFlipped.m[3][2];
            txCam = projMatrix.m[0][0] * projX / zScreen * 2.0f;
            flareY = projMatrix.m[1][1] * projY / zScreen * 2.0f;
            viewZ = projMatrix.m[3][2] / zScreen + projMatrix.m[2][2];
            projX = (txCam + 1.0f) * 160.0f;
            projY = (-flareY + 1.0f) * 120.0f;
            if (!(outcode & 0x80460000) && zScreen > 0.0) {
                radii = (short)(51200 / camDepth);
                radii = radii / 2;
                if (radii > 256)
                    radii = 256;
                if (FLARE_HALF_RADIUS_TEST) {
                    halfRadius = radii / 2;
                } else {
                }
                if (FLARE_RING_VERTEX_TEST) {
                    ringOrangePrims[0][rocketFlareCount].x0 = ((rsin(rocketFlareAngle) * radii) >> 12) + projX;
                } else {
                }
                if (FLARE_RING_VERTEX_TEST) {
                    ringOrangePrims[0][rocketFlareCount].y0 = ((rcos(rocketFlareAngle) * radii) >> 12) + projY;
                } else {
                }
                if (FLARE_RING_VERTEX_TEST) {
                    ringOrangePrims[0][rocketFlareCount].x2 = ((rsin(rocketFlareAngle + 512) * radii) >> 12) + projX;
                } else {
                }
                ringOrangePrims[0][rocketFlareCount].y2 = ((rcos(rocketFlareAngle + 512) * radii) >> 12) + projY;
                ringOrangePrims[0][rocketFlareCount].x1 = projX;
                ringOrangePrims[0][rocketFlareCount].y1 = projY;
                if (FLARE_RING_DRAW_TEST) {
                    DrawPolyG3(viewZ * viewZ, &ringOrangePrims[0][rocketFlareCount], 1);
                } else {
                }
                if (FLARE_RING_VERTEX_TEST) {
                    ringOrangePrims[1][rocketFlareCount].x0 = ((rsin(rocketFlareAngle + 1024) * halfRadius) >> 12) + projX;
                } else {
                }
                if (FLARE_RING_VERTEX_TEST) {
                    ringOrangePrims[1][rocketFlareCount].y0 = ((rcos(rocketFlareAngle + 1024) * halfRadius) >> 12) + projY;
                } else {
                }
                if (FLARE_RING_VERTEX_TEST) {
                    ringOrangePrims[1][rocketFlareCount].x2 = ((rsin(rocketFlareAngle + 1536) * halfRadius) >> 12) + projX;
                } else {
                }
                ringOrangePrims[1][rocketFlareCount].y2 = ((rcos(rocketFlareAngle + 1536) * halfRadius) >> 12) + projY;
                ringOrangePrims[1][rocketFlareCount].x1 = projX;
                ringOrangePrims[1][rocketFlareCount].y1 = projY;
                if (FLARE_RING_DRAW_TEST) {
                    DrawPolyG3(viewZ * viewZ, &ringOrangePrims[1][rocketFlareCount], 1);
                } else {
                }
                if (FLARE_RING_VERTEX_TEST) {
                    ringOrangePrims[2][rocketFlareCount].x0 = ((rsin(rocketFlareAngle + 2048) * radii) >> 12) + projX;
                } else {
                }
                if (FLARE_RING_VERTEX_TEST) {
                    ringOrangePrims[2][rocketFlareCount].y0 = ((rcos(rocketFlareAngle + 2048) * radii) >> 12) + projY;
                } else {
                }
                if (FLARE_RING_VERTEX_TEST) {
                    ringOrangePrims[2][rocketFlareCount].x2 = ((rsin(rocketFlareAngle + 2560) * radii) >> 12) + projX;
                } else {
                }
                ringOrangePrims[2][rocketFlareCount].y2 = ((rcos(rocketFlareAngle + 2560) * radii) >> 12) + projY;
                ringOrangePrims[2][rocketFlareCount].x1 = projX;
                ringOrangePrims[2][rocketFlareCount].y1 = projY;
                if (FLARE_RING_DRAW_TEST) {
                    DrawPolyG3(viewZ * viewZ, &ringOrangePrims[2][rocketFlareCount], 1);
                } else {
                }
                if (FLARE_RING_VERTEX_TEST) {
                    ringOrangePrims[3][rocketFlareCount].x0 = ((rsin(rocketFlareAngle + 3072) * halfRadius) >> 12) + projX;
                } else {
                }
                if (FLARE_RING_VERTEX_TEST) {
                    ringOrangePrims[3][rocketFlareCount].y0 = ((rcos(rocketFlareAngle + 3072) * halfRadius) >> 12) + projY;
                } else {
                }
                if (FLARE_RING_VERTEX_TEST) {
                    ringOrangePrims[3][rocketFlareCount].x2 = ((rsin(rocketFlareAngle + 3584) * halfRadius) >> 12) + projX;
                } else {
                }
                ringOrangePrims[3][rocketFlareCount].y2 = ((rcos(rocketFlareAngle + 3584) * halfRadius) >> 12) + projY;
                ringOrangePrims[3][rocketFlareCount].x1 = projX;
                ringOrangePrims[3][rocketFlareCount].y1 = projY;
                DrawPolyG3(viewZ * viewZ, &ringOrangePrims[3][rocketFlareCount], 1);
            }
        }
        rocketFlareAngle += 256;
    }
    rocketFlareAngle = angle0;
}
#undef rsin
#undef rcos
#pragma function(sin, cos)
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Same computation as the exe. sx, sy, sz hold the scaled camera point and are then reused for
   the screen position, so the frame slots line up with the exe's. */
void DrawRocketFlare(int worldX, int worldY, int worldZ)
{
    SVECTOR vertex;
    DVECTOR screen;
    long depthCue, flag;
    int pass, depth;
    int savedAngle;
    short radius, halfRadius;
    float sx, sz, sy, tx, ty, tz, denominator, projectedDepth;
    float matrix[16];
    VECTOR camera;

    camera.vx = cameraPtr->camPos->relative.t[0];
    camera.vy = cameraPtr->camPos->relative.t[1];
    camera.vz = cameraPtr->camPos->relative.t[2];
    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    savedAngle = rocketFlareAngle;
    for (pass = 0; pass < 2; pass++, rocketFlareAngle += 256) {
        if (rocketFlareCount >= 8)
            return;
        rocketFlareCount++;
        vertex.vx = worldX + camera.vx;
        vertex.vy = worldY + camera.vy;
        vertex.vz = worldZ + camera.vz;
        if (abs(worldX + camera.vx) >= 32768 ||
            abs(worldZ + camera.vz) >= 32768)
            continue;
        depth = RotTransPers(&vertex, (long *)&screen, &depthCue, &flag);
        sx = vertex.vx * electricCoordinateScale;
        sy = vertex.vy * electricCoordinateScale;
        sz = vertex.vz * electricCoordinateScale;
        ConvertMatrix(matrix, GetSkeletonMatrix(cameraPtr->camPos->super));
        tx = matrix[8] * sz + matrix[4] * sy + matrix[0] * sx + matrix[12];
        ty = matrix[9] * sz + matrix[5] * sy + matrix[1] * sx + matrix[13];
        tz = matrix[10] * sz + matrix[6] * sy + matrix[2] * sx + matrix[14];
        sx = viewMatrixFlipped.m[2][0] * tz + viewMatrixFlipped.m[1][0] * ty + viewMatrixFlipped.m[0][0] * tx + viewMatrixFlipped.m[3][0];
        sy = viewMatrixFlipped.m[2][1] * tz + viewMatrixFlipped.m[1][1] * ty + viewMatrixFlipped.m[0][1] * tx + viewMatrixFlipped.m[3][1];
        denominator = viewMatrixFlipped.m[2][2] * tz + viewMatrixFlipped.m[1][2] * ty + viewMatrixFlipped.m[0][2] * tx + viewMatrixFlipped.m[3][2];
        sx = ((sx / denominator * projMatrix.m[0][0]) * 2.0f + 1.0f) * 160.0f;
        projectedDepth = projMatrix.m[3][2] / denominator + projMatrix.m[2][2];
        sy = sy / denominator * projMatrix.m[1][1] * 2.0f;
        sy = (1.0f - sy) * 120.0f;
        if (!(flag & 0x80460000) && denominator > 0.0) {
            radius = (short)(51200 / depth) / 2;
            if (radius > 256)
                radius = 256;
            halfRadius = radius / 2;
            projectedDepth *= projectedDepth;
            ringOrangePrims[0][rocketFlareCount].x0 = ((rsin(rocketFlareAngle) * radius) >> 12) + sx;
            ringOrangePrims[0][rocketFlareCount].y0 = ((rcos(rocketFlareAngle) * radius) >> 12) + sy;
            ringOrangePrims[0][rocketFlareCount].x2 = ((rsin(rocketFlareAngle + 512) * radius) >> 12) + sx;
            ringOrangePrims[0][rocketFlareCount].y2 = ((rcos(rocketFlareAngle + 512) * radius) >> 12) + sy;
            ringOrangePrims[0][rocketFlareCount].x1 = sx;
            ringOrangePrims[0][rocketFlareCount].y1 = sy;
            DrawPolyG3(projectedDepth, &ringOrangePrims[0][rocketFlareCount], 1);
            ringOrangePrims[1][rocketFlareCount].x0 = ((rsin(rocketFlareAngle + 1024) * halfRadius) >> 12) + sx;
            ringOrangePrims[1][rocketFlareCount].y0 = ((rcos(rocketFlareAngle + 1024) * halfRadius) >> 12) + sy;
            ringOrangePrims[1][rocketFlareCount].x2 = ((rsin(rocketFlareAngle + 1536) * halfRadius) >> 12) + sx;
            ringOrangePrims[1][rocketFlareCount].y2 = ((rcos(rocketFlareAngle + 1536) * halfRadius) >> 12) + sy;
            ringOrangePrims[1][rocketFlareCount].x1 = sx;
            ringOrangePrims[1][rocketFlareCount].y1 = sy;
            DrawPolyG3(projectedDepth, &ringOrangePrims[1][rocketFlareCount], 1);
            ringOrangePrims[2][rocketFlareCount].x0 = ((rsin(rocketFlareAngle + 2048) * radius) >> 12) + sx;
            ringOrangePrims[2][rocketFlareCount].y0 = ((rcos(rocketFlareAngle + 2048) * radius) >> 12) + sy;
            ringOrangePrims[2][rocketFlareCount].x2 = ((rsin(rocketFlareAngle + 2560) * radius) >> 12) + sx;
            ringOrangePrims[2][rocketFlareCount].y2 = ((rcos(rocketFlareAngle + 2560) * radius) >> 12) + sy;
            ringOrangePrims[2][rocketFlareCount].x1 = sx;
            ringOrangePrims[2][rocketFlareCount].y1 = sy;
            DrawPolyG3(projectedDepth, &ringOrangePrims[2][rocketFlareCount], 1);
            ringOrangePrims[3][rocketFlareCount].x0 = ((rsin(rocketFlareAngle + 3072) * halfRadius) >> 12) + sx;
            ringOrangePrims[3][rocketFlareCount].y0 = ((rcos(rocketFlareAngle + 3072) * halfRadius) >> 12) + sy;
            ringOrangePrims[3][rocketFlareCount].x2 = ((rsin(rocketFlareAngle + 3584) * halfRadius) >> 12) + sx;
            ringOrangePrims[3][rocketFlareCount].y2 = ((rcos(rocketFlareAngle + 3584) * halfRadius) >> 12) + sy;
            ringOrangePrims[3][rocketFlareCount].x1 = sx;
            ringOrangePrims[3][rocketFlareCount].y1 = sy;
            DrawPolyG3(projectedDepth, &ringOrangePrims[3][rocketFlareCount], 1);
        }
    }
    rocketFlareAngle = savedAngle;
}
#endif
#endif

void InitElectric(void)
{
    int i;
    int j;

    for (i = 0; i < ELECTRIC_COUNT; i++) {
        for (j = 0; j < 2; j++) {
            SetLineG2(&electrics[i].line1[j]);
            SetLineG2(&electrics[i].line2[j]);
            SetLineG2(&electrics[i].line3[j]);
        }
        electrics[i].life = -1;
    }
}

void AddElectric(VECTOR *position, int mode)
{
    int index;

    electricSlotIndex++;
    if (electricSlotIndex >= ELECTRIC_COUNT)
        return;
#ifdef NETPLAY
    SetLocalRandom(0);
#endif
    index = electricSlotIndex;
    if (mode == 0) {
        electrics[index].start.vx = (newRand() & 255) - 128;
        electrics[index].start.vy = (newRand() & 255) - 128;
        electrics[index].start.vz = (newRand() & 255) - 128;
        electrics[index].end.vx = electrics[index].start.vx * 4 + position->vx;
        electrics[index].end.vy = electrics[index].start.vy * 4 + position->vy;
        electrics[index].end.vz = electrics[index].start.vz * 4 + position->vz;
        electrics[index].start.vx += position->vx;
        electrics[index].start.vy += position->vy;
        electrics[index].start.vz += position->vz;
    } else {
        electrics[index].start.vx = newRand() % 480 - 240;
        electrics[index].start.vy = position->vy;
        electrics[index].start.vz = newRand() % 480 - 240;
        electrics[index].end.vx = electrics[index].start.vx * 4 + position->vx;
        electrics[index].end.vy = position->vy + 320;
        electrics[index].end.vz = electrics[index].start.vz * 4 + position->vz;
        electrics[index].start.vx += position->vx;
        electrics[index].start.vz += position->vz;
    }
    electrics[index].jitter[0] = newRand();
    electrics[index].jitter[1] = newRand();
    electrics[index].jitter[2] = newRand();
    electrics[index].jitter[3] = newRand();
    electrics[index].jitter[4] = newRand();
    electrics[index].jitter[5] = newRand();
    if (mode == 2) {
        electrics[index].r1 = 192;
        electrics[index].g1 = 0;
        electrics[index].b1 = 0;
        electrics[index].r2 = 255;
        electrics[index].g2 = 250;
        electrics[index].b2 = 0;
    } else {
        electrics[index].r1 = 0;
        electrics[index].g1 = 0;
        electrics[index].b1 = 255;
        electrics[index].r2 = 255;
        electrics[index].g2 = 255;
        electrics[index].b2 = 255;
    }
#ifdef NETPLAY
    SetLocalRandom(1);
#endif
}

#ifdef EQUIV
#include "pc_vertex.h"
#include "d3d.h"

#include "pc_particles.h"

extern const float electricCoordinateScale;
extern const float electricWidth;
extern const float electricMiddleWidth;


/* NET build (MSVC /Od): the scale is a division by 4096 and the width interpolation divides by its constant, as the NET exe does. */
#ifdef NETPLAY
void AddElectricStreak(VECTOR *unused, int unusedMode)
{
    PCVertex *vertices;
    ParticleTriangleList *triangleLists;
    VECTOR startPt;
    VECTOR center;
    VECTOR endPoint;
    int i;
    VECTOR camPt;

    electricSlotIndex++;
    if (electricSlotIndex >= ELECTRIC_COUNT)
        return;
    for (i = 0; i <= normalSetNo; i++)
        quadBuckets[i].count = 0;
    i = electricSlotIndex;
    camPt.vx = cameraPtr->camPos->relative.t[0];
    camPt.vy = cameraPtr->camPos->relative.t[1];
    camPt.vz = cameraPtr->camPos->relative.t[2];
    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    startPt.vx = electrics[i].start.vx + camPt.vx;
    startPt.vy = electrics[i].start.vy + camPt.vy;
    startPt.vz = electrics[i].start.vz + camPt.vz;
    if (abs(startPt.vx) < 32768 && abs(startPt.vz) < 32768) {
        trailStage[0].x = startPt.vx / 4096.0f;
        trailStage[0].y = startPt.vy / 4096.0f;
        trailStage[0].z = startPt.vz / 4096.0f;
        trailStage[0].u = 0;
        trailStage[0].v = 0;
        trailStage[0].a = particleAlpha;
        trailStage[0].r = electrics[i].r1;
        trailStage[0].g = electrics[i].g1;
        trailStage[0].b = electrics[i].b1;
        endPoint.vx = electrics[i].end.vx + camPt.vx;
        endPoint.vy = electrics[i].end.vy + camPt.vy;
        endPoint.vz = electrics[i].end.vz + camPt.vz;
        electrics[i].life = -1;
        trailStage[1].x = endPoint.vx / 4096.0f;
        trailStage[1].y = endPoint.vy / 4096.0f;
        trailStage[1].z = endPoint.vz / 4096.0f;
        trailStage[1].u = 0;
        trailStage[1].v = 0;
        trailStage[1].a = particleAlpha;
        trailStage[1].r = electrics[i].r1;
        trailStage[1].g = electrics[i].g1;
        trailStage[1].b = electrics[i].b1;
        /* These interpolations use the midpoint values already in the vertex buffer. */
        trailStage[2].x = trailStage[1].x +
            (trailStage[3].x - trailStage[1].x) / 40.0f;
        trailStage[2].y = trailStage[1].y +
            (trailStage[3].y - trailStage[1].y) / 40.0f;
        trailStage[2].z = trailStage[1].z +
            (trailStage[3].z - trailStage[1].z) / 40.0f;
        trailStage[2].u = 0;
        trailStage[2].v = 0;
        trailStage[2].a = particleAlpha;
        trailStage[2].r = electrics[i].r1;
        trailStage[2].g = electrics[i].g1;
        trailStage[2].b = electrics[i].b1;
        center.vx = (endPoint.vx + startPt.vx) / 2;
        center.vy = (endPoint.vy + startPt.vy) / 2;
        center.vz = (endPoint.vz + startPt.vz) / 2;
        center.vx += (electrics[i].jitter[0] & 127) - 63;
        center.vy += (electrics[i].jitter[1] & 127) - 63;
        center.vz += (electrics[i].jitter[2] & 127) - 63;
        trailStage[3].x = center.vx / 4096.0f;
        trailStage[3].y = center.vy / 4096.0f;
        trailStage[3].z = center.vz / 4096.0f;
        trailStage[3].u = 0;
        trailStage[3].v = 0;
        trailStage[3].a = particleAlpha;
        trailStage[3].r = electrics[i].r2;
        trailStage[3].g = electrics[i].g2;
        trailStage[3].b = electrics[i].b2;
        trailStage[4].x = trailStage[3].x +
            (trailStage[5].x - trailStage[3].x) / 80.0f;
        trailStage[4].y = trailStage[3].y +
            (trailStage[5].y - trailStage[3].y) / 80.0f;
        trailStage[4].z = trailStage[3].z +
            (trailStage[5].z - trailStage[3].z) / 80.0f;
        trailStage[4].u = 0;
        trailStage[4].v = 0;
        trailStage[4].a = particleAlpha;
        trailStage[4].r = (electrics[i].r1 + electrics[i].r2) / 2;
        trailStage[4].g = (electrics[i].g1 + electrics[i].g2) / 2;
        trailStage[4].b = (electrics[i].b1 + electrics[i].b2) / 2;
        center.vx = (endPoint.vx + startPt.vx) / 2;
        center.vy = (endPoint.vy + startPt.vy) / 2;
        center.vz = (endPoint.vz + startPt.vz) / 2;
        center.vx += (electrics[i].jitter[3] & 127) - 63;
        center.vy += (electrics[i].jitter[4] & 127) - 63;
        center.vz += (electrics[i].jitter[5] & 127) - 63;
        trailStage[5].x = center.vx / 4096.0f;
        trailStage[5].y = center.vy / 4096.0f;
        trailStage[5].z = center.vz / 4096.0f;
        trailStage[5].u = 0;
        trailStage[5].v = 0;
        trailStage[5].a = particleAlpha;
        trailStage[5].r = electrics[i].r2;
        trailStage[5].g = electrics[i].g2;
        trailStage[5].b = electrics[i].b2;
        trailStage[6].x = trailStage[3].x +
            (trailStage[1].x - trailStage[3].x) / 40.0f;
        trailStage[6].y = trailStage[3].y +
            (trailStage[1].y - trailStage[3].y) / 40.0f;
        trailStage[6].z = trailStage[3].z +
            (trailStage[1].z - trailStage[3].z) / 40.0f;
        trailStage[6].u = 0;
        trailStage[6].v = 0;
        trailStage[6].a = particleAlpha;
        trailStage[6].r = electrics[i].r2;
        trailStage[6].g = electrics[i].g2;
        trailStage[6].b = electrics[i].b2;
        quadBuckets[normalSetNo].triangles[0].v1 = 0;
        quadBuckets[normalSetNo].triangles[0].v2 = 1;
        quadBuckets[normalSetNo].triangles[0].v3 = 2;
        quadBuckets[normalSetNo].triangles[1].v1 = 1;
        quadBuckets[normalSetNo].triangles[1].v2 = 3;
        quadBuckets[normalSetNo].triangles[1].v3 = 4;
        quadBuckets[normalSetNo].triangles[2].v1 = 3;
        quadBuckets[normalSetNo].triangles[2].v2 = 5;
        quadBuckets[normalSetNo].triangles[2].v3 = 6;
        quadBuckets[normalSetNo].count = 3;
        vertices = (PCVertex *)trackVertexCursor;
        trackVertexCursor += 7 * sizeof(PCVertex);
        memcpy(vertices, trailStage, 7 * sizeof(PCVertex));
        triangleLists = (ParticleTriangleList *)trackVertexCursor;
        trackVertexCursor += 23 * sizeof(ParticleTriangleList);
        for (i = 0; i <= normalSetNo; i++) {
            if (quadBuckets[i].count) {
                triangleLists[i].count = quadBuckets[i].count;
                triangleLists[i].tris = (D3DTRIANGLE *)trackVertexCursor;
                trackVertexCursor += quadBuckets[i].count * sizeof(D3DTRIANGLE);
                memcpy(triangleLists[i].tris, quadBuckets[i].triangles, quadBuckets[i].count * sizeof(D3DTRIANGLE));
            } else {
                triangleLists[i].count = 0;
                triangleLists[i].tris = 0;
            }
        }
        AddRenderSkeleton(cameraPtr->camPos->super, vertices, 7, triangleLists, 0, 0, 0);
    }
}
#else
void AddElectricStreak(VECTOR *unused, int unusedMode)
{
    int index;
    int i;
    int count;
    VECTOR start;
    VECTOR middle;
    VECTOR end;
    ParticleTriangleList *triangleLists;
    PCVertex *vertices;
    VECTOR cameraPosition;

    index = electricSlotIndex + 1;
    electricSlotIndex = index;
    if (index >= ELECTRIC_COUNT)
        return;
    for (i = 0; i <= normalSetNo; i++)
        trailIndexLists[i].count = 0;
    electricSlotIndex = index;
    cameraPosition.vx = cameraPtr->camPos->relative.t[0];
    cameraPosition.vy = cameraPtr->camPos->relative.t[1];
    cameraPosition.vz = cameraPtr->camPos->relative.t[2];
    cameraPtr->camPos->super->update = 1;
    SetSkeletonTransform(cameraPtr->camPos->super);
    start.vx = electrics[index].start.vx + cameraPosition.vx;
    start.vy = electrics[index].start.vy + cameraPosition.vy;
    start.vz = electrics[index].start.vz + cameraPosition.vz;
    if (abs(start.vx) >= 32768 || abs(start.vz) >= 32768)
        return;
    trailStage[0].x = start.vx * electricCoordinateScale;
    trailStage[0].y = start.vy * electricCoordinateScale;
    trailStage[0].z = start.vz * electricCoordinateScale;
    trailStage[0].a = particleAlpha;
    trailStage[0].r = electrics[index].r1;
    trailStage[0].g = electrics[index].g1;
    trailStage[0].b = electrics[index].b1;
    trailStage[0].u = 0;
    trailStage[0].v = 0;
    end.vx = electrics[index].end.vx + cameraPosition.vx;
    end.vy = electrics[index].end.vy + cameraPosition.vy;
    end.vz = electrics[index].end.vz + cameraPosition.vz;
    trailStage[1].x = end.vx * electricCoordinateScale;
    trailStage[1].y = end.vy * electricCoordinateScale;
    trailStage[1].z = end.vz * electricCoordinateScale;
    electrics[index].life = -1;
    trailStage[1].u = 0;
    trailStage[1].v = 0;
    trailStage[1].a = particleAlpha;
    trailStage[1].r = electrics[index].r1;
    trailStage[1].g = electrics[index].g1;
    trailStage[1].b = electrics[index].b1;
    /* These interpolations use the midpoint values already in the vertex buffer. */
    trailStage[2].x = trailStage[1].x +
        (trailStage[3].x - trailStage[1].x) * electricWidth;
    trailStage[2].y = trailStage[1].y +
        (trailStage[3].y - trailStage[1].y) * electricWidth;
    trailStage[2].z = trailStage[1].z +
        (trailStage[3].z - trailStage[1].z) * electricWidth;
    trailStage[2].u = 0;
    trailStage[2].v = 0;
    trailStage[2].a = particleAlpha;
    trailStage[2].r = electrics[index].r1;
    trailStage[2].g = electrics[index].g1;
    trailStage[2].b = electrics[index].b1;
    middle.vx = (start.vx + end.vx) / 2;
    middle.vy = (start.vy + end.vy) / 2;
    middle.vz = (start.vz + end.vz) / 2;
    trailStage[3].x = (middle.vx + (electrics[index].jitter[0] & 127) - 63) * electricCoordinateScale;
    trailStage[3].y = (middle.vy + (electrics[index].jitter[1] & 127) - 63) * electricCoordinateScale;
    trailStage[3].z = (middle.vz + (electrics[index].jitter[2] & 127) - 63) * electricCoordinateScale;
    trailStage[3].u = 0;
    trailStage[3].v = 0;
    trailStage[3].a = particleAlpha;
    trailStage[3].r = electrics[index].r2;
    trailStage[3].g = electrics[index].g2;
    trailStage[3].b = electrics[index].b2;
    trailStage[4].x = trailStage[3].x +
        (trailStage[5].x - trailStage[3].x) * electricMiddleWidth;
    trailStage[4].y = trailStage[3].y +
        (trailStage[5].y - trailStage[3].y) * electricMiddleWidth;
    trailStage[4].z = trailStage[3].z +
        (trailStage[5].z - trailStage[3].z) * electricMiddleWidth;
    trailStage[4].u = 0;
    trailStage[4].v = 0;
    trailStage[4].a = particleAlpha;
    trailStage[4].r = (electrics[index].r1 + electrics[index].r2) / 2;
    trailStage[4].g = (electrics[index].g1 + electrics[index].g2) / 2;
    trailStage[4].b = (electrics[index].b1 + electrics[index].b2) / 2;
    trailStage[5].x = (middle.vx + (electrics[index].jitter[3] & 127) - 63) * electricCoordinateScale;
    trailStage[5].y = (middle.vy + (electrics[index].jitter[4] & 127) - 63) * electricCoordinateScale;
    trailStage[5].z = (middle.vz + (electrics[index].jitter[5] & 127) - 63) * electricCoordinateScale;
    trailStage[5].u = 0;
    trailStage[5].v = 0;
    trailStage[5].a = particleAlpha;
    trailStage[5].r = electrics[index].r2;
    trailStage[5].g = electrics[index].g2;
    trailStage[5].b = electrics[index].b2;
    trailStage[6].x = trailStage[3].x +
        (trailStage[1].x - trailStage[3].x) * electricWidth;
    trailStage[6].y = trailStage[3].y +
        (trailStage[1].y - trailStage[3].y) * electricWidth;
    trailStage[6].z = trailStage[3].z +
        (trailStage[1].z - trailStage[3].z) * electricWidth;
    trailStage[6].a = particleAlpha;
    trailStage[6].r = electrics[index].r2;
    trailStage[6].g = electrics[index].g2;
    trailStage[6].b = electrics[index].b2;
    trailStage[6].u = 0;
    trailStage[6].v = 0;
    trailIndexLists[normalSetNo].triangles[0].v1 = 0;
    trailIndexLists[normalSetNo].triangles[0].v2 = 1;
    trailIndexLists[normalSetNo].triangles[0].v3 = 2;
    trailIndexLists[normalSetNo].triangles[1].v1 = 1;
    trailIndexLists[normalSetNo].triangles[1].v2 = 3;
    trailIndexLists[normalSetNo].triangles[1].v3 = 4;
    trailIndexLists[normalSetNo].triangles[2].v1 = 3;
    trailIndexLists[normalSetNo].triangles[2].v2 = 5;
    trailIndexLists[normalSetNo].triangles[2].v3 = 6;
    trailIndexLists[normalSetNo].count = 3;
    vertices = (PCVertex *)trackVertexCursor;
    memcpy(vertices, trailStage, 7 * sizeof(PCVertex));
    trackVertexCursor += 7 * sizeof(PCVertex);
    triangleLists = (ParticleTriangleList *)trackVertexCursor;
    trackVertexCursor += 23 * sizeof(ParticleTriangleList);
    for (i = 0; i <= normalSetNo; i++) {
        count = trailIndexLists[i].count;
        if (count) {
            triangleLists[i].tris = (D3DTRIANGLE *)trackVertexCursor;
            triangleLists[i].count = count;
            /* The original reads the count again after the header stores and
               uses that copy for both the copy length and the cursor advance. */
            count = trailIndexLists[i].count;
            memcpy(trackVertexCursor, trailIndexLists[i].triangles,
                   count * sizeof(D3DTRIANGLE));
            trackVertexCursor += count * sizeof(D3DTRIANGLE);
        } else {
            triangleLists[i].count = 0;
            triangleLists[i].tris = 0;
        }
    }
    AddRenderSkeleton(cameraPtr->camPos->super, vertices, 7, triangleLists, 0, 0, 0);
}
#endif
#endif

void resetMiscFx(void)
{
    electricSlotIndex = 0;
    rocketFlareCount = 0;
    missileFlareCount = 0;
}

void UnsetParticles(SParticle *particles)
{
    int i;

    for (i = 0; i < PARTICLE_COUNT; i++)
        particles[i].flags = 0;
}
