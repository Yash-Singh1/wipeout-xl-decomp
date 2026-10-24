#include <math.h>
#include "functions.h"
#include "input_state.h"
#include "droid.h"

/* sin/cos of a 4096-per-turn angle, scaled by 4096. */
#undef rsin
#undef rcos
#ifdef NETPLAY
#pragma intrinsic(sin, cos, abs)
/* NET-WOXL: empty configuration tests at the start of each branch and after each acc.vy store;
   the exe keeps their jumps. */
#define NET_DROID_START_TEST 1
#endif
#define ANGLE_HALF_TURN 0x800
#define ANGLE_FULL_TURN 0x1000
#define rsin(a) ((long)(sin(((double)(a) / 4096.0) * 6.28318530718) * 4096.0))
#define rcos(a) ((long)(cos(((double)(a) / 4096.0) * 6.28318530718) * 4096.0))

extern TrackData *track;
extern TrackCamera *cameraPtr;
extern CameraUpdateFunc CameraUpdate;
extern short droidVoice;
extern int droidFlyin;

void DroidSound(short on);
void SetNoteVolume(short sfxId, unsigned char volume);
void UpdateCameraDroidView(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);
void UpdateCameraRaceInternal(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);
void UpdateCameraRaceExternal(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);
void AddRenderObject(Object *object, int mode, u_char *rgb);
void UpdateDroidStart(RescueData *rescueDroid, ShipData *shipIndex, Object **shipShapes);

#ifdef NETPLAY
void UpdateRacingShip(ShipData *shipIndex, void *shapes, short shipNo, void *weapons);
#else
#endif

#ifdef NETPLAY
VECTOR droidTarget[8];
#else
VECTOR droidTarget;
#endif

/* Same computation as the exe, with different registers and scheduling of the
   droid-target interpolation. */
