#include "functions.h"
#include "dynam.h"
#include <math.h>
#include "mathinline.h"
#include "camera.h"

extern TrackData *track;

void OuterProduct12(VECTOR *left, VECTOR *right, VECTOR *crossProduct);
void VectorNormal(VECTOR *vector, VECTOR *normalized);

/* The PC build works in doubles: 4096 units make a full turn, scaled through sin and cos. */
#ifdef NETPLAY
/* NET's /Od build expands these expressions at the call site. */
#pragma intrinsic(sin, cos)
#define RotSin(angle) ((short)(sin(((double)(angle) / 4096.0) * 6.28318530718) * 4096.0))
#define RotCos(angle) ((short)(cos(((double)(angle) / 4096.0) * 6.28318530718) * 4096.0))
#else
static __inline short RotSin(short angle)
{
    return (short)(sin(((double)angle / 4096.0) * 6.28318530718) * 4096.0);
}

static __inline short RotCos(short angle)
{
    return (short)(cos(((double)angle / 4096.0) * 6.28318530718) * 4096.0);
}
#endif


extern short flyingFlags[15];
extern int droidFlyin;
#ifdef NETPLAY
extern short ownShip;
#endif

/* Ship reaction against the track: thrust, drag, collision and attitude response. */
#ifdef EQUIV
#ifdef NETPLAY
void ShipTrkReaction(register ShipData *playerShip, Object **shipShapes, short shipNo)
{
    int lift;
    int retard;
    VECTOR offsetToTarget;
    int trackAhead = 0;
    int alpha;
    int load;
    TrackSection *partner;
    VECTOR aimPos;
    short braking;
    Face *plane;
    int drop;
    TrackSection *onward;
    float vy;
    float curXVel;
    TrackSection *here;
    VECTOR nosePos;
    VECTOR push;
    int velMag;
    short boostFactor;
    float vz;
    VECTOR sectDir;
    VECTOR shipTargetVelocity;
    VECTOR dir;
    VECTOR facePoint;

    if (playerShip->thrustCount == 0) {
        boostFactor = 1;
    } else if (playerShip->thrustCount > 0 && playerShip->thrustCount < 3) {
        boostFactor = 3;
    } else if (playerShip->thrustCount >= 3) {
        boostFactor = 6;
    }
    playerShip->thrustVector.vx = (playerShip->unitVecNose.vx * playerShip->thrust * boostFactor) >> 6;
    playerShip->thrustVector.vy = (playerShip->unitVecNose.vy * playerShip->thrust * boostFactor) >> 6;
    playerShip->thrustVector.vz = (playerShip->unitVecNose.vz * playerShip->thrust * boostFactor) >> 6;

    curXVel = playerShip->vpivot.vx >> 1;
    vy = playerShip->vpivot.vy >> 1;
    vz = playerShip->vpivot.vz >> 1;
    velMag = GetMagnitude(curXVel, vy, vz) << 1;
    if (playerShip->autopilotCount == 0) {
        playerShip->speed = velMag >> 1;
    }
    shipTargetVelocity.vx = (velMag * playerShip->unitVecNose.vx) / 4096;
    shipTargetVelocity.vy = (velMag * playerShip->unitVecNose.vy) / 4096;
    shipTargetVelocity.vz = (velMag * playerShip->unitVecNose.vz) / 4096;

    if (playerShip->nearTrkSect->flags & 1) {
        plane = track->faces + playerShip->nearTrkSect->faceStart;
        while (!(plane->flags & TRACK_BASE)) {
            plane++;
        }
        facePoint.vx = track->vertices[plane->vertex[0]].vx;
        facePoint.vy = track->vertices[plane->vertex[0]].vy;
        facePoint.vz = track->vertices[plane->vertex[0]].vz;
        alpha = PlaneLineCollision(facePoint, playerShip->ppivot, plane->normal);
        if (CheckPolygonCollision(plane, playerShip->ppivot, alpha, 2) == 0) {
            playerShip->attr |= FLYING;
            flyingFlags[shipNo] |= 0x10;
        } else {
            flyingFlags[shipNo] = 0;
        }
    }

    if (!(playerShip->attr & FLYING)) {
        plane = track->faces + playerShip->nearTrkSect->faceStart;
        while (!(plane->flags & TRACK_BASE)) {
            plane++;
        }
        Ship2Track(playerShip, plane, shipShapes, shipNo);
        if ((plane->flags & SPEED_UP) && (playerShip->attr & LSIDE)) {
            playerShip->vpivot.vx += playerShip->nearTrkSect->nextSection.ptr->centre.vx - playerShip->nearTrkSect->centre.vx;
            playerShip->vpivot.vy += playerShip->nearTrkSect->nextSection.ptr->centre.vy - playerShip->nearTrkSect->centre.vy;
            playerShip->vpivot.vz += playerShip->nearTrkSect->nextSection.ptr->centre.vz - playerShip->nearTrkSect->centre.vz;
        }
        plane++;
        if ((plane->flags & SPEED_UP) && !(playerShip->attr & LSIDE)) {
            playerShip->vpivot.vx += playerShip->nearTrkSect->nextSection.ptr->centre.vx - playerShip->nearTrkSect->centre.vx;
            playerShip->vpivot.vy += playerShip->nearTrkSect->nextSection.ptr->centre.vy - playerShip->nearTrkSect->centre.vy;
            playerShip->vpivot.vz += playerShip->nearTrkSect->nextSection.ptr->centre.vz - playerShip->nearTrkSect->centre.vz;
        }
        plane--;
        if (!(playerShip->attr & LSIDE)) {
            plane++;
        }
        facePoint.vx = track->vertices[plane->vertex[0]].vx;
        facePoint.vy = track->vertices[plane->vertex[0]].vy;
        facePoint.vz = track->vertices[plane->vertex[0]].vz;
        alpha = PlaneLineCollision(facePoint, playerShip->ppivot, plane->normal);
        if (alpha < -300) {
            alpha = -300;
        }
        lift = alpha;
        Ship2TrackBase(playerShip, plane, alpha);

        braking = playerShip->r_brake_rot + playerShip->l_brake_rot;
        playerShip->apivot.vx = (shipTargetVelocity.vx - playerShip->vpivot.vx) / (playerShip->skid + (braking >> 2));
        playerShip->apivot.vy = (shipTargetVelocity.vy - playerShip->vpivot.vy) / (playerShip->skid + (braking >> 2));
        playerShip->apivot.vz = (shipTargetVelocity.vz - playerShip->vpivot.vz) / (playerShip->skid + (braking >> 2));

        if (alpha < 75) {
            alpha = 75;
        }
        push.vx = (plane->normal.vx << 14) / alpha;
        push.vy = (plane->normal.vy << 14) / alpha;
        push.vz = (plane->normal.vz << 14) / alpha;
        push.vx -= plane->normal.vx << 6;
        push.vy -= plane->normal.vy << 6;
        push.vz -= plane->normal.vz << 6;
        push.vy += 30000;

        drop = -(playerShip->ppivot.vy - playerShip->nearTrkSect->centre.vy);
        load = playerShip->mass;
        if (drop > 704) {
            if (load == 110) {
                playerShip->ppivot.vy += 80;
            }
            if (playerShip->vpivot.vy < 0) {
                playerShip->vpivot.vy >>= 1;
            }
            playerShip->ppivot.vy += 16;
        }
        push.vy += drop << 6;

        playerShip->apivot.vx += (playerShip->thrustVector.vx + push.vx) / load;
        playerShip->apivot.vy += (playerShip->thrustVector.vy + push.vy) / load;
        playerShip->apivot.vz += (playerShip->thrustVector.vz + push.vz) / load;

        playerShip->vpivot.vx += playerShip->apivot.vx;
        playerShip->vpivot.vy += playerShip->apivot.vy;
        playerShip->vpivot.vz += playerShip->apivot.vz;
        retard = (playerShip->resistance * (74 - (braking >> 3))) >> 7;
        playerShip->vpivot.vx -= playerShip->vpivot.vx / retard;
        playerShip->vpivot.vy -= playerShip->vpivot.vy / retard;
        playerShip->vpivot.vz -= playerShip->vpivot.vz / retard;

        playerShip->ppivot.vx += sar(playerShip->vpivot.vx, 6);
        playerShip->ppivot.vy += sar(playerShip->vpivot.vy, 6);
        playerShip->ppivot.vz += sar(playerShip->vpivot.vz, 6);

        nosePos.vx = (playerShip->unitVecNose.vx >> 5) + playerShip->ppivot.vx;
        nosePos.vy = (playerShip->unitVecNose.vy >> 5) + playerShip->ppivot.vy;
        nosePos.vz = (playerShip->unitVecNose.vz >> 5) + playerShip->ppivot.vz;
        alpha = PlaneLineCollision(facePoint, nosePos, plane->normal);

        if (playerShip->noAttitudeControl == 0) {
            lift -= alpha;
            if (alpha < 600) {
                playerShip->vpitch += lift + 5;
            } else {
                playerShip->vpitch -= 50;
            }
            playerShip->vpitch -= playerShip->vpitch >> 2;
            braking = (playerShip->r_brake_rot - playerShip->l_brake_rot) / 8;
            playerShip->hdg += (playerShip->speed * braking) / 32768;
            playerShip->vroll += (playerShip->vhdg / 32);
            playerShip->vroll -= (playerShip->vroll >> 1);
        }
    } else {
        if (playerShip->vpivot.vy < playerShip->nearTrkSect->centre.vy) {
            plane = track->faces + playerShip->nearTrkSect->faceStart;
            while (!(plane->flags & TRACK_BASE)) {
                plane++;
            }
            Ship2Track(playerShip, plane, shipShapes, shipNo);
        }
        partner = playerShip->nearTrkSect->nextSection.ptr;
        GetTargetPos(&aimPos, playerShip->ppivot, partner->centre, playerShip->nearTrkSect->centre);
        offsetToTarget.vx = aimPos.vx - playerShip->ppivot.vx;
        offsetToTarget.vy = aimPos.vy - playerShip->ppivot.vy;
        offsetToTarget.vz = aimPos.vz - playerShip->ppivot.vz;
        if (offsetToTarget.vy > 0) {
            offsetToTarget.vy >>= 7;
        } else {
            offsetToTarget.vx <<= 3;
            offsetToTarget.vy <<= 3;
            offsetToTarget.vz <<= 3;
        }
        if (GetMagnitude(offsetToTarget.vx, offsetToTarget.vy, offsetToTarget.vz) > 32000) {
            playerShip->update = UpdateRescueDroid;
            playerShip->updateCount = 500;
            playerShip->attr |= RESCUE | IN_TOW;
            playerShip->thrust = 0;
            if (shipNo == ownShip) {
                droidFlyin = 1000;
            }
        }

        braking = playerShip->r_brake_rot + playerShip->l_brake_rot;
        playerShip->apivot.vx = (shipTargetVelocity.vx - playerShip->vpivot.vx) / ((braking << 2) + 20);
        playerShip->apivot.vy = (shipTargetVelocity.vy - playerShip->vpivot.vy) / ((braking << 2) + 20);
        playerShip->apivot.vz = (shipTargetVelocity.vz - playerShip->vpivot.vz) / ((braking << 2) + 20);

        drop = -(playerShip->ppivot.vy - playerShip->nearTrkSect->centre.vy);
        load = playerShip->mass;
        if (drop > 704) {
            if (load == 110) {
                playerShip->ppivot.vy += 80;
            }
            if (playerShip->vpivot.vy < 0) {
                playerShip->vpivot.vy >>= 1;
            }
            playerShip->ppivot.vy += 16;
        }

        playerShip->apivot.vx += playerShip->thrustVector.vx / playerShip->mass;
        playerShip->apivot.vy += (playerShip->thrustVector.vy + 80000) / playerShip->mass;
        playerShip->apivot.vz += playerShip->thrustVector.vz / playerShip->mass;

        playerShip->vpivot.vx += playerShip->apivot.vx;
        playerShip->vpivot.vy += playerShip->apivot.vy;
        playerShip->vpivot.vz += playerShip->apivot.vz;
        retard = (playerShip->resistance * (74 - (braking >> 3))) >> 7;
        playerShip->vpivot.vx -= playerShip->vpivot.vx / retard;
        playerShip->vpivot.vy -= playerShip->vpivot.vy / retard;
        playerShip->vpivot.vz -= playerShip->vpivot.vz / retard;

        playerShip->ppivot.vx += (playerShip->vpivot.vx / 64);
        playerShip->ppivot.vy += (playerShip->vpivot.vy / 64);
        playerShip->ppivot.vz += (playerShip->vpivot.vz / 64);

        playerShip->vpitch -= 50;
        playerShip->vpitch -= playerShip->vpitch >> 2;
        braking = (playerShip->r_brake_rot - playerShip->l_brake_rot) / 8;
        playerShip->hdg += (playerShip->speed * braking) / 32768;
        playerShip->vroll += (playerShip->vhdg / 32);
        playerShip->vroll -= (playerShip->vroll / 2);
    }

    playerShip->hdg += (playerShip->vhdg / 64);
    playerShip->pitch += playerShip->vpitch >> 4;
    playerShip->roll += playerShip->vroll;
    playerShip->roll -= playerShip->roll >> 3;
    playerShip->hdg = ang(playerShip->hdg);
    playerShip->pitch = ang(playerShip->pitch);
    playerShip->roll = ang(playerShip->roll);

    partner = playerShip->nearTrkSect->prevSection.ptr;
    here = playerShip->nearTrkSect;
    if (!(playerShip->attr & DIRECTION) && !(here->flags & 1) && (here->prevSection.ptr->flags & 1)) {
        playerShip->pitch -= playerShip->vpitch >> 4;
        playerShip->ppivot.vx -= (playerShip->vpivot.vx / 64);
        playerShip->ppivot.vy -= (playerShip->vpivot.vy / 64);
        playerShip->ppivot.vz -= (playerShip->vpivot.vz / 64);
        dir.vx = playerShip->nearTrkSect->nextSection.ptr->centre.vx - playerShip->nearTrkSect->centre.vx;
        dir.vy = playerShip->nearTrkSect->nextSection.ptr->centre.vy - playerShip->nearTrkSect->centre.vy;
        dir.vz = playerShip->nearTrkSect->nextSection.ptr->centre.vz - playerShip->nearTrkSect->centre.vz;
        playerShip->vpivot.vx = dir.vx << 2;
        playerShip->vpivot.vy = dir.vy << 2;
        playerShip->vpivot.vz = dir.vz << 2;
        playerShip->ppivot.vx += (playerShip->vpivot.vx / 64);
        playerShip->ppivot.vy += (playerShip->vpivot.vy / 64);
        playerShip->ppivot.vz += (playerShip->vpivot.vz / 64);
    }

    onward = playerShip->nearTrkSect->nextSection.ptr;
    sectDir.vx = onward->centre.vx - playerShip->nearTrkSect->centre.vx;
    sectDir.vy = onward->centre.vy - playerShip->nearTrkSect->centre.vy;
    sectDir.vz = onward->centre.vz - playerShip->nearTrkSect->centre.vz;
    trackAhead = sectDir.vx * playerShip->unitVecNose.vx + sectDir.vy * playerShip->unitVecNose.vy + sectDir.vz * playerShip->unitVecNose.vz;
    if (trackAhead < 0 && playerShip->ppivot.vy >= playerShip->nearTrkSect->centre.vy + 1000 && (playerShip->attr & FLYING) && (here->flags & 1) && !(here->prevSection.ptr->flags & 1)) {
        playerShip->update = UpdateRescueDroid;
        playerShip->updateCount = 500;
        playerShip->attr |= RESCUE | IN_TOW;
        playerShip->thrust = 0;
        if (ownShip == shipNo) {
            droidFlyin = 1000;
        }
    }
}
#else
void ShipTrkReaction(register ShipData *playerShip, Object **shipShapes, short shipNo)
{
    VECTOR reaction;
    VECTOR targetVelocity;
    VECTOR nosePos;
    VECTOR facePoint;
    VECTOR targetPos;
    VECTOR toTarget;
    VECTOR direction;
    VECTOR nextDirection;
    short boost;
    short brake;
    int speed;
    float xVelocity;
    float yVelocity;
    float zVelocity;
    int clearance;
    int altitudeDifference;
    int friction;
    int xDrag;
    int yDrag;
    int zDrag;
    int mass;
    int alpha;
    Face *face;
    TrackSection *current;
    TrackSection *neighbour;
    TrackSection *next;

    if (playerShip->thrustCount == 0) {
        boost = 1;
    } else if (playerShip->thrustCount > 0) {
        if (playerShip->thrustCount < 3)
            boost = 3;
        else
            boost = 6;
    } else if (playerShip->thrustCount < 3) {
        /* The original leaves boost uninitialized for negative counts. */
    } else {
        boost = 6;
    }
    playerShip->thrustVector.vx = (playerShip->unitVecNose.vx * playerShip->thrust * boost) >> 6;
    playerShip->thrustVector.vy = (playerShip->unitVecNose.vy * playerShip->thrust * boost) >> 6;
    playerShip->thrustVector.vz = (playerShip->unitVecNose.vz * playerShip->thrust * boost) >> 6;

    xVelocity = playerShip->vpivot.vx >> 1;
    yVelocity = playerShip->vpivot.vy >> 1;
    zVelocity = playerShip->vpivot.vz >> 1;
    speed = GetMagnitude(xVelocity, yVelocity, zVelocity) << 1;
    if (playerShip->autopilotCount == 0) {
        playerShip->speed = speed >> 1;
    }
    targetVelocity.vx = (speed * playerShip->unitVecNose.vx) / 4096;
    targetVelocity.vy = (speed * playerShip->unitVecNose.vy) / 4096;
    targetVelocity.vz = (speed * playerShip->unitVecNose.vz) / 4096;

    if (playerShip->nearTrkSect->flags & 1) {
        face = track->faces + playerShip->nearTrkSect->faceStart;
        while (!(face->flags & TRACK_BASE)) {
            face++;
        }
        facePoint.vx = track->vertices[face->vertex[0]].vx;
        facePoint.vy = track->vertices[face->vertex[0]].vy;
        facePoint.vz = track->vertices[face->vertex[0]].vz;
        alpha = PlaneLineCollision(facePoint, playerShip->ppivot, face->normal);
        if (CheckPolygonCollision(face, playerShip->ppivot, alpha, 2) == 0) {
            playerShip->attr |= FLYING;
            flyingFlags[shipNo] |= 0x10;
        } else {
            flyingFlags[shipNo] = 0;
        }
    }

    if (!(playerShip->attr & FLYING)) {
        face = track->faces + playerShip->nearTrkSect->faceStart;
        while (!(face->flags & TRACK_BASE)) {
            face++;
        }
        Ship2Track(playerShip, face, shipShapes);
        if ((face->flags & SPEED_UP) && (playerShip->attr & LSIDE)) {
            playerShip->vpivot.vx += playerShip->nearTrkSect->nextSection.ptr->centre.vx - playerShip->nearTrkSect->centre.vx;
            playerShip->vpivot.vy += playerShip->nearTrkSect->nextSection.ptr->centre.vy - playerShip->nearTrkSect->centre.vy;
            playerShip->vpivot.vz += playerShip->nearTrkSect->nextSection.ptr->centre.vz - playerShip->nearTrkSect->centre.vz;
        }
        face++;
        if ((face->flags & SPEED_UP) && !(playerShip->attr & LSIDE)) {
            playerShip->vpivot.vx += playerShip->nearTrkSect->nextSection.ptr->centre.vx - playerShip->nearTrkSect->centre.vx;
            playerShip->vpivot.vy += playerShip->nearTrkSect->nextSection.ptr->centre.vy - playerShip->nearTrkSect->centre.vy;
            playerShip->vpivot.vz += playerShip->nearTrkSect->nextSection.ptr->centre.vz - playerShip->nearTrkSect->centre.vz;
        }
        face--;
        if (!(playerShip->attr & LSIDE)) {
            face++;
        }
        facePoint.vx = track->vertices[face->vertex[0]].vx;
        facePoint.vy = track->vertices[face->vertex[0]].vy;
        facePoint.vz = track->vertices[face->vertex[0]].vz;
        alpha = PlaneLineCollision(facePoint, playerShip->ppivot, face->normal);
        if (alpha < -300) {
            alpha = -300;
        }
        clearance = alpha;
        Ship2TrackBase(playerShip, face, clearance);

        brake = playerShip->r_brake_rot + playerShip->l_brake_rot;
        playerShip->apivot.vx = (targetVelocity.vx - playerShip->vpivot.vx) / (playerShip->skid + (brake >> 2));
        playerShip->apivot.vy = (targetVelocity.vy - playerShip->vpivot.vy) / (playerShip->skid + (brake >> 2));
        playerShip->apivot.vz = (targetVelocity.vz - playerShip->vpivot.vz) / (playerShip->skid + (brake >> 2));

        if (clearance < 75) {
            alpha = 75;
        }
        reaction.vx = (face->normal.vx << 14) / alpha;
        reaction.vy = (face->normal.vy << 14) / alpha;
        reaction.vz = (face->normal.vz << 14) / alpha;
        reaction.vx -= face->normal.vx << 6;
        reaction.vy -= face->normal.vy << 6;
        reaction.vz -= face->normal.vz << 6;
        reaction.vy += 30000;

        mass = playerShip->mass;
        altitudeDifference = playerShip->nearTrkSect->centre.vy - playerShip->ppivot.vy;
        if (altitudeDifference > 704) {
            if (mass == 110) {
                playerShip->ppivot.vy += 80;
            }
            if (playerShip->vpivot.vy < 0) {
                playerShip->vpivot.vy >>= 1;
            }
            playerShip->ppivot.vy += 16;
        }
        reaction.vy += altitudeDifference << 6;

        playerShip->apivot.vx += (reaction.vx + playerShip->thrustVector.vx) / mass;
        playerShip->apivot.vy += (reaction.vy + playerShip->thrustVector.vy) / mass;
        playerShip->apivot.vz += (reaction.vz + playerShip->thrustVector.vz) / mass;

        playerShip->vpivot.vx += playerShip->apivot.vx;
        playerShip->vpivot.vy += playerShip->apivot.vy;
        playerShip->vpivot.vz += playerShip->apivot.vz;
        friction = (playerShip->resistance * (74 - (brake >> 3))) >> 7;
        xDrag = playerShip->vpivot.vx;
        yDrag = playerShip->vpivot.vy;
        zDrag = playerShip->vpivot.vz;
        playerShip->vpivot.vx = xDrag - xDrag / friction;
        playerShip->vpivot.vy = yDrag - yDrag / friction;
        playerShip->vpivot.vz = zDrag - zDrag / friction;

        playerShip->ppivot.vx += sar(playerShip->vpivot.vx, 6);
        playerShip->ppivot.vy += sar(playerShip->vpivot.vy, 6);
        playerShip->ppivot.vz += sar(playerShip->vpivot.vz, 6);

        nosePos.vx = (playerShip->unitVecNose.vx >> 5) + playerShip->ppivot.vx;
        nosePos.vy = (playerShip->unitVecNose.vy >> 5) + playerShip->ppivot.vy;
        nosePos.vz = (playerShip->unitVecNose.vz >> 5) + playerShip->ppivot.vz;
        alpha = PlaneLineCollision(facePoint, nosePos, face->normal);

        if (playerShip->noAttitudeControl == 0) {
            clearance -= alpha;
            if (alpha < 600) {
                playerShip->vpitch += clearance + 5;
            } else {
                playerShip->vpitch -= 50;
            }
            playerShip->vpitch -= playerShip->vpitch >> 2;
            playerShip->hdg += ((short)((playerShip->r_brake_rot - playerShip->l_brake_rot) / 8) * playerShip->speed) / 32768;
            playerShip->vroll += (playerShip->vhdg / 32);
            playerShip->vroll -= (playerShip->vroll >> 1);
        }
    } else {
        float targetDistanceX;
        float targetDistanceY;
        float targetDistanceZ;

        if (playerShip->vpivot.vy < playerShip->nearTrkSect->centre.vy) {
            face = track->faces + playerShip->nearTrkSect->faceStart;
            while (!(face->flags & TRACK_BASE)) {
                face++;
            }
            Ship2Track(playerShip, face, shipShapes);
        }
        neighbour = playerShip->nearTrkSect->nextSection.ptr;
        GetTargetPos(&targetPos, playerShip->ppivot, neighbour->centre, playerShip->nearTrkSect->centre);
        toTarget.vx = targetPos.vx - playerShip->ppivot.vx;
        toTarget.vy = targetPos.vy - playerShip->ppivot.vy;
        toTarget.vz = targetPos.vz - playerShip->ppivot.vz;
        if (toTarget.vy > 0) {
            toTarget.vy >>= 7;
        } else {
            toTarget.vx <<= 3;
            toTarget.vy <<= 3;
            toTarget.vz <<= 3;
        }
        targetDistanceX = toTarget.vx;
        targetDistanceY = toTarget.vy;
        targetDistanceZ = toTarget.vz;
        if (GetMagnitude(targetDistanceX, targetDistanceY, targetDistanceZ) > 32000) {
            playerShip->update = UpdateRescueDroid;
            playerShip->updateCount = 500;
            droidFlyin = 1000;
            playerShip->thrust = 0;
            playerShip->attr |= RESCUE | IN_TOW;
        }

        brake = playerShip->r_brake_rot + playerShip->l_brake_rot;
        playerShip->apivot.vx = (targetVelocity.vx - playerShip->vpivot.vx) / ((brake << 2) + 20);
        playerShip->apivot.vy = (targetVelocity.vy - playerShip->vpivot.vy) / ((brake << 2) + 20);
        playerShip->apivot.vz = (targetVelocity.vz - playerShip->vpivot.vz) / ((brake << 2) + 20);

        mass = playerShip->mass;
        altitudeDifference = playerShip->nearTrkSect->centre.vy - playerShip->ppivot.vy;
        if (altitudeDifference > 704) {
            if (mass == 110) {
                playerShip->ppivot.vy += 80;
            }
            if (playerShip->vpivot.vy < 0) {
                playerShip->vpivot.vy >>= 1;
            }
            playerShip->ppivot.vy += 16;
        }

        playerShip->apivot.vx += playerShip->thrustVector.vx / mass;
        playerShip->apivot.vy += (playerShip->thrustVector.vy + 80000) / mass;
        playerShip->apivot.vz += playerShip->thrustVector.vz / mass;

        playerShip->vpivot.vx += playerShip->apivot.vx;
        playerShip->vpivot.vy += playerShip->apivot.vy;
        playerShip->vpivot.vz += playerShip->apivot.vz;
        friction = (playerShip->resistance * (74 - (brake >> 3))) >> 7;
        xDrag = playerShip->vpivot.vx;
        yDrag = playerShip->vpivot.vy;
        zDrag = playerShip->vpivot.vz;
        playerShip->vpivot.vx = xDrag - xDrag / friction;
        playerShip->vpivot.vy = yDrag - yDrag / friction;
        playerShip->vpivot.vz = zDrag - zDrag / friction;

        playerShip->ppivot.vx += (playerShip->vpivot.vx / 64);
        playerShip->ppivot.vy += (playerShip->vpivot.vy / 64);
        playerShip->ppivot.vz += (playerShip->vpivot.vz / 64);

        playerShip->vpitch -= 50;
        playerShip->vpitch -= playerShip->vpitch >> 2;
        playerShip->hdg += ((short)((playerShip->r_brake_rot - playerShip->l_brake_rot) / 8) * playerShip->speed) / 32768;
        playerShip->vroll += (playerShip->vhdg / 32);
        playerShip->vroll -= (playerShip->vroll / 2);
    }

    playerShip->hdg += (playerShip->vhdg / 64);
    playerShip->pitch += playerShip->vpitch >> 4;
    playerShip->roll += playerShip->vroll;
    playerShip->roll -= playerShip->roll >> 3;
    playerShip->hdg = ang(playerShip->hdg);
    playerShip->pitch = ang(playerShip->pitch);
    playerShip->roll = ang(playerShip->roll);

    neighbour = playerShip->nearTrkSect->prevSection.ptr;
    current = playerShip->nearTrkSect;
    if (!(playerShip->attr & DIRECTION) && !(current->flags & 1) && (neighbour->flags & 1)) {
        playerShip->pitch -= playerShip->vpitch >> 4;
        playerShip->ppivot.vx -= (playerShip->vpivot.vx / 64);
        playerShip->ppivot.vy -= (playerShip->vpivot.vy / 64);
        playerShip->ppivot.vz -= (playerShip->vpivot.vz / 64);
        direction.vx = playerShip->nearTrkSect->nextSection.ptr->centre.vx - playerShip->nearTrkSect->centre.vx;
        direction.vy = playerShip->nearTrkSect->nextSection.ptr->centre.vy - playerShip->nearTrkSect->centre.vy;
        direction.vz = playerShip->nearTrkSect->nextSection.ptr->centre.vz - playerShip->nearTrkSect->centre.vz;
        playerShip->vpivot.vx = direction.vx << 2;
        playerShip->vpivot.vy = direction.vy << 2;
        playerShip->vpivot.vz = direction.vz << 2;
        playerShip->ppivot.vx += (playerShip->vpivot.vx / 64);
        playerShip->ppivot.vy += (playerShip->vpivot.vy / 64);
        playerShip->ppivot.vz += (playerShip->vpivot.vz / 64);
    }

    next = playerShip->nearTrkSect->nextSection.ptr;
    nextDirection.vx = next->centre.vx - playerShip->nearTrkSect->centre.vx;
    nextDirection.vy = next->centre.vy - playerShip->nearTrkSect->centre.vy;
    nextDirection.vz = next->centre.vz - playerShip->nearTrkSect->centre.vz;
    if (nextDirection.vx * playerShip->unitVecNose.vx + nextDirection.vy * playerShip->unitVecNose.vy + nextDirection.vz * playerShip->unitVecNose.vz < 0 && playerShip->ppivot.vy >= playerShip->nearTrkSect->centre.vy + 1000 && (playerShip->attr & FLYING) && (current->flags & 1) && !(current->prevSection.ptr->flags & 1)) {
        playerShip->update = UpdateRescueDroid;
        playerShip->updateCount = 500;
        droidFlyin = 1000;
        playerShip->thrust = 0;
        playerShip->attr |= RESCUE | IN_TOW;
    }
}
#endif
#endif

