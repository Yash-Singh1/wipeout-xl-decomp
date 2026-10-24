#include <string.h>
#include "functions.h"
#include "ship_state.h"
#include "ship_lookup.h"
#include "pc.h"
#include "wincd.h"
#include "mathinline.h"
#ifdef NETPLAY
#include "gst.h"
#include "lobby.h"
#pragma intrinsic(sin)
#endif

#define SHARK_FEET 14

typedef union ShipPath {
    char name[25];
    int  words[6];
} ShipPath;

extern TrackData *track;
extern char wobbleDir2[];
extern short ownShip;
extern short numTrackSections;
extern RaceLine raceLines[];
extern int gameType;
extern int proximity;
extern int shipDeathCount[];
#ifdef NETPLAY
extern signed char networkGame;
#endif
extern u_short spreadScale0[];
extern u_short spreadScale1[];
extern u_short spreadScale2[];
extern u_short spreadScale3[];

/* Ship positions rotated into view space, and their distances from the camera. */
VECTOR shipViewPos[15];
int    shipMag[15];

/* Shark animation state and the shark model's float vertices. */
extern short shipTex;
extern short TextureTableCount;

int         sharkVertCount;
int         sharkWiggle;
#ifdef NETPLAY
char         sharkToggle;
#else
u_char      sharkToggle;
#endif
FloatVertex sharkBaseVerts[64];
int         sharkFootIndex[SHARK_FEET];
FloatVertex sharkFootVerts[SHARK_FEET];

/* Draws visible ships and their effects. */
#ifdef NETPLAY
/* NET build: the frame follows the names below (MSVC /Od). The render colour pointer is stored but nothing is drawn with it here. */
void DrawShips(ShipData *shipIndex, Object **shipShapes, int camShip, TrackCamera camera, int flg)
{
    SVECTOR centre;
    int firstShip;
    short difference;
    int nextShip;
    Object *objPtr;
    Face *face;
    VECTOR shipOffset;
    int shipNo;
    long visFlag;
    short sectionDist;
    u_char *colour;
    int minMag;
    short trackDist;
    int nextMag;

    minMag = 3000;
    nextMag = 3000;
    firstShip = -1;
    nextShip = -1;
    proximity = 0;
    sectionDist = raceLines[camera.section->secNo].distance;
    for (shipNo = 0; shipNo < 15; shipNo++) {
        shipIndex[shipNo].combatAttr |= 0x80;
        shipOffset.vx = (shipIndex[shipNo].ppivot.vx + camera.camPos->relative.t[0]) >> 3;
        shipOffset.vy = (shipIndex[shipNo].ppivot.vy + camera.camPos->relative.t[1]) >> 3;
        shipOffset.vz = (shipIndex[shipNo].ppivot.vz + camera.camPos->relative.t[2]) >> 3;
        shipMag[shipNo] = GetMagnitude(shipOffset.vx, shipOffset.vy, shipOffset.vz);
        centre.vx = shipIndex[shipNo].ppivot.vx + camera.camPos->relative.t[0];
        centre.vy = shipIndex[shipNo].ppivot.vy + camera.camPos->relative.t[1];
        centre.vz = shipIndex[shipNo].ppivot.vz + camera.camPos->relative.t[2];
        SetSkeletonTransform(camera.camPos->super);
        RotTrans(&centre, &shipViewPos[shipNo], &visFlag);
        if (shipMag[shipNo] < 11600) {
            proximity |= 1 << shipNo;
            shipIndex[shipNo].attr |= VISIBLE;
            if (!(shipIndex[shipNo].combatAttr & DESTROYED) && shipNo != ownShip) {
                if (shipMag[shipNo] < minMag) {
                    minMag = shipMag[shipNo];
                    firstShip = shipNo;
                } else if (shipMag[shipNo] < nextMag) {
                    nextMag = shipMag[shipNo];
                    nextShip = shipNo;
                }
            }
        }
        if ((shipMag[shipNo] < 4800 && shipViewPos[shipNo].vz > 0) || camShip == shipNo) {
            objPtr = shipShapes[shipIndex[shipNo].highResTableIndex];
            SetSkeletonPosition(objPtr->skeleton, shipIndex[shipNo].ppivot.vx, shipIndex[shipNo].ppivot.vy, shipIndex[shipNo].ppivot.vz);
            SetSkeletonDirectionHPR(objPtr->skeleton, shipIndex[shipNo].hdg, shipIndex[shipNo].pitch, shipIndex[shipNo].roll);
            if (!(shipIndex[shipNo].combatAttr & DESTROYED) || shipDeathCount[shipNo] > 10) {
                if (!(shipIndex[shipNo].attr & VIEW_IN) || (shipIndex[shipNo].attr & REMOTE_VIEW)) {
                    trackDist = raceLines[shipIndex[shipNo].nearTrkSect->secNo].distance;
                    difference = sectionDist - trackDist;
                    if (difference < 0) {
                        difference += numTrackSections;
                    }
                    if (numTrackSections / 2 < difference) {
                        difference -= numTrackSections;
                    }
                    if (difference > -80 && difference < 80) {
                        shipIndex[shipNo].combatAttr &= ~0x80;
                        face = track->faces + shipIndex[shipNo].nearTrkSect->faceStart;
                        while (!(face->flags & TRACK_BASE)) {
                            face++;
                        }
                        if (!(shipIndex[shipNo].attr & LSIDE)) {
                            face++;
                        }
                        colour = (u_char *)&face->red;
                    }
                }
            }
        } else if (!(shipMag[shipNo] < 11600)) {
            shipIndex[shipNo].attr &= ~VISIBLE;
        }
    }
    if (flg == -1) {
        AdjustRemoteEngineSound(firstShip, nextShip, minMag, nextMag);
    }
}
#else
void DrawShips(ShipData *shipIndex, Object **shipShapes, int camShip, TrackCamera camera, int flg)
{
    int         i;
    VECTOR      distance;
    SVECTOR     toCentre;
    long        flag;
    Object     *obj;
    int         firstMag, secondMag, firstShip, secondShip;
    int         mag;
    short       baseIndex;
    u_short     attr2;
    short       diff;
    Face       *face;

    firstMag = 3000;
    secondMag = 3000;
    firstShip = -1;
    secondShip = -1;
    proximity = 0;
    baseIndex = raceLines[camera.section->secNo].distance;
    for (i = 0; i < 15; i++) {
        shipIndex[i].combatAttr |= 0x80;
        distance.vx = (shipIndex[i].ppivot.vx + camera.camPos->relative.t[0]) >> 3;
        distance.vy = (shipIndex[i].ppivot.vy + camera.camPos->relative.t[1]) >> 3;
        distance.vz = (shipIndex[i].ppivot.vz + camera.camPos->relative.t[2]) >> 3;
        shipMag[i] = GetMagnitude(distance.vx, distance.vy, distance.vz);
        toCentre.vx = shipIndex[i].ppivot.vx + camera.camPos->relative.t[0];
        toCentre.vy = shipIndex[i].ppivot.vy + camera.camPos->relative.t[1];
        toCentre.vz = shipIndex[i].ppivot.vz + camera.camPos->relative.t[2];
        SetSkeletonTransform(camera.camPos->super);
        RotTrans(&toCentre, &shipViewPos[i], &flag);
        mag = shipMag[i];
        if (mag < 11600) {
            proximity |= 1 << i;
            shipIndex[i].attr |= VISIBLE;
            if (!(shipIndex[i].combatAttr & DESTROYED) && i != ownShip) {
                if (mag < firstMag) {
                    firstMag = mag;
                    firstShip = i;
                } else if (mag < secondMag) {
                    secondMag = mag;
                    secondShip = i;
                }
            }
        }
        if ((mag < 4800 && shipViewPos[i].vz > 0) || camShip == i) {
            obj = shipShapes[shipIndex[i].highResTableIndex];
            SetSkeletonPosition(obj->skeleton, shipIndex[i].ppivot.vx, shipIndex[i].ppivot.vy, shipIndex[i].ppivot.vz);
            SetSkeletonDirectionHPR(obj->skeleton, shipIndex[i].hdg, shipIndex[i].pitch, shipIndex[i].roll);
            attr2 = shipIndex[i].combatAttr;
            if (!(attr2 & DESTROYED) || shipDeathCount[i] > 10) {
                if (!(shipIndex[i].attr & VIEW_IN) || (shipIndex[i].attr & REMOTE_VIEW)) {
                    diff = baseIndex - raceLines[shipIndex[i].nearTrkSect->secNo].distance;
                    if (diff < 0) {
                        diff += numTrackSections;
                    }
                    if (numTrackSections / 2 < diff) {
                        diff -= numTrackSections;
                    }
                    if (diff > -80 && diff < 80) {
                        shipIndex[i].combatAttr = attr2 & ~0x80;
                        face = track->faces + shipIndex[i].nearTrkSect->faceStart;
                        while (!(face->flags & TRACK_BASE)) {
                            face++;
                        }
                    }
                }
            }
        } else if (!(mag < 11600)) {
            shipIndex[i].attr &= ~VISIBLE;
        }
    }
    if (flg == -1) {
        AdjustRemoteEngineSound(firstShip, secondShip, firstMag, secondMag);
    }
}
#endif



