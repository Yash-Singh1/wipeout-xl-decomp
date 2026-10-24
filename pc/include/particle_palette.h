#ifndef PARTICLE_PALETTE_H
#define PARTICLE_PALETTE_H

#include "wipeout.h"

/* Texture coordinates for the five particle colours in palette mode. */
extern int rgbTextures;
extern float particleWhiteColour, particleOrangeColour;
extern float particleDarkGreyColour, particleBlueColour, particleGreyColour;
void InitParticleColours(void);

#endif
