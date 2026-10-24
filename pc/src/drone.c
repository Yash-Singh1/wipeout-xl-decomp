#include "functions.h"
#include "input_state.h"
#include "mathinline.h"

#pragma intrinsic(sin)
#ifdef NETPLAY
#pragma intrinsic(abs)
#endif

extern TrackData *track;
extern VECTOR racingLine[];
extern RaceLine raceLines[];
extern short activeShipCount;
extern short numTrackSections;
extern short ownShip;
extern int shipDamage[];

#ifdef NETPLAY
extern const double netFixedPointScale;
extern const double netAngleScale;
#endif

/* Per ship class (indexed by shipClassIndex) speed limits and tuning for the drones. */
short classSpeedLimit[4] = {16000, 18000, 20000, 22000};
short classSpeedMargin[4] = {1200, 1600, 2100, 2600};
short classAccel[4] = {100, 120, 140, 160};
short classSteer[4] = {1200, 1600, 2100, 2200};

/* Track sections holding the pit lane junction, per track, and the direction the pit lane branches. */
short pitLaneSection1[8] = {257, 300, 446, 358, 16, 362, 312, 68};
short pitLaneSection2[8] = {290, 325, 468, 378, 57, 379, 337, 88};
int pitLaneDirection[8] = {JUNC_LEFT, JUNC_LEFT, JUNC_LEFT, JUNC_LEFT, JUNC_LEFT, JUNC_LEFT, JUNC_LEFT, JUNC_LEFT};

/* A new drone is lifted to its flying height over its first 200 frames, then starts steering. */
void UpdateDroneShipStart(ShipData *ship, void *unused, short shipNo)
{
    ship += shipNo;
    ship->prevShipSect = ship->nearTrkSect;
    SectionSearch(ship, shipNo);
    if (ship->updateCount == 200) {
        ship->startY = ship->ppivot.vy;
    }
#ifdef NETPLAY
    /* Compiled-out debug block of the NET build, kept as written: /Od turns it into a jump. */
    if (0) {
    }
    ship->ppivot.vy = ship->startY + ((long)(sin((ship->updateCount * ((shipNo * 3) + 80) / netFixedPointScale) * netAngleScale) * netFixedPointScale) >> 7);
#else
    ship->ppivot.vy = ship->startY + (rsin(ship->updateCount * ((shipNo * 3) + 80)) >> 7);
#endif
    ship->updateCount--;
    if (ship->updateCount == 0) {
        ship->update = UpdateDroneShip;
    }
}

#ifdef EQUIV
/* Steers a computer-controlled ship round the track (or over it when it is flying), lets it
   speed up, fire at the human player and take junctions, then integrates its motion. */
/* The executable reloads ship fields across calls. */
#pragma optimize("a", off)
#pragma optimize("w", on)
#ifdef NETPLAY
/* Equivalent with one reversed short-addition load pair. The +0 index and product
   spellings preserve the other original /Od operand load orders. */
