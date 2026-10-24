#include "functions.h"
#include "raceinit.h"
#include <string.h>


#pragma intrinsic(strcpy, strcat, memcpy)

int attractStep = 1;

extern TrackData *track;
extern ConfigData *gameData;
extern char errorString[];
extern char trackDirectory[];


TrackSection *NextIndexSection(TrackSection *section);

/* Sets the speed-up pad texture on its track faces. */
void SetSpeedUpFaceTexture(char texture)
{
    Face *face;
    TrackSection *sec;
    short i;
    short faceNumber;

    sec = track->sections;
    for (i = 0; i < track->sectionCount; i++) {
        face = track->faces + sec->faceStart;
        for (faceNumber = 0; faceNumber < sec->faceCount; faceNumber++) {
            if (face->flags & SPEED_UP) {
                face->texture = texture;
            }
            face++;
        }
        sec = NextIndexSection(sec);
    }
}

/* Sets the boost pad texture on its track faces. */
void SetBoostFaceTexture(short texture)
{
    Face *face;
    TrackSection *sec;
    short i;
    short faceNumber;
    char text[256];

    sec = track->sections;
    for (i = 0; i < track->sectionCount; i++) {
        face = track->faces + sec->faceStart;
        for (faceNumber = 0; faceNumber < sec->faceCount; faceNumber++) {
            if (face->flags & 0x40) {
                if (gameData->actTrackNum == 6 && sec->secNo == 256) {
                    /* track 6 keeps the texture of this one section */
                } else {
                    face->texture = texture + 1;
                }
            }
            face++;
        }
        sec = NextIndexSection(sec);
    }
}

