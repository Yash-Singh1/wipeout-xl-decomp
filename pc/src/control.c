#include "functions.h"
#include "input_state.h"
#include "mathinline.h"
#include "sound_samples.h"
#pragma intrinsic(sin, cos)

int startCountdown;
extern short ownShip;
#ifdef NETPLAY
#endif
extern ShipRank shipRank[];
extern RaceLine raceLines[];

extern int gameType;
extern short activeShipCount;
char countdownActive;

extern TrackData *track;
extern int checkTimer;
extern int classCheckTime[];
extern ConfigData *tempGameData;
extern short startLinePos[];
extern CheckCell checkCells[][7];
extern int shipDeathCount[];
extern CameraUpdateFunc CameraUpdate;
extern u_short steerAnalog;
extern u_short turnDivisor;

/* Runs the update routine of every ship in race order (with the ship-to-ship check, except in
   gameType 1), and records how far each ship moved this frame. */
#ifdef NETPLAY
/* NET-WOXL: the ship number lives in curShip only (no ship pointer), and the network input words
   are copied into the pad globals before the update. */
void UpdateShips(ShipData *shipIndex, WeaponShape *unused, Object **shipShapes, WeaponData *weaponIndex)
{
    short i;
    short activeCount;
    short curShip;
    float startPivotX;
    float startPivotY;
    float startPivotZ;

    activeCount = 0;
    for (i = 0; i < 15; i++) {
        if (!(shipIndex[i].combatAttr & DESTROYED)) {
            activeCount++;
        }
    }
    if (activeCount > 0) {
        SortAllShips(shipIndex);
        for (i = 0; activeShipCount > i; i++) {
            curShip = shipOrder[i][0];
            startPivotX = shipIndex[curShip].ppivot.vx;
            startPivotY = shipIndex[curShip].ppivot.vy;
            startPivotZ = shipIndex[curShip].ppivot.vz;
            if (networkGame) {
                padButtons = shipIndex[curShip].net3FA;
                padPressed = shipIndex[curShip].net3FC;
                steerAnalog = shipIndex[curShip].net3FE;
            }
            switch (gameType) {
            case 1:
                ((ShipUpdateFunc)shipIndex[curShip].update)(shipIndex, shipShapes, curShip, weaponIndex);
                break;
            default:
                ((ShipUpdateFunc)shipIndex[curShip].update)(shipIndex, shipShapes, curShip, weaponIndex);
                doShipShipCollision(curShip, shipIndex, shipShapes);
                break;
            }
            startPivotX = shipIndex[curShip].ppivot.vx - startPivotX;
            startPivotY = shipIndex[curShip].ppivot.vy - startPivotY;
            startPivotZ = shipIndex[curShip].ppivot.vz - startPivotZ;
            shipIndex[curShip].moveDist = GetMagnitude(startPivotX, startPivotY, startPivotZ);
        }
        SetUnitVectors(shipIndex);
    }
}
#else
void UpdateShips(ShipData *shipIndex, WeaponShape *unused, Object **shipShapes, WeaponData *weaponIndex)
{
    short  i;
    short  activeCount;
    short  shipNo;
    short  shipNum;
    float  lastX;
    float  lastY;
    float  lastZ;
    float  dx;
    float  dy;
    float  dz;
    ShipData *ship;

    activeCount = 0;
    for (i = 0; i < 15; i++) {
        if (!(shipIndex[i].combatAttr & DESTROYED)) {
            activeCount++;
        }
    }
    if (activeCount > 0) {
        SortAllShips(shipIndex);
        for (i = 0; i < activeShipCount; i++) {
            shipNo = shipOrder[i][0];
            ship = &shipIndex[shipNo];
            lastX = ship->ppivot.vx;
            lastY = ship->ppivot.vy;
            lastZ = ship->ppivot.vz;
            shipNum = shipNo;
            switch (gameType) {
            case 1:
                ((ShipUpdateFunc)ship->update)(shipIndex, shipShapes, shipNo, weaponIndex);
                break;
            default:
                ((ShipUpdateFunc)shipIndex[shipNo].update)(shipIndex, shipShapes, shipNo, weaponIndex);
                doShipShipCollision(shipNum, shipIndex, shipShapes);
            }
            dx = shipIndex[shipNum].ppivot.vx - lastX;
            dy = shipIndex[shipNum].ppivot.vy - lastY;
            dz = shipIndex[shipNum].ppivot.vz - lastZ;
            shipIndex[shipNum].moveDist = GetMagnitude(dx, dy, dz);
        }
        SetUnitVectors(shipIndex);
    }
}
#endif

/* Matches the executable up to register allocation (EQUIV). The tie-break projects both ships with
   distanceA, as the original does. */
