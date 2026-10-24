#include "functions.h"
#include "weapon_state.h"
#include "cheats.h"
#include "weapon.h"
#include "sparks.h"
#include "sound_samples.h"
#include "mathinline.h"
#include <string.h>
#ifdef NETPLAY
#include <stdio.h>
#endif

extern short ownShip;
extern int proximity;
extern TrackData *track;
extern ConfigData *gameData;
extern RaceLine raceLines[];
extern short numTrackSections;
extern short missileLockTimer;
extern signed char rearLockCycle;
void AddRenderObject(Object *object, int mode, u_char *rgb);
extern SParticle *particleSlots;
extern u_char minigunFrame;
extern short minigunAmmo;
extern int flashNozzleShip;
extern int nextParticleSlot;
extern short playerPosition;
extern ShipData *shipIndex;
extern ConfigData *tempGameData;
extern int gameType;
extern int shipDamage[];
extern int seedWep;
extern int linkSendFlag;
extern int attractDelayFrames;
extern int fracdamage[];
extern int shipDeathCount[];
extern int raceContext;
extern void (*CameraUpdate)();
extern int shieldGreen[];
extern int shieldRed[];
extern int previousPlayerDamage;
extern short TextureTableCount;
long FileLength(char *name);
void FreePrm(Object *object);
Object *LoadPrm(char *name, short textureStart);
void LoadCmpFiles(TIMlist *seq);
TIMlist *LoadCompressedTextureSequence(char *name);
extern int fadeAmount;
extern short fadeRed;
extern short fadeGreen;
extern short fadeBlue;
extern int cheatfade;
#ifdef NETPLAY
extern signed char fadeActive;
#else
extern u_char fadeActive;
#endif
extern TILE fadeTile;
extern DR_MODE fadedrm;
extern short pitAmbientVoice;
void SetPitAmbient(short on);
void SetNoteVolume(short voice, unsigned char volume);
void AddPitParticles(SParticle *particles, int count, VECTOR *pos, int fallSpeed);
void AddElectric(VECTOR *position, int mode);
void AddElectricStreak(VECTOR *position, int mode);
void UpdateCameraAttractDestroyed();
extern signed char weaponDrone[];
extern signed char weaponChance[55];
void InitPlasmaShot(void);
extern int pauseRequested;
extern POLY_G4 plasmaCharge;
void DrawPolyG4(float z, POLY_G4 *poly, int texturePage);

/* Runs every live weapon's update routine, then the fade overlay. */
void UpdateWeapons(ShipData *ships, WeaponShape *shapes, WeaponData *weapons)
{
    int i;

    fadeActive = 0;
    if (cheatfade > 0) {
        fadeActive = 1;
    }
    for (i = 0; i < 20; i++) {
        if (weapons[i].count > 0) {
            weapons[i].update(ships, shapes, &weapons[i], weapons);
        }
    }
    if (fadeAmount > 0) {
        fadeActive = 1;
    }
    if (fadeActive) {
        UpdateFadeScreen();
    }
}

/* Calls the draw callback of every live weapon that has one. */
/* NET adds the fadeActive no-op call after the draw loop; the loops are spelled differently. */
#ifdef NETPLAY
void NoOp429ca0(void);

void DrawWeapons(ShipData *ships, WeaponShape *shapes, WeaponData *weapons)
{
    int i;

    for (i = 0; i < 20; i++) {
        if (weapons[i].count > 0 && weapons[i].draw) {
            weapons[i].draw(ships, shapes, &weapons[i], weapons);
        }
    }
    if (fadeActive) {
        NoOp429ca0();
    }
}
#else
void DrawWeapons(ShipData *ships, WeaponShape *shapes, WeaponData *weapons)
{
    int i;
    WeaponData *slot;

    slot = weapons;
    for (i = 0; i < 20; i++) {
        if (slot->count > 0 && slot->draw) {
            slot->draw(ships, shapes, slot, weapons);
        }
        slot++;
    }
}
#endif

/* Launches a weapon of the given type from the first free slot. Some types act on the ship directly.
   Equivalent to the executable; the ship pointer and the zero constant swap registers. */
#ifdef EQUIV
/* NET adds weapon cases 10, 11, 14 and 15 and reorders the slot logic. */
#ifdef NETPLAY
void InsertWeapon(ShipData *ship, WeaponData *weapons, int owner, char type)
{
    int i;
    int delay = 0;

    for (i = 0; i < 20; i++) {
        if (weapons[i].count == 0) {
            switch (type) {
            case 1:
                weapons[i].update = InitRocket;
                weapons[i].draw = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                if (owner == ownShip)
                    PlayNote(SAMPLE_MISSILES, 160, 15, 15, 256);
                weapons[i].type = 1;
                ship->weapon = 0;
                return;
            case 11:
                if (!plasmaShotBusy) {
                    weapons[i].update = InitPlasma;
                    weapons[i].draw = 0;
                    weapons[i].count = 40;
                    weapons[i].owner = owner;
                    if (owner == ownShip)
                        PlayNote(SAMPLE_PLASMA, 255, 15, 15, 256);
                    weapons[i].type = 11;
                }
                return;
            case 10:
                weapons[i].update = InitQuake;
                weapons[i].draw = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                if (owner == ownShip)
                    PlayNote(SAMPLE_QUAKE, 255, 15, 15, 256);
                weapons[i].type = 10;
                ship->weapon = 0;
                return;
            case 14:
                weapons[i].update = InitRocketLeft;
                weapons[i].draw = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                weapons[i].type = 1;
                ship->weapon = 0;
                return;
            case 15:
                weapons[i].update = InitRocketRight;
                weapons[i].draw = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                weapons[i].type = 1;
                ship->weapon = 0;
                return;
            case 2:
                weapons[i].update = InitMissile;
                weapons[i].draw = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                if (owner == ownShip)
                    PlayNote(SAMPLE_MISSILES, 160, 15, 15, 256);
                weapons[i].type = 2;
                ship->weapon = 0;
                return;
            case 13:
                weapons[i].update = InitMinigun;
                weapons[i].draw = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                weapons[i].type = 13;
                return;
            case 3:
                weapons[i].update = InitMine;
                weapons[i].draw = 0;
                delay += 3;
                weapons[i].owner = owner;
                if (weapons[i].owner != ownShip && !networkGame)
                    weapons[i].count = delay + 40;
                else
                    weapons[i].count = delay;
                weapons[i].type = 3;
                if (delay >= 15) {
                    ship->weapon = 0;
                    return;
                }
                break;
            case 12:
                weapons[i].update = InitSmartBomb;
                weapons[i].draw = 0;
                weapons[i].owner = owner;
                weapons[i].count = 24;
                weapons[i].type = 12;
                ship->weapon = 0;
                return;
            case 5:
                weapons[i].update = InitElecBolt;
                weapons[i].draw = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                weapons[i].type = 5;
                ship->weapon = 0;
                if (owner == ownShip) {
                    PlayNote(SAMPLE_ELECTRIC_BOLT, 255, 15, 15, 256);
                    PlayNote(SAMPLE_MISSILES, 255, 15, 15, 192);
                }
                return;
            case 7:
                ship->weapon = 0;
                ship->autopilotCount = 120;
                ship->update = UpdateDroneShip;
                humanToDroneSpeed(shipIndex, owner);
                if (owner == ownShip)
                    PlayNote(SAMPLE_AUTOPILOT, 480, 15, 15, 256);
                return;
            case 6:
                shipDamage[owner] -= 32;
                fracdamage[owner] -= 8192;
                if (shipDamage[owner] < 0)
                    shipDamage[owner] = 0;
                ship->weapon = 0;
                if (owner == ownShip) {
                    PlayNote(SAMPLE_ENERGY_PACK, 255, 30, 15, 256);
                    PlayNote(SAMPLE_ENERGY_PACK, 255, 0, 15, 256);
                }
                return;
            case 8:
                weapons[i].update = InitShield;
                weapons[i].draw = 0;
                weapons[i].count = 200;
                weapons[i].owner = owner;
                weapons[i].type = 8;
                ship->weapon = 0;
                return;
            case 9:
                ship->weapon = 0;
                ship->thrustCount = 13;
                ship->thrust = ship->maxThrust;
                if (owner == ownShip)
                    PlayNote(SAMPLE_NEW_TURBO, 200, 15, 15, 256);
                return;
            }
        }
    }
}
#else
void InsertWeapon(ShipData *ship, WeaponData *weapons, int owner, char type)
{
    int i;
    int delay = 0;

    for (i = 0; i < 20; i++) {
        if (weapons[i].count == 0) {
            switch (type) {
            case 1:
                weapons[i].update = InitRocket;
                weapons[i].draw = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                if (owner == ownShip)
                    PlayNote(SAMPLE_MISSILES, 160, 15, 15, 256);
                weapons[i].type = 1;
                ship->weapon = 0;
                return;
            case 2:
                weapons[i].update = InitMissile;
                weapons[i].draw = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                if (owner == ownShip)
                    PlayNote(SAMPLE_MISSILES, 160, 15, 15, 256);
                weapons[i].type = 2;
                ship->weapon = 0;
                return;
            case 3:
                weapons[i].update = InitMine;
                weapons[i].draw = 0;
                delay += 3;
                weapons[i].owner = owner;
                if ((signed char)owner != ownShip)
                    weapons[i].count = delay + 40;
                else
                    weapons[i].count = delay;
                weapons[i].type = 3;
                if (delay >= 15) {
                    ship->weapon = 0;
                    return;
                }
                break;
            case 5:
                weapons[i].update = InitElecBolt;
                weapons[i].draw = 0;
                ship->weapon = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                weapons[i].type = 5;
                if (owner == ownShip) {
                    PlayNote(SAMPLE_ELECTRIC_BOLT, 255, 15, 15, 256);
                    PlayNote(SAMPLE_MISSILES, 255, 15, 15, 192);
                }
                return;
            case 6:
                shipDamage[owner] -= 32;
                fracdamage[owner] -= 8192;
                if (shipDamage[owner] < 0)
                    shipDamage[owner] = 0;
                ship->weapon = 0;
                if (owner == ownShip) {
                    PlayNote(SAMPLE_ENERGY_PACK, 255, 30, 15, 256);
                    PlayNote(SAMPLE_ENERGY_PACK, 255, 0, 15, 256);
                }
                return;
            case 7:
                ship->autopilotCount = 120;
                ship->weapon = 0;
                ship->update = UpdateDroneShip;
                humanToDroneSpeed(shipIndex, owner);
                if (owner == ownShip)
                    PlayNote(SAMPLE_AUTOPILOT, 480, 15, 15, 256);
                return;
            case 8:
                weapons[i].update = InitShield;
                weapons[i].draw = 0;
                weapons[i].count = 200;
                weapons[i].owner = owner;
                ship->weapon = 0;
                weapons[i].type = 8;
                return;
            case 9:
                ship->thrustCount = 13;
                ship->weapon = 0;
                ship->thrust = ship->maxThrust;
                if (owner == ownShip)
                    PlayNote(SAMPLE_NEW_TURBO, 200, 15, 15, 256);
                return;
            case 10:
                weapons[i].update = InitQuake;
                weapons[i].draw = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                if (owner == ownShip)
                    PlayNote(SAMPLE_QUAKE, 255, 15, 15, 256);
                weapons[i].type = 10;
                ship->weapon = 0;
                return;
            case 11:
                if (plasmaShotBusy)
                    return;
                weapons[i].update = InitPlasma;
                weapons[i].draw = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                PlayNote(SAMPLE_PLASMA, 255, 15, 15, 256);
                weapons[i].type = 11;
                return;
            case 12:
                weapons[i].update = InitSmartBomb;
                weapons[i].draw = 0;
                weapons[i].owner = owner;
                weapons[i].count = 24;
                weapons[i].type = 12;
                ship->weapon = 0;
                return;
            case 13:
                weapons[i].update = InitMinigun;
                weapons[i].draw = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                weapons[i].type = 13;
                return;
            case 14:
                weapons[i].update = InitRocketLeft;
                weapons[i].draw = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                ship->weapon = 0;
                weapons[i].type = 1;
                return;
            case 15:
                weapons[i].update = InitRocketRight;
                weapons[i].draw = 0;
                ship->weapon = 0;
                weapons[i].count = 40;
                weapons[i].owner = owner;
                weapons[i].type = 1;
                return;
            }
        }
    }
}
#endif
#endif