void UpdateDroneShip(ShipData *ships, Object **shipShapes, short shipNo, WeaponData *weaponIndex)
{
    char          humanSlot;
    short         boostStep;
    int           magnitude;
    unsigned char weaponId;
    short         zoneCode;
    short         speedBonusAccel[4];
    TrackSection *nearSectionPtr;
    float         flatFloorLen;
    short         distanceAhead;
    Face         *faceCurCursor;
    short         shipAhead;
    short         steer;
    VECTOR        bestVector;
    short         curHumanRaceDistRef;
    int           alignDot;
    int           alphaScale;
    TrackSection *nextSection;
    short         raceDistSec;
    TrackSection *trackSection;
    VECTOR        offset;
    short         raceDiffValue;
    short         lateralDisp;
    ShipData     *shipPointer;
    short         aheadDifference;
    short         isBlocked;
    short         decideFlip;
    short         i;
    Vector        path1;
    short         alongPlaneDist;
    VECTOR        targetPoint;
    Vector        path2;
    VECTOR        directionData;
    ShipData     *humanShipPtr;
    VECTOR        facePoint;
    short         humanMovedDist;
    speedBonusAccel[0] = 80;
    speedBonusAccel[1] = 140;
    speedBonusAccel[2] = 200;
    speedBonusAccel[3] = 210;

    isBlocked = 0;
    offset.vx = 0;
    offset.vy = 0;
    offset.vz = 0;
    humanSlot = ownShip;
    humanShipPtr = ships + humanSlot;
    shipPointer = ships + shipNo;
    /* Launch acceleration while boostCount counts down, afterwards the speed creeps up to its limit. */
    if (shipPointer->boostCount > 0) {
        shipPointer->boostCount--;
        shipPointer->updateCount = 0;
        if ((shipPointer->combatAttr & 0x10) || ((shipNo + 0) == ownShip)) {
            if ((shipPointer->remoteMax + ((classSpeedMargin[shipClassIndex] * 32) / 64)) >= shipPointer->speed) {
                shipPointer->speed = shipPointer->speed + (shipPointer->remoteMagnitude + classAccel[shipClassIndex]);
            }
        } else if (shipPointer->combatAttr & 0x20) {
            if ((shipPointer->remoteMax + ((classSpeedMargin[shipClassIndex] * 32) / 35)) >= shipPointer->speed) {
                shipPointer->speed = shipPointer->speed + (shipPointer->remoteMagnitude + classAccel[shipClassIndex]);
            }
        } else if (shipPointer->combatAttr & 0x40) {
            if ((shipPointer->remoteMax + ((classSpeedMargin[shipClassIndex] * 32) / 27)) >= shipPointer->speed) {
                shipPointer->speed = shipPointer->speed + (shipPointer->remoteMagnitude + classAccel[shipClassIndex]);
            }
        }
    } else if (shipPointer->autopilotCount == 0) {
        if (shipPointer->speed <= shipPointer->remoteMax) {
            shipPointer->speed = shipPointer->speed + shipPointer->remoteMagnitude;
        }
    } else {
        if ((shipPointer->remoteMax * 2) >= shipPointer->speed) {
            shipPointer->speed = shipPointer->speed + (shipPointer->remoteMagnitude * 3);
        }
    }
    if (!(shipPointer->attr & FLYING)) {
        /* Steer along the track base face; overtake a ship in front if it is close and slower. */
        faceCurCursor = track->faces + shipPointer->nearTrkSect->faceStart;
        while (!(faceCurCursor->flags & TRACK_BASE)) {
            faceCurCursor++;
        }
        raceDistSec = raceLines[ships[shipNo].nearSecNo].distance;
        raceDiffValue = raceDistSec - raceLines[ships[humanSlot].nearSecNo].distance;
        if (raceDiffValue < 0) {
            raceDiffValue += numTrackSections;
        }
        shipAhead = getShipInfrontLAP(ships, shipNo);
        if (shipAhead != -1) {
            distanceAhead = raceLines[ships[shipAhead].nearSecNo].distance;
            aheadDifference = distanceAhead - raceDistSec;
            if (aheadDifference < 0) {
                aheadDifference += numTrackSections;
            }
            if ((aheadDifference < 32) && (ships[shipNo].moveDist > ships[shipAhead + 0].moveDist)) {
                lookForWayPast(&offset, ships, shipNo, shipAhead);
            } else {
                followRaceLine(&offset, ships, faceCurCursor, shipNo);
            }
        } else {
            followRaceLine(&offset, ships, faceCurCursor, shipNo);
        }
        /* Zone: how far ahead (+) or behind (-) the human ship is, in sections; 100 means out of range. */
        if (shipPointer->autopilotCount == 0) {
            if (numTrackSections / 2 < raceDiffValue) {
                raceDiffValue -= numTrackSections;
            }
            zoneCode = 100;
            if ((raceDiffValue < 84) && (raceDiffValue > -80)) {
                if ((raceDiffValue <= 80) && (raceDiffValue > 40)) {
                    zoneCode = 3;
                }
                if ((raceDiffValue <= 40) && (raceDiffValue > 20)) {
                    zoneCode = 2;
                }
                if ((raceDiffValue <= 20) && (raceDiffValue > 12)) {
                    zoneCode = 1;
                }
                if ((raceDiffValue <= 12) && (raceDiffValue >= -8)) {
                    zoneCode = 0;
                }
                if ((raceDiffValue < -8) && (raceDiffValue >= -20)) {
                    zoneCode = -1;
                }
                if ((raceDiffValue < -20) && (raceDiffValue >= -40)) {
                    zoneCode = -2;
                }
                if ((raceDiffValue < -40) && (raceDiffValue >= -80)) {
                    zoneCode = -3;
                }
                humanMovedDist = ships[humanSlot].moveDist;
                steer = classSteer[shipClassIndex];
                boostStep = steer / 8;
                if (shipPointer->boostCount == 0) {
                    if ((getShipBehindLAP(ships, shipNo) == humanSlot) && (activeShipCount > 3)) {
                        shipAhead = getShipInfrontLAP(ships, shipNo);
                        shipAhead = getShipInfrontLAP(ships, shipAhead);
                        curHumanRaceDistRef = raceLines[ships[humanSlot].nearSecNo].distance;
                        distanceAhead = raceLines[ships[shipAhead].nearSecNo].distance;
                        aheadDifference = distanceAhead - curHumanRaceDistRef;
                        if (aheadDifference < 0) {
                            aheadDifference += numTrackSections;
                        }
                        if ((aheadDifference > 0) && (aheadDifference < 40)) {
                            isBlocked = 1;
                        }
                    }
                }
                if (isBlocked == 0) {
                    if (ships[shipNo].moveDist < humanMovedDist) {
                        if ((zoneCode == 2) || (zoneCode == -2)) {
                            if (classSpeedLimit[shipClassIndex + 0] / 2 < ships[humanSlot].speed) {
                                if (shipPointer->combatAttr & 0x20) {
                                    if (shipPointer->speed <= (shipPointer->remoteMax + (steer / 4))) {
                                        shipPointer->speed = shipPointer->speed + (shipPointer->remoteMagnitude + (boostStep / 4));
                                    }
                                } else if (shipPointer->combatAttr & 0x40) {
                                    if (shipPointer->speed <= (shipPointer->remoteMax + (steer / 4))) {
                                        shipPointer->speed = shipPointer->speed + (shipPointer->remoteMagnitude + (boostStep / 4));
                                    }
                                }
                            }
                        }
                    }
                    if ((ships[shipNo].moveDist < humanMovedDist) && ((zoneCode == 1) || (zoneCode == 0) || (zoneCode == -1))) {
                        if (classSpeedLimit[shipClassIndex + 0] / 2 < ships[humanSlot].speed) {
                            if (shipPointer->combatAttr & 0x20) {
                                if (shipPointer->speed <= (shipPointer->remoteMax + (steer / 2))) {
                                    shipPointer->speed = shipPointer->speed + (shipPointer->remoteMagnitude + (boostStep / 2));
                                }
                            } else if (shipPointer->combatAttr & 0x40) {
                                if (shipPointer->speed <= (shipPointer->remoteMax + (steer / 2))) {
                                    shipPointer->speed = shipPointer->speed + (shipPointer->remoteMagnitude + (boostStep / 2));
                                }
                            }
                        }
                    }
                }
            }
            /* Fire the drone's weapon according to its temperament and the zone. */
            weaponId = shipPointer->weapon;
            if ((weaponId != 0) && (zoneCode != 100) && !(ships[humanSlot].combatAttr & DESTROYED)) {
                if (shipPointer->combatAttr & 0x20) {
                    if ((zoneCode == -3) && ((weaponId == 1) || (weaponId == 2) || (weaponId == 5))) {
                        if (weaponId == 1) PlayNote(22, 510, 15, 15, 256);
                        if (weaponId == 2) PlayNote(21, 510, 15, 15, 256);
                        if (weaponId == 5) PlayNote(24, 510, 15, 15, 256);
                        shipPointer->targetShip = -1;
                        FireWeapon(shipPointer, weaponIndex, shipNo, shipPointer->weapon);
                    } else if (zoneCode == 1) {
                        if (weaponId == 3) {
                            FireWeapon(shipPointer, weaponIndex, shipNo, shipPointer->weapon);
                            PlayNote(20, 470, 15, 15, 256);
                        } else if (weaponId == 2) {
                            shipPointer->targetShip = humanSlot;
                            FireWeapon(shipPointer, weaponIndex, shipNo, shipPointer->weapon);
                            PlayNote(21, 510, 15, 15, 256);
                        }
                    }
                } else if (shipPointer->combatAttr & 0x40) {
                    if (((zoneCode == -3) || (zoneCode == -2)) && ((weaponId == 1) || (weaponId == 2) || (weaponId == 5))) {
                        shipPointer->targetShip = humanSlot;
                        FireWeapon(shipPointer, weaponIndex, shipNo, shipPointer->weapon);
                        if (weaponId == 1) PlayNote(22, 510, 15, 15, 256);
                        if (weaponId == 2) PlayNote(21, 510, 15, 15, 256);
                        if (weaponId == 5) PlayNote(24, 510, 15, 15, 256);
                    }
                    if (zoneCode == 1) {
                        if (weaponId == 3) {
                            FireWeapon(shipPointer, weaponIndex, shipNo, shipPointer->weapon);
                            PlayNote(20, 470, 15, 15, 256);
                        } else if (weaponId == 2) {
                            shipPointer->targetShip = humanSlot;
                            FireWeapon(shipPointer, weaponIndex, shipNo, shipPointer->weapon);
                            PlayNote(21, 510, 15, 15, 256);
                        }
                    }
                }
                if ((weaponId == 8) && (zoneCode >= 1) && (zoneCode <= 3)) {
                    FireWeapon(shipPointer, weaponIndex, shipNo, shipPointer->weapon);
                }
            }
        }
        /* Pick the branch to take at a junction: pit lane when due, otherwise at random. */
        trackSection = shipPointer->nearTrkSect->prevSection.ptr;
        for (i = 0; i < 4; i++) {
            trackSection = trackSection->nextSection.ptr;
        }
        if ((trackSection->junction.ptr != (TrackSection *)-1) && (trackSection->junction.ptr->flags & 0x10)) {
            if ((trackSection->secNo == pitLaneSection1[clipLevel]) || (trackSection->secNo == pitLaneSection2[clipLevel])) {
                shipPointer->attr &= ~JUNC_LEFT;
                if (shipDamage[shipNo] > 0) {
                    shipPointer->attr |= pitLaneDirection[clipLevel];
                } else {
                    shipPointer->attr |= pitLaneDirection[clipLevel] ^ JUNC_LEFT;
                }
            } else {
                decideFlip = newRand() % 2;
                if (decideFlip == 0) {
                    shipPointer->attr |= JUNC_LEFT;
                } else {
                    shipPointer->attr &= ~JUNC_LEFT;
                }
            }
        }
        /* Look four sections ahead, following the left junction when the ship is turning left. */
        trackSection = shipPointer->nearTrkSect->prevSection.ptr;
        for (i = 0; i < 4; i++) {
            if (trackSection->junction.ptr != (TrackSection *)-1) {
                if (trackSection->junction.ptr->flags & 0x10) {
                    if (shipPointer->attr & JUNC_LEFT) {
                        trackSection = trackSection->junction.ptr;
                    } else {
                        trackSection = trackSection->nextSection.ptr;
                    }
                } else {
                    trackSection = trackSection->nextSection.ptr;
                }
            } else {
                trackSection = trackSection->nextSection.ptr;
            }
        }
        /* Target velocity along the track, bled off by turning, then the pull towards the racing line. */
        nextSection = trackSection->nextSection.ptr;
        trackSection = shipPointer->nearTrkSect;
        shipPointer->speed -= abs(shipPointer->speed * shipPointer->vhdg) >> 14;
        shipPointer->speed -= abs(shipPointer->speed * shipPointer->vpitch) >> 14;
        if ((shipPointer->nearTrkSect->flags & 1) || (shipPointer->nearTrkSect->nextSection.ptr->flags & 1)
            || (shipPointer->nearTrkSect->prevSection.ptr->flags & 1)
            || (shipPointer->nearTrkSect->nextSection.ptr->nextSection.ptr->flags & 1)
            || (shipPointer->nearTrkSect->nextSection.ptr->nextSection.ptr->nextSection.ptr->flags & 1)) {
            targetPoint.vx = shipPointer->nearTrkSect->centre.vx - shipPointer->nearTrkSect->prevSection.ptr->centre.vx;
            targetPoint.vy = shipPointer->nearTrkSect->centre.vy - shipPointer->nearTrkSect->prevSection.ptr->centre.vy;
            targetPoint.vz = shipPointer->nearTrkSect->centre.vz - shipPointer->nearTrkSect->prevSection.ptr->centre.vz;
        } else {
            targetPoint.vx = nextSection->centre.vx - trackSection->centre.vx;
            targetPoint.vy = nextSection->centre.vy - trackSection->centre.vy;
            targetPoint.vz = nextSection->centre.vz - trackSection->centre.vz;
        }
        magnitude = GetMagnitude(targetPoint.vx, targetPoint.vy, targetPoint.vz);
        targetPoint.vx = (targetPoint.vx * shipPointer->speed) / magnitude;
        targetPoint.vy = (targetPoint.vy * shipPointer->speed) / magnitude;
        targetPoint.vz = (targetPoint.vz * shipPointer->speed) / magnitude;
        path1.vx = trackSection->centre.vx + offset.vx;
        path1.vy = trackSection->centre.vy + offset.vy;
        path1.vz = trackSection->centre.vz + offset.vz;
        path2.vx = nextSection->centre.vx + offset.vx;
        path2.vy = nextSection->centre.vy + offset.vy;
        path2.vz = nextSection->centre.vz + offset.vz;
        GetTargetPos(&bestVector, shipPointer->ppivot, path2, path1);
        shipPointer->apivot.vx = targetPoint.vx + (bestVector.vx - shipPointer->ppivot.vx);
        shipPointer->apivot.vy = targetPoint.vy + (bestVector.vy - shipPointer->ppivot.vy);
        shipPointer->apivot.vz = targetPoint.vz + (bestVector.vz - shipPointer->ppivot.vz);
        shipPointer->vpivot.vx = shipPointer->vpivot.vx + shipPointer->apivot.vx;
        shipPointer->vpivot.vy = shipPointer->vpivot.vy + shipPointer->apivot.vy;
        shipPointer->vpivot.vz = shipPointer->vpivot.vz + shipPointer->apivot.vz;
        /* Speed-up pads boost the drone when it drives over them on the matching side. */
        calcMyXPos(ships, shipNo, &lateralDisp, &alongPlaneDist);
        if ((faceCurCursor->flags & SPEED_UP) && (lateralDisp < 0)) {
            shipPointer->speed = shipPointer->speed + speedBonusAccel[(unsigned short)shipClassIndex];
        }
        faceCurCursor++;
        if ((faceCurCursor->flags & SPEED_UP) && (lateralDisp > 0)) {
            shipPointer->speed = shipPointer->speed + speedBonusAccel[(unsigned short)shipClassIndex];
        }
        facePoint.vx = track->vertices[faceCurCursor->vertex[0]].vx;
        facePoint.vy = track->vertices[faceCurCursor->vertex[0]].vy;
        facePoint.vz = track->vertices[faceCurCursor->vertex[0]].vz;
        alphaScale = PlaneLineCollision(facePoint, shipPointer->ppivot, faceCurCursor->normal);
        if (alphaScale < 50) {
            alphaScale = 50;
        }
        shipPointer->apivot.vx = ((faceCurCursor->normal.vx * 256 * 64) / alphaScale - (faceCurCursor->normal.vx * 64)) >> 8;
        shipPointer->apivot.vy = ((faceCurCursor->normal.vy * 256 * 64) / alphaScale - (faceCurCursor->normal.vy * 64)) >> 8;
        shipPointer->apivot.vz = ((faceCurCursor->normal.vz * 256 * 64) / alphaScale - (faceCurCursor->normal.vz * 64)) >> 8;
        shipPointer->vpivot.vx += shipPointer->apivot.vx;
        shipPointer->vpivot.vy += shipPointer->apivot.vy;
        shipPointer->vpivot.vz += shipPointer->apivot.vz;
        /* Heading, pitch and roll follow the target direction. */
        flatFloorLen = sqrt(targetPoint.vx * targetPoint.vx + targetPoint.vz * targetPoint.vz);
        shipPointer->vhdg = -ratan2(targetPoint.vx, targetPoint.vz) - shipPointer->hdg;
        shipPointer->vhdg += shipPointer->ahdg;
        shipPointer->ahdg -= shipPointer->ahdg >> 3;
        if (shipPointer->vhdg > 2048) {
            shipPointer->vhdg = shipPointer->vhdg - 4096;
        } else if (shipPointer->vhdg < -2048) {
            shipPointer->vhdg = shipPointer->vhdg + 4096;
        }
        shipPointer->hdg += shipPointer->vhdg >> 4;
        shipPointer->vpitch = -ratan2(targetPoint.vy, flatFloorLen) - shipPointer->pitch;
        shipPointer->pitch += shipPointer->vpitch >> 4;
        shipPointer->vroll += shipPointer->vhdg / 4;
        shipPointer->vroll -= shipPointer->vroll >> 1;
        shipPointer->roll += shipPointer->vroll;
        shipPointer->roll -= shipPointer->roll >> 3;
        facePoint.vx = track->vertices[faceCurCursor->vertex[0]].vx;
        facePoint.vy = track->vertices[faceCurCursor->vertex[0]].vy;
        facePoint.vz = track->vertices[faceCurCursor->vertex[0]].vz;
        if (shipPointer->autopilotCount > 0) {
            alphaScale = PlaneLineCollision(facePoint, shipPointer->ppivot, faceCurCursor->normal);
            if (alphaScale < -300) {
                alphaScale = -300;
            }
            DroneShip2TrackBase(shipPointer, faceCurCursor, alphaScale);
            nearSectionPtr = shipPointer->nearTrkSect->nextSection.ptr;
            directionData.vx = nearSectionPtr->centre.vx - shipPointer->nearTrkSect->centre.vx;
            directionData.vy = nearSectionPtr->centre.vy - shipPointer->nearTrkSect->centre.vy;
            directionData.vz = nearSectionPtr->centre.vz - shipPointer->nearTrkSect->centre.vz;
            alignDot = ((directionData.vy * (shipPointer->unitVecNose.vy + 0)) + directionData.vz * shipPointer->unitVecNose.vz) + directionData.vx * shipPointer->unitVecNose.vx;
            if (alignDot < 0) {
                shipPointer->attr &= ~DIRECTION;
            } else {
                shipPointer->attr |= DIRECTION;
            }
        }
    } else {
        trackSection = shipPointer->nearTrkSect;
        trackSection = trackSection->nextSection.ptr;
        trackSection = trackSection->nextSection.ptr;
        nextSection = trackSection->nextSection.ptr;
        moveDownCentre(&offset, humanShipPtr, faceCurCursor, shipNo);
        shipPointer->speed -= abs(shipPointer->speed * shipPointer->vhdg) >> 14;
        targetPoint.vx = nextSection->centre.vx - trackSection->centre.vx;
        targetPoint.vy = nextSection->centre.vy - trackSection->centre.vy;
        targetPoint.vz = nextSection->centre.vz - trackSection->centre.vz;
        magnitude = GetMagnitude(targetPoint.vx, targetPoint.vy, targetPoint.vz);
        targetPoint.vx = (targetPoint.vx * shipPointer->speed) / magnitude;
        targetPoint.vz = (targetPoint.vz * shipPointer->speed) / magnitude;
        targetPoint.vy = 500;
        GetTargetPos(&bestVector, shipPointer->ppivot, nextSection->centre, shipPointer->nearTrkSect->centre);
        shipPointer->apivot.vx = targetPoint.vx + ((bestVector.vx - shipPointer->ppivot.vx) >> 1);
        shipPointer->apivot.vy = targetPoint.vy;
        shipPointer->apivot.vz = targetPoint.vz + ((bestVector.vz - shipPointer->ppivot.vz) >> 1);
        shipPointer->vpivot.vx += shipPointer->apivot.vx;
        shipPointer->vpivot.vy += shipPointer->apivot.vy;
        shipPointer->vpivot.vz += shipPointer->apivot.vz;
        shipPointer->vpitch = -200 - shipPointer->pitch;
        shipPointer->pitch += shipPointer->vpitch >> 4;
        shipPointer->vhdg = -ratan2(targetPoint.vx, targetPoint.vz) - shipPointer->hdg;
        if (shipPointer->vhdg > 2048) {
            shipPointer->vhdg = shipPointer->vhdg - 4096;
        } else if (shipPointer->vhdg < -2048) {
            shipPointer->vhdg = shipPointer->vhdg + 4096;
        }
        shipPointer->hdg += shipPointer->vhdg >> 4;
    }
    shipPointer->vpivot.vx -= shipPointer->vpivot.vx >> 3;
    shipPointer->vpivot.vy -= shipPointer->vpivot.vy >> 3;
    shipPointer->vpivot.vz -= shipPointer->vpivot.vz >> 3;
    shipPointer->ppivot.vx += shipPointer->vpivot.vx >> 6;
    shipPointer->ppivot.vy += shipPointer->vpivot.vy >> 6;
    shipPointer->ppivot.vz += shipPointer->vpivot.vz >> 6;
    if (shipPointer->attr & ELECTROED) {
        shipPointer->ppivot.vx += (newRand() - 0x4000) % 20;
        shipPointer->ppivot.vy += (newRand() - 0x4000) % 20;
        shipPointer->ppivot.vz += (newRand() - 0x4000) % 20;
        shipPointer->electroCount--;
        if (shipPointer->electroCount <= 0) {
            shipPointer->electroCount = 0;
            shipPointer->attr &= ~ELECTROED;
        }
        if ((newRand() % 10) == 0) {
            shipPointer->speed -= shipPointer->speed >> 1;
        }
    }
    shipPointer->hdg = ang(shipPointer->hdg);
    shipPointer->pitch = ang(shipPointer->pitch);
    shipPointer->roll = ang(shipPointer->roll);
    if (shipPointer->autopilotCount > 0) {
        if (padPressed & 0x8000) {
            shipPointer->update = UpdatePlayerShip;
            shipPointer->vhdg = 0;
            shipPointer->autopilotCount = 0;
            if ((shipNo + 0) == ownShip) {
                PlayNote(46, 255, 15, 15, 256);
            }
        } else {
            shipPointer->autopilotCount--;
            if (shipPointer->autopilotCount == 0) {
                if ((shipPointer->roll > -50) && (shipPointer->roll < 50)) {
                    shipPointer->update = UpdatePlayerShip;
                    shipPointer->vhdg = 0;
                    return;
                }
                shipPointer->autopilotCount = 1;
            }
        }
    }
}
#else
void UpdateDroneShip(ShipData *ships, Object **shipShapes, short shipNo, WeaponData *weaponIndex)
{
    TrackSection *section;
    short         decide;
    short         front2;
    Vector        path1;
    char          humanNo;
    short         lateral;
    short         ahead;
    short         front;
    VECTOR        facePoint;
    short         i;
    ShipData     *humanShip;
    TrackSection *nextSection;
    short         raceDist;
    short         steer;
    short         aheadDiff;
    ShipData     *ship;
    ShipData     *aheadShip;
    short         humanMoveDist;
    short         boostStep;
    short         blocked;
    short         along;
    VECTOR        targetVector;
    short         raceDiff;
    short         weapon;
    short         zone;
    Vector        path2;
    int           magnitude;
    float         flat;
    int           alpha;
    Face         *face;
    VECTOR        bestPath;
    {
        VECTOR offset;
        short speedBonus[4];
        VECTOR direction;
        speedBonus[0] = 80;
        speedBonus[1] = 140;
        speedBonus[2] = 200;
        speedBonus[3] = 210;

        blocked = 0;
        moveDownCentre(&offset, ships, (Face *)0, shipNo);
        humanNo = ownShip;
        ship = ships + shipNo;
        /* Launch acceleration while boostCount counts down, afterwards the speed creeps up to its limit. */
        if (ship->boostCount > 0) {
            ship->boostCount--;
            ship->updateCount = 0;
            if ((ship->combatAttr & 0x10) || (shipNo == ownShip)) {
                if ((ship->remoteMax + ((classSpeedMargin[shipClassIndex] * 32) / 64)) >= ship->speed) {
                    ship->speed = ship->speed + (ship->remoteMagnitude + classAccel[shipClassIndex]);
                }
            } else if (ship->combatAttr & 0x20) {
                if ((ship->remoteMax + ((classSpeedMargin[shipClassIndex] * 32) / 35)) >= ship->speed) {
                    ship->speed = ship->speed + (ship->remoteMagnitude + classAccel[shipClassIndex]);
                }
            } else if (ship->combatAttr & 0x40) {
                if ((ship->remoteMax + ((classSpeedMargin[shipClassIndex] * 32) / 27)) >= ship->speed) {
                    ship->speed = ship->speed + (ship->remoteMagnitude + classAccel[shipClassIndex]);
                }
            }
        } else if (ship->autopilotCount == 0) {
            if (ship->speed <= ship->remoteMax) {
                ship->speed = ship->speed + ship->remoteMagnitude;
            }
        } else {
            if ((ship->remoteMax * 2) >= ship->speed) {
                ship->speed = ship->speed + (ship->remoteMagnitude * 3);
            }
        }
        if (!(ship->attr & FLYING)) {
            /* Steer along the track base face; overtake a ship in front if it is close and slower. */
            face = track->faces + ship->nearTrkSect->faceStart;
            if (!(face->flags & TRACK_BASE)) {
                do {
                    face++;
                } while (!(face->flags & TRACK_BASE));
            }
            raceDist = raceLines[ship->nearSecNo].distance;
            humanShip = ships + humanNo;
            raceDiff = raceDist - raceLines[humanShip->nearSecNo].distance;
            if (raceDiff < 0) {
                raceDiff += numTrackSections;
            }
            ahead = getShipInfrontLAP(ships, shipNo);
            /* 32 keeps the no-ship case out of the branch that reads aheadShip. */
            aheadDiff = 32;
            if (ahead != -1) {
                aheadShip = &ships[ahead];
                aheadDiff = raceLines[aheadShip->nearSecNo].distance - raceDist;
                if (aheadDiff < 0) {
                    aheadDiff += numTrackSections;
                }
            }
            if ((aheadDiff < 32) && (ship->moveDist > aheadShip->moveDist)) {
                lookForWayPast(&offset, ships, shipNo, ahead);
            } else {
                followRaceLine(&offset, ships, face, shipNo);
            }
            /* Zone: how far ahead (+) or behind (-) the human ship is, in sections; 100 means out of range. */
            if (ship->autopilotCount == 0) {
                if (numTrackSections / 2 < raceDiff) {
                    raceDiff -= numTrackSections;
                }
                zone = 100;
                if ((raceDiff < 84) && (raceDiff > -80)) {
                    if ((raceDiff <= 80) && (raceDiff > 40)) {
                        zone = 3;
                    }
                    if ((raceDiff <= 40) && (raceDiff > 20)) {
                        zone = 2;
                    }
                    if ((raceDiff <= 20) && (raceDiff > 12)) {
                        zone = 1;
                    }
                    if ((raceDiff <= 12) && (raceDiff >= -8)) {
                        zone = 0;
                    }
                    if ((raceDiff < -8) && (raceDiff >= -20)) {
                        zone = -1;
                    }
                    if ((raceDiff < -20) && (raceDiff >= -40)) {
                        zone = -2;
                    }
                    if ((raceDiff < -40) && (raceDiff >= -80)) {
                        zone = -3;
                    }
                    humanMoveDist = humanShip->moveDist;
                    steer = classSteer[shipClassIndex];
                    boostStep = steer / 8;
                    if (ship->boostCount == 0) {
                        if ((getShipBehindLAP(ships, shipNo) == humanNo) && (activeShipCount > 3)) {
                            front = getShipInfrontLAP(ships, shipNo);
                            front2 = getShipInfrontLAP(ships, front);
                            aheadDiff = raceLines[ships[front2].nearSecNo].distance - raceLines[humanShip->nearSecNo].distance;
                            if (aheadDiff < 0) {
                                aheadDiff += numTrackSections;
                            }
                            if ((aheadDiff > 0) && (aheadDiff < 40)) {
                                blocked = 1;
                            }
                        }
                    }
                    if ((blocked == 0) && (ship->moveDist < humanMoveDist)) {
                        if ((zone == 2) || (zone == -2)) {
                            if (classSpeedLimit[shipClassIndex] / 2 < humanShip->speed) {
                                if (ship->combatAttr & 0x20) {
                                    if (ship->speed <= (ship->remoteMax + (steer / 4))) {
                                        ship->speed = ship->speed + (ship->remoteMagnitude + (boostStep / 4));
                                    }
                                } else if (ship->combatAttr & 0x40) {
                                    if (ship->speed <= (ship->remoteMax + (steer / 4))) {
                                        ship->speed = ship->speed + (ship->remoteMagnitude + (boostStep / 4));
                                    }
                                }
                            }
                        }
                        if ((ship->moveDist < humanMoveDist) && ((zone == 1) || (zone == 0) || (zone == -1))) {
                            if (classSpeedLimit[shipClassIndex] / 2 < humanShip->speed) {
                                if (ship->combatAttr & 0x20) {
                                    if (ship->speed <= (ship->remoteMax + (steer / 2))) {
                                        ship->speed = ship->speed + (ship->remoteMagnitude + (boostStep / 2));
                                    }
                                } else if (ship->combatAttr & 0x40) {
                                    if (ship->speed <= (ship->remoteMax + (steer / 2))) {
                                        ship->speed = ship->speed + (ship->remoteMagnitude + (boostStep / 2));
                                    }
                                }
                            }
                        }
                    }
                }
                /* Fire the drone's weapon according to its temperament and the zone. */
                weapon = ship->weapon;
                if ((weapon != 0) && (zone != 100) && !(humanShip->combatAttr & DESTROYED)) {
                    if (ship->combatAttr & 0x20) {
                        if ((zone == -3) && ((weapon == 1) || (weapon == 2) || (weapon == 5))) {
                            if (weapon == 1) PlayNote(22, 510, 15, 15, 256);
                            if (weapon == 2) PlayNote(21, 510, 15, 15, 256);
                            if (weapon == 5) PlayNote(24, 510, 15, 15, 256);
                            ship->targetShip = -1;
                            FireWeapon(ship, weaponIndex, shipNo, ship->weapon);
                        } else if (zone == 1) {
                            if (weapon == 3) {
                                FireWeapon(ship, weaponIndex, shipNo, weapon);
                                PlayNote(20, 470, 15, 15, 256);
                            } else if (weapon == 2) {
                                ship->targetShip = humanNo;
                                FireWeapon(ship, weaponIndex, shipNo, weapon);
                                PlayNote(21, 510, 15, 15, 256);
                            }
                        }
                    } else if (ship->combatAttr & 0x40) {
                        if (((zone == -3) || (zone == -2)) && ((weapon == 1) || (weapon == 2) || (weapon == 5))) {
                            ship->targetShip = humanNo;
                            FireWeapon(ship, weaponIndex, shipNo, weapon);
                            if (weapon == 1) PlayNote(22, 510, 15, 15, 256);
                            if (weapon == 2) PlayNote(21, 510, 15, 15, 256);
                            if (weapon == 5) PlayNote(24, 510, 15, 15, 256);
                        }
                        if (zone == 1) {
                            if (weapon == 3) {
                                FireWeapon(ship, weaponIndex, shipNo, ship->weapon);
                                PlayNote(20, 470, 15, 15, 256);
                            } else if (weapon == 2) {
                                ship->targetShip = humanNo;
                                FireWeapon(ship, weaponIndex, shipNo, ship->weapon);
                                PlayNote(21, 510, 15, 15, 256);
                            }
                        }
                    }
                    if ((weapon == 8) && (zone >= 1) && (zone <= 3)) {
                        FireWeapon(ship, weaponIndex, shipNo, ship->weapon);
                    }
                }
            }
            /* Pick the branch to take at a junction: pit lane when due, otherwise at random. */
            section = ship->nearTrkSect->prevSection.ptr;
            for (i = 0; i < 4; i++) {
                section = section->nextSection.ptr;
            }
            if ((section->junction.ptr != (TrackSection *)-1) && (section->junction.ptr->flags & 0x10)) {
                if ((section->secNo == pitLaneSection1[clipLevel]) || (section->secNo == pitLaneSection2[clipLevel])) {
                    int damage = shipDamage[shipNo];

                    ship->attr &= ~JUNC_LEFT;
                    if (damage > 0) {
                        ship->attr |= pitLaneDirection[clipLevel];
                    } else {
                        ship->attr |= pitLaneDirection[clipLevel] ^ JUNC_LEFT;
                    }
                } else {
                    decide = newRand() % 2;
                    if (decide == 0) {
                        ship->attr |= JUNC_LEFT;
                    } else {
                        ship->attr &= ~JUNC_LEFT;
                    }
                }
            }
            /* Keep the current section as the base while the lookahead follows junctions. */
            section = ship->nearTrkSect;
            nextSection = section->prevSection.ptr;
            for (i = 0; i < 4; i++) {
                if ((nextSection->junction.ptr != (TrackSection *)-1) && (nextSection->junction.ptr->flags & 0x10)
                    && (ship->attr & JUNC_LEFT)) {
                    nextSection = nextSection->junction.ptr;
                } else {
                    nextSection = nextSection->nextSection.ptr;
                }
            }
            /* Target velocity along the track, bled off by turning, then the pull towards the racing line. */
            nextSection = nextSection->nextSection.ptr;
            ship->speed -= abs(ship->speed * ship->vhdg) >> 14;
            ship->speed -= abs(ship->speed * ship->vpitch) >> 14;
            if ((section->flags & 1) || (section->nextSection.ptr->flags & 1)
                || (section->prevSection.ptr->flags & 1)
                || (section->nextSection.ptr->nextSection.ptr->flags & 1)
                || (section->nextSection.ptr->nextSection.ptr->nextSection.ptr->flags & 1)) {
                targetVector.vx = section->centre.vx - section->prevSection.ptr->centre.vx;
                targetVector.vy = section->centre.vy - section->prevSection.ptr->centre.vy;
                targetVector.vz = section->centre.vz - section->prevSection.ptr->centre.vz;
            } else {
                targetVector.vx = nextSection->centre.vx - section->centre.vx;
                targetVector.vy = nextSection->centre.vy - section->centre.vy;
                targetVector.vz = nextSection->centre.vz - section->centre.vz;
            }
            magnitude = GetMagnitude(targetVector.vx, targetVector.vy, targetVector.vz);
            targetVector.vx = (targetVector.vx * ship->speed) / magnitude;
            targetVector.vy = (targetVector.vy * ship->speed) / magnitude;
            targetVector.vz = (targetVector.vz * ship->speed) / magnitude;
            path1.vx = section->centre.vx + offset.vx;
            path1.vy = section->centre.vy + offset.vy;
            path1.vz = section->centre.vz + offset.vz;
            path2.vx = nextSection->centre.vx + offset.vx;
            path2.vy = nextSection->centre.vy + offset.vy;
            path2.vz = nextSection->centre.vz + offset.vz;
            GetTargetPos(&bestPath, ship->ppivot, path2, path1);
            ship->apivot.vx = targetVector.vx + (bestPath.vx - ship->ppivot.vx);
            ship->apivot.vy = targetVector.vy + (bestPath.vy - ship->ppivot.vy);
            ship->apivot.vz = targetVector.vz + (bestPath.vz - ship->ppivot.vz);
            ship->vpivot.vx = ship->vpivot.vx + ship->apivot.vx;
            ship->vpivot.vy = ship->vpivot.vy + ship->apivot.vy;
            ship->vpivot.vz = ship->vpivot.vz + ship->apivot.vz;
            /* Speed-up pads boost the drone when it drives over them on the matching side. */
            calcMyXPos(ships, shipNo, &lateral, &along);
            if ((face->flags & SPEED_UP) && (lateral < 0)) {
                ship->speed = ship->speed + speedBonus[(unsigned short)shipClassIndex];
            }
            face++;
            if ((face->flags & SPEED_UP) && (lateral > 0)) {
                ship->speed = ship->speed + speedBonus[(unsigned short)shipClassIndex];
            }
            facePoint.vx = track->vertices[face->vertex[0]].vx;
            facePoint.vy = track->vertices[face->vertex[0]].vy;
            facePoint.vz = track->vertices[face->vertex[0]].vz;
            alpha = PlaneLineCollision(facePoint, ship->ppivot, face->normal);
            if (alpha < 50) {
                alpha = 50;
            }
            ship->apivot.vx = ((face->normal.vx * 256 * 64) / alpha - (face->normal.vx * 64)) >> 8;
            ship->apivot.vy = ((face->normal.vy * 256 * 64) / alpha - (face->normal.vy * 64)) >> 8;
            ship->apivot.vz = ((face->normal.vz * 256 * 64) / alpha - (face->normal.vz * 64)) >> 8;
            ship->vpivot.vx += ship->apivot.vx;
            ship->vpivot.vy += ship->apivot.vy;
            ship->vpivot.vz += ship->apivot.vz;
            /* Heading, pitch and roll follow the target direction. */
            flat = sqrt(targetVector.vx * targetVector.vx + targetVector.vz * targetVector.vz);
            ship->vhdg = -ratan2(targetVector.vx, targetVector.vz) - ship->hdg;
            ship->vhdg += ship->ahdg;
            ship->ahdg -= ship->ahdg >> 3;
            if (ship->vhdg > 2048) {
                ship->vhdg = ship->vhdg - 4096;
            } else if (ship->vhdg < -2048) {
                ship->vhdg = ship->vhdg + 4096;
            }
            ship->hdg += ship->vhdg >> 4;
            ship->vpitch = -ratan2(targetVector.vy, flat) - ship->pitch;
            ship->pitch += ship->vpitch >> 4;
            ship->vroll += ship->vhdg / 4;
            ship->vroll -= ship->vroll >> 1;
            ship->roll += ship->vroll;
            ship->roll -= ship->roll >> 3;
            facePoint.vx = track->vertices[face->vertex[0]].vx;
            facePoint.vy = track->vertices[face->vertex[0]].vy;
            facePoint.vz = track->vertices[face->vertex[0]].vz;
            if (ship->autopilotCount > 0) {
                alpha = PlaneLineCollision(facePoint, ship->ppivot, face->normal);
                if (alpha < -300) {
                    alpha = -300;
                }
                DroneShip2TrackBase(ship, face, alpha);
                section = ship->nearTrkSect;
                nextSection = section->nextSection.ptr;
                direction.vx = nextSection->centre.vx - section->centre.vx;
                direction.vy = nextSection->centre.vy - ship->nearTrkSect->centre.vy;
                direction.vz = nextSection->centre.vz - ship->nearTrkSect->centre.vz;
                if (((direction.vx * ship->unitVecNose.vx) + (direction.vy * ship->unitVecNose.vy) + (direction.vz * ship->unitVecNose.vz)) < 0) {
                    ship->attr &= ~DIRECTION;
                } else {
                    ship->attr |= DIRECTION;
                }
            }
        } else {
            section = ship->nearTrkSect->nextSection.ptr->nextSection.ptr;
            nextSection = section->nextSection.ptr;
            moveDownCentre(&offset, ships, (Face *)0, shipNo);
            ship->speed -= abs(ship->speed * ship->vhdg) >> 14;
            targetVector.vx = nextSection->centre.vx - section->centre.vx;
            targetVector.vy = nextSection->centre.vy - section->centre.vy;
            targetVector.vz = nextSection->centre.vz - section->centre.vz;
            magnitude = GetMagnitude(targetVector.vx, targetVector.vy, targetVector.vz);
            targetVector.vx = (targetVector.vx * ship->speed) / magnitude;
            targetVector.vz = (targetVector.vz * ship->speed) / magnitude;
            targetVector.vy = 500;
            GetTargetPos(&bestPath, ship->ppivot, nextSection->centre, ship->nearTrkSect->centre);
            ship->apivot.vx = targetVector.vx + ((bestPath.vx - ship->ppivot.vx) >> 1);
            ship->apivot.vy = targetVector.vy;
            ship->apivot.vz = targetVector.vz + ((bestPath.vz - ship->ppivot.vz) >> 1);
            ship->vpivot.vx += ship->apivot.vx;
            ship->vpivot.vy += ship->apivot.vy;
            ship->vpivot.vz += ship->apivot.vz;
            ship->vpitch = -200 - ship->pitch;
            ship->pitch += ship->vpitch >> 4;
            ship->vhdg = -ratan2(targetVector.vx, targetVector.vz) - ship->hdg;
            if (ship->vhdg > 2048) {
                ship->vhdg = ship->vhdg - 4096;
            } else if (ship->vhdg < -2048) {
                ship->vhdg = ship->vhdg + 4096;
            }
            ship->hdg += ship->vhdg >> 4;
        }
    }
    ship->vpivot.vx -= ship->vpivot.vx >> 3;
    ship->vpivot.vy -= ship->vpivot.vy >> 3;
    ship->vpivot.vz -= ship->vpivot.vz >> 3;
    ship->ppivot.vx += ship->vpivot.vx >> 6;
    ship->ppivot.vy += ship->vpivot.vy >> 6;
    ship->ppivot.vz += ship->vpivot.vz >> 6;
    if (ship->attr & ELECTROED) {
        ship->ppivot.vx += (newRand() - 0x4000) % 20;
        ship->ppivot.vy += (newRand() - 0x4000) % 20;
        ship->ppivot.vz += (newRand() - 0x4000) % 20;
        ship->electroCount--;
        if (ship->electroCount <= 0) {
            ship->electroCount = 0;
            ship->attr &= ~ELECTROED;
        }
        if ((newRand() % 10) == 0) {
            ship->speed -= ship->speed >> 1;
        }
    }
    ship->hdg = ang(ship->hdg);
    ship->pitch = ang(ship->pitch);
    ship->roll = ang(ship->roll);
    if (ship->autopilotCount > 0) {
        if (padPressed & 0x8000) {
            ship->update = UpdatePlayerShip;
            ship->vhdg = 0;
            ship->autopilotCount = 0;
            if (shipNo == ownShip) {
                PlayNote(46, 255, 15, 15, 256);
            }
        } else {
            ship->autopilotCount--;
            if (ship->autopilotCount == 0) {
                if ((ship->roll > -50) && (ship->roll < 50)) {
                    ship->update = UpdatePlayerShip;
                    ship->vhdg = 0;
                    return;
                }
                ship->autopilotCount = 1;
            }
        }
    }
}
#endif
#pragma optimize("w", off)
#pragma optimize("a", on)
#endif

