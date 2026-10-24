#ifndef AYRACE_H
#define AYRACE_H

#include "psyq.h"

#include "options.h"

/* A CD position as the drive reports it: track and time in BCD. */
struct WipeCdStatus {
    u_char track;
    u_char pad1[3];
    u_char time;
};

#endif