#ifdef EQUIV
/* Adds visible ships to the render list. */
void AddShipRenderObjects(ShipData *shipIndex, Object **shipShapes, int camShip, TrackCamera camera)
{
#ifdef NETPLAY
    /* The NET build keeps four search variables that are set but never read,
       and an empty else-if branch; /Od keeps both, so the source had them. */
    int      i;
    short    baseIndex;
    short    diff;
    short    shipDistance;
    Object  *obj;
    Face    *face;
    u_char  *colour;
    int      nearestDist = 3000;
    int      farthestDist = 3000;
    int      nearestShip = -1;
    int      farthestShip = -1;

    baseIndex = raceLines[camera.section->secNo].distance;
    for (i = 0; i < 15; i++) {
        if ((shipMag[i] < 1200 && shipViewPos[i].vz > 0) || i == camShip) {
            obj = shipShapes[shipIndex[i].highResTableIndex];
            if (!(shipIndex[i].combatAttr & DESTROYED) || shipDeathCount[i] > 10) {
                if (!(shipIndex[i].attr & VIEW_IN) || (shipIndex[i].attr & REMOTE_VIEW)) {
                    shipDistance = raceLines[shipIndex[i].nearTrkSect->secNo].distance;
                    diff = baseIndex - shipDistance;
                    if (diff < 0) {
                        diff += numTrackSections;
                    }
                    if (numTrackSections / 2 < diff) {
                        diff -= numTrackSections;
                    }
                    if (diff > -40 && diff < 40) {
                        face = track->faces + shipIndex[i].nearTrkSect->faceStart;
                        while (!(face->flags & TRACK_BASE)) {
                            face++;
                        }
                        if (!(shipIndex[i].attr & LSIDE)) {
                            face++;
                        }
                        colour = (u_char *)&face->red;
                        if (gameType != 1 || ownShip == i) {
                            AddRenderObject(obj, 0, colour);
                        }
                    }
                }
            }
        } else if (shipMag[i] < 2900) {
        }
    }
#else
    int      i;
    short    baseIndex;
    short    diff;
    Object  *obj;
    Face    *face;

    baseIndex = raceLines[camera.section->secNo].distance;
    for (i = 0; i < 15; i++) {
        if ((shipMag[i] < 1200 && shipViewPos[i].vz > 0) || i == camShip) {
            obj = shipShapes[shipIndex[i].highResTableIndex];
            if (!(shipIndex[i].combatAttr & DESTROYED) || shipDeathCount[i] > 10) {
                if (!(shipIndex[i].attr & VIEW_IN) || (shipIndex[i].attr & REMOTE_VIEW)) {
                    diff = baseIndex - raceLines[shipIndex[i].nearTrkSect->secNo].distance;
                    if (diff < 0) {
                        diff += numTrackSections;
                    }
                    if (numTrackSections / 2 < diff) {
                        diff -= numTrackSections;
                    }
                    if (diff > -40 && diff < 40) {
                        face = track->faces + shipIndex[i].nearTrkSect->faceStart;
                        while (!(face->flags & TRACK_BASE)) {
                            face++;
                        }
                        if (!(shipIndex[i].attr & LSIDE)) {
                            face++;
                        }
                        if (gameType != 1 || ownShip == i) {
                            AddRenderObject(obj, 0, (u_char *)&face->red);
                        }
                    }
                }
            }
        }
    }
#endif
}