/* Clears the offset from the racing line. */
void moveDownCentre(VECTOR *relative, ShipData *ships, Face *segment, int shipNo)
{
    relative->vx = 0;
    relative->vy = 0;
    relative->vz = 0;
}

/* Offset from the racing-line point for the ship's section to the start vertex of its face. */
void followRaceLine(VECTOR *relative, ShipData *ships, Face *segment, int ship)
{
    short index;

    index = ships[ship].nearSecNo;
    relative->vx = racingLine[index].vx - track->vertices[segment->vertex[0]].vx;
    relative->vy = racingLine[index].vy - track->vertices[segment->vertex[0]].vy;
    relative->vz = racingLine[index].vz - track->vertices[segment->vertex[0]].vz;
}

/* Steers past the ship ahead when the two are level sideways and there is room beside it. */
void lookForWayPast(VECTOR *relative, ShipData *ships, short shipNo, short otherShip)
{
    short  openSides;
    Face  *face;
    short  rightGap;
    short  myLateral;
    short  leftGap;
    short  otherLateral;
    short  otherExtent;
    short  myWidth;

    openSides = 0;
    face = &track->faces[ships[shipNo].nearTrkSect->faceStart];
    while (!(face->flags & TRACK_BASE)) {
        face++;
    }
    calcMyXPos(ships, otherShip, &otherLateral, &otherExtent);
    calcMyXPos(ships, shipNo, &myLateral, &myWidth);
    if (otherExtent < otherLateral) {
        otherLateral = otherExtent;
    }
    if (-otherExtent > otherLateral) {
        otherLateral = -otherExtent;
    }
    if (myWidth < myLateral) {
        myLateral = myWidth;
    }
    if (-myWidth > myLateral) {
        myLateral = -myWidth;
    }
    if (abs(otherLateral - myLateral) < 20) {
        leftGap = abs(otherExtent + otherLateral);
        rightGap = otherExtent - otherLateral;
        if (leftGap > 20) {
            openSides |= 1;
        }
        if (rightGap > 20) {
            openSides |= 2;
        }
        if ((otherLateral < myLateral) && (openSides & 2)) {
            goRight(relative, ships, face, shipNo);
        } else if ((otherLateral > myLateral) && (openSides & 1)) {
            goLeft(relative, ships, face, shipNo);
        }
    } else {
        followRaceLine(relative, ships, face, shipNo);
    }
}

