#include "functions.h"
#include "race_globals.h"
#include "pc_math.h"

extern CameraUpdateFunc CameraUpdate;

/* Positions the camera at a track section and clears its motion. */
#ifdef NETPLAY
/* NET-WOXL compiled this function unoptimised (the rest of camera.c is /O2 there); D3D keeps /O2. */
#pragma optimize("", off)
#endif
void InitCamera(TrackCamera *camera, TrackSection *section)
{
    VECTOR pos;

    camera->section = section;
    {
        /* The loop index is block-scoped: that gives the stack layout of the executable. */
        int i;

        for (i = 0; i < 2; i++)
            camera->section = camera->section->prevSection.ptr;
    }
    pos.vx = camera->section->centre.vx;
    pos.vy = camera->section->centre.vy;
    pos.vz = camera->section->centre.vz;
    SetSkeletonPosition(camera->camPos, -pos.vx, -pos.vy, -pos.vz);
    camera->hdg = 0;
    camera->pitch = 0;
    camera->roll = 0;
    camera->vel.vx = 0;
    camera->vel.vy = 0;
    camera->vel.vz = 0;
}

extern int raceExternalVel;
#pragma optimize("", off)
/* Follows the player's ship from an external racing view. */
void UpdateCameraRaceExternal(TrackCamera *camera, ShipData *ship, RescueData *droid)
{
    VECTOR pos;
    TrackSection *nextSection;
    VECTOR bestPath;
    VECTOR distance;
    int distMag;

    pos.vx = ship->ppivot.vx - (ship->unitVecNose.vx >> 2);
    pos.vy = ship->ppivot.vy - (ship->unitVecNose.vy >> 2);
    pos.vz = ship->ppivot.vz - (ship->unitVecNose.vz >> 2);
    pos.vy -= 200;
    camera->section = FindNearestSection(camera->section, pos);
    nextSection = camera->section->nextSection.ptr;
    GetTargetPos(&bestPath, pos, nextSection->centre, camera->section->centre);
    distance.vx = pos.vx - bestPath.vx;
    distance.vy = pos.vy - bestPath.vy;
    distance.vz = pos.vz - bestPath.vz;
    camera->vel.vx -= (short)(distance.vx >> 6);
    camera->vel.vy -= (short)(distance.vy >> 6);
    camera->vel.vz -= (short)(distance.vz >> 6);
    camera->vel.vx -= camera->vel.vx / 8;
    camera->vel.vy -= camera->vel.vy / 8;
    camera->vel.vz -= camera->vel.vz / 8;
    pos.vx += camera->vel.vx;
    pos.vy += camera->vel.vy;
    pos.vz += camera->vel.vz;
    distMag = GetMagnitude(distance.vx, distance.vy, distance.vz);
    raceExternalVel += distMag >> 7;
    raceExternalVel -= raceExternalVel >> 4;
    pos.vy -= raceExternalVel;
    SetSkeletonPosition(camera->camPos, -pos.vx, -pos.vy, -pos.vz);
    camera->pitch = -ship->pitch;
    camera->roll = 0;
    camera->hdg = -ship->hdg;
    SetSkeletonDirection(camera->camPos->super, camera->pitch, camera->hdg, 0);
    camera->camPos->super->update = 1;
}
#pragma optimize("gtyw", on)

/* Byte-exact as written, but in the default build it shifts the compiler heap state so
   UpdateCameraAttractCircle no longer matches even with dummy declarations: kept in EQUIV. */