#ifdef EQUIV
/* Re-sorts the race order: ships are ranked by the section they are in, and ships in the same
   section are ordered by how far each has got along it. */
#ifdef NETPLAY
/* NET keeps these two assignments inside constant ifs; the /Od build keeps the jump after each. */
#define NET_SORT_BEARING_B 1
#define NET_SORT_PROJ_A 1
void SortAllShips(ShipData *shipIndex)
{
    short changed;
    long projA;
    ShipData *racerPtr;
    short shipSlot;
    long projB;
    short currentShip;
    u_short spare;
    VECTOR deltaA;
    ShipRank *current;
    short total;
    short nextIndex;
    ShipRank *following;
    VECTOR vecB;
    VECTOR direction;
    long bearingA;
    long bearingB;
    short i;
    ShipData *owner;

    total = 0;
    for (i = 0; i < activeShipCount; i++) {
        currentShip = shipOrder[i][0];
        if (!(shipIndex[currentShip].combatAttr & DESTROYED)) {
            shipRank[total].position = raceLines[shipIndex[currentShip].nearSecNo].distance;
            shipOrder[total][0] = currentShip;
            total++;
        }
    }
    activeShipCount = total;
    if (activeShipCount > 1) {
        do {
            changed = 0;
            for (i = 0; i < activeShipCount - 1; i++) {
                current = &shipRank[i];
                following = &shipRank[i + 1];
                if (shipRank[i + 1].position > current->position) {
                    spare = following->position;
                    following->position = current->position;
                    current->position = spare;
                    spare = following->shipNo;
                    following->shipNo = current->shipNo;
                    current->shipNo = spare;
                    changed = 1;
                } else if (following->position == current->position) {
                    shipSlot = current->shipNo;
                    nextIndex = following->shipNo;
                    owner = &shipIndex[shipSlot];
                    racerPtr = &shipIndex[shipSlot];
                    deltaA.vx = owner->ppivot.vx - owner->nearTrkSect->centre.vx;
                    deltaA.vy = owner->ppivot.vy - owner->nearTrkSect->centre.vy;
                    deltaA.vz = owner->ppivot.vz - owner->nearTrkSect->centre.vz;
                    vecB.vx = racerPtr->ppivot.vx - racerPtr->nearTrkSect->centre.vx;
                    vecB.vy = racerPtr->ppivot.vy - racerPtr->nearTrkSect->centre.vy;
                    vecB.vz = racerPtr->ppivot.vz - racerPtr->nearTrkSect->centre.vz;
                    direction.vx = owner->nearTrkSect->nextSection.ptr->centre.vx - owner->nearTrkSect->centre.vx;
                    direction.vy = owner->nearTrkSect->nextSection.ptr->centre.vy - owner->nearTrkSect->centre.vy;
                    direction.vz = owner->nearTrkSect->nextSection.ptr->centre.vz - owner->nearTrkSect->centre.vz;
                    bearingA = GetAng(deltaA, direction);
                    if (NET_SORT_BEARING_B) {
                        bearingB = GetAng(vecB, direction);
                    } else {
                    }
                    if (NET_SORT_PROJ_A) {
                        projA = (rcos(bearingA >> 3) * GetMagnitude(deltaA.vx, deltaA.vy, deltaA.vz)) >> 12;
                    } else {
                    }
                    projB = (rcos(bearingB >> 3) * GetMagnitude(vecB.vx, vecB.vy, vecB.vz)) >> 12;
                    if (projB > projA) {
                        spare = following->position;
                        following->position = current->position;
                        current->position = spare;
                        spare = following->shipNo;
                        following->shipNo = current->shipNo;
                        current->shipNo = spare;
                        }
                }
            }
        } while (changed);
    }
}
#else
void SortAllShips(ShipData *shipIndex)
{
    short i, count;
    u_short swapSection, swapShip;
    short firstSection;
    short swapped;
    VECTOR distanceA;
    VECTOR distanceB;
    VECTOR direction;
    long angleA, angleB;
    long projectedA, projectedB;
    ShipData *playerShip;
    ShipRank *current;
    short currentShip;
    u_short sectionNumber;

    count = 0;
    for (i = 0; i < activeShipCount; i++) {
        currentShip = shipOrder[i][0];
        playerShip = &shipIndex[currentShip];
        if (!(playerShip->combatAttr & DESTROYED)) {
            sectionNumber = raceLines[playerShip->nearSecNo].distance;
            shipRank[count].position = sectionNumber;
            shipOrder[count][0] = currentShip;
            count++;
        }
    }
    activeShipCount = count;
    if (count > 1) {
        do {
            swapped = 0;
            for (i = 0; i < activeShipCount - 1; i++) {
                current = &shipRank[i];
                firstSection = current->position;
                if (firstSection < shipRank[i + 1].position) {
                    swapSection = shipRank[i + 1].position;
                    swapShip = shipRank[i + 1].shipNo;
                    shipRank[i + 1].position = firstSection;
                    current->position = swapSection;
                    shipRank[i + 1].shipNo = current->shipNo;
                    current->shipNo = swapShip;
                    swapped = 1;
                } else if (firstSection == shipRank[i + 1].position) {
                    playerShip = &shipIndex[current->shipNo];
                    distanceA.vx = playerShip->ppivot.vx - playerShip->nearTrkSect->centre.vx;
                    distanceA.vy = playerShip->ppivot.vy - playerShip->nearTrkSect->centre.vy;
                    distanceA.vz = playerShip->ppivot.vz - playerShip->nearTrkSect->centre.vz;
                    distanceB.vx = playerShip->ppivot.vx - playerShip->nearTrkSect->centre.vx;
                    distanceB.vy = playerShip->ppivot.vy - playerShip->nearTrkSect->centre.vy;
                    distanceB.vz = playerShip->ppivot.vz - playerShip->nearTrkSect->centre.vz;
                    direction.vx = playerShip->nearTrkSect->nextSection.ptr->centre.vx - playerShip->nearTrkSect->centre.vx;
                    direction.vy = playerShip->nearTrkSect->nextSection.ptr->centre.vy - playerShip->nearTrkSect->centre.vy;
                    direction.vz = playerShip->nearTrkSect->nextSection.ptr->centre.vz - playerShip->nearTrkSect->centre.vz;
                    angleA = GetAng(distanceA, direction);
                    angleB = GetAng(distanceB, direction);
                    projectedA = (rcos(angleA >> 3) * GetMagnitude(distanceA.vx, distanceA.vy, distanceA.vz)) >> 12;
                    projectedB = (rcos(angleB >> 3) * GetMagnitude(distanceA.vx, distanceA.vy, distanceA.vz)) >> 12;
                    if (projectedA < projectedB) {
                        swapSection = shipRank[i + 1].position;
                        swapShip = shipRank[i + 1].shipNo;
                        shipRank[i + 1].position = current->position;
                        current->position = swapSection;
                        shipRank[i + 1].shipNo = current->shipNo;
                        current->shipNo = swapShip;
                    }
                }
            }
        } while (swapped);
    }
}
#endif
#endif