#ifdef NETPLAY
void FireWeapon(ShipData *ship, WeaponData *weapons, int owner, signed char type)
#else
void FireWeapon(ShipData *ship, WeaponData *weapons, int owner, u_char type)
#endif
{
#ifdef NETPLAY
    if (owner == ownShip || networkGame) {
#else
    if (owner == ownShip) {
#endif
        if (type == 2 && ship->rearFire && ship->rearTarget != -1)
            ship->targetShip = ship->rearTarget;
    }
    InsertWeapon(ship, weapons, owner, type);
    if (type == 1) {
        InsertWeapon(ship, weapons, owner, 14);
        InsertWeapon(ship, weapons, owner, 15);
    }
}

void MissileLock(short shipNo, ShipData *ships)
{
    ShipData *s;
    int sec;
    int diff;
    int nearShip;

    s = &ships[shipNo];
    sec = raceLines[s->nearSecNo].distance;
    if (s->targetShip != -1) {
        diff = raceLines[ships[s->targetShip].nearSecNo].distance - sec;
        if (diff < 0)
            diff += numTrackSections;
        if (diff >= 40 || diff <= 0)
            s->targetShip = -1;
    }
    if (s->targetShip == -1) {
        nearShip = (short)getShipInfrontLAP(ships, shipNo);
        if (nearShip != -1) {
            diff = raceLines[ships[nearShip].nearSecNo].distance - sec;
            if (diff < 0)
                diff += numTrackSections;
            if (diff < 36 && diff > 4) {
                missileLockTimer = 10;
                if (shipNo == ownShip)
                    PlayNote(SAMPLE_LOCKED, 0x80, 0xF, 0xF, 0x100);
                s->targetShip = nearShip;
            }
        }
    }
}

void RearMissileLock(short shipNo, ShipData *ships)
{
#ifdef NETPLAY
    int distance;
    int difference;
    ShipData *ship;
    int nearShip;
#else
    int diff;
    int nearShip;
    ShipData *s;
#endif

    nearShip = (short)getShipBehindLAP(ships, shipNo);
#ifdef NETPLAY
    ship = &ships[shipNo];
#else
    s = &ships[shipNo];
#endif
    if (nearShip != -1) {
#ifdef NETPLAY
        distance = raceLines[ships[nearShip].nearSecNo].distance;
        difference = raceLines[ships[shipNo].nearSecNo].distance - distance;
#else
        diff = raceLines[s->nearSecNo].distance - raceLines[ships[nearShip].nearSecNo].distance;
#endif
#ifdef NETPLAY
        if (difference < 0)
            difference += numTrackSections;
        if (difference < 40 && difference > 0) {
#else
        if (diff < 0)
            diff += numTrackSections;
        if (diff < 40 && diff > 0) {
#endif
#ifdef NETPLAY
            ship->rearTarget = nearShip;
#else
            s->rearTarget = nearShip;
#endif
            if (shipNo == ownShip)
                rearLockCycle = (rearLockCycle + 1) % 20;
        } else {
#ifdef NETPLAY
            ship->rearTarget = -1;
#else
            s->rearTarget = -1;
#endif
            if (shipNo == ownShip)
                rearLockCycle = 0;
        }
    } else {
#ifdef NETPLAY
        ship->rearTarget = -1;
#else
        s->rearTarget = -1;
#endif
        if (shipNo == ownShip)
            rearLockCycle = 0;
    }
}

extern char wobbleDir2[];
extern DR_MODE shieldParams[][2];
#include "scene.h"

/* Copies the master shape into the following slots; each copy gets its own skeleton, vertices and
   primitives packed after the object header. */
#ifdef NETPLAY
#define CLONE_SHAPES(master, count)                                                    \
    FreePrm(shapes[master].shapePtr);                                                  \
    shapes[master].shapePtr = LoadPrm(name, texStart);                                 \
    for (i = master + 1; i < master + count + 1; i++) {                                \
        loadStart = DAlloc(heap, size + 32);                                           \
        loader.ptr = loadStart;                                                        \
        shapes[i].shapePtr = loader.object;                                            \
        memcpy(loadStart, shapes[master].shapePtr, size);                              \
        objectPtr = loader.object++;                                                   \
        objectPtr->skeleton = loader.skeleton++;                                       \
        objectPtr->vertices = loader.svector;                                          \
        loader.svector += objectPtr->vertexCount;                                      \
        objectPtr->primitives = loader.primitive;                                      \
    }
#else
#define CLONE_SHAPES(master, count)                                                    \
    FreePrm(shapes[master].shapePtr);                                                  \
    shapes[master].shapePtr = LoadPrm(path, textureStart);                             \
    for (i = 0; i < count; i++) {                                                      \
        start = DAlloc(heap, length + 32);                                             \
        prm.ptr = start;                                                               \
        shapes[master + 1 + i].shapePtr = prm.object;                                  \
        memcpy(start, shapes[master].shapePtr, length);                                \
        object = prm.object++;                                                         \
        object->skeleton = prm.skeleton++;                                             \
        object->vertices = prm.svector;                                                \
        prm.svector += object->vertexCount;                                            \
        object->primitives = prm.primitive;                                            \
    }
#endif

/* Loads the weapon shapes (rockets, mines, missiles, shields, bolts), clears the weapon table and
   builds the semi-transparent draw-mode packets used by the shield. */
#ifdef EQUIV
#ifdef NETPLAY
void initWeapons(WeaponShape *shapes, Skeleton *camPos, WeaponData *weapons)
{
    char *loadStart;
    long size;
    Object *objectPtr;
    Object *walker;
    short primitiveCount;
    FloatVertex *baseVerts;
    int i;
    Prm loader;
    int kk;
    short texStart;
    int phase;
    char name[256];
    Poly polyCur;
    TIMlist *textureList;

    texStart = TextureTableCount;
    strcat(strcpy(name, wobbleDir2), "mine.cmp");
    textureList = LoadCompressedTextureSequence(name);
    LoadCmpFiles(textureList);
    strcat(strcpy(name, wobbleDir2), "rock.prm");
    size = FileLength(name);
    CLONE_SHAPES(0, 3)
    strcat(strcpy(name, wobbleDir2), "mine.prm");
    size = FileLength(name);
    CLONE_SHAPES(4, 14)
    strcat(strcpy(name, wobbleDir2), "miss.prm");
    size = FileLength(name);
    CLONE_SHAPES(19, 3)
    strcat(strcpy(name, wobbleDir2), "shld.prm");
    size = FileLength(name);
    FreePrm(shapes[23].shapePtr);
    shapes[23].shapePtr = LoadPrm(name, texStart);
    baseVerts = shapes[23].shapePtr->floatVerts;
    for (i = 24; i < 26; i++) {
        shapes[i].shapePtr = NewInstance(shapes[23].shapePtr);
        shapes[i].shapePtr->floatVerts = DAlloc(heap, shapes[23].shapePtr->vertexCount * sizeof(FloatVertex));
        memcpy(shapes[i].shapePtr->floatVerts, baseVerts, shapes[23].shapePtr->vertexCount * sizeof(FloatVertex));
    }
    strcat(strcpy(name, wobbleDir2), "ebolt.prm");
    size = FileLength(name);
    CLONE_SHAPES(26, 2)
    for (i = 0; i < 29; i++) {
        if (i >= 23 && i < 26) {
            polyCur.prim = shapes[i].shapePtr->primitives;
            primitiveCount = shapes[i].shapePtr->primitiveCount;
            for (kk = 0; kk < primitiveCount; kk++) {
                switch (polyCur.prim->type) {
                case TypeG3:
                    for (phase = 0; phase < 2; phase++) {
                        polyCur.g3->colour[0].b = 255;
                        polyCur.g3->colour[1].b = 255;
                        polyCur.g3->colour[2].b = 255;
                    }
                    polyCur.g3 += 1;
                    break;
                case TypeG4:
                    for (phase = 0; phase < 2; phase++) {
                        polyCur.g4->colour[0].b = 255;
                        polyCur.g4->colour[1].b = 255;
                        polyCur.g4->colour[2].b = 255;
                        polyCur.g4->colour[3].b = 255;
                    }
                    polyCur.g4 += 1;
                    break;
                }
            }
        }
        shapes[i].used = 0;
        walker = shapes[i].shapePtr;
        while (walker) {
            walker->skeleton->super = camPos;
            walker = walker->next;
        }
    }
    for (i = 0; i < 20; i++) {
        weapons[i].count = 0;
        weapons[i].type = 0;
    }
    for (i = 0; i < 192; i++) {
        for (phase = 0; phase < 2; phase++)
            SetDrawMode(&shieldParams[i][phase], 1, 1, GetTPage(0, 1, 0, 0) & 0xFFFF, 0);
    }
}
#else
/* The draw-mode loop differs only in counter allocation. */
void initWeapons(WeaponShape *shapes, Skeleton *camPos, WeaponData *weapons)
{
    short textureStart;
    char path[256];
    char *start;
    Object *object;
    Object *obj;
    FloatVertex *masterVerts;
    long length;
    Prm prm;
    Poly poly;
    int j;
    int k;
    int i;
    short primitiveCount;

    textureStart = TextureTableCount;
    strcat(strcpy(path, wobbleDir2), "mine.cmp");
    LoadCmpFiles(LoadCompressedTextureSequence(path));
    strcat(strcpy(path, wobbleDir2), "rock.prm");
    length = FileLength(path);
    CLONE_SHAPES(0, 3)
    strcat(strcpy(path, wobbleDir2), "mine.prm");
    length = FileLength(path);
    CLONE_SHAPES(4, 14)
    strcat(strcpy(path, wobbleDir2), "miss.prm");
    length = FileLength(path);
    CLONE_SHAPES(19, 3)
    strcat(strcpy(path, wobbleDir2), "shld.prm");
    length = FileLength(path);
    FreePrm(shapes[23].shapePtr);
    shapes[23].shapePtr = LoadPrm(path, textureStart);
    masterVerts = shapes[23].shapePtr->floatVerts;
    for (i = 0; i < 2; i++) {
        shapes[24 + i].shapePtr = NewInstance(shapes[23].shapePtr);
        shapes[24 + i].shapePtr->floatVerts = DAlloc(heap, shapes[23].shapePtr->vertexCount * sizeof(FloatVertex));
        memcpy(shapes[24 + i].shapePtr->floatVerts, masterVerts, shapes[23].shapePtr->vertexCount * sizeof(FloatVertex));
    }
    strcat(strcpy(path, wobbleDir2), "ebolt.prm");
    length = FileLength(path);
    CLONE_SHAPES(26, 2)
    for (j = 0; j < 29; j++) {
        if (j >= 23 && j < 26) {
            poly.prim = shapes[j].shapePtr->primitives;
            primitiveCount = shapes[j].shapePtr->primitiveCount;
            for (k = 0; k < primitiveCount; k++) {
                switch (poly.prim->type) {
                case TypeG3:
                    poly.g3->colour[0].b = 255;
                    poly.g3->colour[1].b = 255;
                    poly.g3->colour[2].b = 255;
                    poly.g3 += 1;
                    break;
                case TypeG4:
                    poly.g4->colour[0].b = 255;
                    poly.g4->colour[1].b = 255;
                    poly.g4->colour[2].b = 255;
                    poly.g4->colour[3].b = 255;
                    poly.g4 += 1;
                    break;
                }
            }
        }
        shapes[j].used = 0;
        obj = shapes[j].shapePtr;
        while (obj) {
            obj->skeleton->super = camPos;
            obj = obj->next;
        }
    }
    for (i = 0; i < 20; i++) {
        weapons[i].count = 0;
        weapons[i].type = 0;
    }
    for (i = 0; i < 192; i++) {
        for (j = 0; j < 2; j++)
            SetDrawMode(&shieldParams[i][j], 1, 1, GetTPage(0, 1, 0, 0) & 0xFFFF, 0);
    }
}
#endif
#endif

void BumpUp(TrackSection *bump)
{
    Face *face;
    u_char flag;
    if (bump && bump->bump < 8) {
        face = &track->faces[bump->faceStart];
        while (!((flag = face->flags) & TRACK_BASE)) face++;
        track->vertices[face->vertex[0]].vy -= 200;
        track->vertices[face->vertex[1]].vy -= 200;
        track->vertices[(face + 1)->vertex[0]].vy -= 200;
        bump->bump++;
    }
}

void BumpDown(TrackSection *bump)
{
    Face *face;
    u_char flag;
    if (bump && bump->bump > 0) {
        face = &track->faces[bump->faceStart];
        while (!((flag = face->flags) & TRACK_BASE)) face++;
        track->vertices[face->vertex[0]].vy += 200;
        track->vertices[face->vertex[1]].vy += 200;
        track->vertices[(face + 1)->vertex[0]].vy += 200;
        bump->bump--;
    }
}

void BumpFlatten(TrackSection *bump)
{
    Face *face;
    u_char flag;
    if (bump && bump->bump > 0) {
        face = &track->faces[bump->faceStart];
        while (!((flag = face->flags) & TRACK_BASE)) face++;
        while (bump->bump > 0) {
            track->vertices[face->vertex[0]].vy += 200;
            track->vertices[face->vertex[1]].vy += 200;
            track->vertices[(face + 1)->vertex[0]].vy += 200;
            bump->bump--;
        }
    }
}

void FlattenTrack(void)
{
    int i;
    TrackSection *section = track->sections;
    for (i = 0; i < track->sectionCount; i++, section++) {
        BumpFlatten(section);
    }
}

void InitialiseBumps(void)
{
    TrackSection *section = track->sections;
    int i;
#ifndef NETPLAY
    int count = track->sectionCount;
#endif
    int sectionNo;
#ifdef NETPLAY
    for (i = 0; i < track->sectionCount; i++) {
#else
    for (i = 0; i < count; i++) {
#endif
        section->bump = 0;
        sectionNo = section->secNo;
        if ((gameData->actTrackNum == 2 &&
             ((sectionNo > 102 && sectionNo < 158) ||
              (sectionNo > 190 && sectionNo < 221) ||
              (sectionNo > 269 && sectionNo < 386))) ||
            (gameData->actTrackNum == 1 &&
             ((sectionNo >= 0 && sectionNo < 73) ||
              (sectionNo > 86 && sectionNo < 104) ||
              (sectionNo > 184 && sectionNo < 226) ||
              (sectionNo > 133 && sectionNo < 173))) ||
            (gameData->actTrackNum == 8 &&
             ((sectionNo > 34 && sectionNo < 65) ||
              (sectionNo > 152 && sectionNo < 171) ||
              (sectionNo > 250 && sectionNo < 267) ||
              (sectionNo > 282 && sectionNo < 295))) ||
            (gameData->actTrackNum == 13 &&
             ((sectionNo > 124 && sectionNo < 216) ||
              (sectionNo > 260 && sectionNo < 281) ||
              (sectionNo > 298 && sectionNo < 343) ||
              (sectionNo > 412 && sectionNo < 432))) ||
            (gameData->actTrackNum == 17 &&
             ((sectionNo > 14 && sectionNo < 32) ||
              (sectionNo > 73 && sectionNo < 106) ||
              (sectionNo > 129 && sectionNo < 168) ||
              (sectionNo > 229 && sectionNo < 274))) ||
            (gameData->actTrackNum == 6 &&
             ((sectionNo > 18 && sectionNo < 25) ||
              (sectionNo > 194 && sectionNo < 268) ||
              (sectionNo > 357 && sectionNo < 428))) ||
            (gameData->actTrackNum == 20 &&
             ((sectionNo > 212 && sectionNo < 223) ||
              (sectionNo > 230 && sectionNo < 257) ||
              (sectionNo > 303 && sectionNo < 334))) ||
            (gameData->actTrackNum == 7 &&
             ((sectionNo > 26 && sectionNo < 38) ||
              (sectionNo > 94 && sectionNo < 103) ||
              (sectionNo > 108 && sectionNo < 143) ||
              (sectionNo > 170 && sectionNo < 231) ||
              (sectionNo > 323 && sectionNo < 341) ||
              (sectionNo > 73 && sectionNo < 84))))
            section->snow = 1;
        else
            section->snow = 0;
        section++;
    }
}

#ifdef EQUIV
#ifdef NETPLAY
void UpdateQuake(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int sectionNo;
    signed char ownerIndex;
    Face *startFace;
    TrackSection *earlierSection;
    TrackSection *junctionPtr;
    Vector centreVec;
    int reserved;
    Face *oldFaceP;
    TrackSection *curSec;
    int bumpNo;
    int shipIndex;
    Vector pos;
    int unusedIdx2;
    TrackSection *nextSect;
    Face *aheadFace;

    ownerIndex = weapon->owner;
    weapon->section = FindNearestSection(weapon->section, weapon->pos);
    nextSect = weapon->section->nextSection.ptr;
    earlierSection = weapon->section->prevSection.ptr;
    startFace = track->faces + weapon->section->faceStart;
    aheadFace = track->faces + nextSect->faceStart;
    oldFaceP = track->faces + earlierSection->faceStart;
    while (!(startFace->flags & TRACK_BASE)) startFace++;
    while (!(aheadFace->flags & TRACK_BASE)) aheadFace++;
    aheadFace++;
    AddRisingParticles(particleSlots, 10, track->vertices + startFace->vertex[0], 8);
    AddRisingParticles(particleSlots, 10, track->vertices + startFace->vertex[1], 8);
    AddRisingParticles(particleSlots, 10, track->vertices + (startFace + 1)->vertex[0], 8);
    pos.vx = track->vertices[startFace->vertex[0]].vx;
    pos.vy = track->vertices[startFace->vertex[0]].vy;
    pos.vz = track->vertices[startFace->vertex[0]].vz;
    curSec = weapon->section;
    for (shipIndex = 0; shipIndex < 15; shipIndex++) {
        if (curSec == ships[shipIndex].nearTrkSect && shipIndex != ownerIndex && !(ships[shipIndex].combatAttr & DESTROYED)) {
            ships[shipIndex].speed >>= 2;
            ships[shipIndex].vpivot.vx >>= 2;
            ships[shipIndex].vpivot.vy >>= 2;
            ships[shipIndex].vpivot.vz >>= 2;
            if (shipIndex == ownShip)
                SetShake(8);
            PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, ships[shipIndex].ppivot);
            AddExplode(ships[shipIndex].ppivot.vx + (newRand() & 1023) - 512,
                       ships[shipIndex].ppivot.vy - (newRand() & 511),
                       ships[shipIndex].ppivot.vz + (newRand() & 1023) - 512, -1);
            DamageCollide(10, shipIndex, ownerIndex);
        }
    }
    for (bumpNo = 0; bumpNo < weapon->bumpStep; bumpNo++)
        BumpUp(curSec);
    curSec = weapon->section;
    for (sectionNo = 0; sectionNo < 20; sectionNo++) {
        if (curSec->junction.ptr != (TrackSection *)-1) {
            junctionPtr = curSec->junction.ptr;
            if (junctionPtr->flags & 8) {
                for (bumpNo = 0; bumpNo < 10; bumpNo++) {
                    BumpDown(junctionPtr);
                    junctionPtr = junctionPtr->prevSection.ptr;
                }
            }
        }
        curSec = curSec->prevSection.ptr;
        BumpDown(curSec);
    }
    aheadFace++;
    centreVec.vx = nextSect->centre.vx;
    centreVec.vy = nextSect->centre.vy;
    centreVec.vz = nextSect->centre.vz;
    weapon->count--;
    if (ships[ownShip].nearTrkSect->bump > 0 && !networkGame) {
        BumpFlatten(ships[ownShip].nearTrkSect);
        BumpFlatten(ships[ownShip].nearTrkSect->nextSection.ptr);
        BumpFlatten(ships[ownShip].nearTrkSect->prevSection.ptr);
    }
    if (weapon->count & 1) {
        weapon->pos.vx = nextSect->centre.vx;
        weapon->pos.vy = nextSect->centre.vy;
        weapon->pos.vz = nextSect->centre.vz;
        if (ships[ownerIndex].nearTrkSect == nextSect || ships[ownerIndex].nearTrkSect == nextSect->prevSection.ptr ||
            ships[ownerIndex].nearTrkSect == nextSect->prevSection.ptr->prevSection.ptr) {
            weapon->pos.vx = nextSect->nextSection.ptr->centre.vx;
            weapon->pos.vy = nextSect->nextSection.ptr->centre.vy;
            weapon->pos.vz = nextSect->nextSection.ptr->centre.vz;
        }
        if (weapon->bumpStep < 8 && weapon->count > 8)
            weapon->bumpStep += 2;
        if (weapon->bumpStep > 0 && weapon->count <= 8)
            weapon->bumpStep--;
    }
    if (weapon->count == 0) {
        curSec = weapon->section;
        for (sectionNo = 0; sectionNo < 20; sectionNo++) {
            if (curSec->junction.ptr != (TrackSection *)-1) {
                junctionPtr = curSec->junction.ptr;
                if (junctionPtr->flags & 8) {
                    for (bumpNo = 0; bumpNo < 20; bumpNo++) {
                        BumpFlatten(junctionPtr);
                        junctionPtr = junctionPtr->prevSection.ptr;
                    }
                }
            }
            BumpFlatten(curSec);
            curSec = curSec->prevSection.ptr;
        }
    }
}
#else
void UpdateQuake(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    /* Keep this order for MSVC 4.20's loop spill allocation. */
    TrackSection *nearSection;
    TrackSection *bumpSection;
    int i;
    short stepNow;
    short step;
    int sectionCount2;
    int faceNo;
    TrackSection *ownerSection;
    TrackSection *junction;
    int bumpCount;
    int shipNo;
    u_char flag;
    int sectionCount;
    Face *facePtr;
    TrackSection *prevOfNext;
    int j;
    signed char owner;
    Face *nextFace;
    TrackSection *section;
    TrackSection *nextSection;

    owner = weapon->owner;
    weapon->section = FindNearestSection(weapon->section, weapon->pos);
    nextSection = weapon->section->nextSection.ptr;
    faceNo = weapon->section->faceStart;
    facePtr = track->faces + faceNo;
    nextFace = track->faces + nextSection->faceStart;
    while (!((flag = facePtr->flags) & TRACK_BASE)) facePtr++;
    while (!((flag = nextFace->flags) & TRACK_BASE)) nextFace++;
    AddRisingParticles(particleSlots, 10, track->vertices + facePtr->vertex[0], 8);
    AddRisingParticles(particleSlots, 10, track->vertices + facePtr->vertex[1], 8);
    AddRisingParticles(particleSlots, 10, track->vertices + (facePtr + 1)->vertex[0], 8);
    section = weapon->section;
    shipNo = 0;
    do {
        if (section == ships[shipNo].nearTrkSect && shipNo != owner && !(ships[shipNo].combatAttr & DESTROYED)) {
            ships[shipNo].speed >>= 2;
            ships[shipNo].vpivot.vx >>= 2;
            ships[shipNo].vpivot.vy >>= 2;
            ships[shipNo].vpivot.vz >>= 2;
            if (shipNo == ownShip)
                SetShake(8);
            PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, ships[shipNo].ppivot);
            AddExplode(ships[shipNo].ppivot.vx + (newRand() & 1023) - 512,
                       ships[shipNo].ppivot.vy - (newRand() & 511),
                       ships[shipNo].ppivot.vz + (newRand() & 1023) - 512, -1);
#ifdef NETPLAY
            DamageCollide(10, shipNo, owner);
#else
            DamageCollide(10, shipNo);
#endif
        }
        shipNo++;
    } while (shipNo < 15);
    step = weapon->bumpStep;
    for (bumpCount = 0; bumpCount < step; bumpCount++)
        BumpUp(section);
    section = weapon->section;
    for (sectionCount = 0; sectionCount < 20; sectionCount++) {
        junction = section->junction.ptr;
        if (junction != (TrackSection *)-1) {
            bumpSection = junction;
            if (junction->flags & 8) {
                for (i = 0; i < 10; i++) {
                    BumpDown(bumpSection);
                    bumpSection = bumpSection->prevSection.ptr;
                }
            }
        }
        section = section->prevSection.ptr;
        BumpDown(section);
    }
    weapon->count--;
    nearSection = ships[ownShip].nearTrkSect;
    if (nearSection->bump > 0) {
        BumpFlatten(nearSection);
        BumpFlatten(ships[ownShip].nearTrkSect->nextSection.ptr);
        BumpFlatten(ships[ownShip].nearTrkSect->prevSection.ptr);
    }
    if (weapon->count & 1) {
        weapon->pos.vx = nextSection->centre.vx;
        weapon->pos.vy = nextSection->centre.vy;
        weapon->pos.vz = nextSection->centre.vz;
        ownerSection = ships[owner].nearTrkSect;
        prevOfNext = nextSection->prevSection.ptr;
        if (ownerSection == nextSection || ownerSection == prevOfNext ||
            ownerSection == prevOfNext->prevSection.ptr) {
            weapon->pos.vx = nextSection->nextSection.ptr->centre.vx;
            weapon->pos.vy = nextSection->nextSection.ptr->centre.vy;
            weapon->pos.vz = nextSection->nextSection.ptr->centre.vz;
        }
        if (step < 8 && weapon->count > 8)
            weapon->bumpStep = step + 2;
        stepNow = weapon->bumpStep;
        if (stepNow > 0 && weapon->count <= 8)
            weapon->bumpStep = stepNow - 1;
    }
    if (weapon->count == 0) {
        section = weapon->section;
        for (sectionCount2 = 0; sectionCount2 < 20; sectionCount2++) {
            junction = section->junction.ptr;
            if (junction != (TrackSection *)-1) {
                bumpSection = junction;
                if (junction->flags & 8) {
                    for (j = 0; j < 20; j++) {
                        BumpFlatten(bumpSection);
                        bumpSection = bumpSection->prevSection.ptr;
                    }
                }
            }
            BumpFlatten(section);
            section = section->prevSection.ptr;
        }
    }
}
#endif
#endif

#ifdef EQUIV
#ifdef NETPLAY
/* Network build: these steps sit behind a switch that is off; the empty if/else leaves the jumps the network build has there. */
#define NET_MISSILE_TRIG_STEP 1
void UpdateMissile(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    VECTOR toPos;
    VECTOR nextCorner;
    Face *face;
    long rise;
    long mag;
    long t;
    int horizLen;
    signed char farOwner = weapon->owner;
    float xSep, ySep;
    float zAmt;
    int dHdg;
    VECTOR targetVector;
    int along2;
    int k;
    VECTOR dir;
    int pitchErr;
    VECTOR facePt;
    TrackSection *nextSect;
    Face *aheadFace;
    int clearance;

    if (weapon->target != -1) {
        targetVector.vx = (ships[weapon->target].ppivot.vx - weapon->pos.vx) >> 3;
        targetVector.vy = (ships[weapon->target].ppivot.vy - weapon->pos.vy) >> 3;
        targetVector.vz = (ships[weapon->target].ppivot.vz - weapon->pos.vz) >> 3;
        horizLen = sqrt(targetVector.vz * targetVector.vz + targetVector.vx * targetVector.vx);
        dHdg = -ratan2(targetVector.vx, targetVector.vz) - weapon->hdg;
        pitchErr = -ratan2(targetVector.vy, horizLen) - weapon->pitch;
    } else {
        UpdateRocket(ships, shapes, weapon, weapons);
        return;
    }
    weapon->section = FindNearestSection(weapon->section, weapon->pos);
    if (dHdg > 2048)
        dHdg -= 4096;
    else if (dHdg < -2048)
        dHdg += 4096;
    if (pitchErr > 2048)
        pitchErr -= 4096;
    else if (pitchErr < -2048)
        pitchErr += 4096;
    weapon->hdg += dHdg >> 1;
    weapon->pitch += pitchErr >> 1;
    weapon->hdg = ang(weapon->hdg);
    weapon->pitch = ang(weapon->pitch);
    if (NET_MISSILE_TRIG_STEP) { } else { }
    if (NET_MISSILE_TRIG_STEP) { } else { }
    weapon->accel.vx = -(rsin(weapon->hdg) * rcos(weapon->pitch)) >> 16;
    if (NET_MISSILE_TRIG_STEP) { } else { }
    weapon->accel.vy = -rsin(weapon->pitch) >> 4;
    if (NET_MISSILE_TRIG_STEP) { } else { }
    if (NET_MISSILE_TRIG_STEP) { } else { }
    weapon->accel.vz = (rcos(weapon->hdg) * rcos(weapon->pitch)) >> 16;
    weapon->velocity.vx += weapon->accel.vx;
    weapon->velocity.vy += weapon->accel.vy;
    weapon->velocity.vz += weapon->accel.vz;
    weapon->velocity.vx -= weapon->velocity.vx >> 2;
    weapon->velocity.vy -= weapon->velocity.vy >> 2;
    weapon->velocity.vz -= weapon->velocity.vz >> 2;
    weapon->pos.vx += weapon->velocity.vx;
    weapon->pos.vy += weapon->velocity.vy;
    weapon->pos.vz += weapon->velocity.vz;
    face = track->faces + weapon->section->faceStart;
    while (!(face->flags & TRACK_BASE))
        face++;
    nextSect = weapon->section->nextSection.ptr;
    aheadFace = track->faces + nextSect->faceStart;
    while (!(aheadFace->flags & TRACK_BASE))
        aheadFace++;
    facePt.vx = track->vertices[face->vertex[0]].vx;
    facePt.vy = track->vertices[face->vertex[0]].vy;
    facePt.vz = track->vertices[face->vertex[0]].vz;
    nextCorner.vx = track->vertices[aheadFace->vertex[0]].vx;
    nextCorner.vy = track->vertices[aheadFace->vertex[0]].vy;
    nextCorner.vz = track->vertices[aheadFace->vertex[0]].vz;
    dir.vx = nextCorner.vx - facePt.vx;
    dir.vy = nextCorner.vy - facePt.vy;
    dir.vz = nextCorner.vz - facePt.vz;
    toPos.vx = weapon->pos.vx - facePt.vx;
    toPos.vz = weapon->pos.vz - facePt.vz;
    mag = sqrt(dir.vz * dir.vz + dir.vx * dir.vx);
    along2 = toPos.vz * dir.vz + toPos.vx * dir.vx;
    rise = along2 / mag;
    t = (rise << 16) / mag;
    rise = nextCorner.vy - facePt.vy;
    rise = (t * rise) >> 16;
    if (facePt.vy + rise - 300 > weapon->pos.vy)
        weapon->pos.vy = facePt.vy + rise - 300;
    weapon->count--;
    if (weapon->count == 0)
        shapes[weapon->shapeNo].used = 0;
    SetSkeletonPosition(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->pos.vx, weapon->pos.vy, weapon->pos.vz);
    SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->hdg, weapon->pitch, 0);
    for (k = 0; k < 15; k++) {
        if (!(ships[k].combatAttr & DESTROYED) && farOwner != k) {
            xSep = (ships[k].ppivot.vx - weapon->pos.vx) >> 3;
            ySep = (ships[k].ppivot.vy - weapon->pos.vy) >> 3;
            zAmt = (ships[k].ppivot.vz - weapon->pos.vz) >> 3;
            clearance = GetMagnitude(xSep, ySep, zAmt);
            if (clearance < 100) {
                if (!(ships[k].attr & SHIELDED)) {
                    if (k != ownShip && !networkGame) {
                        FlipShip(&ships[k]);
                        ships[k].speed = ships[k].speed >> 5;
                        ships[k].vroll += 5000;
                        ships[k].ahdg = (newRand() - 16384) >> 1;
                    } else {
                        ships[k].vpivot.vx -= (ships[k].vpivot.vx * 3) >> 2;
                        ships[k].vpivot.vy -= (ships[k].vpivot.vy * 3) >> 2;
                        ships[k].vpivot.vz -= (ships[k].vpivot.vz * 3) >> 2;
                        ships[k].vroll += newRand() % 250;
                        ships[k].ahdg = (newRand() - 16384) >> 1;
                        if (k == ownShip)
                            SetShake(20);
                    }
                }
                DamageCollide(2, k, farOwner);
                weapon->count = -1;
                weapon->target = k;
                shapes[weapon->shapeNo].used = 0;
                PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, weapon->pos);
                return;
            }
        }
    }
    face = track->faces + weapon->section->faceStart;
    for (k = 0; k < weapon->section->faceCount; k++) {
        facePt.vx = track->vertices[face->vertex[0]].vx;
        facePt.vy = track->vertices[face->vertex[0]].vy;
        facePt.vz = track->vertices[face->vertex[0]].vz;
        clearance = PlaneLineCollision(facePt, weapon->pos, face->normal);
        if (clearance <= 0 && !(face->flags & TRACK_BASE)) {
            weapon->count = -2;
            shapes[weapon->shapeNo].used = 0;
            PlayDirectionalNote(SAMPLE_WEAPON_WALL, 0, 40, weapon->pos);
            return;
        }
        face++;
    }
    Weapon2Mine(weapon, weapons, shapes);
}
#else
/* Equivalent to the executable: the executable spills its cached track register to the track global at 0x4255e3 inside the face loop; the computation otherwise matches. */
void UpdateMissile(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    VECTOR facePoint;
    VECTOR nextPoint;
    VECTOR path;
    VECTOR offset;
    VECTOR targetVector;
    Face *facePtr;
    Face *nextFace;
    ShipData *ship;
    int i;
    int mag;
    int horizDist;
    int vhdg;
    int vpitch;
    int alpha;
    int height;
    long pitchCosine;
    long headingSine;
    long pitchSine;
    long headingCosine;
    float dx, dy, dz;
    u_char flag;
    signed char owner = weapon->owner;

    if (weapon->target != -1) {
        ship = &ships[weapon->target];
        targetVector.vx = (ship->ppivot.vx - weapon->pos.vx) >> 3;
        targetVector.vy = (ship->ppivot.vy - weapon->pos.vy) >> 3;
        targetVector.vz = (ship->ppivot.vz - weapon->pos.vz) >> 3;
        horizDist = SquareRoot0(targetVector.vx * targetVector.vx + targetVector.vz * targetVector.vz);
        vhdg = -ratan2(targetVector.vx, targetVector.vz) - weapon->hdg;
        vpitch = -ratan2(targetVector.vy, horizDist) - weapon->pitch;
        weapon->section = FindNearestSection(weapon->section, weapon->pos);
        if (vhdg > 2048)
            vhdg -= 4096;
        else if (vhdg < -2048)
            vhdg += 4096;
        if (vpitch > 2048)
            vpitch -= 4096;
        else if (vpitch < -2048)
            vpitch += 4096;
        weapon->hdg += vhdg >> 1;
        weapon->pitch += vpitch >> 1;
        weapon->hdg = ang(weapon->hdg);
        weapon->pitch = ang(weapon->pitch);
        pitchCosine = rcos(weapon->pitch);
        headingSine = rsin(weapon->hdg);
        weapon->accel.vx = (-headingSine * pitchCosine) >> 16;
        pitchSine = rsin(weapon->pitch);
        weapon->accel.vy = -pitchSine >> 4;
        headingCosine = rcos(weapon->hdg);
        weapon->accel.vz = (headingCosine * pitchCosine) >> 16;
        weapon->velocity.vx += weapon->accel.vx;
        weapon->velocity.vy += weapon->accel.vy;
        weapon->velocity.vz += weapon->accel.vz;
        weapon->velocity.vx -= weapon->velocity.vx >> 2;
        weapon->velocity.vy -= weapon->velocity.vy >> 2;
        weapon->velocity.vz -= weapon->velocity.vz >> 2;
        weapon->pos.vx += weapon->velocity.vx;
        weapon->pos.vy += weapon->velocity.vy;
        weapon->pos.vz += weapon->velocity.vz;
        facePtr = track->faces + weapon->section->faceStart;
        if (!((flag = facePtr->flags) & TRACK_BASE)) {
            do {
                facePtr++;
            } while (!((flag = facePtr->flags) & TRACK_BASE));
        }
        nextFace = track->faces + weapon->section->nextSection.ptr->faceStart;
        if (!((flag = nextFace->flags) & TRACK_BASE)) {
            do {
                nextFace++;
            } while (!((flag = nextFace->flags) & TRACK_BASE));
        }
        facePoint.vx = track->vertices[facePtr->vertex[0]].vx;
        facePoint.vy = track->vertices[facePtr->vertex[0]].vy;
        facePoint.vz = track->vertices[facePtr->vertex[0]].vz;
        nextPoint.vx = track->vertices[nextFace->vertex[0]].vx;
        nextPoint.vy = track->vertices[nextFace->vertex[0]].vy;
        nextPoint.vz = track->vertices[nextFace->vertex[0]].vz;
        path.vx = nextPoint.vx - facePoint.vx;
        path.vy = nextPoint.vy - facePoint.vy;
        path.vz = nextPoint.vz - facePoint.vz;
        offset.vx = weapon->pos.vx - facePoint.vx;
        offset.vz = weapon->pos.vz - facePoint.vz;
        mag = SquareRoot0(path.vx * path.vx + path.vz * path.vz);
        alpha = (((path.vx * offset.vx + path.vz * offset.vz) / mag) << 16) / mag;
        height = nextPoint.vy - facePoint.vy;
        height = (height * alpha) >> 16;
        if (weapon->pos.vy < facePoint.vy + height - 300)
            weapon->pos.vy = facePoint.vy + height - 300;
        weapon->count--;
        if (weapon->count == 0)
            shapes[weapon->shapeNo].used = 0;
        SetSkeletonPosition(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->pos.vx, weapon->pos.vy, weapon->pos.vz);
        SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->hdg, weapon->pitch, 0);
        for (i = 0; i < 15; i++) {
            if (!(ships[i].combatAttr & DESTROYED) && owner != i) {
                dx = (ships[i].ppivot.vx - weapon->pos.vx) >> 3;
                dy = (ships[i].ppivot.vy - weapon->pos.vy) >> 3;
                dz = (ships[i].ppivot.vz - weapon->pos.vz) >> 3;
                if (GetMagnitude(dx, dy, dz) < 100) {
                    if (!(ships[i].attr & SHIELDED)) {
                        if (i != ownShip) {
                            FlipShip(&ships[i]);
                            ships[i].speed >>= 5;
                            ships[i].vroll += 5000;
                            ships[i].ahdg = (newRand() - 16384) >> 1;
                        } else {
                            ships[i].vpivot.vx -= (ships[i].vpivot.vx * 3) >> 2;
                            ships[i].vpivot.vy -= (ships[i].vpivot.vy * 3) >> 2;
                            ships[i].vpivot.vz -= (ships[i].vpivot.vz * 3) >> 2;
                            ships[i].vroll += newRand() % 250;
                            ships[i].ahdg = (newRand() - 16384) >> 1;
                            if (i == ownShip)
                                SetShake(20);
                        }
                    }
#ifdef NETPLAY
                    DamageCollide(2, i, owner);
#else
                    DamageCollide(2, i);
#endif
                    weapon->count = -1;
                    weapon->target = i;
                    shapes[weapon->shapeNo].used = 0;
                    PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, weapon->pos);
                    return;
                }
            }
        }
        facePtr = track->faces + weapon->section->faceStart;
        for (i = 0; i < weapon->section->faceCount; i++) {
            facePoint.vx = track->vertices[facePtr->vertex[0]].vx;
            facePoint.vy = track->vertices[facePtr->vertex[0]].vy;
            facePoint.vz = track->vertices[facePtr->vertex[0]].vz;
            if (PlaneLineCollision(facePoint, weapon->pos, facePtr->normal) <= 0 && !(facePtr->flags & TRACK_BASE)) {
                weapon->count = -2;
                shapes[weapon->shapeNo].used = 0;
                PlayDirectionalNote(SAMPLE_WEAPON_WALL, 0, 40, weapon->pos);
                return;
            }
            facePtr++;
        }
        Weapon2Mine(weapon, weapons, shapes);
    } else {
        UpdateRocket(ships, shapes, weapon, weapons);
    }
}
#endif
#endif