#ifdef EQUIV
/* Places the racing camera at the ship's cockpit position. */
#ifdef NETPLAY
void UpdateCameraRaceInternal(TrackCamera *camera, ShipData *ship, RescueData *droid)
{
    short sx, sy, sz;
    short cx, cy, cz;
    VECTOR unitFinVector;

    camera->section = ship->nearTrkSect;
    sy = rsin(ship->hdg);
    sx = rsin(ship->pitch);
    sz = rsin(ship->roll);
    cy = rcos(ship->hdg);
    cx = rcos(ship->pitch);
    cz = rcos(ship->roll);
    unitFinVector.vx = ((cy * sz) >> 12) - ((((sy * sx) >> 12) * cz) >> 12);
    unitFinVector.vy = (cx * cz) >> 12;
    unitFinVector.vz = ((sy * sz) >> 12) + ((((cy * sx) >> 12) * cz) >> 12);
    SetSkeletonPosition(camera->camPos, (unitFinVector.vx >> 5) - ship->ppivot.vx,
                        (unitFinVector.vy >> 5) - ship->ppivot.vy, (unitFinVector.vz >> 5) - ship->ppivot.vz);
    camera->pitch = -ship->pitch;
    camera->hdg = -ship->hdg;
    camera->roll = -ship->roll;
    SetSkeletonDirectionRPH(camera->camPos->super, -ship->roll, -ship->pitch, -ship->hdg);
    camera->camPos->super->update = 1;
}
#else
void UpdateCameraRaceInternal(TrackCamera *camera, ShipData *ship, RescueData *droid)
{
    short sx, sy, sz;
    short cx, cy, cz;
    VECTOR unitFinVector;

    camera->section = ship->nearTrkSect;
    sy = rsin(ship->hdg);
    sx = rsin(ship->pitch);
    cx = rcos(ship->pitch);
    sz = rsin(ship->roll);
    cy = rcos(ship->hdg);
    cz = rcos(ship->roll);
    unitFinVector.vx = ((cy * sz) >> 12) - ((((sy * sx) >> 12) * cz) >> 12);
    unitFinVector.vy = (cx * cz) >> 12;
    unitFinVector.vz = ((sy * sz) >> 12) + ((((cy * sx) >> 12) * cz) >> 12);
    SetSkeletonPosition(camera->camPos, (unitFinVector.vx >> 5) - ship->ppivot.vx,
                        (unitFinVector.vy >> 5) - ship->ppivot.vy, (unitFinVector.vz >> 5) - ship->ppivot.vz);
    camera->pitch = -ship->pitch;
    camera->hdg = -ship->hdg;
    camera->roll = -ship->roll;
    SetSkeletonDirectionRPH(camera->camPos->super, -ship->roll, -ship->pitch, -ship->hdg);
    camera->camPos->super->update = 1;
}
#endif
#endif
#include "pc_game.h"
extern int gameType;
extern PCGameConfig *tempGameData;
#pragma optimize("", off)
/* Animates the camera from the grid to the player's ship. */
void UpdateCameraRaceStart(TrackCamera *camera, ShipData *ship, RescueData *droid)
{
    VECTOR pos;
    VECTOR beginPos;
    VECTOR dir;
    TrackSection *nextSection;
    int i;
    int initialSpeed;
    int elapsed;
    int a;
    int cameraStepX;
    int fraction;
    int velY;
    int zDelta;
    int horizontalDistance;

    initialSpeed = 2;
    a = ((1 - initialSpeed) << 8) / 50;
    nextSection = ship->nearTrkSect->nextSection.ptr;
    if (gameType == 1) {
        for (i = 0; i < 3; i++)
            nextSection = nextSection->nextSection.ptr;
    } else {
        for (i = 0; i < 9; i++)
            nextSection = nextSection->nextSection.ptr;
    }
    pos.vx = nextSection->centre.vx;
    pos.vy = nextSection->centre.vy;
    pos.vz = nextSection->centre.vz;
    pos.vy -= 800;
    beginPos.vx = ship->nearTrkSect->centre.vx;
    beginPos.vy = ship->ppivot.vy - 400;
    beginPos.vz = ship->nearTrkSect->centre.vz;
    cameraStepX = ((beginPos.vx - pos.vx) * (ship->updateCount - 100)) << 8;
    velY = ((beginPos.vy - pos.vy) * (ship->updateCount - 100)) << 8;
    zDelta = ((beginPos.vz - pos.vz) * (ship->updateCount - 100)) << 8;
    cameraStepX = (cameraStepX / 100) >> 8;
    velY = (velY / 100) >> 8;
    zDelta = (zDelta / 100) >> 8;
    elapsed = 200 - ship->updateCount;
    fraction = (elapsed * initialSpeed) << 8;
    fraction += (a * elapsed * elapsed) >> 1;
    fraction = 25600 - fraction;
    cameraStepX = beginPos.vx - pos.vx;
    velY = beginPos.vy - pos.vy;
    zDelta = beginPos.vz - pos.vz;
    cameraStepX = ((cameraStepX * fraction) / 100) >> 8;
    velY = ((velY * fraction) / 100) >> 8;
    zDelta = ((zDelta * fraction) / 100) >> 8;
    pos.vx = beginPos.vx - cameraStepX;
    pos.vy = beginPos.vy - velY;
    pos.vz = beginPos.vz - zDelta;
    camera->section = FindNearestSection(camera->section, pos);
    dir.vx = ship->ppivot.vx - pos.vx;
    dir.vy = ship->ppivot.vy - pos.vy;
    dir.vz = ship->ppivot.vz - pos.vz;
    horizontalDistance = sqrt(dir.vz * dir.vz + dir.vx * dir.vx);
    SetSkeletonPosition(camera->camPos, -pos.vx, -pos.vy, -pos.vz);
    camera->hdg = ratan2(dir.vx, dir.vz);
    camera->pitch = ratan2(dir.vy, horizontalDistance);
    SetSkeletonDirection(camera->camPos->super, camera->pitch, camera->hdg, 0);
    camera->camPos->super->update = 1;
    if (ship->updateCount == 100) {
        if (tempGameData->cameraInternalView) {
            ship->attr |= VIEW_IN;
            CameraUpdate = UpdateCameraRaceInternal;
        } else {
            ship->attr &= ~VIEW_IN;
            CameraUpdate = UpdateCameraRaceExternal;
        }
    }
}
#pragma optimize("gtyw", on)