#ifdef EQUIV
#ifdef NETPLAY
void UpdateRacingShip(ShipData *shipIndex, void *shapes, short shipNo, void *weapons)
{
    short targetTurn;
    short upper;
    short maxThrust;
    ShipData *ship;
    short isDestroyed = 0;
    /* The original declares and initialises this table and never reads it. */
    short thrustTable[4] = {0, 6, 12, 18};
    short loThrust;

    ship = &shipIndex[shipNo];
    if (ship->combatAttr & DESTROYED) {
        return;
    }
    if (ship->electroCount > 0) {
        ship->electroCount--;
    } else if (ship->electroCount == 0) {
        ship->attr &= ~ELECTROED;
    }
    targetTurn = -((((steerAnalog - 128) * (ship->maxHeading + 400)) << 1) / turnDivisor);
    if (ship->vhdg < targetTurn) {
        if (targetTurn - ship->vhdg <= ship->headingInc) {
            ship->vhdg += targetTurn - ship->vhdg;
        } else if (ship->vhdg >= 0) {
            ship->vhdg += ship->headingInc;
        } else if (ship->vhdg < 0) {
            ship->vhdg += ship->headingInc << 1;
        }
    } else if (ship->vhdg > targetTurn) {
        if (targetTurn - ship->vhdg >= -ship->headingInc) {
            ship->vhdg += targetTurn - ship->vhdg;
        }
        if (ship->vhdg <= 0) {
            ship->vhdg -= ship->headingInc;
        } else if (ship->vhdg > 0) {
            ship->vhdg -= ship->headingInc << 1;
        }
    }
    if (ship->vhdg < -ship->maxHeading) {
        ship->vhdg = -ship->maxHeading;
    }
    if (ship->vhdg > ship->maxHeading) {
        ship->vhdg = ship->maxHeading;
    }
    if (ship->attr & ELECTROED) {
        if (shipNo == ownShip && (ship->attr & VIEW_IN)) {
            SetShake(2);
        }
        ship->vhdg += newRand() % 600 - 300;
    }
    if (padButtons & 0x40) {
        ship->vpitch += 30;
    } else if (padButtons & 0x10) {
        ship->vpitch -= 30;
    }
    if (ship->updateCount == 90) {
        loThrust = (ship->maxThrust >> 3) * 5;
        upper = (ship->maxThrust >> 3) * 6;
        /* NET-WOXL keeps this empty then branch: the exe jumps over it. */
        if (ship->thrust < loThrust) {
        } else {
            if (ship->thrust < upper) {
                ship->thrustCount = 9;
                ship->thrust = ship->maxThrust;
                if (shipNo == ownShip) {
                    PlayNote(SAMPLE_NEW_TURBO, 0xFF, 0xF, 0xF, 0x100);
                }
            } else {
                ship->stallCount = 15;
                if (shipNo == ownShip) {
                    PlayNote(SAMPLE_SHIP_STALL, 0xFF, 0xF, 0xF, 0x100);
                }
            }
        }
    }
    if (ship->thrustCount > 0) {
        ship->thrustCount--;
    }
    if (ship->updateCount > 0) {
        ship->updateCount--;
    }
    if (!(ship->combatAttr & 8)) {
        maxThrust = ship->maxThrust;
        if (padButtons & PAD_BUTTON_THRUST) {
            ship->thrust += 16;
        } else if (ship->thrust >= 8) {
            ship->thrust -= 8;
        }
        if (ship->thrust > maxThrust) {
            ship->thrust = maxThrust;
        }
    } else {
        ship->thrust -= 64;
        if (ship->thrust < 0) {
            ship->thrust = 0;
            if ((int)ship->speed < 5000) {
                isDestroyed = 1;
            }
        }
    }
    if ((ship->attr & ELECTROED) && newRand() % 80 == 0) {
        ship->thrust -= ship->thrust >> 2;
    }
    if (ship->stallCount > 0) {
        ship->stallCount--;
        ship->thrust -= ship->thrust >> 2;
    }
    SetBrake(ship);
    SetFire(ship, weapons, shipNo);
    ((void (*)(ShipData *, void *, int))SetDitch)(ship, weapons, shipNo);
    ShipTrkReaction(ship, shapes, shipNo);
    if (isDestroyed) {
        shipDeathCount[shipNo] = 100;
        ship->combatAttr |= 4;
        if (ship->attr & VIEW_IN) {
            ship->attr &= ~VIEW_IN;
            CameraUpdate = UpdateCameraRaceExternal;
        }
    }
}
#endif
#endif

