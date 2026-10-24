#include "functions.h"
#include "hud_state.h"
#include "hiscores.h"
#include "menus.h"
#include "font.h"
#include "sound_samples.h"
#include <string.h>

extern int gameType;
extern int blinkFlag;
extern int nameDrawFirst;
extern GlyphWidth glyphWidth[];
extern short g_keyboardCode;
extern unsigned char raceCompleteFlag;
extern unsigned char flyingFlags[];
extern unsigned char nameCharPrev[];
extern char GetKeyboardChar(void);
void DrawHallOfFame(PcGameData *config, HiScoreData *score);

/* Fills the high-score table with the default records: a name and best race and lap
   time for each track slot and class, and no ship. Tracks 8 to 15 repeat tracks 0 to 7. */
#ifdef NETPLAY
#pragma intrinsic(strcpy)
#endif
#ifndef NETPLAY
void ScoreTabInit(PcGameData *gameData)
{
    HiScoreNames names = {{
        {"ROB", "NIK", "ASH", "EVE"},
        {"POL", "AJS", "ASH", "DAV"},
        {"NIC", "JON", "PAT", "JEF"},
        {"MIC", "TIM", "JEN", "JIM"},
        {"STU", "NIK", "PAT", "CJR"},
        {"TWE", "SUE", "ROB", "DAR"},
        {"\0", "\0", "AJS", "NIK"},
        {"\0", "\0", "POL", "CJR"}
    }};
    HiScoreTimes raceTimes = {{
        {0x1428, 0x1A04, 0x1E00, 0x2328},
        {0x1590, 0x1C20, 0x22B0, 0x27D8},
        {0x2058, 0x2DB4, 0x3660, 0x3714},
        {0x1F68, 0x28C8, 0x31B0, 0x34BC},
        {0x2058, 0x2C4C, 0x3480, 0x3E1C},
        {0x1D88, 0x2814, 0x31B0, 0x396C},
        {0, 0, 0x2C10, 0x3264},
        {0, 0, 0x2D00, 0x3390}
    }};
    HiScoreTimes lapTimes = {{
        {0x09D8, 0x0870, 0x0744, 0x06CC},
        {0x0A8C, 0x0924, 0x0870, 0x07BC},
        {0x0FF0, 0x0F00, 0x0D5C, 0x0AC8},
        {0x0F78, 0x0D5C, 0x0C30, 0x0A50},
        {0x0FF0, 0x0E88, 0x0CE4, 0x0C30},
        {0x0E88, 0x0D20, 0x0C30, 0x0B40},
        {0, 0, 0x0AC8, 0x09D8},
        {0, 0, 0x0B04, 0x0A14}
    }};
    int track, trackSlot, cls;


    for (track = 0; track < 16; track++) {
        trackSlot = (track >= 8) ? track - 8 : track;
        for (cls = 0; cls < 4; cls++) {
            strcpy(hiScoreTable[track][cls].name, names.name[trackSlot][cls]);
            hiScoreTable[track][cls].time = raceTimes.time[trackSlot][cls];
            strcpy(hiScoreTable[track][cls].lapName, names.name[trackSlot][cls]);
            hiScoreTable[track][cls].lapTime = lapTimes.time[trackSlot][cls];
            hiScoreTable[track][cls].lapShip = 15;
            hiScoreTable[track][cls].ship = 15;
        }
    }
}
#endif

