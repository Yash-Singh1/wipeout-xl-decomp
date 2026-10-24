#include "functions.h"
#include <math.h>
#ifdef NETPLAY
#pragma intrinsic(sqrt)
#endif

extern short flyingFlags[];
int SectionSearch(ShipData *playerShip, short shipNo);

/* Finds the nearby section and updates the ship's section and flying state. */
int SectionSearch(ShipData *playerShip, short shipNo)
{
#ifdef NETPLAY
    TrackSection *track, *closestSection;
    short sectionFlag;
    VECTOR centreToShip;
    int minDistSq;
    int magnitudeSq;
    int index;
    short junctionChecked = 0;
    TrackSection *junctionTrk;

    track = playerShip->nearTrkSect;
    for (index = 0; index < 3; index++)
        track = track->prevSection.ptr;
    sectionFlag = track->flags;
    centreToShip.vx = playerShip->ppivot.vx - track->centre.vx;
    centreToShip.vy = (playerShip->ppivot.vy - track->centre.vy) >> 2;
    centreToShip.vz = playerShip->ppivot.vz - track->centre.vz;
    minDistSq = centreToShip.vx * centreToShip.vx + (centreToShip.vy * centreToShip.vy + centreToShip.vz * centreToShip.vz);
    if (track->junction.index != -1) {
        junctionTrk = track->junction.ptr;
        junctionChecked = 1;
    }
    closestSection = track;
    for (index = 0; index < 6; index++) {
        track = track->nextSection.ptr;
        if (track->junction.index != -1) {
            junctionTrk = track->junction.ptr;
            junctionChecked = 1;
        }
        centreToShip.vx = playerShip->ppivot.vx - track->centre.vx;
        centreToShip.vy = (playerShip->ppivot.vy - track->centre.vy) >> 2;
        centreToShip.vz = playerShip->ppivot.vz - track->centre.vz;
        magnitudeSq = centreToShip.vx * centreToShip.vx + (centreToShip.vy * centreToShip.vy + centreToShip.vz * centreToShip.vz);
        if (magnitudeSq < minDistSq) {
            minDistSq = magnitudeSq;
            closestSection = track;
        }
    }
    if (junctionChecked == 1) {
        if (junctionTrk->flags & 0x10) {
            track = junctionTrk;
        } else if (sectionFlag == 0x20) {
            track = junctionTrk->nextSection.ptr;
        } else {
            track = junctionTrk;
        }
        for (index = 0; index < 6; index++) {
            centreToShip.vx = playerShip->ppivot.vx - track->centre.vx;
            centreToShip.vy = (playerShip->ppivot.vy - track->centre.vy) >> 2;
            centreToShip.vz = playerShip->ppivot.vz - track->centre.vz;
            magnitudeSq = centreToShip.vx * centreToShip.vx + (centreToShip.vy * centreToShip.vy + centreToShip.vz * centreToShip.vz);
            if (magnitudeSq < minDistSq) {
                minDistSq = magnitudeSq;
                closestSection = track;
            }
            if (junctionTrk->flags & 8)
                track = track->prevSection.ptr;
            else
                track = track->nextSection.ptr;
        }
    }
    minDistSq = sqrt((float)minDistSq);
    if (minDistSq > 3700) {
        playerShip->attr |= FLYING;
        if (playerShip->update == UpdateDroneShip)
            flyingFlags[shipNo] |= FLYING;
    } else {
        playerShip->attr &= ~FLYING;
    }
    playerShip->nearTrkSect = closestSection;
    return minDistSq;
#else
    TrackSection *trk, *nearestSection;
    int smallestMag;
    VECTOR centreToShip;
    int mag;
    int i;
    short juncChecked = 0;
    TrackSection *junc;
    short sectionFlags;

    trk = playerShip->nearTrkSect;
    for (i = 0; i < 3; i++)
        trk = trk->prevSection.ptr;
    sectionFlags = trk->flags;
    centreToShip.vx = playerShip->ppivot.vx - trk->centre.vx;
    centreToShip.vy = (playerShip->ppivot.vy - trk->centre.vy) >> 2;
    centreToShip.vz = playerShip->ppivot.vz - trk->centre.vz;
    smallestMag = centreToShip.vx * centreToShip.vx + centreToShip.vy * centreToShip.vy + centreToShip.vz * centreToShip.vz;
    if (trk->junction.index != -1) {
        junc = trk->junction.ptr;
        juncChecked = 1;
    }
    nearestSection = trk;
    for (i = 0; i < 6; i++) {
        trk = trk->nextSection.ptr;
        if (trk->junction.index != -1) {
            junc = trk->junction.ptr;
            juncChecked = 1;
        }
        centreToShip.vx = playerShip->ppivot.vx - trk->centre.vx;
        centreToShip.vy = (playerShip->ppivot.vy - trk->centre.vy) >> 2;
        centreToShip.vz = playerShip->ppivot.vz - trk->centre.vz;
        mag = centreToShip.vx * centreToShip.vx + centreToShip.vy * centreToShip.vy + centreToShip.vz * centreToShip.vz;
        if (mag < smallestMag) {
            smallestMag = mag;
            nearestSection = trk;
        }
    }
    if (juncChecked == 1) {
        if (junc->flags & 0x10) {
            trk = junc;
        } else {
            trk = junc;
            if (sectionFlags == 0x20)
                trk = trk->nextSection.ptr;
        }
        for (i = 0; i < 6; i++) {
            centreToShip.vx = playerShip->ppivot.vx - trk->centre.vx;
            centreToShip.vy = (playerShip->ppivot.vy - trk->centre.vy) >> 2;
            centreToShip.vz = playerShip->ppivot.vz - trk->centre.vz;
            mag = centreToShip.vx * centreToShip.vx + centreToShip.vy * centreToShip.vy + centreToShip.vz * centreToShip.vz;
            if (mag < smallestMag) {
                smallestMag = mag;
                nearestSection = trk;
            }
            if (junc->flags & 8)
                trk = trk->prevSection.ptr;
            else
                trk = trk->nextSection.ptr;
        }
    }
    smallestMag = sqrt(smallestMag);
    if (smallestMag > 3700) {
        playerShip->attr |= FLYING;
        if (playerShip->update == UpdateDroneShip)
            flyingFlags[shipNo] |= FLYING;
    } else {
        playerShip->attr &= ~FLYING;
    }
    playerShip->nearTrkSect = nearestSection;
    return smallestMag;
#endif
}