#ifdef NETPLAY
/* Configuration switch for the start-of-race height lift (always on in NET-WOXL). */
#define NET_START_LEVEL 1

/* NET-WOXL only: start-of-race update for any ship. It is UpdatePlayerShipStart without the
   check on the player's ship; racing ships then hand over to UpdateRacingShip, the race update. */
void UpdateShipStart(ShipData *shipIndex, void *shipShapes, short shipNo, void *weaponIndex)
{
    ShipData *ship;

    ship = &shipIndex[shipNo];
    GeneralStartProcs(ship);
    /* NET-WOXL keeps the constant switch: its empty else leaves the jump the exe has. */
    if (NET_START_LEVEL) {
        if (ship->updateCount == 199) {
            ship->startY = ship->ppivot.vy;
        }
    } else {
    }
    ship->ppivot.vy = ship->startY + (rsin(ship->updateCount * 80) >> 7);
    if (ship->updateCount == 0) {
        if (ship->attr & RACING) {
            ship->updateCount = 90;
            ship->update = UpdateRacingShip;
        } else {
            ship->update = UpdateDroneShip;
        }
    }
    if (ship->attr & RACING) {
        SetBrake(ship);
    }
    if (padButtons & PAD_BUTTON_THRUST) {
        ship->thrust += 16;
    } else if (ship->thrust > 15) {
        ship->thrust -= 16;
    }
    if (ship->thrust > ship->maxThrust) {
        ship->thrust = ship->maxThrust;
    }
}