#endif

/* Copies the shark model vertices into its animated pose. */
void CopySharkVerts(Object *ship)
{
#ifdef NETPLAY
    int   i;
    int   j;
    int   k;
    float lowestY;

    for (i = 0; i < 5; i++) {
        Object *shape = shipShapeTable[i];
        for (j = 0; j < shape->vertexCount; j++) {
            shape->floatVerts[j].y -= 48 / 4096.0f;
        }
    }
    wsprintfA(g_cdMessage, "shark has %d verts\n", ship->vertexCount);
    sharkVertCount = ship->vertexCount;
    for (i = 0; i < sharkVertCount; i++) {
        sharkBaseVerts[i] = ship->floatVerts[i];
    }
    {
        Object *legs = shipShapeTable[4];
        for (k = 0; k < SHARK_FEET; k++) {
            lowestY = 0;
            for (j = 0; j < legs->vertexCount; j++) {
                if (legs->floatVerts[j].y < lowestY) {
                    lowestY = legs->floatVerts[j].y;
                    sharkFootIndex[k] = j;
                    sharkFootVerts[k] = legs->floatVerts[j];
                }
            }
            legs->floatVerts[sharkFootIndex[k]].y = 0;
        }
    }
#else
    int   i;
    int   j;
    int   k;
    float lowestY;

    for (i = 0; i < 5; i++) {
        for (j = 0; j < shipShapeTable[i]->vertexCount; j++) {
            shipShapeTable[i]->floatVerts[j].y -= 48 / 4096.0f;
        }
    }
    wsprintfA(g_cdMessage, "shark has %d verts\n", ship->vertexCount);
    sharkVertCount = ship->vertexCount;
    for (i = 0; i < sharkVertCount; i++) {
        sharkBaseVerts[i] = ship->floatVerts[i];
    }
    for (k = 0; k < SHARK_FEET; k++) {
        lowestY = 0;
        for (j = 0; j < shipShapeTable[4]->vertexCount; j++) {
            if (shipShapeTable[4]->floatVerts[j].y < lowestY) {
                lowestY = shipShapeTable[4]->floatVerts[j].y;
                sharkFootIndex[k] = j;
                sharkFootVerts[k] = shipShapeTable[4]->floatVerts[j];
            }
        }
        shipShapeTable[4]->floatVerts[sharkFootIndex[k]].y = 0;
    }
#endif
}




/* Updates the shark model animation. */
#ifdef NETPLAY
#define SHARK_TRACE_WIGGLE 1
#endif
void UpdateSharkModel(void)
{
#ifdef NETPLAY
    int     i;
    Object *model;

    model = shipShapeTable[2];
    for (i = 0; i < sharkVertCount; i++) {
        if (SHARK_TRACE_WIGGLE) {
        } else {
            /* The NET build's wiggle trace is compiled out; /Od keeps the empty branches. */
        }
        model->floatVerts[i].x = sharkBaseVerts[i].x + (float)(rsin((int)(sharkBaseVerts[i].z * 4096.0f) * 4 + sharkWiggle) / 64) / 4096.0f;
    }
    sharkWiggle = (sharkWiggle + 0x80) & 0xfff;
#else
    int     count;
    int     wiggle;
    int     i;
    Object *body;

    body = shipShapeTable[2];
    count = sharkVertCount;
    wiggle = sharkWiggle;
    i = 0;
    if (count > 0) {
        do {
            body->floatVerts[i].x = sharkBaseVerts[i].x + (float)(rsin(wiggle + (int)(sharkBaseVerts[i].z * 4096.0f) * 4) / 64) * (1 / 4096.0f);
            i++;
        } while (--count);
    }
    sharkWiggle = (wiggle + 0x80) & 0xfff;
#endif
}


/* Updates the shark foot animation. */
void UpdateSharkFeet(void)
{
    Object  *legs;

    legs = shipShapeTable[4];
    if (sharkToggle == 0) {
        int j;

        sharkToggle = 1;
        for (j = 0; j < SHARK_FEET; j++) {
            legs->floatVerts[sharkFootIndex[j]].y = sharkFootVerts[j].y;
        }
    } else {
        int j;

        sharkToggle = 0;
        for (j = 0; j < SHARK_FEET; j++) {
            legs->floatVerts[sharkFootIndex[j]].y = sharkFootVerts[j].y + 64 / 4096.0f;
        }
    }
}