#ifdef EQUIV
/* Updates the rescue ship's motion while it tows the player. */
#ifdef NETPLAY
void UpdateRescueDroid(ShipData *shipIndex, Object **shipShapes, short shipNo, WeaponData *weaponIndex)
{
    TrackSection *section;
    TrackSection *nextSection;
    ShipData *droid;
    float offX;
    float offY;
    int index;
    float offZ;
    VECTOR nose;

    droid = &shipIndex[shipNo];
    droid->attr |= FLYING;

    if (padButtons & PAD_BUTTON_THRUST) {
        droid->thrust += 0x10;
    } else if (droid->thrust > 15) {
        droid->thrust -= 0x10;
    }
    if (droid->thrust > droid->maxThrust) {
        droid->thrust = droid->maxThrust;
    }

    if (droid->updateCount == 500) {
        section = droid->nearTrkSect->prevSection.ptr;
        section = section->prevSection.ptr;
        section = section->prevSection.ptr;
        section = section->prevSection.ptr;
        for (index = 0; index < 10; index++) {
            section = section->nextSection.ptr;
            if (section->flags & TRACK_BASE) {
                break;
            }
        }
        droid->nearTrkSect = section;
        nextSection = section->nextSection.ptr;

        droidTarget[shipNo].vx = (section->centre.vx + nextSection->centre.vx) >> 1;
        droidTarget[shipNo].vy = ((nextSection->centre.vy + section->centre.vy) >> 1) - 0x7d0;
        droidTarget[shipNo].vz = (section->centre.vz + nextSection->centre.vz) >> 1;
        droid->vpivot.vx = 0;
        droid->vpivot.vy = 0;
        droid->vpivot.vz = 0;
        droid->updateCount--;
    }

    nextSection = droid->nearTrkSect->nextSection.ptr;

    if (droid->attr & IN_TOW) {
        droidTarget[shipNo].vx += (nextSection->centre.vx - droidTarget[shipNo].vx) >> 6;
        droidTarget[shipNo].vy += (nextSection->centre.vy - droidTarget[shipNo].vy) >> 6;
        droidTarget[shipNo].vz += (nextSection->centre.vz - droidTarget[shipNo].vz) >> 6;

        droid->vpivot.vx = droidTarget[shipNo].vx - droid->ppivot.vx;
        droid->vpivot.vy = droidTarget[shipNo].vy - droid->ppivot.vy;
        droid->vpivot.vz = droidTarget[shipNo].vz - droid->ppivot.vz;

        nose.vx = nextSection->centre.vx - droid->nearTrkSect->centre.vx;
        nose.vy = nextSection->centre.vy - droid->nearTrkSect->centre.vy;
        nose.vz = nextSection->centre.vz - droid->nearTrkSect->centre.vz;

        droid->vhdg = -ratan2(nose.vx, nose.vz) - droid->hdg;
        if (droid->vhdg > ANGLE_HALF_TURN) {
            droid->vhdg -= ANGLE_FULL_TURN;
        } else if (droid->vhdg < -ANGLE_HALF_TURN) {
            droid->vhdg += ANGLE_FULL_TURN;
        }
        droid->hdg += droid->vhdg >> 6;
        droid->hdg = ang(droid->hdg);
    }

    droid->vpivot.vx -= droid->vpivot.vx >> 4;
    droid->vpivot.vy -= droid->vpivot.vy >> 4;
    droid->vpivot.vz -= droid->vpivot.vz >> 4;

    droid->ppivot.vx += droid->vpivot.vx >> 5;
    droid->ppivot.vy += droid->vpivot.vy >> 5;
    droid->ppivot.vz += droid->vpivot.vz >> 5;

    offX = droid->ppivot.vx - droid->nearTrkSect->centre.vx;
    offY = droid->ppivot.vy - droid->nearTrkSect->centre.vy;
    offZ = droid->ppivot.vz - droid->nearTrkSect->centre.vz;

    droid->pitch -= droid->pitch >> 3;
    droid->roll -= droid->roll >> 5;

    if ((droid->attr & IN_TOW) && (GetMagnitude(offX, offY, offZ) < 1600)) {
        if (networkGame) {
            droid->update = UpdateRacingShip;
        } else {
            droid->update = UpdatePlayerShip;
        }
        droid->attr &= ~RESCUE;
        droid->updateCount = 90;
    }
}
#else
void UpdateRescueDroid(ShipData *shipIndex, Object **shipShapes, short shipNo, WeaponData *weaponIndex)
{
    ShipData *ship;
    TrackSection *section;
    TrackSection *next;
    VECTOR nose;
    float distX, distY, distZ;
    int updateCount;
    int attr;
    short heading;
    int i;

    ship = &shipIndex[shipNo];
    attr = ship->attr | FLYING;
    ship->attr = attr;

    if (padButtons & PAD_BUTTON_THRUST) {
        ship->thrust += 0x10;
    } else if (ship->thrust > 15) {
        ship->thrust -= 0x10;
    }
    if (ship->maxThrust < ship->thrust) {
        ship->thrust = ship->maxThrust;
    }

    updateCount = ship->updateCount;
    if (updateCount == 500) {
        section = ship->nearTrkSect->prevSection.ptr;
        section = section->prevSection.ptr;
        section = section->prevSection.ptr;
        section = section->prevSection.ptr;
        i = 0;
        do {
            section = section->nextSection.ptr;
            if (section->flags & TRACK_BASE) {
                break;
            }
            i++;
        } while (i < 10);
        next = section->nextSection.ptr;
        ship->nearTrkSect = section;

        droidTarget.vx = (section->centre.vx + next->centre.vx) >> 1;
        ship->vpivot.vx = 0;
        ship->vpivot.vy = 0;
        ship->vpivot.vz = 0;
        droidTarget.vy = ((next->centre.vy + section->centre.vy) >> 1) - 0x7d0;
        ship->updateCount = updateCount - 1;
        droidTarget.vz = (section->centre.vz + next->centre.vz) >> 1;
    }

    section = ship->nearTrkSect;
    next = section->nextSection.ptr;

    if (attr & IN_TOW) {
        droidTarget.vx += (next->centre.vx - droidTarget.vx) >> 6;
        droidTarget.vy += (next->centre.vy - droidTarget.vy) >> 6;
        droidTarget.vz += (next->centre.vz - droidTarget.vz) >> 6;

        ship->vpivot.vx = droidTarget.vx - ship->ppivot.vx;
        ship->vpivot.vy = droidTarget.vy - ship->ppivot.vy;
        ship->vpivot.vz = droidTarget.vz - ship->ppivot.vz;

        nose.vx = next->centre.vx - section->centre.vx;
        nose.vz = next->centre.vz - section->centre.vz;

        heading = -(ship->hdg + ratan2(nose.vx, nose.vz));
        ship->vhdg = heading;
        if (heading > ANGLE_HALF_TURN) {
            ship->vhdg = heading - ANGLE_FULL_TURN;
        } else if (heading < -ANGLE_HALF_TURN) {
            ship->vhdg = heading + ANGLE_FULL_TURN;
        }
        heading = ship->hdg + (ship->vhdg >> 6);
        ship->hdg = heading;
        ship->hdg = ang(heading);
    }

    ship->vpivot.vx -= ship->vpivot.vx >> 4;
    ship->vpivot.vy -= ship->vpivot.vy >> 4;
    ship->vpivot.vz -= ship->vpivot.vz >> 4;

    ship->ppivot.vx += ship->vpivot.vx >> 5;
    ship->ppivot.vy += ship->vpivot.vy >> 5;
    ship->ppivot.vz += ship->vpivot.vz >> 5;

    distX = ship->ppivot.vx - ship->nearTrkSect->centre.vx;
    distY = ship->ppivot.vy - ship->nearTrkSect->centre.vy;
    distZ = ship->ppivot.vz - ship->nearTrkSect->centre.vz;

    ship->pitch -= ship->pitch >> 3;
    ship->roll -= ship->roll >> 5;

    if ((ship->attr & IN_TOW) && (GetMagnitude(distX, distY, distZ) < 1600)) {
        ship->update = UpdatePlayerShip;
        ship->updateCount = 90;
        ship->attr &= ~RESCUE;
    }
}
#endif
#endif

