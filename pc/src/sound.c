#include "sound.h"
#include "sound_samples.h"
#include <string.h>

#ifdef NETPLAY
extern FILE *_wfopen(char *name, char *mode);
#pragma intrinsic(strcpy, strcat)
#endif

enum {
    GLOBAL_ALLOC_ZERO_INIT = 0x40
};

extern char msgInitSound[];
extern char msgLoadSample[];
extern char msgLoadSampleWarning[];
extern char msgSampleNotLoaded[];
extern char sampleReadMode[];
extern char msgStartControlledSound[];
extern char msgKilledControlledSound[];
extern char msgTerminatedSound[];
extern short loop3;
#ifdef NETPLAY
extern short ambientLoop3Voice;
extern int ambientLoop3Sample;
#endif


void LoadSample(int sampleId, char *name)
{
#ifdef NETPLAY
    struct SampleLoadBuffers {
        char path[60];
        char message[100];
    } buffers;
#else
    char path[60];
    char message[100];
#endif

#ifdef NETPLAY
    wsprintf(buffers.message, "loadSample(%s)\n", name);
    OutputDebugString(buffers.message);
    strcpy(buffers.path, wobbleDir6);
    strcat(buffers.path, name);
    sampleBuffer = ReadSample(buffers.path);
#else
    wsprintf(message, msgLoadSample, name);
    OutputDebugString(message);
    strcpy(path, wobbleDir6);
    strcat(path, name);
    sampleBuffer = ReadSample(path);
#endif
    sampleHandle[sampleId] = SfxTransEffect((struct WavFile *)sampleBuffer, 0, 0);
    if (sampleHandle[sampleId] < 0) {
#ifdef NETPLAY
        wsprintf(buffers.message, "WARNING: during load sample: %s\n", name);
        wsprintf(buffers.message, msgSampleNotLoaded);
#else
        wsprintf(message, msgLoadSampleWarning, name);
        wsprintf(message, msgSampleNotLoaded);
#endif
    }
    GlobalFree(sampleBuffer);
}

#ifdef NETPLAY
char *ReadSample(char *name)
{
    char *data;
    unsigned long length;
    FILE *file;

    file = fopen(name, sampleReadMode);
    if (!file) {
        FileAccessError(name);
        return 0;
    }
    length = SampleFileLength(file);
    data = GlobalAlloc(GLOBAL_ALLOC_ZERO_INIT, length);
    if (!data) {
        fclose(file);
        return 0;
    }
    fread(data, 1, length, file);
    fclose(file);
    return data;
}
#else
char *ReadSample(char *name)
{
    char *buffer;
    unsigned long length;
    FILE *file;

    file = fopen(name, sampleReadMode);
    if (!file) {
        FileAccessError(name);
        return 0;
    }
    length = SampleFileLength(file);
    buffer = GlobalAlloc(GLOBAL_ALLOC_ZERO_INIT, length);
    if (!buffer) {
        fclose(file);
        return 0;
    }
    fread(buffer, 1, length, file);
    fclose(file);
    return buffer;
}
#endif

long SampleFileLength(FILE *file)
{
    long position, length;

    position = ftell(file);
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, position, SEEK_SET);
    return length;
}

void InitSpuSound(void)
{
    int i;

    OutputDebugString(msgInitSound);
    SfxInit(g_hwnd, sfxMode, 22050);
    SfxSetVolume(255);
    SfxOn();
    SfxSetCdVolume(255);
    OutputDebugString("sfx init complete...\n");
    SfxInitReverb(3, 0);
    SfxReverbOn();
    SfxSetReverbDepth(0);
    OutputDebugString("reverb init complete...\n");
    OutputDebugString("loading samples...\n");
    LoadSample(SAMPLE_SHIP_TO_SHIP, "shp2shp.wav");
    for (i = 1; i < SAMPLE_COUNT; i++)
        sampleHandle[i] = sampleHandle[SAMPLE_SHIP_TO_SHIP];
    LoadSample(SAMPLE_WEAPON_HIT, "weapnhit.wav");
}