/* Equivalent to the exe; the loops' register allocation differs (no byte-exact match). */
#ifdef EQUIV
#ifdef NETPLAY
#pragma intrinsic(strcpy, strcat)
/* Loads ship models and initializes their track positions. */
void loadShipData(TrackSection *section, ShipData *shipIndex, Object **shipShapes, Skeleton *camPos)
{
    TIMlist *sequence;
    Object   *obj;
    char      modelFile[256];
    short     firstShipPage;
    short     droidPageStart;
    short     modelIndex = 0;

    if (sillyShips) {
        OutputDebugStringA("***** LOADING SILLY SHIPS *******\n");
        highResTable[0] = 0;
        highResTable[1] = 5;
        highResTable[2] = 10;
        highResTable[3] = 1;
        highResTable[4] = 6;
        highResTable[5] = 11;
        highResTable[6] = 3;
        highResTable[7] = 8;
        highResTable[8] = 13;
        highResTable[9] = 4;
        highResTable[10] = 9;
        highResTable[11] = 14;
        highResTable[12] = 2;
        highResTable[13] = 7;
        highResTable[14] = 12;
    }
    strcpy(modelFile, wobbleDir2);
    if (!sillyShips) {
        strcat(modelFile, "terry");
    } else {
        strcat(modelFile, "wierd");
    }
    strcat(modelFile, ".cmp");
    firstShipPage = TextureTableCount;
    shipTex = TextureTableCount;
    sequence = LoadCompressedTextureSequence(modelFile);
    GhostLoadCmpFiles(sequence);
    droidPageStart = TextureTableCount;
    strcpy(modelFile, wobbleDir2);
    strcat(modelFile, "rescu.cmp");
    sequence = LoadCompressedTextureSequence(modelFile);
    LoadCmpFiles(sequence);
    strcpy(modelFile, wobbleDir2);
    if (!sillyShips) {
        strcat(modelFile, "terry");
    } else {
        strcat(modelFile, "wierd");
    }
    strcat(modelFile, ".prm");
    FreePrm(shipShapes[0]);
    obj = (shipShapes[0] = LoadPrm(modelFile, firstShipPage));
    while (obj) {
        obj->skeleton->super = camPos;
        shipShapes[modelIndex] = obj;
        modelIndex = modelIndex + 1;
        obj = obj->next;
    }

    for (modelIndex = 0; modelIndex < 15; modelIndex++) {
        shipIndex[modelIndex].highResTableIndex = highResTable[modelIndex];
    }

    shipShapes[5] = NewInstance(shipShapes[0]);
    shipShapes[6] = NewInstance(shipShapes[1]);
    shipShapes[7] = NewInstance(shipShapes[2]);
    shipShapes[8] = NewInstance(shipShapes[3]);
    shipShapes[9] = NewInstance(shipShapes[4]);
    shipShapes[10] = NewInstance(shipShapes[0]);
    shipShapes[11] = NewInstance(shipShapes[1]);
    shipShapes[12] = NewInstance(shipShapes[2]);
    shipShapes[13] = NewInstance(shipShapes[3]);
    shipShapes[14] = NewInstance(shipShapes[4]);
    strcpy(modelFile, wobbleDir2);
    strcat(modelFile, "telco.prm");
    FreePrm(shipShapes[15]);
    obj = (shipShapes[15] = LoadPrm(modelFile, shipTex));
    modelIndex = 15;
    while (obj) {
        shipIndex[modelIndex - 15].lowResTableIndex = modelIndex;
        obj->skeleton->super = camPos;
        shipShapes[modelIndex] = obj;
        modelIndex = modelIndex + 1;
        obj = obj->next;
    }

    FreePrm(shipShapes[30]);
    strcpy(modelFile, wobbleDir2);
    strcat(modelFile, "rescu.prm");
    obj = (shipShapes[30] = LoadPrm(modelFile, droidPageStart));
    while (obj) {
        obj->skeleton->super = camPos;
        obj = obj->next;
    }

    if (sillyShips) {
        obj = shipShapes[2];
        CopySharkVerts(obj);
    }
}
#else
/* Loads ship models and initializes their track positions. */
void loadShipData(TrackSection *section, ShipData *shipIndex, Object **shipShapes, Skeleton *camPos)
{
    Object   *obj;
    char      shipFile[256];
    short     shipTextures;
    short     droidTextures;
    short     count = 0;
    short     i;
    Object   *telco;

    if (sillyShips) {
        OutputDebugStringA("***** LOADING SILLY SHIPS *******\n");
        highResTable[1] = 5;
        highResTable[2] = 10;
        highResTable[3] = 1;
        highResTable[4] = 6;
        highResTable[5] = 11;
        highResTable[6] = 3;
        highResTable[7] = 8;
        highResTable[8] = 13;
        highResTable[9] = 4;
        highResTable[10] = 9;
        highResTable[11] = 14;
        highResTable[0] = 0;
        highResTable[12] = 2;
        highResTable[13] = 7;
        highResTable[14] = 12;
    }
    strcpy(shipFile, wobbleDir2);
    if (!sillyShips) {
        strcat(shipFile, "terry");
    } else {
        strcat(shipFile, "wierd");
    }
    strcat(shipFile, ".cmp");
    shipTextures = TextureTableCount;
    shipTex = shipTextures;
    GhostLoadCmpFiles(LoadCompressedTextureSequence(shipFile));
    droidTextures = TextureTableCount;
    strcpy(shipFile, wobbleDir2);
    strcat(shipFile, "rescu.cmp");
    LoadCmpFiles(LoadCompressedTextureSequence(shipFile));
    strcpy(shipFile, wobbleDir2);
    if (!sillyShips) {
        strcat(shipFile, "terry");
    } else {
        strcat(shipFile, "wierd");
    }
    strcat(shipFile, ".prm");
    FreePrm(shipShapes[0]);
    obj = (shipShapes[0] = LoadPrm(shipFile, shipTextures));
    while (obj) {
        obj->skeleton->super = camPos;
        shipShapes[count] = obj;
        count++;
        obj = obj->next;
    }

    for (i = 0; i < 15; i++) {
        shipIndex[i].highResTableIndex = highResTable[i];
    }

    shipShapes[5] = NewInstance(shipShapes[0]);
    shipShapes[6] = NewInstance(shipShapes[1]);
    shipShapes[7] = NewInstance(shipShapes[2]);
    shipShapes[8] = NewInstance(shipShapes[3]);
    shipShapes[9] = NewInstance(shipShapes[4]);
    shipShapes[10] = NewInstance(shipShapes[0]);
    shipShapes[11] = NewInstance(shipShapes[1]);
    shipShapes[12] = NewInstance(shipShapes[2]);
    shipShapes[13] = NewInstance(shipShapes[3]);
    shipShapes[14] = NewInstance(shipShapes[4]);
    strcpy(shipFile, wobbleDir2);
    strcat(shipFile, "telco.prm");
    FreePrm(shipShapes[15]);
    telco = (shipShapes[15] = LoadPrm(shipFile, shipTex));
    i = 15;
    while (telco) {
        shipShapes[i] = telco;
        shipIndex[i - 15].lowResTableIndex = i;
        i++;
        telco->skeleton->super = camPos;
        telco = telco->next;
    }

    FreePrm(shipShapes[30]);
    strcpy(shipFile, wobbleDir2);
    strcat(shipFile, "rescu.prm");
    obj = (shipShapes[30] = LoadPrm(shipFile, droidTextures));
    while (obj) {
        obj->skeleton->super = camPos;
        obj = obj->next;
    }

    if (sillyShips) {
        CopySharkVerts(shipShapes[2]);
    }
}
#endif