/* Returns the nearest section around the supplied position. */
TrackSection *FindNearestSection(TrackSection *trk, VECTOR pos)
{
#ifdef NETPLAY
    VECTOR diff;
    short sectionFlag;
    int smallestMagSq;
    TrackSection *succ;
    TrackSection *closestSection;
    int distSq;
    TrackSection *jn;
    int index;
    short junctionChecked;

    junctionChecked = 0;
    succ = trk->nextSection.ptr;
    for (index = 0; index < 3; index++)
        trk = trk->prevSection.ptr;
    sectionFlag = trk->flags;
    diff.vx = pos.vx - trk->centre.vx;
    diff.vy = pos.vy - trk->centre.vy;
    diff.vz = pos.vz - trk->centre.vz;
    smallestMagSq = diff.vx * diff.vx + diff.vy * diff.vy + diff.vz * diff.vz;
    closestSection = trk;
    if (trk->junction.index != -1) {
        jn = trk->junction.ptr;
        junctionChecked = 1;
    }
    for (index = 0; index < 6; index++) {
        trk = trk->nextSection.ptr;
        if (trk->junction.index != -1) {
            jn = trk->junction.ptr;
            junctionChecked = 1;
        }
        diff.vx = pos.vx - trk->centre.vx;
        diff.vy = pos.vy - trk->centre.vy;
        diff.vz = pos.vz - trk->centre.vz;
        distSq = diff.vx * diff.vx + diff.vy * diff.vy + diff.vz * diff.vz;
        if (distSq < smallestMagSq) {
            smallestMagSq = distSq;
            closestSection = trk;
        }
    }
    if (junctionChecked == 1) {
        if (jn->flags & 0x10) {
            trk = jn;
        } else if (sectionFlag == 0x20) {
            trk = jn->nextSection.ptr;
        } else {
            trk = jn;
        }
        for (index = 0; index < 6; index++) {
            diff.vx = pos.vx - trk->centre.vx;
            diff.vy = pos.vy - trk->centre.vy;
            diff.vz = pos.vz - trk->centre.vz;
            distSq = diff.vx * diff.vx + diff.vy * diff.vy + diff.vz * diff.vz;
            if (distSq < smallestMagSq) {
                smallestMagSq = distSq;
                closestSection = trk;
            }
            if (jn->flags & 8)
                trk = trk->prevSection.ptr;
            else
                trk = trk->nextSection.ptr;
        }
    }
    return closestSection;
#else
    TrackSection *nearestSection;
    int smallestMag;
    VECTOR centreToPos;
    int mag;
    int i;
    short juncChecked = 0;
    TrackSection *junc;
    short sectionFlags;

    for (i = 0; i < 3; i++)
        trk = trk->prevSection.ptr;
    sectionFlags = trk->flags;
    centreToPos.vx = pos.vx - trk->centre.vx;
    centreToPos.vy = pos.vy - trk->centre.vy;
    centreToPos.vz = pos.vz - trk->centre.vz;
    smallestMag = centreToPos.vx * centreToPos.vx + centreToPos.vy * centreToPos.vy + centreToPos.vz * centreToPos.vz;
    nearestSection = trk;
    if (trk->junction.index != -1) {
        junc = trk->junction.ptr;
        juncChecked = 1;
    }
    for (i = 0; i < 6; i++) {
        trk = trk->nextSection.ptr;
        if (trk->junction.index != -1) {
            junc = trk->junction.ptr;
            juncChecked = 1;
        }
        centreToPos.vx = pos.vx - trk->centre.vx;
        centreToPos.vy = pos.vy - trk->centre.vy;
        centreToPos.vz = pos.vz - trk->centre.vz;
        mag = centreToPos.vx * centreToPos.vx + centreToPos.vy * centreToPos.vy + centreToPos.vz * centreToPos.vz;
        if (mag < smallestMag) {
            smallestMag = mag;
            nearestSection = trk;
        }
    }
    if (juncChecked == 1) {
        if (junc->flags & 0x10) {
            trk = junc;
        } else {
            trk = junc;
            if (sectionFlags == 0x20)
                trk = trk->nextSection.ptr;
        }
        for (i = 0; i < 6; i++) {
            centreToPos.vx = pos.vx - trk->centre.vx;
            centreToPos.vy = pos.vy - trk->centre.vy;
            centreToPos.vz = pos.vz - trk->centre.vz;
            mag = centreToPos.vx * centreToPos.vx + centreToPos.vy * centreToPos.vy + centreToPos.vz * centreToPos.vz;
            if (mag < smallestMag) {
                smallestMag = mag;
                nearestSection = trk;
            }
            if (junc->flags & 8)
                trk = trk->prevSection.ptr;
            else
                trk = trk->nextSection.ptr;
        }
    }
    return nearestSection;
#endif
}