#ifdef NETPLAY
/* Loads the remaining gameplay and menu samples. */
void InitSpuSamples(void)
{
    LoadSample(SAMPLE_ELECTRIC_BOLT, "elecbolt.wav");
    LoadSample(SAMPLE_ENGINE, "engines.wav");
    LoadSample(SAMPLE_WEAPON_WALL, "wepnwall.wav");
    LoadSample(SAMPLE_WALL_HIT, "hitwall2.wav");
    LoadSample(SAMPLE_MENU_MOVE, "menumove.wav");
    LoadSample(SAMPLE_MENU_SELECT, "menusel.wav");
    LoadSample(SAMPLE_RECHARGE, "recharge.wav");
    LoadSample(SAMPLE_MINE, "mine.wav");
    LoadSample(SAMPLE_MISSILES, "missiles.wav");
    LoadSample(SAMPLE_PICKUP, "pickup.wav");
    LoadSample(SAMPLE_SHIELD, "shield.wav");
    LoadSample(SAMPLE_AUTOPILOT, "autoplot.wav");
    LoadSample(SAMPLE_DROID, "droid.wav");
    LoadSample(SAMPLE_TURBULENCE, "turblnce.wav");
    LoadSample(SAMPLE_CROWD, "crowd.wav");
    LoadSample(SAMPLE_MINES, "mines.wav");
    LoadSample(SAMPLE_MISSILE, "missile.wav");
    LoadSample(SAMPLE_ROCKETS, "rockets.wav");
    LoadSample(SAMPLE_ELECTRO, "electro.wav");
    LoadSample(SAMPLE_COUNTDOWN_THREE, "three.wav");
    LoadSample(SAMPLE_COUNTDOWN_TWO, "two.wav");
    LoadSample(SAMPLE_COUNTDOWN_ONE, "one.wav");
    LoadSample(SAMPLE_COUNTDOWN_GO, "go.wav");
    LoadSample(SAMPLE_NEW_TURBO, "newturbo.wav");
    LoadSample(SAMPLE_THUNDER, "thunder.wav");
    LoadSample(SAMPLE_SHIP_STALL, "shpstall.wav");
    LoadSample(SAMPLE_LOCKED, "locked.wav");
    LoadSample(SAMPLE_QUAKE, "quake.wav");
    LoadSample(SAMPLE_PLASMA, "plasma.wav");
    LoadSample(SAMPLE_CHECKPOINT, "chkpoint.wav");
    LoadSample(SAMPLE_DITCH_WEAPON, "dtchwepn.wav");
    LoadSample(SAMPLE_ENERGY_PACK, "epak.wav");
    LoadSample(SAMPLE_WARNING, "warning.wav");
    LoadSample(SAMPLE_MENU_SLIDE, "menuslid.wav");
    LoadSample(SAMPLE_SCRAPE, "scrape.wav");
    LoadSample(SAMPLE_CONTINUE_ELIMINATED, "contelim.wav");
    LoadSample(SAMPLE_LOW_ENERGY, "nrg_low.wav");
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* Same LoadSample inlining and ReadSample calls; the import pointers get different registers. */
void InitSpuSamples(void)
{
    LoadSample(SAMPLE_ELECTRIC_BOLT, "elecbolt.wav");
    LoadSample(SAMPLE_ENGINE, "engines.wav");
    LoadSample(SAMPLE_WEAPON_WALL, "wepnwall.wav");
    LoadSample(SAMPLE_WALL_HIT, "hitwall2.wav");
    LoadSample(SAMPLE_MENU_MOVE, "menumove.wav");
    LoadSample(SAMPLE_MENU_SELECT, "menusel.wav");
    LoadSample(SAMPLE_RECHARGE, "recharge.wav");
    LoadSample(SAMPLE_MINE, "mine.wav");
    LoadSample(SAMPLE_MISSILES, "missiles.wav");
    LoadSample(SAMPLE_PICKUP, "pickup.wav");
    LoadSample(SAMPLE_SHIELD, "shield.wav");
    LoadSample(SAMPLE_AUTOPILOT, "autoplot.wav");
    LoadSample(SAMPLE_DROID, "droid.wav");
    LoadSample(SAMPLE_TURBULENCE, "turblnce.wav");
    LoadSample(SAMPLE_CROWD, "crowd.wav");
    LoadSample(SAMPLE_MINES, "mines.wav");
    LoadSample(SAMPLE_MISSILE, "missile.wav");
    LoadSample(SAMPLE_ROCKETS, "rockets.wav");
    LoadSample(SAMPLE_ELECTRO, "electro.wav");
    LoadSample(SAMPLE_COUNTDOWN_THREE, "three.wav");
    LoadSample(SAMPLE_COUNTDOWN_TWO, "two.wav");
    LoadSample(SAMPLE_COUNTDOWN_ONE, "one.wav");
    LoadSample(SAMPLE_COUNTDOWN_GO, "go.wav");
    LoadSample(SAMPLE_NEW_TURBO, "newturbo.wav");
    LoadSample(SAMPLE_THUNDER, "thunder.wav");
    LoadSample(SAMPLE_SHIP_STALL, "shpstall.wav");
    LoadSample(SAMPLE_LOCKED, "locked.wav");
    LoadSample(SAMPLE_QUAKE, "quake.wav");
    LoadSample(SAMPLE_PLASMA, "plasma.wav");
    LoadSample(SAMPLE_CHECKPOINT, "chkpoint.wav");
    LoadSample(SAMPLE_DITCH_WEAPON, "dtchwepn.wav");
    LoadSample(SAMPLE_ENERGY_PACK, "epak.wav");
    LoadSample(SAMPLE_WARNING, "warning.wav");
    LoadSample(SAMPLE_MENU_SLIDE, "menuslid.wav");
    LoadSample(SAMPLE_SCRAPE, "scrape.wav");
    LoadSample(SAMPLE_CONTINUE_ELIMINATED, "contelim.wav");
    LoadSample(SAMPLE_LOW_ENERGY, "nrg_low.wav");
}
#endif
#endif


void StartControlledSound(void)
{
#ifdef NETPLAY
    if (soundEnabled != 0)
        return;
    else
        soundEnabled = 1;
    remoteEngineVoice2 = SfxPlayId(sampleHandle[SAMPLE_ENGINE], 0, 15, 15, 256, 11);
    remoteEngineVoice1 = SfxPlayId(sampleHandle[SAMPLE_ENGINE], 0, 15, 15, 255, 11);
    turbulenceVoice = SfxPlayId(sampleHandle[SAMPLE_TURBULENCE], 0, 15, 15, 640, 11);
    playerEngineVoice = SfxPlayId(sampleHandle[SAMPLE_ENGINE], 0, 15, 15, 255, 11);
    previousSoundCamera[0] = 0;
    previousSoundCamera[1] = 0;
    previousSoundCamera[2] = 0;
    OutputDebugString(msgStartControlledSound);
#else
    if (soundEnabled == 0) {
        soundEnabled = 1;
        remoteEngineVoice2 = SfxPlayId(sampleHandle[SAMPLE_ENGINE], 0, 15, 15, 256, 11);
        remoteEngineVoice1 = SfxPlayId(sampleHandle[SAMPLE_ENGINE], 0, 15, 15, 255, 11);
        turbulenceVoice = SfxPlayId(sampleHandle[SAMPLE_TURBULENCE], 0, 15, 15, 640, 11);
        playerEngineVoice = SfxPlayId(sampleHandle[SAMPLE_ENGINE], 0, 15, 15, 255, 11);
        previousSoundCamera[0] = 0;
        previousSoundCamera[1] = 0;
        previousSoundCamera[2] = 0;
        OutputDebugString(msgStartControlledSound);
    }
#endif
}

/* PC sound leaves the PS1 note-update entry point empty. */
void UpdateNotes(int paused) {}

/* PC sound does not implement the PS1 single-note kill entry point. */
void KillNote(void) {}
/* PC sound does not implement the PS1 note kill entry point. */
void KillNotes(void) {}
/* PC sound does not implement the PS1 fade-note kill entry point. */
void KillFadeNotes(void) {}

#ifdef NETPLAY
void PlayNote(int sampleId, short volume, int lrPan,
              int fbPan, int pitch)
{
    short noteId;

    /* The original writes these remaps, then passes sampleHandle[sampleId]. */
    noteId = 0;
    if (sampleId == 0x33)
        noteId = 0x17;
    if (sampleId == 0x36)
        noteId = 0x16;
    if (sampleId == 0x10)
        noteId = 0x15;
    SfxPlay(sampleHandle[sampleId], volume, lrPan, fbPan, pitch, 2);
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* Argument and register scheduling still differ. */
void PlayNote(int sampleId, short volume, int lrPan,
              int fbPan, int pitch)
{
    SfxPlay(sampleHandle[sampleId], volume & 0xFFFF,
            lrPan & 0xFF, fbPan & 0xFF, pitch, 2);
}
#endif
#endif

int KillControlledNote(short voice)
{
    SfxStop(voice);
    return -1;
}

#ifdef NETPLAY
short PlayDirectionalNote(short sampleId, short pitch, short duration, VECTOR pos)
{
    /* Local names and declaration order preserve the NET /Od frame layout. */
    float xDistance;
    float cameraDistanceY, cameraDistanceZ;
    short direction;
    int combinedVolume;
    int leftVol, panPosition, sourceDistance;
    int rightChannelVolume;

    xDistance = (pos.vx + cameraPtr->camPos->relative.t[0]) >> 1;
    cameraDistanceY = (pos.vy + cameraPtr->camPos->relative.t[1]) >> 1;
    cameraDistanceZ = (pos.vz + cameraPtr->camPos->relative.t[2]) >> 1;
    sourceDistance = GetMagnitude(xDistance, cameraDistanceY, cameraDistanceZ);
    leftVol = 16384 - sourceDistance;
    if (leftVol < 0) leftVol = 0;
    rightChannelVolume = 16384 - sourceDistance;
    if (rightChannelVolume < 0) rightChannelVolume = 0;
    direction = -ratan2(xDistance, cameraDistanceZ) + cameraPtr->hdg;
    direction = ang(direction);
    /* Preserve the NET build's two-step reflection at each pan limit. */
    if (direction > 1024) direction = 1024 - (direction - 1024);
    if (direction < -1024) direction = -1024 - (direction + 1024);
    leftVol += (leftVol * direction) >> 11;
    rightChannelVolume -= (rightChannelVolume * direction) >> 11;
    leftVol = (leftVol << 13) >> 18;
    rightChannelVolume = (rightChannelVolume << 13) >> 18;
    combinedVolume = (rightChannelVolume + leftVol) >> 1;
    if (combinedVolume > 255) combinedVolume = 255;
    panPosition = 15;
    if (leftVol > 0 && rightChannelVolume > 0) {
        leftVol = (leftVol * 0x1e0000) / (rightChannelVolume + leftVol);
        panPosition = leftVol >> 16;
        panPosition = (panPosition - 15) * 2 + 15;
        if (panPosition < 0) panPosition = 0;
        if (panPosition > 30) panPosition = 30;
    }
    panPosition = 30 - panPosition;
    SfxPlay(sampleHandle[sampleId], combinedVolume, panPosition, 15, pitch + 256, 2);
    return 0;
}
#else
short PlayDirectionalNote(short sampleId, short pitch, short duration, VECTOR pos)
{
    float distanceX, distanceY, distanceZ;
    int magnitude;
    int leftVol, rightVol, sum, volume, pan;
    short angle, relAngle;

    distanceX = (pos.vx + cameraPtr->camPos->relative.t[0]) >> 1;
    distanceY = (pos.vy + cameraPtr->camPos->relative.t[1]) >> 1;
    distanceZ = (pos.vz + cameraPtr->camPos->relative.t[2]) >> 1;
    magnitude = GetMagnitude(distanceX, distanceY, distanceZ);
    leftVol = 16384 - magnitude;
    if (leftVol < 0) leftVol = 0;
    rightVol = 16384 - magnitude;
    if (rightVol < 0) rightVol = 0;
    angle = ang((short)(cameraPtr->hdg - ratan2(distanceX, distanceZ)));
    relAngle = angle;
    if (angle > 1024) relAngle = 2048 - angle;
    if (relAngle < -1024) relAngle = -2048 - relAngle;
    leftVol += (leftVol * relAngle) >> 11;
    rightVol -= (rightVol * relAngle) >> 11;
    leftVol = (leftVol << 13) >> 18;
    rightVol = (rightVol << 13) >> 18;
    sum = leftVol + rightVol;
    volume = sum >> 1;
    if (volume > 255) volume = 255;
    pan = 15;
    if (leftVol > 0 && rightVol > 0) {
        leftVol = (leftVol * 0x1e0000) / sum;
        pan = leftVol >> 16;
        pan = pan * 2 - 15;
        if (pan < 0) pan = 0;
        if (pan > 30) pan = 30;
    }
    pan = 30 - pan;
    SfxPlay(sampleHandle[sampleId], volume, pan, 15, pitch + 256, 2);
    return 0;
}
#endif

void SetPitch(short voice, int pitch)
{
#ifdef NETPLAY
    if (soundEnabled == 0)
        return;
    SfxEffectPitch(voice, pitch);
#else
    if (soundEnabled != 0)
        SfxEffectPitch(voice, pitch);
#endif
}

void SetNoteVolume(short voice, unsigned char volume)
{
#ifdef NETPLAY
    if (soundEnabled == 0)
        return;
    SfxEffectVol(voice, volume);
#else
    if (soundEnabled != 0)
        SfxEffectVol(voice, volume);
#endif
}

#ifdef NETPLAY
void AdjustDirectionalNote(short voice, short unused1, short unused2, VECTOR pos)
{
    /* Local names and declaration order preserve the NET /Od frame layout. */
    float xDistance;
    float cameraDistanceY, cameraDistanceZ;
    short direction;
    int combinedVolume, volumeReduction;
    int leftVol;
    int rightChannelVolume;
    int panPosition, sourceDistance;

    xDistance = (pos.vx + cameraPtr->camPos->relative.t[0]) >> 1;
    cameraDistanceY = (pos.vy + cameraPtr->camPos->relative.t[1]) >> 1;
    cameraDistanceZ = (pos.vz + cameraPtr->camPos->relative.t[2]) >> 1;
    sourceDistance = GetMagnitude(xDistance, cameraDistanceY, cameraDistanceZ);
    leftVol = 12287 - sourceDistance;
    if (leftVol < 0) leftVol = 0;
    rightChannelVolume = 12287 - sourceDistance;
    if (rightChannelVolume < 0) rightChannelVolume = 0;
    direction = -ratan2(xDistance, cameraDistanceZ) + cameraPtr->hdg;
    direction = ang(direction);
    /* Preserve the NET build's two-step reflection at each pan limit. */
    if (direction > 1024) direction = 1024 - (direction - 1024);
    if (direction < -1024) direction = -1024 - (direction + 1024);
    leftVol += (leftVol * direction) >> 11;
    rightChannelVolume -= (rightChannelVolume * direction) >> 11;
    leftVol = (leftVol << 13) >> 18;
    rightChannelVolume = (rightChannelVolume << 13) >> 18;
    combinedVolume = (leftVol + rightChannelVolume) >> 1;
    if (combinedVolume > 255) combinedVolume = 255;
    panPosition = 15;
    if (leftVol > 0 && rightChannelVolume > 0) {
        if (leftVol > 255) leftVol = 255;
        if (rightChannelVolume > 255) rightChannelVolume = 255;
        panPosition = (leftVol << 8) / (leftVol + rightChannelVolume);
        panPosition = ((panPosition << 5) >> 8) - 1;
        if (panPosition < 0) panPosition = 0;
        if (panPosition > 30) panPosition = 30;
    }
    if (panPosition > 15 && panPosition < 25) panPosition = 25;
    if (panPosition < 15 && panPosition > 5) panPosition = 5;
    panPosition = 30 - panPosition;
    SfxEffectLRPan(voice, panPosition);
    if (shipIndex[ownShip].attr & VIEW_IN)
        volumeReduction = combinedVolume >> 1;
    else
        volumeReduction = combinedVolume >> 2;
    SetNoteVolume(voice, combinedVolume - volumeReduction);
}
#else
void AdjustDirectionalNote(short voice, short unused1, short unused2, VECTOR pos)
{
    float distanceX, distanceY, distanceZ;
    int magnitude;
    int leftVol, rightVol, volume, pan, reduced;
    short angle, relAngle;

    distanceX = (pos.vx + cameraPtr->camPos->relative.t[0]) >> 1;
    distanceY = (pos.vy + cameraPtr->camPos->relative.t[1]) >> 1;
    distanceZ = (pos.vz + cameraPtr->camPos->relative.t[2]) >> 1;
    magnitude = GetMagnitude(distanceX, distanceY, distanceZ);
    leftVol = 12287 - magnitude;
    if (leftVol < 0) leftVol = 0;
    rightVol = 12287 - magnitude;
    if (rightVol < 0) rightVol = 0;
    angle = ang((short)(cameraPtr->hdg - ratan2(distanceX, distanceZ)));
    relAngle = angle;
    if (angle > 1024) relAngle = 2048 - angle;
    if (relAngle < -1024) relAngle = -2048 - relAngle;
    leftVol += (leftVol * relAngle) >> 11;
    rightVol -= (rightVol * relAngle) >> 11;
    leftVol = (leftVol << 13) >> 18;
    rightVol = (rightVol << 13) >> 18;
    volume = (leftVol + rightVol) >> 1;
    if (volume > 255) volume = 255;
    pan = 15;
    if (leftVol > 0 && rightVol > 0) {
        if (leftVol > 255) leftVol = 255;
        if (rightVol > 255) rightVol = 255;
        pan = (leftVol << 8) / (leftVol + rightVol);
        pan = ((pan << 5) >> 8) - 1;
        if (pan < 0) pan = 0;
        if (pan > 30) pan = 30;
    }
    if (pan > 15 && pan < 25) pan = 25;
    if (pan < 15 && pan > 5) pan = 5;
    pan = 30 - pan;
    SfxEffectLRPan(voice, pan);
    if (shipIndex[ownShip].attr & VIEW_IN)
        reduced = volume >> 1;
    else
        reduced = volume >> 2;
    SetNoteVolume(voice, volume - reduced);
}
#endif

#ifdef NETPLAY
/* NET retains the unused prevEngine and prevPivot locals in its 0x74-byte frame. */
void AdjustDopplerEngine(int shipNo, short channel, int unused)
{
    int engineLevel;
    int nextEngine;
    int leftLevel;
    int panVolume;
    signed char avgVolume;
    short gain;
    float nextZ;
    float nextY;
    float nextOffsetX;
    float offsetZ;
    int nextPivotZ;
    int prevEngine;
    int newPivotY;
    int nextPivotX;
    int basePivotZ;
    int prevPivot;
    int shipPivotY;
    short bearing;
    float spanY;
    short newPitch;
    float relX;
    int rightVol;
    int shipPivotX;

    gain = 0;
    newPitch = shipIndex[shipNo].speed >> 8;
    if (CameraUpdate != oldcameraupdate) {
        previousSoundCamera[0] = cameraPtr->camPos->relative.t[0];
        previousSoundCamera[1] = cameraPtr->camPos->relative.t[1];
        previousSoundCamera[2] = cameraPtr->camPos->relative.t[2];
        oldcameraupdate = CameraUpdate;
        return;
    }
    shipPivotX = shipIndex[shipNo].ppivot.vx;
    shipPivotY = shipIndex[shipNo].ppivot.vy;
    basePivotZ = shipIndex[shipNo].ppivot.vz;
    relX = (previousSoundCamera[0] + shipPivotX) >> 2;
    spanY = (previousSoundCamera[1] + shipPivotY) >> 2;
    offsetZ = (previousSoundCamera[2] + basePivotZ) >> 2;
    engineLevel = GetMagnitude(relX, spanY, offsetZ);
    nextPivotX = (shipIndex[shipNo].vpivot.vx >> 6) + shipIndex[shipNo].ppivot.vx;
    newPivotY = (shipIndex[shipNo].vpivot.vy >> 6) + shipIndex[shipNo].ppivot.vy;
    nextPivotZ = (shipIndex[shipNo].vpivot.vz >> 6) + shipIndex[shipNo].ppivot.vz;
    nextOffsetX = (cameraPtr->camPos->relative.t[0] + nextPivotX) >> 2;
    nextY = (cameraPtr->camPos->relative.t[1] + newPivotY) >> 2;
    nextZ = (cameraPtr->camPos->relative.t[2] + nextPivotZ) >> 2;
    nextEngine = GetMagnitude(nextOffsetX, nextY, nextZ);
    nextEngine -= engineLevel;
    newPitch -= nextEngine >> 2;
    leftLevel = 3000 - engineLevel;
    if (leftLevel < 0)
        leftLevel = 0;
    rightVol = 3000 - engineLevel;
    if (rightVol < 0)
        rightVol = 0;
    bearing = -ratan2(relX, offsetZ) + cameraPtr->hdg;
    bearing = ang(bearing);
    if (bearing > 1024)
        bearing = 1024 - (bearing - 1024);
    if (bearing < -1024)
        bearing = -1024 - (bearing + 1024);
    leftLevel += (leftLevel * bearing) >> 11;
    rightVol -= (rightVol * bearing) >> 11;
    leftLevel = (leftLevel << 13) >> 18;
    rightVol = (rightVol << 13) >> 18;
    panVolume = 15;
    if (leftLevel > 0 && rightVol > 0) {
        if (leftLevel > 255)
            leftLevel = 255;
        if (rightVol > 255)
            rightVol = 255;
        panVolume = (leftLevel << 8) / (rightVol + leftLevel);
        panVolume = ((panVolume << 5) >> 8) - 1;
        if (panVolume < 0)
            panVolume = 0;
        if (panVolume > 30)
            panVolume = 30;
    }
    avgVolume = (rightVol + leftLevel) >> 1;
    panVolume = 30 - panVolume;
    SfxEffectLRPan(channel, panVolume);
    gain += avgVolume;
    gain += gain * 2;
    if (gain > 255)
        gain = 255;
    gain -= gain >> 2;
    SetPitch(channel, newPitch + 225);
    SetNoteVolume(channel, gain);
}

/* Voice-volume and camera-cache scheduling still differ; missing push ebx. */
void AdjustRemoteEngineSound(int ship1, int ship2, int extra2, int extra3)
{
    if ((char)linkRaceDone == 1) return;
    if (ship1 > -1)
        AdjustDopplerEngine(ship1, remoteEngineVoice1, extra2);
    else
        SetNoteVolume(remoteEngineVoice1, 0);
    if (ship2 > -1)
        AdjustDopplerEngine(ship2, remoteEngineVoice2, extra3);
    else
        SetNoteVolume(remoteEngineVoice2, 0);
    previousSoundCamera[0] = cameraPtr->camPos->relative.t[0];
    previousSoundCamera[1] = cameraPtr->camPos->relative.t[1];
    previousSoundCamera[2] = cameraPtr->camPos->relative.t[2];
}
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* Register allocation and camera-position reloads still differ. */
void AdjustDopplerEngine(int shipNo, short channel, int unused)
{
    float distanceX, distanceY, distanceZ;
    float nextX, nextY, nextZ;
    ShipData *ship;
    int leftVol, rightVol, engine, pan;
    short volume, angle, pitch;

    pitch = shipIndex[shipNo].speed >> 8;
    if (oldcameraupdate != CameraUpdate) {
        TrackCamera *cam = cameraPtr;
        Skeleton *camPos = cam->camPos;
        previousSoundCamera[0] = camPos->relative.t[0];
        oldcameraupdate = CameraUpdate;
        camPos = cameraPtr->camPos;
        previousSoundCamera[1] = camPos->relative.t[1];
        previousSoundCamera[2] = camPos->relative.t[2];
        return;
    }
    ship = &shipIndex[shipNo];
    distanceX = (ship->ppivot.vx + previousSoundCamera[0]) >> 2;
    distanceY = (ship->ppivot.vy + previousSoundCamera[1]) >> 2;
    distanceZ = (ship->ppivot.vz + previousSoundCamera[2]) >> 2;
    engine = GetMagnitude(distanceX, distanceY, distanceZ);
    nextX = ((shipIndex[shipNo].vpivot.vx >> 6) + cameraPtr->camPos->relative.t[0] + shipIndex[shipNo].ppivot.vx) >> 2;
    nextY = ((shipIndex[shipNo].vpivot.vy >> 6) + cameraPtr->camPos->relative.t[1] + shipIndex[shipNo].ppivot.vy) >> 2;
    nextZ = ((shipIndex[shipNo].vpivot.vz >> 6) + shipIndex[shipNo].ppivot.vz + cameraPtr->camPos->relative.t[2]) >> 2;
    pitch -= (GetMagnitude(nextX, nextY, nextZ) - engine) >> 2;
    leftVol = 3000 - engine;
    if (leftVol < 0) leftVol = 0;
    rightVol = 3000 - engine;
    if (rightVol < 0) rightVol = 0;
    angle = ang((short)(cameraPtr->hdg - ratan2(distanceX, distanceZ)));
    if (angle > 1024) angle = 2048 - angle;
    if (angle < -1024) angle = -2048 - angle;
    leftVol += (leftVol * angle) >> 11;
    rightVol -= (rightVol * angle) >> 11;
    leftVol = (leftVol << 13) >> 18;
    rightVol = (rightVol << 13) >> 18;
    pan = 15;
    if (leftVol > 0 && rightVol > 0) {
        if (leftVol > 255) leftVol = 255;
        if (rightVol > 255) rightVol = 255;
        pan = (leftVol << 8) / (leftVol + rightVol);
        pan = ((pan << 5) >> 8) - 1;
        if (pan < 0) pan = 0;
        if (pan > 30) pan = 30;
    }
    pan = 30 - pan;
    SfxEffectLRPan(channel, pan);
    volume = (signed char)((leftVol + rightVol) >> 1);
    volume *= 3;
    if (volume > 255) volume = 255;
    volume -= volume >> 2;
    SetPitch(channel, pitch + 225);
    SetNoteVolume(channel, volume);
}

/* Voice-volume and camera-cache scheduling still differ; missing push ebx. */
void AdjustRemoteEngineSound(int ship1, int ship2, int extra2, int extra3)
{
    if (linkRaceDone == 1) return;
    if (ship1 > -1)
        AdjustDopplerEngine(ship1, remoteEngineVoice1, extra2);
    else
        SetNoteVolume(remoteEngineVoice1, 0);
    if (ship2 > -1)
        AdjustDopplerEngine(ship2, remoteEngineVoice2, extra3);
    else
        SetNoteVolume(remoteEngineVoice2, 0);
    previousSoundCamera[0] = cameraPtr->camPos->relative.t[0];
    previousSoundCamera[1] = cameraPtr->camPos->relative.t[1];
    previousSoundCamera[2] = cameraPtr->camPos->relative.t[2];
}
#endif
#endif

#ifdef NETPLAY
/* Updates engine, brake and turbulence playback for the player's ship. */
#pragma optimize("a", off)
#pragma optimize("w", on)
void AdjustEngineSound(ShipData *ship, TrackCamera *camera)
{
    int panPos;
    unsigned char boostLevel;
    short volumeOut;
    short brakeLevel;
    short pitch;

    if ((char)linkRaceDone == 1)
        return;
    if (scrapeVol > 0)
        scrapeVol -= 10;
    if (scrapeVol > 0) {
        ScrapeSound(1);
        SetNoteVolume(scrapeVoice, scrapeVol);
    } else {
        ScrapeSound(0);
    }
    if ((shipIndex[ownShip].combatAttr & DESTROYED) || (char)linkRaceDone == 1) {
        SetNoteVolume(playerEngineVoice, 0);
        SetNoteVolume(turbulenceVoice, 0);
    } else {
        if (camera->section->snow == 1) {
            SfxSetReverbDepth(64);
            undercover = 1;
        } else {
            SfxSetReverbDepth(0);
            undercover = 0;
        }
        volumeOut = ((int)ship->speed << 12) >> 22;
        if (volumeOut > 32)
            volumeOut = 32;
        pitch = ship->speed >> 8;
        if (ship->attr & VIEW_IN) {
            boostLevel = 128;
            SfxEffectLRPan(playerEngineVoice, 15);
            SfxEffectFBPan(playerEngineVoice, 15);
            SfxEffectFBPan(turbulenceVoice, 15);
            volumeOut += boostLevel;
            if (volumeOut > 255)
                volumeOut = 255;
            volumeOut -= volumeOut >> 2;
            SetPitch(playerEngineVoice, pitch + 255);
            SetNoteVolume(playerEngineVoice, volumeOut);
        } else {
            SfxEffectFBPan(playerEngineVoice, 22);
            SfxEffectFBPan(turbulenceVoice, 22);
            AdjustDopplerEngine(ownShip, playerEngineVoice, 0);
        }
        volumeOut = (ship->r_brake_rot + ship->l_brake_rot) >> 2;
        if (volumeOut > 128)
            volumeOut = 128;
        brakeLevel = ship->speed >> 8;
        if (brakeLevel > 128)
            brakeLevel = 128;
        volumeOut = (brakeLevel * volumeOut) >> 7;
        volumeOut = brakeLevel + volumeOut;
        if (volumeOut > 255)
            volumeOut = 255;
        volumeOut -= volumeOut >> 2;
        SetNoteVolume(turbulenceVoice, volumeOut);
        volumeOut = ship->r_brake_rot >> 4;
        if (!(ship->attr & VIEW_IN))
            volumeOut = volumeOut >> 1;
        if (volumeOut > 15)
            volumeOut = 15;
        panPos = 15 - volumeOut;
        volumeOut = ship->l_brake_rot >> 4;
        if (!(ship->attr & VIEW_IN))
            volumeOut = volumeOut >> 2;
        if (volumeOut > 15)
            volumeOut = 15;
        panPos += volumeOut;
        SfxEffectLRPan(turbulenceVoice, panPos);
    }
}
#pragma optimize("w", off)
#pragma optimize("a", on)
#endif
#ifdef EQUIV
#ifndef NETPLAY
/* A few control-flow edges and instructions remain different. */
#pragma optimize("a", off)
#pragma optimize("w", on)
void AdjustEngineSound(ShipData *ship, TrackCamera *camera)
{
    short volume, pitch, brake;
    int pan;
    if (linkRaceDone == 1) return;
    if (scrapeVol > 0) scrapeVol -= 10;
    if (scrapeVol > 0) {
        if (scrapeSoundPlaying == 0) {
            scrapeVoice = SfxPlayId(sampleHandle[SAMPLE_SCRAPE], 0, 15, 15, 192, 3);
            scrapeSoundPlaying = 1;
        }
        scrapeVol = scrapeVol;
        SetNoteVolume(scrapeVoice, scrapeVol);
    } else {
        if (scrapeSoundPlaying != 0) {
            SfxStop(scrapeVoice);
            scrapeSoundPlaying = 0;
        }
    }
    if (!(shipIndex[ownShip].combatAttr & DESTROYED) && linkRaceDone != 1) {
        if (camera->section->snow == 1) {
            SfxSetReverbDepth(64);
            undercover = 1;
        } else {
            SfxSetReverbDepth(0);
            undercover = 0;
        }
        volume = ((int)ship->speed << 12) >> 22;
        if (volume > 32) volume = 32;
        pitch = ship->speed >> 8;
        if (ship->attr & VIEW_IN) {
            SfxEffectLRPan(playerEngineVoice, 15);
            SfxEffectFBPan(playerEngineVoice, 15);
            volume += 128;
            SfxEffectFBPan(turbulenceVoice, 15);
            if (volume > 255) volume = 255;
            volume -= volume >> 2;
            SetPitch(playerEngineVoice, pitch + 255);
            SetNoteVolume(playerEngineVoice, volume);
        } else {
            SfxEffectFBPan(playerEngineVoice, 22);
            SfxEffectFBPan(turbulenceVoice, 22);
            AdjustDopplerEngine(ownShip, playerEngineVoice, 0);
        }
        volume = (ship->r_brake_rot + ship->l_brake_rot) >> 2;
        if (volume > 128) volume = 128;
        brake = ship->speed >> 8;
        if (brake > 128) brake = 128;
        volume = ((brake * volume) >> 7) + brake;
        if (volume > 255) volume = 255;
        volume -= volume >> 2;
        SetNoteVolume(turbulenceVoice, volume);
        volume = ship->r_brake_rot >> 4;
        if (!(ship->attr & VIEW_IN)) volume >>= 1;
        if (volume > 15) volume = 15;
        pan = 15 - volume;
        volume = ship->l_brake_rot >> 4;
        if (!(ship->attr & VIEW_IN)) volume >>= 2;
        if (volume > 15) volume = 15;
        pan += volume;
        SfxEffectLRPan(turbulenceVoice, pan);
    } else {
        SetNoteVolume(playerEngineVoice, 0);
        SetNoteVolume(turbulenceVoice, 0);
    }
}
#pragma optimize("w", off)
#pragma optimize("a", on)
#endif
#endif

int InitCD(CdlLOC *locs, int unused)
{
    unsigned char mode[8];

    CdOpen();
    CdSetDebug(0);
    mode[0] = 5;
    CdControlB(14, mode, 0);
    cdMusicCount = CdCountTracks(1, locs);
    return cdMusicCount;
}

#ifdef NETPLAY
int InitCrowdSound(Object **prm, short prmCount, Object **destination, int count)
{
    Object *currentObject;
    int i;

    for (i = 0; i < prmCount; i++) {
        currentObject = prm[i];
        while (currentObject) {
            if (strncmp(currentObject->name, msgCrowdPrefix, 4) == 0) {
                destination[count] = currentObject;
                count++;
            }
            currentObject = currentObject->next;
        }
    }
    return count;
}
#else
int InitCrowdSound(Object **prm, short prmCount, Object **destination, int count)
{
    Object *obj;
    int i;

    for (i = 0; i < prmCount; i++) {
        obj = prm[i];
        while (obj) {
            if (strncmp(obj->name, msgCrowdPrefix, 4) == 0) {
                destination[count] = obj;
                count++;
            }
            obj = obj->next;
        }
    }
    return count;
}
#endif

/* PC sound leaves crowd-object positioning to the sound backend. */
void AdjustCrowdSound(Object **objects, int count)
{
#ifdef NETPLAY
    int closestDistance, secondDistance;
    int closestIndex, secondIndex;
    int i, distance;
    float distanceX, distanceY, distanceZ;
    VECTOR position;

    return;
    if (linkRaceDone == 1)
        return;
    closestDistance = 10000;
    secondDistance = 10000;
    closestIndex = -1;
    secondIndex = -1;
    i = 0;
    do {
        distanceX = (objects[i]->skeleton->relative.t[0] + cameraPtr->camPos->relative.t[0]) >> 3;
        distanceY = (objects[i]->skeleton->relative.t[1] + cameraPtr->camPos->relative.t[1]) >> 3;
        distanceZ = (objects[i]->skeleton->relative.t[2] + cameraPtr->camPos->relative.t[2]) >> 3;
        distance = GetMagnitude(distanceX, distanceY, distanceZ);
        if (closestDistance > distance) {
            if (closestIndex != -1) {
                secondDistance = closestDistance;
                secondIndex = closestIndex;
            }
            closestDistance = distance;
            closestIndex = i;
        } else if (secondDistance > distance) {
            secondDistance = distance;
            secondIndex = i;
        }
        i++;
    } while (i < count);

    if (closestDistance < 0x2000) {
        position.vx = objects[closestIndex]->skeleton->relative.t[0];
        position.vy = objects[closestIndex]->skeleton->relative.t[1];
        position.vz = objects[closestIndex]->skeleton->relative.t[2];
        SetAmbientLoop3(1);
        AdjustDirectionalNote(ambientLoop3Voice, 0, -1, position);
    } else {
        SetAmbientLoop3(0);
    }
    if (secondDistance < 0x2000) {
        position.vx = objects[secondIndex]->skeleton->relative.t[0];
        position.vy = objects[secondIndex]->skeleton->relative.t[1];
        position.vz = objects[secondIndex]->skeleton->relative.t[2];
        CrowdSound(1);
        AdjustDirectionalNote(crowdVoice, 0, -1, position);
    } else {
        CrowdSound(0);
    }
#else
    (void)objects;
    (void)count;
#endif
}


void KillControlledSound(void)
{
#ifdef NETPLAY
    if (soundEnabled == 0)
        return;
    SetNoteVolume(playerEngineVoice, 0);
    SetNoteVolume(remoteEngineVoice1, 0);
    SetNoteVolume(remoteEngineVoice2, 0);
    SetNoteVolume(turbulenceVoice, 0);
    OutputDebugString(msgKilledControlledSound);
#else
    if (soundEnabled != 0) {
        SetNoteVolume(playerEngineVoice, 0);
        SetNoteVolume(remoteEngineVoice1, 0);
        SetNoteVolume(remoteEngineVoice2, 0);
        SetNoteVolume(turbulenceVoice, 0);
        OutputDebugString(msgKilledControlledSound);
    }
#endif
}

void MuteControlledSound(void)
{
#ifdef NETPLAY
    if (soundEnabled == 0)
        return;
    SetNoteVolume(playerEngineVoice, 0);
    SetNoteVolume(remoteEngineVoice1, 0);
    SetNoteVolume(remoteEngineVoice2, 0);
    SetNoteVolume(turbulenceVoice, 0);
#else
    if (soundEnabled != 0) {
        SetNoteVolume(playerEngineVoice, 0);
        SetNoteVolume(remoteEngineVoice1, 0);
        SetNoteVolume(remoteEngineVoice2, 0);
        SetNoteVolume(turbulenceVoice, 0);
    }
#endif
}

void CdMusicPlay(int track, int unused)
{
    CdPlayTrack(g_hwnd, track);
}

void CdMusicStop(void)
{
    CdPause();
    CdControlB(9, 0, 0);
    OutputDebugString(msgCdMusicStop);
}

#ifdef NETPLAY
void PlayPitchSequence(void)
{
    if (testPitchStep > 128)
        return;
    switch (testPitchStep) {
    case 0:
        PlayNote(SAMPLE_WARNING, 255, 15, 15, 128);
        break;
    case 8:
        PlayNote(SAMPLE_WARNING, 255, 15, 15, 107);
        break;
    case 16:
        PlayNote(SAMPLE_WARNING, 255, 15, 15, 86);
        break;
    case 32:
        PlayNote(SAMPLE_WARNING, 255, 15, 15, 86);
        break;
    case 48:
        PlayNote(SAMPLE_WARNING, 255, 15, 15, 86);
        break;
    case 56:
        PlayNote(SAMPLE_WARNING, 255, 15, 15, 96);
        break;
    case 64:
        PlayNote(SAMPLE_WARNING, 255, 15, 15, 107);
        break;
    case 72:
        PlayNote(SAMPLE_WARNING, 255, 15, 15, 115);
        break;
    case 80:
        PlayNote(SAMPLE_WARNING, 255, 15, 15, 128);
        break;
    case 96:
        PlayNote(SAMPLE_WARNING, 255, 15, 15, 128);
        break;
    case 112:
        PlayNote(SAMPLE_WARNING, 255, 15, 15, 128);
        break;
    case 128:
        PlayNote(SAMPLE_WARNING, 255, 15, 15, 107);
        break;
    }
    testPitchStep += 2;
}
#else
void PlayPitchSequence(void)
{
    if (testPitchStep <= 128) {
        switch (testPitchStep) {
        case 0:
            SfxPlay(sampleHandle[SAMPLE_WARNING], 255, 15, 15, 128, 2);
            break;
        case 8:
            SfxPlay(sampleHandle[SAMPLE_WARNING], 255, 15, 15, 107, 2);
            break;
        case 16:
            SfxPlay(sampleHandle[SAMPLE_WARNING], 255, 15, 15, 86, 2);
            break;
        case 32:
            SfxPlay(sampleHandle[SAMPLE_WARNING], 255, 15, 15, 86, 2);
            break;
        case 48:
            SfxPlay(sampleHandle[SAMPLE_WARNING], 255, 15, 15, 86, 2);
            break;
        case 56:
            SfxPlay(sampleHandle[SAMPLE_WARNING], 255, 15, 15, 96, 2);
            break;
        case 64:
            SfxPlay(sampleHandle[SAMPLE_WARNING], 255, 15, 15, 107, 2);
            break;
        case 72:
            SfxPlay(sampleHandle[SAMPLE_WARNING], 255, 15, 15, 115, 2);
            break;
        case 80:
            SfxPlay(sampleHandle[SAMPLE_WARNING], 255, 15, 15, 128, 2);
            break;
        case 96:
            SfxPlay(sampleHandle[SAMPLE_WARNING], 255, 15, 15, 128, 2);
            break;
        case 112:
            SfxPlay(sampleHandle[SAMPLE_WARNING], 255, 15, 15, 128, 2);
            break;
        case 128:
            SfxPlay(sampleHandle[SAMPLE_WARNING], 255, 15, 15, 107, 2);
            break;
        }
        testPitchStep += 2;
    }
}
#endif

void TerminateSound(void)
{
#ifdef NETPLAY
    if (soundEnabled != 0)
        soundEnabled = 0;
    else
        return;
    SfxStop(playerEngineVoice);
    SfxStop(remoteEngineVoice1);
    SfxStop(remoteEngineVoice2);
    SfxStop(turbulenceVoice);
    OutputDebugString(msgTerminatedSound);
#else
    if (soundEnabled != 0) {
        soundEnabled = 0;
        SfxStop(playerEngineVoice);
        SfxStop(remoteEngineVoice1);
        SfxStop(remoteEngineVoice2);
        SfxStop(turbulenceVoice);
        OutputDebugString(msgTerminatedSound);
    }
#endif
}

void SetShieldHum(short enabled)
{
#ifdef NETPLAY
    if (enabled == 1) {
        if (shieldHumPlaying != 0) {
            return;
        } else {
            shieldHumVoice = SfxPlayId(sampleHandle[SAMPLE_SHIELD], 0, 15, 15, 153, 3);
            shieldHumPlaying = 1;
        }
    } else {
        if (shieldHumPlaying != 0) {
            SfxStop(shieldHumVoice);
            shieldHumPlaying = 0;
        } else {
            return;
        }
    }
#else
    if (enabled == 1) {
        if (shieldHumPlaying == 0) {
            shieldHumVoice = SfxPlayId(sampleHandle[SAMPLE_SHIELD], 0, 15, 15, 153, 3);
            shieldHumPlaying = 1;
        }
    } else {
        if (shieldHumPlaying != 0) {
            SfxStop(shieldHumVoice);
            shieldHumPlaying = 0;
        }
    }
#endif
}

void DroidSound(short enabled)
{
#ifdef NETPLAY
    if (enabled == 1) {
        if (droidSoundPlaying != 0) {
            return;
        } else {
            droidVoice = SfxPlayId(sampleHandle[SAMPLE_DROID], 0, 15, 15, 256, 3);
            droidSoundPlaying = 1;
        }
    } else {
        if (droidSoundPlaying != 0) {
            SfxStop(droidVoice);
            droidSoundPlaying = 0;
        } else {
            return;
        }
    }
#else
    if (enabled == 1) {
        if (droidSoundPlaying == 0) {
            droidVoice = SfxPlayId(sampleHandle[SAMPLE_DROID], 0, 15, 15, 256, 3);
            droidSoundPlaying = 1;
        }
    } else {
        if (droidSoundPlaying != 0) {
            SfxStop(droidVoice);
            droidSoundPlaying = 0;
        }
    }
#endif
}

void SetPitAmbient(short enabled)
{
#ifdef NETPLAY
    if (enabled == 1) {
        if (pitAmbientPlaying != 0) {
            return;
        } else {
            pitAmbientVoice = SfxPlayId(sampleHandle[SAMPLE_RECHARGE], 0, 15, 15, 255, 3);
            pitAmbientPlaying = 1;
        }
    } else {
        if (pitAmbientPlaying != 0) {
            SfxStop(pitAmbientVoice);
            pitAmbientPlaying = 0;
        } else {
            return;
        }
    }
#else
    if (enabled == 1) {
        if (pitAmbientPlaying == 0) {
            pitAmbientVoice = SfxPlayId(sampleHandle[SAMPLE_RECHARGE], 0, 15, 15, 255, 3);
            pitAmbientPlaying = 1;
        }
    } else {
        if (pitAmbientPlaying != 0) {
            SfxStop(pitAmbientVoice);
            pitAmbientPlaying = 0;
        }
    }
#endif
}

/* This PC sound callback has no effect. */
void SetAmbientLoop3(short enabled)
{
#ifdef NETPLAY
    return;
    if (enabled == 1) {
        if (loop3 != 0) {
            return;
        } else {
            ambientLoop3Voice = SfxPlayId(ambientLoop3Sample, 0, 15, 15, 256, 3);
            loop3 = 1;
        }
    } else {
        if (loop3 != 0) {
            SfxStop(ambientLoop3Voice);
            loop3 = 0;
        } else {
            return;
        }
    }
#endif
}

void CrowdSound(short enabled)
{
#ifdef NETPLAY
    if (enabled == 1) {
        if (crowdSoundPlaying != 0) {
            return;
        } else {
            crowdVoice = SfxPlayId(sampleHandle[SAMPLE_CROWD], 0, 15, 15, 256, 3);
            crowdSoundPlaying = 1;
        }
    } else {
        if (crowdSoundPlaying != 0) {
            SfxStop(crowdVoice);
            crowdSoundPlaying = 0;
        } else {
            return;
        }
    }
#else
    if (enabled == 1) {
        if (crowdSoundPlaying == 0) {
            crowdVoice = SfxPlayId(sampleHandle[SAMPLE_CROWD], 0, 15, 15, 256, 3);
            crowdSoundPlaying = 1;
        }
    } else {
        if (crowdSoundPlaying != 0) {
            SfxStop(crowdVoice);
            crowdSoundPlaying = 0;
        }
    }
#endif
}

void ScrapeSound(short enabled)
{
#ifdef NETPLAY
    if (enabled == 1) {
        if (scrapeSoundPlaying != 0) {
            return;
        } else {
            scrapeVoice = SfxPlayId(sampleHandle[SAMPLE_SCRAPE], 0, 15, 15, 192, 3);
            scrapeSoundPlaying = 1;
        }
    } else {
        if (scrapeSoundPlaying != 0) {
            SfxStop(scrapeVoice);
            scrapeSoundPlaying = 0;
        } else {
            return;
        }
    }
#else
    if (enabled == 1) {
        if (scrapeSoundPlaying == 0) {
            scrapeVoice = SfxPlayId(sampleHandle[SAMPLE_SCRAPE], 0, 15, 15, 192, 3);
            scrapeSoundPlaying = 1;
        }
    } else {
        if (scrapeSoundPlaying != 0) {
            SfxStop(scrapeVoice);
            scrapeSoundPlaying = 0;
        }
    }
#endif
}


/* PC sound-driver compatibility entries with fixed return values or no operation. */
/* Does nothing in the PC sound driver. */
void SoundDriverNoOp(void) { return; }

/* Returns one. */
int SoundDriverReturnsOneA(void) { return 1; }

/* Returns zero. */
int SoundDriverReturnsZeroA(void) { return 0; }

/* Returns zero. */
int SoundDriverReturnsZeroB(void) { return 0; }

/* Returns one. */
int SoundDriverReturnsOneB(void) { return 1; }

/* Returns zero. */
int SoundDriverReturnsZeroC(void) { return 0; }

/* Returns zero. */
int SoundDriverReturnsZeroD(void) { return 0; }

/* Returns the supplied value. */
int SoundDriverReturnsInput(int value) { return value; }

/* Returns the second supplied value. */
int SoundDriverReturnsSecondInput(int first, int second) { return second; }

/* Returns one. */
int SoundDriverReturnsOneC(void) { return 1; }

/* Returns zero. */
int UsesAlternateTexturePageLayout(void) { return 0; }

/* Returns zero. */
int SoundDriverReturnsZeroE(void) { return 0; }

/* Does nothing in the PC sound driver. */
void SoundDriverNoOp2(void) {}

/* Does nothing in the PC sound driver. */
void SoundDriverNoOp3(void) {}

/* Does nothing in the PC sound driver. */
void SoundDriverNoOp4(void) {}

/* Does nothing in the PC sound driver. */
void SoundDriverNoOp5(void) {}

/* Does nothing in the PC sound driver. */
void SoundDriverNoOp6(void) {}