/* Projects one vertex of objectCollide into camera space: the two skeletons are parked at the origin,
   the object's transform is applied, then their translations are restored. */
void getWorldPoint(VECTOR *output, Object *object, Object *objectCollide, int pointNo)
{
    VECTOR cameraPosition;
    VECTOR worldPosition;
    long flag;

    cameraPosition.vx = residentSkeleton.camPos->relative.t[0];
    cameraPosition.vy = residentSkeleton.camPos->relative.t[1];
    cameraPosition.vz = residentSkeleton.camPos->relative.t[2];
    worldPosition.vx = upperSkeleton->relative.t[0];
    worldPosition.vy = upperSkeleton->relative.t[1];
    worldPosition.vz = upperSkeleton->relative.t[2];
    SetSkeletonPosition(residentSkeleton.camPos, 0, 0, 0);
    SetSkeletonDirection(residentSkeleton.camPos, 0, 0, 0);
    SetSkeletonPosition(upperSkeleton, 0, 0, 0);
    SetSkeletonDirection(upperSkeleton, 0, 0, 0);
    SetSkeletonTransform(object->skeleton);
    residentSkeleton.camPos->relative.t[0] = cameraPosition.vx;
    residentSkeleton.camPos->relative.t[1] = cameraPosition.vy;
    residentSkeleton.camPos->relative.t[2] = cameraPosition.vz;
    upperSkeleton->relative.t[0] = worldPosition.vx;
    upperSkeleton->relative.t[1] = worldPosition.vy;
    upperSkeleton->relative.t[2] = worldPosition.vz;
    RotTrans(objectCollide->vertices + pointNo, output, &flag);
}