#ifdef NETPLAY
void ScoreTabInit(PcGameData *gameData)
{
    int slot;
    HiScoreTimes raceRecordTimes;
    union {
        HiScoreNames table;
        u32 words[32];
        u16 halves[64];
    } nameTable;
    int track;
    int cls;
    HiScoreTimes lapTimes;
    extern u32 scoreNameTrack0Class0, scoreNameTrack0Class1, scoreNameTrack0Class2, scoreNameTrack0Class3;
    extern u32 scoreNameTrack1Class0, scoreNameTrack1Class1, scoreNameTrack1Class2, scoreNameTrack1Class3;
    extern u32 scoreNameTrack2Class0, scoreNameTrack2Class1, scoreNameTrack2Class2, scoreNameTrack2Class3;
    extern u32 scoreNameTrack3Class0, scoreNameTrack3Class1, scoreNameTrack3Class2, scoreNameTrack3Class3;
    extern u32 scoreNameTrack4Class0, scoreNameTrack4Class1, scoreNameTrack4Class2, scoreNameTrack4Class3;
    extern u32 scoreNameTrack5Class0, scoreNameTrack5Class1, scoreNameTrack5Class2, scoreNameTrack5Class3;
    extern u16 scoreNameTrack6Class0, scoreNameTrack6Class1, scoreNameTrack7Class0, scoreNameTrack7Class1;
    extern u32 scoreNameTrack6Class2, scoreNameTrack6Class3, scoreNameTrack7Class2, scoreNameTrack7Class3;

    nameTable.words[0] = scoreNameTrack0Class0;
    nameTable.words[1] = scoreNameTrack0Class1;
    nameTable.words[2] = scoreNameTrack0Class2;
    nameTable.words[3] = scoreNameTrack0Class3;
    nameTable.words[4] = scoreNameTrack1Class0;
    nameTable.words[5] = scoreNameTrack1Class1;
    nameTable.words[6] = scoreNameTrack1Class2;
    nameTable.words[7] = scoreNameTrack1Class3;
    nameTable.words[8] = scoreNameTrack2Class0;
    nameTable.words[9] = scoreNameTrack2Class1;
    nameTable.words[10] = scoreNameTrack2Class2;
    nameTable.words[11] = scoreNameTrack2Class3;
    nameTable.words[12] = scoreNameTrack3Class0;
    nameTable.words[13] = scoreNameTrack3Class1;
    nameTable.words[14] = scoreNameTrack3Class2;
    nameTable.words[15] = scoreNameTrack3Class3;
    nameTable.words[16] = scoreNameTrack4Class0;
    nameTable.words[17] = scoreNameTrack4Class1;
    nameTable.words[18] = scoreNameTrack4Class2;
    nameTable.words[19] = scoreNameTrack4Class3;
    nameTable.words[20] = scoreNameTrack5Class0;
    nameTable.words[21] = scoreNameTrack5Class1;
    nameTable.words[22] = scoreNameTrack5Class2;
    nameTable.words[23] = scoreNameTrack5Class3;
    nameTable.halves[48] = scoreNameTrack6Class0;
    nameTable.halves[49] = 0;
    nameTable.halves[50] = scoreNameTrack6Class1;
    nameTable.halves[51] = 0;
    nameTable.words[26] = scoreNameTrack6Class2;
    nameTable.words[27] = scoreNameTrack6Class3;
    nameTable.halves[56] = scoreNameTrack7Class0;
    nameTable.halves[57] = 0;
    nameTable.halves[58] = scoreNameTrack7Class1;
    nameTable.halves[59] = 0;
    nameTable.words[30] = scoreNameTrack7Class2;
    nameTable.words[31] = scoreNameTrack7Class3;
    raceRecordTimes.time[0][0] = 0x1428;
    raceRecordTimes.time[0][1] = 0x1A04;
    raceRecordTimes.time[0][2] = 0x1E00;
    raceRecordTimes.time[0][3] = 0x2328;
    raceRecordTimes.time[1][0] = 0x1590;
    raceRecordTimes.time[1][1] = 0x1C20;
    raceRecordTimes.time[1][2] = 0x22B0;
    raceRecordTimes.time[1][3] = 0x27D8;
    raceRecordTimes.time[2][0] = 0x2058;
    raceRecordTimes.time[2][1] = 0x2DB4;
    raceRecordTimes.time[2][2] = 0x3660;
    raceRecordTimes.time[2][3] = 0x3714;
    raceRecordTimes.time[3][0] = 0x1F68;
    raceRecordTimes.time[3][1] = 0x28C8;
    raceRecordTimes.time[3][2] = 0x31B0;
    raceRecordTimes.time[3][3] = 0x34BC;
    raceRecordTimes.time[4][0] = 0x2058;
    raceRecordTimes.time[4][1] = 0x2C4C;
    raceRecordTimes.time[4][2] = 0x3480;
    raceRecordTimes.time[4][3] = 0x3E1C;
    raceRecordTimes.time[5][0] = 0x1D88;
    raceRecordTimes.time[5][1] = 0x2814;
    raceRecordTimes.time[5][2] = 0x31B0;
    raceRecordTimes.time[5][3] = 0x396C;
    raceRecordTimes.time[6][0] = 0;
    raceRecordTimes.time[6][1] = 0;
    raceRecordTimes.time[6][2] = 0x2C10;
    raceRecordTimes.time[6][3] = 0x3264;
    raceRecordTimes.time[7][0] = 0;
    raceRecordTimes.time[7][1] = 0;
    raceRecordTimes.time[7][2] = 0x2D00;
    raceRecordTimes.time[7][3] = 0x3390;
    lapTimes.time[0][0] = 0x09D8;
    lapTimes.time[0][1] = 0x0870;
    lapTimes.time[0][2] = 0x0744;
    lapTimes.time[0][3] = 0x06CC;
    lapTimes.time[1][0] = 0x0A8C;
    lapTimes.time[1][1] = 0x0924;
    lapTimes.time[1][2] = 0x0870;
    lapTimes.time[1][3] = 0x07BC;
    lapTimes.time[2][0] = 0x0FF0;
    lapTimes.time[2][1] = 0x0F00;
    lapTimes.time[2][2] = 0x0D5C;
    lapTimes.time[2][3] = 0x0AC8;
    lapTimes.time[3][0] = 0x0F78;
    lapTimes.time[3][1] = 0x0D5C;
    lapTimes.time[3][2] = 0x0C30;
    lapTimes.time[3][3] = 0x0A50;
    lapTimes.time[4][0] = 0x0FF0;
    lapTimes.time[4][1] = 0x0E88;
    lapTimes.time[4][2] = 0x0CE4;
    lapTimes.time[4][3] = 0x0C30;
    lapTimes.time[5][0] = 0x0E88;
    lapTimes.time[5][1] = 0x0D20;
    lapTimes.time[5][2] = 0x0C30;
    lapTimes.time[5][3] = 0x0B40;
    lapTimes.time[6][0] = 0;
    lapTimes.time[6][1] = 0;
    lapTimes.time[6][2] = 0x0AC8;
    lapTimes.time[6][3] = 0x09D8;
    lapTimes.time[7][0] = 0;
    lapTimes.time[7][1] = 0;
    lapTimes.time[7][2] = 0x0B04;
    lapTimes.time[7][3] = 0x0A14;

    slot = 0;
    for (track = 0; track < 16; track++) {
        if (track >= 8)
            slot = track - 8;
        else
            slot = track;
        for (cls = 0; cls < 4; cls++) {
            strcpy(hiScoreTable[track][cls].name, nameTable.table.name[slot][cls]);
            hiScoreTable[track][cls].time = raceRecordTimes.time[slot][cls];
            strcpy(hiScoreTable[track][cls].lapName, nameTable.table.name[slot][cls]);
            hiScoreTable[track][cls].lapTime = lapTimes.time[slot][cls];
            hiScoreTable[track][cls].lapShip = 15;
            hiScoreTable[track][cls].ship = hiScoreTable[track][cls].lapShip;
        }
    }
}
#endif
#ifdef NETPLAY
#pragma function(strcpy)
#endif