/* Flies the rescue droid into position behind the player's ship. */
void UpdateDroidStart(RescueData *rescueDroid, ShipData *shipIndex, Object **shipShapes)
{
    rescueDroid->updateCount--;

    if (rescueDroid->updateCount < 400) {
#ifdef NETPLAY
        if (NET_DROID_START_TEST) {
        } else {
        }
        if (NET_DROID_START_TEST) {
        } else {
        }
#endif
        rescueDroid->acc.vx = (-rsin(rescueDroid->hdg) * rcos(rescueDroid->pitch)) >> 14;
        rescueDroid->acc.vy = 0;
#ifdef NETPLAY
        if (NET_DROID_START_TEST) {
        } else {
        }
        if (NET_DROID_START_TEST) {
        } else {
        }
#endif
        rescueDroid->acc.vz = (rcos(rescueDroid->hdg) * rcos(rescueDroid->pitch)) >> 14;
        rescueDroid->vhdg = 0;
    } else if (rescueDroid->updateCount < 710) {
#ifdef NETPLAY
        if (NET_DROID_START_TEST) {
        } else {
        }
        if (NET_DROID_START_TEST) {
        } else {
        }
#endif
        rescueDroid->acc.vx = (-rsin(rescueDroid->hdg) * rcos(rescueDroid->pitch)) >> 15;
        rescueDroid->acc.vy = -140;
#ifdef NETPLAY
        if (NET_DROID_START_TEST) {
        } else {
        }
        if (NET_DROID_START_TEST) {
        } else {
        }
#endif
        rescueDroid->acc.vz = (rcos(rescueDroid->hdg) * rcos(rescueDroid->pitch)) >> 15;
        rescueDroid->vhdg = -8;
    } else if (rescueDroid->updateCount == 770) {
        rescueDroid->acc.vy -= 90;
    } else {
        rescueDroid->hdg = shipIndex->hdg;
    }

    SetDroidVectors(rescueDroid, shipShapes);

    if (rescueDroid->updateCount < 500) {
        rescueDroid->updateCount = 800;
        rescueDroid->droidUpdate = UpdateDroidCircle;
        rescueDroid->pos.vx = rescueDroid->section->centre.vx;
        rescueDroid->pos.vy = rescueDroid->section->centre.vy - 5000;
        rescueDroid->pos.vz = rescueDroid->section->centre.vz;
    }
}