/* Angle of the point pos as seen from the line cent1-cent2, used to place the target on that line. */
#ifdef NETPLAY
/* NET-WOXL: the locals follow the /Od frame layout (tools/odslots.py), and the angle keeps its empty else branch. */
#define NET_TARGET_ANGLE 1

void GetTargetPos(VECTOR *result, VECTOR pos, Vector cent1, Vector cent2)
{
    VECTOR a;
    VECTOR b;
    VECTOR c;
    int aMagSq;
    int cMagSq;
    register int aLength;
    int arcAngle;
    int bMagSq;
    register int cMagnitude;
    register int bLength;
    int cosAlpha;

    a.vx = pos.vx - cent1.vx;
    a.vy = pos.vy - cent1.vy;
    a.vz = pos.vz - cent1.vz;
    b.vx = cent2.vx - cent1.vx;
    b.vy = cent2.vy - cent1.vy;
    b.vz = cent2.vz - cent1.vz;
    c.vx = pos.vx - cent2.vx;
    c.vy = pos.vy - cent2.vy;
    c.vz = pos.vz - cent2.vz;
    aLength = GetMagnitude(a.vx, a.vy, a.vz);
    bLength = GetMagnitude(b.vx, b.vy, b.vz);
    cMagnitude = GetMagnitude(c.vx, c.vy, c.vz);
    aMagSq = aLength * aLength;
    bMagSq = bLength * bLength;
    cMagSq = cMagnitude * cMagnitude;
    if ((aLength != 0) && (bLength != 0)) {
        cosAlpha = (((-bMagSq) - (aMagSq + cMagSq)) << 12) / ((aLength * bLength) << 1);
        if (NET_TARGET_ANGLE) {
            arcAngle = arccos(cosAlpha) >> 2;
        } else {
        }
        cMagnitude = (rcos(arcAngle) * aLength) >> 12;
        result->vx = cent1.vx - ((b.vx * cMagnitude) / bLength);
        result->vy = cent1.vy - ((b.vy * cMagnitude) / bLength);
        result->vz = cent1.vz - ((b.vz * cMagnitude) / bLength);
    }
}
#else
void GetTargetPos(VECTOR *result, VECTOR pos, Vector cent1, Vector cent2)
{
    register int a_mag;
    register int b_mag;
    register int c_mag;
    VECTOR a;
    VECTOR b;
    VECTOR c;
    int a_sq;
    int b_sq;
    int c_sq;
    int cos_alpha;
    int angle;

    a.vx = pos.vx - cent1.vx;
    a.vy = pos.vy - cent1.vy;
    a.vz = pos.vz - cent1.vz;
    b.vx = cent2.vx - cent1.vx;
    b.vy = cent2.vy - cent1.vy;
    b.vz = cent2.vz - cent1.vz;
    c.vx = pos.vx - cent2.vx;
    c.vy = pos.vy - cent2.vy;
    c.vz = pos.vz - cent2.vz;
    a_mag = GetMagnitude(a.vx, a.vy, a.vz);
    b_mag = GetMagnitude(b.vx, b.vy, b.vz);
    c_mag = GetMagnitude(c.vx, c.vy, c.vz);
    a_sq = a_mag * a_mag;
    b_sq = b_mag * b_mag;
    c_sq = c_mag * c_mag;
    if ((a_mag != 0) && (b_mag != 0)) {
        cos_alpha = (((-b_sq) - (a_sq + c_sq)) << 12) / ((a_mag * b_mag) << 1);
        angle = arccos(cos_alpha) >> 2;
        c_mag = (rcos(angle) * a_mag) >> 12;
        result->vx = cent1.vx - ((b.vx * c_mag) / b_mag);
        result->vy = cent1.vy - ((b.vy * c_mag) / b_mag);
        result->vz = cent1.vz - ((b.vz * c_mag) / b_mag);
    }
}
#endif



