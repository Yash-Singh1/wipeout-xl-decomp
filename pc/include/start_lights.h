#ifndef START_LIGHTS_H
#define START_LIGHTS_H

#include "wipeout.h"
#include "pc_config.h"

void UpdateLightStart(Object **objects, ShipData *ships, CheckCell *cells, short textureBase, ConfigData *config, int mode);
void DrawLightStart(Object **objects, ShipData *ships, CheckCell *unusedCells, short unusedTextureBase, ConfigData *unusedConfig, int unusedMode);

#endif