extern short cameraTarget;
/* Circles the ship during the attract mode demo. */
void UpdateCameraAttractCircle(TrackCamera *camera, ShipData *ship, RescueData *droid)
{
    VECTOR pos;
    int magnitude;
    VECTOR targetVector;
    int finX, finY, finZ;
    short sy, sx, sz;
    short cy, cx, cz;

    camera->section = ship->nearTrkSect;
    if (cameraTarget == 0)
        cameraTarget = 150;
    cameraTarget--;
    if (cameraTarget == 0)
        CameraUpdate = UpdateCameraAttractMode;
    sy = rsin(ship->hdg);
    sx = rsin(ship->pitch);
    sz = rsin(ship->roll);
    cy = rcos(ship->hdg);
    cx = rcos(ship->pitch);
    cz = rcos(ship->roll);
    finX = ((cy * sz) >> 12) - ((((sy * sx) >> 12) * cz) >> 12);
    finY = (cx * cz) >> 12;
    finZ = ((sy * sz) >> 12) + ((((cy * sx) >> 12) * cz) >> 12);
    pos.vx = ship->ppivot.vx + (rsin(ship->hdg) / 6);
    pos.vy = ship->ppivot.vy + ((ship->pitch >> 2) - 200);
    pos.vz = ship->ppivot.vz - (rcos(ship->hdg) / 6);
    pos.vx += rsin(cameraTarget << 3) >> 3;
    pos.vy -= 200;
    pos.vz += rcos(cameraTarget << 3) >> 3;
    pos.vx -= finX >> 4;
    pos.vy -= finY >> 4;
    pos.vz -= finZ >> 4;
    targetVector.vx = ship->ppivot.vx - pos.vx;
    targetVector.vy = ship->ppivot.vy - pos.vy;
    targetVector.vz = ship->ppivot.vz - pos.vz;
    magnitude = SquareRoot0(targetVector.vx * targetVector.vx + targetVector.vz * targetVector.vz);
    SetSkeletonPosition(camera->camPos, -pos.vx, -pos.vy, -pos.vz);
    camera->hdg = ratan2(targetVector.vx, targetVector.vz);
    camera->pitch = ratan2(targetVector.vy, magnitude);
    SetSkeletonDirection(camera->camPos->super, camera->pitch, camera->hdg, 0);
    camera->camPos->super->update = 1;
}
#ifdef NETPLAY
/* NET-WOXL compiled this without /Oa, /Ow or /Oy: cameraTarget stays in memory across the statements. */
#pragma optimize("yaw", off)
#endif
/* Frames the ship and medal during the attract mode demo. */
void UpdateCameraAttractMedal(TrackCamera *camera, ShipData *ship, RescueData *droid)
{
    VECTOR pos;
    VECTOR targetVector;
    int magnitude;

    camera->section = ship->nearTrkSect;
    if (cameraTarget == 0)
        cameraTarget = 150;
    cameraTarget--;
    if (cameraTarget == 0)
        CameraUpdate = UpdateCameraAttractMode;
    pos.vx = ship->ppivot.vx + (rsin(ship->hdg) >> 5);
    pos.vy = ship->ppivot.vy;
    pos.vz = ship->ppivot.vz - (rcos(ship->hdg) >> 5);
    pos.vx += rsin(cameraTarget << 3) >> 5;
    pos.vy -= 1024;
    pos.vz += rcos(cameraTarget << 3) >> 5;
    targetVector.vx = ship->ppivot.vx - pos.vx;
    targetVector.vy = ship->ppivot.vy - pos.vy;
    targetVector.vz = ship->ppivot.vz - pos.vz;
    magnitude = SquareRoot0(targetVector.vx * targetVector.vx + targetVector.vz * targetVector.vz);
    SetSkeletonPosition(camera->camPos, -pos.vx, -pos.vy, -pos.vz);
    camera->hdg = ratan2(targetVector.vx, targetVector.vz);
    camera->pitch = ratan2(targetVector.vy, magnitude);
    SetSkeletonDirection(camera->camPos->super, camera->pitch, camera->hdg, 0);
    camera->camPos->super->update = 1;
}
#ifdef NETPLAY
#pragma optimize("gtyaw", on)
#endif