void DrawMissile(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    shapes[weapon->shapeNo].shapePtr->skeleton->update = 1;
    AddRenderObject(shapes[weapon->shapeNo].shapePtr, 0, 0);
}

#ifdef POWERVR
void InitMissile(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    Face *node;
    signed char ship = weapon->owner;
    int i;

    if (ownShip != ship && weapon->count > 1) {
        weapon->count--;
        return;
    }
    weapon->count = 200;
    weapon->section = ships[ship].nearTrkSect;
    for (i = 0; i < 4; i++) {
        if (shapes[i + 19].used == 0) {
            weapon->shapeNo = i + 19;
            shapes[i + 19].used = 1;
            weapon->target = ships[ship].targetShip;
            node = &track->faces[ships[ship].nearTrkSect->faceStart];
            while (!(node->flags & TRACK_BASE))
                node++;
            GetFireVector(&weapon->accel, ships[ship].unitVecNose, node, ships[ship].ppivot);
            weapon->velocity.vx = ships[ship].vpivot.vx >> 6;
            weapon->velocity.vy = ships[ship].vpivot.vy >> 6;
            weapon->velocity.vz = ships[ship].vpivot.vz >> 6;
            weapon->pos.vx = ships[ship].ppivot.vx;
            weapon->pos.vy = ships[ship].ppivot.vy;
            weapon->pos.vz = ships[ship].ppivot.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ships[ship].hdg, ships[ship].pitch, ships[ship].roll);
            weapon->hdg = ships[ship].hdg;
            weapon->pitch = ships[ship].pitch;
            weapon->update = UpdateMissile;
            weapon->draw = DrawMissile;
            ships[ship].targetShip = -1;
            return;
        }
    }
    weapon->count = 0;
}
#endif

/* The WeaponData.pitch store at S+0x3e still uses the cached pitch instead of the post-callback reload. */
/* NET-only body: this build uses its own InitMissile with ship-indexed accesses. */
#ifdef NETPLAY
void InitMissile(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    Face *node;
    signed char ship = weapon->owner;
    int i;

    if (ship != ownShip && !networkGame) {
        if (weapon->count > 1) {
            weapon->count--;
            return;
        } else {
            weapon->count = 200;
        }
    } else {
        weapon->count = 200;
    }
    weapon->section = ships[ship].nearTrkSect;
    for (i = 0; i < 4; i++) {
        if (shapes[i + 19].used == 0) {
            weapon->shapeNo = i + 19;
            shapes[i + 19].used = 1;
            weapon->target = ships[ship].targetShip;
            node = &track->faces[ships[ship].nearTrkSect->faceStart];
            while (!(node->flags & TRACK_BASE)) node++;
            GetFireVector(&weapon->accel, ships[ship].unitVecNose, node, ships[ship].ppivot);
            weapon->velocity.vx = ships[ship].vpivot.vx >> 6;
            weapon->velocity.vy = ships[ship].vpivot.vy >> 6;
            weapon->velocity.vz = ships[ship].vpivot.vz >> 6;
            weapon->pos.vx = ships[ship].ppivot.vx;
            weapon->pos.vy = ships[ship].ppivot.vy;
            weapon->pos.vz = ships[ship].ppivot.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ships[ship].hdg, ships[ship].pitch, ships[ship].roll);
            weapon->hdg = ships[ship].hdg;
            weapon->pitch = ships[ship].pitch;
            weapon->update = UpdateMissile;
            weapon->draw = DrawMissile;
            ships[ship].targetShip = -1;
            return;
        }
    }
    weapon->count = 0;
}
#endif
#ifdef EQUIV
#if !defined(NETPLAY) && !defined(POWERVR)
void InitMissile(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int i;
    Face *node;
    u_char flag;
    short heading;
    short pitch;
    ShipData *ship;

    if (weapon->owner != ownShip && weapon->count > 1) {
        weapon->count--;
        return;
    }
    weapon->count = 200;
    ship = &ships[weapon->owner];
    weapon->section = ship->nearTrkSect;
    for (i = 0; i < 4; i++) {
        if (shapes[i + 19].used == 0) {
            weapon->shapeNo = i + 19;
            shapes[i + 19].used = 1;
            weapon->target = ship->targetShip;
            node = &track->faces[ship->nearTrkSect->faceStart];
            if (!((flag = node->flags) & TRACK_BASE)) {
                do {
                    node++;
                } while (!((flag = node->flags) & TRACK_BASE));
            }
            GetFireVector(&weapon->accel, ship->unitVecNose, node, ship->ppivot);
            weapon->velocity.vx = ship->vpivot.vx >> 6;
            weapon->velocity.vy = ship->vpivot.vy >> 6;
            weapon->velocity.vz = ship->vpivot.vz >> 6;
            weapon->pos.vx = ship->ppivot.vx;
            weapon->pos.vy = ship->ppivot.vy;
            weapon->pos.vz = ship->ppivot.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ship->hdg, ship->pitch, ship->roll);
            weapon->update = UpdateMissile;
            weapon->draw = DrawMissile;
            ship->targetShip = -1;
            heading = ship->hdg;
            pitch = ship->pitch;
            weapon->hdg = heading;
            weapon->pitch = pitch;
            return;
        }
    }
    weapon->count = 0;
}

#endif
#endif

#ifdef NETPLAY
void UpdateMinigun(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int len;
    VECTOR vertex;
    VECTOR zero;
    int i;
    float vz;
    float vy;
    signed char ship = weapon->owner;
    float vx;
    Face *face;

    zero.vx = 0;
    zero.vy = 0;
    zero.vz = 0;
    weapon->pos.vx += weapon->velocity.vx;
    weapon->pos.vy += weapon->velocity.vy;
    weapon->pos.vz += weapon->velocity.vz;
    weapon->section = FindNearestSection(weapon->section, weapon->pos);
    face = &track->faces[weapon->section->faceStart];
    while (!(face->flags & TRACK_BASE)) face++;
    weapon->count--;
    for (i = 0; i < 15; i++) {
        if (!(ships[i].combatAttr & DESTROYED) && ship != i) {
            vx = (ships[i].ppivot.vx - weapon->pos.vx) >> 3;
            vy = (ships[i].ppivot.vy - weapon->pos.vy) >> 3;
            vz = (ships[i].ppivot.vz - weapon->pos.vz) >> 3;
            len = GetMagnitude(vx, vy, vz);
            if (len < 100) {
                PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, weapon->pos);
                if (!(ships[i].attr & SHIELDED))
#ifdef NETPLAY
                    DamageCollide(13, i, ship);
#else
                    DamageCollide(13, i);
#endif
                weapon->count = -1;
                weapon->pos.vx -= weapon->velocity.vx;
                weapon->pos.vy -= weapon->velocity.vy;
                weapon->pos.vz -= weapon->velocity.vz;
                AddRandomParticles(particleSlots, 8, &weapon->pos, &weapon->velocity);
                weapon->target = i;
                return;
            }
        }
    }
    face = &track->faces[weapon->section->faceStart];
    for (i = 0; i < weapon->section->faceCount; i++) {
        vertex.vx = track->vertices[face->vertex[0]].vx;
        vertex.vy = track->vertices[face->vertex[0]].vy;
        vertex.vz = track->vertices[face->vertex[0]].vz;
        len = PlaneLineCollision(vertex, weapon->pos, face->normal);
        if (len <= 0) {
            weapon->count = -2;
            PlayDirectionalNote(SAMPLE_WEAPON_WALL, 0, 40, weapon->pos);
            weapon->pos.vx -= weapon->velocity.vx;
            weapon->pos.vy -= weapon->velocity.vy;
            weapon->pos.vz -= weapon->velocity.vz;
            AddRandomParticles(particleSlots, 8, &weapon->pos, &zero);
            return;
        }
        face++;
    }
    Weapon2Mine(weapon, weapons, shapes);
}
#else
void UpdateMinigun(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    VECTOR vertex;
    VECTOR zero;
    Face *face;
    float dx, dy, dz;
    int i;
    u_char flag;
    signed char owner = weapon->owner;

    zero.vx = 0;
    zero.vy = 0;
    zero.vz = 0;
    weapon->pos.vx += weapon->velocity.vx;
    weapon->pos.vy += weapon->velocity.vy;
    weapon->pos.vz += weapon->velocity.vz;
    weapon->section = FindNearestSection(weapon->section, weapon->pos);
    face = &track->faces[weapon->section->faceStart];
    while (!((flag = face->flags) & TRACK_BASE)) face++;
    weapon->count--;
    i = 0;
    do {
        if (!(ships[i].combatAttr & DESTROYED) && owner != i) {
            dx = (ships[i].ppivot.vx - weapon->pos.vx) >> 3;
            dy = (ships[i].ppivot.vy - weapon->pos.vy) >> 3;
            dz = (ships[i].ppivot.vz - weapon->pos.vz) >> 3;
            if (GetMagnitude(dx, dy, dz) < 100) {
                PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, weapon->pos);
                if (!(ships[i].attr & SHIELDED))
                    DamageCollide(13, i);
                weapon->count = -1;
                weapon->pos.vx -= weapon->velocity.vx;
                weapon->pos.vy -= weapon->velocity.vy;
                weapon->pos.vz -= weapon->velocity.vz;
                AddRandomParticles(particleSlots, 8, &weapon->pos, &weapon->velocity);
                weapon->target = i;
                return;
            }
        }
        i++;
    } while (i < 15);
    face = &track->faces[weapon->section->faceStart];
    for (i = 0; i < weapon->section->faceCount; i++) {
        vertex.vx = track->vertices[face->vertex[0]].vx;
        vertex.vy = track->vertices[face->vertex[0]].vy;
        vertex.vz = track->vertices[face->vertex[0]].vz;
        if (PlaneLineCollision(vertex, weapon->pos, face->normal) <= 0) {
            weapon->count = -2;
            PlayDirectionalNote(SAMPLE_WEAPON_WALL, 0, 40, weapon->pos);
            weapon->pos.vx -= weapon->velocity.vx;
            weapon->pos.vy -= weapon->velocity.vy;
            weapon->pos.vz -= weapon->velocity.vz;
            AddRandomParticles(particleSlots, 8, &weapon->pos, &zero);
            return;
        }
        face++;
    }
    Weapon2Mine(weapon, weapons, shapes);
}
#endif


void InitMinigun(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    Face *face;
    int i;
    signed char ship = weapon->owner;

    if (ship != ownShip) {
        if (weapon->count > 1) {
            weapon->count--;
            return;
        } else {
            weapon->count = 30;
        }
    } else {
        weapon->count = 30;
    }
    minigunFrame = minigunFrame + 1;
    if (minigunFrame & 3) {
        weapon->count = 0;
        return;
    }
    if (ship == ownShip) {
        ships[ownShip].vpivot.vx -= ships[ownShip].unitVecNose.vx >> 3;
        ships[ownShip].vpivot.vy -= ships[ownShip].unitVecNose.vy >> 3;
        ships[ownShip].vpivot.vz -= ships[ownShip].unitVecNose.vz >> 3;
        PlayNote(SAMPLE_MISSILES, 192, 15, 15, 400);
        PlayNote(SAMPLE_WEAPON_HIT, 96, 15, 15, 400);
    }
    flashNozzleShip = ship;
    weapon->section = ships[ship].nearTrkSect;
    face = &track->faces[ships[ship].nearTrkSect->faceStart];
    while (!(face->flags & TRACK_BASE)) face++;
    weapon->accel.vx = ships[ship].unitVecNose.vx >> 6;
    weapon->accel.vy = ships[ship].unitVecNose.vy >> 6;
    weapon->accel.vz = ships[ship].unitVecNose.vz >> 6;
    weapon->velocity.vx = ships[ship].vpivot.vx >> 6;
    weapon->velocity.vy = ships[ship].vpivot.vy >> 6;
    weapon->velocity.vz = ships[ship].vpivot.vz >> 6;
    weapon->velocity.vx += weapon->accel.vx * 8;
    weapon->velocity.vy += weapon->accel.vy * 8;
    weapon->velocity.vz += weapon->accel.vz * 8;
    weapon->velocity.vy += weapon->accel.vy * 8;
    weapon->pos.vx = ships[ship].ppivot.vx - (weapon->velocity.vx >> 1);
    weapon->pos.vy = ships[ship].ppivot.vy - (weapon->velocity.vy >> 1);
    weapon->pos.vz = ships[ship].ppivot.vz - (weapon->velocity.vz >> 1);
    weapon->hdg = ships[ship].hdg;
    weapon->pitch = ships[ship].pitch + 0x800;
    weapon->update = UpdateMinigun;
    weapon->draw = 0;
    ships[ship].targetShip = -1;
    i = nextParticleSlot;
    nextParticleSlot = (nextParticleSlot + 1) % PARTICLE_COUNT;
    particleSlots[i].pos.vx = weapon->pos.vx;
    particleSlots[i].pos.vy = weapon->pos.vy;
    particleSlots[i].pos.vz = weapon->pos.vz;
    particleSlots[i].vel.vx = (short)weapon->velocity.vx;
    particleSlots[i].vel.vy = (short)weapon->velocity.vy;
    particleSlots[i].vel.vz = (short)weapon->velocity.vz;
    particleSlots[i].life = 15;
    if (!(minigunFrame & 7))
        particleSlots[i].type = 2;
    else
        particleSlots[i].type = 3;
    particleSlots[i].flags = 0;
    minigunAmmo--;
    if (minigunAmmo <= 0)
        ships[ship].weapon = 0;
}



#ifdef NETPLAY
void UpdateMine(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int len;
    Poly poly;
    int i;
    float vz;
    float vy;
    signed char ship = weapon->owner;
    float vx;
    int hover;
    short r;

    weapon->count--;
    if (weapon->count == 0) {
        shapes[weapon->shapeNo].used = 0;
        return;
    }
    if (0) {
    }
    hover = rsin(weapon->shapeNo * 400 + weapon->count * 40) / 512;
    weapon->pos.vx += hover;
    weapon->hdg += 128;
    poly.prim = shapes[weapon->shapeNo].shapePtr->primitives;
    if (0) {
    }
    r = (rsin(weapon->count * 140) >> 6) + 230;
    if (r > 255)
        r = 255;
    for (i = 0; i < 8; i++) {
        switch (poly.prim->type) {
        case TypeF3:
            poly.f3->colour.r = r;
            poly.f3->colour.g = 0x40;
            poly.f3->colour.b = 0x40;
            poly.f3 += 1;
            break;
        }
    }
    SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->hdg, 0, 0);
    SetSkeletonPosition(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->pos.vx, weapon->pos.vy, weapon->pos.vz);
    for (i = 0; i < 15; i++) {
        if (!(ships[i].combatAttr & DESTROYED) && ship != i) {
            vx = (ships[i].ppivot.vx - weapon->pos.vx) >> 2;
            vy = (ships[i].ppivot.vy - weapon->pos.vy) >> 2;
            vz = (ships[i].ppivot.vz - weapon->pos.vz) >> 2;
            len = GetMagnitude(vx, vy, vz);
            if (len < 100) {
                if (!(ships[i].attr & SHIELDED)) {
                    weapon->count = -1;
                    weapon->target = i;
#ifdef NETPLAY
                    if (ownShip != i && !networkGame) {
#else
                    if (i != ownShip) {
#endif
                        ships[i].speed = ships[i].speed >> 3;
                    } else {
                        ships[i].vpivot.vx -= ships[i].vpivot.vx >> 3;
                        ships[i].vpivot.vy -= ships[i].vpivot.vy >> 3;
                        ships[i].vpivot.vz -= ships[i].vpivot.vz >> 3;
                        if (i == ownShip)
                            SetShake(20);
                    }
                }
#ifdef NETPLAY
                DamageCollide(3, i, ship);
#else
                DamageCollide(3, i);
#endif
                weapon->count = -1;
                shapes[weapon->shapeNo].used = 0;
                PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, weapon->pos);
                return;
            }
        }
    }
}
#else
void UpdateMine(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    Poly poly;
    int i;
    int j;
    short r;
    float dx, dy, dz;
    signed char owner = weapon->owner;

    weapon->count--;
    if (weapon->count == 0) {
        shapes[weapon->shapeNo].used = 0;
        return;
    }
    weapon->pos.vx += rsin(weapon->shapeNo * 400 + weapon->count * 40) / 512;
    weapon->hdg += 128;
    poly.prim = shapes[weapon->shapeNo].shapePtr->primitives;
    r = (rsin(weapon->count * 140) >> 6) + 230;
    if (r > 255)
        r = 255;
    for (j = 0; j < 8; j++) {
        switch (poly.prim->type) {
        case TypeF3:
            poly.f3->colour.r = r;
            poly.f3->colour.g = 0x40;
            poly.f3->colour.b = 0x40;
            poly.f3 += 1;
            break;
        }
    }
    SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->hdg, 0, 0);
    SetSkeletonPosition(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->pos.vx, weapon->pos.vy, weapon->pos.vz);
    i = 0;
    do {
        if (!(ships[i].combatAttr & DESTROYED) && owner != i) {
            dx = (ships[i].ppivot.vx - weapon->pos.vx) >> 2;
            dy = (ships[i].ppivot.vy - weapon->pos.vy) >> 2;
            dz = (ships[i].ppivot.vz - weapon->pos.vz) >> 2;
            if (GetMagnitude(dx, dy, dz) < 100) {
                if (!(ships[i].attr & SHIELDED)) {
                    weapon->target = i;
                    weapon->count = -1;
                    if (i != ownShip) {
                        ships[i].speed >>= 3;
                    } else {
                        ships[i].vpivot.vx -= ships[i].vpivot.vx >> 3;
                        ships[i].vpivot.vy -= ships[i].vpivot.vy >> 3;
                        ships[i].vpivot.vz -= ships[i].vpivot.vz >> 3;
                        if (i == ownShip)
                            SetShake(20);
                    }
                }
                DamageCollide(3, i);
                weapon->count = -1;
                shapes[weapon->shapeNo].used = 0;
                PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, weapon->pos);
                return;
            }
        }
        i++;
    } while (i < 15);
}
#endif


void DrawMine(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    if (weapon->count == 0) {
        shapes[weapon->shapeNo].used = 0;
    } else if (shapes[weapon->shapeNo].used) {
        shapes[weapon->shapeNo].shapePtr->skeleton->update = 1;
        AddRenderObject(shapes[weapon->shapeNo].shapePtr, 0, 0);
    }
}

/* NET lets every ship tick the countdown only when not networkGame; locals are spelled with ships[ship]. */
#ifdef NETPLAY
void InitQuake(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    Face *node;
    signed char ship = weapon->owner;

    if (ship != ownShip && !networkGame) {
        if (weapon->count > 1) {
            weapon->count--;
            return;
        } else {
            weapon->count = 70;
        }
    } else {
        weapon->count = 70;
    }
    weapon->section = ships[ship].nearTrkSect;
    node = &track->faces[ships[ship].nearTrkSect->faceStart];
    while (!(node->flags & TRACK_BASE)) node++;
    GetFireVector(&weapon->accel, ships[ship].unitVecNose, node, ships[ship].ppivot);
    weapon->velocity.vx = ships[ship].vpivot.vx >> 6;
    weapon->velocity.vy = ships[ship].vpivot.vy >> 6;
    weapon->velocity.vz = ships[ship].vpivot.vz >> 6;
    weapon->bumpStep = 0;
    weapon->pos.vx = ships[ship].nearTrkSect->nextSection.ptr->nextSection.ptr->centre.vx;
    weapon->pos.vy = ships[ship].nearTrkSect->nextSection.ptr->nextSection.ptr->centre.vy;
    weapon->pos.vz = ships[ship].nearTrkSect->nextSection.ptr->nextSection.ptr->centre.vz;
    weapon->update = UpdateQuake;
    weapon->draw = 0;
}
#else
void InitQuake(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    Face *node;
    u_char flag;
    ShipData *ship;
    short owner;

    owner = weapon->owner;
    if (ownShip != owner && weapon->count > 1) {
        weapon->count--;
        return;
    }
    weapon->count = 70;
    ship = &ships[owner];
    weapon->section = ship->nearTrkSect;
    node = &track->faces[ship->nearTrkSect->faceStart];
    while (!((flag = node->flags) & TRACK_BASE)) node++;
    GetFireVector(&weapon->accel, ship->unitVecNose, node, ship->ppivot);
    weapon->velocity.vx = ship->vpivot.vx >> 6;
    weapon->velocity.vy = ship->vpivot.vy >> 6;
    weapon->velocity.vz = ship->vpivot.vz >> 6;
    weapon->bumpStep = 0;
    weapon->pos.vx = ship->nearTrkSect->nextSection.ptr->nextSection.ptr->centre.vx;
    weapon->pos.vy = ship->nearTrkSect->nextSection.ptr->nextSection.ptr->centre.vy;
    weapon->pos.vz = ship->nearTrkSect->nextSection.ptr->nextSection.ptr->centre.vz;
    weapon->update = UpdateQuake;
    weapon->draw = 0;
}
#endif

