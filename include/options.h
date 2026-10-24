#ifndef OPTIONS_H
#define OPTIONS_H

#include "psyq.h"

enum { PAUSE_CONTINUE, PAUSE_RESTART, PAUSE_RETURN_TO_MENU, PAUSE_AUDIO };
enum { PAUSE_REQUESTED, PAUSE_REMOTE, PAUSE_NONE };

/* Background box and its semi-transparent blend mode, one pair per screen. */
struct SelectBox {
    POLY_F4 box[2];
    DR_MODE trans[2];
};

/* Pause menu state shared by race control, the link handshake and the renderer. */
typedef struct PauseState {
    int selection;          /* -1 while the game is not paused */
    int musicPaused;
    int command;            /* -1 until an option is chosen */
    int count;
    union {
        int signedCount;
        unsigned long unsignedCount;
    } frames;
    unsigned char pad14;
    unsigned char active;
    unsigned char showOptions;
    unsigned char stage;    /* 0 requested, 1 paused remotely, 2 none */
    unsigned char confirmed;
    unsigned char confirmChoice;
} PauseState;

/* Pause-menu background, blend modes and volume bars, one set per screen. */
typedef struct InGameMenu {
    struct SelectBox dimmer;
    u_char pad48[0xA0];
    POLY_G4 box[2];
    DR_MODE drawMode[2];
    POLY_G4 musicBars[20][2];
    POLY_G4 sfxBars[20][2];
} InGameMenu;

/* A menu entry's text, also copied as words. */
typedef union MenuText {
    long align[4];
    char text[16];
} MenuText;

#endif
