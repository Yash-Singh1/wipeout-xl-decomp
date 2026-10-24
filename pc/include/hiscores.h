#ifndef HISCORES_H
#define HISCORES_H

/* Width and metrics of one glyph of the menu font (8 bytes per entry on PC). */
typedef struct GlyphWidth {
    short width, height, offset;
    short reserved;
} GlyphWidth;

enum {
    HISCORE_LAP_RECORD = 1,
    HISCORE_RACE_RECORD = 2
};

/* State of the race that has just finished and of the name entry for a new record.
   pendingRecords holds records waiting for a name; displayedRecords holds the
   records shown on the score tab. Both use the HISCORE_*_RECORD bits. */
struct HiScoreData {
    char nameEntered[9];       /* 0x00: entered name, NUL terminated */
    u8  selectedChar;          /* 0x09: letter under the cursor */
    short alphabetIndex;       /* 0x0A: cursor position in the alphabet; 26 = delete, 27 = end */
    short charactersTyped;     /* 0x0C: letters typed, counted up with each one (EnterName) */
    short nameCursor;          /* 0x0E: name position being edited */
    u8  dispChar[2];           /* 0x10: the letter under the cursor, as a string */
    int cursorBlinkFrames;     /* 0x14: frame counter for the blinking cursor */
#ifdef NETPLAY
    signed char enteringName;  /* 0x18: name entry in progress (signed in the network build) */
#else
    u8  enteringName;          /* 0x18: name entry in progress */
#endif
    u8  firstAlphabetIndex;    /* 0x19: lowest cursor position allowed */
    u8  storeRecords;          /* 0x1A: a new record is waiting to be stored */
    u8  pad1B;
    int finishDelayFrames;     /* 0x1C: frames since the entry finished */
    int inputDelayFrames;      /* 0x20: frames until the pad is read again */
    int totalTime;             /* 0x24: total time of the race */
    u8  highScore;             /* 0x28 */
    u8  displayHiScore;        /* 0x29 */
    short nameEndX;            /* 0x2A: x position after the letters entered so far */
    u8  confirm;               /* 0x2C */
    u8  lConfirm;              /* 0x2D */
    u8  pad2E[2];
    int raceTime;              /* 0x30: race time for the race record */
    int lapTime;               /* 0x34: lap time for the lap record */
    u8  reserved38[8];
    u8  racePilot;             /* 0x40: ship that set the race record */
    u8  lapPilot;              /* 0x41: ship that set the lap record */
    u8  pad42[2];
#ifdef NETPLAY
    signed char pendingRecords; /* 0x44: records waiting for a name (1: lap, 2: race) */
    signed char displayedRecords; /* 0x45: records being shown on the score tab */
#else
    u8  pendingRecords;        /* 0x44: records waiting for a name (1: lap, 2: race) */
    u8  displayedRecords;      /* 0x45: records being shown on the score tab */
#endif
};

/* One best-time entry of the PC high-score table: race and lap records for a track and ship class. */
typedef struct HiScore {
    int         time;           /* 0x00: race time */
    int         lapTime;        /* 0x04: lap time */
    char        name[4];        /* 0x08: race record holder, NUL terminated */
    char        lapName[4];     /* 0x0C: lap record holder, NUL terminated */
    signed char ship;           /* 0x10: ship that set the race record */
    signed char lapShip;        /* 0x11: ship that set the lap record */
    char        reserved12[2];
} HiScore;

/* The best times of every track slot (tracks 8 to 15 are the reversed-track records) and class. */
extern HiScore hiScoreTable[16][4];

/* Default high-score table: a three-letter name (NUL padded) for each of eight track
   slots and four ship classes. Tracks 8 to 15 start with the entries of tracks 0 to 7. */
typedef struct HiScoreNames {
    char name[8][4][4];
} HiScoreNames;

/* Default race or lap times for the same eight track slots and four classes. */
typedef struct HiScoreTimes {
    s32 time[8][4];
} HiScoreTimes;

#endif