/* NET-WOXL keeps the setup constant switch: its empty else branch leaves the jumps the exe has. */
#define NET_SETUP_LEVEL 1

void SetUnitVectors(ShipData *shipIndex)
{
    short sinPitch;
    Face *facePtr;
    short sinHdg;
    VECTOR shipSectVec;
    short i;
    VECTOR vertexDelta;
    short sinTilt;
    short cosRz;
    short cosPitch;
    short cosHeading;
    int toFace;
    ShipData *curShip;

    for (i = 0; i < 15; i++) {
        if (NET_SETUP_LEVEL) {
            curShip = &shipIndex[i];
        } else {
        }
        if (NET_SETUP_LEVEL) {
            sinHdg = RotSin(curShip->hdg);
        } else {
        }
        if (NET_SETUP_LEVEL) {
            cosHeading = RotCos(curShip->hdg);
        } else {
        }
        if (NET_SETUP_LEVEL) {
            sinPitch = RotSin(curShip->pitch);
        } else {
        }
        if (NET_SETUP_LEVEL) {
            cosPitch = RotCos(curShip->pitch);
        } else {
        }
        if (NET_SETUP_LEVEL) {
            sinTilt = RotSin(curShip->roll);
        } else {
        }
        cosRz = RotCos(curShip->roll);
        curShip->unitVecNose.vx = -((sinHdg * cosPitch) >> 12);
        curShip->unitVecNose.vy = -sinPitch;
        curShip->unitVecNose.vz = (cosHeading * cosPitch) >> 12;
        curShip->unitVecWing.vx = ((cosHeading * cosRz) >> 12) + ((((sinHdg * sinTilt) >> 12) * sinPitch) >> 12);
        curShip->unitVecWing.vy = (-(sinTilt * cosPitch)) >> 12;
        curShip->unitVecWing.vz = ((sinHdg * cosRz) >> 12) - ((((cosHeading * sinPitch) >> 12) * sinTilt) >> 12);
        OuterProduct12(&curShip->unitVecNose, &curShip->unitVecWing, &curShip->unitVecFin);
        VectorNormal(&curShip->unitVecFin, &curShip->unitVecFin);
        curShip->prevShipSect = curShip->nearTrkSect;
        SectionSearch(curShip, i);
        curShip->prevSecNo = curShip->prevShipSect->secNo;
        curShip->nearSecNo = curShip->nearTrkSect->secNo;

        facePtr = track->faces + curShip->nearTrkSect->faceStart;
        while ((facePtr->flags & TRACK_BASE) == 0) {
            facePtr++;
        }

        vertexDelta.vx = track->vertices[facePtr->vertex[0]].vx - track->vertices[facePtr->vertex[1]].vx;
        vertexDelta.vy = track->vertices[facePtr->vertex[0]].vy - track->vertices[facePtr->vertex[1]].vy;
        vertexDelta.vz = track->vertices[facePtr->vertex[0]].vz - track->vertices[facePtr->vertex[1]].vz;
        shipSectVec.vx = curShip->nearTrkSect->centre.vx - curShip->ppivot.vx;
        shipSectVec.vy = curShip->nearTrkSect->centre.vy - curShip->ppivot.vy;
        shipSectVec.vz = curShip->nearTrkSect->centre.vz - curShip->ppivot.vz;
        toFace = ((vertexDelta.vy * shipSectVec.vy) + (vertexDelta.vz * shipSectVec.vz)) + (vertexDelta.vx * shipSectVec.vx);
        if (toFace > 0) {
            curShip->attr |= LSIDE;
        } else {
            curShip->attr &= ~LSIDE;
        }
    }
}


