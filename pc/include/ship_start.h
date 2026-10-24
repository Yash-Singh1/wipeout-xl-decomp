#ifndef SHIP_START_H
#define SHIP_START_H

#include "wipeout.h"

void UpdatePlayerShipStart(ShipData *ships, void *shapes, short shipNo, void *weapons);
#ifdef NETPLAY
void UpdateShipStart(ShipData *ships, void *shapes, short shipNo, void *weapons);
#endif

#endif