/* NET lets every ship tick the countdown only when not networkGame; bumpStep is cleared earlier. */
#ifdef NETPLAY
void InitPlasma(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    Face *node;
    signed char ship = weapon->owner;

    if (plasmaShotBusy)
        return;
    plasmaShotBusy = 1;
    InitPlasmaShot();
    if (ship != ownShip && !networkGame) {
        if (weapon->count > 1) {
            weapon->count--;
            return;
        } else {
            weapon->count = 220;
        }
    } else {
        weapon->count = 220;
    }
    weapon->section = ships[ship].nearTrkSect;
    node = &track->faces[ships[ship].nearTrkSect->faceStart];
    while (!(node->flags & TRACK_BASE)) node++;
    GetFireVector(&weapon->accel, ships[ship].unitVecNose, node, ships[ship].ppivot);
    weapon->velocity.vx = ships[ship].vpivot.vx >> 6;
    weapon->velocity.vy = ships[ship].vpivot.vy >> 6;
    weapon->velocity.vz = ships[ship].vpivot.vz >> 6;
    weapon->bumpStep = 0;
    weapon->pos.vx = ships[ship].ppivot.vx;
    weapon->pos.vy = ships[ship].ppivot.vy;
    weapon->pos.vz = ships[ship].ppivot.vz;
    weapon->update = UpdatePlasma;
    weapon->draw = DrawPlasma;
}
#else
void InitPlasma(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    Face *node;
    u_char flag;
    ShipData *ship;
    signed char owner;

    owner = weapon->owner;
    if (plasmaShotBusy)
        return;
    plasmaShotBusy = 1;
    InitPlasmaShot();
    if (ownShip != owner && weapon->count > 1) {
        weapon->count--;
        return;
    }
    weapon->count = 220;
    ship = &ships[owner];
    weapon->section = ship->nearTrkSect;
    node = &track->faces[ship->nearTrkSect->faceStart];
    while (!((flag = node->flags) & TRACK_BASE)) node++;
    GetFireVector(&weapon->accel, ship->unitVecNose, node, ship->ppivot);
    weapon->velocity.vx = ship->vpivot.vx >> 6;
    weapon->velocity.vy = ship->vpivot.vy >> 6;
    weapon->velocity.vz = ship->vpivot.vz >> 6;
    weapon->pos.vx = ship->ppivot.vx;
    weapon->pos.vy = ship->ppivot.vy;
    weapon->pos.vz = ship->ppivot.vz;
    weapon->update = UpdatePlasma;
    weapon->draw = DrawPlasma;
    weapon->bumpStep = 0;
    weapon->pos.vz = ship->ppivot.vz;
}
#endif

/* NET lets every ship tick the countdown only when not networkGame; locals are spelled with ships[ship]. */
#ifdef NETPLAY
void InitRocket(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    Face *node;
    int i;
    signed char ship = weapon->owner;

    if (ship != ownShip && !networkGame) {
        if (weapon->count > 1) {
            weapon->count--;
            return;
        } else {
            weapon->count = 200;
        }
    } else {
        weapon->count = 200;
    }
    weapon->section = ships[ship].nearTrkSect;
    for (i = 0; i < 4; i++) {
        if (shapes[i].used == 0) {
            weapon->shapeNo = i;
            shapes[i].used = 1;
            node = &track->faces[ships[ship].nearTrkSect->faceStart];
            while (!(node->flags & TRACK_BASE)) node++;
            GetFireVector(&weapon->accel, ships[ship].unitVecNose, node, ships[ship].ppivot);
            weapon->velocity.vx = weapon->accel.vx << 3;
            weapon->velocity.vy = weapon->accel.vy << 3;
            weapon->velocity.vz = weapon->accel.vz << 3;
            weapon->bumpStep = 0;
            weapon->pos.vx = ships[ship].ppivot.vx;
            weapon->pos.vy = ships[ship].ppivot.vy;
            weapon->pos.vz = ships[ship].ppivot.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ships[ship].hdg, ships[ship].pitch, ships[ship].roll);
            weapon->update = UpdateRocket;
            weapon->draw = DrawRocket;
            return;
        }
    }
    weapon->count = 0;
}
#else
void InitRocket(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int i;
    Face *node;
    u_char flag;
    ShipData *ship;
    short owner;

    owner = weapon->owner;
    if (ownShip != owner && weapon->count > 1) {
        weapon->count--;
        return;
    }
    weapon->count = 200;
    ship = &ships[owner];
    weapon->section = ship->nearTrkSect;
    for (i = 0; i < 4; i++) {
        if (shapes[i].used == 0) {
            weapon->shapeNo = i;
            shapes[i].used = 1;
            node = &track->faces[ship->nearTrkSect->faceStart];
            while (!((flag = node->flags) & TRACK_BASE)) node++;
            GetFireVector(&weapon->accel, ship->unitVecNose, node, ship->ppivot);
            weapon->velocity.vx = weapon->accel.vx << 3;
            weapon->velocity.vy = weapon->accel.vy << 3;
            weapon->velocity.vz = weapon->accel.vz << 3;
            weapon->pos.vx = ship->ppivot.vx;
            weapon->pos.vy = ship->ppivot.vy;
            weapon->bumpStep = 0;
            weapon->pos.vz = ship->ppivot.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ship->hdg, ship->pitch, ship->roll);
            weapon->update = UpdateRocket;
            weapon->draw = DrawRocket;
            return;
        }
    }
    weapon->count = 0;
}
#endif

/* NET lets every ship tick the countdown only when not networkGame; locals are spelled with ships[ship]. */
#ifdef NETPLAY
void InitRocketLeft(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    Face *node;
    VECTOR delta;
    int i;
    signed char ship = weapon->owner;

    if (ship != ownShip && !networkGame) {
        if (weapon->count > 1) {
            weapon->count--;
            return;
        } else {
            weapon->count = 200;
        }
    } else {
        weapon->count = 200;
    }
    weapon->section = ships[ship].nearTrkSect;
    for (i = 0; i < 4; i++) {
        if (shapes[i].used == 0) {
            weapon->shapeNo = i;
            shapes[i].used = 1;
            node = &track->faces[ships[ship].nearTrkSect->faceStart];
            while (!(node->flags & TRACK_BASE)) node++;
            GetFireVector(&weapon->accel, ships[ship].unitVecNose, node, ships[ship].ppivot);
            weapon->velocity.vx = weapon->accel.vx << 3;
            weapon->velocity.vy = weapon->accel.vy << 3;
            weapon->velocity.vz = weapon->accel.vz << 3;
            weapon->velocity.vx += ships[ship].unitVecWing.vx >> 6;
            weapon->velocity.vy += ships[ship].unitVecWing.vy >> 6;
            weapon->velocity.vz += ships[ship].unitVecWing.vz >> 6;
            delta.vx = ships[ship].unitVecWing.vx >> 4;
            delta.vy = ships[ship].unitVecWing.vy >> 4;
            delta.vz = ships[ship].unitVecWing.vz >> 4;
            delta.vx -= ships[ship].unitVecNose.vx >> 4;
            delta.vy -= ships[ship].unitVecNose.vy >> 4;
            delta.vz -= ships[ship].unitVecNose.vz >> 4;
            weapon->pos.vx = ships[ship].ppivot.vx + delta.vx;
            weapon->pos.vy = ships[ship].ppivot.vy + delta.vy;
            weapon->pos.vz = ships[ship].ppivot.vz + delta.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ships[ship].hdg, ships[ship].pitch, ships[ship].roll);
            weapon->update = UpdateRocket;
            weapon->draw = DrawRocket;
            return;
        }
    }
    weapon->count = 0;
}
#else
void InitRocketLeft(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int i;
    Face *node;
    u_char flag;
    ShipData *ship;
    short owner;
    VECTOR offset;

    owner = weapon->owner;
    if (ownShip != owner && weapon->count > 1) {
        weapon->count--;
        return;
    }
    weapon->count = 200;
    ship = &ships[owner];
    weapon->section = ship->nearTrkSect;
    for (i = 0; i < 4; i++) {
        if (shapes[i].used == 0) {
            weapon->shapeNo = i;
            shapes[i].used = 1;
            node = &track->faces[ship->nearTrkSect->faceStart];
            while (!((flag = node->flags) & TRACK_BASE)) node++;
            GetFireVector(&weapon->accel, ship->unitVecNose, node, ship->ppivot);
            weapon->velocity.vx = weapon->accel.vx << 3;
            weapon->velocity.vy = weapon->accel.vy << 3;
            weapon->velocity.vz = weapon->accel.vz << 3;
            weapon->velocity.vx += ship->unitVecWing.vx >> 6;
            weapon->velocity.vy += ship->unitVecWing.vy >> 6;
            weapon->velocity.vz += ship->unitVecWing.vz >> 6;
            offset.vx = ship->unitVecWing.vx >> 4;
            offset.vy = ship->unitVecWing.vy >> 4;
            offset.vz = ship->unitVecWing.vz >> 4;
            offset.vx -= ship->unitVecNose.vx >> 4;
            offset.vy -= ship->unitVecNose.vy >> 4;
            offset.vz -= ship->unitVecNose.vz >> 4;
            weapon->pos.vx = ship->ppivot.vx + offset.vx;
            weapon->pos.vy = ship->ppivot.vy + offset.vy;
            weapon->pos.vz = ship->ppivot.vz + offset.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ship->hdg, ship->pitch, ship->roll);
            weapon->update = UpdateRocket;
            weapon->draw = DrawRocket;
            return;
        }
    }
    weapon->count = 0;
}
#endif

/* NET lets every ship tick the countdown only when not networkGame; locals are spelled with ships[ship]. */
#ifdef NETPLAY
void InitRocketRight(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    Face *node;
    VECTOR delta;
    int i;
    signed char ship = weapon->owner;

    if (ship != ownShip && !networkGame) {
        if (weapon->count > 1) {
            weapon->count--;
            return;
        } else {
            weapon->count = 200;
        }
    } else {
        weapon->count = 200;
    }
    weapon->section = ships[ship].nearTrkSect;
    for (i = 0; i < 4; i++) {
        if (shapes[i].used == 0) {
            weapon->shapeNo = i;
            shapes[i].used = 1;
            node = &track->faces[ships[ship].nearTrkSect->faceStart];
            while (!(node->flags & TRACK_BASE)) node++;
            GetFireVector(&weapon->accel, ships[ship].unitVecNose, node, ships[ship].ppivot);
            weapon->velocity.vx = weapon->accel.vx << 3;
            weapon->velocity.vy = weapon->accel.vy << 3;
            weapon->velocity.vz = weapon->accel.vz << 3;
            weapon->velocity.vx -= ships[ship].unitVecWing.vx >> 6;
            weapon->velocity.vy -= ships[ship].unitVecWing.vy >> 6;
            weapon->velocity.vz -= ships[ship].unitVecWing.vz >> 6;
            delta.vx = -ships[ship].unitVecWing.vx >> 4;
            delta.vy = -ships[ship].unitVecWing.vy >> 4;
            delta.vz = -ships[ship].unitVecWing.vz >> 4;
            delta.vx -= ships[ship].unitVecNose.vx >> 4;
            delta.vy -= ships[ship].unitVecNose.vy >> 4;
            delta.vz -= ships[ship].unitVecNose.vz >> 4;
            weapon->pos.vx = ships[ship].ppivot.vx + delta.vx;
            weapon->pos.vy = ships[ship].ppivot.vy + delta.vy;
            weapon->pos.vz = ships[ship].ppivot.vz + delta.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ships[ship].hdg, ships[ship].pitch, ships[ship].roll);
            weapon->update = UpdateRocket;
            weapon->draw = DrawRocket;
            return;
        }
    }
    weapon->count = 0;
}
#else
void InitRocketRight(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int i;
    Face *node;
    u_char flag;
    ShipData *ship;
    short owner;
    VECTOR offset;

    owner = weapon->owner;
    if (ownShip != owner && weapon->count > 1) {
        weapon->count--;
        return;
    }
    weapon->count = 200;
    ship = &ships[owner];
    weapon->section = ship->nearTrkSect;
    for (i = 0; i < 4; i++) {
        if (shapes[i].used == 0) {
            weapon->shapeNo = i;
            shapes[i].used = 1;
            node = &track->faces[ship->nearTrkSect->faceStart];
            while (!((flag = node->flags) & TRACK_BASE)) node++;
            GetFireVector(&weapon->accel, ship->unitVecNose, node, ship->ppivot);
            weapon->velocity.vx = weapon->accel.vx << 3;
            weapon->velocity.vy = weapon->accel.vy << 3;
            weapon->velocity.vz = weapon->accel.vz << 3;
            weapon->velocity.vx -= ship->unitVecWing.vx >> 6;
            weapon->velocity.vy -= ship->unitVecWing.vy >> 6;
            weapon->velocity.vz -= ship->unitVecWing.vz >> 6;
            offset.vx = -ship->unitVecWing.vx >> 4;
            offset.vy = -ship->unitVecWing.vy >> 4;
            offset.vz = -ship->unitVecWing.vz >> 4;
            offset.vx -= ship->unitVecNose.vx >> 4;
            offset.vy -= ship->unitVecNose.vy >> 4;
            offset.vz -= ship->unitVecNose.vz >> 4;
            weapon->pos.vx = ship->ppivot.vx + offset.vx;
            weapon->pos.vy = ship->ppivot.vy + offset.vy;
            weapon->pos.vz = ship->ppivot.vz + offset.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ship->hdg, ship->pitch, ship->roll);
            weapon->update = UpdateRocket;
            weapon->draw = DrawRocket;
            return;
        }
    }
    weapon->count = 0;
}
#endif

/* The collision distances are converted to floats before the magnitude test. */
#ifdef NETPLAY
void UpdateElecBolt(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int closest;
    int dpitch;
    VECTOR pt;
    VECTOR targetVector;
    int j;
    int dhdg;
    float vz;
    float vy;
    int horizDist;
    signed char ship = weapon->owner;
    float vx;
    Face *facePtr;
    int alpha;

    if (weapon->target != -1) {
        targetVector.vx = (ships[weapon->target].ppivot.vx - weapon->pos.vx) >> 3;
        targetVector.vy = (ships[weapon->target].ppivot.vy - weapon->pos.vy) >> 3;
        targetVector.vz = (ships[weapon->target].ppivot.vz - weapon->pos.vz) >> 3;
        horizDist = SquareRoot0(targetVector.vx * targetVector.vx + targetVector.vz * targetVector.vz);
        dhdg = -ratan2(targetVector.vx, targetVector.vz) - weapon->hdg;
        dpitch = -ratan2(targetVector.vy, horizDist) - weapon->pitch;
    } else {
        dhdg = 0;
        dpitch = 0;
    }
    if (dhdg > 2048) {
        dhdg -= 4096;
    } else if (dhdg < -2048) {
        dhdg += 4096;
    }
    if (dpitch > 2048) {
        dpitch -= 4096;
    } else if (dpitch < -2048) {
        dpitch += 4096;
    }
    weapon->hdg += dhdg >> 2;
    weapon->pitch += dpitch >> 2;
    weapon->hdg = ang(weapon->hdg);
    weapon->pitch = ang(weapon->pitch);
    if (0) {
    }
    if (0) {
    }
    weapon->accel.vx = -(rsin(weapon->hdg) * rcos(weapon->pitch)) >> 16;
    if (0) {
    }
    weapon->accel.vy = -rsin(weapon->pitch) >> 4;
    if (0) {
    }
    if (0) {
    }
    weapon->accel.vz = (rcos(weapon->hdg) * rcos(weapon->pitch)) >> 16;
    weapon->velocity.vx += weapon->accel.vx;
    weapon->velocity.vy += weapon->accel.vy;
    weapon->velocity.vz += weapon->accel.vz;
    weapon->velocity.vx -= weapon->velocity.vx >> 2;
    weapon->velocity.vy -= weapon->velocity.vy >> 2;
    weapon->velocity.vz -= weapon->velocity.vz >> 2;
    weapon->pos.vx += weapon->velocity.vx;
    weapon->pos.vy += weapon->velocity.vy;
    weapon->pos.vz += weapon->velocity.vz;
    weapon->section = FindNearestSection(weapon->section, weapon->pos);
    facePtr = track->faces + weapon->section->faceStart;
    while (!(facePtr->flags & TRACK_BASE)) facePtr++;
    pt.vx = track->vertices[facePtr->vertex[0]].vx;
    pt.vy = track->vertices[facePtr->vertex[0]].vy;
    pt.vz = track->vertices[facePtr->vertex[0]].vz;
    alpha = PlaneLineCollision(pt, weapon->pos, facePtr->normal);
    if (alpha < 2000) {
        weapon->pos.vx += ((200 - alpha) * facePtr->normal.vx) >> 13;
        weapon->pos.vy += ((200 - alpha) * facePtr->normal.vy) >> 13;
        weapon->pos.vz += ((200 - alpha) * facePtr->normal.vz) >> 13;
    }
    weapon->count--;
    if (weapon->count == 0) {
        shapes[weapon->shapeNo].used = 0;
    }
    SetSkeletonPosition(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->pos.vx, weapon->pos.vy, weapon->pos.vz);
    SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->hdg, weapon->pitch, 0);
    for (j = 0; j < 15; j++) {
        if (!(ships[j].combatAttr & DESTROYED) && j != ship) {
            vx = (ships[j].ppivot.vx - weapon->pos.vx) >> 3;
            vy = (ships[j].ppivot.vy - weapon->pos.vy) >> 3;
            vz = (ships[j].ppivot.vz - weapon->pos.vz) >> 3;
            closest = GetMagnitude(vx, vy, vz);
            if (closest < 100) {
                if (!(ships[j].attr & SHIELDED)) {
                    ships[j].attr |= ELECTROED;
                    ships[j].electroCount = 140;
                }
#ifdef NETPLAY
                DamageCollide(5, j, ship);
#else
                DamageCollide(5, j);
#endif
                AddRandomParticles(particleSlots, 64, &weapon->pos, &weapon->velocity);
                weapon->count = -1;
                weapon->target = j;
                shapes[weapon->shapeNo].used = 0;
                return;
            }
        }
    }
    facePtr = track->faces + weapon->section->faceStart;
    for (j = 0; j < weapon->section->faceCount; j++) {
        pt.vx = track->vertices[facePtr->vertex[0]].vx;
        pt.vy = track->vertices[facePtr->vertex[0]].vy;
        pt.vz = track->vertices[facePtr->vertex[0]].vz;
        closest = PlaneLineCollision(pt, weapon->pos, facePtr->normal);
        if (closest <= 0) {
            weapon->count = -2;
            shapes[weapon->shapeNo].used = 0;
            PlayDirectionalNote(SAMPLE_WEAPON_WALL, 0, 40, weapon->pos);
            return;
        }
        facePtr++;
    }
    Weapon2Mine(weapon, weapons, shapes);
}
#else
void UpdateElecBolt(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    float dx, dy, dz;
    VECTOR    targetVector;
    VECTOR    facePoint;
    Face     *facePtr;
    int       i;
    int       horizDist;
    int       vhdg;
    int       vpitch;
    int       alpha;
    u_char    flag;
    signed char owner = weapon->owner;

    if (weapon->target != -1) {
        targetVector.vx = (ships[weapon->target].ppivot.vx - weapon->pos.vx) >> 3;
        targetVector.vy = (ships[weapon->target].ppivot.vy - weapon->pos.vy) >> 3;
        targetVector.vz = (ships[weapon->target].ppivot.vz - weapon->pos.vz) >> 3;
        horizDist = SquareRoot0(targetVector.vx * targetVector.vx + targetVector.vz * targetVector.vz);
        vhdg = -ratan2(targetVector.vx, targetVector.vz) - weapon->hdg;
        vpitch = -ratan2(targetVector.vy, horizDist) - weapon->pitch;
    } else {
        vhdg = 0;
        vpitch = 0;
    }
    if (vhdg > 2048) {
        vhdg -= 4096;
    } else if (vhdg < -2048) {
        vhdg += 4096;
    }
    if (vpitch > 2048) {
        vpitch -= 4096;
    } else if (vpitch < -2048) {
        vpitch += 4096;
    }
    weapon->hdg += vhdg >> 2;
    weapon->pitch += vpitch >> 2;
    weapon->hdg = ang(weapon->hdg);
    weapon->pitch = ang(weapon->pitch);
    weapon->accel.vx = (-rsin(weapon->hdg) * rcos(weapon->pitch)) >> 16;
    weapon->accel.vy = -rsin(weapon->pitch) >> 4;
    weapon->accel.vz = (rcos(weapon->hdg) * rcos(weapon->pitch)) >> 16;
    weapon->velocity.vx += weapon->accel.vx;
    weapon->velocity.vy += weapon->accel.vy;
    weapon->velocity.vz += weapon->accel.vz;
    weapon->velocity.vx -= weapon->velocity.vx >> 2;
    weapon->velocity.vy -= weapon->velocity.vy >> 2;
    weapon->velocity.vz -= weapon->velocity.vz >> 2;
    weapon->pos.vx += weapon->velocity.vx;
    weapon->pos.vy += weapon->velocity.vy;
    weapon->pos.vz += weapon->velocity.vz;
    weapon->section = FindNearestSection(weapon->section, weapon->pos);
    facePtr = track->faces + weapon->section->faceStart;
    while (!((flag = facePtr->flags) & TRACK_BASE)) facePtr++;
    facePoint.vx = track->vertices[facePtr->vertex[0]].vx;
    facePoint.vy = track->vertices[facePtr->vertex[0]].vy;
    facePoint.vz = track->vertices[facePtr->vertex[0]].vz;
    alpha = PlaneLineCollision(facePoint, weapon->pos, facePtr->normal);
    if (alpha < 2000) {
        weapon->pos.vx += ((200 - alpha) * facePtr->normal.vx) >> 13;
        weapon->pos.vy += ((200 - alpha) * facePtr->normal.vy) >> 13;
        weapon->pos.vz += ((200 - alpha) * facePtr->normal.vz) >> 13;
    }
    weapon->count--;
    if (weapon->count == 0) {
        shapes[weapon->shapeNo].used = 0;
    }
    SetSkeletonPosition(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->pos.vx, weapon->pos.vy, weapon->pos.vz);
    SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->hdg, weapon->pitch, 0);
    for (i = 0; i < 15; i++) {
        if (!(ships[i].combatAttr & DESTROYED) && i != owner) {
            dx = (ships[i].ppivot.vx - weapon->pos.vx) >> 3;
            dy = (ships[i].ppivot.vy - weapon->pos.vy) >> 3;
            dz = (ships[i].ppivot.vz - weapon->pos.vz) >> 3;
            if (GetMagnitude(dx, dy, dz) < 100) {
                if (!(ships[i].attr & SHIELDED)) {
                    ships[i].attr |= ELECTROED;
                    ships[i].electroCount = 140;
                }
                DamageCollide(5, i);
                AddRandomParticles(particleSlots, 64, &weapon->pos, &weapon->velocity);
                weapon->count = -1;
                weapon->target = i;
                shapes[weapon->shapeNo].used = 0;
                return;
            }
        }
    }
    facePtr = track->faces + weapon->section->faceStart;
    for (i = 0; i < weapon->section->faceCount; i++) {
        facePoint.vx = track->vertices[facePtr->vertex[0]].vx;
        facePoint.vy = track->vertices[facePtr->vertex[0]].vy;
        facePoint.vz = track->vertices[facePtr->vertex[0]].vz;
        if (PlaneLineCollision(facePoint, weapon->pos, facePtr->normal) <= 0) {
            weapon->count = -2;
            shapes[weapon->shapeNo].used = 0;
            PlayDirectionalNote(SAMPLE_WEAPON_WALL, 0, 40, weapon->pos);
            return;
        }
        facePtr++;
    }
    Weapon2Mine(weapon, weapons, shapes);
}
#endif


void UpdateElecBolt(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);

void DrawElecBolt(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    shapes[weapon->shapeNo].shapePtr->skeleton->update = 1;
    AddRenderObject(shapes[weapon->shapeNo].shapePtr, 0, 0);
}

#ifdef NETPLAY
void InitElecBolt(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int i;
    signed char ship = weapon->owner;

#ifdef NETPLAY
    if (ship != ownShip && !networkGame) {
        if (weapon->count > 1) {
            weapon->count--;
            return;
        } else {
            weapon->count = 0x8C;
        }
    } else {
        weapon->count = 0x8C;
    }
#else
    if (ship == ownShip || weapon->count <= 1) {
        weapon->count = 0x8C;
    } else {
        weapon->count--;
        return;
    }
#endif
    weapon->section = ships[ship].nearTrkSect;
    for (i = 0; i < 3; i++) {
        if (shapes[i + 26].used == 0) {
            weapon->shapeNo = i + 26;
            shapes[i + 26].used = 1;
            weapon->target = ships[ship].targetShip;
            weapon->accel.vx = ships[ship].unitVecNose.vx >> 8;
            weapon->accel.vy = ships[ship].unitVecNose.vy >> 8;
            weapon->accel.vz = ships[ship].unitVecNose.vz >> 8;
            weapon->velocity.vx = ships[ship].vpivot.vx >> 6;
            weapon->velocity.vy = ships[ship].vpivot.vy >> 6;
            weapon->velocity.vz = ships[ship].vpivot.vz >> 6;
            weapon->pos.vx = ships[ship].ppivot.vx;
            weapon->pos.vy = ships[ship].ppivot.vy;
            weapon->pos.vz = ships[ship].ppivot.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ships[ship].hdg, ships[ship].pitch, ships[ship].roll);
            weapon->hdg = ships[ship].hdg;
            weapon->pitch = ships[ship].pitch;
            weapon->update = UpdateElecBolt;
            weapon->draw = DrawElecBolt;
            ships[ship].targetShip = -1;
            return;
        }
    }
    weapon->count = 0;
}
#else
void InitElecBolt(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int i;
    signed char owner = weapon->owner;

    if (owner == ownShip || weapon->count <= 1) {
        weapon->count = 0x8C;
    } else {
        weapon->count--;
        return;
    }
    weapon->section = ships[owner].nearTrkSect;
    for (i = 0; i < 3; i++) {
        if (shapes[i + 26].used == 0) {
            weapon->shapeNo = i + 26;
            shapes[i + 26].used = 1;
            weapon->target = ships[owner].targetShip;
            weapon->accel.vx = ships[owner].unitVecNose.vx >> 8;
            weapon->accel.vy = ships[owner].unitVecNose.vy >> 8;
            weapon->accel.vz = ships[owner].unitVecNose.vz >> 8;
            weapon->velocity.vx = ships[owner].vpivot.vx >> 6;
            weapon->velocity.vy = ships[owner].vpivot.vy >> 6;
            weapon->velocity.vz = ships[owner].vpivot.vz >> 6;
            weapon->pos.vx = ships[owner].ppivot.vx;
            weapon->pos.vy = ships[owner].ppivot.vy;
            weapon->pos.vz = ships[owner].ppivot.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ships[owner].hdg, ships[owner].pitch, ships[owner].roll);
            weapon->hdg = ships[owner].hdg;
            weapon->pitch = ships[owner].pitch;
            weapon->update = UpdateElecBolt;
            weapon->draw = DrawElecBolt;
            ships[owner].targetShip = -1;
            return;
        }
    }
    weapon->count = 0;
}
#endif