#endif

/* Ship field initialization and local stack layout still differ. */
#ifdef EQUIV
extern short gridPosition[];
extern short startLinePos[];
extern int shakeAmount;
extern short bigGrid;
extern short activeShipCount;
extern short finishPosition;
extern short flyingFlags[];
extern ShipRank shipRank[];
extern u_short shipMassClass0[], shipMassClass1[], shipMassClass2[], shipMassClass3[];
extern u_short shipMaxThrustClass0[], shipMaxThrustClass1[], shipMaxThrustClass2[], shipMaxThrustClass3[];
extern u_short shipSkidClass0[], shipSkidClass1[], shipSkidClass2[], shipSkidClass3[];
extern u_short shipResistanceClass0[], shipResistanceClass1[], shipResistanceClass2[], shipResistanceClass3[];
extern u_short shipHeadingIncClass0[], shipHeadingIncClass1[], shipHeadingIncClass2[], shipHeadingIncClass3[];
extern u_short shipMaxHeadingClass0[], shipMaxHeadingClass1[], shipMaxHeadingClass2[], shipMaxHeadingClass3[];
extern u_short shipRemoteMaxClass0[], shipRemoteMaxClass1[], shipRemoteMaxClass2[], shipRemoteMaxClass3[];
extern u_short shipRemoteMagClass0[], shipRemoteMagClass1[], shipRemoteMagClass2[], shipRemoteMagClass3[];
#include "ship_start.h"