#ifndef EQUIV
void UpdateRacingShip(ShipData *shipIndex, void *shapes, short shipNo, void *weapons)
{
    short     upper;
    short     destroyed;
    short     thrust;
    short     lower;
    ShipData *playerShip;
    short     launchThrust;
    short     maxThrust;
    short     targetTurn;

    destroyed = 0;
    playerShip = &shipIndex[shipNo];
    if (playerShip->combatAttr & DESTROYED) {
        return;
    }
    if (playerShip->electroCount > 0) {
        playerShip->electroCount--;
    } else if (playerShip->electroCount == 0) {
        playerShip->attr &= ~ELECTROED;
    }
    targetTurn = (((steerAnalog - 128) * (playerShip->maxHeading + 400)) << 1) / turnDivisor;
    targetTurn = -targetTurn;
    if (playerShip->vhdg < targetTurn) {
        if (targetTurn - playerShip->vhdg <= playerShip->headingInc) {
            playerShip->vhdg += targetTurn - playerShip->vhdg;
        } else if (playerShip->vhdg >= 0) {
            playerShip->vhdg += playerShip->headingInc;
        } else if (playerShip->vhdg < 0) {
            playerShip->vhdg += playerShip->headingInc << 1;
        }
    } else if (playerShip->vhdg > targetTurn) {
        if (targetTurn - playerShip->vhdg >= -playerShip->headingInc) {
            playerShip->vhdg += targetTurn - playerShip->vhdg;
        }
        if (playerShip->vhdg <= 0) {
            playerShip->vhdg -= playerShip->headingInc;
        } else {
            playerShip->vhdg -= playerShip->headingInc << 1;
        }
    }
    if (playerShip->vhdg < -playerShip->maxHeading) {
        playerShip->vhdg = -playerShip->maxHeading;
    }
    if (playerShip->vhdg > playerShip->maxHeading) {
        playerShip->vhdg = playerShip->maxHeading;
    }
    if (playerShip->attr & ELECTROED) {
        if (shipNo == ownShip && (playerShip->attr & VIEW_IN)) {
            SetShake(2);
        }
        playerShip->vhdg += newRand() % 600 - 300;
    }
    if (padButtons & 0x40) {
        playerShip->vpitch += 30;
    } else if (padButtons & 0x10) {
        playerShip->vpitch -= 30;
    }
    if (playerShip->updateCount == 90) {
        maxThrust = playerShip->maxThrust;
        launchThrust = maxThrust >> 3;
        upper = launchThrust * 6;
        thrust = playerShip->thrust;
        lower = launchThrust * 5;
        if (lower <= thrust) {
            if (thrust < upper) {
                playerShip->thrustCount = 9;
                playerShip->thrust = maxThrust;
                if (shipNo == ownShip) {
                    PlayNote(SAMPLE_NEW_TURBO, 0xFF, 0xF, 0xF, 0x100);
                }
            } else {
                playerShip->stallCount = 15;
                if (shipNo == ownShip) {
                    PlayNote(SAMPLE_SHIP_STALL, 0xFF, 0xF, 0xF, 0x100);
                }
            }
        }
    }
    if (playerShip->thrustCount > 0) {
        playerShip->thrustCount--;
    }
    if (playerShip->updateCount > 0) {
        playerShip->updateCount--;
    }
    if (!(playerShip->combatAttr & 8)) {
        maxThrust = playerShip->maxThrust;
        if (padButtons & PAD_BUTTON_THRUST) {
            playerShip->thrust += 16;
        } else if (playerShip->thrust >= 8) {
            playerShip->thrust -= 8;
        }
        if (playerShip->thrust > maxThrust) {
            playerShip->thrust = maxThrust;
        }
    } else {
        playerShip->thrust -= 64;
        if (playerShip->thrust < 0) {
            playerShip->thrust = 0;
            if ((int)playerShip->speed < 5000) {
                destroyed = 1;
            }
        }
    }
    if ((playerShip->attr & ELECTROED) && newRand() % 80 == 0) {
        playerShip->thrust -= playerShip->thrust >> 2;
    }
    if (playerShip->stallCount > 0) {
        playerShip->stallCount--;
        playerShip->thrust -= playerShip->thrust >> 2;
    }
    SetBrake(playerShip);
    SetFire(playerShip, weapons, shipNo);
    ((void (*)(ShipData *, void *, int))SetDitch)(playerShip, weapons, shipNo);
    ShipTrkReaction(playerShip, shapes, shipNo);
    if (destroyed) {
        shipDeathCount[shipNo] = 100;
        playerShip->combatAttr |= 4;
        if (playerShip->attr & VIEW_IN) {
            playerShip->attr &= ~VIEW_IN;
            CameraUpdate = UpdateCameraRaceExternal;
        }
    }
}
#endif
#endif

/* Start-of-race update for the player's ship: runs the countdown, lifts the ship to its
   starting height, then hands over to the race update once the countdown has finished. */
