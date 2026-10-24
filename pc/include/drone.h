#ifndef DRONE_H
#define DRONE_H

#include "wipeout.h"

struct WeaponData;
extern unsigned short clipLevel, shipClassIndex;
/* drone.c */
extern short shipOrder[][2];
void UpdateDroneShip(ShipData *ships, Object **shapes, short shipNo, struct WeaponData *weapons);
void UpdateDroneShipStart(ShipData *ships, void *unused, short shipNo);
void calcMyXPos(ShipData *ships, short shipNo, short *lateral, short *along);
void followRaceLine(VECTOR *relative, ShipData *ships, Face *segment, int ship);
short getShipBehindLAP(ShipData *ships, short ship);
short getShipInfrontLAP(ShipData *ships, short ship);
void goLeft(VECTOR *result, ShipData *shipIndex, Face *facePtr, int shipNo);
void goRight(VECTOR *result, ShipData *shipIndex, Face *facePtr, int shipNo);
void humanToDroneSpeed(ShipData *ships, int shipNo);
void lookForWayPast(VECTOR *relative, ShipData *ships, short shipNo, short otherShip);
void moveDownCentre(VECTOR *relative, ShipData *ships, Face *segment, int shipNo);

#endif