#ifndef NETPLAY
/* Initializes ship state for a new race. */
void initShipData(TrackSection *section, ShipData *shipIndex, ConfigData *options)
{
    short         gridSides[16];
    TrackSection *startSections[15];
    u_short       droneSpeedGroups[16] = { 0x10, 0x20, 0x40, 0x10, 0x20, 0x40, 0x10, 0x20,
                                 0x40, 0x10, 0x20, 0x40, 0x10, 0x20, 0x40, 0x10 };
    short         i;
    short         startPosition;
    short         nextGridSide;
    short         startSectionNo;
    ShipData     *ship;
    short         j;
    Face         *face;
    TrackSection *ahead;
    int           dx;
    int           dz;
    VECTOR       *v2;

    shakeAmount = 0;
    bigGrid = options->shipClass == 3;
    if (ownShip >= 12) {
        bigGrid = 1;
    }
    startSectionNo = startLinePos[options->actTrackNum];
    for (;;) {
        if (section->secNo == startSectionNo) {
            break;
        }
        section++;
    }
    section = section->prevSection.ptr;
    nextGridSide = 0;
    for (i = 0; i < 15; i++) {
        startSections[i] = section;
        section = section->prevSection.ptr;
        section = section->prevSection.ptr;
        gridSides[i] = nextGridSide;
        nextGridSide ^= 1;
    }
    if (gameType == 1) {
        gridPosition[ownShip] = 0;
    } else {
        short order[16];
        short groups;
        short next;

        groups = 4;
        if (bigGrid) {
            groups = 5;
        }
        for (i = 0; i < 3; i++) {
            order[i * groups] = 2 - i;
            order[i * groups + 1] = 5 - i;
            order[i * groups + 2] = 8 - i;
            order[i * groups + 3] = 11 - i;
            order[i * groups + 4] = 14 - i;
        }
        next = 0;
        for (j = 0; j < groups * 3; j++) {
            if (ownShip != order[j]) {
                gridPosition[order[j]] = next;
                next++;
            }
        }
        gridPosition[ownShip] = next;
        next++;
        if (!bigGrid) {
            gridPosition[12] = next;
            gridPosition[13] = next + 1;
            gridPosition[14] = next + 2;
        }
    }
    for (i = 0; i < 15; i++) {
        flyingFlags[i] = 0;
        startPosition = gridPosition[i];
        shipIndex[i].attr = VISIBLE | RACING | DIRECTION;
        shipIndex[i].vpivot.vx = 0;
        shipIndex[i].vpivot.vy = 0;
        shipIndex[i].vpivot.vz = 0;
        shipIndex[i].apivot.vx = 0;
        shipIndex[i].apivot.vy = 0;
        shipIndex[i].apivot.vz = 0;
        shipIndex[i].hdg = 0;
        shipIndex[i].pitch = 0;
        shipIndex[i].roll = 0;
        shipIndex[i].vhdg = 0;
        shipIndex[i].vpitch = 0;
        shipIndex[i].vroll = 0;
        shipIndex[i].ahdg = 0;
        shipIndex[i].r_brake_rot = 0;
        shipIndex[i].l_brake_rot = 0;
        shipIndex[i].speed = 0;
        shipIndex[i].reserved96 = 0;
        shipIndex[i].thrust = 0;
        shipIndex[i].thrustCount = 0;
        shipIndex[i].noAttitudeControl = 0;
        shipIndex[i].weapon = 0;
        shipIndex[i].lastWeapon = 0;
        shipIndex[i].targetShip = -1;
        shipIndex[i].rearTarget = -1;
        shipIndex[i].electroCount = 0;
        shipIndex[i].autopilotCount = 0;
        shipIndex[i].stallCount = 0;
        shipIndex[i].lap = 0;
        shipIndex[i].reverseLap = 0;
        for (j = 0; j < 5; j++) {
            shipIndex[i].lapTimes[j] = 0;
        }
        shipIndex[i].updateCount = 200;
        if (ownShip == i) {
            shipIndex[i].combatAttr = 0;
        } else if (gameType == 1) {
            shipIndex[i].combatAttr |= DESTROYED;
        } else {
            shipIndex[i].combatAttr = droneSpeedGroups[i];
            if (!bigGrid && i >= 12) {
                shipIndex[i].combatAttr |= DESTROYED;
            }
        }
        switch (options->shipClass) {
        case 0: shipIndex[i].mass = shipMassClass0[i]; shipIndex[i].maxThrust = shipMaxThrustClass0[i]; shipIndex[i].skid = shipSkidClass0[i]; shipIndex[i].resistance = shipResistanceClass0[i]; break;
        case 1: shipIndex[i].mass = shipMassClass1[i]; shipIndex[i].maxThrust = shipMaxThrustClass1[i]; shipIndex[i].skid = shipSkidClass1[i]; shipIndex[i].resistance = shipResistanceClass1[i]; break;
        case 2: shipIndex[i].mass = shipMassClass2[i]; shipIndex[i].maxThrust = shipMaxThrustClass2[i]; shipIndex[i].skid = shipSkidClass2[i]; shipIndex[i].resistance = shipResistanceClass2[i]; break;
        case 3: shipIndex[i].mass = shipMassClass3[i]; shipIndex[i].maxThrust = shipMaxThrustClass3[i]; shipIndex[i].skid = shipSkidClass3[i]; shipIndex[i].resistance = shipResistanceClass3[i]; break;
        }
        switch (options->shipClass) {
        case 0: shipIndex[i].headingInc = shipHeadingIncClass0[i]; shipIndex[i].maxHeading = shipMaxHeadingClass0[i]; break;
        case 1: shipIndex[i].headingInc = shipHeadingIncClass1[i]; shipIndex[i].maxHeading = shipMaxHeadingClass1[i]; break;
        case 2: shipIndex[i].headingInc = shipHeadingIncClass2[i]; shipIndex[i].maxHeading = shipMaxHeadingClass2[i]; break;
        case 3: shipIndex[i].headingInc = shipHeadingIncClass3[i]; shipIndex[i].maxHeading = shipMaxHeadingClass3[i]; break;
        }
        shipIndex[i].update = UpdatePlayerShipStart;
        if (ownShip != i) {
            shipIndex[i].update = UpdateDroneShipStart;
        }
        switch (options->shipClass) {
        case 0: shipIndex[i].remoteMax = shipRemoteMaxClass0[i]; shipIndex[i].remoteMagnitude = shipRemoteMagClass0[i]; break;
        case 1: shipIndex[i].remoteMax = shipRemoteMaxClass1[i]; shipIndex[i].remoteMagnitude = shipRemoteMagClass1[i]; break;
        case 2: shipIndex[i].remoteMax = shipRemoteMaxClass2[i]; shipIndex[i].remoteMagnitude = shipRemoteMagClass2[i]; break;
        case 3: shipIndex[i].remoteMax = shipRemoteMaxClass3[i]; shipIndex[i].remoteMagnitude = shipRemoteMagClass3[i]; break;
        }
        if (startPosition == 0 || startPosition == 1) {
            shipIndex[i].remoteMax += options->shipClass * 30;
        }
        SetSpread(shipIndex, options, startPosition, i, bigGrid);
        shipIndex[i].nearTrkSect = startSections[startPosition];
        shipIndex[i].prevShipSect = startSections[startPosition];
        face = track->faces + startSections[startPosition]->faceStart;
        while (!(face->flags & TRACK_BASE)) {
            face++;
        }
        if (gridSides[startPosition]) {
            face++;
        }
        ship = &shipIndex[i];
        v2 = &track->vertices[face->vertex[2]];
        ship->ppivot.vx = ((face->normal.vx * 300) >> 12) + ((v2->vx + track->vertices[face->vertex[0]].vx) >> 1);
        ship->ppivot.vy = ((face->normal.vy * 300) >> 12) + ((v2->vy + track->vertices[face->vertex[0]].vy) >> 1);
        ship->ppivot.vz = ((face->normal.vz * 300) >> 12) + ((v2->vz + track->vertices[face->vertex[0]].vz) >> 1);
        ship->nearSecNo = startSections[startPosition]->secNo;
        ship->prevSecNo = startSections[startPosition]->secNo;
        ship->raceProgress.distance = raceLines[ship->nearSecNo].distance;
        ship->raceProgress.trackDistance = ship->raceProgress.distance;
        ahead = startSections[startPosition]->nextSection.ptr;
        dx = ahead->centre.vx - startSections[startPosition]->centre.vx;
        dz = ahead->centre.vz - startSections[startPosition]->centre.vz;
        ship->hdg = -ratan2(dx, dz);
    }
    j = 0;
    for (i = 0; i < 15; i++) {
        if (!(shipIndex[i].combatAttr & DESTROYED)) {
            shipRank[j].position = raceLines[shipIndex[i].nearSecNo].distance;
            shipRank[j].shipNo = i;
            j++;
        }
        activeShipCount = j;
    }
    finishPosition = gridPosition[ownShip] + 1;
}
#endif

/* Network version: the grid orderTable comes from the network ship list (gstShipTaken) and from the
   tournament orderTable, and the start grid is kept in gridPosition[] (the NET build has no separate
   per-ship grid table). */