void InitMine(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int i;
    signed char ship = weapon->owner;

    if (weapon->count == 1) {
        weapon->section = ships[ship].nearTrkSect;
        for (i = 0; i < 15; i++) {
            if (shapes[i + 4].used == 0) {
                weapon->shapeNo = i + 4;
                shapes[i + 4].used = 1;
                weapon->pos.vx = ships[ship].ppivot.vx;
                weapon->pos.vy = ships[ship].ppivot.vy;
                weapon->pos.vz = ships[ship].ppivot.vz;
                weapon->hdg = 0;
                if (ship == ownShip) {
                    PlayNote(SAMPLE_MINE, 200, 15, 15, 256);
                }
                weapon->update = UpdateMine;
                weapon->draw = DrawMine;
                weapon->count = 450;
                return;
            }
        }
        weapon->count = 0;
    } else if (weapon->count > 0) {
        weapon->count--;
    }
}

/* Detonates every active mine and releases the mine and smart-bomb shapes. */
void KillMines(WeaponData *weapon, WeaponData *weapons, WeaponShape *shapes)
{
    int i;
    int destroyed = 0;

    AddExplode(weapon->pos.vx, weapon->pos.vy, weapon->pos.vz, -1);
    for (i = 0; i < 20; i++) {
        if (weapons[i].type == 3 && weapons[i].count > 0) {
            weapons[i].count = 0;
            weapon->count = 0;
            AddExplode(weapons[i].pos.vx, weapons[i].pos.vy, weapons[i].pos.vz, -1);
            shapes[weapons[i].shapeNo].used = 0;
            shapes[weapon->shapeNo].used = 0;
            PlayDirectionalNote(SAMPLE_WEAPON_WALL, 0, 0x28, weapon->pos);
            destroyed++;
        }
    }
}

void UpdateSmartBomb(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void DrawSmartBomb(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);

void InitSmartBomb(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int i;
    signed char ship = weapon->owner;

    weapon->section = ships[ship].nearTrkSect;
    for (i = 0; i < 15; i++) {
        if (shapes[i + 4].used == 0) {
            weapon->shapeNo = i + 4;
            shapes[i + 4].used = 1;
            weapon->pos.vx = ships[ship].ppivot.vx;
            weapon->pos.vy = ships[ship].ppivot.vy;
            weapon->pos.vz = ships[ship].ppivot.vz;
            weapon->velocity.vx = ships[ship].vpivot.vx >> 5;
            weapon->velocity.vy = ships[ship].vpivot.vy >> 5;
            weapon->velocity.vz = ships[ship].vpivot.vz >> 5;
            weapon->hdg = 0;
            if (ship == ownShip) {
                PlayNote(SAMPLE_MINE, 200, 15, 15, 256);
            }
            weapon->update = UpdateSmartBomb;
            weapon->draw = DrawSmartBomb;
            weapon->count = 24;
            return;
        }
    }
    weapon->count = 0;
}

#ifdef NETPLAY
void UpdateSmartBomb(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int i;
    signed char ship = weapon->owner;

    weapon->count--;
    if (weapon->count == 0) {
        fadeAmount = 255;
        PlayNote(SAMPLE_WEAPON_HIT, 255, 15, 15, 256);
        KillMines(weapon, weapons, shapes);
        for (i = 0; i < 15; i++) {
#ifdef NETPLAY
            if (networkGame)
                proximity = ships[ship].netProximity;
#endif
            if ((proximity & (1 << i)) && i != ship && !(ships[i].combatAttr & DESTROYED)) {
#ifdef NETPLAY
                DamageCollide(12, i, ship);
#else
                DamageCollide(12, i);
#endif
                if (!(ships[i].attr & SHIELDED)) {
#ifdef NETPLAY
                    if (ownShip != i && !networkGame) {
#else
                    if (i != ownShip) {
#endif
                        ships[i].speed = ships[i].speed >> 1;
                        ships[i].vroll += 5000;
                        ships[i].ahdg = (newRand() - 16384) >> 1;
                    } else {
                        ships[i].vpivot.vx -= (ships[i].vpivot.vx * 3) >> 1;
                        ships[i].vpivot.vy -= (ships[i].vpivot.vy * 3) >> 1;
                        ships[i].vpivot.vz -= (ships[i].vpivot.vz * 3) >> 1;
                        ships[i].vroll += newRand() % 250;
                        ships[i].ahdg = (newRand() - 16384) >> 3;
                        if (i == ownShip)
                            SetShake(20);
                    }
                    AddExplode(ships[i].ppivot.vx + (newRand() & 1023) - 512,
                               ships[i].ppivot.vy - (newRand() & 511),
                               ships[i].ppivot.vz + (newRand() & 1023) - 512, -1);
                    AddExplode(ships[i].ppivot.vx + (newRand() & 1023) - 512,
                               ships[i].ppivot.vy - (newRand() & 511),
                               ships[i].ppivot.vz + (newRand() & 1023) - 512, -1);
                    PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, ships[i].ppivot);
                }
            }
        }
        weapon->count = 0;
        shapes[weapon->shapeNo].used = 0;
        return;
    }
    weapon->pos.vy -= 10;
    weapon->velocity.vx -= weapon->velocity.vx >> 2;
    weapon->velocity.vy -= weapon->velocity.vy >> 2;
    weapon->velocity.vz -= weapon->velocity.vz >> 2;
    weapon->pos.vx += weapon->velocity.vx;
    weapon->pos.vy += weapon->velocity.vy;
    weapon->pos.vz += weapon->velocity.vz;
    weapon->hdg += 256;
    SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->hdg, 0, 0);
    SetSkeletonPosition(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->pos.vx, weapon->pos.vy, weapon->pos.vz);
}
#else
void UpdateSmartBomb(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    signed char owner = weapon->owner;
    int i;

    weapon->count--;
    if (weapon->count == 0) {
        fadeAmount = 255;
        PlayNote(SAMPLE_WEAPON_HIT, 255, 15, 15, 256);
        KillMines(weapon, weapons, shapes);
        for (i = 0; i < 15; i++) {
            if ((proximity & (1 << i)) && i != owner && !(ships[i].combatAttr & DESTROYED)) {
                DamageCollide(12, i);
                if (!(ships[i].attr & SHIELDED)) {
                    if (i != ownShip) {
                        ships[i].speed >>= 1;
                        ships[i].vroll += 5000;
                        ships[i].ahdg = (newRand() - 16384) >> 1;
                    } else {
                        ships[i].vpivot.vx -= (ships[i].vpivot.vx * 3) >> 1;
                        ships[i].vpivot.vy -= (ships[i].vpivot.vy * 3) >> 1;
                        ships[i].vpivot.vz -= (ships[i].vpivot.vz * 3) >> 1;
                        ships[i].vroll += newRand() % 250;
                        ships[i].ahdg = (newRand() - 16384) >> 3;
                        if (i == ownShip)
                            SetShake(20);
                    }
                    AddExplode(ships[i].ppivot.vx + (newRand() & 1023) - 512,
                               ships[i].ppivot.vy - (newRand() & 511),
                               ships[i].ppivot.vz + (newRand() & 1023) - 512, -1);
                    AddExplode(ships[i].ppivot.vx + (newRand() & 1023) - 512,
                               ships[i].ppivot.vy - (newRand() & 511),
                               ships[i].ppivot.vz + (newRand() & 1023) - 512, -1);
                    PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, ships[i].ppivot);
                }
            }
        }
        weapon->count = 0;
        shapes[weapon->shapeNo].used = 0;
        return;
    }
    weapon->pos.vy -= 10;
    weapon->velocity.vx -= weapon->velocity.vx >> 2;
    weapon->velocity.vy -= weapon->velocity.vy >> 2;
    weapon->velocity.vz -= weapon->velocity.vz >> 2;
    weapon->pos.vx += weapon->velocity.vx;
    weapon->pos.vy += weapon->velocity.vy;
    weapon->pos.vz += weapon->velocity.vz;
    weapon->hdg += 256;
    SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->hdg, 0, 0);
    SetSkeletonPosition(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->pos.vx, weapon->pos.vy, weapon->pos.vz);
}
#endif


void DrawSmartBomb(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    shapes[weapon->shapeNo].shapePtr->skeleton->update = 1;
    AddRenderObject(shapes[weapon->shapeNo].shapePtr, 0, 0);
}

#ifdef POWERVR
#include "shield_hum.h"
void DrawShield(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);



void InitShield(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    short ship = weapon->owner;
    int i;

    shieldGreen[ship] = 0;
    shieldRed[ship] = 0;
    weapon->section = ships[ship].nearTrkSect;
    for (i = 0; i < 3; i++) {
        if (shapes[i + 23].used == 0) {
            weapon->shapeNo = i + 23;
            shapes[i + 23].used = 1;
            weapon->pos.vx = ships[ship].ppivot.vx;
            weapon->pos.vy = ships[ship].ppivot.vy;
            weapon->pos.vz = ships[ship].ppivot.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ships[ship].hdg, ships[ship].pitch, ships[ship].roll);
            weapon->update = UpdateShield;
            weapon->draw = DrawShield;
            ships[ship].attr |= SHIELDED;
            if (ship == ownShip)
                SetShieldHum(1);
            return;
        }
    }
    weapon->count = 0;
}
#endif

#ifdef EQUIV
#if defined(NETPLAY) || defined(POWERVR)
extern int gameFlags;
#include "shield_hum.h"
void DrawShield(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);

