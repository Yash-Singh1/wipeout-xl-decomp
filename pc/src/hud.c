

#include "hud.h"
#include "cheats.h"
#include "track_results.h"
#include "sound_samples.h"
#include <math.h>
#include "d3dpoly.h"
#include "ayrace.h"
#include "functions.h"
#include "menus.h"
#include <string.h>
#ifdef NETPLAY
#pragma intrinsic(strcpy, strcat, cos)
#include "gst.h"
extern short startLinePos[];
extern int hudThrust;
extern short lapTimeRows[4];
extern signed char rearLockCycle;
extern int pauseRequested;
extern char trackNameText[];
#endif


#ifdef NETPLAY
/* NET build: the race loop passes its arguments through; the function only advances the frame. */
void AdvanceHudFrame(ShipData *ships, RaceConfig *config, int choice)
{
    hudFrame++;
}
#else
void AdvanceHudFrame(void)
{
    hudFrame++;
}
#endif

#ifdef NETPLAY
/* NET build: the frame is laid out from these names (MSVC /Od). The NET source keeps the energy-based shield width in the same
   variable it first stores the raw energy in, and the working copy of each bar length is a separate local for the blit loops. */
void fill_speedo(ShipData *ships, RaceConfig *config, int choice)
{
    int speedDividers[4] = {125, 167, 209, 250};
    int thrustEdge;
    int shieldBase;
    int limit;
    int speedAmount;
    int thrustLength;
    int top;
    int speedRaw;
    int shieldOn = 0;
    int speedEdge;
    int shieldBar;
    int index;
    int rowDy;

    if (CameraUpdate == UpdateCameraAttractInternal) speedRaw = ships[ownShip].speed * 7;
    else speedRaw = ships[ownShip].speed;
    if (nameDrawFirst) {
        shieldBar = 100 - shipDamage[ownShip];
        shieldBar = shieldBar * 192 / 100;
        if (shieldBar >= 190) shieldBar = 190;
        if (choice == -1) {
            if (100 - shipDamage[ownShip] <= 40) {
                if (hudFrame % 8 <= 4) shieldOn = 1;
            } else shieldOn = 1;
        } else shieldOn = 1;
        if (shieldOn) {
            shieldBase = shieldBar - 22;
            if (shieldBase < 0) shieldBase = 0;
            if (shieldBar > 0) {
                SetTextQuad(&menuBarPoly, 208, 198, 0, 210, shieldBar + 2, 44, 33, 0);
                menuBarPoly.u0 = 0;
                menuBarPoly.v0 = 210;
                menuBarPoly.u1 = shieldBase + 2;
                menuBarPoly.v1 = 210;
                menuBarPoly.u2 = 0;
                menuBarPoly.v2 = 254;
                menuBarPoly.u3 = shieldBar + 2;
                menuBarPoly.v3 = 254;
                menuBarPoly.x0 = 416;
                menuBarPoly.y0 = 396;
                menuBarPoly.x1 = shieldBase + 418;
                menuBarPoly.y1 = 396;
                menuBarPoly.x2 = 416;
                menuBarPoly.y2 = 440;
                menuBarPoly.x3 = shieldBar + 418;
                menuBarPoly.y3 = 440;
                DrawScaledTexturedQuad(0.3f, &menuBarPoly, hudTexturePage);
            }
        }
        if (ships[ownShip].autopilotCount == 0) speedAmount = speedRaw / speedDividers[config->shipClass];
        else speedAmount = (speedRaw / speedDividers[config->shipClass]) * 3;
        if (speedAmount >= 192) speedAmount = 192;
        speedEdge = speedAmount - 22;
        if (speedEdge < 0) speedEdge = 0;
        if (speedAmount > 0) {
            SetTextQuad(&menuBarPoly, 218, 170, 0, 122, speedAmount, 44, 33, 0);
            menuBarPoly.u0 = 0;
            menuBarPoly.v0 = 122;
            menuBarPoly.u1 = speedAmount;
            menuBarPoly.v1 = 122;
            menuBarPoly.u2 = 0;
            menuBarPoly.v2 = 166;
            menuBarPoly.u3 = speedEdge;
            menuBarPoly.v3 = 166;
            menuBarPoly.x0 = 436;
            menuBarPoly.y0 = 340;
            menuBarPoly.x1 = speedAmount + 436;
            menuBarPoly.y1 = 340;
            menuBarPoly.x2 = 436;
            menuBarPoly.y2 = 384;
            menuBarPoly.x3 = speedEdge + 436;
            menuBarPoly.y3 = 384;
            DrawScaledTexturedQuad(0.29f, &menuBarPoly, hudTexturePage);
        }
        if (ships[ownShip].autopilotCount == 0) {
            thrustLength = ships[ownShip].thrust * 192 / ships[ownShip].maxThrust;
            hudThrust = thrustLength;
        } else thrustLength = hudThrust;
        if (thrustLength >= 192) thrustLength = 192;
        thrustEdge = thrustLength - 22;
        if (thrustEdge < 0) thrustEdge = 0;
        if (thrustLength > 0) {
            SetTextQuad(&menuBarPoly, 218, 170, 0, 166, thrustLength, 44, 33, 0);
            menuBarPoly.u0 = 0;
            menuBarPoly.v0 = 166;
            menuBarPoly.u1 = thrustLength;
            menuBarPoly.v1 = 166;
            menuBarPoly.u2 = 0;
            menuBarPoly.v2 = 210;
            menuBarPoly.u3 = thrustEdge;
            menuBarPoly.v3 = 210;
            menuBarPoly.x0 = 436;
            menuBarPoly.y0 = 340;
            menuBarPoly.x1 = thrustLength + 436;
            menuBarPoly.y1 = 340;
            menuBarPoly.x2 = 436;
            menuBarPoly.y2 = 384;
            menuBarPoly.x3 = thrustEdge + 436;
            menuBarPoly.y3 = 384;
            DrawScaledTexturedQuad(0.3f, &menuBarPoly, hudTexturePage);
        }
        DrawScaledTexturedQuad(0.28f, &panelQuads[0], hudTexturePage);
        DrawScaledTexturedQuad(0.28f, &panelQuads[1], hudTexturePage);
    } else {
        DrawScaledTexturedQuad(0.28f, &panelQuads[0], hudTexturePage);
        DrawScaledTexturedQuad(0.28f, &panelQuads[1], hudTexturePage);
        shieldBar = 100 - shipDamage[ownShip];
        shieldBar = shieldBar * 192 / 100;
        if (shieldBar >= 192) shieldBar = 192;
        if (choice == -1) {
            if (100 - shipDamage[ownShip] <= 40) {
                if (hudFrame % 8 <= 4) shieldOn = 1;
            } else shieldOn = 1;
        } else shieldOn = 1;
        if (shieldOn && shieldBar > 0) {
            limit = shieldBar;
            rowDy = 2;
            top = 44 - rowDy;
            for (index = 0; index < 22; index++) {
                if (limit > 0) {
                    if (mmxAvailable)
                        AddBlit640(0, 416, top + 396, limit + 416, rowDy + top + 396,
                                    256, top + 210, limit + 256, rowDy + top + 210, 3);
                    else
                        AddBlit640Shifted(0, 416, top + 396, limit + 416, rowDy + top + 396,
                                    0, top + 210, limit, rowDy + top + 210, 33);
                    limit--;
                    top -= rowDy;
                    rowDy++;
                    if (rowDy == 3) rowDy = 2;
                }
            }
        }
        if (ships[ownShip].autopilotCount == 0) speedAmount = speedRaw / speedDividers[config->shipClass];
        else speedAmount = (speedRaw / speedDividers[config->shipClass]) * 3;
        if (speedAmount >= 192) speedAmount = 192;
        if (speedAmount > 0) {
            limit = speedAmount;
            top = 0;
            rowDy = 2;
            for (index = 0; index < 22; index++) {
                if (limit > 0) {
                    if (mmxAvailable)
                        AddBlit640(0, 436, top + 340, limit + 436, rowDy + top + 340,
                                    256, top + 122, limit + 256, rowDy + top + 122, 3);
                    else
                        AddBlit640Shifted(0, 436, top + 340, limit + 436, rowDy + top + 340,
                                    256, top + 122, limit + 256, rowDy + top + 122, 3);
                    limit--;
                    top += rowDy;
                    rowDy++;
                    if (rowDy == 3) rowDy = 2;
                }
            }
        }
        if (ships[ownShip].autopilotCount == 0) {
            thrustLength = ships[ownShip].thrust * 192 / ships[ownShip].maxThrust;
            hudThrust = thrustLength;
        } else thrustLength = hudThrust;
        if (thrustLength >= 192) thrustLength = 192;
        if (thrustLength > 0) {
            limit = thrustLength;
            top = 0;
            rowDy = 2;
            for (index = 0; index < 22; index++) {
                if (limit > 0) {
                    if (mmxAvailable)
                        AddBlit640(0, 436, top + 340, limit + 436, rowDy + top + 340,
                                    256, top + 166, limit + 256, rowDy + top + 166, 3);
                    else
                        AddBlit640Shifted(0, 436, top + 340, limit + 436, rowDy + top + 340,
                                    0, top + 166, limit, rowDy + top + 166, 33);
                    limit--;
                    top += rowDy;
                    rowDy++;
                    if (rowDy == 3) rowDy = 2;
                }
            }
        }
    }
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* Equivalent; the quad setup stores are scheduled differently. */
void fill_speedo(ShipData *ships, RaceConfig *config, int choice)
{
    int row;
    int showShield = 0;
    int energy;
    int bottom;
    int step;
    int speedDividers[4] = {125, 167, 209, 250};
    int i;
    int speed;
    int width;

    if (CameraUpdate == UpdateCameraAttractInternal) speed = ships[ownShip].speed * 7;
    else speed = ships[ownShip].speed;
    if (nameDrawFirst) {
        energy = 100 - shipDamage[ownShip];
        width = energy * 192 / 100;
        if (width >= 190) width = 190;
        if (choice != -1 || energy > 40 || hudFrame % 8 <= 4) showShield = 1;
        if (showShield) {
            bottom = width - 22;
            if (bottom < 0) bottom = 0;
            if (width > 0) {
                SetTextQuad(&menuBarPoly, 208, 198, 0, 210, width + 2, 44, 33, 0);
                menuBarPoly.u0 = 0;
                menuBarPoly.v0 = 210;
                menuBarPoly.u1 = bottom + 2;
                menuBarPoly.v1 = 210;
                menuBarPoly.u2 = 0;
                menuBarPoly.v2 = 254;
                menuBarPoly.u3 = width + 2;
                menuBarPoly.y0 = 396;
                menuBarPoly.v3 = 254;
                menuBarPoly.x0 = 416;
                menuBarPoly.x1 = bottom + 418;
                menuBarPoly.y1 = 396;
                menuBarPoly.x2 = 416;
                menuBarPoly.y2 = 440;
                menuBarPoly.x3 = width + 418;
                menuBarPoly.y3 = 440;
                DrawScaledTexturedQuad(0.3f, &menuBarPoly, hudTexturePage);
            }
        }
        if (ships[ownShip].autopilotCount == 0) speed /= speedDividers[config->shipClass];
        else speed = (speed / speedDividers[config->shipClass]) * 3;
        if (speed >= 192) speed = 192;
        bottom = speed - 22;
        if (bottom < 0) bottom = 0;
        if (speed > 0) {
            SetTextQuad(&menuBarPoly, 218, 170, 0, 122, speed, 44, 33, 0);
            menuBarPoly.u0 = 0;
            menuBarPoly.v0 = 122;
            menuBarPoly.u1 = speed;
            menuBarPoly.v1 = 122;
            menuBarPoly.u2 = 0;
            menuBarPoly.v2 = 166;
            menuBarPoly.u3 = bottom;
            menuBarPoly.v3 = 166;
            menuBarPoly.x0 = 436;
            menuBarPoly.y0 = 340;
            menuBarPoly.x1 = 436 + speed;
            menuBarPoly.y1 = 340;
            menuBarPoly.x2 = 436;
            menuBarPoly.x3 = 436 + bottom;
            menuBarPoly.y2 = 384;
            menuBarPoly.y3 = 384;
            DrawScaledTexturedQuad(0.29f, &menuBarPoly, hudTexturePage);
        }
        if (ships[ownShip].autopilotCount == 0) {
            width = ships[ownShip].thrust * 192 / ships[ownShip].maxThrust;
            hudThrust = width;
        } else width = hudThrust;
        if (width >= 192) width = 192;
        bottom = width - 22;
        if (bottom < 0) bottom = 0;
        if (width > 0) {
            SetTextQuad(&menuBarPoly, 218, 170, 0, 166, width, 44, 33, 0);
            menuBarPoly.u0 = 0;
            menuBarPoly.v0 = 166;
            menuBarPoly.u1 = width;
            menuBarPoly.v1 = 166;
            menuBarPoly.u2 = 0;
            menuBarPoly.v2 = 210;
            menuBarPoly.u3 = bottom;
            menuBarPoly.v3 = 210;
            menuBarPoly.x1 = width + 436;
            menuBarPoly.x0 = 436;
            menuBarPoly.x2 = 436;
            menuBarPoly.y0 = 340;
            menuBarPoly.y1 = 340;
            menuBarPoly.x3 = bottom + 436;
            menuBarPoly.y2 = 384;
            menuBarPoly.y3 = 384;
            DrawScaledTexturedQuad(0.3f, &menuBarPoly, hudTexturePage);
        }
        DrawScaledTexturedQuad(0.28f, &panelQuads[0], hudTexturePage);
        DrawScaledTexturedQuad(0.28f, &panelQuads[1], hudTexturePage);
    } else {
        DrawScaledTexturedQuad(0.28f, &panelQuads[0], hudTexturePage);
        DrawScaledTexturedQuad(0.28f, &panelQuads[1], hudTexturePage);
        energy = 100 - shipDamage[ownShip];
        width = energy * 192 / 100;
        if (width >= 192) width = 192;
        if (choice != -1 || energy > 40 || hudFrame % 8 <= 4) showShield = 1;
        if (showShield && width > 0) {
            step = 2;
            row = 42;
            for (i = 22; i != 0; i--) {
                if (width > 0) {
                    if (mmxAvailable)
                        AddBlit640(0, 416, row + 396, width + 416, row + step + 396,
                                    256, row + 210, width + 256, row + step + 210, 3);
                    else
                        AddBlit640Shifted(0, 416, row + 396, width + 416, row + step + 396,
                                    0, row + 210, width, row + step + 210, 33);
                    width--;
                    row -= step;
                    step++;
                    if (step == 3) step = 2;
                }
            }
        }
        if (ships[ownShip].autopilotCount == 0) width = speed / speedDividers[config->shipClass];
        else width = (speed / speedDividers[config->shipClass]) * 3;
        if (width >= 192) width = 192;
        if (width > 0) {
            row = 0;
            step = 2;
            for (i = 22; i != 0; i--) {
                if (width > 0) {
                    if (mmxAvailable)
                        AddBlit640(0, 436, row + 340, width + 436, row + step + 340,
                                    256, row + 122, width + 256, row + step + 122, 3);
                    else
                        AddBlit640Shifted(0, 436, row + 340, width + 436, row + step + 340,
                                    256, row + 122, width + 256, row + step + 122, 3);
                    width--;
                    row += step;
                    step++;
                    if (step == 3) step = 2;
                }
            }
        }
        if (ships[ownShip].autopilotCount == 0) {
            width = ships[ownShip].thrust * 192 / ships[ownShip].maxThrust;
            hudThrust = width;
        } else width = hudThrust;
        if (width >= 192) width = 192;
        if (width > 0) {
            row = 0;
            step = 2;
            for (i = 22; i != 0; i--) {
                if (width > 0) {
                    if (mmxAvailable)
                        AddBlit640(0, 436, row + 340, width + 436, row + step + 340,
                                    256, row + 166, width + 256, row + step + 166, 3);
                    else
                        AddBlit640Shifted(0, 436, row + 340, width + 436, row + step + 340,
                                    0, row + 166, width, row + step + 166, 33);
                    width--;
                    row += step;
                    step++;
                    if (step == 3) step = 2;
                }
            }
        }
    }
}
#endif
#endif

void DrawWeaponIcons(ShipData *ship)
{
    int texturePage = 1;
    int weapon;
    if (typeSwitchesMode) texturePage = 0;
#ifdef NETPLAY
    DrawScaledTexturedQuad(0.3f, &panelQuads[2], texturePage);
    DrawScaledTexturedQuad(0.3f, &panelQuads[3], texturePage);
#else
    DrawScaledTexturedQuad(0.3f, &weaponHudPolys[0], texturePage);
    DrawScaledTexturedQuad(0.3f, &weaponHudPolys[1], texturePage);
#endif
    weapon = ship->weapon;
    if (weapon) {
        weapon--;
        SetTextQuad(&menuBarPoly, weaponX[weapon], weaponY[weapon],
                   weaponU[weapon], weaponV[weapon],
                   weaponWidth[weapon], weaponHeight[weapon], 34, 0);
        DrawScaledTexturedQuad(0.29f, &menuBarPoly, texturePage);
    }
}

#ifdef NETPLAY
/* NET build: the frame is laid out from these names (MSVC /Od). finishPlace, raceLap and finishTime are stored and never read,
   as in the NET source. */
void UpdateRaceHud(ShipData *ships, int choice, RaceConfig *config, struct HiScoreData *score)
{
    int j;
    unsigned int bestLapTime;
    int finishTime;
    int finishPlace = 0;
    int sectionDiff;
    short autoDelay;
    short raceLap = 0;
    int startPos;
    short lapNo;
    int i;

    for (i = 0; i < 15; i++) {
        if (!(ships[i].combatAttr & DESTROYED)) {
            sectionDiff = ships[i].raceProgress.trackDistance - raceLines[ships[i].nearTrkSect->secNo].distance;
            if (sectionDiff > (numTrackSections >> 1)) {
                if (ships[i].reverseLap == 0) {
                    ships[i].lap++;
                    if (i == ownShip && ships[ownShip].lap >= 2 && ships[ownShip].lap <= classLaps[config->shipClass] + 1)
                        CheckForHiScore(config, score, ships, ownShip, 1);
                    if (networkGame) {
                        bestLapTime = -1;
                        for (j = 0; j < ships[i].lap - 1; j++) {
                            if (ships[i].lapTimes[j] < bestLapTime) {
                                ships[i].netEd = j;
                                bestLapTime = ships[i].lapTimes[j];
                            }
                        }
                    }
                } else ships[i].reverseLap--;
                ships[i].combatAttr |= 0x4000;
                if (i == ownShip && gameType == 1 && i < 12) ships[i].weapon = 9;
                if (i == ownShip) {
                    if (perfectLap) perfectLapCount = 120;
                    perfectLap = 1;
                }
            } else if (sectionDiff < -(numTrackSections >> 1)) {
                if (i == ownShip) perfectLap = 0;
                ships[i].reverseLap++;
            }
            lapNo = ships[i].lap - ships[i].reverseLap;
            if (lapNo < 0) lapNo = 0;
            ships[i].raceProgress.distance = raceLines[ships[i].nearTrkSect->secNo].distance + lapNo * numTrackSections;
            ships[i].raceProgress.trackDistance = raceLines[ships[i].nearTrkSect->secNo].distance;
        }
    }
    if (startLinePos[config->trackNum] >= ships[ownShip].nearSecNo)
        startPos = ships[ownShip].nearSecNo + track->sectionCount;
    else
        startPos = ships[ownShip].nearSecNo;
    currentRacePosition = CalcPosition(ships);
    playerPosition = currentRacePosition;
    if (ships[ownShip].combatAttr & 0x4000 && !networkGame) {
        lapNo = ships[ownShip].lap;
        if (lapNo == 0) lapNo = 1;
        if (choice == -1 && classLaps[config->shipClass] + 1 > ships[ownShip].lap)
            ships[ownShip].lapTimes[lapNo - 1] += 2;
    }
    if (ships[ownShip].attr & RACING) {
        autoDelay = ships[ownShip].autopilotCount;
        if (autoDelay) finishTime = ships[ownShip].moveDist * 34;
        else finishTime = ships[ownShip].speed;
        if (perfectLapCount > 0) perfectLapCount--;
        if (trackNameScroll > -480) {
            trackNameOffset = trackNameScroll;
            if (trackNameOffset < 0 && trackNameOffset > -200) trackNameOffset = 0;
            if (trackNameOffset <= -200) trackNameOffset += 200;
            if (trackNameOffset < 0) trackNameOffset = -trackNameOffset;
            if (pause.selection == -1) trackNameScroll -= 6;
            if (config->cdTrack == 1) cdMusicTrack = cdMusicTable[cdMusicIndex];
            else cdMusicTrack = config->cdTrack;
        }
    }
    for (i = 0; i < 15; i++) {
        if (i != ownShip && ships[i].lap > classLaps[config->shipClass] && (ships[i].attr & RACING))
            ships[i].attr &= ~RACING;
    }
    if (ships[ownShip].lap > classLaps[config->shipClass] && choice == -1 && (ships[ownShip].attr & RACING) && !networkGame) {
        ships[ownShip].update = UpdateDroneShip;
        ships[ownShip].updateCount = 0;
        ships[ownShip].attr = 0;
        ships[ownShip].autopilotCount = 0;
        humanToDroneSpeed(ships, ownShip);
        CameraUpdate = UpdateCameraAttractMode;
        copyInTempLapTimes(ships);
        config->raceInfo = 1;
        linkSendFlag = 1;
        attractDelayFrames = 1;
        raceOutcome = 1;
        finishPosition = playerPosition;
        AssignMedals(config);
    }
    if (ships[ownShip].attr & RACING) {
        if (ships[ownShip].attr & RESCUE) ships[ownShip].speed = 0;
        AdvanceHudFrame(ships, config, choice);
    }
    TickHUDMessage();
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* Equivalent; the callee-saved registers of the ship loop (index, config, choice) are assigned differently. */
void UpdateRaceHud(ShipData *ships, int choice, RaceConfig *config, struct HiScoreData *score)
{
    int choiceMode = choice;
    RaceConfig * raceConfig = config;
    struct HiScoreData * raceScore = score;
    int sectionDifference;
    short lap;
    int i;
    short position;
    short lapDistance;
    ShipData *player;
    signed char lapsMax;

    for (i = 0; i < 15; i++) {
        if (!(ships[i].combatAttr & DESTROYED)) {
            sectionDifference = ships[i].raceProgress.trackDistance - raceLines[ships[i].nearTrkSect->secNo].distance;
            if (sectionDifference > (numTrackSections >> 1)) {
                if (ships[i].reverseLap == 0) {
                    ships[i].lap++;
                    if (i == ownShip && ships[ownShip].lap >= 2 && ships[ownShip].lap <= classLaps[raceConfig->shipClass] + 1)
                        CheckForHiScore(raceConfig, raceScore, ships, ownShip, 1);
                } else ships[i].reverseLap--;
                ships[i].combatAttr |= 0x4000;
                if (i == ownShip) {
                    if (gameType == 1 && i < 12) ships[i].weapon = 9;
                    if (i == ownShip) {
                        if (perfectLap) perfectLapCount = 120;
                        perfectLap = 1;
                    }
                }
            } else if (sectionDifference < -(numTrackSections >> 1)) {
                if (i == ownShip) perfectLap = 0;
                ships[i].reverseLap++;
            }
            lap = ships[i].lap - ships[i].reverseLap;
            if (lap < 0) lap = 0;
            lapDistance = numTrackSections * lap;
            ships[i].raceProgress.distance = lapDistance + raceLines[ships[i].nearTrkSect->secNo].distance;
            ships[i].raceProgress.trackDistance = raceLines[ships[i].nearTrkSect->secNo].distance;
        }
    }
    position = CalcPosition(ships);
    currentRacePosition = position;
    playerPosition = position;
    player = &ships[ownShip];
    if (player->combatAttr & 0x4000) {
        lap = player->lap;
        if (lap == 0) lap = 1;
        if (choiceMode == -1 && player->lap < classLaps[raceConfig->shipClass] + 1)
            ships[ownShip].raceLapTimes[lap] += 2;
    }
    if (player->attr & RACING) {
        if (perfectLapCount > 0) perfectLapCount--;
        if (trackNameScroll > -480) {
            trackNameOffset = trackNameScroll;
            if (trackNameScroll < 0 && trackNameScroll > -200) trackNameOffset = 0;
            if (trackNameOffset <= -200) trackNameOffset += 200;
            if (trackNameOffset < 0) trackNameOffset = -trackNameOffset;
            if (pause.selection == -1) trackNameScroll -= 6;
            if (raceConfig->cdTrack == 1) cdMusicTrack = cdMusicTable[cdMusicIndex];
            else cdMusicTrack = raceConfig->cdTrack;
        }
    }
    for (i = 0; i < 15; i++) {
        if (i != ownShip && ships[i].lap > classLaps[raceConfig->shipClass] && (ships[i].attr & RACING))
            ships[i].attr &= ~RACING;
    }
    lapsMax = classLaps[raceConfig->shipClass];
    if (player->lap > lapsMax && choiceMode == -1 && (player->attr & RACING)) {
        player->update = UpdateDroneShip;
        player->updateCount = 0;
        player->attr = 0;
        player->autopilotCount = 0;
        humanToDroneSpeed(ships, ownShip);
        CameraUpdate = UpdateCameraAttractMode;
        for (i = 0; i < 5; i++) {
            savedLapTimes[i] = ships[ownShip].lapTimes[i] - ships[ownShip].lapTimes[i] % 6;
        }
        raceConfig->raceInfo = 1;
        linkSendFlag = 1;
        finishPosition = playerPosition;
        attractDelayFrames = 1;
        raceOutcome = 1;
        AssignMedals(raceConfig);
    }
    if (ships[ownShip].attr & RACING) {
        if (ships[ownShip].attr & RESCUE) ships[ownShip].speed = 0;
        hudFrame++;
    }
    if (messageTimer > 0) messageTimer--;
}
#endif
#endif

#ifdef EQUIV
/* Equivalent; a few register choices differ (lap loop test, speed * 34, checkpoint timer).
   Alias handling is relaxed so the compiler reloads the globals as the PC build does. */
#pragma optimize("a", off)
#pragma optimize("w", on)
#ifndef NETPLAY
void DrawRaceHud(ShipData *ships, int choice, RaceConfig *config)
{
    short autopilot;
    int i;
    short racingCount = 0;
    short lap;
    ShipData *player;
    char text[8];
    int lapRow;
    int trackNumber;
    int speed;

    if (ships[ownShip].combatAttr & 0x4000) {
        lap = ships[ownShip].lap;
        if (lap == 0) lap = 1;
        if (ships[ownShip].attr & RACING) {
            if (ships[ownShip].lap < classLaps[config->shipClass] + 1)
                AddTime(ships[ownShip].raceLapTimes[lap], 16, 205, 16, 0, 1);
            else AddTime(ships[ownShip].raceLapTimes[classLaps[config->shipClass]], 16, 205, 16, 0, 1);
        }
    }
    if (ships[ownShip].attr & RACING) {
        if (ships[ownShip].combatAttr & 0x4000) {
            lapRow = 0;
            Print(hudLapTimesText, 22, lapTimeRows[config->shipClass], 0);
            i = 0;
            while (i < classLaps[config->shipClass] - 1) {
                wsprintfA(text, hudLapNumberFormat, ++i);
                Print(text, 16, lapTimeRows[config->shipClass] + lapRow + 12, 0);
                lapRow += 10;
            }
            for (i = 1; i < ships[ownShip].lap; i++) {
                if (i < classLaps[config->shipClass])
                    AddTime(ships[ownShip].raceLapTimes[i], 24, lapTimeRows[config->shipClass] + i * 10, 8, 1, 1);
            }
        }
        autopilot = ships[ownShip].autopilotCount;
        if (autopilot != 0) {
            AddText(hudAutopilotText, 120, 108, 8, 1);
            if (autopilot < 30) AddText(hudDisengagingText, 110, 120, 8, 1);
            else AddNumber((short)(autopilot / 30), 160, 128, 8, 0);
            speed = ships[ownShip].moveDist * 34;
        } else speed = ships[ownShip].speed;
        AddText(hudLapText, 258, 50, 8, 1);
        if (ships[ownShip].lap > classLaps[config->shipClass])
            AddJustifyNumber(classLaps[config->shipClass], 263, 60, 16, 0);
        else AddJustifyNumber(ships[ownShip].lap, 263, 60, 16, 0);
        AddText(hudLapTotalText, 263, 68, 8, 1);
        AddNumber(classLaps[config->shipClass], 285, 60, 16, 0);
        player = &ships[ownShip];
        if (player->rearTarget != -1 && rearLockCycle < 13 && player->weapon == 2 && !pauseRequested)
            PrintCX(hudRearLockText, 168, 70, 0);
        if (perfectLapCount > 0) CentreText(hudPerfectLapText, 120, 160, 8, 0);
        if (trackNameScroll > -480) {
            trackNumber = cdMusicTrack;
            if (trackNumber != 0)
                wsprintfA(trackNameText, hudCdPlayingFormat, trackNumber,
                          GetCdTrackName(trackNumber - 1), GetCdTrackArtist(trackNumber - 1));
            else wsprintfA(trackNameText, hudCdStoppedFormat, trackNumber,
                           GetCdTrackName(trackNumber), GetCdTrackArtist(trackNumber));
            if (cdMusicCount != 0) Print(trackNameText, 16, (trackNameOffset >> 2) + 227, 0);
        }
        for (i = 0; i < 15; i++) {
            if (!(ships[i].combatAttr & DESTROYED)) racingCount++;
        }
        AddText(hudCheckpointText, 20, 20, 8, 1);
        AddText(hudPositionText, 258, 20, 8, 1);
        AddJustifyNumber(currentRacePosition, 263, 30, 16, 0);
        AddText(hudPositionTotalText, 263, 38, 8, 1);
        AddNumber(racingCount, 285, 30, 16, 0);
        if ((ships[ownShip].attr & RACING) && ships[ownShip].lap >= 0) {
            if (checkTimer <= 0) {
                if (blinkFlag) AddTime(checkTimer, 20, 30, 16, 1, 1);
            } else AddTime(checkTimer, 20, 30, 16, 0, 1);
        }
        if (autopilot == 0 && shipDamage[ownShip] < 100 && !(ships[ownShip].attr & DIRECTION) &&
            ships[ownShip].nearTrkSect->junction.ptr == (TrackSection *)-1 &&
            ships[ownShip].nearTrkSect->nextSection.ptr->junction.ptr == (TrackSection *)-1)
            CentreText(hudWrongWayText, 50, 110, 8, 1);
        AddJustifyNumber(speed / 95, 250, 168, 16, 0);
    }
    player = &ships[ownShip];
    if (player->attr & RACING) {
        if (player->attr & RESCUE) player->speed = 0;
        fill_speedo(ships, config, choice);
    }
    if (messageTimer > 0) Print(hudMessage, 210, 226, 1);
}
#endif
#pragma optimize("w", off)
#pragma optimize("a", on)
#endif

#ifdef NETPLAY
#define SORT_TIE_BREAK_ENABLED 1
/* NET race HUD: the death and results overlays (ranked by playerPoints, with the tournament table
   and the lap times), then the usual race readout. The fourth argument is passed by the caller but not read. */
extern int playerPoints[];
extern unsigned char gstLevelLives;
extern unsigned char gstIsHost;
void DrawRaceHud(ShipData *ships, int choice, RaceConfig *config, struct HiScoreData *score)
{
    short racingCount;
    short curLapNum;
    short autopilot;
    int row;
    int placeTotal;     /* initialised, never read in the original */
    int rows;
    char lifeText[100];
    int msgIndex;
    char lapBuf[8];
    int spd;
    u_char place;
    int listed2;
    char killsBuf[100];
    char numbers[100];
    signed char swapTally;
    char rankText[100];
    int shipIdx;
    signed char swapFlag;
    int order[15];

    placeTotal = 0;
    racingCount = 0;
    if (networkGame) {
        if (killMessageTimer > 0) {
            killMessageTimer--;
            if (blinkFlag) killMessageFlash = 1;
        }
    }
    if (networkGame && gstSyncActive) {
        for (msgIndex = 0; msgIndex < 15; msgIndex++)
            CentreText(gstSyncMessage[msgIndex], 0, msgIndex * 10 + 43, 8, 0);
    }
    if (networkGame && playerDestroyed) {
        CentreText("YOU HAVE BEEN DESTROYED", 40, 100, 8, 0);
        sprintf(lifeText, "LIVES REMAINING %d", livesRemaining[ownShip]);
        if (gstLevelLives) CentreText(lifeText, 0, 120, 8, 1);
        if (livesRemaining[ownShip] > 0 || !gstLevelLives)
            CentreText("PRESS ENTER TO RESUME RACING", 0, 140, 8, 0);
        if (gstRaceMode == 3) CentreText("PRESS ESC TO QUIT TOURNAMENT", 0, 160, 8, 0);
        else CentreText("PRESS ESC TO QUIT RACE", 0, 160, 8, 0);
    }
    if (!(ships[ownShip].attr & RACING) && deathCameraActive && networkGame) {
        place = 0;
        rows = 0;
        if (gstRaceMode == 3) {
            CentreText("PRESS TAB TO VIEW ", 0, 200, 8, 0);
            CentreText("TOURNAMENT RANKING", 0, 210, 8, 0);
        } else {
            CentreText("PRESS TAB TO RESUME RACING", 0, 210, 8, 0);
        }
        CentreText("PRESS ESC TO QUIT RACE", 0, 220, 8, 0);
        CentreText("RACE RESULTS", 100, 10, 8, 0);
        AddText("BEST LAP", 40, 30, 8, 0);
        AddText("NAME", 135, 30, 8, 1);
        AddText("KILLS", 270, 30, 8, 1);
        for (row = 0; row < 15; row++) {
            if (ships[row].netActive & 1 || ships[row].netActive & 3) {
                if (ships[row].netPlace != -1) {
                    place = ships[row].netPlace - 1;
                    sprintf(numbers, "%d", ships[row].netPlace);
                    AddText(numbers, 5, place * 10 + 43, 8, 0);
                    if (ships[row].netEd != -1)
                        AddTime(ships[row].lapTimes[ships[row].netEd], 40, place * 10 + 43, 8, 0, 1);
                    AddText(ships[row].netName, 135, place * 10 + 43, 8, 1);
                    sprintf(numbers, "%d", killerCount[row]);
                    AddText(numbers, 270, place * 10 + 43, 8, 1);
                    rows++;
                }
            }
        }
        for (row = 0; row < 15; row++) {
            if (ships[row].netActive & 1 || ships[row].netActive & 3) {
                if (ships[row].netPlace == -1) {
                    if (ships[row].netEd != -1) AddText("00:00:00", 40, rows * 10 + 43, 8, 0);
                    AddText(ships[row].netName, 135, rows * 10 + 43, 8, 1);
                    sprintf(killsBuf, "%d", killerCount[row]);
                    AddText(killsBuf, 270, rows * 10 + 43, 8, 1);
                    rows++;
                }
            }
        }
    }
    if (!(ships[ownShip].attr & RACING) && resultsShown && networkGame) {
        listed2 = 0;
        for (row = 0; row < 15; row++) order[row] = row;
        do {
            swapTally = 0;
            for (row = 0; row < 14; row++) {
                swapFlag = 0;
                if (ships[order[row]].netActive & 1 || ships[order[row]].netActive & 3) {
                    if (ships[order[row + 1]].netActive & 1 || ships[order[row + 1]].netActive & 3) {
                        if (playerPoints[order[row + 1]] > playerPoints[order[row]]) swapFlag = 1;
                        else if (playerPoints[order[row + 1]] == playerPoints[order[row]]) {
                            /* Equal points: the lower ship index goes first. The network build keeps this
                               tie-break behind a switch that is on, so its else branch is empty. */
                            if (SORT_TIE_BREAK_ENABLED) {
                                if (order[row + 1] < order[row]) swapFlag = 1;
                            } else { }
                        }
                    } else { }
                } else if (ships[order[row + 1]].netActive & 1 || ships[order[row + 1]].netActive & 3) {
                    swapFlag = 1;
                } else if (order[row + 1] < order[row]) {
                    swapFlag = 1;
                }
                if (swapFlag) {
                    shipIdx = order[row];
                    order[row] = order[row + 1];
                    order[row + 1] = shipIdx;
                    swapTally++;
                }
            }
        } while (swapTally != 0);
        if (tournamentTrack == 7 || !gstIsHost) {
            CentreText("PRESS ESC TO QUIT TOURNAMENT", 0, 200, 8, 0);
        } else {
            CentreText("PRESS THE N KEY TO ADVANCE", 0, 200, 8, 0);
            CentreText("TO NEXT TOURNAMENT RACE", 0, 210, 8, 0);
        }
        CentreText("PRESS TAB TO RESUME RACING", 0, 220, 8, 0);
        CentreText("TOURNAMENT RESULTS", 100, 10, 8, 0);
        AddText("POINTS", 40, 30, 8, 0);
        AddText("NAME", 135, 30, 8, 1);
        AddText("KILLS", 270, 30, 8, 1);
        for (row = 0; row < 15; row++) {
            if (ships[order[row]].netActive & 1 || ships[order[row]].netActive & 3) {
                sprintf(rankText, "%d", listed2 + 1);
                AddText(rankText, 5, listed2 * 10 + 43, 8, 0);
                sprintf(rankText, "%d", playerPoints[order[row]]);
                AddText(rankText, 40, listed2 * 10 + 43, 8, 1);
                AddText(ships[order[row]].netName, 135, listed2 * 10 + 43, 8, 1);
                sprintf(rankText, "%d", killerTotal[order[row]]);
                AddText(rankText, 270, listed2 * 10 + 43, 8, 1);
                listed2++;
            }
        }
    }
    if (ships[ownShip].combatAttr & 0x4000) {
        curLapNum = ships[ownShip].lap;
        if (curLapNum == 0) curLapNum = 1;
        if (ships[ownShip].attr & RACING) {
            if (ships[ownShip].lap < classLaps[config->shipClass] + 1)
                AddTime(ships[ownShip].lapTimes[curLapNum - 1], 16, 205, 16, 0, 1);
            else
                AddTime(ships[ownShip].lapTimes[classLaps[config->shipClass] - 1], 16, 205, 16, 0, 1);
        }
    }
    if (ships[ownShip].attr & RACING) {
        if (ships[ownShip].combatAttr & 0x4000) {
            Print("LAP TIMES", 22, lapTimeRows[config->shipClass], 0);
            for (row = 0; row < classLaps[config->shipClass] - 1; row++) {
                wsprintfA(lapBuf, "%d", row + 1);
                Print(lapBuf, 16, lapTimeRows[config->shipClass] + row * 10 + 12, 0);
            }
            for (row = 1; row < ships[ownShip].lap; row++) {
                if (row < classLaps[config->shipClass])
                    AddTime(ships[ownShip].lapTimes[row - 1], 24, lapTimeRows[config->shipClass] + row * 10, 8, 1, 1);
            }
        }
        autopilot = ships[ownShip].autopilotCount;
        if (autopilot != 0) {
            AddText("AUTOPILOT", 120, 108, 8, 1);
            if (autopilot < 30) AddText("DISENGAGING", 110, 120, 8, 1);
            else AddNumber(autopilot / 30, 160, 128, 8, 0);
            spd = ships[ownShip].moveDist * 34;
        } else {
            spd = ships[ownShip].speed;
        }
        AddText("LAP", 258, 50, 8, 1);
        if (ships[ownShip].lap > classLaps[config->shipClass])
            AddJustifyNumber(classLaps[config->shipClass], 263, 60, 16, 0);
        else
            AddJustifyNumber(ships[ownShip].lap, 263, 60, 16, 0);
        AddText("OF", 263, 68, 8, 1);
        AddNumber(classLaps[config->shipClass], 285, 60, 16, 0);
        if (ships[ownShip].rearTarget != -1 && rearLockCycle < 13 && ships[ownShip].weapon == 2 && !pauseRequested)
            PrintCX("REAR LOCK", 168, 70, 0);
        if (perfectLapCount > 0) CentreText("PERFECT LAP", 120, 160, 8, 0);
        if (trackNameScroll > -480 && !networkGame) {
            if (cdMusicTrack != 0)
                wsprintfA(trackNameText, "CD TRACK: %d   %s [%s]", cdMusicTrack,
                          GetCdTrackName(cdMusicTrack - 1), GetCdTrackArtist(cdMusicTrack - 1));
            else
                wsprintfA(trackNameText, "CD TRACK: %d   %s [%s]", cdMusicTrack,
                          GetCdTrackName(cdMusicTrack), GetCdTrackArtist(cdMusicTrack));
            if (cdMusicCount != 0) Print(trackNameText, 16, (trackNameOffset >> 2) + 227, 0);
        }
        for (row = 0; row < 15; row++) {
            if (!(ships[row].combatAttr & DESTROYED)) racingCount++;
        }
        if (!networkGame || gstRaceMode == 0) AddText("CHECK", 20, 20, 8, 1);
        AddText("POS", 258, 20, 8, 1);
        AddJustifyNumber(currentRacePosition, 263, 30, 16, 0);
        AddText("OF", 263, 38, 8, 1);
        AddNumber(racingCount, 285, 30, 16, 0);
        if ((ships[ownShip].attr & RACING) && ships[ownShip].lap >= 0) {
            if (checkTimer <= 0) {
                if (blinkFlag) AddTime(checkTimer, 20, 30, 16, 1, 1);
            } else if (!networkGame || gstRaceMode == 0) {
                AddTime(checkTimer, 20, 30, 16, 0, 1);
            }
        }
        if (autopilot == 0 && shipDamage[ownShip] < 100 && !(ships[ownShip].attr & DIRECTION) &&
            ships[ownShip].nearTrkSect->junction.ptr == (TrackSection *)-1 &&
            ships[ownShip].nearTrkSect->nextSection.ptr->junction.ptr == (TrackSection *)-1)
            CentreText("WRONG WAY", 50, 110, 8, 1);
        AddJustifyNumber(spd / 95, 250, 168, 16, 0);
    }
    if (ships[ownShip].attr & RACING) {
        if (ships[ownShip].attr & RESCUE) ships[ownShip].speed = 0;
        fill_speedo(ships, config, choice);
    }
    DrawHUDMessage();
}
#endif

#ifdef NETPLAY
/* NET build: the statements marked NET_POSITION_STEP sit behind a constant test with an empty else (MSVC /Od keeps a jump over the else). */
#define NET_POSITION_STEP 1
short CalcPosition(ShipData *ships)
{
    short racePosition = 1;
    long playerDist, shipDist;
    VECTOR direction, playerOffset;
    long playerFacing;
    VECTOR otherOffset;
    short currProgress;
    long shipBearing;
    short shipNum;

    currProgress = ships[ownShip].raceProgress.distance;
    playerOffset.vx = ships[ownShip].ppivot.vx - ships[ownShip].nearTrkSect->centre.vx;
    playerOffset.vy = ships[ownShip].ppivot.vy - ships[ownShip].nearTrkSect->centre.vy;
    playerOffset.vz = ships[ownShip].ppivot.vz - ships[ownShip].nearTrkSect->centre.vz;
    direction.vx = ships[ownShip].nearTrkSect->nextSection.ptr->centre.vx - ships[ownShip].nearTrkSect->centre.vx;
    direction.vy = ships[ownShip].nearTrkSect->nextSection.ptr->centre.vy - ships[ownShip].nearTrkSect->centre.vy;
    direction.vz = ships[ownShip].nearTrkSect->nextSection.ptr->centre.vz - ships[ownShip].nearTrkSect->centre.vz;
    for (shipNum = 0; shipNum < 15; shipNum++) {
        if (shipNum != ownShip) {
            if (!(ships[shipNum].combatAttr & DESTROYED)) {
                if (currProgress == ships[shipNum].raceProgress.distance) {
                    otherOffset.vx = ships[shipNum].ppivot.vx - ships[shipNum].nearTrkSect->centre.vx;
                    otherOffset.vy = ships[shipNum].ppivot.vy - ships[shipNum].nearTrkSect->centre.vy;
                    otherOffset.vz = ships[shipNum].ppivot.vz - ships[shipNum].nearTrkSect->centre.vz;
                    playerFacing = GetAng(playerOffset, direction);
                    if (NET_POSITION_STEP) {
                        shipBearing = GetAng(otherOffset, direction);
                    } else {
                    }
                    /* The NET build reads its own copies of 4096.0 and 2pi (not the shared named constants). */
                    if (NET_POSITION_STEP) {
                        playerDist = ((long)(cos(((playerFacing >> 3) / 4096.0) * 6.28318530718) * 4096.0) * GetMagnitude(playerOffset.vx, playerOffset.vy, playerOffset.vz)) >> 12;
                    } else {
                    }
                    shipDist = ((long)(cos(((shipBearing >> 3) / 4096.0) * 6.28318530718) * 4096.0) * GetMagnitude(otherOffset.vx, otherOffset.vy, otherOffset.vz)) >> 12;
                    if (shipDist >= playerDist) racePosition++;
                } else if (currProgress < ships[shipNum].raceProgress.distance) {
                    racePosition++;
                }
            }
        }
    }
    return racePosition;
}
#else
short CalcPosition(ShipData *ships)
{
    VECTOR playerOffset, shipOffset, direction;
    long playerDistance, shipDistance, playerAngle, shipAngle;
    short playerProgress;
    short shipNo;
    short position = 1;

    playerProgress = ships[ownShip].raceProgress.distance;
    playerOffset.vx = ships[ownShip].ppivot.vx - ships[ownShip].nearTrkSect->centre.vx;
    playerOffset.vy = ships[ownShip].ppivot.vy - ships[ownShip].nearTrkSect->centre.vy;
    playerOffset.vz = ships[ownShip].ppivot.vz - ships[ownShip].nearTrkSect->centre.vz;
    direction.vx = ships[ownShip].nearTrkSect->nextSection.ptr->centre.vx - ships[ownShip].nearTrkSect->centre.vx;
    direction.vy = ships[ownShip].nearTrkSect->nextSection.ptr->centre.vy - ships[ownShip].nearTrkSect->centre.vy;
    direction.vz = ships[ownShip].nearTrkSect->nextSection.ptr->centre.vz - ships[ownShip].nearTrkSect->centre.vz;
    for (shipNo = 0; shipNo < 15; shipNo++) {
        if (shipNo == ownShip) continue;
        if (!(ships[shipNo].combatAttr & DESTROYED)) {
            if (playerProgress == ships[shipNo].raceProgress.distance) {
                shipOffset.vx = ships[shipNo].ppivot.vx - ships[shipNo].nearTrkSect->centre.vx;
                shipOffset.vy = ships[shipNo].ppivot.vy - ships[shipNo].nearTrkSect->centre.vy;
                shipOffset.vz = ships[shipNo].ppivot.vz - ships[shipNo].nearTrkSect->centre.vz;
                playerAngle = GetAng(playerOffset, direction);
                shipAngle = GetAng(shipOffset, direction);
                playerDistance = ((long)(cos(((playerAngle >> 3) * angleScale) * fullCircle) * fixedPointScale) * GetMagnitude(playerOffset.vx, playerOffset.vy, playerOffset.vz)) >> 12;
                shipDistance = ((long)(cos(((shipAngle >> 3) * angleScale) * fullCircle) * fixedPointScale) * GetMagnitude(shipOffset.vx, shipOffset.vy, shipOffset.vz)) >> 12;
                if (shipDistance >= playerDistance) position++;
            } else if (playerProgress < ships[shipNo].raceProgress.distance) {
                position++;
            }
        }
    }
    return position;
}
#endif


#ifdef NETPLAY
/* Awards the medal for the finishing position and unlocks the extra challenges once enough tracks are won. */
void AssignMedals(RaceConfig *config)
{
    short trackIndex;
    short wins = 0;

    if (gameType == 0 || gameType == 2 || 0) {
        if (finishPosition >= 1 && finishPosition <= 3) {
            switch (finishPosition) {
            case 1:
                if (tracksWon[config->trackNum].medal == 0)
                    tracksWon[config->trackNum].medal = 1;
                else if ((short)tracksWon[config->trackNum].medal > finishPosition)
                    tracksWon[config->trackNum].medal = 1;
                awardedMedal = 1;
                envmapr = 255;
                envmapg = 128;
                envmapb = 0;
                break;
            case 2:
                if (tracksWon[config->trackNum].medal == 0)
                    tracksWon[config->trackNum].medal = 2;
                else if ((short)tracksWon[config->trackNum].medal > finishPosition)
                    tracksWon[config->trackNum].medal = 2;
                awardedMedal = 2;
                envmapr = 224;
                envmapg = 224;
                envmapb = 224;
                break;
            case 3:
                if (tracksWon[config->trackNum].medal == 0)
                    tracksWon[config->trackNum].medal = 3;
                else if ((short)tracksWon[config->trackNum].medal > finishPosition)
                    tracksWon[config->trackNum].medal = 3;
                awardedMedal = 3;
                envmapr = 192;
                envmapg = 32;
                envmapb = 0;
                break;
            }
        }
        for (trackIndex = 0; trackIndex < 8; trackIndex++) {
            if (tracksWon[trackIndex].medal == 1)
                wins++;
        }
        if (wins == 6 && config->phantomUnlocked == 0) {
            config->phantomChallenge = 1;
            sixTracks = 1;
        }
        if (wins == 8) {
            config->piranhaChallenge = 1;
            config->phantomChallenge = 0;
            eightTracks = 1;
            if (config->shipClass == 3) {
                menu[8].selection = 7;
                savedTrackSelection = menu[8].selection;
            }
        }
    }
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* Equivalent; the final menu[8] store picks eax and ecx the other way round. */
void AssignMedals(RaceConfig *config)
{
    short goldCount = 0;
    short trackIndex;
    short position;

    if (gameType == 0 || gameType == 2) {
        position = finishPosition;
        if (position >= 1 && position <= 3) {
            switch (position) {
            case 1:
                if (tracksWon[config->trackNum].medal == 0 || (short)tracksWon[config->trackNum].medal > position)
                    tracksWon[config->trackNum].medal = 1;
                awardedMedal = 1;
                envmapr = 255;
                envmapg = 128;
                envmapb = 0;
                break;
            case 2:
                if (tracksWon[config->trackNum].medal == 0 || (short)tracksWon[config->trackNum].medal > position)
                    tracksWon[config->trackNum].medal = 2;
                awardedMedal = 2;
                envmapr = 224;
                envmapg = 224;
                envmapb = 224;
                break;
            case 3:
                if (tracksWon[config->trackNum].medal == 0 || (short)tracksWon[config->trackNum].medal > position)
                    tracksWon[config->trackNum].medal = 3;
                awardedMedal = 3;
                envmapr = 192;
                envmapg = 32;
                envmapb = 0;
                break;
            }
        }
        for (trackIndex = 0; trackIndex < 8; trackIndex++) {
            if (tracksWon[trackIndex].medal == 1)
                goldCount++;
        }
        if (goldCount == 6 && config->phantomUnlocked == 0) {
            sixTracks = 1;
            config->phantomChallenge = 1;
        }
        if (goldCount == 8) {
            eightTracks = 1;
            config->piranhaChallenge = 1;
            config->phantomChallenge = 0;
            if (config->shipClass == 3) {
                menu[8].selection = 7;
                savedTrackSelection = 7;
            }
        }
    }
}
#endif
#endif

void copyInTempLapTimes(ShipData *ships)
{
    int count;
    for (count = 0; count < 5; count++) {
        savedLapTimes[count] = ships[ownShip].lapTimes[count] - ships[ownShip].lapTimes[count] % 6;
    }
}

void EndChallenge(RaceConfig *config)
{
    if (gameType == 2 && config->phantomChallenge == 1) {
        config->phantomCircuit = 0;
        gameType = 99;
    } else if (gameType == 2 && config->piranhaChallenge != 0) {
        config->piranhaCircuit = 0;
        gameType = 99;
    }
}


#ifdef EQUIV
/* Equivalent; only the VENOM string copy picks its registers the other way round. */
void DisplayRaceInfo(RaceConfig *config, ScoreData *score, ShipData *ships)
{
#ifdef NETPLAY
    int total = 0;
    short shipsLeft = 0;
    short eliminations = 0;
    short numShips = 0;
    char text[40];

    numShips = config->shipClass == 3 ? 15 : 12;
    if (ownShip >= 12) numShips = 15;
    {
        short i;
        for (i = 0; i < 15; i++) {
            if (!(ships[i].combatAttr & DESTROYED)) shipsLeft++;
        }
    }
    eliminations = numShips - shipsLeft;
    if (raceOutcome == 2) {
        if (ships[ownShip].combatAttr & 0x2d) {
            CentreText("BAD LUCK", 15, 24, 8, 1);
            CentreText("YOU RAN OUT OF TIME", 15, 60, 8, 1);
        } else if (shipDeathCount[ownShip] < 8) {
            CentreText("BAD LUCK", 15, 24, 8, 1);
            CentreText("YOUR SHIP WAS DESTROYED", 45, 60, 8, 1);
        }
        score->totalTime = 0;
        if (ships[ownShip].lap > 1) {
            short j;

            AddText("RACE STATISTICS", 15, 100, 8, 1);
            for (j = 0; j < ships[ownShip].lap - 1; j++) {
                AddText("LAP", 25, j * 12 + 115, 8, 1);
                AddNumber(j + 1, 65, j * 12 + 115, 8, 1);
                AddTime(savedLapTimes[j], 85, j * 12 + 115, 8, 0, 1);
                score->totalTime += savedLapTimes[j];
            }
        }
    } else {
        if (gameType != 1) {
            if (finishPosition <= 3) {
                CentreText("CONGRATULATIONS", 15, 24, 8, 1);
                switch (config->shipClass) {
                case 0: strcpy(text, "VECTOR"); break;
                case 1: strcpy(text, "VENOM"); break;
                case 2: strcpy(text, "RAPIER"); break;
                case 3: strcpy(text, "PHANTOM"); break;
                }
                switch (finishPosition) {
                case 1: strcat(text, " GOLD"); break;
                case 2: strcat(text, " SILVER"); break;
                case 3: strcat(text, " BRONZE"); break;
                }
                strcat(text, " AWARDED");
                CentreText(text, 15, 48, 8, 0);
            } else {
                CentreText("BAD LUCK", 15, 24, 8, 1);
                CentreText("NO MEDAL AWARDED", 15, 48, 8, 0);
            }
            AddText("RACE POSITION", 15, 74, 8, 1);
            AddNumber(finishPosition, 155, 74, 8, 0);
            if (eliminations != 0) {
                AddText("ELIMINATIONS", 15, 196, 8, 1);
                AddNumber(eliminations, 144, 196, 8, 0);
            }
        }
        AddText("RACE STATISTICS", 15, 100, 8, 1);
        score->totalTime = 0;
        {
            short k;
            for (k = 0; k < classLaps[config->shipClass]; k++) {
                AddText("LAP", 25, k * 12 + 115, 8, 1);
                AddNumber(k + 1, 65, k * 12 + 115, 8, 1);
                AddTime(savedLapTimes[k], 85, k * 12 + 115, 8, 0, 1);
                score->totalTime += savedLapTimes[k];
            }
        }
        if ((gameType == 0 || gameType == 2) && finishPosition <= 3) {
            DrawTeamShips(80, 0, 150, cardNum.angle, 0, teamShip);
        }
        AddText("RACE TIME", 15, 180, 8, 1);
        if (gameType == 0 || gameType == 2 || gameType == 1) {
            AddTime(score->totalTime, 128, 180, 8, 0, 1);
        } else {
            short m;
            for (m = 0; m < classLaps[config->shipClass]; m++) total += savedLapTimes[m];
            AddTime(total, 128, 180, 8, 0, 1);
        }
    }
    DoButton(127, 210, 2, "CONTINUE");
#else
    short j;
    int total = 0;
    short i;
    short remaining = 0;
    short k;
    short maxShips;
    short m;
    char text[40];

    maxShips = config->shipClass == 3 ? 15 : 12;
    if (ownShip >= 12) maxShips = 15;
    for (i = 0; i < 15; i++) {
        if (!(ships[i].combatAttr & DESTROYED)) remaining++;
    }
    maxShips -= remaining;
    if (raceOutcome == 2) {
        if (ships[ownShip].combatAttr & 0x2d) {
            CentreText("BAD LUCK", 15, 24, 8, 1);
            CentreText("YOU RAN OUT OF TIME", 15, 60, 8, 1);
        } else if (shipDeathCount[ownShip] < 8) {
            CentreText("BAD LUCK", 15, 24, 8, 1);
            CentreText("YOUR SHIP WAS DESTROYED", 45, 60, 8, 1);
        }
        score->totalTime = 0;
        if (ships[ownShip].lap > 1) {
            AddText("RACE STATISTICS", 15, 100, 8, 1);
            for (j = 0; j < ships[ownShip].lap - 1; j++) {
                AddText("LAP", 25, j * 12 + 115, 8, 1);
                AddNumber(j + 1, 65, j * 12 + 115, 8, 1);
                AddTime(savedLapTimes[j], 85, j * 12 + 115, 8, 0, 1);
                score->totalTime += savedLapTimes[j];
            }
        }
    } else {
        if (gameType != 1) {
            if (finishPosition <= 3) {
                CentreText("CONGRATULATIONS", 15, 24, 8, 1);
                switch (config->shipClass) {
                case 0: strcpy(text, "VECTOR"); break;
                case 1: strcpy(text, "VENOM"); break;
                case 2: strcpy(text, "RAPIER"); break;
                case 3: strcpy(text, "PHANTOM"); break;
                }
                switch (finishPosition) {
                case 1: strcat(text, " GOLD"); break;
                case 2: strcat(text, " SILVER"); break;
                case 3: strcat(text, " BRONZE"); break;
                }
                strcat(text, " AWARDED");
                CentreText(text, 15, 48, 8, 0);
            } else {
                CentreText("BAD LUCK", 15, 24, 8, 1);
                CentreText("NO MEDAL AWARDED", 15, 48, 8, 0);
            }
            AddText("RACE POSITION", 15, 74, 8, 1);
            AddNumber(finishPosition, 155, 74, 8, 0);
            if (maxShips != 0) {
                AddText("ELIMINATIONS", 15, 196, 8, 1);
                AddNumber(maxShips, 144, 196, 8, 0);
            }
        }
        AddText("RACE STATISTICS", 15, 100, 8, 1);
        score->totalTime = 0;
        for (k = 0; k < classLaps[config->shipClass]; k++) {
            AddText("LAP", 25, k * 12 + 115, 8, 1);
            AddNumber(k + 1, 65, k * 12 + 115, 8, 1);
            AddTime(savedLapTimes[k], 85, k * 12 + 115, 8, 0, 1);
            score->totalTime += savedLapTimes[k];
        }
        if ((gameType == 0 || gameType == 2) && finishPosition <= 3) {
            DrawTeamShips(80, 0, 150, cardNum.angle, 0, teamShip);
        }
        AddText("RACE TIME", 15, 180, 8, 1);
        if (gameType == 0 || gameType == 2 || gameType == 1) {
            AddTime(score->totalTime, 128, 180, 8, 0, 1);
        } else {
            for (m = 0; m < classLaps[config->shipClass]; m++) total += savedLapTimes[m];
            AddTime(total, 128, 180, 8, 0, 1);
        }
    }
    SetTextQuad(&menuBarPoly, 127, 210, buttonIcons[2].u, buttonIcons[2].v,
               buttonIcons[2].width, buttonIcons[2].height, 32, 0);
    DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
    Print("CONTINUE", 145, 216, 0);
#endif
}
#endif

#ifdef EQUIV
#ifdef NETPLAY
/* The network build posts a lockstep packet instead of taking the autopilot path, and it
   leaves out the seconds/totalSeconds/bonus locals of the single-player body. */
void gstPostLockstepAsynch(void *data, int size, u_char port);
extern u_char lockstepPosted[];

void AlterCheckPoint(struct CheckPoints *checkpoints, ShipData *ships, int targetShip, RaceConfig *config)
{
    Face *face;
    int tone;
    short i;
    unsigned char packet;

    if (networkGame && gstRaceMode) return;
    face = &track->faces[ships[ownShip].nearTrkSect->faceStart];
    while (!(face->flags & TRACK_BASE)) face++;
    if (face->flags & CHECKPOINT) {
        for (i = 0; i < 6; i++) {
            if (checkpoints[i].section == ships[ownShip].nearTrkSect->secNo) {
                if (checkpoints[i].reached < ships[ownShip].lap &&
                    ((ships[ownShip].attr & DIRECTION) || ships[ownShip].nearTrkSect->junction.index != -1 ||
                     ships[ownShip].nearTrkSect->nextSection.ptr->junction.index != -1)) {
                    checkTimer += checkpoints[i].time * 60;
                    glyphIndex.bonusFrames = 60;
                    ships[ownShip].combatAttr &= ~8;
                    checkpoints[i].reached = ships[ownShip].lap;
                    PlayNote(SAMPLE_CHECKPOINT, 255, 15, 15, 256);
                }
                break;
            }
        }
    }
    if ((ships[ownShip].attr & RACING) && ships[ownShip].lap >= 0 && glyphIndex.bonusFrames > 0) {
        if (blinkFlag) extendedTimeFlag = 1;
        glyphIndex.bonusFrames--;
    }
    if (targetShip == -1 && ships[ownShip].lap < classLaps[config->shipClass] + 1 && ships[ownShip].lap >= 0) {
        if (checkTimer > 0) {
            checkTimer -= 2;
            if (checkTimer / 60 % 60 <= 10 && checkTimer / 6 % 10 == 0 && checkTimer / 60 / 60 == 0) {
                tone = 10 - checkTimer / 60 % 60;
                if (shipDeathCount[ownShip] == -1)
                    PlayNote(SAMPLE_CHECKPOINT, 255, 15, 15, tone * 8 + 260);
            }
            if (cheatTime) checkTimer = 35999;
        } else if (!noCheckPoints) {
            if (!(ships[ownShip].combatAttr & DESTROYED)) {
                if (networkGame && !lockstepPosted[ownShip]) {
                    packet = 5;
                    lockstepPosted[ownShip] = 1;
                    gstPostLockstepAsynch(&packet, 1, 1);
                } else if (!networkGame) {
                    ships[ownShip].combatAttr |= 8;
                }
                if (ships[ownShip].autopilotCount > 0) {
                    ships[ownShip].autopilotCount = 0;
                    ships[ownShip].update = UpdatePlayerShip;
                    ships[ownShip].vhdg = 0;
                }
            }
        }
    }
}
#else
/* Equivalent; the compare and the final stores are ordered differently. */
void AlterCheckPoint(struct CheckPoints *checkpoints, ShipData *ships, int targetShip, RaceConfig *config)
{
    int seconds;
    Face *face;
    int totalSeconds;
    ShipData *player;
    short i;
    int bonus;
    int pitch;
    TrackSection *section;

    section = ships[ownShip].nearTrkSect;
    face = &track->faces[section->faceStart];
    while (!(face->flags & TRACK_BASE)) face++;
    if (face->flags & CHECKPOINT) {
        for (i = 0; i < 6; i++) {
            if (checkpoints[i].section == section->secNo) {
                if (checkpoints[i].reached < ships[ownShip].lap &&
                    ((ships[ownShip].attr & DIRECTION) || section->junction.index != -1 ||
                     section->nextSection.ptr->junction.index != -1)) {
                    bonus = checkpoints[i].time * 60;
                    checkTimer += bonus;
                    glyphIndex.bonusFrames = 60;
                    ships[ownShip].combatAttr &= ~8;
                    checkpoints[i].reached = ships[ownShip].lap;
                    PlayNote(SAMPLE_CHECKPOINT, 255, 15, 15, 256);
                }
                break;
            }
        }
    }
    if ((ships[ownShip].attr & RACING) && ships[ownShip].lap >= 0 && glyphIndex.bonusFrames > 0) {
        if (blinkFlag) extendedTimeFlag = 1;
        glyphIndex.bonusFrames--;
    }
    player = &ships[ownShip];
    if (targetShip == -1 && player->lap < classLaps[config->shipClass] + 1 && player->lap >= 0) {
        if (checkTimer > 0) {
            checkTimer -= 2;
            totalSeconds = checkTimer / 60;
            seconds = totalSeconds % 60;
            if (seconds <= 10 && (checkTimer / 6) % 10 == 0 && totalSeconds / 60 == 0) {
                pitch = 10 - seconds;
                if (shipDeathCount[ownShip] == -1)
                    PlayNote(SAMPLE_CHECKPOINT, 255, 15, 15, pitch * 8 + 260);
            }
            if (cheatTime) checkTimer = 35999;
        } else if (!noCheckPoints) {
            if (!(player->combatAttr & DESTROYED)) {
                player->combatAttr |= 8;
                if (player->autopilotCount > 0) {
                    player->autopilotCount = 0;
                    player->update = UpdatePlayerShip;
                    player->vhdg = 0;
                }
            }
        }
    }
}
#endif
#endif

#ifdef EQUIV
#ifdef NETPLAY
void DisplayContinues(short x, short y)
{
    short cellHeight;
    short index;
    short quadWidth;
    short gap;
    short top;
    short startX;
    short ratio;
    short penX;
    int iconHeight;
    int iconW;

    penX = 0;
    gap = 20;
    iconW = 64;
    iconHeight = 64;
    x *= 2;
    y *= 2;
    for (index = 0; continuesLeft > index; index++)
        penX += iconW;
    penX = (continuesLeft - 1) * gap + penX;
    penX = x - (penX >> 1);
    for (index = 0; continuesLeft > index; index++) {
        if (continuesLeft - 1 == index)
            ratio = 64 - continueAnimFrame * 2;
        else
            ratio = 64;
        quadWidth = ratio * iconW / 64;
        cellHeight = ratio * iconHeight / 64;
        startX = penX - quadWidth / 2;
        top = y - cellHeight / 2;
        SetTextQuad(&menuBarPoly, 0, 0, 190, 160, iconW, iconHeight, 32, 0);
        menuBarPoly.x0 = startX;
        menuBarPoly.y0 = top;
        menuBarPoly.x1 = startX + quadWidth;
        menuBarPoly.y1 = top;
        menuBarPoly.x2 = startX;
        menuBarPoly.y2 = top + cellHeight;
        menuBarPoly.x3 = startX + quadWidth;
        menuBarPoly.y3 = top + cellHeight;
        DrawScaledTexturedQuad(0.3f, &menuBarPoly, hudTexturePage);
        penX = gap + penX + iconW;
    }
}
#endif
#endif
#ifndef NETPLAY
void DisplayContinues(short x, short y)
{

    short totalWidth = 0;
    short i = 0;
    short width, scale, left, top;
    signed char count;
    x *= 2;
    y *= 2;
    if (continuesLeft > 0) totalWidth = continuesLeft * 64;
    x -= (short)(totalWidth + (continuesLeft - 1) * 20) >> 1;
    for (i = 0; i < (count = continuesLeft); i++) {
        if (count - i == 1) scale = (32 - continueAnimFrame) * 2;
        else scale = 64;
        left = x;
        top = y;
        width = scale * 64 / 64;
        left -= width / 2;
        top -= width / 2;
        x += 84;
        SetTextQuad(&menuBarPoly, 0, 0, 190, 160, 64, 64, 32, 0);
        menuBarPoly.x0 = left; menuBarPoly.y0 = top;
        menuBarPoly.x1 = left + width; menuBarPoly.y1 = top;
        menuBarPoly.x2 = left; menuBarPoly.y2 = top + width;
        menuBarPoly.x3 = left + width; menuBarPoly.y3 = top + width;
        DrawScaledTexturedQuad(0.3f, &menuBarPoly, hudTexturePage);
    }
}
#endif


void DoButton(short x, short y, short icon, char *text)
{
    SetTextQuad(&menuBarPoly, x, y, buttonIcons[icon].u,
               buttonIcons[icon].v, buttonIcons[icon].width,
               buttonIcons[icon].height, 32, 0);
    DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
    if (icon == 1) Print(text, x - 25, y + 6, 0);
    else Print(text, x + 18, y + 6, 0);
}





void TickHUDMessage(void)
{
    if (messageTimer > 0)
        messageTimer--;
}

void DrawHUDMessage(void)
{
    if (messageTimer > 0)
        Print(hudMessage, 210, 226, 1);
}

extern short fontRows16[];
extern short fontRows8[];
extern short fontRows12[];
extern FontGlyph fontGlyphs16[];
extern FontGlyph fontGlyphs8[];
extern FontGlyph fontGlyphs12[];

/* The loop counters live on the stack next to the table pointers, hence the struct. */
#ifdef EQUIV
#ifdef NETPLAY
void InitText(void)
{
    short numRows;
    short tableIndex;
    short *rowTable;
    short glyphLeft;
    short fontSel;
    short glyphTop;
    FontGlyph *currGlyph;
    short quadWidth;
    short glyphIndex;
    FontGlyph *glyphSets[3];
    short rowIndex;
    short *rowPtrArr[3];
    short remaining;

    rowPtrArr[0] = fontRows16;
    rowPtrArr[1] = fontRows8;
    rowPtrArr[2] = fontRows12;
    glyphSets[0] = fontGlyphs16;
    glyphSets[1] = fontGlyphs8;
    glyphSets[2] = fontGlyphs12;
    for (fontSel = 0; fontSel < 3; fontSel++) {
        rowTable = rowPtrArr[fontSel];
        currGlyph = glyphSets[fontSel];
        tableIndex = 0;
        numRows = rowTable[tableIndex++];
        for (rowIndex = 0; numRows > rowIndex; rowIndex++) {
            glyphTop = rowTable[tableIndex];
            remaining = rowTable[tableIndex + 1];
            tableIndex += 2;
            for (glyphIndex = 0; remaining > glyphIndex; glyphIndex++) {
                glyphLeft = rowTable[tableIndex];
                quadWidth = rowTable[tableIndex + 1] - glyphLeft;
                currGlyph->vramX = glyphLeft;
                currGlyph->vramY = glyphTop;
                if (fontSel == 2)
                    currGlyph->width = quadWidth - 2;
                else
                    currGlyph->width = quadWidth - 1;
                currGlyph->advance = quadWidth;
                currGlyph++;
                tableIndex++;
            }
            tableIndex++;
        }
    }
}
#endif
#endif
#ifndef NETPLAY
void InitText(void)
{

    struct {
        short pad;
        short fontIndex;
        short numRows;
        short pad2;
        short *rows[3];
        FontGlyph *fonts[3];
    } locals;
    FontGlyph *currGlyph;
    short *rowTable;
    short tableIndex;
    short glyphY;
    short glyphsLeft;
    short glyphX;
    short glyphWidth;

    locals.rows[0] = fontRows16;
    locals.rows[1] = fontRows8;
    locals.rows[2] = fontRows12;
    locals.fonts[0] = fontGlyphs16;
    locals.fonts[1] = fontGlyphs8;
    locals.fonts[2] = fontGlyphs12;
    locals.fontIndex = 0;
    for (; locals.fontIndex < 3; locals.fontIndex++) {
        rowTable = locals.rows[locals.fontIndex];
        currGlyph = locals.fonts[locals.fontIndex];
        tableIndex = 1;
        if (rowTable[0] > 0) {
            locals.numRows = rowTable[0];
            do {
                glyphY = rowTable[tableIndex];
                glyphsLeft = rowTable[tableIndex + 1];
                tableIndex += 2;
                if (glyphsLeft > 0) {
                    do {
                        currGlyph->vramY = glyphY;
                        glyphX = rowTable[tableIndex];
                        glyphWidth = rowTable[tableIndex + 1] - glyphX;
                        currGlyph->vramX = glyphX;
                        if (locals.fontIndex == 2)
                            currGlyph->width = glyphWidth - 2;
                        else
                            currGlyph->width = glyphWidth - 1;
                        currGlyph->advance = glyphWidth;
                        currGlyph++;
                        tableIndex++;
                    } while (--glyphsLeft);
                }
                tableIndex++;
            } while (--locals.numRows);
        }
    }
}
#endif


extern u_char numberGlyphLookup[];


extern float textDepth;
void DrawScaledTexturedQuad(float depth, POLY_FT4 *quad, int type);

void AddText(u_char *text, long x, long y, long size, long colour)
{
#ifdef NETPLAY
    u_char character;
    u_char glyphNum;
    u_char texU;
    u_char texV;
    short page;
    u_char quadWidth;
    FontGlyph *glyphs;
    long cursorX;

    if (size == 16) {
        glyphs = fontGlyphs16;
        page = 32;
    } else if (size == 8) {
        glyphs = fontGlyphs8;
        page = 32;
    }
    cursorX = x;
    while (1) {
        character = *text++;
        if (character == 0)
            break;
        if (character >= 32 && character < 96)
            glyphNum = glyphIndex.character[character];
        else
            glyphNum = 50;
        if (glyphNum == 50)
            cursorX += 8;
        else {
            quadWidth = glyphs[glyphNum].width;
            texU = glyphs[glyphNum].vramX;
            texV = glyphs[glyphNum].vramY;
            SetTextQuad(&menuBarPoly, cursorX, y, texU, texV, quadWidth, size * 2, page, colour);
            DrawScaledTexturedQuad(textDepth, &menuBarPoly, hudTexturePage);
            cursorX += glyphs[glyphNum].advance >> 1;
        }
    }
#else
    u_char letter;
    long xPos;
    FontGlyph *font;
    FontGlyph *glyph;
    u_char u, v, width;
    short tpage;

    if (size == 16) {
        tpage = 32;
        font = fontGlyphs16;
    } else if (size == 8) {
        tpage = 32;
        font = fontGlyphs8;
    }
    xPos = x;
    while ((letter = *text++) != 0) {
        if (letter >= 32 && letter < 96)
            letter = glyphIndex.character[letter];
        else
            letter = 50;
        if (letter == 50)
            xPos += 8;
        else {
            glyph = &font[letter];
            width = glyph->width;
            u = glyph->vramX;
            v = glyph->vramY;
            SetTextQuad(&menuBarPoly, xPos, y, u, v,
                         width, (u_char)(size * 2), tpage, colour);
            DrawScaledTexturedQuad(textDepth, &menuBarPoly, hudTexturePage);
            xPos += glyph->advance >> 1;
        }
    }
#endif
}


void AddNumber(int number, int x, int y, int size, int colour)
{
#ifdef NETPLAY
    u_char glyphNum;
    u_char u;
    u_char v;
    int digitPos;
    int digit;
    FontGlyph *fontGlyphs;
    short tp;
    int nums[12];
    u_char quadWidth;
    long cursorX;

    if (size == 16) {
        fontGlyphs = fontGlyphs16;
        tp = 32;
    } else if (size == 8) {
        fontGlyphs = fontGlyphs8;
        tp = 32;
    }
    for (digit = 0; digit < 5; digit++) {
        nums[digit] = number % 10;
        number /= 10;
        if (number == 0)
            break;
    }
    cursorX = x;
    for (digitPos = digit; digitPos >= 0; digitPos--) {
        glyphNum = numberGlyphLookup[nums[digitPos]];
        quadWidth = fontGlyphs[glyphNum].width;
        u = fontGlyphs[glyphNum].vramX;
        v = fontGlyphs[glyphNum].vramY;
        SetTextQuad(&menuBarPoly, cursorX, y, u, v, quadWidth, size * 2, tp, colour);
        DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
        cursorX += fontGlyphs[glyphNum].advance >> 1;
    }
#else
    short tpage;
    u_char u;
    FontGlyph *font;
    int digits[12];
    int length;
    u_char v;
    u_char width;
    FontGlyph *glyph;
    int *digit;
    long xPos;

    if (size == 16) {
        font = fontGlyphs16;
        tpage = 32;
    } else if (size == 8) {
        font = fontGlyphs8;
        tpage = 32;
    }
    length = 0;
    digit = digits;
    for (;;) {
        *digit = number % 10;
        number /= 10;
        if (number == 0)
            break;
        digit++;
        length++;
        if (digit >= &digits[5])
            break;
    }
    xPos = x;
    if (length >= 0) {
        digit = &digits[length];
        do {
            glyph = &font[numberGlyphLookup[*digit]];
            width = glyph->width;
            v = glyph->vramY;
            u = glyph->vramX;
            SetTextQuad(&menuBarPoly, xPos, y, u, v, width, (u_char)(size * 2), tpage, colour);
            DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
            xPos += glyph->advance >> 1;
        } while (--digit >= digits);
    }
#endif
}


void AddJustifyNumber(long number, long x, long y, long size, long colour)
{
#ifdef NETPLAY
    u_char glyphNum;
    u_char u;
    u_char v;
    int digitPos;
    int digit;
    FontGlyph *fontGlyphs;
    short tp;
    int nums[12];
    u_short quadWidth;
    long cursorX;

    if (size == 16) {
        fontGlyphs = fontGlyphs16;
        tp = 32;
    } else if (size == 8) {
        fontGlyphs = fontGlyphs8;
        tp = 32;
    }
    for (digit = 0; digit < 5; digit++) {
        nums[digit] = number % 10;
        number /= 10;
        if (number == 0)
            break;
    }
    for (digitPos = digit; digitPos >= 0; digitPos--) {
        glyphNum = numberGlyphLookup[nums[digitPos]];
        quadWidth = fontGlyphs[glyphNum].advance;
        x -= quadWidth >> 1;
    }
    cursorX = x;
    for (digitPos = digit; digitPos >= 0; digitPos--) {
        glyphNum = numberGlyphLookup[nums[digitPos]];
        quadWidth = fontGlyphs[glyphNum].width;
        u = fontGlyphs[glyphNum].vramX;
        v = fontGlyphs[glyphNum].vramY;
        SetTextQuad(&menuBarPoly, cursorX, y, u, v, quadWidth, size * 2, tp, colour);
        DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
        cursorX += fontGlyphs[glyphNum].advance >> 1;
    }
#else

    long digits[12];
    int length;
    int i;
    u_short width;
    u_char u, v;
    FontGlyph *font;
    FontGlyph *glyph;
    short tpage;

    if (size == 16) {
        tpage = 32;
        font = fontGlyphs16;
    } else if (size == 8) {
        tpage = 32;
        font = fontGlyphs8;
    }
    length = 0;
    do {
        digits[length] = number % 10;
        number /= 10;
        if (number == 0)
            break;
        length++;
    } while (length < 5);
    for (i = length; i >= 0; i--) {
        width = font[numberGlyphLookup[digits[i]]].advance;
        x -= width >> 1;
    }
    for (i = length; i >= 0; i--) {
        glyph = &font[numberGlyphLookup[digits[i]]];
        u = glyph->vramX;
        v = glyph->vramY;
        SetTextQuad(&menuBarPoly, x, y, u, v,
                     glyph->width, (u_char)(size * 2), tpage, colour);
        DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
        x += glyph->advance >> 1;
    }
#endif
}


#ifdef NETPLAY
/* NET build: the frame is laid out from these names (MSVC /Od), and the minutes, seconds, tenths and digit count are zeroed first. */
void AddTime(int time, int xInput, int y, int size, int colour, signed char flags)
{
    int seconds;
    u_char glyphNum;
    u_char u;
    short count;
    u_char v;
    short tp;
    FontGlyph *font;
    int i;
    int numbers[20];
    int min;
    int penX;
    u_short quadWidth;
    int tenthValue;

    min = 0;
    seconds = 0;
    tenthValue = 0;
    count = 0;
    if (size == 16) {
        font = fontGlyphs16;
        tp = 32;
    } else if (size == 8) {
        font = fontGlyphs8;
        tp = 32;
    }
    tenthValue = (time / 6) % 10;
    time /= 60;
    seconds = time % 60;
    min = time / 60;
    if (flags & 1)
        numbers[0] = min % 10;
    else {
        numbers[0] = min % 10;
        if (min > 9) {
            min /= 10;
            numbers[1] = min % 10;
        } else
            numbers[1] = 0;
        count = 1;
    }
    penX = xInput;
    if (!(flags & 2)) {
        for (i = count; i >= 0; i--) {
            glyphNum = numberGlyphLookup[numbers[i]];
            quadWidth = font[glyphNum].width;
            u = font[glyphNum].vramX;
            v = font[glyphNum].vramY;
            SetTextQuad(&menuBarPoly, penX, y, u, v, quadWidth, size * 2, tp, colour);
            DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
            penX += font[glyphNum].advance >> 1;
        }
    }
    if (!(flags & 2)) {
        quadWidth = font[36].width;
        u = font[36].vramX;
        v = font[36].vramY;
        SetTextQuad(&menuBarPoly, penX, y, u, v, quadWidth, size * 2, tp, colour);
        DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
        penX += font[36].advance >> 1;
    }
    numbers[0] = seconds % 10;
    if (seconds > 9) {
        seconds /= 10;
        numbers[1] = seconds % 10;
    } else
        numbers[1] = 0;
    for (i = 1; i >= 0; i--) {
        glyphNum = numberGlyphLookup[numbers[i]];
        quadWidth = font[glyphNum].width;
        u = font[glyphNum].vramX;
        v = font[glyphNum].vramY;
        SetTextQuad(&menuBarPoly, penX, y, u, v, quadWidth, size * 2, tp, colour);
        DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
        penX += font[glyphNum].advance >> 1;
    }

    quadWidth = font[37].width;
    u = font[37].vramX;
    v = font[37].vramY;
    SetTextQuad(&menuBarPoly, penX, y, u, v, quadWidth, size * 2, tp, colour);
    DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
    penX += font[37].advance >> 1;
    font = fontGlyphs8;
    tp = 32;
    if (size != 8) {
        size = 8;
        y += 8;
    }
    glyphNum = numberGlyphLookup[tenthValue];
    quadWidth = font[glyphNum].width;
    u = font[glyphNum].vramX;
    v = font[glyphNum].vramY;
    SetTextQuad(&menuBarPoly, penX, y, u, v, quadWidth, size * 2, tp, colour);
    DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
    penX += font[glyphNum].advance >> 1;
}
#else
void AddTime(int time, int xInput, int y, int size, int colour, u_char flags)
{
    int x;
    short length;
    int digits[20];
    int minutes;
    int seconds;
    int tenths;
    int i;
    FontGlyph *font;
    FontGlyph *glyph;
    short tpage;
    u_char u, v;

    length = 0;
    if (size == 16) {
        tpage = 32;
        font = fontGlyphs16;
    } else if (size == 8) {
        tpage = 32;
        font = fontGlyphs8;
    }
    tenths = (time / 6) % 10;
    time /= 60;
    seconds = time % 60;
    minutes = time / 60;
    if (flags & 1)
        digits[0] = minutes % 10;
    else {
        digits[0] = minutes % 10;
        if (minutes > 9) {
            minutes /= 10;
            digits[1] = minutes % 10;
        } else
            digits[1] = 0;
        length = 1;
    }
    x = xInput;
    if (!(flags & 2)) {
        for (i = length; i >= 0; i--) {
            glyph = &font[numberGlyphLookup[digits[i]]];
            u = glyph->vramX;
            v = glyph->vramY;
            SetTextQuad(&menuBarPoly, x, y, u, v, glyph->width,
                     (u_char)(size * 2), tpage, colour);
            DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
            x += glyph->advance >> 1;
        }
    }
    if (!(flags & 2)) {
        u = font[36].vramX;
        v = font[36].vramY;
        SetTextQuad(&menuBarPoly, x, y, u, v, font[36].width,
                     (u_char)(size * 2), tpage, colour);
        DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
        x += font[36].advance >> 1;
    }
    digits[0] = seconds % 10;
    if (seconds > 9) {
        seconds /= 10;
        digits[1] = seconds % 10;
    } else
        digits[1] = 0;
    for (i = 1; i >= 0; i--) {
        glyph = &font[numberGlyphLookup[digits[i]]];
        u = glyph->vramX;
        v = glyph->vramY;
        SetTextQuad(&menuBarPoly, x, y, u, v, glyph->width,
                     (u_char)(size * 2), tpage, colour);
        DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
        x += glyph->advance >> 1;
    }

    u = font[37].vramX;
    v = font[37].vramY;
    SetTextQuad(&menuBarPoly, x, y, u, v, font[37].width,
                 (u_char)(size * 2), tpage, colour);
    DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
    x += font[37].advance >> 1;
    if (size != 8) {
        size = 8;
        y += 8;
    }

    u = fontGlyphs8[numberGlyphLookup[tenths]].vramX;
    v = fontGlyphs8[numberGlyphLookup[tenths]].vramY;
    SetTextQuad(&menuBarPoly, x, y, u, v, fontGlyphs8[numberGlyphLookup[tenths]].width,
                 (u_char)(size * 2), 32, colour);
    DrawScaledTexturedQuad(0.1f, &menuBarPoly, hudTexturePage);
}
#endif


void CentreText(u_char *text, long x, long y, long size, long colour)
{
#ifdef NETPLAY
    u_char character;
    u_char *strPos;
    u_char glyphNum;
    u_char u;
    u_char v;
    int textWidth;
    FontGlyph *fontGlyphs;
    short tp;
    u_short quadWidth;
    long cursorX;

    /* The NET build keeps the centred start in a local; the x argument is unused. */
    if (size == 16) {
        fontGlyphs = fontGlyphs16;
        tp = 32;
    } else if (size == 8) {
        fontGlyphs = fontGlyphs8;
        tp = 32;
    }
    strPos = text;
    textWidth = 0;
    for (;;) {
        character = *strPos++;
        if (character == 0)
            break;
        if (character >= 32 && character < 96)
            glyphNum = glyphIndex.character[character];
        else
            glyphNum = 50;
        if (glyphNum == 50)
            textWidth += 8;
        else
            textWidth += fontGlyphs[glyphNum].advance >> 1;
    }
    cursorX = 160 - (textWidth >> 1);
    for (;;) {
        character = *text++;
        if (character == 0)
            break;
        if (character >= 32 && character < 96)
            glyphNum = glyphIndex.character[character];
        else
            glyphNum = 50;
        if (glyphNum == 50)
            cursorX += 8;
        else {
            quadWidth = fontGlyphs[glyphNum].width;
            u = fontGlyphs[glyphNum].vramX;
            v = fontGlyphs[glyphNum].vramY;
            SetTextQuad(&menuBarPoly, cursorX, y, u, v, quadWidth, size * 2, tp, colour);
            DrawScaledTexturedQuad(textDepth, &menuBarPoly, hudTexturePage);
            cursorX += fontGlyphs[glyphNum].advance >> 1;
        }
    }
#else

    u_char *scan;
    u_char letter;
    FontGlyph *font;
    FontGlyph *glyph;
    int width;
    u_char u, v;
    short tpage;

    if (size == 16) {
        tpage = 32;
        font = fontGlyphs16;
    } else if (size == 8) {
        tpage = 32;
        font = fontGlyphs8;
    }
    scan = text;
    width = 0;
    for (;;) {
        letter = *scan++;
        if (letter == 0)
            break;
        if (letter >= 32 && letter < 96)
            letter = glyphIndex.character[letter];
        else
            letter = 50;
        if (letter == 50)
            width += 8;
        else
            width += font[letter].advance >> 1;
    }
    x = 160 - (width >> 1);
    while ((letter = *text++) != 0) {
        if (letter >= 32 && letter < 96)
            letter = glyphIndex.character[letter];
        else
            letter = 50;
        if (letter == 50)
            x += 8;
        else {
            glyph = &font[letter];
            u = glyph->vramX;
            v = glyph->vramY;
            SetTextQuad(&menuBarPoly, x, y, u, v,
                         glyph->width, (u_char)(size * 2), tpage, colour);
            DrawScaledTexturedQuad(textDepth, &menuBarPoly, hudTexturePage);
            x += glyph->advance >> 1;
        }
    }
#endif
}


void CentreText2(u_char *text, long x, long y, long size, long colour)
{
#ifdef NETPLAY
    u_char character;
    u_char *strPos;
    u_char glyphNum;
    u_char u;
    u_char v;
    int textWidth;
    FontGlyph *fontGlyphs;
    short tp;
    u_short quadWidth;
    long cursorX;

    if (size == 16) {
        fontGlyphs = fontGlyphs16;
        tp = 32;
    } else if (size == 8) {
        fontGlyphs = fontGlyphs8;
        tp = 32;
    }
    strPos = text;
    textWidth = 0;
    for (;;) {
        character = *strPos++;
        if (character == 0)
            break;
        if (character >= 32 && character < 96)
            glyphNum = glyphIndex.character[character];
        else
            glyphNum = 50;
        if (glyphNum == 50)
            textWidth += 8;
        else
            textWidth += fontGlyphs[glyphNum].advance >> 1;
    }
    cursorX = x - (textWidth >> 1);
    for (;;) {
        character = *text++;
        if (character == 0)
            break;
        if (character >= 32 && character < 96)
            glyphNum = glyphIndex.character[character];
        else
            glyphNum = 50;
        if (glyphNum == 50)
            cursorX += 8;
        else {
            quadWidth = fontGlyphs[glyphNum].width;
            u = fontGlyphs[glyphNum].vramX;
            v = fontGlyphs[glyphNum].vramY;
            SetTextQuad(&menuBarPoly, cursorX, y, u, v, quadWidth, size * 2, tp, colour);
            DrawScaledTexturedQuad(textDepth, &menuBarPoly, hudTexturePage);
            cursorX += fontGlyphs[glyphNum].advance >> 1;
        }
    }
#else

    u_char *scan;
    u_char letter;
    FontGlyph *font;
    FontGlyph *glyph;
    int width;
    u_char u, v;
    short tpage;

    if (size == 16) {
        tpage = 32;
        font = fontGlyphs16;
    } else if (size == 8) {
        tpage = 32;
        font = fontGlyphs8;
    }
    scan = text;
    width = 0;
    for (;;) {
        letter = *scan++;
        if (letter == 0)
            break;
        if (letter >= 32 && letter < 96)
            letter = glyphIndex.character[letter];
        else
            letter = 50;
        if (letter == 50)
            width += 8;
        else
            width += font[letter].advance >> 1;
    }
    x -= width >> 1;
    while ((letter = *text++) != 0) {
        if (letter >= 32 && letter < 96)
            letter = glyphIndex.character[letter];
        else
            letter = 50;
        if (letter == 50)
            x += 8;
        else {
            glyph = &font[letter];
            u = glyph->vramX;
            v = glyph->vramY;
            SetTextQuad(&menuBarPoly, x, y, u, v,
                         glyph->width, (u_char)(size * 2), tpage, colour);
            DrawScaledTexturedQuad(textDepth, &menuBarPoly, hudTexturePage);
            x += glyph->advance >> 1;
        }
    }
#endif
}


extern int textExtendedSpacing;
void Print(u_char *text, int x, int y, int colour)
{
    u_char u;
    u_char letter;
    u_short width;
    u_char currLett;
    u_char v;
    int xPos;

    xPos = x * 2;
    letter = *text++;
    if (letter != 0) {
        do {
            if (letter >= 32 && letter < 96)
                currLett = glyphIndex.character[letter];
            else
                currLett = 50;
            if (currLett == 50) {
                if (textExtendedSpacing == 0)
                    xPos += 8;
                else
                    xPos += 12;
            } else {
                v = fontGlyphs12[currLett].vramY;
                width = fontGlyphs12[currLett].width;
                u = fontGlyphs12[currLett].vramX;
                if (textExtendedSpacing == 0 && (currLett == 8 || currLett == 37)) {
                    width -= 6;
                    u += 4;
                    SetTextQuad(&menuBarPoly, xPos >> 1, y, u, v, width, 10, 32, colour);
                    DrawScaledTexturedQuad(textDepth, &menuBarPoly, hudTexturePage);
                    xPos += width;
                } else {
                    SetTextQuad(&menuBarPoly, xPos >> 1, y, u, v, width, 10, 32, colour);
                    DrawScaledTexturedQuad(textDepth, &menuBarPoly, hudTexturePage);
                    xPos += fontGlyphs12[currLett].advance;
                }
            }
            letter = *text++;
        } while (letter != 0);
    }
}

void PrintCX(char *text, int x, int y, int colour)
{
#ifdef NETPLAY
    u_char letter;
    char *scan;
    u_char glyphNum;
    u_char u;
    u_char v;
    int textWidth;
    u_char halfHeight;
    long penX;
    u_short quadWidth;

    halfHeight = 5;
    scan = text;
    textWidth = 0;
    for (;;) {
        letter = *scan++;
        if (letter == 0)
            break;
        if (letter >= 32 && letter < 96)
            glyphNum = glyphIndex.character[letter];
        else
            glyphNum = 50;
        if (glyphNum == 50)
            textWidth += 4;
        else if (glyphNum == 8 || glyphNum == 37)
            textWidth += 3;
        else
            textWidth += fontGlyphs12[glyphNum].advance >> 1;
    }
    penX = x - (textWidth >> 1);
    for (;;) {
        letter = *text++;
        if (letter == 0)
            break;
        if (letter >= 32 && letter < 96)
            glyphNum = glyphIndex.character[letter];
        else
            glyphNum = 50;
        if (glyphNum == 50)
            penX += 4;
        else {
            quadWidth = fontGlyphs12[glyphNum].width;
            u = fontGlyphs12[glyphNum].vramX;
            v = fontGlyphs12[glyphNum].vramY;
            if (glyphNum == 8 || glyphNum == 37) {
                quadWidth -= 3;
                u += 2;
                SetTextQuad(&menuBarPoly, penX, y, u, v, quadWidth, halfHeight * 2, 32, colour);
                DrawScaledTexturedQuad(textDepth, &menuBarPoly, hudTexturePage);
                penX += quadWidth >> 1;
            } else {
                SetTextQuad(&menuBarPoly, penX, y, u, v, quadWidth, halfHeight * 2, 32, colour);
                DrawScaledTexturedQuad(textDepth, &menuBarPoly, hudTexturePage);
                penX += fontGlyphs12[glyphNum].advance >> 1;
            }
        }
    }
#else

    char *scan;
    u_short width;
    u_char u;
    u_char letter;
    u_char currLett;
    u_char v;
    int textWidth;

    textWidth = 0;
    scan = text;
    for (;;) {
        letter = *scan++;
        if (letter == 0)
            break;
        if (letter >= 32 && letter < 96)
            currLett = glyphIndex.character[letter];
        else
            currLett = 50;
        if (currLett == 50)
            textWidth += 4;
        else if (currLett == 8 || currLett == 37)
            textWidth += 3;
        else
            textWidth += fontGlyphs12[currLett].advance >> 1;
    }
    x -= textWidth >> 1;
    letter = *text++;
    if (letter != 0) {
        do {
            if (letter >= 32 && letter < 96)
                currLett = glyphIndex.character[letter];
            else
                currLett = 50;
            if (currLett == 50)
                x += 4;
            else {
                width = fontGlyphs12[currLett].width;
                v = fontGlyphs12[currLett].vramY;
                u = fontGlyphs12[currLett].vramX;
                if (currLett == 8 || currLett == 37) {
                    width -= 3;
                    u += 2;
                    SetTextQuad(&menuBarPoly, x, y, u, v, width, 10, 32, colour);
                    DrawScaledTexturedQuad(textDepth, &menuBarPoly, hudTexturePage);
                    x += width >> 1;
                } else {
                    SetTextQuad(&menuBarPoly, x, y, u, v, width, 10, 32, colour);
                    DrawScaledTexturedQuad(textDepth, &menuBarPoly, hudTexturePage);
                    x += fontGlyphs12[currLett].advance >> 1;
                }
            }
            letter = *text++;
        } while (letter != 0);
    }
#endif
}


extern int doubleSourceRects;

void InitPanelQuads(void)
{
    SetTextQuad(&panelQuads[0], 205, 161, 0, 0, 224, 66, 33, 0);
    SetTextQuad(&panelQuads[1], 205, 196, 0, 68, 224, 52, 33, 0);
    if (doubleSourceRects) {
        SetTextQuad(&panelQuads[2], 144, 20, 0, 204, 96, 48, 34, 0);
        SetTextQuad(&panelQuads[3], 144, 44, 128, 204, 96, 48, 34, 0);
    } else {
        SetTextQuad(&panelQuads[2], 145, 20, 0, 204, 96, 48, 34, 0);
        SetTextQuad(&panelQuads[3], 145, 44, 128, 204, 96, 48, 34, 0);
    }
}

void SetTextQuad(POLY_FT4 *quad, short x, short y, u_char u, u_char v,
                 u_char width, u_char height, short tpage, int colour)
{
    SetPolyFT4(quad);
    if (colour == 0) {
        quad->r0 = 255;
        quad->g0 = 255;
        quad->b0 = 255;
    } else if (colour == 1) {
        if (texture16Bit) {
            quad->r0 = 240;
            quad->g0 = 192;
            quad->b0 = 48;
        } else {
            tpage = 36;
        }
    } else if (colour == 2) {
        quad->r0 = 0;
        quad->g0 = 192;
        quad->b0 = 0;
    }
    quad->tpage = tpage;
    quad->u0 = u;
    quad->v0 = v;
    quad->u1 = u + width;
    quad->v1 = v;
    quad->u2 = u;
    quad->v2 = v + height;
    quad->u3 = u + width;
    quad->v3 = v + height;
    x *= 2;
    y *= 2;
    quad->x0 = x;
    quad->y0 = y;
    quad->x1 = x + (u_char)width;
    quad->y1 = y;
    quad->x2 = x;
    quad->y2 = y + (u_char)height;
    quad->x3 = x + (u_char)width;
    quad->y3 = y + (u_char)height;
}




/* Small helpers. */

void DeSelectScreen(void *screen)
{
}
