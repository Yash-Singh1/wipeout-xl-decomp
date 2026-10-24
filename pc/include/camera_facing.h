#ifndef CAMERA_FACING_H
#define CAMERA_FACING_H

#include "wipeout.h"

/* Facing bits returned in AL; track rendering sign-extends the byte. */
char GetCameraFacing(TrackCamera *camera);

#endif