/* Circles the ship until the player activates the rescue droid. */
#ifdef NETPLAY
void UpdateDroidCircle(RescueData *rescueDroid, ShipData *shipIndex, Object **shipShapes)
{
    TrackSection *nextSection;
    int desiredHdg;
    short minTurn;
    short turnDelta;
    VECTOR targetPoint;
    VECTOR targetVector;

    nextSection = rescueDroid->section->nextSection.ptr;

    targetPoint.vx = (rescueDroid->section->centre.vx + nextSection->centre.vx) >> 1;
    targetPoint.vy = rescueDroid->section->centre.vy - 3000;
    targetPoint.vz = (rescueDroid->section->centre.vz + nextSection->centre.vz) >> 1;

    targetVector.vx = targetPoint.vx - rescueDroid->pos.vx;
    targetVector.vy = targetPoint.vy - rescueDroid->pos.vy;
    targetVector.vz = targetPoint.vz - rescueDroid->pos.vz;

    desiredHdg = -ratan2(targetVector.vx, targetVector.vz);

    minTurn = desiredHdg - rescueDroid->hdg;

    if (rescueDroid->hdg < 0) {
        turnDelta = desiredHdg - (rescueDroid->hdg + 4096);
    } else {
        turnDelta = desiredHdg - (rescueDroid->hdg - 4096);
    }

    if (abs(turnDelta) < abs(minTurn)) {
        rescueDroid->vhdg = turnDelta >> 6;
    } else {
        rescueDroid->vhdg = minTurn >> 6;
    }
    if (NET_DROID_START_TEST) {
    } else {
    }
    if (NET_DROID_START_TEST) {
    } else {
    }

    rescueDroid->acc.vx = (-rsin(rescueDroid->hdg) * rcos(rescueDroid->pitch)) >> 15;
    rescueDroid->acc.vy = targetVector.vy >> 6;
    if (NET_DROID_START_TEST) {
    } else {
    }
    if (NET_DROID_START_TEST) {
    } else {
    }
    rescueDroid->acc.vz = (rcos(rescueDroid->hdg) * rcos(rescueDroid->pitch)) >> 15;

    if (shipIndex->attr & RESCUE) {
        rescueDroid->droidUpdate = UpdateDroidRescue;
        rescueDroid->updateCount = 800;
        DroidSound(1);
        SetNoteVolume(droidVoice, 255);
        CameraUpdate = UpdateCameraDroidView;
        shipIndex->attr |= REMOTE_VIEW;
        if (shipIndex->nearTrkSect->flags & 1) {
            cameraPtr->section = shipIndex->nearTrkSect->nextSection.ptr;
        } else {
            cameraPtr->section = shipIndex->nearTrkSect;
        }
    }

    SetDroidVectors(rescueDroid, shipShapes);
}
#else
void UpdateDroidCircle(RescueData *rescueDroid, ShipData *shipIndex, Object **shipShapes)
{
    TrackSection *section, *nextSection;
    VECTOR targetVector;
    VECTOR target;
    int targetHdg;
    short quickestTurn, turn;

    section = rescueDroid->section;
    nextSection = section->nextSection.ptr;

    target.vx = (section->centre.vx + nextSection->centre.vx) >> 1;
    target.vy = section->centre.vy - 3000;
    target.vz = (section->centre.vz + nextSection->centre.vz) >> 1;

    targetVector.vx = target.vx - rescueDroid->pos.vx;
    targetVector.vy = target.vy - rescueDroid->pos.vy;
    targetVector.vz = target.vz - rescueDroid->pos.vz;

    targetHdg = -ratan2(targetVector.vx, targetVector.vz);

    quickestTurn = targetHdg - rescueDroid->hdg;

    if (rescueDroid->hdg < 0) {
        turn = quickestTurn - 4096;
    } else {
        turn = quickestTurn + 4096;
    }

    if (abs(turn) < abs(quickestTurn)) {
        rescueDroid->vhdg = turn >> 6;
    } else {
        rescueDroid->vhdg = quickestTurn >> 6;
    }

    rescueDroid->acc.vx = (-rsin(rescueDroid->hdg) * rcos(rescueDroid->pitch)) >> 15;
    rescueDroid->acc.vy = targetVector.vy >> 6;
    rescueDroid->acc.vz = (rcos(rescueDroid->hdg) * rcos(rescueDroid->pitch)) >> 15;

    if (shipIndex->attr & RESCUE) {
        rescueDroid->droidUpdate = UpdateDroidRescue;
        rescueDroid->updateCount = 800;
        DroidSound(1);
        SetNoteVolume(droidVoice, 255);
        CameraUpdate = UpdateCameraDroidView;
        shipIndex->attr |= REMOTE_VIEW;
        if (shipIndex->nearTrkSect->flags & 1) {
            cameraPtr->section = shipIndex->nearTrkSect->nextSection.ptr;
        } else {
            cameraPtr->section = shipIndex->nearTrkSect;
        }
    }

    SetDroidVectors(rescueDroid, shipShapes);
}
#endif