#ifdef EQUIV
#ifdef NETPLAY
void initShipData(TrackSection *section, ShipData *shipIndex, ConfigData *options)
{
    TrackSection *forward;
    short         nextSlot;
    short         gridRank = 0;
    short         gridSidePtr[16];
    short         sectionNo;
    short         groupCount;
    short         nextSide;
    short         i;
    TrackSection *gridSections[15];
    short         j;
    u_short       droneSpeedGroups[16] = { 0x10, 0x20, 0x40, 0x10, 0x20, 0x40, 0x10, 0x20,
                                 0x40, 0x10, 0x20, 0x40, 0x10, 0x20, 0x40, 0x10 };
    VECTOR        base;
    Face         *gridFace;
    VECTOR        dir;
    short         orderTable[16];

    shakeAmount = 0;
    bigGrid = options->shipClass == 3;
    if (ownShip >= 12) {
        bigGrid = 1;
    }
    for (;;) {
        if (startLinePos[options->actTrackNum] == section->secNo) {
            break;
        }
        section++;
    }
    section = section->prevSection.ptr;
    nextSide = 0;
    for (i = 0; i < 15; i++) {
        gridSections[i] = section;
        section = section->prevSection.ptr;
        section = section->prevSection.ptr;
        gridSidePtr[i] = nextSide;
        nextSide ^= 1;
    }
    if (gameType == 1) {
        gridPosition[ownShip] = 0;
    } else {
        groupCount = 4;
        if (bigGrid) {
            groupCount++;
        }
        for (i = 0; i < 3; i++) {
            orderTable[i * groupCount] = 2 - i;
            orderTable[i * groupCount + 1] = 5 - i;
            orderTable[i * groupCount + 2] = 8 - i;
            orderTable[i * groupCount + 3] = 11 - i;
            orderTable[i * groupCount + 4] = 14 - i;
        }
        nextSlot = 0;
        for (i = 0; i < groupCount * 3; i++) {
            if (orderTable[i] != ownShip) {
                gridPosition[orderTable[i]] = nextSlot;
                nextSlot++;
            }
        }
        gridPosition[ownShip] = nextSlot;
        nextSlot++;
        if (!bigGrid) {
            gridPosition[12] = nextSlot;
            gridPosition[13] = nextSlot + 1;
            gridPosition[14] = nextSlot + 2;
        }
    }
    if (networkGame) {
        nextSlot = 0;
        for (i = 0; i < 15; i++) {
            if (gstShipTaken[i] == 1) {
                gridPosition[i] = nextSlot;
                nextSlot++;
            }
        }
        for (i = 0; i < 15; i++) {
            if (!gstShipTaken[i]) {
                gridPosition[i] = nextSlot;
                nextSlot++;
            }
        }
    }
    if (networkGame && gstRaceMode == 3 && tournamentTrack > 0) {
        gstDebugPrintf("InitShipData: doing Tournament Ship Ordering...");
        for (i = 0; i < 15; i++) {
            gridPosition[i] = gstGlobalShipOrder[i];
        }
    }
    if (networkGame && gstDebugOutputEnabled) {
        for (i = 0; i < 15; i++) {
            printf("shipOrder[ %2d ]     == %2d\n", i, gridPosition[i]);
        }
        for (i = 0; i < 15; i++) {
            printf("ReservedShips[ %2d ] == %2d\n", i, gstShipTaken[i]);
        }
    }
    for (i = 0; i < 15; i++) {
        gridRank = gridPosition[i];
        shipIndex[i].attr = VISIBLE | RACING | DIRECTION;
        flyingFlags[i] = 0;
        shipIndex[i].vpivot.vx = 0;
        shipIndex[i].vpivot.vy = 0;
        shipIndex[i].vpivot.vz = 0;
        shipIndex[i].apivot.vx = 0;
        shipIndex[i].apivot.vy = 0;
        shipIndex[i].apivot.vz = 0;
        shipIndex[i].hdg = 0;
        shipIndex[i].pitch = 0;
        shipIndex[i].roll = 0;
        shipIndex[i].vhdg = 0;
        shipIndex[i].vpitch = 0;
        shipIndex[i].vroll = 0;
        shipIndex[i].ahdg = 0;
        shipIndex[i].r_brake_rot = 0;
        shipIndex[i].l_brake_rot = 0;
        shipIndex[i].speed = 0;
        shipIndex[i].reserved96 = 0;
        shipIndex[i].thrust = 0;
        shipIndex[i].thrustCount = 0;
        shipIndex[i].noAttitudeControl = 0;
        shipIndex[i].weapon = 0;
        shipIndex[i].lastWeapon = 0;
        shipIndex[i].targetShip = -1;
        shipIndex[i].rearTarget = -1;
        shipIndex[i].electroCount = 0;
        shipIndex[i].autopilotCount = 0;
        shipIndex[i].stallCount = 0;
        shipIndex[i].lap = 0;
        shipIndex[i].reverseLap = 0;
        for (j = 0; j < 5; j++) {
            shipIndex[i].lapTimes[j] = 0;
        }
        shipIndex[i].updateCount = 200;
        if (i == ownShip && !networkGame) {
            shipIndex[i].combatAttr = 0;
        } else if (gameType == 1) {
            shipIndex[i].combatAttr |= DESTROYED;
        } else {
            shipIndex[i].combatAttr = droneSpeedGroups[i];
            if (!bigGrid && !networkGame && i >= 12) {
                shipIndex[i].combatAttr |= DESTROYED;
            }
        }
        switch (options->shipClass) {
        case 0: shipIndex[i].mass = shipMassClass0[i]; shipIndex[i].maxThrust = shipMaxThrustClass0[i]; shipIndex[i].skid = shipSkidClass0[i]; shipIndex[i].resistance = shipResistanceClass0[i]; break;
        case 1: shipIndex[i].mass = shipMassClass1[i]; shipIndex[i].maxThrust = shipMaxThrustClass1[i]; shipIndex[i].skid = shipSkidClass1[i]; shipIndex[i].resistance = shipResistanceClass1[i]; break;
        case 2: shipIndex[i].mass = shipMassClass2[i]; shipIndex[i].maxThrust = shipMaxThrustClass2[i]; shipIndex[i].skid = shipSkidClass2[i]; shipIndex[i].resistance = shipResistanceClass2[i]; break;
        case 3: shipIndex[i].mass = shipMassClass3[i]; shipIndex[i].maxThrust = shipMaxThrustClass3[i]; shipIndex[i].skid = shipSkidClass3[i]; shipIndex[i].resistance = shipResistanceClass3[i]; break;
        }
        switch (options->shipClass) {
        case 0: shipIndex[i].headingInc = shipHeadingIncClass0[i]; shipIndex[i].maxHeading = shipMaxHeadingClass0[i]; break;
        case 1: shipIndex[i].headingInc = shipHeadingIncClass1[i]; shipIndex[i].maxHeading = shipMaxHeadingClass1[i]; break;
        case 2: shipIndex[i].headingInc = shipHeadingIncClass2[i]; shipIndex[i].maxHeading = shipMaxHeadingClass2[i]; break;
        case 3: shipIndex[i].headingInc = shipHeadingIncClass3[i]; shipIndex[i].maxHeading = shipMaxHeadingClass3[i]; break;
        }
        if (ownShip == i) {
            shipIndex[i].update = UpdatePlayerShipStart;
        } else {
            shipIndex[i].update = UpdateDroneShipStart;
        }
        if (networkGame) {
            shipIndex[i].update = UpdateShipStart;
        }
        switch (options->shipClass) {
        case 0: shipIndex[i].remoteMax = shipRemoteMaxClass0[i]; shipIndex[i].remoteMagnitude = shipRemoteMagClass0[i]; break;
        case 1: shipIndex[i].remoteMax = shipRemoteMaxClass1[i]; shipIndex[i].remoteMagnitude = shipRemoteMagClass1[i]; break;
        case 2: shipIndex[i].remoteMax = shipRemoteMaxClass2[i]; shipIndex[i].remoteMagnitude = shipRemoteMagClass2[i]; break;
        case 3: shipIndex[i].remoteMax = shipRemoteMaxClass3[i]; shipIndex[i].remoteMagnitude = shipRemoteMagClass3[i]; break;
        }
        if (gridRank == 0 || gridRank == 1) {
            shipIndex[i].remoteMax += options->shipClass * 30;
        }
        SetSpread(shipIndex, options, gridRank, i, bigGrid);
        shipIndex[i].nearTrkSect = gridSections[gridRank];
        shipIndex[i].prevShipSect = gridSections[gridRank];
        gridFace = track->faces + gridSections[gridRank]->faceStart;
        while (!(gridFace->flags & TRACK_BASE)) {
            gridFace++;
        }
        if (gridSidePtr[gridRank]) {
            gridFace++;
        }
        base.vx = (track->vertices[gridFace->vertex[2]].vx + track->vertices[gridFace->vertex[0]].vx) >> 1;
        base.vy = (track->vertices[gridFace->vertex[2]].vy + track->vertices[gridFace->vertex[0]].vy) >> 1;
        base.vz = (track->vertices[gridFace->vertex[2]].vz + track->vertices[gridFace->vertex[0]].vz) >> 1;
        shipIndex[i].ppivot.vx = ((gridFace->normal.vx * 300) >> 12) + base.vx;
        shipIndex[i].ppivot.vy = ((gridFace->normal.vy * 300) >> 12) + base.vy;
        shipIndex[i].ppivot.vz = ((gridFace->normal.vz * 300) >> 12) + base.vz;
        sectionNo = gridSections[gridRank]->secNo;
        shipIndex[i].nearSecNo = sectionNo;
        shipIndex[i].prevSecNo = sectionNo;
        shipIndex[i].raceProgress.distance = raceLines[sectionNo].distance;
        shipIndex[i].raceProgress.trackDistance = shipIndex[i].raceProgress.distance;
        forward = gridSections[gridRank]->nextSection.ptr;
        dir.vx = forward->centre.vx - gridSections[gridRank]->centre.vx;
        dir.vy = forward->centre.vy - gridSections[gridRank]->centre.vy;
        dir.vz = forward->centre.vz - gridSections[gridRank]->centre.vz;
        shipIndex[i].hdg = -ratan2(dir.vx, dir.vz);
    }
    activeShipCount = 0;
    for (i = 0; i < 15; i++) {
        if (!(shipIndex[i].combatAttr & DESTROYED)) {
            shipRank[activeShipCount].position = raceLines[shipIndex[i].nearSecNo].distance;
            shipRank[activeShipCount].shipNo = i;
            activeShipCount++;
        }
    }
    finishPosition = gridPosition[ownShip] + 1;
}
#endif
#endif