void UpdateCameraStaticFollow(TrackCamera *camera, ShipData *ship, RescueData *droid);

extern VECTOR cameraStaticPos;

#ifdef NETPLAY
/* NET-WOXL compiled this without /Oa, /Ow or /Oy: cameraTarget stays in memory across the statements. */
#pragma optimize("yaw", off)
#endif
/* Shows the player's destroyed ship during attract mode. */
void UpdateCameraAttractDestroyed(TrackCamera *camera, ShipData *ship, RescueData *droid)
{
    VECTOR pos;
    VECTOR targetVector;
    int magnitude;

    shipIndex[ownShip].attr &= ~VIEW_IN;
    camera->section = ship->nearTrkSect;
    if (cameraTarget == 0)
        cameraTarget = 32000;
    cameraTarget -= 2;
    if (cameraTarget <= 0)
        cameraTarget = 32000;
    pos.vx = ship->ppivot.vx + rsin(ship->hdg) / 6;
    pos.vy = ship->ppivot.vy;
    pos.vz = ship->ppivot.vz + rcos(ship->hdg) / -6;
    pos.vx += rsin(cameraTarget << 3) >> 2;
    pos.vy -= 800;
    pos.vz += rcos(cameraTarget << 3) >> 2;
    targetVector.vx = ship->ppivot.vx - pos.vx;
    targetVector.vy = ship->ppivot.vy - pos.vy;
    targetVector.vz = ship->ppivot.vz - pos.vz;
    magnitude = SquareRoot0(targetVector.vx * targetVector.vx + targetVector.vz * targetVector.vz);
    SetSkeletonPosition(camera->camPos, -pos.vx, -pos.vy, -pos.vz);
    camera->hdg = ratan2(targetVector.vx, targetVector.vz);
    camera->pitch = ratan2(targetVector.vy, magnitude);
    SetSkeletonDirection(camera->camPos->super, camera->pitch, camera->hdg, 0);
    camera->camPos->super->update = 1;
}
#ifdef NETPLAY
#pragma optimize("gtyaw", on)
#endif


extern int droidViewVel;
#ifdef NETPLAY
/* NET-WOXL compiled this without /Oa, /Ow or /Oy: cameraTarget stays in memory across the statements. */
#pragma optimize("yaw", off)
#endif
/* Updates the camera while the rescue droid is active. */
void UpdateCameraDroidView(TrackCamera *camera, ShipData *ship, RescueData *droid)
{
    VECTOR pos;
    VECTOR bestPath;
    VECTOR distance;
    TrackSection *previous;
    int magnitude;

    pos.vx = ship->ppivot.vx + (ship->unitVecNose.vx >> 2);
    pos.vy = ship->ppivot.vy + (ship->unitVecNose.vy >> 2);
    pos.vz = ship->ppivot.vz + (ship->unitVecNose.vz >> 2);
    pos.vy -= 200;
    camera->section = FindNearestSection(camera->section, pos);
    previous = camera->section->prevSection.ptr;
    GetTargetPos(&bestPath, pos, previous->centre, camera->section->centre);
    distance.vx = pos.vx - bestPath.vx;
    distance.vy = pos.vy - bestPath.vy;
    distance.vz = pos.vz - bestPath.vz;
    camera->vel.vx -= distance.vx >> 6;
    camera->vel.vy -= distance.vy >> 6;
    camera->vel.vz -= distance.vz >> 6;
    camera->vel.vx -= camera->vel.vx / 8;
    camera->vel.vy -= camera->vel.vy / 8;
    camera->vel.vz -= camera->vel.vz / 8;
    pos.vx += camera->vel.vx;
    pos.vy += camera->vel.vy;
    pos.vz += camera->vel.vz;
    magnitude = GetMagnitude(distance.vx, distance.vy, distance.vz);
    droidViewVel += magnitude >> 7;
    droidViewVel -= droidViewVel >> 4;
    pos.vy -= droidViewVel;
    SetSkeletonPosition(camera->camPos, -pos.vx, -pos.vy, -pos.vz);
    camera->pitch = -ship->pitch;
    camera->roll = 0;
    camera->hdg = -(ship->hdg + 2048);
    SetSkeletonDirection(camera->camPos->super, camera->pitch, camera->hdg, 0);
    camera->camPos->super->update = 1;
}
#ifdef NETPLAY
#pragma optimize("gtyaw", on)
#endif