/* Moves the rescue droid with the ship while towing it. */
void UpdateDroidRescue(RescueData *rescueDroid, ShipData *shipIndex, Object **shipShapes)
{
    short count;

    count = 0;
    rescueDroid->acc.vx = 0;
    rescueDroid->acc.vy = 0;
    rescueDroid->acc.vz = 0;
    rescueDroid->vel.vx = 0;
    rescueDroid->vel.vy = 0;
    rescueDroid->vel.vz = 0;

    rescueDroid->pos.vx = shipIndex->ppivot.vx;
    rescueDroid->pos.vy = shipIndex->ppivot.vy - droidFlyin;
    rescueDroid->pos.vz = shipIndex->ppivot.vz;
    rescueDroid->vhdg = 0;
    rescueDroid->hdg = shipIndex->hdg;

    if (droidFlyin > 350) {
        droidFlyin -= 50;
    }

    if (!(shipIndex->attr & RESCUE)) {
        DroidSound(0);
        rescueDroid->droidUpdate = UpdateDroidCircle;
        rescueDroid->updateCount = 800;
        shipIndex->attr &= ~REMOTE_VIEW;
        if (shipIndex->attr & VIEW_IN) {
            CameraUpdate = UpdateCameraRaceInternal;
        } else {
            CameraUpdate = UpdateCameraRaceExternal;
        }
        while (!(rescueDroid->section->flags & 1)) {
            rescueDroid->section = rescueDroid->section->prevSection.ptr;
            count++;
            if (track->sectionCount == count) {
                break;
            }
        }
    }

    SetDroidVectors(rescueDroid, shipShapes);
}

/* Integrates droid motion and updates its model transform. */
void SetDroidVectors(RescueData *d, Object **shapes)
{
    Face *face;
    VECTOR *vel;

    d->vel.vx += d->acc.vx;
    d->vel.vy += d->acc.vy;
    d->vel.vz += d->acc.vz;
    d->vel.vx -= d->vel.vx >> 3;
    d->vel.vy -= d->vel.vy >> 3;
    d->vel.vz -= d->vel.vz >> 3;
    d->pos.vx += d->vel.vx >> 6;
    d->pos.vy += d->vel.vy >> 6;
    d->pos.vz += d->vel.vz >> 6;
    d->hdg += d->vhdg;
    d->hdg = ang(d->hdg);
    SetSkeletonPosition(shapes[RescueDroidModel]->skeleton, d->pos.vx, d->pos.vy, d->pos.vz);
    SetSkeletonDirectionHPR(shapes[RescueDroidModel]->skeleton, d->hdg, d->pitch, d->roll);
    face = track->faces + d->section->faceStart;
    while (!(face->flags & TRACK_BASE)) {
        face++;
    }
}

/* Adds the rescue droid model to the render list. */
void DrawRescueDroid(RescueData *d, Object **shapes)
{
    shapes[RescueDroidModel]->skeleton->update = 1;
    AddRenderObject(shapes[RescueDroidModel], 0, 0);
}

/* Initializes the rescue droid beside the player's ship. */
void InitRescueDroid(RescueData *rescueDroid, Object **shipShapes, ShipData *shipIndex)
{
    int i = 0;

    rescueDroid->section = track->sections;
    while (1) {
        rescueDroid->section = rescueDroid->section->nextSection.ptr;
        if (rescueDroid->section->flags & 1) {
            break;
        }
        i++;
        if (i > track->sectionCount) {
            break;
        }
    }
    rescueDroid->pos.vx = shipIndex->ppivot.vx;
    rescueDroid->pos.vy = shipIndex->ppivot.vy - 200;
    rescueDroid->pos.vz = shipIndex->ppivot.vz;
    rescueDroid->vel.vx = 0;
    rescueDroid->vel.vy = 0;
    rescueDroid->vel.vz = 0;
    rescueDroid->acc.vx = 0;
    rescueDroid->acc.vy = 0;
    rescueDroid->acc.vz = 0;
    rescueDroid->hdg = shipIndex->hdg;
    rescueDroid->pitch = shipIndex->pitch;
    rescueDroid->roll = shipIndex->roll;
    rescueDroid->vhdg = 0;
    rescueDroid->vpitch = 0;
    rescueDroid->vroll = 0;
    rescueDroid->updateCount = 800;
    rescueDroid->modelID = RescueDroidModel;
    rescueDroid->hdg = 0;
    rescueDroid->pitch = 0;
    rescueDroid->roll = 0;
    rescueDroid->droidUpdate = UpdateDroidStart;
}