#ifdef NETPLAY
void InitShield(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int i;
    signed char ship = weapon->owner;

    shieldGreen[ship] = 0;
    shieldRed[ship] = 0;
    weapon->section = ships[ship].nearTrkSect;
    for (i = 0; i < 3; i++) {
        if (shapes[i + 23].used == 0) {
            weapon->shapeNo = i + 23;
            shapes[i + 23].used = 1;
            weapon->pos.vx = ships[ship].ppivot.vx;
            weapon->pos.vy = ships[ship].ppivot.vy;
            weapon->pos.vz = ships[ship].ppivot.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ships[ship].hdg, ships[ship].pitch, ships[ship].roll);
            if (((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[COPIED_NORMAL_GROUP].count == 0 && !(gameFlags & 2)) {
                ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[COPIED_NORMAL_GROUP].count = ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[normalSetNo].count;
                ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[COPIED_NORMAL_GROUP].data = DAlloc(heap, ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[normalSetNo].count << 3);
                memcpy(((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[COPIED_NORMAL_GROUP].data, ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[normalSetNo].data, ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[normalSetNo].count << 3);
                ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[normalSetNo].count = 0;
            }
            weapon->update = UpdateShield;
            weapon->draw = DrawShield;
            ships[ship].attr |= SHIELDED;
            if (ship == ownShip) {
                SetShieldHum(1);
            }
            return;
        }
    }
    weapon->count = 0;
}

#endif
#endif
#endif

#if !defined(NETPLAY) && !defined(POWERVR)
extern int gameFlags;
#include "shield_hum.h"
void DrawShield(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);

void InitShield(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int i;
    signed char ship = weapon->owner;

    shieldGreen[ship] = 0;
    shieldRed[ship] = 0;
    weapon->section = ships[ship].nearTrkSect;
    for (i = 0; i < 3; i++) {
        if (shapes[i + 23].used == 0) {
            weapon->shapeNo = i + 23;
            shapes[i + 23].used = 1;
            weapon->pos.vx = ships[ship].ppivot.vx;
            weapon->pos.vy = ships[ship].ppivot.vy;
            weapon->pos.vz = ships[ship].ppivot.vz;
            SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ships[ship].hdg, ships[ship].pitch, ships[ship].roll);
            if (((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[COPIED_NORMAL_GROUP].count == 0 && !(gameFlags & 2)) {
                ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[COPIED_NORMAL_GROUP].count = ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[normalSetNo].count;
                ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[COPIED_NORMAL_GROUP].data = DAlloc(heap, ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[normalSetNo].count << 3);
                memcpy(((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[COPIED_NORMAL_GROUP].data, ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[normalSetNo].data, ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[normalSetNo].count << 3);
                ((ShieldNormals *)shapes[weapon->shapeNo].shapePtr->normals)->frame[normalSetNo].count = 0;
            }
            weapon->update = UpdateShield;
            weapon->draw = DrawShield;
            ships[ship].attr |= SHIELDED;
            if (ship == ownShip) {
                SetShieldHum(1);
            }
            return;
        }
    }
    weapon->count = 0;
}

#endif

/* A shield vertex as the PC renderer reads it: a D3D lit vertex with byte colour components. */
typedef struct ShieldVertex {
    float         x, y, z;
    long          reserved;
    unsigned char blue, green, red, alpha;
    long          specular;
    float         tu, tv;
} ShieldVertex;

extern short shieldHumVoice;
extern u_char particleAlpha;
extern int shieldFlash;

u_char FindNearestColourIndex(const struct tagPALETTEENTRY *colour, short count);

/* Animates the shield around a ship: colours the model's vertices and, in the cockpit view, fades the screen. */
#ifdef EQUIV
#ifdef NETPLAY
/* The network build keeps these statements behind switches that are off (an empty
   if/else leaves the jump the network build has after each one). */
#define SHIELD_VIEW_LEAD_ENABLED 1
#define SHIELD_LRPAN_ENABLED 1
#define SHIELD_COCKPIT_LEAD_ENABLED 1
#define SHIELD_COORDS_ENABLED 1
#define SHIELD_COL0_ENABLED 1
#define SHIELD_COL1_ENABLED 1
#define SHIELD_COL3_ENABLED 1
void UpdateShield(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    short        *coordList;
    CVECTOR      tint;
    float        flashTu;
    signed char  ship;
    short        primitiveCount;
    Poly         primitive;
    int          vi;
    short        col0;
    int          i;
    short        col1;
    short        col3;
    short        colC;
    short        fadeVal;
    ShieldVertex *shieldVerts;

    ship = weapon->owner;
    if (shieldGreen[ship] > 0)
        shieldGreen[ship] -= 10;
    if (shieldRed[ship] > 0)
        shieldRed[ship] -= 10;
    if (shieldGreen[ship] < 0)
        shieldGreen[ship] = 0;
    if (shieldRed[ship] < 0)
        shieldRed[ship] = 0;
    if (ships[ship].attr & VIEW_IN) {
        weapon->pos.vx = ships[ship].ppivot.vx + (ships[ship].unitVecNose.vx >> 6);
        weapon->pos.vy = ships[ship].ppivot.vy + (ships[ship].unitVecNose.vy >> 6);
        weapon->pos.vz = ships[ship].ppivot.vz + (ships[ship].unitVecNose.vz >> 6);
    } else {
        weapon->pos.vx = ships[ship].ppivot.vx;
        weapon->pos.vy = ships[ship].ppivot.vy;
        weapon->pos.vz = ships[ship].ppivot.vz;
    }
    weapon->count--;
    fadeVal = 0;
    if (ship == ownShip) {
        if (ships[ownShip].attr & VIEW_IN) {
            if (SHIELD_VIEW_LEAD_ENABLED) { } else { }
            if (SHIELD_LRPAN_ENABLED) {
                /* Preserve the original caller's byte wrap of the computed pan. */
                SfxEffectLRPan(shieldHumVoice, (((rsin(weapon->count * 32) * 14) >> 12) + 15) & 0xff);
            } else { }
            SfxEffectFBPan(shieldHumVoice, ((rcos(weapon->count * 64) * 14) >> 12) + 15);
            SetNoteVolume(shieldHumVoice, 255 - fadeVal);
        } else {
            SetNoteVolume(shieldHumVoice, 245 - fadeVal);
            SfxEffectLRPan(shieldHumVoice, 15);
            SfxEffectFBPan(shieldHumVoice, 30);
        }
    }
    if (weapon->count < 20)
        fadeVal = (20 - weapon->count) << 1;
    if (weapon->count == 0 || (ships[ship].attr & RESCUE)) {
        weapon->count = 0;
        fadeRed = 0;
        fadeBlue = 0;
        fadeGreen = 0;
        shapes[weapon->shapeNo].used = 0;
        ships[ship].attr &= ~SHIELDED;
        if (ship == ownShip)
            SetShieldHum(0);
    }
    if (!(ships[ship].attr & VIEW_IN) || ship != ownShip) {
        SetSkeletonPosition(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->pos.vx, weapon->pos.vy, weapon->pos.vz);
        SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ships[ship].hdg, ships[ship].pitch, ships[ship].roll);
        primitive.prim = shapes[weapon->shapeNo].shapePtr->primitives;
        primitiveCount = shapes[weapon->shapeNo].shapePtr->primitiveCount;
        shieldVerts = (ShieldVertex *)shapes[weapon->shapeNo].shapePtr->floatVerts;
        tint.r = shieldRed[ship];
        tint.g = shieldGreen[ship];
        col0 = 128 - fadeVal;
        if (col0 < 0)
            col0 = 0;
        tint.b = col0;
        flashTu = (unsigned long)FindNearestColourIndex(&tint, 1) / 255.0f;
        i = 0;
        vi = 0;
        for (; i < primitiveCount; i++) {
            switch (primitive.prim->type) {
            case TypeG3:
                if (SHIELD_COORDS_ENABLED) {
                    coordList = primitive.g3->coords;
                } else { }
                if (SHIELD_COL0_ENABLED) {
                    col0 = (rsin(weapon->count * coordList[0]) >> 5) + 128;
                } else { }
                if (SHIELD_COL1_ENABLED) {
                    col1 = (rsin(weapon->count * coordList[1]) >> 5) + 128;
                } else { }
                colC = (rsin(weapon->count * coordList[2]) >> 5) + 128;
                if (col0 > 255)
                    col0 = 255;
                if (col1 > 255)
                    col1 = 255;
                if (colC > 255)
                    colC = 255;
                col0 = col0 - fadeVal;
                col1 = col1 - fadeVal;
                colC = colC - fadeVal;
                if (col0 < 0)
                    col0 = 0;
                if (col1 < 0)
                    col1 = 0;
                if (colC < 0)
                    colC = 0;
                shieldVerts[vi].alpha = particleAlpha;
                shieldVerts[vi + 1].alpha = particleAlpha;
                shieldVerts[vi + 2].alpha = particleAlpha;
                shieldVerts[vi].red = shieldRed[ship];
                shieldVerts[vi + 1].red = shieldRed[ship];
                shieldVerts[vi + 2].red = shieldRed[ship];
                shieldVerts[vi].green = shieldGreen[ship];
                shieldVerts[vi + 1].green = shieldGreen[ship];
                shieldVerts[vi + 2].green = shieldGreen[ship];
                shieldVerts[vi].blue = (char)col0 >> 1;
                shieldVerts[vi + 1].blue = (char)col1 >> 1;
                shieldVerts[vi + 2].blue = (char)colC >> 1;
                if (!rgbTextures) {
                    if ((i + shieldFlash) % 4 == 0) {
                        shieldVerts[vi].tu = flashTu;
                        shieldVerts[vi + 1].tu = flashTu;
                        shieldVerts[vi + 2].tu = flashTu;
                    } else {
                        shieldVerts[vi].tu = 0;
                        shieldVerts[vi + 1].tu = 0;
                        shieldVerts[vi + 2].tu = 0;
                    }
                    shieldVerts[vi].tv = 110.0f / 255.0f;
                    shieldVerts[vi + 1].tv = 110.0f / 255.0f;
                    shieldVerts[vi + 2].tv = 110.0f / 255.0f;
                } else if (!(gameFlags & 2)) {
                    if ((i + shieldFlash) % 4 == 0) {
                        shieldVerts[vi].tu = 209.0f / 255.0f;
                        shieldVerts[vi + 1].tu = 209.0f / 255.0f;
                        shieldVerts[vi + 2].tu = 209.0f / 255.0f;
                    } else {
                        shieldVerts[vi].tu = 163.0f / 255.0f;
                        shieldVerts[vi + 1].tu = 163.0f / 255.0f;
                        shieldVerts[vi + 2].tu = 163.0f / 255.0f;
                    }
                    shieldVerts[vi].tv = 187.0f / 255.0f;
                    shieldVerts[vi + 1].tv = 187.0f / 255.0f;
                    shieldVerts[vi + 2].tv = 187.0f / 255.0f;
                }
                primitive.g3 += 1;
                vi += 3;
                break;
            case TypeG4:
                if (SHIELD_COORDS_ENABLED) {
                    coordList = primitive.g4->coords;
                } else { }
                if (SHIELD_COL0_ENABLED) {
                    col0 = (rsin(weapon->count * coordList[0]) >> 5) + 128;
                } else { }
                if (SHIELD_COL1_ENABLED) {
                    col1 = (rsin(weapon->count * coordList[1]) >> 5) + 128;
                } else { }
                colC = (rsin(weapon->count * coordList[2]) >> 5) + 128;
                if (SHIELD_COL3_ENABLED) {
                    col3 = (rsin(weapon->count * coordList[3]) >> 5) + 128;
                } else { }
                if (col0 > 255)
                    col0 = 255;
                if (col1 > 255)
                    col1 = 255;
                if (colC > 255)
                    colC = 255;
                /* Sic: the executable caps colC here rather than col3, so col3 is never capped. */
                if (col3 > 255)
                    colC = 255;
                col0 = col0 - fadeVal;
                col1 = col1 - fadeVal;
                colC = colC - fadeVal;
                col3 = col3 - fadeVal;
                if (col0 < 0)
                    col0 = 0;
                if (col1 < 0)
                    col1 = 0;
                if (colC < 0)
                    colC = 0;
                if (col3 < 0)
                    col3 = 0;
                shieldVerts[vi].alpha = particleAlpha;
                shieldVerts[vi + 1].alpha = particleAlpha;
                shieldVerts[vi + 2].alpha = particleAlpha;
                shieldVerts[vi + 3].alpha = particleAlpha;
                shieldVerts[vi].red = shieldRed[ship];
                shieldVerts[vi + 1].red = shieldRed[ship];
                shieldVerts[vi + 2].red = shieldRed[ship];
                shieldVerts[vi + 3].red = shieldRed[ship];
                shieldVerts[vi].green = shieldGreen[ship];
                shieldVerts[vi + 1].green = shieldGreen[ship];
                shieldVerts[vi + 2].green = shieldGreen[ship];
                shieldVerts[vi + 3].green = shieldGreen[ship];
                shieldVerts[vi].blue = (char)col0 >> 1;
                shieldVerts[vi + 1].blue = (char)col1 >> 1;
                shieldVerts[vi + 2].blue = (char)colC >> 1;
                shieldVerts[vi + 3].blue = (char)col3 >> 1;
                if (!rgbTextures) {
                    if ((i + shieldFlash) % 4 == 0) {
                        shieldVerts[vi].tu = flashTu;
                        shieldVerts[vi + 1].tu = flashTu;
                        shieldVerts[vi + 2].tu = flashTu;
                        shieldVerts[vi + 3].tu = flashTu;
                    } else {
                        shieldVerts[vi].tu = 0;
                        shieldVerts[vi + 1].tu = 0;
                        shieldVerts[vi + 2].tu = 0;
                        shieldVerts[vi + 3].tu = 0;
                    }
                    shieldVerts[vi].tv = 110.0f / 255.0f;
                    shieldVerts[vi + 1].tv = 110.0f / 255.0f;
                    shieldVerts[vi + 2].tv = 110.0f / 255.0f;
                    shieldVerts[vi + 3].tv = 110.0f / 255.0f;
                } else if (!(gameFlags & 2)) {
                    if ((i + shieldFlash) % 4 == 0) {
                        shieldVerts[vi].tu = 209.0f / 255.0f;
                        shieldVerts[vi + 1].tu = 209.0f / 255.0f;
                        shieldVerts[vi + 2].tu = 209.0f / 255.0f;
                        shieldVerts[vi + 3].tu = 209.0f / 255.0f;
                    } else {
                        shieldVerts[vi].tu = 163.0f / 255.0f;
                        shieldVerts[vi + 1].tu = 163.0f / 255.0f;
                        shieldVerts[vi + 2].tu = 163.0f / 255.0f;
                        shieldVerts[vi + 3].tu = 163.0f / 255.0f;
                    }
                    shieldVerts[vi].tv = 187.0f / 255.0f;
                    shieldVerts[vi + 1].tv = 187.0f / 255.0f;
                    shieldVerts[vi + 2].tv = 187.0f / 255.0f;
                    shieldVerts[vi + 3].tv = 187.0f / 255.0f;
                }
                primitive.g4 += 1;
                vi += 4;
                break;
            }
        }
    } else {
        if (SHIELD_COCKPIT_LEAD_ENABLED) { } else { }
        col0 = (rsin(weapon->count * 128) >> 7) + 64;
        col0 = col0 - fadeVal;
        if (col0 < 0)
            col0 = 0;
        fadeRed = shieldRed[weapon->owner] << 2;
        fadeGreen = shieldGreen[weapon->owner] << 2;
        fadeBlue = col0 << 2;
        fadeActive = 1;
    }
}
#else
void UpdateShield(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int          k;
    CVECTOR      colour;
    short        owner;
    short        fade;
    short        col0;
    short        col1;
    short        col2;
    short        col3;
    short        primitiveCount;
    int          vertexIndex;
    Poly         poly;
    short       *coords;
    ShieldVertex *verts;
    float        flash;
    ShipData    *ship;
    unsigned long shade;

    owner = weapon->owner;
    if (shieldGreen[owner] > 0)
        shieldGreen[owner] -= 10;
    if (shieldRed[owner] > 0)
        shieldRed[owner] -= 10;
    if (shieldGreen[owner] < 0)
        shieldGreen[owner] = 0;
    if (shieldRed[owner] < 0)
        shieldRed[owner] = 0;
    ship = &ships[owner];
    if (ship->attr & VIEW_IN) {
        weapon->pos.vx = ship->ppivot.vx + (ship->unitVecNose.vx >> 6);
        weapon->pos.vy = ship->ppivot.vy + (ship->unitVecNose.vy >> 6);
        weapon->pos.vz = ship->ppivot.vz + (ship->unitVecNose.vz >> 6);
    } else {
        weapon->pos.vx = ship->ppivot.vx;
        weapon->pos.vy = ship->ppivot.vy;
        weapon->pos.vz = ship->ppivot.vz;
    }
    fade = 0;
    weapon->count--;
    if (owner == ownShip) {
        if (!(ships[ownShip].attr & VIEW_IN)) {
            SetNoteVolume(shieldHumVoice, 245);
            SfxEffectLRPan(shieldHumVoice, 15);
            SfxEffectFBPan(shieldHumVoice, 30);
        } else {
            /* Preserve the original caller's byte wrap of the computed pan. */
            SfxEffectLRPan(shieldHumVoice, (((rsin(weapon->count * 32) * 14) >> 12) + 15) & 0xff);
            SfxEffectFBPan(shieldHumVoice, ((rcos(weapon->count * 64) * 14) >> 12) + 15);
            SetNoteVolume(shieldHumVoice, 255);
        }
    }
    if (weapon->count < 20)
        fade = (20 - weapon->count) << 1;
    if (weapon->count == 0 || (ship->attr & RESCUE)) {
        weapon->count = 0;
        fadeRed = 0;
        fadeBlue = 0;
        fadeGreen = 0;
        shapes[weapon->shapeNo].used = 0;
        ship->attr &= ~SHIELDED;
        if (owner == ownShip)
            SetShieldHum(0);
    }
    if (!(ship->attr & VIEW_IN) || owner != ownShip) {
        SetSkeletonPosition(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->pos.vx, weapon->pos.vy, weapon->pos.vz);
        SetSkeletonDirectionHPR(shapes[weapon->shapeNo].shapePtr->skeleton, ship->hdg, ship->pitch, ship->roll);
        poly.prim = shapes[weapon->shapeNo].shapePtr->primitives;
        primitiveCount = shapes[weapon->shapeNo].shapePtr->primitiveCount;
        verts = (ShieldVertex *)shapes[weapon->shapeNo].shapePtr->floatVerts;
        colour.r = shieldRed[owner];
        colour.g = shieldGreen[owner];
        col0 = 128 - fade;
        if (col0 < 0)
            col0 = 0;
        colour.b = col0;
        shade = FindNearestColourIndex(&colour, 1);
        flash = shade * (1.0f / 255.0f);
        k = 0;
        vertexIndex = 0;
        for (; k < primitiveCount; k++) {
            switch (poly.prim->type) {
            case TypeG3:
                coords = poly.g3->coords;
                col0 = (rsin(weapon->count * coords[0]) >> 5) + 128;
                col1 = (rsin(weapon->count * coords[1]) >> 5) + 128;
                col2 = (rsin(weapon->count * coords[2]) >> 5) + 128;
                if (col0 > 255)
                    col0 = 255;
                if (col1 > 255)
                    col1 = 255;
                if (col2 > 255)
                    col2 = 255;
                col0 = col0 - fade;
                col1 = col1 - fade;
                col2 = col2 - fade;
                if (col0 < 0)
                    col0 = 0;
                if (col1 < 0)
                    col1 = 0;
                if (col2 < 0)
                    col2 = 0;
                verts[vertexIndex].alpha = particleAlpha;
                verts[vertexIndex + 1].alpha = particleAlpha;
                verts[vertexIndex + 2].alpha = particleAlpha;
                verts[vertexIndex].red = shieldRed[owner];
                verts[vertexIndex + 1].red = shieldRed[owner];
                verts[vertexIndex + 2].red = shieldRed[owner];
                verts[vertexIndex].green = shieldGreen[owner];
                verts[vertexIndex + 1].green = shieldGreen[owner];
                verts[vertexIndex + 2].green = shieldGreen[owner];
                verts[vertexIndex].blue = (char)col0 >> 1;
                verts[vertexIndex + 1].blue = (char)col1 >> 1;
                verts[vertexIndex + 2].blue = (char)col2 >> 1;
                if (!rgbTextures) {
                    if ((shieldFlash + k) % 4 == 0) {
                        verts[vertexIndex].tu = flash;
                        verts[vertexIndex + 1].tu = flash;
                        verts[vertexIndex + 2].tu = flash;
                    } else {
                        verts[vertexIndex].tu = 0;
                        verts[vertexIndex + 1].tu = 0;
                        verts[vertexIndex + 2].tu = 0;
                    }
                    verts[vertexIndex].tv = 110.0f / 255.0f;
                    verts[vertexIndex + 1].tv = 110.0f / 255.0f;
                    verts[vertexIndex + 2].tv = 110.0f / 255.0f;
                } else if (!(gameFlags & 2)) {
                    if ((shieldFlash + k) % 4 == 0) {
                        verts[vertexIndex].tu = 209.0f / 255.0f;
                        verts[vertexIndex + 1].tu = 209.0f / 255.0f;
                        verts[vertexIndex + 2].tu = 209.0f / 255.0f;
                    } else {
                        verts[vertexIndex].tu = 163.0f / 255.0f;
                        verts[vertexIndex + 1].tu = 163.0f / 255.0f;
                        verts[vertexIndex + 2].tu = 163.0f / 255.0f;
                    }
                    verts[vertexIndex].tv = 187.0f / 255.0f;
                    verts[vertexIndex + 1].tv = 187.0f / 255.0f;
                    verts[vertexIndex + 2].tv = 187.0f / 255.0f;
                }
                poly.g3 += 1;
                vertexIndex += 3;
                break;
            case TypeG4:
                coords = poly.g4->coords;
                col0 = (rsin(weapon->count * coords[0]) >> 5) + 128;
                col1 = (rsin(weapon->count * coords[1]) >> 5) + 128;
                col2 = (rsin(weapon->count * coords[2]) >> 5) + 128;
                col3 = (rsin(weapon->count * coords[3]) >> 5) + 128;
                if (col0 > 255)
                    col0 = 255;
                if (col1 > 255)
                    col1 = 255;
                if (col2 > 255)
                    col2 = 255;
                /* Sic: the executable caps col2 here rather than col3, so col3 is never capped. */
                if (col3 > 255)
                    col2 = 255;
                col0 = col0 - fade;
                col1 = col1 - fade;
                col2 = col2 - fade;
                col3 = col3 - fade;
                if (col0 < 0)
                    col0 = 0;
                if (col1 < 0)
                    col1 = 0;
                if (col2 < 0)
                    col2 = 0;
                if (col3 < 0)
                    col3 = 0;
                verts[vertexIndex].alpha = particleAlpha;
                verts[vertexIndex + 1].alpha = particleAlpha;
                verts[vertexIndex + 2].alpha = particleAlpha;
                verts[vertexIndex + 3].alpha = particleAlpha;
                verts[vertexIndex].red = shieldRed[owner];
                verts[vertexIndex + 1].red = shieldRed[owner];
                verts[vertexIndex + 2].red = shieldRed[owner];
                verts[vertexIndex + 3].red = shieldRed[owner];
                verts[vertexIndex].green = shieldGreen[owner];
                verts[vertexIndex + 1].green = shieldGreen[owner];
                verts[vertexIndex + 2].green = shieldGreen[owner];
                verts[vertexIndex + 3].green = shieldGreen[owner];
                verts[vertexIndex].blue = (char)col0 >> 1;
                verts[vertexIndex + 1].blue = (char)col1 >> 1;
                verts[vertexIndex + 2].blue = (char)col2 >> 1;
                verts[vertexIndex + 3].blue = (char)col3 >> 1;
                if (!rgbTextures) {
                    if ((shieldFlash + k) % 4 == 0) {
                        verts[vertexIndex].tu = flash;
                        verts[vertexIndex + 1].tu = flash;
                        verts[vertexIndex + 2].tu = flash;
                        verts[vertexIndex + 3].tu = flash;
                    } else {
                        verts[vertexIndex].tu = 0;
                        verts[vertexIndex + 1].tu = 0;
                        verts[vertexIndex + 2].tu = 0;
                        verts[vertexIndex + 3].tu = 0;
                    }
                    verts[vertexIndex].tv = 110.0f / 255.0f;
                    verts[vertexIndex + 1].tv = 110.0f / 255.0f;
                    verts[vertexIndex + 2].tv = 110.0f / 255.0f;
                    verts[vertexIndex + 3].tv = 110.0f / 255.0f;
                } else if (!(gameFlags & 2)) {
                    if ((shieldFlash + k) % 4 == 0) {
                        verts[vertexIndex].tu = 209.0f / 255.0f;
                        verts[vertexIndex + 1].tu = 209.0f / 255.0f;
                        verts[vertexIndex + 2].tu = 209.0f / 255.0f;
                        verts[vertexIndex + 3].tu = 209.0f / 255.0f;
                    } else {
                        verts[vertexIndex].tu = 163.0f / 255.0f;
                        verts[vertexIndex + 1].tu = 163.0f / 255.0f;
                        verts[vertexIndex + 2].tu = 163.0f / 255.0f;
                        verts[vertexIndex + 3].tu = 163.0f / 255.0f;
                    }
                    verts[vertexIndex].tv = 187.0f / 255.0f;
                    verts[vertexIndex + 1].tv = 187.0f / 255.0f;
                    verts[vertexIndex + 2].tv = 187.0f / 255.0f;
                    verts[vertexIndex + 3].tv = 187.0f / 255.0f;
                }
                poly.g4 += 1;
                vertexIndex += 4;
                break;
            }
        }
    } else {
        col0 = (rsin(weapon->count * 128) >> 7) + 64;
        col0 = col0 - fade;
        if (col0 < 0)
            col0 = 0;
        fadeRed = shieldRed[weapon->owner] << 2;
        fadeGreen = shieldGreen[weapon->owner] << 2;
        fadeBlue = col0 << 2;
        fadeActive = 1;
    }
}
#endif
#endif

/* Cycles the shield flash and draws the model when the cockpit fade is not used. */
void DrawShield(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    int shieldNo;
    signed char owner = weapon->owner;

    shieldFlash--;
    if (shieldFlash < 0) {
        shieldFlash = 11;
    }
    if (!((ships[owner].attr & VIEW_IN) && owner == ownShip)) {
        shapes[weapon->shapeNo].shapePtr->skeleton->update = 1;
        shieldNo = weapon->shapeNo - 23;
        AddRenderObject(shapes[weapon->shapeNo].shapePtr, 1, 0);
    }
}

/* An entry of the reserved palette: the weapon grid colours are written into four of them. */
#include "palette_state.h"

extern int paletteDirty;

/* Animates the colours of the weapon pick-up grids and hands out a weapon to a ship that crosses one. */
/* NET adds networkGame and netWeaponUnique conditions to grid pickup and cheat handling. */
#ifdef NETPLAY
extern u_char netWeaponUnique;

void CycleWeaponGrids(ShipData *ships, struct WeaponGridZone *grids, int noGrids)
{
    int j;
    int i;
    int g;
    Face *f;
    short brightness[4] = { 100, 80, 60, 40 };
    SVECTOR col;

    if (cheatDrone && !networkGame)
        ships[ownShip].weapon = 13;
    for (g = 0; g < noGrids; g++) {
        f = track->faces + grids[g].section->faceStart;
        while (!(f->flags & TRACK_BASE))
            f++;
        for (i = 0; i < 12; i++) {
            if (!(ships[i].combatAttr & DESTROYED)) {
                if ((f->flags & WEAPON_GRID_LEFT) && (ships[i].attr & LSIDE) && grids[g].section == ships[i].nearTrkSect && grids[g].count >= 0) {
                    grids[g].count = -10;
                    if (ownShip == i || networkGame) {
                        if (!ships[i].weapon) {
                            if (!networkGame || netWeaponUnique || 0) {
                                if (ownShip == i)
                                    PlayNote(SAMPLE_PICKUP, 192, 15, 15, 256);
                                GetNewWeapon(ships, i);
                            }
                        }
                    } else {
                        GetNewDroneWeapon(ships, i);
                    }
                }
                f++;
                if ((f->flags & WEAPON_GRID_RIGHT) && !(ships[i].attr & LSIDE) && grids[g].section == ships[i].nearTrkSect && grids[g].count >= 0) {
                    grids[g].count = -10;
                    if (ownShip == i || networkGame) {
                        if (!ships[i].weapon) {
                            if (!networkGame || netWeaponUnique || 0) {
                                if (ownShip == i)
                                    PlayNote(SAMPLE_PICKUP, 192, 15, 15, 256);
                                GetNewWeapon(ships, i);
                            }
                        }
                    } else {
                        GetNewDroneWeapon(ships, i);
                    }
                }
                f--;
            }
        }
        if (grids[g].count >= 0) {
            if (0) {
            }
            col.vx = (rsin(grids[g].angle) >> 5) + 128;
            if (0) {
            }
            col.vy = (rcos(grids[g].angle) >> 5) + 128;
            if (0) {
            }
            col.vz = (rsin(-grids[g].angle) >> 5) + 128;
            if (col.vx > 255)
                col.vx = 255;
            if (col.vy > 255)
                col.vy = 255;
            if (col.vz > 255)
                col.vz = 255;
            grids[g].r = col.vx;
            grids[g].g = col.vy;
            grids[g].b = col.vz;
            if (!rgbTextures) {
                for (j = 244; j < 248; j++) {
                    defaultPaletteEntries[j].peRed = brightness[j - 244] * col.vx / 100;
                    defaultPaletteEntries[j].peGreen = brightness[j - 244] * col.vy / 100;
                    defaultPaletteEntries[j].peBlue = brightness[j - 244] * col.vz / 100;
                }
                paletteDirty |= 1;
            }
        } else {
            grids[g].r = 128;
            grids[g].g = 128;
            grids[g].b = 128;
        }
        if (grids[g].count < 0)
            grids[g].count++;
        grids[g].angle += 32;
        if (grids[g].attr & WEAPON_GRID_LEFT) {
            f->red = grids[g].r;
            f->green = grids[g].g;
            f->blue = grids[g].b;
        }
        if (grids[g].attr & WEAPON_GRID_RIGHT) {
            f++;
            f->red = grids[g].r;
            f->green = grids[g].g;
            f->blue = grids[g].b;
        }
    }
}
#else
void CycleWeaponGrids(ShipData *ships, struct WeaponGridZone *grids, int noGrids)
{
    short           brightness[4] = { 100, 80, 60, 40 };
    short          *scale;
    PALETTEENTRY   *entry;
    Face           *face;
    SVECTOR         colour;
    int             white;
    int             i;
    int             k;

    if (cheatDrone)
        ships[ownShip].weapon = 13;
    if (noGrids > 0) {
        white = 255;
        for (k = 0; k < noGrids; k++) {
            face = track->faces + grids[k].section->faceStart;
            while (!(face->flags & TRACK_BASE))
                face++;
            for (i = 0; i < 12; i++) {
                if (!(ships[i].combatAttr & DESTROYED)) {
                    if ((face->flags & WEAPON_GRID_LEFT) && (ships[i].attr & LSIDE) && grids[k].section == ships[i].nearTrkSect && grids[k].count >= 0) {
                        grids[k].count = -10;
                        if (ownShip == i) {
                            if (!ships[i].weapon) {
                                if (ownShip == i)
                                    PlayNote(SAMPLE_PICKUP, 192, 15, 15, 256);
                                GetNewWeapon(ships, i);
                            }
                        } else {
                            GetNewDroneWeapon(ships, i);
                        }
                    }
                    face++;
                    if ((face->flags & WEAPON_GRID_RIGHT) && !(ships[i].attr & LSIDE) && grids[k].section == ships[i].nearTrkSect && grids[k].count >= 0) {
                        grids[k].count = -10;
                        if (ownShip == i) {
                            if (!ships[i].weapon) {
                                if (ownShip == i)
                                    PlayNote(SAMPLE_PICKUP, 192, 15, 15, 256);
                                GetNewWeapon(ships, i);
                            }
                        } else {
                            GetNewDroneWeapon(ships, i);
                        }
                    }
                    face--;
                }
            }
            if (grids[k].count < 0) {
                grids[k].r = 128;
                grids[k].g = 128;
                grids[k].b = 128;
            } else {
                colour.vx = (rsin(grids[k].angle) >> 5) + 128;
                colour.vy = (rcos(grids[k].angle) >> 5) + 128;
                colour.vz = (rsin(-grids[k].angle) >> 5) + 128;
                if (colour.vx > 255)
                    colour.vx = white;
                if (colour.vy > 255)
                    colour.vy = white;
                if (colour.vz > 255)
                    colour.vz = white;
                grids[k].r = colour.vx;
                grids[k].g = colour.vy;
                grids[k].b = colour.vz;
                if (!rgbTextures) {
                    entry = &reservedPalette[4];
                    scale = brightness;
                    do {
                        entry->peRed = *scale * colour.vx / 100;
                        entry->peGreen = *scale * colour.vy / 100;
                        entry->peBlue = *scale * colour.vz / 100;
                        entry++;
                        scale++;
                    } while (entry < &reservedPalette[8]);
                    paletteDirty |= 1;
                }
            }
            if (grids[k].count < 0)
                grids[k].count++;
            grids[k].angle += 32;
            if (grids[k].attr & WEAPON_GRID_LEFT) {
                face->red = grids[k].r;
                face->green = grids[k].g;
                face->blue = grids[k].b;
            }
            if (grids[k].attr & WEAPON_GRID_RIGHT) {
                face++;
                face->red = grids[k].r;
                face->green = grids[k].g;
                face->blue = grids[k].b;
            }
        }
    }
}
#endif

void GetFireVector(VECTOR *result, VECTOR target, Face *facePtr, VECTOR shipPos)
{
    int alpha;
    int origAlpha;
    VECTOR facePoint;

    facePoint.vx = track->vertices[facePtr->vertex[0]].vx;
    facePoint.vy = track->vertices[facePtr->vertex[0]].vy;
    facePoint.vz = track->vertices[facePtr->vertex[0]].vz;
    target.vx = shipPos.vx + (target.vx >> 6);
    target.vy = shipPos.vy + (target.vy >> 6);
    target.vz = shipPos.vz + (target.vz >> 6);
    alpha = PlaneLineCollision(facePoint, target, facePtr->normal);
    origAlpha = PlaneLineCollision(facePoint, shipPos, facePtr->normal);
    alpha = (alpha * 31) >> 5;
    alpha -= origAlpha;
    result->vx = target.vx - ((alpha * facePtr->normal.vx) >> 12);
    result->vy = target.vy - ((alpha * facePtr->normal.vy) >> 12);
    result->vz = target.vz - ((alpha * facePtr->normal.vz) >> 12);
    result->vx -= shipPos.vx;
    result->vy -= shipPos.vy;
    result->vz -= shipPos.vz;
}

void Weapon2Mine(WeaponData *weapon, WeaponData *weapons, WeaponShape *shapes)
{
    int i;
    int len;
    float vx, vy, vz;

    for (i = 0; i < 20; i++) {
        if (weapons[i].type == 3 && weapons[i].count > 0) {
            vx = (weapons[i].pos.vx - weapon->pos.vx) >> 2;
            vy = (weapons[i].pos.vy - weapon->pos.vy) >> 2;
            vz = (weapons[i].pos.vz - weapon->pos.vz) >> 2;
            len = GetMagnitude(vx, vy, vz);
            if (len < 30) {
                weapons->count = 0;
                shapes[weapons->shapeNo].used = 0;
                weapon->count = -2;
                shapes[weapon->shapeNo].used = 0;
                PlayDirectionalNote(SAMPLE_WEAPON_WALL, 0, 40, weapon->pos);
                return;
            }
        }
    }
}

/* NET adds a networkGame weapon roll from netWeaponChance, with different selection for ordinary races. */
#ifdef NETPLAY
extern u_char netWeaponUnique;
extern u_char netWeaponChance[];

void GetNewWeapon(ShipData *ships, int shipNo)
{
    short rnd;
    u_char choice;
    short place;
    short g;
    ShipData *ship;

    choice = 0;
    ship = ships + shipNo;
    if (networkGame) {
        if (shipDamage[shipNo] >= 75 && randWep() % 2 == 1) {
            choice = 6;
        } else {
            while (1) {
                rnd = randWep() % 100;
                if (rnd < netWeaponChance[0])
                    choice = 12;
                else if (rnd < netWeaponChance[1])
                    choice = 11;
                else if (rnd < netWeaponChance[2])
                    choice = 3;
                else if (rnd < netWeaponChance[3])
                    choice = 5;
                else if (rnd < netWeaponChance[4])
                    choice = 2;
                else if (rnd < netWeaponChance[5])
                    choice = 1;
                else if (rnd < netWeaponChance[6])
                    choice = 10;
                else if (rnd < netWeaponChance[7])
                    choice = 9;
                else if (rnd < netWeaponChance[8]) {
                    if (gameData->actTrackNum != 1 && gameData->actTrackNum != 7)
                        choice = 8;
                    else if (netWeaponUnique == 1)
                        return;
                }
                if ((ship->lastWeapon != choice || netWeaponUnique == 1) && choice != 0)
                    break;
            }
        }
        ship->weapon = choice;
        ship->lastWeapon = choice;
        return;
    }
    if (ownShip == shipNo)
        place = playerPosition;
    else
        place = 1;
    if (shipNo > 11) {
        shipIndex[shipNo].weapon = 0;
        return;
    }
    if (!tempGameData->weaponsOnSigned && gameType != 2) {
        if (ship->weapon == 0) {
        } else {
        }
        return;
    }
    if (ship->weapon == 0) {
        if (shipDamage[shipNo] >= 75 && randWep() % 2 == 1) {
            choice = 6;
        } else {
            if (place == 1)
                g = 0;
            else if (place >= 2 && place <= 4)
                g = 1;
            else if (place >= 5 && place <= 8)
                g = 2;
            else if (place >= 9 && place <= 11)
                g = 3;
            else if (place >= 12)
                g = 4;
            while (1) {
                rnd = randWep() % 100;
                if (rnd < weaponChance[g * 11])
                    choice = 2;
                else if (rnd < weaponChance[g * 11 + 1])
                    choice = 1;
                else if (rnd < weaponChance[g * 11 + 2])
                    choice = 5;
                else if (rnd < weaponChance[g * 11 + 3])
                    choice = 11;
                else if (rnd < weaponChance[g * 11 + 4])
                    choice = 3;
                else if (rnd < weaponChance[g * 11 + 5])
                    choice = 12;
                else if (rnd < weaponChance[g * 11 + 6])
                    choice = 10;
                else if (rnd < weaponChance[g * 11 + 7])
                    choice = 7;
                else if (rnd < weaponChance[g * 11 + 8])
                    choice = 9;
                else if (rnd < weaponChance[g * 11 + 9]) {
                    if (gameData->actTrackNum != 1 && gameData->actTrackNum != 7)
                        choice = 8;
                    else
                        choice = 2;
                }
                if (ship->lastWeapon != choice)
                    break;
            }
        }
        ship->weapon = choice;
        ship->lastWeapon = choice;
    }
}
#else
void GetNewWeapon(ShipData *ships, int shipNo)
{
    ShipData *ship;
    u_char weapon;
    short position;
    short group;
    short chance;
    int groupOffset;

    ship = ships + shipNo;
    if (shipNo == ownShip)
        position = playerPosition;
    if (shipNo > 11) {
        shipIndex[shipNo].weapon = 0;
        return;
    }
    if ((tempGameData->weaponsOn || gameType == 2) && ship->weapon == 0) {
        if (shipDamage[shipNo] >= 75 && randWep() % 2 == 1) {
            weapon = 6;
        } else {
            if (position == 1)
                group = 0;
            else if (position >= 2 && position <= 4)
                group = 1;
            else if (position >= 5 && position <= 8)
                group = 2;
            else if (position >= 9 && position <= 11)
                group = 3;
            else if (position >= 12)
                group = 4;
            groupOffset = group * 11;
            do {
                chance = randWep() % 100;
                if (chance < weaponChance[groupOffset + 0])
                    weapon = 2;
                else if (chance < weaponChance[groupOffset + 1])
                    weapon = 1;
                else if (chance < weaponChance[groupOffset + 2])
                    weapon = 5;
                else if (chance < weaponChance[groupOffset + 3])
                    weapon = 11;
                else if (chance < weaponChance[groupOffset + 4])
                    weapon = 3;
                else if (chance < weaponChance[groupOffset + 5])
                    weapon = 12;
                else if (chance < weaponChance[groupOffset + 6])
                    weapon = 10;
                else if (chance < weaponChance[groupOffset + 7])
                    weapon = 7;
                else if (chance < weaponChance[groupOffset + 8])
                    weapon = 9;
                else if (chance < weaponChance[groupOffset + 9]) {
                    if (gameData->actTrackNum == 1 || gameData->actTrackNum == 7)
                        weapon = 2;
                    else
                        weapon = 8;
                }
            } while (weapon == ship->lastWeapon);
        }
        ship->weapon = weapon;
        ship->lastWeapon = weapon;
    }
}
#endif

/* Chooses a drone weapon from the chance group for its position relative to the player. */
void GetNewDroneWeapon(ShipData *ships, int shipNo)
{
    short rnd;
    u_char w;
    ShipData *ship;
    short front;

    front = 0;
    ship = &ships[shipNo];
    if (tempGameData->weaponsOnSigned) {
        if (!(shipIndex[shipNo].attr & SHIELDED)) {
            if (shipIndex[ownShip].distance >= ship->distance)
                front++;
            while (1) {
                rnd = randWep() % 100;
                if (rnd < weaponDrone[front * 6]) {
                    w = 2;
                } else if (rnd < weaponDrone[front * 6 + 1]) {
                    w = 1;
                } else if (rnd < weaponDrone[front * 6 + 2]) {
                    w = 5;
                } else if (rnd < weaponDrone[front * 6 + 3]) {
                    w = 3;
                } else if (rnd < weaponDrone[front * 6 + 4]) {
                    if (gameData->actTrackNum != 1 && gameData->actTrackNum != 7) {
                        w = 8;
                    } else if (ship->autopilotCount == 0) {
                        w = 123;
                        break;
                    }
                } else if (rnd < weaponDrone[front * 6 + 5]) {
                    w = 123;
                    break;
                }
                if (ship->lastWeapon != w)
                    break;
            }
            if (w != 123) {
                ship->weapon = w;
                ship->lastWeapon = w;
            }
        }
    }
}

#ifdef EQUIV
#ifdef NETPLAY
void UpdateRocket(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    VECTOR toPos;
    VECTOR nextCorner;
    Face *face;
    long rise;
    long mag;
    long t;
    VECTOR nextWing;
    int horizLen;
    signed char farOwner = weapon->owner;
    float xSep, ySep;
    int proj;
    int i;
    float zAmt;
    TrackSection *nextSector;
    int tilt;
    VECTOR dir;
    VECTOR wingOrigin;
    Face *aheadFace;
    int clearance;
    VECTOR facePoint;

    weapon->section = FindNearestSection(weapon->section, weapon->pos);
    nextSector = weapon->section->nextSection.ptr;
    face = track->faces + weapon->section->faceStart;
    aheadFace = track->faces + nextSector->faceStart;
    while (!(face->flags & TRACK_BASE))
        face++;
    while (!(aheadFace->flags & TRACK_BASE))
        aheadFace++;
    weapon->velocity.vx += weapon->accel.vx;
    weapon->velocity.vy += weapon->accel.vy;
    weapon->velocity.vz += weapon->accel.vz;
    weapon->velocity.vx -= weapon->velocity.vx >> 5;
    weapon->velocity.vy -= weapon->velocity.vy >> 5;
    weapon->velocity.vz -= weapon->velocity.vz >> 5;
    weapon->pos.vx += weapon->velocity.vx;
    weapon->pos.vz += weapon->velocity.vz;
    facePoint.vx = track->vertices[face->vertex[0]].vx;
    facePoint.vy = track->vertices[face->vertex[0]].vy;
    facePoint.vz = track->vertices[face->vertex[0]].vz;
    nextCorner.vx = track->vertices[aheadFace->vertex[0]].vx;
    nextCorner.vy = track->vertices[aheadFace->vertex[0]].vy;
    nextCorner.vz = track->vertices[aheadFace->vertex[0]].vz;
    dir.vx = nextCorner.vx - facePoint.vx;
    dir.vy = nextCorner.vy - facePoint.vy;
    dir.vz = nextCorner.vz - facePoint.vz;
    toPos.vx = weapon->pos.vx - facePoint.vx;
    toPos.vz = weapon->pos.vz - facePoint.vz;
    mag = sqrt(dir.vz * dir.vz + dir.vx * dir.vx);
    proj = dir.vz * toPos.vz + toPos.vx * dir.vx;
    rise = proj / mag;
    t = (rise << 16) / mag;
    rise = nextCorner.vy - facePoint.vy;
    rise = (t * rise) >> 16;
    weapon->pos.vy = facePoint.vy + rise - 300;
    horizLen = sqrt(dir.vz * dir.vz + dir.vx * dir.vx);
    tilt = ratan2(dir.vy, horizLen);
    wingOrigin.vx = facePoint.vx;
    wingOrigin.vy = facePoint.vy;
    wingOrigin.vz = facePoint.vz;
    face++;
    nextWing.vx = track->vertices[face->vertex[0]].vx;
    nextWing.vy = track->vertices[face->vertex[0]].vy;
    nextWing.vz = track->vertices[face->vertex[0]].vz;
    dir.vx = nextWing.vx - wingOrigin.vx;
    dir.vz = nextWing.vz - wingOrigin.vz;
    toPos.vx = weapon->pos.vx - wingOrigin.vx;
    toPos.vz = weapon->pos.vz - wingOrigin.vz;
    mag = sqrt(dir.vz * dir.vz + dir.vx * dir.vx);
    proj = dir.vz * toPos.vz + toPos.vx * dir.vx;
    rise = proj / mag;
    t = (rise << 16) / mag;
    rise = nextWing.vy - wingOrigin.vy;
    rise = ((t * rise) >> 16) - (rise >> 1);
    weapon->pos.vy += rise;
    weapon->pitch = 0;
    weapon->count--;
    if (weapon->count == 0)
        shapes[weapon->shapeNo].used = 0;
    SetSkeletonPosition(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->pos.vx, weapon->pos.vy, weapon->pos.vz);
    for (i = 0; i < 15; i++) {
        if (!(ships[i].combatAttr & DESTROYED) && farOwner != i) {
            xSep = (ships[i].ppivot.vx - weapon->pos.vx) >> 2;
            ySep = (ships[i].ppivot.vy - weapon->pos.vy) >> 2;
            zAmt = (ships[i].ppivot.vz - weapon->pos.vz) >> 2;
            clearance = GetMagnitude(xSep, ySep, zAmt);
            if (clearance < 100) {
                if (!(ships[i].attr & SHIELDED)) {
                    if (i != ownShip && !networkGame) {
                        FlipShip(&ships[i]);
                        ships[i].speed = ships[i].speed >> 5;
                        ships[i].vroll += newRand() % 250;
                        ships[i].ahdg = (newRand() - 16384) >> 1;
                    } else {
                        ships[i].vpivot.vx -= (ships[i].vpivot.vx * 3) >> 2;
                        ships[i].vpivot.vy -= (ships[i].vpivot.vy * 3) >> 2;
                        ships[i].vpivot.vz -= (ships[i].vpivot.vz * 3) >> 2;
                        ships[i].vroll += newRand() % 250;
                        ships[i].ahdg = (newRand() - 16384) >> 1;
                        if (i == ownShip)
                            SetShake(20);
                    }
                }
                DamageCollide(1, i, farOwner);
                weapon->count = -1;
                weapon->target = i;
                shapes[weapon->shapeNo].used = 0;
                if (i != ownShip)
                    PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, weapon->pos);
                else
                    PlayNote(SAMPLE_WEAPON_HIT, 255, 15, 15, 256);
                return;
            }
        }
    }
    face = track->faces + weapon->section->faceStart;
    if (!(weapon->section->flags & 1)) {
        for (i = 0; i < weapon->section->faceCount; i++) {
            facePoint.vx = track->vertices[face->vertex[0]].vx;
            facePoint.vy = track->vertices[face->vertex[0]].vy;
            facePoint.vz = track->vertices[face->vertex[0]].vz;
            clearance = PlaneLineCollision(facePoint, weapon->pos, face->normal);
            if (clearance <= 0 && !(face->flags & TRACK_BASE)) {
                weapon->count = -2;
                shapes[weapon->shapeNo].used = 0;
                PlayDirectionalNote(SAMPLE_WEAPON_WALL, 0, 40, weapon->pos);
                return;
            }
            face++;
        }
    }
    Weapon2Mine(weapon, weapons, shapes);
}
#else
void UpdateRocket(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    long mag, alpha, height;
    VECTOR offset;
    int i;
    VECTOR nextPoint;
    VECTOR facePoint;
    VECTOR path;
    Face *facePtr;
    Face *nextFace;
    VECTOR wingNext;
    int j;
    VECTOR wingPoint;
    float dx, dy, dz;
    u_char flag;
    signed char owner = weapon->owner;

    weapon->section = FindNearestSection(weapon->section, weapon->pos);
    facePtr = track->faces + weapon->section->faceStart;
    nextFace = track->faces + weapon->section->nextSection.ptr->faceStart;
    if (!((flag = facePtr->flags) & TRACK_BASE)) {
        do {
            facePtr++;
        } while (!((flag = facePtr->flags) & TRACK_BASE));
    }
    if (!((flag = nextFace->flags) & TRACK_BASE)) {
        do {
            nextFace++;
        } while (!((flag = nextFace->flags) & TRACK_BASE));
    }
    weapon->velocity.vx += weapon->accel.vx;
    weapon->velocity.vy += weapon->accel.vy;
    weapon->velocity.vz += weapon->accel.vz;
    weapon->velocity.vx -= weapon->velocity.vx >> 5;
    weapon->velocity.vy -= weapon->velocity.vy >> 5;
    weapon->velocity.vz -= weapon->velocity.vz >> 5;
    weapon->pos.vx += weapon->velocity.vx;
    weapon->pos.vz += weapon->velocity.vz;
    facePoint.vx = track->vertices[facePtr->vertex[0]].vx;
    facePoint.vy = track->vertices[facePtr->vertex[0]].vy;
    facePoint.vz = track->vertices[facePtr->vertex[0]].vz;
    nextPoint.vx = track->vertices[nextFace->vertex[0]].vx;
    nextPoint.vy = track->vertices[nextFace->vertex[0]].vy;
    nextPoint.vz = track->vertices[nextFace->vertex[0]].vz;
    path.vx = nextPoint.vx - facePoint.vx;
    path.vy = nextPoint.vy - facePoint.vy;
    path.vz = nextPoint.vz - facePoint.vz;
    offset.vx = weapon->pos.vx - facePoint.vx;
    offset.vz = weapon->pos.vz - facePoint.vz;
    mag = SquareRoot0(path.vx * path.vx + path.vz * path.vz);
    alpha = (((path.vx * offset.vx + path.vz * offset.vz) / mag) << 16) / mag;
    height = nextPoint.vy - facePoint.vy;
    height = (height * alpha) >> 16;
    weapon->pos.vy = facePoint.vy + height - 300;
    ratan2(path.vy, mag);
    wingPoint.vx = facePoint.vx;
    wingPoint.vy = facePoint.vy;
    wingPoint.vz = facePoint.vz;
    facePtr++;
    wingNext.vx = track->vertices[facePtr->vertex[0]].vx;
    wingNext.vy = track->vertices[facePtr->vertex[0]].vy;
    wingNext.vz = track->vertices[facePtr->vertex[0]].vz;
    path.vx = wingNext.vx - wingPoint.vx;
    path.vz = wingNext.vz - wingPoint.vz;
    offset.vx = weapon->pos.vx - wingPoint.vx;
    offset.vz = weapon->pos.vz - wingPoint.vz;
    mag = SquareRoot0(path.vx * path.vx + path.vz * path.vz);
    alpha = (((path.vx * offset.vx + path.vz * offset.vz) / mag) << 16) / mag;
    height = wingNext.vy - wingPoint.vy;
    alpha = (height * alpha) >> 16;
    height = alpha - (height >> 1);
    weapon->pitch = 0;
    weapon->pos.vy += height;
    weapon->count--;
    if (weapon->count == 0)
        shapes[weapon->shapeNo].used = 0;
    SetSkeletonPosition(shapes[weapon->shapeNo].shapePtr->skeleton, weapon->pos.vx, weapon->pos.vy, weapon->pos.vz);
    for (i = 0; i < 15; i++) {
        if (!(ships[i].combatAttr & DESTROYED) && owner != i) {
            dx = (ships[i].ppivot.vx - weapon->pos.vx) >> 2;
            dy = (ships[i].ppivot.vy - weapon->pos.vy) >> 2;
            dz = (ships[i].ppivot.vz - weapon->pos.vz) >> 2;
            if (GetMagnitude(dx, dy, dz) < 100) {
                if (!(ships[i].attr & SHIELDED)) {
                    if (i != ownShip) {
                        FlipShip(&ships[i]);
                        ships[i].speed >>= 5;
                        ships[i].vroll += newRand() % 250;
                        ships[i].ahdg = (newRand() - 16384) >> 1;
                    } else {
                        ships[i].vpivot.vx -= (ships[i].vpivot.vx * 3) >> 2;
                        ships[i].vpivot.vy -= (ships[i].vpivot.vy * 3) >> 2;
                        ships[i].vpivot.vz -= (ships[i].vpivot.vz * 3) >> 2;
                        ships[i].vroll += newRand() % 250;
                        ships[i].ahdg = (newRand() - 16384) >> 1;
                        if (i == ownShip)
                            SetShake(20);
                    }
                }
#ifdef NETPLAY
                DamageCollide(1, i, owner);
#else
                DamageCollide(1, i);
#endif
                weapon->count = -1;
                weapon->target = i;
                shapes[weapon->shapeNo].used = 0;
                if (i != ownShip)
                    PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, weapon->pos);
                else
                    PlayNote(SAMPLE_WEAPON_HIT, 255, 15, 15, 256);
                return;
            }
        }
    }
    facePtr = track->faces + weapon->section->faceStart;
    if (!(weapon->section->flags & 1)) {
        for (j = 0; j < weapon->section->faceCount; j++) {
            facePoint.vx = track->vertices[facePtr->vertex[0]].vx;
            facePoint.vy = track->vertices[facePtr->vertex[0]].vy;
            facePoint.vz = track->vertices[facePtr->vertex[0]].vz;
            if (PlaneLineCollision(facePoint, weapon->pos, facePtr->normal) <= 0 && !(facePtr->flags & TRACK_BASE)) {
                weapon->count = -2;
                shapes[weapon->shapeNo].used = 0;
                PlayDirectionalNote(SAMPLE_WEAPON_WALL, 0, 40, weapon->pos);
                return;
            }
            facePtr++;
        }
    }
    Weapon2Mine(weapon, weapons, shapes);
}
#endif
#endif

void DrawRocket(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    shapes[weapon->shapeNo].shapePtr->skeleton->update = 1;
    AddRenderObject(shapes[weapon->shapeNo].shapePtr, 0, 0);
}

#ifndef NETPLAY
void UpdatePlasma(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    VECTOR facePoint;
    VECTOR nextPoint;
    VECTOR path;
    VECTOR offset;
    VECTOR wingPoint;
    VECTOR wingNext;
    Face *facePtr;
    Face *nextFace;
    ShipData *ship;
    int i;
    long mag, alpha, height;
    float dx, dy, dz;
    u_char flag;
    signed char owner = weapon->owner;

    weapon->count--;
    if (weapon->count > 200 && owner == ownShip) {
        SetPolyG4(&plasmaCharge);
        plasmaCharge.r0 = 255;
        plasmaCharge.g0 = 255;
        plasmaCharge.b0 = 255;
        plasmaCharge.r1 = 255;
        plasmaCharge.g1 = 255;
        plasmaCharge.b1 = 255;
        plasmaCharge.r2 = 128;
        plasmaCharge.g2 = 128;
        plasmaCharge.b2 = 128;
        plasmaCharge.r3 = 128;
        plasmaCharge.g3 = 128;
        plasmaCharge.b3 = 128;
        plasmaCharge.x0 = 140;
        plasmaCharge.x1 = 800 - weapon->count * 3;
        plasmaCharge.x2 = 140;
        plasmaCharge.x3 = 800 - weapon->count * 3;
        plasmaCharge.y0 = 70;
        plasmaCharge.y1 = 70;
        plasmaCharge.y2 = 74;
        plasmaCharge.y3 = 74;
        return;
    }
    if (weapon->count == 200) {
        ship = &ships[owner];
        ship->weapon = 0;
        if (cheatWeapon)
            GetCheatWeapon(ships, ownShip);
        weapon->section = ship->nearTrkSect;
        facePtr = track->faces + ship->nearTrkSect->faceStart;
        while (!((flag = facePtr->flags) & TRACK_BASE)) facePtr++;
        GetFireVector(&weapon->accel, ship->unitVecNose, facePtr, ship->ppivot);
        weapon->velocity.vx = ship->vpivot.vx >> 6;
        weapon->velocity.vy = ship->vpivot.vy >> 6;
        weapon->velocity.vz = ship->vpivot.vz >> 6;
        weapon->pos.vx = ship->ppivot.vx;
        weapon->pos.vy = ship->ppivot.vy;
        weapon->bumpStep = 0;
        weapon->pos.vz = ship->ppivot.vz;
    }
    weapon->section = FindNearestSection(weapon->section, weapon->pos);
    facePtr = track->faces + weapon->section->faceStart;
    nextFace = track->faces + weapon->section->nextSection.ptr->faceStart;
    while (!((flag = facePtr->flags) & TRACK_BASE)) facePtr++;
    while (!((flag = nextFace->flags) & TRACK_BASE)) nextFace++;
    weapon->velocity.vx += weapon->accel.vx;
    weapon->velocity.vy += weapon->accel.vy;
    weapon->velocity.vz += weapon->accel.vz;
    weapon->velocity.vx -= weapon->velocity.vx >> 5;
    weapon->velocity.vy -= weapon->velocity.vy >> 5;
    weapon->velocity.vz -= weapon->velocity.vz >> 5;
    weapon->pos.vx += weapon->velocity.vx;
    weapon->pos.vz += weapon->velocity.vz;
    facePoint.vx = track->vertices[facePtr->vertex[0]].vx;
    facePoint.vy = track->vertices[facePtr->vertex[0]].vy;
    facePoint.vz = track->vertices[facePtr->vertex[0]].vz;
    nextPoint.vx = track->vertices[nextFace->vertex[0]].vx;
    nextPoint.vy = track->vertices[nextFace->vertex[0]].vy;
    nextPoint.vz = track->vertices[nextFace->vertex[0]].vz;
    path.vx = nextPoint.vx - facePoint.vx;
    path.vy = nextPoint.vy - facePoint.vy;
    path.vz = nextPoint.vz - facePoint.vz;
    offset.vx = weapon->pos.vx - facePoint.vx;
    offset.vz = weapon->pos.vz - facePoint.vz;
    mag = SquareRoot0(path.vx * path.vx + path.vz * path.vz);
    alpha = (((path.vx * offset.vx + path.vz * offset.vz) / mag) << 16) / mag;
    height = nextPoint.vy - facePoint.vy;
    height = (height * alpha) >> 16;
    weapon->pos.vy = facePoint.vy + height - 300;
    ratan2(path.vy, mag);
    wingPoint.vx = facePoint.vx;
    wingPoint.vy = facePoint.vy;
    wingPoint.vz = facePoint.vz;
    facePtr++;
    wingNext.vx = track->vertices[facePtr->vertex[0]].vx;
    wingNext.vy = track->vertices[facePtr->vertex[0]].vy;
    wingNext.vz = track->vertices[facePtr->vertex[0]].vz;
    path.vx = wingNext.vx - wingPoint.vx;
    path.vz = wingNext.vz - wingPoint.vz;
    offset.vx = weapon->pos.vx - wingPoint.vx;
    offset.vz = weapon->pos.vz - wingPoint.vz;
    mag = SquareRoot0(path.vx * path.vx + path.vz * path.vz);
    alpha = (((path.vx * offset.vx + path.vz * offset.vz) / mag) << 16) / mag;
    height = wingNext.vy - wingPoint.vy;
    alpha = (height * alpha) >> 16;
    height = alpha - (height >> 1);
    weapon->pos.vy += height;
    weapon->pitch = 0;
    NewPlasmaSegment(&weapon->pos);
    if (weapon->count == 0)
        plasmaShotBusy = 0;
    for (i = 0; i < 15; i++) {
        if (!(ships[i].combatAttr & DESTROYED) && owner != i) {
            dx = (ships[i].ppivot.vx - weapon->pos.vx) >> 2;
            dy = (ships[i].ppivot.vy - weapon->pos.vy) >> 2;
            dz = (ships[i].ppivot.vz - weapon->pos.vz) >> 2;
            if (GetMagnitude(dx, dy, dz) < 100) {
                if (!(ships[i].attr & SHIELDED)) {
                    if (i != ownShip) {
                        ships[i].speed >>= 5;
                        ships[i].vroll += newRand() % 250;
                        ships[i].ahdg = (newRand() - 16384) >> 1;
                    } else {
                        ships[i].vpivot.vx -= (ships[i].vpivot.vx * 3) >> 2;
                        ships[i].vpivot.vy -= (ships[i].vpivot.vy * 3) >> 2;
                        ships[i].vpivot.vz -= (ships[i].vpivot.vz * 3) >> 2;
                        ships[i].vroll += newRand() % 250;
                        ships[i].ahdg = (newRand() - 16384) >> 1;
                        if (i == ownShip)
                            SetShake(20);
                    }
                }
#ifdef NETPLAY
                DamageCollide(11, i, owner);
#else
                DamageCollide(11, i);
#endif
                weapon->count = -1;
                weapon->target = i;
                plasmaShotBusy = 0;
                shapes[weapon->shapeNo].used = 0;
                if (i != ownShip)
                    PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, weapon->pos);
                else
                    PlayNote(SAMPLE_WEAPON_HIT, 255, 15, 15, 256);
                return;
            }
        }
    }
    facePtr = track->faces + weapon->section->faceStart;
    if (!(weapon->section->flags & 1)) {
        for (i = 0; i < weapon->section->faceCount; i++) {
            facePoint.vx = track->vertices[facePtr->vertex[0]].vx;
            facePoint.vy = track->vertices[facePtr->vertex[0]].vy;
            facePoint.vz = track->vertices[facePtr->vertex[0]].vz;
            if (PlaneLineCollision(facePoint, weapon->pos, facePtr->normal) <= 0 && !(facePtr->flags & TRACK_BASE)) {
                weapon->count = -2;
                plasmaShotBusy = 0;
                shapes[weapon->shapeNo].used = 0;
                PlayDirectionalNote(SAMPLE_WEAPON_WALL, 0, 40, weapon->pos);
                return;
            }
            facePtr++;
        }
    }
}
#endif
#ifdef EQUIV
#ifdef NETPLAY
void UpdatePlasma(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    VECTOR toPos;
    VECTOR nextCorner;
    Face *face;
    long rise;
    long mag;
    long t;
    VECTOR nextWing;
    int horizLen;
    signed char farOwner = weapon->owner;
    float xSep, ySep;
    int proj;
    int i;
    float zAmt;
    TrackSection *nextSector;
    int tilt;
    VECTOR dir;
    VECTOR wingOrigin;
    Face *aheadFace;
    int clearance;
    VECTOR facePoint;

    weapon->count--;
    if (weapon->count > 200 && farOwner == ownShip) {
        SetPolyG4(&plasmaCharge);
        plasmaCharge.r0 = 255;
        plasmaCharge.g0 = 255;
        plasmaCharge.b0 = 255;
        plasmaCharge.r1 = 255;
        plasmaCharge.g1 = 255;
        plasmaCharge.b1 = 255;
        plasmaCharge.r2 = 128;
        plasmaCharge.g2 = 128;
        plasmaCharge.b2 = 128;
        plasmaCharge.r3 = 128;
        plasmaCharge.g3 = 128;
        plasmaCharge.b3 = 128;
        plasmaCharge.x0 = 140;
        plasmaCharge.x1 = 200 - (weapon->count - 200) * 3;
        plasmaCharge.x2 = 140;
        plasmaCharge.x3 = 200 - (weapon->count - 200) * 3;
        plasmaCharge.y0 = 70;
        plasmaCharge.y1 = 70;
        plasmaCharge.y2 = 74;
        plasmaCharge.y3 = 74;
        return;
    } else if (weapon->count > 200 && networkGame) {
        return;
    }
    if (weapon->count == 200) {
        ships[farOwner].weapon = 0;
        if (cheatWeapon && !networkGame)
            GetCheatWeapon(ships, ownShip);
        weapon->section = ships[farOwner].nearTrkSect;
        face = track->faces + ships[farOwner].nearTrkSect->faceStart;
        while (!(face->flags & TRACK_BASE))
            face++;
        GetFireVector(&weapon->accel, ships[farOwner].unitVecNose, face, ships[farOwner].ppivot);
        weapon->velocity.vx = ships[farOwner].vpivot.vx >> 6;
        weapon->velocity.vy = ships[farOwner].vpivot.vy >> 6;
        weapon->velocity.vz = ships[farOwner].vpivot.vz >> 6;
        weapon->bumpStep = 0;
        weapon->pos.vx = ships[farOwner].ppivot.vx;
        weapon->pos.vy = ships[farOwner].ppivot.vy;
        weapon->pos.vz = ships[farOwner].ppivot.vz;
    }
    weapon->section = FindNearestSection(weapon->section, weapon->pos);
    nextSector = weapon->section->nextSection.ptr;
    face = track->faces + weapon->section->faceStart;
    aheadFace = track->faces + nextSector->faceStart;
    while (!(face->flags & TRACK_BASE))
        face++;
    while (!(aheadFace->flags & TRACK_BASE))
        aheadFace++;
    weapon->velocity.vx += weapon->accel.vx;
    weapon->velocity.vy += weapon->accel.vy;
    weapon->velocity.vz += weapon->accel.vz;
    weapon->velocity.vx -= weapon->velocity.vx >> 5;
    weapon->velocity.vy -= weapon->velocity.vy >> 5;
    weapon->velocity.vz -= weapon->velocity.vz >> 5;
    weapon->pos.vx += weapon->velocity.vx;
    weapon->pos.vz += weapon->velocity.vz;
    facePoint.vx = track->vertices[face->vertex[0]].vx;
    facePoint.vy = track->vertices[face->vertex[0]].vy;
    facePoint.vz = track->vertices[face->vertex[0]].vz;
    nextCorner.vx = track->vertices[aheadFace->vertex[0]].vx;
    nextCorner.vy = track->vertices[aheadFace->vertex[0]].vy;
    nextCorner.vz = track->vertices[aheadFace->vertex[0]].vz;
    dir.vx = nextCorner.vx - facePoint.vx;
    dir.vy = nextCorner.vy - facePoint.vy;
    dir.vz = nextCorner.vz - facePoint.vz;
    toPos.vx = weapon->pos.vx - facePoint.vx;
    toPos.vz = weapon->pos.vz - facePoint.vz;
    mag = sqrt(dir.vz * dir.vz + dir.vx * dir.vx);
    proj = dir.vz * toPos.vz + toPos.vx * dir.vx;
    rise = proj / mag;
    t = (rise << 16) / mag;
    rise = nextCorner.vy - facePoint.vy;
    rise = (t * rise) >> 16;
    weapon->pos.vy = facePoint.vy + rise - 300;
    horizLen = sqrt(dir.vz * dir.vz + dir.vx * dir.vx);
    tilt = ratan2(dir.vy, horizLen);
    wingOrigin.vx = facePoint.vx;
    wingOrigin.vy = facePoint.vy;
    wingOrigin.vz = facePoint.vz;
    face++;
    nextWing.vx = track->vertices[face->vertex[0]].vx;
    nextWing.vy = track->vertices[face->vertex[0]].vy;
    nextWing.vz = track->vertices[face->vertex[0]].vz;
    dir.vx = nextWing.vx - wingOrigin.vx;
    dir.vz = nextWing.vz - wingOrigin.vz;
    toPos.vx = weapon->pos.vx - wingOrigin.vx;
    toPos.vz = weapon->pos.vz - wingOrigin.vz;
    mag = sqrt(dir.vz * dir.vz + dir.vx * dir.vx);
    proj = dir.vz * toPos.vz + toPos.vx * dir.vx;
    rise = proj / mag;
    t = (rise << 16) / mag;
    rise = nextWing.vy - wingOrigin.vy;
    rise = ((t * rise) >> 16) - (rise >> 1);
    weapon->pos.vy += rise;
    weapon->pitch = 0;
    NewPlasmaSegment(&weapon->pos);
    if (weapon->count == 0)
        plasmaShotBusy = 0;
    for (i = 0; i < 15; i++) {
        if (!(ships[i].combatAttr & DESTROYED) && farOwner != i) {
            xSep = (ships[i].ppivot.vx - weapon->pos.vx) >> 2;
            ySep = (ships[i].ppivot.vy - weapon->pos.vy) >> 2;
            zAmt = (ships[i].ppivot.vz - weapon->pos.vz) >> 2;
            clearance = GetMagnitude(xSep, ySep, zAmt);
            if (clearance < 100) {
                if (!(ships[i].attr & SHIELDED)) {
                    if (i != ownShip && !networkGame) {
                        ships[i].speed = ships[i].speed >> 5;
                        ships[i].vroll += newRand() % 250;
                        ships[i].ahdg = (newRand() - 16384) >> 1;
                    } else {
                        ships[i].vpivot.vx -= (ships[i].vpivot.vx * 3) >> 2;
                        ships[i].vpivot.vy -= (ships[i].vpivot.vy * 3) >> 2;
                        ships[i].vpivot.vz -= (ships[i].vpivot.vz * 3) >> 2;
                        ships[i].vroll += newRand() % 250;
                        ships[i].ahdg = (newRand() - 16384) >> 1;
                        if (i == ownShip)
                            SetShake(20);
                    }
                }
                DamageCollide(11, i, farOwner);
                weapon->count = -1;
                plasmaShotBusy = 0;
                weapon->target = i;
                shapes[weapon->shapeNo].used = 0;
                if (i != ownShip)
                    PlayDirectionalNote(SAMPLE_WEAPON_HIT, 0, 40, weapon->pos);
                else
                    PlayNote(SAMPLE_WEAPON_HIT, 255, 15, 15, 256);
                return;
            }
        }
    }
    face = track->faces + weapon->section->faceStart;
    if (!(weapon->section->flags & 1)) {
        for (i = 0; i < weapon->section->faceCount; i++) {
            facePoint.vx = track->vertices[face->vertex[0]].vx;
            facePoint.vy = track->vertices[face->vertex[0]].vy;
            facePoint.vz = track->vertices[face->vertex[0]].vz;
            clearance = PlaneLineCollision(facePoint, weapon->pos, face->normal);
            if (clearance <= 0 && !(face->flags & TRACK_BASE)) {
                weapon->count = -2;
                plasmaShotBusy = 0;
                shapes[weapon->shapeNo].used = 0;
                PlayDirectionalNote(SAMPLE_WEAPON_WALL, 0, 40, weapon->pos);
                return;
            }
            face++;
        }
    }
}
#endif
#endif

void DrawPlasma(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons)
{
    if (weapon->count > 200 && weapon->owner == ownShip) {
        if (!pauseRequested)
            DrawPolyG4(0, &plasmaCharge, 1);
        return;
    }
}

/* NET adds the attacker argument, kill counters and points, and networkGame conditions. */
#ifdef NETPLAY
extern u_char lastKiller;
extern u_char lastVictim;
extern int playerPoints[];

void DamageCollide(int damageType, int shipNo, int attacker)
{
    int scaledDamage;

    if (linkSendFlag != 0 || attractDelayFrames > 0) {
    } else {
        switch (damageType) {
        case 1:
            scaledDamage = 0x1400;
            break;
        case 10:
            scaledDamage = 0x800;
            break;
        case 11:
            scaledDamage = 0x3E700;
            break;
        case 12:
            scaledDamage = 0x1400;
            break;
        case 2:
            scaledDamage = 0xA00;
            break;
        case 3:
            scaledDamage = 0x500;
            break;
        case 13:
            scaledDamage = 0x400;
            break;
        case 5:
            scaledDamage = 0x500;
            break;
        case 0x80:
            scaledDamage = -0x100;
            break;
        case 0x82:
            scaledDamage = 0x40;
            break;
        case 0x81:
            scaledDamage = 0x80;
            break;
        }
        if (networkGame && damageType == 12)
            scaledDamage = 0xA00;
        if (damageType != 0x80) {
            if (shipNo < 3)
                scaledDamage = (scaledDamage * 3) >> 1;
            if (shipNo > 2 && shipNo < 6)
                scaledDamage += scaledDamage >> 2;
            if (shipNo > 5 && shipNo < 9)
                scaledDamage = (scaledDamage * 3) >> 2;
            if (shipNo > 11 && shipNo < 15)
                scaledDamage >>= 1;
        }
        scaledDamage = (scaledDamage * 3) >> 1;
        if (damageType == 0x80) {
            if (gameData->shipClass == 0)
                scaledDamage = -0xA0;
            if (gameData->shipClass == 1)
                scaledDamage = -0xC8;
            if (gameData->shipClass == 2)
                scaledDamage = -0xF0;
            if (gameData->shipClass == 3)
                scaledDamage = -0x118;
            switch (gameData->actTrackNum) {
            case 1:
                scaledDamage = (scaledDamage * 29) / 31;
                break;
            case 2:
                scaledDamage = (scaledDamage * 29) / 39;
                break;
            case 6:
                scaledDamage = (scaledDamage * 29) / 23;
                break;
            case 7:
                scaledDamage = (scaledDamage * 29) / 18;
                break;
            case 8:
                scaledDamage = (scaledDamage * 29) / 23;
                break;
            case 13:
                scaledDamage = (scaledDamage * 29) / 20;
                break;
            case 17:
                scaledDamage = (scaledDamage * 29) / 15;
                break;
            case 20:
                scaledDamage = (scaledDamage * 29) / 18;
                break;
            }
            if (ownShip != shipNo && !networkGame)
                scaledDamage = (scaledDamage * 3) >> 2;
        }
        if (ownShip == shipNo && damageType != 0x80)
            perfectLap = 0;
        if (shipIndex[shipNo].attr & SHIELDED) {
            if (scaledDamage > 0)
                scaledDamage = 0;
        }
        fracdamage[shipNo] += scaledDamage;
        if (fracdamage[shipNo] < 0)
            fracdamage[shipNo] = 0;
        shipDamage[shipNo] = fracdamage[shipNo] >> 8;
        if (cheatDamage)
            shipDamage[ownShip] = 0;
        if (shipDamage[shipNo] > 100) {
            if (shipDeathCount[shipNo] == -1) {
                shipDamage[shipNo] = 100;
                if (ownShip == shipNo || networkGame) {
                    if (raceContext != 1) {
                        shipDeathCount[shipNo] = 100;
                        if (ownShip == shipNo) {
                            shipIndex[shipNo].attr &= ~VIEW_IN;
                            CameraUpdate = UpdateCameraAttractDestroyed;
                            if (deathCameraActive)
                                deathCameraActive = 0;
                        }
                    }
                } else {
                    shipDeathCount[shipNo] = 15;
                }
                if (networkGame && attacker != -1) {
                    lastKiller = attacker;
                    lastVictim = shipNo;
                    killMessageTimer = 60;
                    killerCount[attacker]++;
                    killerTotal[attacker]++;
                    playerPoints[lastVictim]--;
                    printf("Player Killed: PlayerPoints[ %d ] == %d\n", lastVictim, playerPoints[lastVictim]);
                    playerPoints[lastKiller]++;
                    printf("Player Killer: PlayerPoints[ %d ] == %d\n", lastKiller, playerPoints[lastKiller]);
                }
            }
        }
        if (damageType != 0x80 && damageType != 0x81) {
            shieldGreen[shipNo] = 255;
            shieldRed[shipNo] = 255;
        }
        if (damageType == 0x81)
            shieldRed[shipNo] = 255;
        if (previousPlayerDamage < 60 && shipDamage[ownShip] >= 60 && ownShip == shipNo)
            PlayNote(SAMPLE_LOW_ENERGY, 510, 15, 15, 256);
        previousPlayerDamage = shipDamage[ownShip];
    }
}
#else
void DamageCollide(int damageType, int shipNo)
{
    int scaledDamage;
    int shipAttributes;
    signed char classIndex;
    signed char trackIndex;
    int *attrPtr;

    if (linkSendFlag == 0 && attractDelayFrames <= 0) {
        switch (damageType) {
        case 1:
            scaledDamage = 0x1400;
            break;
        case 2:
            scaledDamage = 0xA00;
            break;
        case 3:
            scaledDamage = 0x500;
            break;
        case 5:
            scaledDamage = 0x500;
            break;
        case 10:
            scaledDamage = 0x800;
            break;
        case 11:
            scaledDamage = 0x3E700;
            break;
        case 12:
            scaledDamage = 0x1400;
            break;
        case 13:
            scaledDamage = 0x400;
            break;
        case 0x80:
            scaledDamage = -0x100;
            break;
        case 0x81:
            scaledDamage = 0x80;
            break;
        case 0x82:
            scaledDamage = 0x40;
            break;
        }
        if (damageType != 0x80) {
            if (shipNo < 3)
                scaledDamage = (scaledDamage * 3) >> 1;
            if (shipNo > 2 && shipNo < 6)
                scaledDamage += scaledDamage >> 2;
            if (shipNo > 5 && shipNo < 9)
                scaledDamage = (scaledDamage * 3) >> 2;
            if (shipNo > 11 && shipNo < 15)
                scaledDamage >>= 1;
        }
        scaledDamage = (scaledDamage * 3) >> 1;
        if (damageType == 0x80) {
            classIndex = gameData->shipClass;
            if (classIndex == 0)
                scaledDamage = -0xA0;
            if (classIndex == 1)
                scaledDamage = -0xC8;
            if (classIndex == 2)
                scaledDamage = -0xF0;
            if (classIndex == 3)
                scaledDamage = -0x118;
            trackIndex = gameData->actTrackNum;
            switch (trackIndex) {
            case 1:
                scaledDamage = (scaledDamage * 29) / 31;
                break;
            case 2:
                scaledDamage = (scaledDamage * 29) / 39;
                break;
            case 6:
                scaledDamage = (scaledDamage * 29) / 23;
                break;
            case 8:
                scaledDamage = (scaledDamage * 29) / 23;
                break;
            case 13:
                scaledDamage = (scaledDamage * 29) / 20;
                break;
            case 17:
                scaledDamage = (scaledDamage * 29) / 15;
                break;
            case 7:
                scaledDamage = (scaledDamage * 29) / 18;
                break;
            case 20:
                scaledDamage = (scaledDamage * 29) / 18;
                break;
            }
            if (shipNo != ownShip)
                scaledDamage = (scaledDamage * 3) >> 2;
        }
        if (shipNo == ownShip && damageType != 0x80)
            perfectLap = 0;
        attrPtr = &shipIndex[shipNo].attr;
        if ((*attrPtr & SHIELDED) && scaledDamage > 0)
            scaledDamage = 0;
        fracdamage[shipNo] += scaledDamage;
        if (fracdamage[shipNo] < 0)
            fracdamage[shipNo] = 0;
        shipDamage[shipNo] = fracdamage[shipNo] >> 8;
        if (cheatDamage != 0)
            shipDamage[ownShip] = 0;
        if (shipDamage[shipNo] > 100) {
            if (shipDeathCount[shipNo] == -1) {
                shipDamage[shipNo] = 100;
                if (shipNo == ownShip) {
                    if (raceContext != 1) {
                        shipDeathCount[shipNo] = 100;
                        if (shipNo == ownShip) {
                            shipAttributes = *attrPtr;
                            CameraUpdate = UpdateCameraAttractDestroyed;
                            *attrPtr = shipAttributes & ~VIEW_IN;
                        }
                    }
                } else {
                    shipDeathCount[shipNo] = 15;
                }
            }
        }
        if (damageType != 0x80 && damageType != 0x81) {
            shieldGreen[shipNo] = 255;
            shieldRed[shipNo] = 255;
        }
        if (damageType == 0x81)
            shieldRed[shipNo] = 255;
        if (previousPlayerDamage < 60 && shipDamage[ownShip] >= 60 && shipNo == ownShip)
            PlayNote(SAMPLE_LOW_ENERGY, 510, 15, 15, 256);
        previousPlayerDamage = shipDamage[ownShip];
    }
}
#endif

/* Applies pit damage and adds particles while a ship crosses a pit face. */
#ifdef NETPLAY
void CheckPitShips(void)
{
    int i;
    int j;
    Face *facePtr;

    for (i = 0; i < 15; i++) {
        if (!(shipIndex[i].combatAttr & DESTROYED)) {
            facePtr = track->faces + shipIndex[i].nearTrkSect->faceStart;
            if (facePtr->flags & 0x40) {
#ifdef NETPLAY
                DamageCollide(0x80, i, -1);
#else
                DamageCollide(0x80, i);
#endif
                if (i == ownShip) {
                    SetPitAmbient(1);
                    SetNoteVolume(pitAmbientVoice, 192);
                }
                AddPitParticles(particleSlots, 10, &shipIndex[i].ppivot, 8);
            } else {
                facePtr++;
                if (facePtr->flags & 0x40) {
#ifdef NETPLAY
                    DamageCollide(0x80, i, -1);
#else
                    DamageCollide(0x80, i);
#endif
                    for (j = 0; j < 8; j++)
                        AddElectric(&shipIndex[i].ppivot, 2);
                    if (i == ownShip) {
                        SetPitAmbient(1);
                        SetNoteVolume(pitAmbientVoice, 192);
                    }
                    AddPitParticles(particleSlots, 10, &shipIndex[i].ppivot, 3);
                } else if (i == ownShip) {
                    SetPitAmbient(0);
                }
            }
        } else if (i == ownShip) {
            SetPitAmbient(0);
        }
    }
}
#else
void CheckPitShips(void)
{
    int i;
    int j;
    Face *facePtr;

    for (i = 0; i < 15; i++) {
        if (!(shipIndex[i].combatAttr & DESTROYED)) {
            facePtr = track->faces + shipIndex[i].nearTrkSect->faceStart;
            if (facePtr->flags & 0x40) {
                DamageCollide(0x80, i);
                if (i == ownShip) {
                    SetPitAmbient(1);
                    SetNoteVolume(pitAmbientVoice, 192);
                }
                AddPitParticles(particleSlots, 10, &shipIndex[i].ppivot, 8);
            } else if ((facePtr + 1)->flags & 0x40) {
                DamageCollide(0x80, i);
                for (j = 0; j < 8; j++)
                    AddElectric(&shipIndex[i].ppivot, 2);
                if (i == ownShip) {
                    SetPitAmbient(1);
                    SetNoteVolume(pitAmbientVoice, 192);
                }
                AddPitParticles(particleSlots, 10, &shipIndex[i].ppivot, 3);
            } else if (i == ownShip) {
                SetPitAmbient(0);
            }
        } else if (i == ownShip) {
            SetPitAmbient(0);
        }
    }
}
#endif

/* Adds electric streaks at the edge of a pit. */
/* The empty branches are in the original: both builds jump over them. */
void CheckPitShipsElectric(void)
{
    int i;
    int j;
    Face *facePtr;

    for (i = 0; i < 15; i++) {
        if (!(shipIndex[i].combatAttr & DESTROYED)) {
            facePtr = track->faces + shipIndex[i].nearTrkSect->faceStart;
            if (facePtr->flags & 0x40) {
            } else {
                facePtr++;
                if (facePtr->flags & 0x40) {
                    for (j = 0; j < 8; j++)
                        AddElectricStreak(&shipIndex[i].ppivot, 2);
                } else {
                }
            }
        } else {
        }
    }
}

void UpdateFadeScreen(void)
{
    SetTile(&fadeTile);
    SetSemiTrans(&fadeTile, 1);
    SetDrawMode(&fadedrm, 1, 1, GetTPage(0, 1, 0, 0) & 0xFFFF, 0);
    fadeTile.r0 = (fadeRed + fadeAmount < 255) ? fadeRed + fadeAmount : 255;
    fadeTile.g0 = (fadeGreen + fadeAmount + cheatfade < 255) ? fadeGreen + fadeAmount + cheatfade : 255;
    fadeTile.b0 = (fadeBlue + fadeAmount < 255) ? fadeBlue + fadeAmount : 255;
    fadeTile.x0 = 0;
    fadeTile.y0 = 0;
    fadeTile.w = 320;
    fadeTile.h = 240;
    if (fadeAmount > 0)
        fadeAmount -= 8;
    if (fadeAmount < 0)
        fadeAmount = 0;
    if (cheatfade > 0)
        cheatfade -= 10;
    if (cheatfade < 0)
        cheatfade = 0;
}

void NoOp429ca0(void)
{
}

void FlipShip(ShipData *playerShip)
{
    Face *facePtr;
    Face *nextFace;
    SVector nrm;

    facePtr = track->faces + playerShip->nearTrkSect->faceStart;
    while (!(facePtr->flags & TRACK_BASE)) facePtr++;
    facePtr++;
    nextFace = facePtr;
    nrm.vx = (nextFace->normal.vx + facePtr->normal.vx) >> 1;
    nrm.vy = (nextFace->normal.vy + facePtr->normal.vy) >> 1;
    nrm.vz = (nextFace->normal.vz + facePtr->normal.vz) >> 1;
    playerShip->vpivot.vx += nrm.vx * 2;
    playerShip->vpivot.vy += nrm.vy * 2;
    playerShip->vpivot.vz += nrm.vz * 2;
}

void GetCheatWeapon(ShipData *ships, int shipNo)
{
    short which;
    u_char w;
    ShipData *ship = &ships[shipNo];

    if (shipNo > 11) {
        shipIndex[shipNo].weapon = 0;
        return;
    }
    if (!tempGameData->weaponsOnSigned) {
        if (ship->weapon == 0) {
        } else {
        }
        return;
    }
    if (ship->weapon == 0) {
        while (1) {
            which = newRand() & 7;
            switch (which) {
            case 0:
                w = 2;
                break;
            case 1:
                w = 1;
                break;
            case 2:
                w = 5;
                break;
            case 3:
                w = 11;
                break;
            case 4:
                w = 12;
                break;
            case 5:
                w = 1;
                break;
            case 6:
                w = 10;
                break;
            default:
                w = 2;
                break;
            }
            if (w == 11 && plasmaShotBusy)
                w = ship->lastWeapon;
            if (ship->lastWeapon != w)
                break;
        }
        ship->weapon = w;
        ship->lastWeapon = w;
    }
}

int randWep(void)
{
    int num = 0;
    int seed;

    seed = (seedWep << 3) ^ (seedWep >> 1);
    seedWep = seed;
    num = seedWep & 0x7fff;
    return num;
}

#ifdef POWERVR
void Stub_44C480(void)
{
}

void Stub_44C490(void)
{
}

void Stub_44C4A0(void)
{
}
#endif