/* Loads the compressed textures needed by the selected track. */
void InitTrackTextures(ConfigData *config)
{
#ifdef NETPLAY
    TrackSection *section;
    u_char *texData;
    int offset;
    int fileLength;
    char textureFilename[8];
    Face *faceCursor;
    short sectionIndex;
    short faceNumberCounter;
    char trackTextureBuffer[256];
#else
    char fname[256];
    char num[8];
    int offset;
    int length;
    short i;
    TrackSection *section;
    u_char *texData;
    Face *facePtr;
    short fn;
#endif

    offset = 0;
#ifdef NETPLAY
    strcpy(trackTextureBuffer, trackDirectory);
    wsprintf(textureFilename, "%02d", config->actTrackNum);
    strcat(trackTextureBuffer, textureFilename);
    strcat(trackTextureBuffer, "\\track.tex");
    fileLength = FileLength(trackTextureBuffer);
#else
    strcpy(fname, trackDirectory);
    wsprintf(num, "%02d", config->actTrackNum);
    strcat(fname, num);
    strcat(fname, "\\track.tex");
    length = FileLength(fname);
#endif
#ifdef NETPLAY
    if (fileLength <= 0) {
        FileAccessError(trackTextureBuffer);
#else
    if (length <= 0) {
        FileAccessError(fname);
#endif
        OutputDebugString("initrace.c: InitTrackTextures: File has zero length \n");
        return;
    }
    {
#ifdef NETPLAY
        texData = DAlloc(heap, fileLength + 0x20);
#else
        texData = DAlloc(heap, length + 0x20);
#endif
        if (texData == 0) {
            wsprintf(errorString, "initrace.c : InitTrackTextures (): Failed to allocate memory for file \n");
            Error(errorString, Fatal);
        }
#ifdef NETPLAY
        if (LoadFile(trackTextureBuffer, texData) != fileLength) {
#else
        if (LoadFile(fname, texData) != length) {
#endif
            wsprintf(errorString, "initrace.c:InitTrackTextures(): Failed to load file \n");
            Error(errorString, Fatal);
        }
        section = track->sections;
#ifdef NETPLAY
        for (sectionIndex = 0; sectionIndex < track->sectionCount; sectionIndex++) {
            faceCursor = track->faces + section->faceStart;
            for (faceNumberCounter = 0; faceNumberCounter < section->faceCount; faceNumberCounter++) {
                faceCursor->texture = texData[offset++];
                faceCursor->flags = texData[offset++];
                faceCursor++;
            }
            section = NextIndexSection(section);
        }
#else
        for (i = 0; i < track->sectionCount; i++) {
            facePtr = track->faces + section->faceStart;
            for (fn = 0; fn < section->faceCount; fn++) {
                facePtr->texture = texData[offset++];
                facePtr->flags = texData[offset++];
                facePtr++;
            }
            section = NextIndexSection(section);
        }
#endif
        DFree(heap, texData);
        texData = 0;
    }
}

/* Builds checkpoint records from the track configuration. */
void InitCheckPoints(CheckPoints *checkpoints, ConfigData *config)
{
#ifdef NETPLAY
    long length;
    char *buffer;
    char checkpointFileSource[256];
    short n;
    char suffix[8];
#else
    char path[256];
    char suffix[8];
    long length;
    char *buffer;
    short n;
#endif

#ifdef NETPLAY
    strcpy(checkpointFileSource, trackDirectory);
    wsprintf(suffix, "%02d", config->actTrackNum);
    strcat(checkpointFileSource, suffix);
    strcat(checkpointFileSource, "\\cpoint");
    wsprintf(suffix, "%01d", config->shipClass);
    strcat(checkpointFileSource, suffix);
    strcat(checkpointFileSource, ".chk");
#else
    strcpy(path, trackDirectory);
    wsprintf(suffix, "%02d", config->actTrackNum);
    strcat(path, suffix);
    strcat(path, "\\cpoint");
    wsprintf(suffix, "%01d", config->shipClass);
    strcat(path, suffix);
    strcat(path, ".chk");
#endif
    noCheckPoints = 0;
#ifdef NETPLAY
    length = FileLength(checkpointFileSource);
#else
    length = FileLength(path);
#endif
    if (length <= 0) {
#ifdef NETPLAY
        FileAccessError(checkpointFileSource);
#else
        FileAccessError(path);
#endif
        OutputDebugString("initrace.c: InitCheckPoints: File has zero length \n");
        noCheckPoints = 1;
        return;
    }
    buffer = DAlloc(heap, length + 32);
    if (!buffer) {
        wsprintf(errorString, "initrace.c : InitCheckPoints (): Failed to allocate memory for file \n");
        Error(errorString, Fatal);
    }
#ifdef NETPLAY
    if (LoadFile(checkpointFileSource, buffer) != length) {
#else
    if (LoadFile(path, buffer) != length) {
#endif
        wsprintf(errorString, "initrace.c:InitCheckPoints(): Failed to load file \n");
        Error(errorString, Fatal);
    }
    memcpy(checkpoints, buffer, 36);
    for (n = 0; n < 6; n++) {
        checkpoints[n].reached = -1;
    }
    DFree(heap, buffer);
    buffer = 0;
}


extern int attractDelayFrames;
extern int attractModeFlag;
extern CameraUpdateFunc CameraUpdate;

void UpdateCameraAttractMode(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);
void InitCredits(void);

/* Sets up the ships and camera for attract mode. */
void InitAttractMode(int mode, ShipData *ships)
{
    if (mode == 1) {
        attractStep++;
        ships[ownShip].update = UpdateDroneShipStart;
        ships[ownShip].attr &= ~VIEW_IN;
        ships[ownShip].attr &= ~RACING;
        CameraUpdate = UpdateCameraAttractMode;
        if (attractStep % 3 == 0) {
            attractDelayFrames = 3300;
            attractModeFlag = 1;
        } else {
            attractDelayFrames = 900;
        }
        if (attractModeFlag == 1) {
            InitCredits();
        }
    }
}

extern u_int randSeed;
extern int linkSendFlag;

extern int roadSectionCount;
extern int weaponGridSections;

/* Marks weapon grids and builds the runtime list of available pickups. */
void InitWeaponGrids(WeaponGrid *weaponGrid, TrackData *context, int *noWeaponGrids, short *specialCount, SpecialGrid *specialGrid)
{
    TrackSection *current;
    TrackSection *orig;
    Face *face;
    short more;
    short onBranch;
    short num;

    more = 1;
    onBranch = 1;
    current = context->sections;
    num = 0;
    roadSectionCount = 0;
    while (more) {
        current->secNo = num;
        if (gameData->actTrackNum == 7 && current->secNo >= 70 && current->secNo <= 86) {
            face = context->faces + current->faceStart;
            while (!(face->flags & TRACK_BASE)) {
                face++;
            }
            face->flags |= 0x40;
            face++;
            face->flags |= 0x40;
        }
        *noWeaponGrids = SetTrackAttributes(current, num, weaponGrid, *noWeaponGrids, specialCount, specialGrid);
        if (current->junction.index != -1 && (current->junction.ptr->flags & 16)) {
            orig = current;
            current = current->junction.ptr;
            while (onBranch) {
                num++;
                current->secNo = num;
                if (gameData->actTrackNum == 7 && current->secNo >= 70 && current->secNo <= 86) {
                    face = context->faces + current->faceStart;
                    while (!(face->flags & TRACK_BASE)) {
                        face++;
                    }
                    face->flags |= 0x40;
                    face++;
                    face->flags |= 0x40;
                }
                *noWeaponGrids = SetTrackAttributes(current, num, weaponGrid, *noWeaponGrids, specialCount, specialGrid);
                current = current->nextSection.ptr;
                if (current->junction.index != -1) {
                    onBranch = 0;
                }
            }
            onBranch = 1;
            current = orig;
            current = current->nextSection.ptr;
            num++;
            current->secNo = num;
            if (gameData->actTrackNum == 7 && current->secNo >= 70 && current->secNo <= 86) {
                face = context->faces + current->faceStart;
                while (!(face->flags & TRACK_BASE)) {
                    face++;
                }
                face->flags |= 0x40;
                face++;
                face->flags |= 0x40;
            }
            *noWeaponGrids = SetTrackAttributes(current, num, weaponGrid, *noWeaponGrids, specialCount, specialGrid);
            roadSectionCount++;
            if (current == context->sections) {
                more = 0;
            }
        }
        num++;
        roadSectionCount++;
        current = current->nextSection.ptr;
        if (current == context->sections) {
            more = 0;
        }
    }
    weaponGridSections = num;
}

/* Initializes the color bars used by the in-game menu. */
void InitInGameMenu(InGameMenuPolys *menu)
{
    short i;

    for (i = 0; i < 20; i++) {
        SetPolyG4(&menu->red[i]);
        menu->red[i].r0 = 0x3f;
        menu->red[i].g0 = 0;
        menu->red[i].b0 = 0;
        menu->red[i].r1 = 0x7f;
        menu->red[i].g1 = 0;
        menu->red[i].b1 = 0;
        menu->red[i].r2 = 0xbf;
        menu->red[i].g2 = 0;
        menu->red[i].b2 = 0;
        menu->red[i].r3 = 0xff;
        menu->red[i].g3 = 0;
        menu->red[i].b3 = 0;
        SetPolyG4(&menu->green[i]);
        menu->green[i].r0 = 0;
        menu->green[i].g0 = 0x3f;
        menu->green[i].b0 = 0;
        menu->green[i].r1 = 0;
        menu->green[i].g1 = 0x7f;
        menu->green[i].b1 = 0;
        menu->green[i].r2 = 0;
        menu->green[i].g2 = 0xbf;
        menu->green[i].b2 = 0;
        menu->green[i].r3 = 0;
        menu->green[i].g3 = 0xff;
        menu->green[i].b3 = 0;
    }
    SetPolyG4(&menu->footer);
    menu->footer.r0 = 0x20;
    menu->footer.g0 = 0x20;
    menu->footer.b0 = 0x20;
    menu->footer.r1 = 0x40;
    menu->footer.g1 = 0x40;
    menu->footer.b1 = 0x40;
    menu->footer.r2 = 0x40;
    menu->footer.g2 = 0x40;
    menu->footer.b2 = 0x40;
    menu->footer.r3 = 0x20;
    menu->footer.g3 = 0x20;
    menu->footer.b3 = 0x20;
    SetSemiTrans(&menu->footer, 1);
    SetDrawMode((DR_MODE *)menu->textureWindow, 1, 1, GetTPage(2, 2, 0, 0), 0);
}

/* Returns the next value from the game's seeded random sequence. */
int myrand(void)
{
    u_int nextSeed;

    nextSeed = (randSeed >> 1) ^ (randSeed << 3);
    randSeed = nextSeed;
    return randSeed & 0xFFFF;
}

/* Resets per-race state and initializes the race data record. */
void InitRaceData(struct RaceData *race)
{
    race->selection = -1;
    race->showOptions = 1;
    race->musicPaused = 0;
    race->command = 0;
    race->pad0cLow = 0;
    race->pad0cHigh = 0;
    linkSendFlag = 0;
}

/* Captures the initial state used when restarting a race. */
void InitRaceResetData(struct ResetData *reset, ShipData *ships, ConfigData *config, WeaponData *weapons, WeaponShape *weaponShapes, RescueData *rescue, Object **shipShapes, CheckPoints *checkPoints)
{
    reset->ships = ships;
    reset->config = config;
    reset->weapons = weapons;
    reset->weaponShapes = weaponShapes;
    reset->rescue = rescue;
    reset->shipShapes = shipShapes;
    reset->checkPoints = checkPoints;
}

extern int cdMusicCount;
extern signed char cdMusicTable[];
extern signed char cdMusicIndex;
extern int cdMusicTrack;
extern int trackNameScroll;

void CdMusicPlay(int track, int mode);

/* Chooses the starting track for shuffled CD playback. */
void InitShufflePlay(void)
{
    int trackIndex;
    int i;
    int found;
    int n;

    randSeed = 0x12345678;
#ifdef NETPLAY
    if (networkGame)
        return;
#endif
    if (cdMusicCount == 0) {
        n = cdMusicCount;
    } else {
        n = cdMusicCount - 1;
    }
    if (cdMusicCount == 0) {
        for (i = 0; i < 25; i++) {
            cdMusicTable[i] = 0;
        }
    } else {
        for (i = 0; i < n; i++) {
            cdMusicTable[i] = 0;
        }
    }
    for (i = 0; i < n; i++) {
        found = 0;
        while (found == 0) {
            trackIndex = myrand() % n;
            if (cdMusicTable[trackIndex] == 0) {
                cdMusicTable[trackIndex] = i + 2;
                found = 1;
            }
        }
    }
    cdMusicIndex = 0;
}

/* Plays the next shuffled CD track when cdTrack is 1, otherwise the track the config names. */
void InitCDAudio(ConfigData *menu, unsigned char *unused)
{
#ifdef NETPLAY
    if (networkGame)
        return;
#endif
    if (menu->cdTrack == 1) {
        cdMusicIndex = (cdMusicIndex + 1) % (cdMusicCount - 1);
        trackNameScroll = 480;
        cdMusicTrack = cdMusicTable[cdMusicIndex];
        CdMusicPlay(cdMusicTrack, 0);
    } else {
        CdMusicPlay(menu->cdTrack, 0);
    }
}




/* Loads the racing line for the selected track. */
void InitRaceLine(TrackData *track, ConfigData *game)
{
#ifdef NETPLAY
    short unusedCount;
    TrackSection *sectionPtr;
    int routeLength;
    short valueDistance;
    int distanceIndex;
    int raceSection;
    TrackSection *branchStartPosition;
    RaceLine *raceLinePointerCurrent;
    short *sectionPointerNum;
    int mainStepNum;
    short mainContinueIndex;
    short startValuesBuffer[24];
    int branchPositionNum;
    int loopIndexSection;
    int branchDistanceNum;
    short runningFlag;
    short unusedCounter;
    int mainPositionTotalStep;
    int branchStepSize;
    int mainLengthTemp;
    Face *raceFace;
    short trackOffsetTemp;
    int minDistanceNumber;
    short sectionBuffer[500];
    short scanFlag;

    runningFlag = 1;
    unusedCount = 0;
    unusedCounter = 0;
    startValuesBuffer[0] = 0;
    startValuesBuffer[1] = 0x14;
    startValuesBuffer[2] = 0xBC;
    startValuesBuffer[3] = 0;
    startValuesBuffer[4] = 0;
    startValuesBuffer[5] = 0;
    startValuesBuffer[6] = 0x80;
    startValuesBuffer[7] = 0x57C;
    startValuesBuffer[8] = 0x18;
    startValuesBuffer[9] = 0;
    startValuesBuffer[10] = 0;
    startValuesBuffer[11] = 0;
    startValuesBuffer[12] = 0;
    startValuesBuffer[13] = 0x40;
    startValuesBuffer[14] = 0;
    startValuesBuffer[15] = 0;
    startValuesBuffer[16] = 0;
    startValuesBuffer[17] = 0x30;
    startValuesBuffer[18] = 0;
    startValuesBuffer[19] = 0;
    startValuesBuffer[20] = 0x58;
    startValuesBuffer[21] = 0;
    startValuesBuffer[22] = 0;
    startValuesBuffer[23] = 0;

    sectionPtr = track->sections;
    raceLinePointerCurrent = raceLines;
    sectionPointerNum = sectionBuffer;
    distanceIndex = 0;
    while (runningFlag) {
        raceLinePointerCurrent[sectionPtr->secNo].distance = distanceIndex;
        raceLinePointerCurrent[sectionPtr->secNo].x = 0;
        raceLinePointerCurrent[sectionPtr->secNo].y = 0;
        raceLinePointerCurrent[sectionPtr->secNo].z = 0;
        *sectionPointerNum++ = sectionPtr->secNo;
        if (sectionPtr->junction.index != -1 && (sectionPtr->junction.ptr->flags & 16)) {
            branchStartPosition = sectionPtr;
            sectionPtr = sectionPtr->junction.ptr;
            branchDistanceNum = 0;
            branchPositionNum = distanceIndex;
            mainPositionTotalStep = distanceIndex;
            mainLengthTemp = 0;
            scanFlag = 1;
            while (scanFlag) {
                raceSection = sectionPtr->secNo;
                branchDistanceNum++;
                sectionPtr = sectionPtr->nextSection.ptr;
                if (sectionPtr->junction.index != -1) {
                    scanFlag = 0;
                }
            }
            sectionPtr = branchStartPosition;
            sectionPtr = sectionPtr->nextSection.ptr;
            mainContinueIndex = 1;
            mainLengthTemp++;
            sectionPtr = sectionPtr->nextSection.ptr;
            while (mainContinueIndex) {
                mainLengthTemp++;
                sectionPtr = sectionPtr->nextSection.ptr;
                if (sectionPtr->junction.index != -1) {
                    mainContinueIndex = 0;
                }
            }
            mainLengthTemp += 2;
            if (branchDistanceNum <= mainLengthTemp) {
                routeLength = mainLengthTemp;
                minDistanceNumber = branchDistanceNum;
                mainStepNum = 4;
                branchStepSize = (minDistanceNumber << 2) / routeLength;
            } else {
                routeLength = branchDistanceNum;
                minDistanceNumber = mainLengthTemp;
                mainStepNum = (minDistanceNumber << 2) / routeLength;
                branchStepSize = 4;
            }
            sectionPtr = branchStartPosition;
            sectionPtr = sectionPtr->junction.ptr;
            for (loopIndexSection = 0; loopIndexSection < branchDistanceNum; loopIndexSection++) {
                raceLinePointerCurrent[sectionPtr->secNo].distance = branchPositionNum;
                branchPositionNum += mainStepNum;
                raceLinePointerCurrent[sectionPtr->secNo].x = 0;
                raceLinePointerCurrent[sectionPtr->secNo].y = 0;
                raceLinePointerCurrent[sectionPtr->secNo].z = 0;
                sectionPtr = sectionPtr->nextSection.ptr;
            }
            sectionPtr = branchStartPosition;
            for (loopIndexSection = 0; loopIndexSection < mainLengthTemp; loopIndexSection++) {
                raceLinePointerCurrent[sectionPtr->secNo].distance = mainPositionTotalStep;
                mainPositionTotalStep += branchStepSize;
                raceLinePointerCurrent[sectionPtr->secNo].x = 0;
                raceLinePointerCurrent[sectionPtr->secNo].y = 0;
                raceLinePointerCurrent[sectionPtr->secNo].z = 0;
                *sectionPointerNum++ = sectionPtr->secNo;
                sectionPtr = sectionPtr->nextSection.ptr;
            }
            distanceIndex += minDistanceNumber << 2;
            raceLinePointerCurrent[sectionPtr->secNo].distance = distanceIndex;
            raceLinePointerCurrent[sectionPtr->secNo].x = 0;
            raceLinePointerCurrent[sectionPtr->secNo].y = 0;
            raceLinePointerCurrent[sectionPtr->secNo].z = 0;
            *sectionPointerNum++ = sectionPtr->secNo;
            if (track->sections == sectionPtr) {
                runningFlag = 0;
            }
        }
        distanceIndex += 4;
        sectionPtr = sectionPtr->nextSection.ptr;
        if (track->sections == sectionPtr) {
            runningFlag = 0;
        }
    }
    *sectionPointerNum = -1;
    numTrackSections = distanceIndex;
    sectionPtr = track->sections;
    for (loopIndexSection = 0; loopIndexSection < weaponGridSections; loopIndexSection++) {
        raceFace = track->faces + sectionPtr->faceStart;
        while (!(raceFace->flags & TRACK_BASE)) {
            raceFace++;
        }
        raceLines[sectionPtr->secNo].x = track->vertices[raceFace->vertex[0]].vx;
        raceLines[sectionPtr->secNo].y = track->vertices[raceFace->vertex[0]].vy;
        raceLines[sectionPtr->secNo].z = track->vertices[raceFace->vertex[0]].vz;
        sectionPtr++;
    }
    trackOffsetTemp = startValuesBuffer[game->actTrackNum];
    for (loopIndexSection = 0; loopIndexSection < weaponGridSections; loopIndexSection++) {
        valueDistance = raceLines[loopIndexSection].distance - trackOffsetTemp;
        if (valueDistance >= numTrackSections) {
            valueDistance = valueDistance - numTrackSections;
        }
        if (valueDistance < 0) {
            valueDistance = numTrackSections + valueDistance;
        }
        raceLines[loopIndexSection].distance = valueDistance;
    }

#else
    short alive = 1;
    short unusedANum = 0;
    short tmpC = 0;
    short *secPtr;
    short starts[24] = {0, 0x14, 0xBC, 0, 0, 0, 0x80, 0x57C, 0x18, 0, 0, 0, 0, 0x40, 0, 0, 0, 0x30, 0, 0, 0x58, 0, 0, 0};
    int distanceIndex;
    int secNoTmp;
    short sectionsIdx[500];
    int mainPositionTotal;
    int branchPositionIdx;
    int mainStep;
    int branchDistance1;
    short valueDist;
    int stepBranch;
    RaceLine *rlPtr;
    int maxDistance1;
    short off;
    short more;
    TrackSection *secNum;
    Face *trackFace;
    int minDistance2;
    short more21;
    TrackSection *branchStart;
    int mainLength;
    int jTmp;

    secNum = track->sections;
    rlPtr = raceLines;
    secPtr = sectionsIdx;
    distanceIndex = 0;
    while (alive) {
        rlPtr[secNum->secNo].distance = distanceIndex;
        rlPtr[secNum->secNo].x = 0;
        rlPtr[secNum->secNo].y = 0;
        rlPtr[secNum->secNo].z = 0;
        *secPtr++ = secNum->secNo;
        if (secNum->junction.index != -1 && (secNum->junction.ptr->flags & 16)) {
            branchStart = secNum;
            secNum = secNum->junction.ptr;
            branchDistance1 = 0;
            branchPositionIdx = distanceIndex;
            mainPositionTotal = distanceIndex;
            mainLength = 0;
            more = 1;
            while (more) {
                secNoTmp = secNum->secNo;
                branchDistance1++;
                secNum = secNum->nextSection.ptr;
                if (secNum->junction.index != -1) {
                    more = 0;
                }
            }
            secNum = branchStart;
            secNum = secNum->nextSection.ptr;
            more21 = 1;
            mainLength++;
            secNum = secNum->nextSection.ptr;
            while (more21) {
                mainLength++;
                secNum = secNum->nextSection.ptr;
                if (secNum->junction.index != -1) {
                    more21 = 0;
                }
            }
            mainLength += 2;
            if (branchDistance1 <= mainLength) {
                maxDistance1 = mainLength;
                minDistance2 = branchDistance1;
                stepBranch = 4;
                mainStep = (minDistance2 << 2) / maxDistance1;
            } else {
                maxDistance1 = branchDistance1;
                minDistance2 = mainLength;
                stepBranch = (minDistance2 << 2) / maxDistance1;
                mainStep = 4;
            }
            secNum = branchStart;
            secNum = secNum->junction.ptr;
            for (jTmp = 0; branchDistance1 > jTmp; jTmp++) {
                rlPtr[secNum->secNo].distance = branchPositionIdx;
                branchPositionIdx += stepBranch;
                rlPtr[secNum->secNo].x = 0;
                rlPtr[secNum->secNo].y = 0;
                rlPtr[secNum->secNo].z = 0;
                secNum = secNum->nextSection.ptr;
            }
            secNum = branchStart;
            for (jTmp = 0; mainLength > jTmp; jTmp++) {
                rlPtr[secNum->secNo].distance = mainPositionTotal;
                mainPositionTotal += mainStep;
                rlPtr[secNum->secNo].x = 0;
                rlPtr[secNum->secNo].y = 0;
                rlPtr[secNum->secNo].z = 0;
                *secPtr++ = secNum->secNo;
                secNum = secNum->nextSection.ptr;
            }
            distanceIndex += minDistance2 << 2;
            rlPtr[secNum->secNo].distance = distanceIndex;
            rlPtr[secNum->secNo].x = 0;
            rlPtr[secNum->secNo].y = 0;
            rlPtr[secNum->secNo].z = 0;
            *secPtr++ = secNum->secNo;
            if (track->sections == secNum) {
                alive = 0;
            }
        }
        distanceIndex += 4;
        secNum = secNum->nextSection.ptr;
        if (track->sections == secNum) {
            alive = 0;
        }
    }
    *secPtr = -1;
    numTrackSections = distanceIndex;
    secNum = track->sections;
    for (jTmp = 0; jTmp < weaponGridSections; jTmp++) {
        trackFace = track->faces + secNum->faceStart;
        while (!(trackFace->flags & TRACK_BASE)) {
            trackFace++;
        }
        raceLines[secNum->secNo].x = track->vertices[trackFace->vertex[0]].vx;
        raceLines[secNum->secNo].y = track->vertices[trackFace->vertex[0]].vy;
        raceLines[secNum->secNo].z = track->vertices[trackFace->vertex[0]].vz;
        secNum++;
    }
    off = starts[game->actTrackNum];
    for (jTmp = 0; jTmp < weaponGridSections; jTmp++) {
        valueDist = raceLines[jTmp].distance - off;
        if (valueDist >= numTrackSections) {
            valueDist = valueDist - numTrackSections;
        }
        if (valueDist < 0) {
            valueDist = numTrackSections + valueDist;
        }
        raceLines[jTmp].distance = valueDist;
    }

#endif
}