#ifdef EQUIV
/* Lateral offset of the ship across its track base edge (also stored in lateralOffset), and the
   position of the section centre along that edge. */
void calcMyXPos(ShipData *ships, short shipNo, short *lateral, short *along)
{
#ifdef NETPLAY
    VECTOR edge;
    Face *face;
    VECTOR centreToEnd;
    VECTOR centreOffset;

    face = &track->faces[ships[shipNo].nearTrkSect->faceStart];
    while (!(face->flags & TRACK_BASE)) {
        face++;
    }
    edge.vx = track->vertices[face->vertex[0]].vx - track->vertices[face->vertex[1]].vx;
    edge.vy = track->vertices[face->vertex[0]].vy - track->vertices[face->vertex[1]].vy;
    edge.vz = track->vertices[face->vertex[0]].vz - track->vertices[face->vertex[1]].vz;
    centreOffset.vx = ships[shipNo].nearTrkSect->centre.vx - ships[shipNo].ppivot.vx;
    centreOffset.vy = ships[shipNo].nearTrkSect->centre.vy - ships[shipNo].ppivot.vy;
    centreOffset.vz = ships[shipNo].nearTrkSect->centre.vz - ships[shipNo].ppivot.vz;
    centreToEnd.vx = ships[shipNo].nearTrkSect->centre.vx - track->vertices[face->vertex[1]].vx;
    centreToEnd.vy = ships[shipNo].nearTrkSect->centre.vy - track->vertices[face->vertex[1]].vy;
    centreToEnd.vz = ships[shipNo].nearTrkSect->centre.vz - track->vertices[face->vertex[1]].vz;
    *lateral = -(centreOffset.vy * edge.vy + centreOffset.vz * edge.vz + centreOffset.vx * edge.vx) >> 16;
    *along = (centreToEnd.vz * edge.vz + centreToEnd.vy * edge.vy + centreToEnd.vx * edge.vx) >> 16;
    if (*lateral > 0) {
        ships[shipNo].attr |= LSIDE;
    } else {
        ships[shipNo].attr &= ~LSIDE;
    }
    ships[shipNo].lateralOffset = *lateral;
#else
    /* Preserve section reloads after the output stores. */
    ShipData *ship;
    TrackData *trackData;
    VECTOR *vertices;
    VECTOR *firstVertex;
    VECTOR *secondVertex;
    Face *face;
    VECTOR edge;
    int lateralProjection;
    int centreProjection;
    int centreX;
    short lateralResult;
    u_char flag;

    ship = &ships[shipNo];
    trackData = track;
    face = &trackData->faces[ship->nearTrkSect->faceStart];
    while (!((flag = face->flags) & TRACK_BASE)) {
        face++;
    }
    vertices = trackData->vertices;
    firstVertex = &vertices[face->vertex[0]];
    secondVertex = &vertices[face->vertex[1]];
    edge.vx = firstVertex->vx - secondVertex->vx;
    edge.vy = firstVertex->vy - secondVertex->vy;
    edge.vz = firstVertex->vz - secondVertex->vz;
    centreX = ship->nearTrkSect->centre.vx;
    lateralProjection = (ship->ppivot.vz - ship->nearTrkSect->centre.vz) * edge.vz;
    lateralProjection += (ship->ppivot.vx - centreX) * edge.vx;
    lateralProjection += (ship->ppivot.vy - ship->nearTrkSect->centre.vy) * edge.vy;
    lateralResult = lateralProjection >> 16;
    *lateral = lateralResult;
    centreProjection = (ship->nearTrkSect->centre.vz - secondVertex->vz) * edge.vz;
    centreProjection += (ship->nearTrkSect->centre.vy - secondVertex->vy) * edge.vy;
    centreProjection += (centreX - secondVertex->vx) * edge.vx;
    *along = centreProjection >> 16;
    if (lateralResult > 0) {
        ship->attr |= LSIDE;
    } else {
        ship->attr &= ~LSIDE;
    }
    ship->lateralOffset = lateralResult;
#endif
}
#endif