void UpdatePlayerShipStart(ShipData *shipIndex, void *shipShapes, short shipNo, void *weaponIndex)
{
    ShipData *playerShip;

    if (shipIndex[ownShip].combatAttr & DESTROYED) {
        return;
    }
    playerShip = &shipIndex[shipNo];
    GeneralStartProcs(playerShip);
#ifdef NETPLAY
    /* NET-WOXL keeps the constant switch: its empty else leaves the jump the exe has. */
    if (NET_START_LEVEL) {
        if (playerShip->updateCount == 199) {
            playerShip->startY = playerShip->ppivot.vy;
        }
    } else {
    }
#else
    if (playerShip->updateCount == 199) {
        playerShip->startY = playerShip->ppivot.vy;
    }
#endif
    playerShip->ppivot.vy = playerShip->startY + (rsin(playerShip->updateCount * 80) >> 7);
    if (playerShip->updateCount == 0) {
        if (playerShip->attr & RACING) {
            playerShip->updateCount = 90;
            playerShip->update = UpdatePlayerShip;
        } else {
            playerShip->update = UpdateDroneShip;
        }
    }
    if (playerShip->attr & RACING) {
        SetBrake(playerShip);
    }
    if (padButtons & PAD_BUTTON_THRUST) {
        playerShip->thrust += 16;
    } else if (playerShip->thrust > 15) {
        playerShip->thrust -= 16;
    }
    if (playerShip->thrust > playerShip->maxThrust) {
        playerShip->thrust = playerShip->maxThrust;
    }
}

#ifndef NETPLAY
/* Per-frame update of the player's ship: turns it towards the pad steering, works the thrust and
   launch, and runs brake, fire, ditch and track reaction. The locals are declared in this order
   because it gives the stack and register layout of the executable. SetDitch is called with three
   arguments, as the executable does (the PC SetDitch takes only the ship). */
void UpdatePlayerShip(ShipData *shipIndex, void *shapes, short shipNo, void *weapons)
{
    short     upper;
    short     destroyed;
    short     thrust;
    short     lower;
    ShipData *playerShip;
    short     launchThrust;
    short     maxThrust;
    short     targetTurn;

    destroyed = 0;
    playerShip = &shipIndex[shipNo];
    if (shipDeathCount[shipNo] != -1) {
        ShipTrkReaction(playerShip, shapes, shipNo);
        return;
    }
    if (playerShip->combatAttr & DESTROYED) {
        return;
    }
    if (playerShip->electroCount > 0) {
        playerShip->electroCount--;
    } else if (playerShip->electroCount == 0) {
        playerShip->attr &= ~ELECTROED;
    }
    targetTurn = (((steerAnalog - 128) * (-400 - playerShip->maxHeading)) << 1) / turnDivisor;
    if (playerShip->vhdg < targetTurn) {
        if (targetTurn - playerShip->vhdg <= playerShip->headingInc) {
            playerShip->vhdg += targetTurn - playerShip->vhdg;
        } else if (playerShip->vhdg >= 0) {
            playerShip->vhdg += playerShip->headingInc;
        } else if (playerShip->vhdg < 0) {
            playerShip->vhdg += playerShip->headingInc << 1;
        }
    } else if (playerShip->vhdg > targetTurn) {
        if (targetTurn - playerShip->vhdg >= -playerShip->headingInc) {
            playerShip->vhdg += targetTurn - playerShip->vhdg;
        }
        if (playerShip->vhdg <= 0) {
            playerShip->vhdg -= playerShip->headingInc;
        } else {
            playerShip->vhdg -= playerShip->headingInc << 1;
        }
    }
    if (playerShip->vhdg < -playerShip->maxHeading) {
        playerShip->vhdg = -playerShip->maxHeading;
    }
    if (playerShip->vhdg > playerShip->maxHeading) {
        playerShip->vhdg = playerShip->maxHeading;
    }
    if (playerShip->attr & ELECTROED) {
        if (shipNo == ownShip && (playerShip->attr & VIEW_IN)) {
            SetShake(2);
        }
        playerShip->vhdg += newRand() % 600 - 300;
    }
    if (padButtons & 0x40) {
        playerShip->vpitch += 30;
    } else if (padButtons & 0x10) {
        playerShip->vpitch -= 30;
    }
    if (playerShip->updateCount == 90) {
        maxThrust = playerShip->maxThrust;
        launchThrust = maxThrust >> 3;
        upper = launchThrust * 6;
        thrust = playerShip->thrust;
        lower = launchThrust * 5;
        if (lower <= thrust) {
            if (thrust < upper) {
                playerShip->thrustCount = 9;
                playerShip->thrust = maxThrust;
                if (shipNo == ownShip) {
                    PlayNote(SAMPLE_NEW_TURBO, 0xFF, 0xF, 0xF, 0x100);
                }
            } else {
                playerShip->stallCount = 15;
                if (shipNo == ownShip) {
                    PlayNote(SAMPLE_SHIP_STALL, 0xFF, 0xF, 0xF, 0x100);
                }
            }
        }
    }
    if (playerShip->thrustCount > 0) {
        playerShip->thrustCount--;
    }
    if (playerShip->updateCount > 0) {
        playerShip->updateCount--;
    }
    if (!(playerShip->combatAttr & 8)) {
        maxThrust = playerShip->maxThrust;
        if (padButtons & PAD_BUTTON_THRUST) {
            playerShip->thrust += 16;
        } else if (playerShip->thrust >= 8) {
            playerShip->thrust -= 8;
        }
        if (playerShip->thrust > maxThrust) {
            playerShip->thrust = maxThrust;
        }
    } else {
        playerShip->thrust -= 64;
        if (playerShip->thrust < 0) {
            playerShip->thrust = 0;
            if (playerShip->speed < 5000U) {
                destroyed = 1;
            }
        }
    }
    if ((playerShip->attr & ELECTROED) && newRand() % 80 == 0) {
        playerShip->thrust -= playerShip->thrust >> 2;
    }
    if (playerShip->stallCount > 0) {
        playerShip->stallCount--;
        playerShip->thrust -= playerShip->thrust >> 2;
    }
    SetBrake(playerShip);
    SetFire(playerShip, weapons, shipNo);
    ((void (*)(ShipData *, void *, int))SetDitch)(playerShip, weapons, shipNo);
    ShipTrkReaction(playerShip, shapes, shipNo);
    if (destroyed) {
        shipDeathCount[shipNo] = 100;
        playerShip->combatAttr |= 4;
        if (playerShip->attr & VIEW_IN) {
            playerShip->attr &= ~VIEW_IN;
            CameraUpdate = UpdateCameraRaceExternal;
        }
    }
}
#endif
#ifdef EQUIV
#ifdef NETPLAY
/* NET-WOXL: the locals follow the /Od frame layout (tools/odslots.py). thrustTable is declared and
   initialised but never read in the original. The empty then branch on the thrust test is the jump the
   exe keeps. */