#ifdef NETPLAY
/* Clears the state used by high-score entry and display. */
void InitScoreTabData(HiScoreData *score)
{
    score->nameEntered[0] = 0;
    score->selectedChar = 'A';
    score->alphabetIndex = 0;
    score->dispChar[0] = 0;
    score->cursorBlinkFrames = 0;
    score->enteringName = 1;
    score->firstAlphabetIndex = 0;
    score->storeRecords = 1;
    score->finishDelayFrames = 0;
    score->inputDelayFrames = 0;
    score->totalTime = 0;
    score->pendingRecords = 0;
    score->displayedRecords = 0;
    score->highScore = 0;
    score->nameEndX = 40;
    score->confirm = 2;
    score->lConfirm = 0;
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Clears the state used by high-score entry and display. */
void InitScoreTabData(HiScoreData *score)
{
    score->alphabetIndex = 0;
    score->nameEntered[0] = 0;
    score->dispChar[0] = 0;
    score->cursorBlinkFrames = 0;
    score->enteringName = 1;
    score->firstAlphabetIndex = 0;
    score->storeRecords = 1;
    score->finishDelayFrames = 0;
    score->inputDelayFrames = 0;
    score->totalTime = 0;
    score->pendingRecords = 0;
    score->displayedRecords = 0;
    score->highScore = 0;
    score->selectedChar = 'A';
    score->confirm = 2;
    score->nameEndX = 40;
    score->lConfirm = 0;
}
#endif
#endif

extern signed char classLaps[];
extern int myBestLap, myBestRace;
extern short ownShip;

/* After a race: notes whether the player has beaten the stored lap or race record of the track and class. */
#ifndef NETPLAY
void CheckForHiScore(PcGameData *config, HiScoreData *score, ShipData *ships, int shipNum, u_char lap)
{
    int trackNum;
    int i;

    if (gameType == 1)
        trackNum = config->trackNum + 8;
    else
        trackNum = config->trackNum;
    score->totalTime = 0;
    if (lap == 1) {
        if (ships[shipNum].lap == 2 || !score->pendingRecords) {
            if (ships[shipNum].lapTimes[ships[shipNum].lap - 2] < hiScoreTable[trackNum][config->shipClass].lapTime) {
                score->lapTime = ships[ownShip].lapTimes[ships[ownShip].lap - 2];
                score->lapPilot = ownShip;
                score->pendingRecords = score->pendingRecords | HISCORE_LAP_RECORD;
                score->displayedRecords = score->pendingRecords;
            }
        } else if (ships[shipNum].lapTimes[ships[shipNum].lap - 2] < score->lapTime) {
            score->lapTime = ships[ownShip].lapTimes[ships[ownShip].lap - 2];
            myBestLap = score->lapTime;
            score->lapPilot = ownShip;
            score->pendingRecords = score->pendingRecords | HISCORE_LAP_RECORD;
            score->displayedRecords = score->pendingRecords;
        }
    } else {
        for (i = 0; i < classLaps[config->shipClass]; i++)
            score->totalTime += savedLapTimes[i];
        if (score->totalTime < hiScoreTable[trackNum][config->shipClass].time) {
            score->raceTime = score->totalTime;
            myBestRace = score->totalTime;
            score->racePilot = ownShip;
            score->pendingRecords = score->pendingRecords | HISCORE_RACE_RECORD;
            score->displayedRecords = score->pendingRecords;
        }
    }
}
#endif

#ifdef NETPLAY
void CheckForHiScore(PcGameData *config, HiScoreData *score, ShipData *ships, int shipNum, u_char lap)
{
    int trackNum;
    int i;

    trackNum = 0;
    if (gameType == 1)
        trackNum = config->trackNum + 8;
    else
        trackNum = config->trackNum;
    score->totalTime = 0;
    if ((signed char)lap == 1) {
        if (ships[shipNum].lap == 2 || !score->pendingRecords) {
            if (ships[shipNum].lapTimes[ships[shipNum].lap - 2] < hiScoreTable[trackNum][config->shipClass].lapTime) {
                score->lapTime = ships[ownShip].lapTimes[ships[ownShip].lap - 2];
                score->lapPilot = ownShip;
                score->pendingRecords = score->pendingRecords | HISCORE_LAP_RECORD;
                score->displayedRecords = score->pendingRecords;
            }
        } else if (ships[shipNum].lapTimes[ships[shipNum].lap - 2] < score->lapTime) {
            score->lapTime = ships[ownShip].lapTimes[ships[ownShip].lap - 2];
            score->lapPilot = ownShip;
            myBestLap = score->lapTime;
            score->pendingRecords = score->pendingRecords | HISCORE_LAP_RECORD;
            score->displayedRecords = score->pendingRecords;
        }
    } else {
        for (i = 0; i < classLaps[config->shipClass]; i++)
            score->totalTime += savedLapTimes[i];
        if (score->totalTime < hiScoreTable[trackNum][config->shipClass].time) {
            score->raceTime = score->totalTime;
            myBestRace = score->raceTime;
            score->racePilot = ownShip;
            score->pendingRecords = score->pendingRecords | HISCORE_RACE_RECORD;
            score->displayedRecords = score->pendingRecords;
        }
    }
    return;
}
#endif

/* Runs high-score name entry while a new record is pending. */
#ifndef NETPLAY
void EnterHighScoreIfNeeded(PcGameData *config, HiScoreData *score)
{
    if (score->pendingRecords != 0) {
        EnterHighScore(config, score);
    }
}
#endif

#ifdef NETPLAY
void EnterHighScoreIfNeeded(PcGameData *config, HiScoreData *score)
{
    /* The original initializes this counter but never reads it. */
    char count = 20;
    /* The original also computes the track and class without reading them here. */
    int track = 0;
    int classIndex = 0;

    if (gameType == 1)
        track = config->trackNum + 8;
    else
        track = config->trackNum;
    classIndex = config->shipClass;

    if (score->pendingRecords != 0) {
        EnterHighScore(config, score);
    }
}
#endif

/* Draws the race and lap records for the current track and class. */
#ifndef NETPLAY
void DisplayScoreTab(PcGameData *config, HiScoreData *score)
{
    int track;
    int cls;

    if (gameType == 1) track = config->trackNum + 8;
    else track = config->trackNum;
    cls = config->shipClass;
    if (score->pendingRecords != 0) {
        DrawHallOfFame(config, score);
    }
    if (score->displayedRecords != 0) {
        CentreText("ENTER PLAYER NAME", 0x28, 0x28, 8, 1);
        DoButton(0x85, 0xD2, 2, "FINISH");
    } else {
        CentreText("PLAYER TRACK RECORDS", 0x28, 0x28, 8, 1);
    }
    if (score->displayedRecords & HISCORE_RACE_RECORD) {
        if (blinkFlag) {
            AddText("NEW RACE RECORD", 0x28, 0x3C, 8, 0);
        }
        AddTime(score->raceTime, 0xA0, 0x50, 8, 0, 1);
    } else {
        AddText("RACE RECORD", 0x28, 0x3C, 8, 0);
        AddText(hiScoreTable[track][cls].name, 0x28, 0x50, 8, 0);
        AddTime(hiScoreTable[track][cls].time, 0xA0, 0x50, 8, 0, 1);
    }
    if (score->displayedRecords & HISCORE_LAP_RECORD) {
        if (blinkFlag) {
            AddText("NEW LAP RECORD", 0x28, 0x64, 8, 0);
        }
        AddTime(score->lapTime, 0xA0, 0x78, 8, 0, 1);
    } else {
        AddText("LAP RECORD", 0x28, 0x64, 8, 0);
        AddText(hiScoreTable[track][cls].lapName, 0x28, 0x78, 8, 0);
        AddTime(hiScoreTable[track][cls].lapTime, 0xA0, 0x78, 8, 0, 1);
    }
}
#endif

#ifdef NETPLAY
void DisplayScoreTab(PcGameData *config, HiScoreData *score)
{
    /* The original initializes this counter but never reads it. */
    char count = 20;
    int track = 0;
    int classIndex = 0;

    if (gameType == 1) track = config->trackNum + 8;
    else track = config->trackNum;
    classIndex = config->shipClass;
    if (score->pendingRecords != 0) {
        DrawHallOfFame(config, score);
    }
    if (score->displayedRecords != 0) {
        CentreText("ENTER PLAYER NAME", 0x28, 0x28, 8, 1);
        DoButton(0x85, 0xD2, 2, "FINISH");
    } else {
        CentreText("PLAYER TRACK RECORDS", 0x28, 0x28, 8, 1);
    }
    if (score->displayedRecords & HISCORE_RACE_RECORD) {
        if (blinkFlag) {
            AddText("NEW RACE RECORD", 0x28, 0x3C, 8, 0);
        }
        AddTime(score->raceTime, 0xA0, 0x50, 8, 0, 1);
    } else {
        AddText("RACE RECORD", 0x28, 0x3C, 8, 0);
        AddText(hiScoreTable[track][classIndex].name, 0x28, 0x50, 8, 0);
        AddTime(hiScoreTable[track][classIndex].time, 0xA0, 0x50, 8, 0, 1);
    }
    if (score->displayedRecords & HISCORE_LAP_RECORD) {
        if (blinkFlag) {
            AddText("NEW LAP RECORD", 0x28, 0x64, 8, 0);
        }
        AddTime(score->lapTime, 0xA0, 0x78, 8, 0, 1);
    } else {
        AddText("LAP RECORD", 0x28, 0x64, 8, 0);
        AddText(hiScoreTable[track][classIndex].lapName, 0x28, 0x78, 8, 0);
        AddTime(hiScoreTable[track][classIndex].lapTime, 0xA0, 0x78, 8, 0, 1);
    }
}
#endif

/* Handles name entry and stores a new race or lap record. */
#ifdef NETPLAY
#pragma intrinsic(strcpy)
#endif
#ifndef NETPLAY
void EnterHighScore(PcGameData *config, HiScoreData *score)
{
    char  ch;
    short scoreTrack;
    int   i;
    short x;

    if (score->enteringName) {
        if (g_keyboardCode == 0xE /* DIK_BACK */ && score->nameCursor > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
            score->nameCursor--;
            g_keyboardCode = -1;
        }
        ch = GetKeyboardChar();
        if (ch != 0 && ch != '-' && score->nameCursor < 3) {
            PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
            score->nameEntered[score->nameCursor] = ch;
            score->nameCursor++;
        }
        if (GetMenuKey(2) && score->nameCursor > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
            score->nameCursor--;
        } else if (GetMenuKey(3) && score->nameCursor < 2) {
            PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
            score->nameCursor++;
        } else if (GetMenuKey(0)) {
            if (score->nameCursor < 3) {
                PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
                score->nameEntered[score->nameCursor] = flyingFlags[score->nameEntered[score->nameCursor]];
            }
        } else if (GetMenuKey(1) && score->nameCursor < 3) {
            PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
            score->nameEntered[score->nameCursor] = nameCharPrev[score->nameEntered[score->nameCursor]];
        }
        if (GetMenuKey(5)) {
            PlayNote(SAMPLE_MENU_SELECT, 0xFF, 0xF, 0xF, 0x100);
            score->enteringName = 0;
        }
    }
    else if (score->storeRecords) {
        score->enteringName = 0;
        score->nameEntered[3] = 0;
        if (gameType == 1)
            scoreTrack = config->trackNum + 8;
        else
            scoreTrack = config->trackNum;
        if (score->pendingRecords & HISCORE_RACE_RECORD) {
            strcpy(hiScoreTable[scoreTrack][config->shipClass].name, score->nameEntered);
            hiScoreTable[scoreTrack][config->shipClass].time = score->raceTime;
            hiScoreTable[scoreTrack][config->shipClass].ship = score->racePilot;
        }
        if (score->pendingRecords & HISCORE_LAP_RECORD) {
            strcpy(hiScoreTable[scoreTrack][config->shipClass].lapName, score->nameEntered);
            hiScoreTable[scoreTrack][config->shipClass].lapTime = score->lapTime;
            hiScoreTable[scoreTrack][config->shipClass].lapShip = score->lapPilot;
        }
        score->displayedRecords = 0;
        score->alphabetIndex = 0;
        score->storeRecords = 0;
    }
    if (score->enteringName) {
        x = 0x28;
        for (i = 0; i < score->nameCursor; i++) {
            if (glyphIndex.character[score->nameEntered[i]] != 0x32) {
                x += glyphWidth[(signed char)glyphIndex.character[score->nameEntered[i]]].width >> 1;
            } else {
                x += 8;
            }
        }
    } else {
        if (score->finishDelayFrames > 2) {
            if (GetMenuKey(5)) {
                score->nameEndX = 0x28;
                score->highScore = 0;
                score->totalTime = 0;
                score->pendingRecords = 0;
                config->complete = 1;
                raceCompleteFlag = 1;
                PlayNote(SAMPLE_MENU_SELECT, 0xFF, 0xF, 0xF, 0x100);
            }
        } else {
            score->finishDelayFrames++;
        }
    }
}
#endif

#ifdef NETPLAY
void EnterHighScore(PcGameData *config, HiScoreData *score)
{
    char characterText[2];
    short trackNum;
    char character;
    short nameLeft;
    /* The original initializes recordX and height but never reads them here. */
    short recordX;
    short nameCursorX;
    int i;
    short height = 0x10;

    nameLeft = 0x28;
    recordX = 0xa0;
    trackNum = 0;

    if (score->enteringName) {
        if (g_keyboardCode == 0xE /* DIK_BACK */ && score->nameCursor > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
            score->nameCursor--;
            g_keyboardCode = -1;
        }
        character = GetKeyboardChar();
        if (character != 0 && character != '-' && score->nameCursor < 3) {
            PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
            score->nameEntered[score->nameCursor] = character;
            score->nameCursor++;
        }
        if (GetMenuKey(2) && score->nameCursor > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
            score->nameCursor--;
        } else if (GetMenuKey(3) && score->nameCursor < 2) {
            PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
            score->nameCursor++;
        } else if (GetMenuKey(0)) {
            if (score->nameCursor < 3) {
                PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
                score->nameEntered[score->nameCursor] = flyingFlags[score->nameEntered[score->nameCursor]];
            }
        } else if (GetMenuKey(1) && score->nameCursor < 3) {
            PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
            score->nameEntered[score->nameCursor] = nameCharPrev[score->nameEntered[score->nameCursor]];
        }
        if (GetMenuKey(5)) {
            PlayNote(SAMPLE_MENU_SELECT, 0xFF, 0xF, 0xF, 0x100);
            score->enteringName = 0;
        }
    }
    else if ((signed char)score->storeRecords) {
        score->enteringName = 0;
        score->nameEntered[3] = 0;
        if (gameType == 1)
            trackNum = config->trackNum + 8;
        else
            trackNum = config->trackNum;
        if (score->pendingRecords & HISCORE_RACE_RECORD) {
            strcpy(hiScoreTable[trackNum][config->shipClass].name, score->nameEntered);
            hiScoreTable[trackNum][config->shipClass].time = score->raceTime;
            hiScoreTable[trackNum][config->shipClass].ship = score->racePilot;
        }
        if (score->pendingRecords & HISCORE_LAP_RECORD) {
            strcpy(hiScoreTable[trackNum][config->shipClass].lapName, score->nameEntered);
            hiScoreTable[trackNum][config->shipClass].lapTime = score->lapTime;
            hiScoreTable[trackNum][config->shipClass].lapShip = score->lapPilot;
        }
        score->displayedRecords = 0;
        score->alphabetIndex = 0;
        score->storeRecords = 0;
    }
    if (score->enteringName) {
        nameCursorX = nameLeft;
        for (i = 0; i < score->nameCursor; i++) {
            character = glyphIndex.character[score->nameEntered[i]];
            if (character != 0x32) {
                nameCursorX += glyphWidth[character].width >> 1;
            } else {
                nameCursorX += 8;
            }
        }
        /* The original captures the cursor character even with its drawing code disabled. */
        characterText[0] = score->nameEntered[score->nameCursor];
        characterText[1] = 0;
    } else {
        if (score->finishDelayFrames > 2) {
            if (GetMenuKey(5)) {
                score->highScore = 0;
                score->nameEndX = 0x28;
                score->totalTime = 0;
                score->pendingRecords = 0;
                config->complete = 1;
                raceCompleteFlag = 1;
                PlayNote(SAMPLE_MENU_SELECT, 0xFF, 0xF, 0xF, 0x100);
            }
        } else {
            score->finishDelayFrames++;
        }
    }
}
#endif
#ifdef NETPLAY
#pragma function(strcpy)
#endif

/* Hall of fame heading, with the name being entered (or the continue button once the name is stored). */
void DrawHallOfFame(PcGameData *config, HiScoreData *score)
{
#ifdef NETPLAY
    short i;
    signed char characterCode;
    char nameChar[2];
    short nameX;
    short nameY;
    short fontSize;
    int characterIndex;
    short namePos;

    fontSize = 8;
    nameX = 0x28;
    nameY = 0xa0;
    i = 0;
    CentreText("HALL OF FAME", 0x6B, 0x14, 8, 0);
    if (score->enteringName) {
        namePos = nameX;
        for (characterIndex = 0; characterIndex < score->nameCursor; characterIndex++) {
            characterCode = glyphIndex.character[score->nameEntered[characterIndex]];
            if (characterCode != 0x32) {
                namePos += glyphWidth[characterCode].width >> 1;
            } else {
                namePos += 8;
            }
        }
        nameChar[0] = score->nameEntered[score->nameCursor];
        nameChar[1] = 0;
        if (nameDrawFirst) {
            AddText(score->nameEntered, nameX, nameY, fontSize, 0);
        }
        AddText(nameChar, namePos, nameY, fontSize, 1);
        if (!nameDrawFirst) {
            AddText(score->nameEntered, nameX, nameY, fontSize, 0);
        }
        AddTime(score->totalTime, 0xA5, nameY, 8, 1, 1);
    } else {
        DoButton(0x7F, 0xD2, 2, "CONTINUE");
    }
#else
    char   nameChar[2];
    short  x;
    int    i;

    CentreText("HALL OF FAME", 0x6B, 0x14, 8, 0);
    if (score->enteringName) {
        x = 0x28;
        for (i = 0; i < score->nameCursor; i++) {
            if (glyphIndex.character[score->nameEntered[i]] != 0x32) {
                x += glyphWidth[(signed char)glyphIndex.character[score->nameEntered[i]]].width >> 1;
            } else {
                x += 8;
            }
        }
        nameChar[0] = score->nameEntered[score->nameCursor];
        nameChar[1] = 0;
        if (nameDrawFirst) {
            AddText(score->nameEntered, 0x28, 0xA0, 8, 0);
        }
        AddText(nameChar, x, 0xA0, 8, 1);
        if (!nameDrawFirst) {
            AddText(score->nameEntered, 0x28, 0xA0, 8, 0);
        }
        AddTime(score->totalTime, 0xA5, 0xA0, 8, 1, 1);
    } else {
        DoButton(0x7F, 0xD2, 2, "CONTINUE");
    }
#endif
}