/* Half the base-edge vector of the face, from its second vertex to its first. */
void goRight(VECTOR *result, ShipData *shipIndex, Face *facePtr, int shipNo)
{
    VECTOR faceVert1;
    VECTOR faceVert2;

    faceVert1.vx = track->vertices[facePtr->vertex[0]].vx;
    faceVert1.vy = track->vertices[facePtr->vertex[0]].vy;
    faceVert1.vz = track->vertices[facePtr->vertex[0]].vz;
    faceVert2.vx = track->vertices[facePtr->vertex[1]].vx;
    faceVert2.vy = track->vertices[facePtr->vertex[1]].vy;
    faceVert2.vz = track->vertices[facePtr->vertex[1]].vz;
    result->vx = (faceVert1.vx - faceVert2.vx) >> 1;
    result->vy = (faceVert1.vy - faceVert2.vy) >> 1;
    result->vz = (faceVert1.vz - faceVert2.vz) >> 1;
}

/* Half the base-edge vector of the face, from its first vertex to its second. */
void goLeft(VECTOR *result, ShipData *shipIndex, Face *facePtr, int shipNo)
{
    VECTOR faceVert1;
    VECTOR faceVert2;

    faceVert1.vx = track->vertices[facePtr->vertex[1]].vx;
    faceVert1.vy = track->vertices[facePtr->vertex[1]].vy;
    faceVert1.vz = track->vertices[facePtr->vertex[1]].vz;
    faceVert2.vx = track->vertices[facePtr->vertex[0]].vx;
    faceVert2.vy = track->vertices[facePtr->vertex[0]].vy;
    faceVert2.vz = track->vertices[facePtr->vertex[0]].vz;
    result->vx = (faceVert1.vx - faceVert2.vx) >> 1;
    result->vy = (faceVert1.vy - faceVert2.vy) >> 1;
    result->vz = (faceVert1.vz - faceVert2.vz) >> 1;
}