void UpdatePlayerShip(ShipData *shipIndex, void *shapes, short shipNo, void *weapons)
{
    short     targetTurn;
    short     upper;
    short     maxThrust;
    ShipData *ship;
    short     loThrust;
    short     isDestroyed = 0;
    short     thrustTable[4] = {0, 6, 12, 18};

    ship = &shipIndex[shipNo];
    if (shipDeathCount[shipNo] != -1) {
        ShipTrkReaction(ship, shapes, shipNo);
        return;
    }
    if (ship->combatAttr & DESTROYED) {
        return;
    }
    if (ship->electroCount > 0) {
        ship->electroCount--;
    } else if (ship->electroCount == 0) {
        ship->attr &= ~ELECTROED;
    }
    targetTurn = -((((ship->maxHeading + 400) * (steerAnalog - 128)) << 1) / turnDivisor);
    if (ship->vhdg < targetTurn) {
        if (targetTurn - ship->vhdg <= ship->headingInc) {
            ship->vhdg += targetTurn - ship->vhdg;
        } else if (ship->vhdg >= 0) {
            ship->vhdg += ship->headingInc;
        } else if (ship->vhdg < 0) {
            ship->vhdg += ship->headingInc << 1;
        }
    } else if (ship->vhdg > targetTurn) {
        if (targetTurn - ship->vhdg >= -ship->headingInc) {
            ship->vhdg += targetTurn - ship->vhdg;
        }
        if (ship->vhdg <= 0) {
            ship->vhdg -= ship->headingInc;
        } else if (ship->vhdg > 0) {
            ship->vhdg -= ship->headingInc << 1;
        }
    }
    if (ship->vhdg < -ship->maxHeading) {
        ship->vhdg = -ship->maxHeading;
    }
    if (ship->vhdg > ship->maxHeading) {
        ship->vhdg = ship->maxHeading;
    }
    if (ship->attr & ELECTROED) {
        if (shipNo == ownShip && (ship->attr & VIEW_IN)) {
            SetShake(2);
        }
        ship->vhdg += newRand() % 600 - 300;
    }
    if (padButtons & 0x40) {
        ship->vpitch += 30;
    } else if (padButtons & 0x10) {
        ship->vpitch -= 30;
    }
    if (ship->updateCount == 90) {
        loThrust = (ship->maxThrust >> 3) * 5;
        upper = (ship->maxThrust >> 3) * 6;
        if (loThrust > ship->thrust) {
            /* Empty then branch: the exe jumps over the launch update below. */
        } else {
            if (ship->thrust < upper) {
                ship->thrustCount = 9;
                ship->thrust = ship->maxThrust;
                if (ownShip == shipNo) {
                    PlayNote(SAMPLE_NEW_TURBO, 0xFF, 0xF, 0xF, 0x100);
                }
            } else {
                ship->stallCount = 15;
                if (ownShip == shipNo) {
                    PlayNote(SAMPLE_SHIP_STALL, 0xFF, 0xF, 0xF, 0x100);
                }
            }
        }
    }
    if (ship->thrustCount > 0) {
        ship->thrustCount--;
    }
    if (ship->updateCount > 0) {
        ship->updateCount--;
    }
    if (!(ship->combatAttr & 8)) {
        maxThrust = ship->maxThrust;
        if (padButtons & PAD_BUTTON_THRUST) {
            ship->thrust += 16;
        } else if (ship->thrust >= 8) {
            ship->thrust -= 8;
        }
        if (ship->thrust > maxThrust) {
            ship->thrust = maxThrust;
        }
    } else {
        ship->thrust -= 64;
        if (ship->thrust < 0) {
            ship->thrust = 0;
            if (ship->speed < 5000) {
                isDestroyed = 1;
            }
        }
    }
    if ((ship->attr & ELECTROED) && newRand() % 80 == 0) {
        ship->thrust -= ship->thrust >> 2;
    }
    if (ship->stallCount > 0) {
        ship->stallCount--;
        ship->thrust -= ship->thrust >> 2;
    }
    SetBrake(ship);
    SetFire(ship, weapons, shipNo);
    ((void (*)(ShipData *, void *, int))SetDitch)(ship, weapons, shipNo);
    ShipTrkReaction(ship, shapes, shipNo);
    if (isDestroyed) {
        shipDeathCount[shipNo] = 100;
        ship->combatAttr |= 4;
        if (ship->attr & VIEW_IN) {
            ship->attr &= ~VIEW_IN;
            CameraUpdate = UpdateCameraRaceExternal;
        }
    }
}
#endif
#endif

