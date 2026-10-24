#ifndef PAUSE_H
#define PAUSE_H

#include "psyq.h"

/* The in-race PC pause menu. */
typedef struct PauseMenu {
    int    selection;       /* highlighted option, -1 while the game is not paused */
    int    musicPaused;
    int    command;         /* option chosen when the menu closes, -1 for none */
    short  pad0c;
#ifdef NETPLAY
    signed char showOptions;
#else
    u_char showOptions;
#endif
#ifdef NETPLAY
    signed char confirmChoice;
#else
    u_char confirmChoice;
#endif
} PauseMenu;

#endif