/* The nearest ship ahead in race order that is still running, or -1. */
short getShipInfrontLAP(ShipData *ships, short shipNo)
{
#ifdef NETPLAY
    short position;
    short attempts;
    short candidate;

    position = 0;
    while (position < activeShipCount) {
        if (shipOrder[position][0] == shipNo) {
            break;
        }
        position++;
    }
    for (attempts = 0; attempts < 15; attempts++) {
        if (position == 0) {
            position = activeShipCount;
        }
        position--;
        candidate = shipOrder[position][0];
        if (!(ships[candidate].combatAttr & DESTROYED)) {
            if (candidate != shipNo) {
                return candidate;
            }
        }
    }
    return -1;
#else
    short attempts;
    short position;
    short candidate;

    for (position = 0; position < activeShipCount; position++) {
        if (shipOrder[position][0] == shipNo) {
            break;
        }
    }
    for (attempts = 0; attempts < 15; attempts++) {
        if (position == 0) {
            position = activeShipCount;
        }
        position--;
        candidate = shipOrder[position][0];
        if (!(ships[candidate].combatAttr & DESTROYED)) {
            if (candidate != shipNo) {
                return candidate;
            }
        }
    }
    return -1;
#endif
}

/* The nearest running ship behind in race order, or -1. */
short getShipBehindLAP(ShipData *ships, short ship)
{
#ifdef NETPLAY
    short candidate;
    short position;
    short attempts;

    position = 0;
    while (position < activeShipCount) {
        if (shipOrder[position][0] == ship) {
            break;
        }
        position++;
    }
    for (attempts = 0; attempts < 15; attempts++) {
        position++;
        if (position >= activeShipCount) {
            position = 0;
        }
        candidate = shipOrder[position][0];
        if (!(ships[candidate].combatAttr & DESTROYED)) {
            if (candidate != ship) {
                return candidate;
            }
        }
    }
    return -1;
#else
    short position;
    short attempts;
    short candidate;

    for (position = 0; position < activeShipCount; position++) {
        if (shipOrder[position][0] == ship) {
            break;
        }
    }
    for (attempts = 0; attempts < 15; attempts++) {
        position++;
        if (position >= activeShipCount) {
            position = 0;
        }
        candidate = shipOrder[position][0];
        if (!(ships[candidate].combatAttr & DESTROYED)) {
            if (candidate != ship) {
                return candidate;
            }
        }
    }
    return -1;
#endif
}

/* Sets a ship's speed from the distance it moved (times 9), clamped to its remote maximum, and clears boostCount and ahdg. */
void humanToDroneSpeed(ShipData *ships, int shipNo)
{
#ifdef NETPLAY
    int speed;

    speed = ships[shipNo].moveDist * 9;
    if (speed > ships[shipNo].remoteMax) {
        speed = ships[shipNo].remoteMax;
    }
    ships[shipNo].speed = speed;
    ships[shipNo].boostCount = 0;
    ships[shipNo].ahdg = 0;
#else
    ShipData *ship;
    int       speed;

    ship = &ships[shipNo];
    speed = ship->moveDist * 9;
    if (ship->remoteMax < speed) {
        speed = ship->remoteMax;
    }
    ship->speed = speed;
    ship->boostCount = 0;
    ship->ahdg = 0;
#endif
}
