#ifndef CHEATS_H
#define CHEATS_H

/* NET sign-extends these flags; D3D uses byte tests and stores. */
#ifdef NETPLAY
extern signed char cheatWeapon, cheatDamage, cheatRain, cheatTime, cheatDrone;
#else
extern unsigned char cheatWeapon, cheatDamage, cheatRain, cheatTime, cheatDrone;
#endif

#endif