#ifdef NETPLAY
/* NET-WOXL compiled this without /Oa, /Ow or /Oy: cameraTarget stays in memory across the statements. */
#pragma optimize("yaw", off)
#endif
/* Tracks the ship from its cockpit during attract mode. */
void UpdateCameraAttractInternal(TrackCamera *camera, ShipData *ship, RescueData *droid)
{
    short sx, sy, sz;
    short cx, cy, cz;
    VECTOR unitFinVector;

    if (cameraTarget == 0)
        cameraTarget = 150;
    cameraTarget--;
    if (cameraTarget == 0)
        CameraUpdate = UpdateCameraAttractMode;
    camera->section = ship->nearTrkSect;
    sy = rsin(ship->hdg);
    sx = rsin(ship->pitch);
    sz = rsin(ship->roll);
    cy = rcos(ship->hdg);
    cx = rcos(ship->pitch);
    cz = rcos(ship->roll);
    unitFinVector.vx = ((cy * sz) >> 12) - ((((sy * sx) >> 12) * cz) >> 12);
    unitFinVector.vy = (cx * cz) >> 12;
    unitFinVector.vz = ((sy * sz) >> 12) + ((((cy * sx) >> 12) * cz) >> 12);
    SetSkeletonPosition(camera->camPos, (unitFinVector.vx >> 5) - ship->ppivot.vx,
                        (unitFinVector.vy >> 5) - ship->ppivot.vy, (unitFinVector.vz >> 5) - ship->ppivot.vz);
    camera->pitch = -ship->pitch;
    camera->hdg = -ship->hdg;
    SetSkeletonDirectionRPH(camera->camPos->super, -ship->roll, -ship->pitch, -ship->hdg);
    camera->camPos->super->update = 1;
}
#ifdef NETPLAY
#pragma optimize("gtyaw", on)
#endif