void SetUpFirstCheckPoint(void)
{
    TrackSection *sect;
    Face         *face;
    short         i;

    checkTimer = classCheckTime[tempGameData->shipClass];
    sect = track->sections;
    while (sect->secNo != startLinePos[tempGameData->actTrackNum]) {
        sect = sect->nextSection.ptr;
    }
    face = track->faces + sect->faceStart;
    while ((face->flags & TRACK_BASE) == 0) {
        face++;
    }
    if (face->flags & CHECKPOINT) {
        i = 0;
        do {
            if (checkCells[tempGameData->trackNum * 4 + tempGameData->shipClass][i].section == sect->secNo) {
                checkCells[tempGameData->trackNum * 4 + tempGameData->shipClass][i].done = 1;
                checkTimer += checkCells[tempGameData->trackNum * 4 + tempGameData->shipClass][i].bonusSeconds * 60;
            }
            i++;
        } while (i < 6);
    }
}

/* Counts a ship down from the start of the race and plays the countdown tones at 33, 66 and 100 frames to go. */
void GeneralStartProcs(ShipData *ship)
{
#ifdef NETPLAY
    /* NET-WOXL decrements startCountdown in memory and keeps no local copy of it. */
    if (ship->updateCount == 200) {
        startCountdown = 200;
        countdownActive = 1;
    }
    ship->updateCount--;
    startCountdown--;
#else
    int countdown;

    countdown = 200;
    if (ship->updateCount == 200) {
        countdownActive = 1;
    } else {
        countdown = startCountdown;
    }
    ship->updateCount--;
    countdown--;
    startCountdown = countdown;
#endif
    if (ship->updateCount == 0) {
        PlayNote(SAMPLE_COUNTDOWN_GO, 0xFF, 0xF, 0xF, 0x100);
        countdownActive = 0;
    }
    if (ship->updateCount == 0x21) {
        PlayNote(SAMPLE_COUNTDOWN_ONE, 0xFF, 0xF, 0xF, 0xDC);
    } else if (ship->updateCount == 0x42) {
        PlayNote(SAMPLE_COUNTDOWN_TWO, 0xFF, 0xF, 0xF, 0xDC);
    } else if (ship->updateCount == 0x64) {
        PlayNote(SAMPLE_COUNTDOWN_THREE, 0xFF, 0xF, 0xF, 0xDC);
    }
}

#ifdef POWERVR
void Stub_4135A0(void)
{
}

void Stub_413710(void)
{
}

int Stub_413720(void)
{
    return 1;
}

int Stub_413730(void)
{
    return 0;
}

int Stub_413740(void)
{
    return 0;
}

int Stub_413750(void)
{
    return 1;
}

int Stub_413760(void)
{
    return 0;
}

int Stub_413770(void)
{
    return 0;
}

int Stub_413780(int value)
{
    return value;
}

int Stub_4137A0(void)
{
    return 1;
}

int Stub_4137C0(void)
{
    return 0;
}

void Stub_4137D0(void)
{
}
#endif