#endif

/* Sets the launch boost duration from the starting grid position and track. */
void SetSpread(ShipData *shipIndex, ConfigData *cfg, short pos, short num, short bigGrid)
{
    short boostFrames;

    if (!bigGrid) {
        boostFrames = 12 - pos;
    } else {
        boostFrames = 15 - pos;
    }
    switch (cfg->shipClass) {
    case 0:
        shipIndex[num].boostCount = boostFrames *
#ifdef NETPLAY
            (short)spreadScale0[cfg->trackNum];
#else
            spreadScale0[cfg->trackNum];
#endif
        break;
    case 1:
        shipIndex[num].boostCount = boostFrames *
#ifdef NETPLAY
            (short)spreadScale1[cfg->trackNum];
#else
            spreadScale1[cfg->trackNum];
#endif
        break;
    case 2:
        shipIndex[num].boostCount = boostFrames *
#ifdef NETPLAY
            (short)spreadScale2[cfg->trackNum];
#else
            spreadScale2[cfg->trackNum];
#endif
        break;
    case 3:
        shipIndex[num].boostCount = boostFrames *
#ifdef NETPLAY
            (short)spreadScale3[cfg->trackNum];
#else
            spreadScale3[cfg->trackNum];
#endif
        break;
    }
#ifdef NETPLAY
    if (num == ownShip && !networkGame) {
#else
    if (num == ownShip) {
#endif
        shipIndex[num].boostCount = 0;
    }
}