/* The section Z load precedes the cameraStaticPos.vy store. */
#ifdef EQUIV
/* Holds a fixed camera ahead of the ship during attract mode. */
#ifdef NETPLAY
/* NET-WOXL compiled this without /Oa, /Ow or /Oy (it keeps the frame pointer, and cameraTarget stays in memory). */
#pragma optimize("yaw", off)
void UpdateCameraStaticFollow(TrackCamera *camera, ShipData *ship, RescueData *droid)
{
    int i;
    int magnitude;
    VECTOR delta;
    TrackSection *section;
    int vy;

    if (cameraTarget == 0) {
        section = ship->nearTrkSect->nextSection.ptr;
        for (i = 0; i < 10; i++)
            section = section->nextSection.ptr;
        camera->section = section;
        cameraStaticPos.vx = section->centre.vx;
        vy = section->centre.vy - 500;
        cameraStaticPos.vy = vy;
        {
            int sectionCentreZ;

            sectionCentreZ = section->centre.vz;
            cameraTarget = 150;
            cameraStaticPos.vz = sectionCentreZ;
        }
    }
    cameraTarget--;
    if (cameraTarget == 0)
        CameraUpdate = UpdateCameraAttractMode;
    delta.vx = ship->ppivot.vx - cameraStaticPos.vx;
    delta.vy = ship->ppivot.vy - cameraStaticPos.vy;
    delta.vz = ship->ppivot.vz - cameraStaticPos.vz;
    magnitude = SquareRoot0(delta.vx * delta.vx + delta.vz * delta.vz);
    SetSkeletonPosition(camera->camPos, -cameraStaticPos.vx, -cameraStaticPos.vy, -cameraStaticPos.vz);
    camera->hdg = ratan2(delta.vx, delta.vz);
    camera->pitch = ratan2(delta.vy, magnitude);
    SetSkeletonDirection(camera->camPos->super, camera->pitch, camera->hdg, 0);
    camera->camPos->super->update = 1;
}
#pragma optimize("gtyaw", on)
#else
void UpdateCameraStaticFollow(TrackCamera *camera, ShipData *ship, RescueData *droid)
{
    int i;
    int magnitude;
    VECTOR delta;
    TrackSection *section;
    int vy;

    if (cameraTarget == 0) {
        section = ship->nearTrkSect->nextSection.ptr;
        for (i = 0; i < 10; i++)
            section = section->nextSection.ptr;
        cameraStaticPos.vx = section->centre.vx;
        vy = section->centre.vy - 500;
        camera->section = section;
        {
            int sectionCentreZ;

            sectionCentreZ = section->centre.vz;
            cameraStaticPos.vz = sectionCentreZ;
            cameraStaticPos.vy = vy;
        }
        cameraTarget = 150;
    }
    cameraTarget--;
    if (cameraTarget == 0)
        CameraUpdate = UpdateCameraAttractMode;
    delta.vx = ship->ppivot.vx - cameraStaticPos.vx;
    delta.vy = ship->ppivot.vy - cameraStaticPos.vy;
    delta.vz = ship->ppivot.vz - cameraStaticPos.vz;
    magnitude = SquareRoot0(delta.vx * delta.vx + delta.vz * delta.vz);
    SetSkeletonPosition(camera->camPos, -cameraStaticPos.vx, -cameraStaticPos.vy, -cameraStaticPos.vz);
    camera->hdg = ratan2(delta.vx, delta.vz);
    camera->pitch = ratan2(delta.vy, magnitude);
    SetSkeletonDirection(camera->camPos->super, camera->pitch, camera->hdg, 0);
    camera->camPos->super->update = 1;
}
#endif
#endif

/* Selects and updates an attract mode camera view. */
void UpdateCameraAttractMode(TrackCamera *camera, ShipData *ship, RescueData *droid)
{
    cameraTarget = 0;
    if (shipIndex[ownShip].combatAttr & DESTROYED) {
        CameraUpdate = UpdateCameraAttractDestroyed;
        ship->attr &= ~VIEW_IN;
    } else {
        switch (newRand() % 2) {
        case 0:
            CameraUpdate = UpdateCameraAttractCircle;
            ship->attr &= ~VIEW_IN;
            break;
        case 1:
            CameraUpdate = UpdateCameraStaticFollow;
            ship->attr &= ~VIEW_IN;
            break;
        }
    }
    CameraUpdate(camera, ship, droid);
}

/* Calculates the ship fin's unit vector in world coordinates. */
void SetUnitFinVector(VECTOR *fin, ShipData *ship)
{
    short sx, sy, sz;
    short cx, cy, cz;

    sy = rsin(ship->hdg);
    sx = rsin(ship->pitch);
    sz = rsin(ship->roll);
    cy = rcos(ship->hdg);
    cx = rcos(ship->pitch);
    cz = rcos(ship->roll);
    fin->vx = ((cy * sz) >> 12) - ((((sy * sx) >> 12) * cz) >> 12);
    fin->vy = (cx * cz) >> 12;
    fin->vz = ((sy * sz) >> 12) + ((((cy * sx) >> 12) * cz) >> 12);
}

/* Places the camera above the ship and points it down the track. */
void SetCameraForShip(TrackCamera *camera, ShipData *ship)
{
    VECTOR pos;

    camera->section = ship->nearTrkSect;
    pos.vx = ship->ppivot.vx;
    pos.vy = ship->ppivot.vy;
    pos.vz = ship->ppivot.vz;
    pos.vy -= 1500;
    SetSkeletonPosition(camera->camPos, -pos.vx, -pos.vy, -pos.vz);
    camera->pitch = 1024;
    camera->hdg = 0;
    SetSkeletonDirection(camera->camPos->super, 1024, 0, 0);
    camera->camPos->super->update = 1;
}