extern int gameType;
extern ConfigData *tempGameData;

/* Equivalent face-flag tests with a different short-circuit branch layout. */
#ifdef EQUIV
#ifdef NETPLAY
int SetTrackAttributes(TrackSection *section, short sectionNo, WeaponGrid *weaponGrid, int noWeaponGrids, short *specialCount, SpecialGrid *specialGrid)
{
    Face *facePtr;

    facePtr = track->faces + section->faceStart;
    while (!(facePtr->flags & TRACK_BASE)) {
        facePtr++;
    }

    if (facePtr->flags & WEAPON_GRID_LEFT) {
        if (gameType != 1 && tempGameData->weaponsOnSigned) {
            facePtr->red = 255;
            facePtr->green = 35;
            facePtr->blue = 117;
        } else if (!tempGameData->weaponsOnSigned) {
            /* Empty then branch: the exe tests gameType != 2 and jumps over the colour store. */
            if (gameType != 2) {
            } else {
                facePtr->red = 255;
                facePtr->green = 35;
                facePtr->blue = 117;
            }
        }
        weaponGrid[noWeaponGrids].section = section;
        weaponGrid[noWeaponGrids].count = 0;
        weaponGrid[noWeaponGrids].attr = 0;
        weaponGrid[noWeaponGrids].attr |= WEAPON_GRID_LEFT;
        noWeaponGrids++;
    }
    if (facePtr->flags & SPEED_UP) {
        facePtr->red = 35;
        facePtr->green = 35;
        facePtr->blue = 255;
    }
    if (facePtr->flags & 64) {
        facePtr->red = 128;
        facePtr->green = 0;
        facePtr->blue = 0;
    }
    if (facePtr->flags & CHECKPOINT) {
        specialGrid[*specialCount].count = 10;
        specialGrid[*specialCount].section = section->secNo;
        specialGrid[*specialCount].attr = 0;
        facePtr->red = 255;
        facePtr->green = 255;
        facePtr->blue = 255;
        (*specialCount)++;
    }
    facePtr++;
    if (facePtr->flags & WEAPON_GRID_RIGHT) {
        if (gameType != 1 && tempGameData->weaponsOnSigned) {
            facePtr->red = 84;
            facePtr->green = 238;
            facePtr->blue = 117;
        } else if (!tempGameData->weaponsOnSigned) {
            /* Empty then branch: the exe tests gameType != 2 and jumps over the colour store. */
            if (gameType != 2) {
            } else {
                facePtr->red = 84;
                facePtr->green = 238;
                facePtr->blue = 117;
            }
        }
        weaponGrid[noWeaponGrids].section = section;
        weaponGrid[noWeaponGrids].count = 0;
        weaponGrid[noWeaponGrids].attr = 0;
        weaponGrid[noWeaponGrids].attr |= WEAPON_GRID_RIGHT;
        noWeaponGrids++;
    } else if (facePtr->flags & SPEED_UP) {
        facePtr->red = 35;
        facePtr->green = 35;
        facePtr->blue = 255;
    }
    if (facePtr->flags & 64) {
        facePtr->red = 128;
        facePtr->green = 0;
        facePtr->blue = 0;
    }
    if (facePtr->flags & CHECKPOINT) {
        facePtr->red = 255;
        facePtr->green = 255;
        facePtr->blue = 255;
    }
    return noWeaponGrids;
}
#else
int SetTrackAttributes(TrackSection *section, short sectionNo, WeaponGrid *weaponGrid, int noWeaponGrids, short *specialCount, SpecialGrid *specialGrid)
{
    Face *facePtr;
    short gridIndex;

    facePtr = track->faces + section->faceStart;
    while (!(facePtr->flags & TRACK_BASE)) {
        facePtr++;
    }

    if (facePtr->flags & WEAPON_GRID_LEFT) {
        char weaponsOn = tempGameData->weaponsOn;
        if ((gameType != 1 && weaponsOn) || gameType == 2) {
            facePtr->red = 255;
            facePtr->green = 35;
            facePtr->blue = 117;
        }
        weaponGrid[noWeaponGrids].section = section;
        weaponGrid[noWeaponGrids].count = 0;
        weaponGrid[noWeaponGrids].attr = WEAPON_GRID_LEFT;
        noWeaponGrids++;
    }
    if (facePtr->flags & SPEED_UP) {
        facePtr->red = 35;
        facePtr->green = 35;
        facePtr->blue = 255;
    }
    if (facePtr->flags & 64) {
        facePtr->red = 128;
        facePtr->green = 0;
        facePtr->blue = 0;
    }
    if (facePtr->flags & CHECKPOINT) {
        gridIndex = *specialCount;
        facePtr->red = 255;
        facePtr->green = 255;
        facePtr->blue = 255;
        specialGrid[gridIndex].count = 10;
        specialGrid[gridIndex].section = section->secNo;
        specialGrid[gridIndex].attr = 0;
        *specialCount = gridIndex + 1;
    }
    facePtr++;
    if (facePtr->flags & WEAPON_GRID_RIGHT) {
        char weaponsOn = tempGameData->weaponsOn;
        if ((gameType != 1 && weaponsOn) || gameType == 2) {
            facePtr->red = 84;
            facePtr->green = 238;
            facePtr->blue = 117;
        }
        weaponGrid[noWeaponGrids].section = section;
        weaponGrid[noWeaponGrids].count = 0;
        weaponGrid[noWeaponGrids].attr = WEAPON_GRID_RIGHT;
        noWeaponGrids++;
    } else if (facePtr->flags & SPEED_UP) {
        facePtr->red = 35;
        facePtr->green = 35;
        facePtr->blue = 255;
    }
    if (facePtr->flags & 64) {
        facePtr->red = 128;
        facePtr->green = 0;
        facePtr->blue = 0;
    }
    if (facePtr->flags & CHECKPOINT) {
        facePtr->red = 255;
        facePtr->green = 255;
        facePtr->blue = 255;
    }
    return noWeaponGrids;
}
#endif
#endif
