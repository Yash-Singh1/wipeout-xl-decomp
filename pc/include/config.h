#ifndef CONFIG_H
#define CONFIG_H

typedef struct Password {
    signed char digits[13];
} Password;

/* Shared PC race, menu and rendering configuration. */
typedef struct ConfigData {
    signed char trackNum;          /* 0x00 */
    signed char actTrackNum;
    char shipNum;
    signed char shipClass;         /* 0x03 */
    union { char initialConfigByte; short initialConfigValue; }; /* 0x04 */
    signed char cdTrack;           /* 0x06 */
    signed char soundMode;
    union { short raceInfo; short raceCount; }; /* 0x08 */
    union { short complete; short finished; };  /* 0x0a */
    short highScore;               /* 0x0c */
    short musicVolume;             /* 0x0e */
    short effectVolume;            /* 0x10 */
    short otherMusicVolume;        /* 0x12 */
    short otherEffectVolume;       /* 0x14 */
    char paused;                   /* 0x16 */
    union { unsigned char weaponsOn; char weaponsOnSigned; }; /* 0x17 */
    union { unsigned char internalView; signed char cameraInternalView; }; /* 0x18 */
    signed char trailDetail;       /* 0x19 */
#ifdef NETPLAY
    union { unsigned char skyEnabled; signed char skyEnabledSigned; }; /* 0x1a: the network build's DoDetailKeys reads it signed */
#else
    unsigned char skyEnabled;             /* 0x1a */
#endif
    union { signed char viewDetail; signed char detailLevel; }; /* 0x1b */
#ifdef NETPLAY
    union { unsigned char texturesOn; signed char texturesOnSigned; }; /* 0x1c: the network build's track code reads it signed */
#else
    unsigned char texturesOn;             /* 0x1c */
#endif
    char extraGraphicsOption;              /* 0x1d */
#ifdef NETPLAY
    union { unsigned char smoothTextures; signed char smoothTexturesSigned; }; /* 0x1e: the network build's DoDetailKeys reads it signed */
#else
    unsigned char smoothTextures;             /* 0x1e */
#endif
    char name[9];                  /* 0x1f */
    union { unsigned char venomComplete; char phantomChallenge; }; /* 0x28 */
    union { unsigned char rapierComplete; char piranhaChallenge; };
#ifdef NETPLAY
    union { unsigned char teamCheat; char teamCheatSigned; }; /* 0x2a: the network build tests it signed */
#else
    unsigned char teamCheat;              /* 0x2a */
#endif
    union { unsigned char class1Unlocked; char phantomUnlocked; };
    union { unsigned char class2Unlocked; char piranhaUnlocked; };
    union { signed char class1Race; char phantomCircuit; };
    union { signed char class2Race; char piranhaCircuit; };
    unsigned char savedClass;             /* 0x2f */
    unsigned char savedActTrackNum;       /* 0x30 */
    unsigned char savedTrackNum;          /* 0x31 */
#ifdef NETPLAY
    union { unsigned char trackCheat; char trackCheatSigned; }; /* 0x32: the network build tests it signed */
#else
    unsigned char trackCheat;              /* 0x32 */
#endif
#ifdef NETPLAY
    union { unsigned char phantomCheat; char phantomCheatSigned; }; /* 0x33: the network build tests it signed */
#else
    unsigned char phantomCheat;              /* 0x33 */
#endif
} ConfigData;

typedef ConfigData RaceConfig;


#endif
