#include "functions.h"
#include "race_globals.h"

#ifdef NETPLAY
#include <math.h>
#pragma intrinsic(abs)
#pragma intrinsic(sqrt)
#endif

extern int collideMinAlpha;
extern SParticle *particleSlots;

void doShipShipCollision(int shipNo, ShipData *shipIndex, Object **shipShapes)
{
    int infrontShip;

    infrontShip = getShipInfrontLAP(shipIndex, shipNo);
    if (infrontShip != -1)
        Ship2ShipCollision(shipIndex, shipShapes, infrontShip, shipNo);
}

#ifdef EQUIV
/* Velocity temporary allocation differs. */
#ifdef NETPLAY
void Ship2ShipCollision(ShipData *shipIndex, Object **shipShapes, int i, int j)
{
    VECTOR sparkPos;
    VECTOR firstResponse;
    VECTOR soundPos;
    VECTOR interShipSeparation;
    VECTOR shipDistance;
    int collisionExtent;
    VECTOR centreVelocity;
    int shipCollision;
    VECTOR shipCollisionResponse;
    int massOther;
    int firstMass;

    shipDistance.vx = (shipIndex[i].ppivot.vx - shipIndex[j].ppivot.vx) >> 3;
    shipDistance.vy = (shipIndex[i].ppivot.vy - shipIndex[j].ppivot.vy) >> 3;
    shipDistance.vz = (shipIndex[i].ppivot.vz - shipIndex[j].ppivot.vz) >> 3;
    collisionExtent = GetMagnitude(shipDistance.vx, shipDistance.vy, shipDistance.vz);
    if (collisionExtent < 120) {
        shipCollision = collideLineToPlane(j, i, shipShapes, shipIndex);
        if (shipCollision == 1) {
            if (j == ownShip && !networkGame)
                firstMass = shipIndex[j].mass;
            else
                firstMass = shipIndex[j].mass * 2;
            if (i == ownShip && !networkGame)
                massOther = shipIndex[i].mass;
            else
                massOther = shipIndex[i].mass * 2;
            centreVelocity.vx = (shipIndex[i].vpivot.vx * massOther + shipIndex[j].vpivot.vx * firstMass) / (firstMass + massOther);
            centreVelocity.vy = (shipIndex[i].vpivot.vy * massOther + shipIndex[j].vpivot.vy * firstMass) / (firstMass + massOther);
            centreVelocity.vz = (shipIndex[i].vpivot.vz * massOther + shipIndex[j].vpivot.vz * firstMass) / (firstMass + massOther);
            shipCollisionResponse.vx = (centreVelocity.vx - shipIndex[j].vpivot.vx) >> 1;
            shipCollisionResponse.vy = (centreVelocity.vy - shipIndex[j].vpivot.vy) >> 1;
            shipCollisionResponse.vz = (centreVelocity.vz - shipIndex[j].vpivot.vz) >> 1;
            firstResponse.vx = (centreVelocity.vx - shipIndex[i].vpivot.vx) >> 1;
            firstResponse.vy = (centreVelocity.vy - shipIndex[i].vpivot.vy) >> 1;
            firstResponse.vz = (centreVelocity.vz - shipIndex[i].vpivot.vz) >> 1;
            shipIndex[j].ppivot.vx -= shipIndex[j].vpivot.vx / 64;
            shipIndex[j].ppivot.vy -= shipIndex[j].vpivot.vy / 64;
            shipIndex[j].ppivot.vz -= shipIndex[j].vpivot.vz / 64;
            shipIndex[i].ppivot.vx -= shipIndex[i].vpivot.vx / 64;
            shipIndex[i].ppivot.vy -= shipIndex[i].vpivot.vy / 64;
            shipIndex[i].ppivot.vz -= shipIndex[i].vpivot.vz / 64;
            shipIndex[j].vpivot.vx = centreVelocity.vx + shipCollisionResponse.vx;
            shipIndex[j].vpivot.vy = centreVelocity.vy + shipCollisionResponse.vy;
            shipIndex[j].vpivot.vz = centreVelocity.vz + shipCollisionResponse.vz;
            shipIndex[i].vpivot.vx = centreVelocity.vx + firstResponse.vx;
            shipIndex[i].vpivot.vy = centreVelocity.vy + firstResponse.vy;
            shipIndex[i].vpivot.vz = centreVelocity.vz + firstResponse.vz;
            interShipSeparation.vx = shipIndex[i].ppivot.vx - shipIndex[j].ppivot.vx;
            interShipSeparation.vy = shipIndex[i].ppivot.vy - shipIndex[j].ppivot.vy;
            interShipSeparation.vz = shipIndex[i].ppivot.vz - shipIndex[j].ppivot.vz;
            shipIndex[i].vpivot.vx += interShipSeparation.vx << 2;
            shipIndex[i].vpivot.vy += interShipSeparation.vy << 2;
            shipIndex[i].vpivot.vz += interShipSeparation.vz << 2;
            shipIndex[j].vpivot.vx -= interShipSeparation.vx << 2;
            shipIndex[j].vpivot.vy -= interShipSeparation.vy << 2;
            shipIndex[j].vpivot.vz -= interShipSeparation.vz << 2;
            shipIndex[j].ppivot.vx += shipIndex[j].vpivot.vx / 64;
            shipIndex[j].ppivot.vy += shipIndex[j].vpivot.vy / 64;
            shipIndex[j].ppivot.vz += shipIndex[j].vpivot.vz / 64;
            shipIndex[i].ppivot.vx += shipIndex[i].vpivot.vx / 64;
            shipIndex[i].ppivot.vy += shipIndex[i].vpivot.vy / 64;
            shipIndex[i].ppivot.vz += shipIndex[i].vpivot.vz / 64;
            if (!(shipIndex[i].attr & SHIP_COLL)) {
                if (!(shipIndex[j].attr & SHIP_COLL)) {
                    soundPos.vx = (shipIndex[j].ppivot.vx + shipIndex[i].ppivot.vx) >> 1;
                    soundPos.vy = (shipIndex[j].ppivot.vy + shipIndex[i].ppivot.vy) >> 1;
                    soundPos.vz = (shipIndex[j].ppivot.vz + shipIndex[i].ppivot.vz) >> 1;
                    if (i == ownShip || j == ownShip)
                        PlayDirectionalNote(0, 0, 30, soundPos);
#ifdef NETPLAY
                    DamageCollide(130, i, -1);
#else
                    DamageCollide(130, i);
#endif
#ifdef NETPLAY
                    DamageCollide(130, j, -1);
#else
                    DamageCollide(130, j);
#endif
                    sparkPos.vx = shipIndex[i].vpivot.vx >> 6;
                    sparkPos.vy = shipIndex[i].vpivot.vy >> 6;
                    sparkPos.vz = shipIndex[i].vpivot.vz >> 6;
                    AddRandomParticles(particleSlots, 8, &soundPos, &sparkPos);
                }
            }
            shipIndex[i].attr |= SHIP_COLL;
            shipIndex[j].attr |= SHIP_COLL;
        }
    } else {
        shipIndex[i].attr &= ~SHIP_COLL;
        shipIndex[j].attr &= ~SHIP_COLL;
    }
}
#else
void Ship2ShipCollision(ShipData *shipIndex, Object **shipShapes, int i, int j)
{
    int mag;
    VECTOR distance;
    int collide;
    VECTOR vc;
    int ownMass;
    int remMass;
    VECTOR ship2ship;
    VECTOR ship1React;
    VECTOR ship2React;
    VECTOR soundPos;
    VECTOR sparkPos;

    distance.vx = (shipIndex[i].ppivot.vx - shipIndex[j].ppivot.vx) >> 3;
    distance.vy = (shipIndex[i].ppivot.vy - shipIndex[j].ppivot.vy) >> 3;
    distance.vz = (shipIndex[i].ppivot.vz - shipIndex[j].ppivot.vz) >> 3;
    mag = GetMagnitude(distance.vx, distance.vy, distance.vz);
    if (mag < 120) {
        collide = collideLineToPlane(j, i, shipShapes, shipIndex);
        if (collide == 1) {
            if (j == ownShip)
                ownMass = shipIndex[j].mass;
            else
                ownMass = shipIndex[j].mass * 2;
            if (i == ownShip)
                remMass = shipIndex[i].mass;
            else
                remMass = shipIndex[i].mass * 2;
            vc.vx = (shipIndex[j].vpivot.vx * ownMass + shipIndex[i].vpivot.vx * remMass) / (ownMass + remMass);
            vc.vy = (shipIndex[j].vpivot.vy * ownMass + shipIndex[i].vpivot.vy * remMass) / (ownMass + remMass);
            vc.vz = (shipIndex[j].vpivot.vz * ownMass + shipIndex[i].vpivot.vz * remMass) / (ownMass + remMass);
            ship1React.vx = (vc.vx - shipIndex[j].vpivot.vx) >> 1;
            ship1React.vy = (vc.vy - shipIndex[j].vpivot.vy) >> 1;
            ship1React.vz = (vc.vz - shipIndex[j].vpivot.vz) >> 1;
            ship2React.vx = (vc.vx - shipIndex[i].vpivot.vx) >> 1;
            ship2React.vy = (vc.vy - shipIndex[i].vpivot.vy) >> 1;
            ship2React.vz = (vc.vz - shipIndex[i].vpivot.vz) >> 1;
            shipIndex[j].ppivot.vx -= shipIndex[j].vpivot.vx / 64;
            shipIndex[j].ppivot.vy -= shipIndex[j].vpivot.vy / 64;
            shipIndex[j].ppivot.vz -= shipIndex[j].vpivot.vz / 64;
            shipIndex[i].ppivot.vx -= shipIndex[i].vpivot.vx / 64;
            shipIndex[i].ppivot.vy -= shipIndex[i].vpivot.vy / 64;
            shipIndex[i].ppivot.vz -= shipIndex[i].vpivot.vz / 64;
            shipIndex[j].vpivot.vx = vc.vx + ship1React.vx;
            shipIndex[j].vpivot.vy = vc.vy + ship1React.vy;
            shipIndex[j].vpivot.vz = vc.vz + ship1React.vz;
            shipIndex[i].vpivot.vx = vc.vx + ship2React.vx;
            shipIndex[i].vpivot.vy = vc.vy + ship2React.vy;
            shipIndex[i].vpivot.vz = vc.vz + ship2React.vz;
            ship2ship.vx = shipIndex[i].ppivot.vx - shipIndex[j].ppivot.vx;
            ship2ship.vy = shipIndex[i].ppivot.vy - shipIndex[j].ppivot.vy;
            ship2ship.vz = shipIndex[i].ppivot.vz - shipIndex[j].ppivot.vz;
            shipIndex[i].vpivot.vx += ship2ship.vx << 2;
            shipIndex[i].vpivot.vy += ship2ship.vy << 2;
            shipIndex[i].vpivot.vz += ship2ship.vz << 2;
            shipIndex[j].vpivot.vx -= ship2ship.vx << 2;
            shipIndex[j].vpivot.vy -= ship2ship.vy << 2;
            shipIndex[j].vpivot.vz -= ship2ship.vz << 2;
            shipIndex[j].ppivot.vx += shipIndex[j].vpivot.vx / 64;
            shipIndex[j].ppivot.vy += shipIndex[j].vpivot.vy / 64;
            shipIndex[j].ppivot.vz += shipIndex[j].vpivot.vz / 64;
            shipIndex[i].ppivot.vx += shipIndex[i].vpivot.vx / 64;
            shipIndex[i].ppivot.vy += shipIndex[i].vpivot.vy / 64;
            shipIndex[i].ppivot.vz += shipIndex[i].vpivot.vz / 64;
            if (!(shipIndex[i].attr & SHIP_COLL)) {
                if (!(shipIndex[j].attr & SHIP_COLL)) {
                    soundPos.vx = (shipIndex[i].ppivot.vx + shipIndex[j].ppivot.vx) >> 1;
                    soundPos.vy = (shipIndex[i].ppivot.vy + shipIndex[j].ppivot.vy) >> 1;
                    soundPos.vz = (shipIndex[i].ppivot.vz + shipIndex[j].ppivot.vz) >> 1;
                    PlayDirectionalNote(0, 0, 30, soundPos);
#ifdef NETPLAY
                    DamageCollide(130, i, -1);
#else
                    DamageCollide(130, i);
#endif
#ifdef NETPLAY
                    DamageCollide(130, j, -1);
#else
                    DamageCollide(130, j);
#endif
                    sparkPos.vx = shipIndex[i].vpivot.vx >> 6;
                    sparkPos.vy = shipIndex[i].vpivot.vy >> 6;
                    sparkPos.vz = shipIndex[i].vpivot.vz >> 6;
                    AddRandomParticles(particleSlots, 8, &soundPos, &sparkPos);
                }
            }
            shipIndex[i].attr |= SHIP_COLL;
            shipIndex[j].attr |= SHIP_COLL;
        }
    } else {
        shipIndex[i].attr &= ~SHIP_COLL;
        shipIndex[j].attr &= ~SHIP_COLL;
    }
}
#endif

#endif

#ifdef EQUIV
/* NET local names reproduce its /Od frame layout. */
#ifdef NETPLAY
int collideLineToPlane(int shipNo1, int shipNo2, Object **shipShapes, ShipData *shipIndex)
{
    VECTOR deltaVector;
    VECTOR a;
    int edgeCount;
    short *pointIndices;
    int scaleFactor;
    VECTOR planeNormal;
    VECTOR pointTwo;
    VECTOR b;
    VECTOR pointOne;
    VECTOR DC;
    VECTOR direction;
    short primitiveCount;
    VECTOR thirdVertexValue;
    int alphaValue;
    int angleSumValue;
    VECTOR AB;
    VECTOR edgeStartData;
    VECTOR P1P2Data;
    Object *shapeIndex;
    VECTOR pointDShape;
    VECTOR p3;
    VECTOR AC;
    VECTOR triangleEdge13Value;
    VECTOR AD;
    int high2Point;
    int high1Index;
    int primitiveIndexVector;
    VECTOR scaledEdge;
    VECTOR firstFactorValue;
    VECTOR relativePoint2Edge;
    int lowRes2Local;
    VECTOR offset0Location;
    int lowShape1Value;
    VECTOR BDIndex;
    Poly primitiveValue;
    VECTOR intersectionPoint;
    VECTOR BC;

    primitiveIndexVector = shipNo1;
    shapeIndex = shipShapes[shipIndex[primitiveIndexVector].highResTableIndex];
    SetSkeletonPosition(shapeIndex->skeleton, shipIndex[primitiveIndexVector].ppivot.vx, shipIndex[primitiveIndexVector].ppivot.vy, shipIndex[primitiveIndexVector].ppivot.vz);
    SetSkeletonDirectionHPR(shapeIndex->skeleton, shipIndex[primitiveIndexVector].hdg, shipIndex[primitiveIndexVector].pitch, shipIndex[primitiveIndexVector].roll);
    primitiveIndexVector = shipNo2;
    shapeIndex = shipShapes[shipIndex[primitiveIndexVector].highResTableIndex];
    SetSkeletonPosition(shapeIndex->skeleton, shipIndex[primitiveIndexVector].ppivot.vx, shipIndex[primitiveIndexVector].ppivot.vy, shipIndex[primitiveIndexVector].ppivot.vz);
    SetSkeletonDirectionHPR(shapeIndex->skeleton, shipIndex[primitiveIndexVector].hdg, shipIndex[primitiveIndexVector].pitch, shipIndex[primitiveIndexVector].roll);
    high1Index = shipIndex[shipNo1].highResTableIndex;
    lowShape1Value = shipIndex[shipNo1].lowResTableIndex;
    high2Point = shipIndex[shipNo2].highResTableIndex;
    lowRes2Local = shipIndex[shipNo2].lowResTableIndex;

    getWorldPoint(&a, shipShapes[high2Point], shipShapes[lowRes2Local], 0);
    getWorldPoint(&b, shipShapes[high2Point], shipShapes[lowRes2Local], 1);
    getWorldPoint(&thirdVertexValue, shipShapes[high2Point], shipShapes[lowRes2Local], 2);
    getWorldPoint(&pointDShape, shipShapes[high2Point], shipShapes[lowRes2Local], 3);
    MinusVector(a, thirdVertexValue, &AC);
    MinusVector(b, thirdVertexValue, &BC);
    MinusVector(pointDShape, thirdVertexValue, &DC);
    MinusVector(a, b, &AB);
    MinusVector(b, pointDShape, &BDIndex);
    MinusVector(a, pointDShape, &AD);

    primitiveValue.prim = shipShapes[lowRes2Local]->primitives;
    primitiveCount = shipShapes[lowRes2Local]->primitiveCount;
    for (primitiveIndexVector = 0; primitiveIndexVector < primitiveCount; primitiveIndexVector++) {
        switch (primitiveValue.prim->type) {
        case TypeF3:
            pointIndices = primitiveValue.f3->coords;
            getWorldPoint(&pointOne, shipShapes[high1Index], shipShapes[lowShape1Value], pointIndices[0]);
            getWorldPoint(&pointTwo, shipShapes[high1Index], shipShapes[lowShape1Value], pointIndices[1]);
            getWorldPoint(&p3, shipShapes[high1Index], shipShapes[lowShape1Value], pointIndices[2]);
            primitiveValue.f3++;
            break;
        case TypeG3:
            pointIndices = primitiveValue.g3->coords;
            getWorldPoint(&pointOne, shipShapes[high1Index], shipShapes[lowShape1Value], pointIndices[0]);
            getWorldPoint(&pointTwo, shipShapes[high1Index], shipShapes[lowShape1Value], pointIndices[1]);
            getWorldPoint(&p3, shipShapes[high1Index], shipShapes[lowShape1Value], pointIndices[2]);
            primitiveValue.g3++;
            break;
        case TypeFT3:
            pointIndices = primitiveValue.ft3->coords;
            getWorldPoint(&pointOne, shipShapes[high1Index], shipShapes[lowShape1Value], pointIndices[0]);
            getWorldPoint(&pointTwo, shipShapes[high1Index], shipShapes[lowShape1Value], pointIndices[1]);
            getWorldPoint(&p3, shipShapes[high1Index], shipShapes[lowShape1Value], pointIndices[2]);
            primitiveValue.ft3++;
            break;
        case TypeGT3:
            pointIndices = primitiveValue.gt3->coords;
            getWorldPoint(&pointOne, shipShapes[high1Index], shipShapes[lowShape1Value], pointIndices[0]);
            getWorldPoint(&pointTwo, shipShapes[high1Index], shipShapes[lowShape1Value], pointIndices[1]);
            getWorldPoint(&p3, shipShapes[high1Index], shipShapes[lowShape1Value], pointIndices[2]);
            primitiveValue.gt3++;
            break;
        default:
            break;
        }
        MinusVector(pointOne, pointTwo, &P1P2Data);
        MinusVector(pointOne, p3, &triangleEdge13Value);
        CrossProduct(&planeNormal, P1P2Data, triangleEdge13Value);
        for (edgeCount = 0; edgeCount < 6; edgeCount++) {
            switch (edgeCount) {
            case 0:
                assign(&edgeStartData, b);
                assign(&direction, BC);
                break;
            case 1:
                assign(&edgeStartData, a);
                assign(&direction, AC);
                break;
            case 2:
                assign(&edgeStartData, pointDShape);
                assign(&direction, DC);
                break;
            case 3:
                assign(&edgeStartData, a);
                assign(&direction, AB);
                break;
            case 4:
                assign(&edgeStartData, a);
                assign(&direction, AD);
                break;
            case 5:
                assign(&edgeStartData, b);
                assign(&direction, BDIndex);
                break;
            }
            MinusVector(edgeStartData, pointOne, &firstFactorValue);
            alphaValue = ScalarProduct(firstFactorValue, planeNormal) << 12;
            scaleFactor = ScalarProduct(direction, planeNormal);
            if (scaleFactor != 0) {
                alphaValue /= scaleFactor;
                scaledEdge.vx = (alphaValue * direction.vx) >> 12;
                scaledEdge.vy = (alphaValue * direction.vy) >> 12;
                scaledEdge.vz = (alphaValue * direction.vz) >> 12;
                intersectionPoint.vx = edgeStartData.vx + scaledEdge.vx;
                intersectionPoint.vy = edgeStartData.vy + scaledEdge.vy;
                intersectionPoint.vz = edgeStartData.vz + scaledEdge.vz;
                if (alphaValue >= 0 && alphaValue <= 4096) {
                    offset0Location.vx = intersectionPoint.vx - pointOne.vx;
                    offset0Location.vy = intersectionPoint.vy - pointOne.vy;
                    offset0Location.vz = intersectionPoint.vz - pointOne.vz;
                    relativePoint2Edge.vx = intersectionPoint.vx - pointTwo.vx;
                    relativePoint2Edge.vy = intersectionPoint.vy - pointTwo.vy;
                    relativePoint2Edge.vz = intersectionPoint.vz - pointTwo.vz;
                    deltaVector.vx = intersectionPoint.vx - p3.vx;
                    deltaVector.vy = intersectionPoint.vy - p3.vy;
                    deltaVector.vz = intersectionPoint.vz - p3.vz;
                    angleSumValue = GetAng(offset0Location, relativePoint2Edge);
                    angleSumValue += GetAng(relativePoint2Edge, deltaVector);
                    angleSumValue += GetAng(deltaVector, offset0Location);
                    if (angleSumValue >= 30000)
                        return 1;
                }
            }
        }
    }
    return 0;
}
#else
int collideLineToPlane(int shipNo1, int shipNo2, Object **shipShapes, ShipData *shipIndex)
{
    int i;
    int count;
    Poly poly;
    short *coords;
    short primitiveCount;
    VECTOR p1, p2, p3;
    VECTOR a, b, c, d;
    int factor2;
    VECTOR factor1;
    VECTOR term1;
    VECTOR intersection;
    int alpha;
    VECTOR DC, BC, AC;
    VECTOR BD, AB, AD;
    VECTOR start;
    VECTOR direction;
    VECTOR normal;
    VECTOR P1P2, P1P3;
    VECTOR vec0, vec1, vec2;
    int angle;
    int high1, low1, low2, high2;
    Object *shape;
    ShipData *ship;

    ship = &shipIndex[shipNo1];
    shape = shipShapes[ship->highResTableIndex];
    SetSkeletonPosition(shape->skeleton, ship->ppivot.vx, ship->ppivot.vy, ship->ppivot.vz);
    SetSkeletonDirectionHPR(shape->skeleton, ship->hdg, ship->pitch, ship->roll);
    ship = &shipIndex[shipNo2];
    shape = shipShapes[ship->highResTableIndex];
    SetSkeletonPosition(shape->skeleton, ship->ppivot.vx, ship->ppivot.vy, ship->ppivot.vz);
    SetSkeletonDirectionHPR(shape->skeleton, ship->hdg, ship->pitch, ship->roll);
    high2 = ship->highResTableIndex;
    low2 = ship->lowResTableIndex;
    high1 = shipIndex[shipNo1].highResTableIndex;
    low1 = shipIndex[shipNo1].lowResTableIndex;

    getWorldPoint(&a, shipShapes[high2], shipShapes[low2], 0);
    getWorldPoint(&b, shipShapes[high2], shipShapes[low2], 1);
    getWorldPoint(&c, shipShapes[high2], shipShapes[low2], 2);
    getWorldPoint(&d, shipShapes[high2], shipShapes[low2], 3);
    MinusVector(a, c, &AC);
    MinusVector(b, c, &BC);
    MinusVector(d, c, &DC);
    MinusVector(a, b, &AB);
    MinusVector(b, d, &BD);
    MinusVector(a, d, &AD);

    poly.prim = shipShapes[low2]->primitives;
    primitiveCount = shipShapes[low2]->primitiveCount;
    for (i = 0; i < primitiveCount; i++) {
        switch (poly.prim->type) {
        case TypeF3:
            coords = poly.f3->coords;
            getWorldPoint(&p1, shipShapes[high1], shipShapes[low1], coords[0]);
            getWorldPoint(&p2, shipShapes[high1], shipShapes[low1], coords[1]);
            getWorldPoint(&p3, shipShapes[high1], shipShapes[low1], coords[2]);
            poly.f3++;
            break;
        case TypeG3:
            coords = poly.g3->coords;
            getWorldPoint(&p1, shipShapes[high1], shipShapes[low1], coords[0]);
            getWorldPoint(&p2, shipShapes[high1], shipShapes[low1], coords[1]);
            getWorldPoint(&p3, shipShapes[high1], shipShapes[low1], coords[2]);
            poly.g3++;
            break;
        case TypeFT3:
            coords = poly.ft3->coords;
            getWorldPoint(&p1, shipShapes[high1], shipShapes[low1], coords[0]);
            getWorldPoint(&p2, shipShapes[high1], shipShapes[low1], coords[1]);
            getWorldPoint(&p3, shipShapes[high1], shipShapes[low1], coords[2]);
            poly.ft3++;
            break;
        case TypeGT3:
            coords = poly.gt3->coords;
            getWorldPoint(&p1, shipShapes[high1], shipShapes[low1], coords[0]);
            getWorldPoint(&p2, shipShapes[high1], shipShapes[low1], coords[1]);
            getWorldPoint(&p3, shipShapes[high1], shipShapes[low1], coords[2]);
            poly.gt3++;
            break;
        default:
            break;
        }
        MinusVector(p1, p2, &P1P2);
        MinusVector(p1, p3, &P1P3);
        CrossProduct(&normal, P1P2, P1P3);
        for (count = 0; count < 6; count++) {
            switch (count) {
            case 0:
                assign(&start, b);
                assign(&direction, BC);
                break;
            case 1:
                assign(&start, a);
                assign(&direction, AC);
                break;
            case 2:
                assign(&start, d);
                assign(&direction, DC);
                break;
            case 3:
                assign(&start, a);
                assign(&direction, AB);
                break;
            case 4:
                assign(&start, a);
                assign(&direction, AD);
                break;
            case 5:
                assign(&start, b);
                assign(&direction, BD);
                break;
            }
            MinusVector(start, p1, &factor1);
            alpha = ScalarProduct(factor1, normal) << 12;
            factor2 = ScalarProduct(direction, normal);
            if (factor2 != 0) {
                alpha /= factor2;
                term1.vx = (alpha * direction.vx) >> 12;
                term1.vy = (alpha * direction.vy) >> 12;
                term1.vz = (alpha * direction.vz) >> 12;
                intersection.vx = start.vx + term1.vx;
                intersection.vy = start.vy + term1.vy;
                intersection.vz = start.vz + term1.vz;
                if (alpha >= 0 && alpha <= 4096) {
                    vec0.vx = intersection.vx - p1.vx;
                    vec0.vy = intersection.vy - p1.vy;
                    vec0.vz = intersection.vz - p1.vz;
                    vec1.vx = intersection.vx - p2.vx;
                    vec1.vy = intersection.vy - p2.vy;
                    vec1.vz = intersection.vz - p2.vz;
                    vec2.vx = intersection.vx - p3.vx;
                    vec2.vy = intersection.vy - p3.vy;
                    vec2.vz = intersection.vz - p3.vz;
                    angle = GetAng(vec0, vec1);
                    angle += GetAng(vec1, vec2);
                    angle += GetAng(vec2, vec0);
                    if (angle >= 30000)
                        return 1;
                }
            }
        }
    }
    return 0;
}
#endif

#endif

void Ship2TrackBase(ShipData *playerShip, Face *facePtr, int alpha)
{
    if (collideMinAlpha > alpha)
        collideMinAlpha = alpha;
    if (alpha <= 0) {
        PlayDirectionalNote(7, 1, 20, playerShip->ppivot);
        playerShip->vpivot.vx -= playerShip->vpivot.vx >> 3;
        playerShip->vpivot.vy -= playerShip->vpivot.vy >> 3;
        playerShip->vpivot.vz -= playerShip->vpivot.vz >> 3;
        playerShip->vpivot.vx += facePtr->normal.vx;
        playerShip->vpivot.vy += facePtr->normal.vy;
        playerShip->vpivot.vz += facePtr->normal.vz;
        playerShip->vpivot.vx -= (facePtr->normal.vx * alpha) >> 4;
        playerShip->vpivot.vy -= (facePtr->normal.vy * alpha) >> 4;
        playerShip->vpivot.vz -= (facePtr->normal.vz * alpha) >> 4;
    } else if (alpha <= 30) {
        playerShip->vpivot.vx += facePtr->normal.vx;
        playerShip->vpivot.vy += facePtr->normal.vy;
        playerShip->vpivot.vz += facePtr->normal.vz;
    }
}

void DroneShip2TrackBase(ShipData *playerShip, Face *facePtr, int alpha)
{
    if (collideMinAlpha > alpha)
        collideMinAlpha = alpha;
    if (alpha <= 0) {
        playerShip->ppivot.vx += facePtr->normal.vx >> 3;
        playerShip->ppivot.vy += facePtr->normal.vy >> 3;
        playerShip->ppivot.vz += facePtr->normal.vz >> 3;
        return;
    }
    if (alpha <= 30) {
        playerShip->vpivot.vx += facePtr->normal.vx >> 3;
        playerShip->vpivot.vy += facePtr->normal.vy >> 3;
        playerShip->vpivot.vz += facePtr->normal.vz >> 3;
    }
}

#ifdef POWERVR
void Geometry_43BCD0(ShipData *playerShip, Face *facePtr, int alpha)
{
    if (collideMinAlpha > alpha)
        collideMinAlpha = alpha;
    if (alpha <= 0) {
        playerShip->ppivot.vx += facePtr->normal.vx >> 3;
        playerShip->ppivot.vy += facePtr->normal.vy >> 3;
        playerShip->ppivot.vz += facePtr->normal.vz >> 3;
    } else if (alpha <= 30) {
        playerShip->vpivot.vx += facePtr->normal.vx >> 3;
        playerShip->vpivot.vy += facePtr->normal.vy >> 3;
        playerShip->vpivot.vz += facePtr->normal.vz >> 3;
    }
}
#endif

#ifdef EQUIV
extern short beamCollide;

#ifdef NETPLAY
void Ship2Track(ShipData *playerShip, Face *facePtr, Object **shipShapes, short shipNo)
{
    VECTOR facePoint;
    VECTOR wingPos;
    VECTOR toFaceVector;
    Vector sectionDelta;
    int j, alpha;
    Face *facePtr2;
    VECTOR nosePos;
    int downTrack;
    short velocityShift = 4;
    short collided;
    short faceIndex;
    TrackSection *section;
    /* The executable initializes this local but never reads it. */
    short unusedFaceOffset = 0;

    (void)shipShapes;
    collided = 0;
    faceIndex = 0;
    nosePos.vx = playerShip->ppivot.vx + (playerShip->unitVecNose.vx >> 3);
    nosePos.vy = playerShip->ppivot.vy + (playerShip->unitVecNose.vy >> 3);
    nosePos.vz = playerShip->ppivot.vz + (playerShip->unitVecNose.vz >> 3);
    wingPos.vx = playerShip->ppivot.vx - (playerShip->vpivot.vx >> velocityShift)
               - (playerShip->unitVecNose.vx >> velocityShift);
    wingPos.vy = playerShip->ppivot.vy - (playerShip->vpivot.vy >> velocityShift)
               - (playerShip->unitVecNose.vy >> velocityShift);
    wingPos.vz = playerShip->ppivot.vz - (playerShip->vpivot.vz >> velocityShift)
               - (playerShip->unitVecNose.vz >> velocityShift);
    section = playerShip->nearTrkSect->nextSection.ptr;
    sectionDelta.vx = section->centre.vx - playerShip->nearTrkSect->centre.vx;
    sectionDelta.vy = section->centre.vy - playerShip->nearTrkSect->centre.vy;
    sectionDelta.vz = section->centre.vz - playerShip->nearTrkSect->centre.vz;
    downTrack = sectionDelta.vz * playerShip->unitVecNose.vz
              + sectionDelta.vx * playerShip->unitVecNose.vx
              + sectionDelta.vy * playerShip->unitVecNose.vy;
    if (downTrack < 0)
        playerShip->attr &= ~DIRECTION;
    else
        playerShip->attr |= DIRECTION;
    toFaceVector.vx = track->vertices[facePtr->vertex[0]].vx - track->vertices[facePtr->vertex[1]].vx;
    toFaceVector.vy = track->vertices[facePtr->vertex[0]].vy - track->vertices[facePtr->vertex[1]].vy;
    toFaceVector.vz = track->vertices[facePtr->vertex[0]].vz - track->vertices[facePtr->vertex[1]].vz;
    alpha = (playerShip->nearTrkSect->centre.vy - playerShip->ppivot.vy) * toFaceVector.vy
          + (playerShip->nearTrkSect->centre.vz - playerShip->ppivot.vz) * toFaceVector.vz
          + (playerShip->nearTrkSect->centre.vx - playerShip->ppivot.vx) * toFaceVector.vx;
    if (alpha > 0) {
        facePtr = track->faces + playerShip->nearTrkSect->faceStart;
        for (j = 0; j < playerShip->nearTrkSect->faceCount; j++) {
            if (facePtr->flags & TRACK_BASE)
                break;
            playerShip->attr |= 32;
            if (facePtr == track->faces + playerShip->nearTrkSect->faceStart || playerShip->nearTrkSect->junction.ptr != 0) {
                facePoint.vx = track->vertices[facePtr->vertex[0]].vx;
                facePoint.vy = track->vertices[facePtr->vertex[0]].vy;
                facePoint.vz = track->vertices[facePtr->vertex[0]].vz;
                alpha = PlaneLineCollision(facePoint, nosePos, facePtr->normal);
                if (alpha <= 0) {
                    if (playerShip->nearTrkSect->flags & 16) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->nextSection.ptr->faceStart;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                                collided = 1;
                            }
                        }
                    } else if (playerShip->nearTrkSect->prevSection.ptr->junction.index != -1 &&
                           (playerShip->nearTrkSect->prevSection.ptr->junction.ptr->flags & 16)) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->nextSection.ptr->faceStart;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha, shipNo);
                                collided = 1;
                            } else {
                                facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                                if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                    NoseCollision(playerShip, facePtr2, -downTrack, alpha, shipNo);
                                    collided = 1;
                                }
                            }
                        }
                    } else if (playerShip->nearTrkSect->nextSection.ptr->junction.index != -1 &&
                           (playerShip->nearTrkSect->nextSection.ptr->junction.ptr->flags & 16)) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha, shipNo);
                                collided = 1;
                            } else {
                                facePtr2 = track->faces + playerShip->nearTrkSect->nextSection.ptr->junction.ptr->faceStart;
                                if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                    NoseCollision(playerShip, facePtr2, -downTrack, alpha, shipNo);
                                    collided = 1;
                                }
                            }
                        }
                    } else if (playerShip->nearTrkSect->flags & 8) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                                collided = 1;
                            }
                        }
                    } else if (playerShip->nearTrkSect->nextSection.ptr->junction.index != -1 &&
                           (playerShip->nearTrkSect->nextSection.ptr->junction.ptr->flags & 8)) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha, shipNo);
                                collided = 1;
                            }
                        }
                    } else {
                        NoseCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                        collided = 1;
                    }
                }
                wingPos.vx = playerShip->ppivot.vx - (playerShip->unitVecWing.vx >> 4) - (playerShip->unitVecNose.vx >> 4);
                wingPos.vy = playerShip->ppivot.vy - (playerShip->unitVecWing.vy >> 4) - (playerShip->unitVecNose.vy >> 4);
                wingPos.vz = playerShip->ppivot.vz - (playerShip->unitVecWing.vz >> 4) - (playerShip->unitVecNose.vz >> 4);
                alpha = PlaneLineCollision(facePoint, wingPos, facePtr->normal);
                if (alpha <= 0) {
                    if ((playerShip->nearTrkSect->flags & 16) || (playerShip->nearTrkSect->flags & 8)) {
                        if (CheckPolygonCollision(facePtr, wingPos, alpha, 1) == 1) {
                            NewWingCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                            collided = 1;
                        }
                    } else if (collided) {
                        WingCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                    } else {
                        NewWingCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                    }
                }
                wingPos.vx = playerShip->ppivot.vx + (playerShip->unitVecWing.vx >> 4) - (playerShip->unitVecNose.vx >> 4);
                wingPos.vy = playerShip->ppivot.vy + (playerShip->unitVecWing.vy >> 4) - (playerShip->unitVecNose.vy >> 4);
                wingPos.vz = playerShip->ppivot.vz + (playerShip->unitVecWing.vz >> 4) - (playerShip->unitVecNose.vz >> 4);
                alpha = PlaneLineCollision(facePoint, wingPos, facePtr->normal);
                if (alpha <= 0) {
                    if ((playerShip->nearTrkSect->flags & 16) || (playerShip->nearTrkSect->flags & 8)) {
                        if (CheckPolygonCollision(facePtr, wingPos, alpha, 1) == 1) {
                            NewWingCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                            collided = 1;
                        }
                    } else if (collided) {
                        WingCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                    } else {
                        NewWingCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                    }
                }
            }
            facePtr++;
            if (facePtr->flags & TRACK_BASE)
                break;
        }
    } else {
        playerShip->attr &= ~32;
        facePtr = track->faces + playerShip->nearTrkSect->faceStart;
        while (!(facePtr->flags & TRACK_BASE)) {
            facePtr++;
            faceIndex++;
        }
        while (facePtr->flags & TRACK_BASE) {
            facePtr++;
            faceIndex++;
        }
        for (j = faceIndex; j < playerShip->nearTrkSect->faceCount; j++, facePtr++) {
            if (playerShip->nearTrkSect->faceCount > 3 || playerShip->nearTrkSect->junction.ptr != 0) {
                facePoint.vx = track->vertices[facePtr->vertex[0]].vx;
                facePoint.vy = track->vertices[facePtr->vertex[0]].vy;
                facePoint.vz = track->vertices[facePtr->vertex[0]].vz;
                alpha = PlaneLineCollision(facePoint, nosePos, facePtr->normal);
                if (alpha <= 0) {
                    if (playerShip->nearTrkSect->flags & 16) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, downTrack, alpha, shipNo);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->nextSection.ptr->faceStart;
                            facePtr2 += 3;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                                collided = 1;
                            }
                        }
                    } else if (playerShip->nearTrkSect->prevSection.ptr->junction.index != -1 &&
                           (playerShip->nearTrkSect->prevSection.ptr->junction.ptr->flags & 16)) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, downTrack, alpha, shipNo);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            facePtr2 += 3;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha, shipNo);
                                collided = 1;
                            } else {
                                facePtr2 = track->faces + playerShip->nearTrkSect->nextSection.ptr->faceStart;
                                facePtr2 += 3;
                                if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                    NoseCollision(playerShip, facePtr2, -downTrack, alpha, shipNo);
                                    collided = 1;
                                }
                            }
                        }
                    } else if (playerShip->nearTrkSect->nextSection.ptr->junction.index != -1 &&
                           (playerShip->nearTrkSect->nextSection.ptr->junction.ptr->flags & 16)) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, downTrack, alpha, shipNo);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            facePtr2 += 3;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha, shipNo);
                                collided = 1;
                            } else {
                                facePtr2 = track->faces + playerShip->nearTrkSect->nextSection.ptr->junction.ptr->faceStart;
                                facePtr2 += 3;
                                if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                    NoseCollision(playerShip, facePtr2, -downTrack, alpha, shipNo);
                                    collided = 1;
                                }
                            }
                        }
                    } else if (playerShip->nearTrkSect->flags & 8) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            facePtr2 += 3;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha, shipNo);
                                collided = 1;
                            }
                        }
                    } else if (playerShip->nearTrkSect->nextSection.ptr->junction.index != -1 &&
                           (playerShip->nearTrkSect->nextSection.ptr->junction.ptr->flags & 8)) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha, shipNo);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            facePtr2 += 3;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha, shipNo);
                                collided = 1;
                            }
                        }
                    } else {
                        NoseCollision(playerShip, facePtr, downTrack, alpha, shipNo);
                        collided = 1;
                    }
                }
                wingPos.vx = playerShip->ppivot.vx - (playerShip->unitVecWing.vx >> 4) - (playerShip->unitVecNose.vx >> 4);
                wingPos.vy = playerShip->ppivot.vy - (playerShip->unitVecWing.vy >> 4) - (playerShip->unitVecNose.vy >> 4);
                wingPos.vz = playerShip->ppivot.vz - (playerShip->unitVecWing.vz >> 4) - (playerShip->unitVecNose.vz >> 4);
                alpha = PlaneLineCollision(facePoint, wingPos, facePtr->normal);
                if (alpha <= 0) {
                    if ((playerShip->nearTrkSect->flags & 16) || (playerShip->nearTrkSect->flags & 8)) {
                        if (CheckPolygonCollision(facePtr, wingPos, alpha, 1) == 1) {
                            NewWingCollision(playerShip, facePtr, downTrack, alpha, shipNo);
                            collided = 1;
                        }
                    } else if (collided) {
                        WingCollision(playerShip, facePtr, downTrack, alpha, shipNo);
                    } else {
                        NewWingCollision(playerShip, facePtr, downTrack, alpha, shipNo);
                    }
                }
                wingPos.vx = playerShip->ppivot.vx + (playerShip->unitVecWing.vx >> 4) - (playerShip->unitVecNose.vx >> 4);
                wingPos.vy = playerShip->ppivot.vy + (playerShip->unitVecWing.vy >> 4) - (playerShip->unitVecNose.vy >> 4);
                wingPos.vz = playerShip->ppivot.vz + (playerShip->unitVecWing.vz >> 4) - (playerShip->unitVecNose.vz >> 4);
                alpha = PlaneLineCollision(facePoint, wingPos, facePtr->normal);
                if (alpha <= 0) {
                    if ((playerShip->nearTrkSect->flags & 16) || (playerShip->nearTrkSect->flags & 8)) {
                        if (CheckPolygonCollision(facePtr, wingPos, alpha, 1) == 1) {
                            NewWingCollision(playerShip, facePtr, downTrack, alpha, shipNo);
                            collided = 1;
                        }
                    } else if (collided) {
                        WingCollision(playerShip, facePtr, downTrack, alpha, shipNo);
                    } else {
                        NewWingCollision(playerShip, facePtr, downTrack, alpha, shipNo);
                    }
                }
            }
        }
    }
    if (beamCollide)
        BeamCollideReaction(playerShip);
}
#else
void Ship2Track(ShipData *playerShip, Face *facePtr, Object **shipShapes)
{
    VECTOR facePoint;
    VECTOR wingPos;
    VECTOR toFaceVector;
    int j, alpha;
    Face *facePtr2;
    VECTOR nosePos;
    int downTrack;
    short collided;
    short faceIndex;
    TrackSection *section;
    /* The executable reads the section centre again after updating attr. */
    Vector *sectionCentre;

    (void)shipShapes;
    collided = 0;
    faceIndex = 0;
    section = playerShip->nearTrkSect;
    sectionCentre = &section->centre;
    nosePos.vx = playerShip->ppivot.vx + (playerShip->unitVecNose.vx >> 3);
    nosePos.vy = playerShip->ppivot.vy + (playerShip->unitVecNose.vy >> 3);
    nosePos.vz = playerShip->ppivot.vz + (playerShip->unitVecNose.vz >> 3);
    downTrack = (section->nextSection.ptr->centre.vz - sectionCentre->vz) * playerShip->unitVecNose.vz
              + (section->nextSection.ptr->centre.vx - sectionCentre->vx) * playerShip->unitVecNose.vx
              + (section->nextSection.ptr->centre.vy - sectionCentre->vy) * playerShip->unitVecNose.vy;
    if (downTrack < 0)
        playerShip->attr &= ~DIRECTION;
    else
        playerShip->attr |= DIRECTION;
    toFaceVector.vx = track->vertices[facePtr->vertex[0]].vx - track->vertices[facePtr->vertex[1]].vx;
    toFaceVector.vy = track->vertices[facePtr->vertex[0]].vy - track->vertices[facePtr->vertex[1]].vy;
    toFaceVector.vz = track->vertices[facePtr->vertex[0]].vz - track->vertices[facePtr->vertex[1]].vz;
    alpha = (sectionCentre->vy - playerShip->ppivot.vy) * toFaceVector.vy
          + (sectionCentre->vz - playerShip->ppivot.vz) * toFaceVector.vz
          + (sectionCentre->vx - playerShip->ppivot.vx) * toFaceVector.vx;
    if (alpha > 0) {
        facePtr = track->faces + playerShip->nearTrkSect->faceStart;
        for (j = 0; j < playerShip->nearTrkSect->faceCount; j++) {
            if (facePtr->flags & TRACK_BASE)
                break;
            playerShip->attr |= 32;
            if (facePtr == track->faces + playerShip->nearTrkSect->faceStart || playerShip->nearTrkSect->junction.ptr != 0) {
                facePoint.vx = track->vertices[facePtr->vertex[0]].vx;
                facePoint.vy = track->vertices[facePtr->vertex[0]].vy;
                facePoint.vz = track->vertices[facePtr->vertex[0]].vz;
                alpha = PlaneLineCollision(facePoint, nosePos, facePtr->normal);
                if (alpha <= 0) {
                    if (playerShip->nearTrkSect->flags & 16) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->nextSection.ptr->faceStart;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr, -downTrack, alpha);
                                collided = 1;
                            }
                        }
                    } else if (playerShip->nearTrkSect->prevSection.ptr->junction.index != -1 &&
                           (playerShip->nearTrkSect->prevSection.ptr->junction.ptr->flags & 16)) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->nextSection.ptr->faceStart;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha);
                                collided = 1;
                            } else {
                                facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                                if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                    NoseCollision(playerShip, facePtr2, -downTrack, alpha);
                                    collided = 1;
                                }
                            }
                        }
                    } else if (playerShip->nearTrkSect->nextSection.ptr->junction.index != -1 &&
                           (playerShip->nearTrkSect->nextSection.ptr->junction.ptr->flags & 16)) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha);
                                collided = 1;
                            } else {
                                facePtr2 = track->faces + playerShip->nearTrkSect->nextSection.ptr->junction.ptr->faceStart;
                                if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                    NoseCollision(playerShip, facePtr2, -downTrack, alpha);
                                    collided = 1;
                                }
                            }
                        }
                    } else if (playerShip->nearTrkSect->flags & 8) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr, -downTrack, alpha);
                                collided = 1;
                            }
                        }
                    } else if (playerShip->nearTrkSect->nextSection.ptr->junction.index != -1 &&
                           (playerShip->nearTrkSect->nextSection.ptr->junction.ptr->flags & 8)) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha);
                                collided = 1;
                            }
                        }
                    } else {
                        NoseCollision(playerShip, facePtr, -downTrack, alpha);
                        collided = 1;
                    }
                }
                wingPos.vx = playerShip->ppivot.vx - (playerShip->unitVecWing.vx >> 4) - (playerShip->unitVecNose.vx >> 4);
                wingPos.vy = playerShip->ppivot.vy - (playerShip->unitVecWing.vy >> 4) - (playerShip->unitVecNose.vy >> 4);
                wingPos.vz = playerShip->ppivot.vz - (playerShip->unitVecWing.vz >> 4) - (playerShip->unitVecNose.vz >> 4);
                alpha = PlaneLineCollision(facePoint, wingPos, facePtr->normal);
                if (alpha <= 0) {
                    if (playerShip->nearTrkSect->flags & 0x18) {
                        if (CheckPolygonCollision(facePtr, wingPos, alpha, 1) == 1) {
                            NewWingCollision(playerShip, facePtr, -downTrack, alpha);
                            collided = 1;
                        }
                    } else if (collided) {
                        WingCollision(playerShip, facePtr, -downTrack, alpha);
                    } else {
                        NewWingCollision(playerShip, facePtr, -downTrack, alpha);
                    }
                }
                wingPos.vx = playerShip->ppivot.vx + (playerShip->unitVecWing.vx >> 4) - (playerShip->unitVecNose.vx >> 4);
                wingPos.vy = playerShip->ppivot.vy + (playerShip->unitVecWing.vy >> 4) - (playerShip->unitVecNose.vy >> 4);
                wingPos.vz = playerShip->ppivot.vz + (playerShip->unitVecWing.vz >> 4) - (playerShip->unitVecNose.vz >> 4);
                alpha = PlaneLineCollision(facePoint, wingPos, facePtr->normal);
                if (alpha <= 0) {
                    if (playerShip->nearTrkSect->flags & 0x18) {
                        if (CheckPolygonCollision(facePtr, wingPos, alpha, 1) == 1) {
                            NewWingCollision(playerShip, facePtr, -downTrack, alpha);
                            collided = 1;
                        }
                    } else if (collided) {
                        WingCollision(playerShip, facePtr, -downTrack, alpha);
                    } else {
                        NewWingCollision(playerShip, facePtr, -downTrack, alpha);
                    }
                }
            }
            facePtr++;
            if (facePtr->flags & TRACK_BASE)
                break;
        }
    } else {
        playerShip->attr &= ~32;
        facePtr = track->faces + playerShip->nearTrkSect->faceStart;
        while (!(facePtr->flags & TRACK_BASE)) {
            facePtr++;
            faceIndex++;
        }
        while (facePtr->flags & TRACK_BASE) {
            facePtr++;
            faceIndex++;
        }
        for (j = faceIndex; j < playerShip->nearTrkSect->faceCount; j++, facePtr++) {
            if (playerShip->nearTrkSect->faceCount > 3 || playerShip->nearTrkSect->junction.ptr != 0) {
                facePoint.vx = track->vertices[facePtr->vertex[0]].vx;
                facePoint.vy = track->vertices[facePtr->vertex[0]].vy;
                facePoint.vz = track->vertices[facePtr->vertex[0]].vz;
                alpha = PlaneLineCollision(facePoint, nosePos, facePtr->normal);
                if (alpha <= 0) {
                    if (playerShip->nearTrkSect->flags & 16) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, downTrack, alpha);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->nextSection.ptr->faceStart;
                            facePtr2 += 3;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr, -downTrack, alpha);
                                collided = 1;
                            }
                        }
                    } else if (playerShip->nearTrkSect->prevSection.ptr->junction.index != -1 &&
                           (playerShip->nearTrkSect->prevSection.ptr->junction.ptr->flags & 16)) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, downTrack, alpha);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            facePtr2 += 3;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha);
                                collided = 1;
                            } else {
                                facePtr2 = track->faces + playerShip->nearTrkSect->nextSection.ptr->faceStart;
                                facePtr2 += 3;
                                if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                    NoseCollision(playerShip, facePtr2, -downTrack, alpha);
                                    collided = 1;
                                }
                            }
                        }
                    } else if (playerShip->nearTrkSect->nextSection.ptr->junction.index != -1 &&
                           (playerShip->nearTrkSect->nextSection.ptr->junction.ptr->flags & 16)) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, downTrack, alpha);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            facePtr2 += 3;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha);
                                collided = 1;
                            } else {
                                facePtr2 = track->faces + playerShip->nearTrkSect->nextSection.ptr->junction.ptr->faceStart;
                                facePtr2 += 3;
                                if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                    NoseCollision(playerShip, facePtr2, -downTrack, alpha);
                                    collided = 1;
                                }
                            }
                        }
                    } else if (playerShip->nearTrkSect->flags & 8) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            facePtr2 += 3;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha);
                                collided = 1;
                            }
                        }
                    } else if (playerShip->nearTrkSect->nextSection.ptr->junction.index != -1 &&
                           (playerShip->nearTrkSect->nextSection.ptr->junction.ptr->flags & 8)) {
                        if (CheckPolygonCollision(facePtr, nosePos, alpha, 1) == 1) {
                            NoseCollision(playerShip, facePtr, -downTrack, alpha);
                            collided = 1;
                        } else {
                            facePtr2 = track->faces + playerShip->nearTrkSect->prevSection.ptr->faceStart;
                            facePtr2 += 3;
                            if (CheckPolygonCollision(facePtr2, nosePos, alpha, 1) == 1) {
                                NoseCollision(playerShip, facePtr2, -downTrack, alpha);
                                collided = 1;
                            }
                        }
                    } else {
                        NoseCollision(playerShip, facePtr, downTrack, alpha);
                        collided = 1;
                    }
                }
                wingPos.vx = playerShip->ppivot.vx - (playerShip->unitVecWing.vx >> 4) - (playerShip->unitVecNose.vx >> 4);
                wingPos.vy = playerShip->ppivot.vy - (playerShip->unitVecWing.vy >> 4) - (playerShip->unitVecNose.vy >> 4);
                wingPos.vz = playerShip->ppivot.vz - (playerShip->unitVecWing.vz >> 4) - (playerShip->unitVecNose.vz >> 4);
                alpha = PlaneLineCollision(facePoint, wingPos, facePtr->normal);
                if (alpha <= 0) {
                    if (playerShip->nearTrkSect->flags & 0x18) {
                        if (CheckPolygonCollision(facePtr, wingPos, alpha, 1) == 1) {
                            NewWingCollision(playerShip, facePtr, downTrack, alpha);
                            collided = 1;
                        }
                    } else if (collided) {
                        WingCollision(playerShip, facePtr, downTrack, alpha);
                    } else {
                        NewWingCollision(playerShip, facePtr, downTrack, alpha);
                    }
                }
                wingPos.vx = playerShip->ppivot.vx + (playerShip->unitVecWing.vx >> 4) - (playerShip->unitVecNose.vx >> 4);
                wingPos.vy = playerShip->ppivot.vy + (playerShip->unitVecWing.vy >> 4) - (playerShip->unitVecNose.vy >> 4);
                wingPos.vz = playerShip->ppivot.vz + (playerShip->unitVecWing.vz >> 4) - (playerShip->unitVecNose.vz >> 4);
                alpha = PlaneLineCollision(facePoint, wingPos, facePtr->normal);
                if (alpha <= 0) {
                    if (playerShip->nearTrkSect->flags & 0x18) {
                        if (CheckPolygonCollision(facePtr, wingPos, alpha, 1) == 1) {
                            NewWingCollision(playerShip, facePtr, downTrack, alpha);
                            collided = 1;
                        }
                    } else if (collided) {
                        WingCollision(playerShip, facePtr, downTrack, alpha);
                    } else {
                        NewWingCollision(playerShip, facePtr, downTrack, alpha);
                    }
                }
            }
        }
    }
    if (beamCollide)
        BeamCollideReaction(playerShip);
}
#endif
#endif

#include "functions.h"
#include <stdlib.h>
#include <stdio.h>

extern SParticle *particleSlots;
extern short scrapeVol;

void BeamCollideReaction(ShipData *playerShip)
{
    VECTOR reaction;
    SVector normal;
    int normalX, normalY, normalZ;
    Face *facePtr;

    reaction.vx = playerShip->vpivot.vx >> 2;
    reaction.vy = playerShip->vpivot.vy >> 2;
    reaction.vz = playerShip->vpivot.vz >> 2;
    playerShip->vpivot.vx -= reaction.vx * 3;
    playerShip->vpivot.vy -= reaction.vy * 3;
    playerShip->vpivot.vz -= reaction.vz * 3;
    playerShip->ppivot.vx -= playerShip->vpivot.vx >> 6;
    playerShip->ppivot.vy -= playerShip->vpivot.vy >> 6;
    playerShip->ppivot.vz -= playerShip->vpivot.vz >> 6;
    facePtr = track->faces + playerShip->nearTrkSect->faceStart;
    while (!(facePtr->flags & TRACK_BASE))
        facePtr++;
    facePtr++;
    normalX = facePtr->normal.vx * 2;
    normal.vx = normalX >> 1;
    normalY = facePtr->normal.vy * 2;
    normal.vy = normalY >> 1;
    normalZ = facePtr->normal.vz * 2;
    normal.vz = normalZ >> 1;
    playerShip->ppivot.vx += normal.vx;
    playerShip->ppivot.vy += normal.vy;
    playerShip->ppivot.vz += normal.vz;
}

#ifdef NETPLAY
void NewWingCollision(ShipData *playerShip, Face *facePtr, int direction, int alpha, short shipNo)
#else
void NewWingCollision(ShipData *playerShip, Face *facePtr, int direction, int alpha)
#endif
{
    VECTOR reaction;
    VECTOR wingPos;

    playerShip->vpivot.vx += facePtr->normal.vx;
    playerShip->vpivot.vy += facePtr->normal.vy >> 1;
    playerShip->vpivot.vz += facePtr->normal.vz;
    if (direction > 0) {
        wingPos.vx = playerShip->ppivot.vx + (playerShip->unitVecWing.vx >> 4) - (playerShip->unitVecNose.vx >> 4);
        wingPos.vy = playerShip->ppivot.vy + (playerShip->unitVecWing.vy >> 4) - (playerShip->unitVecNose.vy >> 4);
        wingPos.vz = playerShip->ppivot.vz + (playerShip->unitVecWing.vz >> 4) - (playerShip->unitVecNose.vz >> 4);
    } else {
        wingPos.vx = playerShip->ppivot.vx - (playerShip->unitVecWing.vx >> 4) - (playerShip->unitVecNose.vx >> 4);
        wingPos.vy = playerShip->ppivot.vy - (playerShip->unitVecWing.vy >> 4) - (playerShip->unitVecNose.vy >> 4);
        wingPos.vz = playerShip->ppivot.vz - (playerShip->unitVecWing.vz >> 4) - (playerShip->unitVecNose.vz >> 4);
    }
#ifdef NETPLAY
    if (shipNo == ownShip) {
#endif
        if (scrapeVol < 200)
            scrapeVol += 100;
        if (scrapeVol > 200)
            scrapeVol = 200;
#ifdef NETPLAY
    }
#endif
    reaction.vx = playerShip->vpivot.vx >> 6;
    reaction.vy = playerShip->vpivot.vy >> 6;
    reaction.vz = playerShip->vpivot.vz >> 6;
    AddScrapingParticles(particleSlots, 8, &wingPos, &reaction);
    AddRandomParticles(particleSlots, 8, &wingPos, &reaction);
}

#ifdef NETPLAY
void WingCollision(ShipData *playerShip, Face *facePtr, int direction, int alpha, short shipNo)
#else
void WingCollision(ShipData *playerShip, Face *facePtr, int direction, int alpha)
#endif
{
    VECTOR reflect;
    int mag;
    VECTOR reaction;
    VECTOR wingPos;

    if (alpha < 0) {
        reflect.vx = playerShip->vpivot.vx / abs(alpha);
        reflect.vy = playerShip->vpivot.vy / abs(alpha);
        reflect.vz = playerShip->vpivot.vz / abs(alpha);
        playerShip->vpivot.vx = 0;
        playerShip->vpivot.vy = 0;
        playerShip->vpivot.vz = 0;
        playerShip->vpivot.vx += reflect.vx >> 5;
        playerShip->vpivot.vy += reflect.vy >> 5;
        playerShip->vpivot.vz += reflect.vz >> 5;
    }
    playerShip->vpivot.vx += facePtr->normal.vx * 2;
    playerShip->vpivot.vy += facePtr->normal.vy >> 1;
    playerShip->vpivot.vz += facePtr->normal.vz * 2;
    playerShip->ppivot.vx += playerShip->vpivot.vx >> 6;
    playerShip->ppivot.vy += playerShip->vpivot.vy >> 6;
    playerShip->ppivot.vz += playerShip->vpivot.vz >> 6;
    mag = (playerShip->speed >> 5) + 200;
    if (direction > 0)
        playerShip->vhdg += mag;
    else
        playerShip->vhdg -= mag;
    if (direction > 0) {
        wingPos.vx = playerShip->ppivot.vx + (playerShip->unitVecWing.vx >> 4) - (playerShip->unitVecNose.vx >> 4);
        wingPos.vy = playerShip->ppivot.vy + (playerShip->unitVecWing.vy >> 4) - (playerShip->unitVecNose.vy >> 4);
        wingPos.vz = playerShip->ppivot.vz + (playerShip->unitVecWing.vz >> 4) - (playerShip->unitVecNose.vz >> 4);
    } else {
        wingPos.vx = playerShip->ppivot.vx - (playerShip->unitVecWing.vx >> 4) - (playerShip->unitVecNose.vx >> 4);
        wingPos.vy = playerShip->ppivot.vy - (playerShip->unitVecWing.vy >> 4) - (playerShip->unitVecNose.vy >> 4);
        wingPos.vz = playerShip->ppivot.vz - (playerShip->unitVecWing.vz >> 4) - (playerShip->unitVecNose.vz >> 4);
    }
#ifdef NETPLAY
    if (shipNo == ownShip)
#endif
        PlayDirectionalNote(7, 1, 20, wingPos);
#ifdef NETPLAY
    DamageCollide(129, shipNo, -1);
#else
    DamageCollide(129, ownShip);
#endif
    reaction.vx = playerShip->vpivot.vx >> 6;
    reaction.vy = playerShip->vpivot.vy >> 6;
    reaction.vz = playerShip->vpivot.vz >> 6;
    AddRandomParticles(particleSlots, 8, &wingPos, &reaction);
}

#ifdef NETPLAY
void NoseCollision(ShipData *playerShip, Face *facePtr, int direction, int alpha, short shipNo)
#else
void NoseCollision(ShipData *playerShip, Face *facePtr, int direction, int alpha)
#endif
{
    VECTOR reflect;
    VECTOR tip;
    int mag;
    VECTOR reaction;

    if (alpha < 0) {
        reflect.vx = playerShip->vpivot.vx / abs(alpha);
        reflect.vy = playerShip->vpivot.vy / abs(alpha);
        reflect.vz = playerShip->vpivot.vz / abs(alpha);
        playerShip->vpivot.vx = 0;
        playerShip->vpivot.vy = 0;
        playerShip->vpivot.vz = 0;
        playerShip->vpivot.vx += reflect.vx >> 5;
        playerShip->vpivot.vy += reflect.vy >> 5;
        playerShip->vpivot.vz += reflect.vz >> 5;
    }
    playerShip->vpivot.vx += facePtr->normal.vx * 2;
    playerShip->vpivot.vy += facePtr->normal.vy >> 1;
    playerShip->vpivot.vz += facePtr->normal.vz * 2;
    playerShip->ppivot.vx += playerShip->vpivot.vx >> 6;
    playerShip->ppivot.vy += playerShip->vpivot.vy >> 6;
    playerShip->ppivot.vz += playerShip->vpivot.vz >> 6;
    mag = (playerShip->speed >> 2) + 400;
    if (direction > 0)
        playerShip->vhdg += mag;
    else
        playerShip->vhdg -= mag;
    tip.vx = playerShip->ppivot.vx + (playerShip->unitVecNose.vx >> 3);
    tip.vy = playerShip->ppivot.vy + (playerShip->unitVecNose.vy >> 3);
    tip.vz = playerShip->ppivot.vz + (playerShip->unitVecNose.vz >> 3);
#ifdef NETPLAY
    if (shipNo == ownShip)
#endif
        PlayDirectionalNote(7, 1, 20, tip);
#ifdef NETPLAY
    DamageCollide(129, shipNo, -1);
#else
    DamageCollide(129, ownShip);
#endif
#ifdef NETPLAY
    DamageCollide(129, shipNo, -1);
#else
    DamageCollide(129, ownShip);
#endif
    reaction.vx = playerShip->vpivot.vx >> 6;
    reaction.vy = playerShip->vpivot.vy >> 6;
    reaction.vz = playerShip->vpivot.vz >> 6;
    AddRandomParticles(particleSlots, 8, &tip, &reaction);
}


int PlaneLineCollision(VECTOR plane_point, VECTOR ship_pos, SVector normal)
{
    int dot_product, norm_dot_product;
    int coll_data;
    dot_product = (plane_point.vx - ship_pos.vx) * normal.vx;
    dot_product += (plane_point.vy - ship_pos.vy) * normal.vy;
    dot_product += (plane_point.vz - ship_pos.vz) * normal.vz;
    norm_dot_product = ((-normal.vx * normal.vx) >> 12) +
                       ((-normal.vy * normal.vy) >> 12) +
                       ((-normal.vz * normal.vz) >> 12);
    coll_data = dot_product / norm_dot_product;
    return coll_data;
}


void Reflection(VECTOR *incidence, SVector normal)
{
    int dotProduct;
    dotProduct = ((normal.vz * -incidence->vz) >> 12) +
           ((normal.vy * -incidence->vy) >> 12) +
           ((normal.vx * -incidence->vx) >> 12);
    incidence->vx = ((2 * normal.vx * dotProduct) >> 12) + incidence->vx;
    incidence->vy = ((2 * normal.vy * dotProduct) >> 12) + incidence->vy;
    incidence->vz = ((2 * normal.vz * dotProduct) >> 12) + incidence->vz;
}

int AngleOfCollision(SVector normal, VECTOR ship_point)
{
#ifdef NETPLAY
    int dot4096, cosineAngle, normalLen, dotProduct, pointLength, dist_product;

    ship_point.vx >>= 9;
    ship_point.vy >>= 9;
    ship_point.vz >>= 9;
    dotProduct = normal.vx * ship_point.vx + normal.vy * ship_point.vy + normal.vz * ship_point.vz;
    normalLen = sqrt(normal.vx * normal.vx + normal.vy * normal.vy + normal.vz * normal.vz);
    pointLength = sqrt(ship_point.vy * ship_point.vy + ship_point.vz * ship_point.vz + ship_point.vx * ship_point.vx);
    dist_product = pointLength * normalLen;
    dot4096 = dotProduct << 12;
    if (dist_product)
        cosineAngle = dot4096 / dist_product;
    else
        cosineAngle = 0;
    return cosineAngle;
#else
    int dot_product;
    int norm_dist, shpoint_dist;
    int dist_product;
    int scaledDot, cosAngle;
    ship_point.vx >>= 9;
    ship_point.vy >>= 9;
    ship_point.vz >>= 9;
    dot_product = ship_point.vx * normal.vx + ship_point.vy * normal.vy + ship_point.vz * normal.vz;
    norm_dist = sqrt(normal.vx * normal.vx + normal.vy * normal.vy + normal.vz * normal.vz);
    shpoint_dist = sqrt(ship_point.vx * ship_point.vx + ship_point.vy * ship_point.vy + ship_point.vz * ship_point.vz);
    dist_product = shpoint_dist * norm_dist;
    scaledDot = dot_product << 12;
    if (dist_product)
        cosAngle = scaledDot / dist_product;
    else
        cosAngle = 0;
    return cosAngle;
#endif
}

int CheckPolygonCollision(Face *facePtr, VECTOR pos, int alpha, int side)
{
    VECTOR planePoint;
    VECTOR vec0, vec1, vec2, vec3;
    int angle;
    planePoint.vx = pos.vx - ((facePtr->normal.vx * alpha) >> 12);
    planePoint.vy = pos.vy - ((facePtr->normal.vy * alpha) >> 12);
    planePoint.vz = pos.vz - ((facePtr->normal.vz * alpha) >> 12);
    vec0.vx = planePoint.vx - track->vertices[facePtr->vertex[1]].vx;
    vec0.vy = planePoint.vy - track->vertices[facePtr->vertex[1]].vy;
    vec0.vz = planePoint.vz - track->vertices[facePtr->vertex[1]].vz;
    vec1.vx = planePoint.vx - track->vertices[facePtr->vertex[2]].vx;
    vec1.vy = planePoint.vy - track->vertices[facePtr->vertex[2]].vy;
    vec1.vz = planePoint.vz - track->vertices[facePtr->vertex[2]].vz;
    if (side != 1)
        facePtr++;
    vec2.vx = planePoint.vx - track->vertices[facePtr->vertex[0]].vx;
    vec2.vy = planePoint.vy - track->vertices[facePtr->vertex[0]].vy;
    vec2.vz = planePoint.vz - track->vertices[facePtr->vertex[0]].vz;
    vec3.vx = planePoint.vx - track->vertices[facePtr->vertex[3]].vx;
    vec3.vy = planePoint.vy - track->vertices[facePtr->vertex[3]].vy;
    vec3.vz = planePoint.vz - track->vertices[facePtr->vertex[3]].vz;
    angle = GetAng(vec0, vec2);
    angle += GetAng(vec2, vec3);
    angle += GetAng(vec3, vec1);
    angle += GetAng(vec1, vec0);
    return angle > 30000;
}

#ifndef NETPLAY
int NewCollideLineToPlane(Object *object, Object **shipShapes, ShipData *shipIndex, int shipNo)
{
    int i;
    int count;
    Poly poly;
    short *coords;
    short primitiveCount;

    VECTOR p1, p2, p3, p4;

    VECTOR a, b, c, d;

    int factor2;
    VECTOR factor1;
    int stepX, stepY, stepZ;

    VECTOR res_vec;
    int alpha;

    VECTOR DC, BC, AC;
    VECTOR BD, AB;
    VECTOR AD;

    VECTOR start_vec;
    VECTOR pos_vec;

    VECTOR plane1;

    VECTOR P1P2;
    VECTOR P1P3;

    VECTOR vec0, vec1, vec2, vec3;
    int angle;

    short high2;
    short low2;
#ifdef NETPLAY
    int polygonSize = 0;
#else
    short polygonSize = 0;
#endif
#ifdef NETPLAY
    short planeSize = 0;
#else
    short planeSize;
#endif

    high2 = shipIndex[shipNo].highResTableIndex;
    low2 = shipIndex[shipNo].lowResTableIndex;

    getWorldPoint(&a, shipShapes[high2], shipShapes[low2], 0);
    getWorldPoint(&b, shipShapes[high2], shipShapes[low2], 1);
    getWorldPoint(&c, shipShapes[high2], shipShapes[low2], 2);
    getWorldPoint(&d, shipShapes[high2], shipShapes[low2], 3);

    MinusVector(a, c, &AC);
    MinusVector(b, c, &BC);
    MinusVector(d, c, &DC);
    MinusVector(a, b, &AB);
    MinusVector(b, d, &BD);
    MinusVector(a, d, &AD);

    poly.prim = object->primitives;
    primitiveCount = object->primitiveCount;

    for (i = 0; i < primitiveCount; i++) {
        switch (poly.prim->type) {
        case TypeF4:
            coords = poly.f4->coords;
            getWorldPoint(&p1, object, object, coords[0]);
            getWorldPoint(&p2, object, object, coords[1]);
            getWorldPoint(&p3, object, object, coords[2]);
            getWorldPoint(&p4, object, object, coords[3]);
            polygonSize = 4;
            poly.f4++;
            break;
        case TypeG4:
            coords = poly.g4->coords;
            getWorldPoint(&p1, object, object, coords[0]);
            getWorldPoint(&p2, object, object, coords[1]);
            getWorldPoint(&p3, object, object, coords[2]);
            getWorldPoint(&p4, object, object, coords[3]);
            polygonSize = 4;
            poly.g4++;
            break;
        case TypeFT4:
            coords = poly.ft4->coords;
            getWorldPoint(&p1, object, object, coords[0]);
            getWorldPoint(&p2, object, object, coords[1]);
            getWorldPoint(&p3, object, object, coords[2]);
            getWorldPoint(&p4, object, object, coords[3]);
            polygonSize = 4;
            poly.ft4++;
            break;
        case TypeGT4:
            coords = poly.gt4->coords;
            getWorldPoint(&p1, object, object, coords[0]);
            getWorldPoint(&p2, object, object, coords[1]);
            getWorldPoint(&p3, object, object, coords[2]);
            getWorldPoint(&p4, object, object, coords[3]);
            polygonSize = 4;
            poly.gt4++;
            break;
        case TypeF3:
            coords = poly.f3->coords;
            getWorldPoint(&p1, object, object, coords[0]);
            getWorldPoint(&p2, object, object, coords[1]);
            getWorldPoint(&p3, object, object, coords[2]);
            polygonSize = 3;
            poly.f3++;
            break;
        case TypeG3:
            coords = poly.g3->coords;
            getWorldPoint(&p1, object, object, coords[0]);
            getWorldPoint(&p2, object, object, coords[1]);
            getWorldPoint(&p3, object, object, coords[2]);
            polygonSize = 3;
            poly.g3++;
            break;
        case TypeFT3:
            coords = poly.ft3->coords;
            getWorldPoint(&p1, object, object, coords[0]);
            getWorldPoint(&p2, object, object, coords[1]);
            getWorldPoint(&p3, object, object, coords[2]);
            polygonSize = 3;
            poly.ft3++;
            break;
        case TypeGT3:
            coords = poly.gt3->coords;
            getWorldPoint(&p1, object, object, coords[0]);
            getWorldPoint(&p2, object, object, coords[1]);
            getWorldPoint(&p3, object, object, coords[2]);
            polygonSize = 3;
            poly.gt3++;
            break;
        default:
            break;
        }
        MinusVector(p1, p2, &P1P2);
        MinusVector(p1, p3, &P1P3);

        CrossProduct(&plane1, P1P2, P1P3);
        planeSize = polygonSize;

        for (count = 0; count < 6; count++) {
            switch (count) {
            case 0:
                assign(&start_vec, b);
                assign(&pos_vec, BC);
                break;
            case 1:
                assign(&start_vec, a);
                assign(&pos_vec, AC);
                break;
            case 2:
                assign(&start_vec, d);
                assign(&pos_vec, DC);
                break;
            case 3:
                assign(&start_vec, a);
                assign(&pos_vec, AB);
                break;
            case 4:
                assign(&start_vec, a);
                assign(&pos_vec, AD);
                break;
            case 5:
                assign(&start_vec, b);
                assign(&pos_vec, BD);
                break;
            }

            MinusVector(start_vec, p1, &factor1);
            alpha = (ScalarProduct(factor1, plane1) << 12);
            factor2 = ScalarProduct(pos_vec, plane1);
            if (factor2 != 0) {
                alpha = alpha / factor2;

                stepX = (alpha * pos_vec.vx) >> 12;
                stepY = (alpha * pos_vec.vy) >> 12;
                stepZ = (alpha * pos_vec.vz) >> 12;

                res_vec.vx = start_vec.vx + stepX;
                res_vec.vy = start_vec.vy + stepY;
                res_vec.vz = start_vec.vz + stepZ;

                if (alpha >= 0 && alpha <= 4096) {
                    vec0.vx = res_vec.vx - p1.vx;
                    vec0.vy = res_vec.vy - p1.vy;
                    vec0.vz = res_vec.vz - p1.vz;

                    vec1.vx = res_vec.vx - p2.vx;
                    vec1.vy = res_vec.vy - p2.vy;
                    vec1.vz = res_vec.vz - p2.vz;

                    vec2.vx = res_vec.vx - p3.vx;
                    vec2.vy = res_vec.vy - p3.vy;
                    vec2.vz = res_vec.vz - p3.vz;

                    vec3.vx = res_vec.vx - p4.vx;
                    vec3.vy = res_vec.vy - p4.vy;
                    vec3.vz = res_vec.vz - p4.vz;
                    if (planeSize == 3) {
                        angle = GetAng(vec0, vec1);
                        angle += GetAng(vec1, vec2);
                        angle += GetAng(vec2, vec0);
                    } else {
                        angle = GetAng(vec0, vec1);
                        angle += GetAng(vec1, vec2);
                        angle += GetAng(vec2, vec0);
                        angle += GetAng(vec2, vec3);
                    }
                    if (planeSize == 4) {
                        if (angle >= 33000)
                            return 1;
                    } else {
                        if (angle >= 30000)
                            return 1;
                    }
                }
            }
        }
    }
    return 0;
}
#endif
#ifdef EQUIV
#ifdef NETPLAY
    /* NET local names preserve the /Od frame slots. */
int NewCollideLineToPlane(Object *object, Object **shipShapes, ShipData *shipIndex, int shipNo)
{
    int factor2Second;
    VECTOR plane1;
    VECTOR vec2End;
    short primitiveCountFirst;
    VECTOR p2World;
    VECTOR bValue;
    VECTOR p1Edge;
    VECTOR highAValue;
    VECTOR vec3End;
    int edgeCount;
    short *coordsEnd;
    int collisionAlpha;
    VECTOR DCEnd;
    VECTOR pos_vecEnd;
    VECTOR polygonPointFour;
    VECTOR d;
    VECTOR lowP3;
    VECTOR shipVertexC;
    int stepDepthExtra; /* The executable allocates this word but never reads or writes it. */
    VECTOR ADPlane;
    VECTOR vertexDifferenceAC;
    VECTOR edge13Edge;
    int angle = 0;
    VECTOR firstAB;
    VECTOR sourceVector;
    VECTOR polygonEdge12;
    int deltaDepth;
    int yStep;
    VECTOR vertexDifferenceBC;
    short high2End;
    int i;
    int stepComponent;
    VECTOR vectorFactor;
    VECTOR secondAngleVector;
    short lowResIndex;
    VECTOR edgeVectorZero;
    SVECTOR *objectVertexPointer; /* Initialized by the executable and never read. */
    VECTOR shipBD;
    short polygonSize = 0;
    Poly polyData;
    VECTOR planeResult;

    high2End = shipIndex[shipNo].highResTableIndex;
    lowResIndex = shipIndex[shipNo].lowResTableIndex;

    getWorldPoint(&highAValue, shipShapes[high2End], shipShapes[lowResIndex], 0);
    getWorldPoint(&bValue, shipShapes[high2End], shipShapes[lowResIndex], 1);
    getWorldPoint(&shipVertexC, shipShapes[high2End], shipShapes[lowResIndex], 2);
    getWorldPoint(&d, shipShapes[high2End], shipShapes[lowResIndex], 3);

    MinusVector(highAValue, shipVertexC, &vertexDifferenceAC);
    MinusVector(bValue, shipVertexC, &vertexDifferenceBC);
    MinusVector(d, shipVertexC, &DCEnd);
    MinusVector(highAValue, bValue, &firstAB);
    MinusVector(bValue, d, &shipBD);
    MinusVector(highAValue, d, &ADPlane);

    polyData.prim = object->primitives;
    primitiveCountFirst = object->primitiveCount;
    objectVertexPointer = object->vertices;

    for (i = 0; i < primitiveCountFirst; i++) {
        switch (polyData.prim->type) {
        case TypeF4:
            coordsEnd = polyData.f4->coords;
            getWorldPoint(&p1Edge, object, object, coordsEnd[0]);
            getWorldPoint(&p2World, object, object, coordsEnd[1]);
            getWorldPoint(&lowP3, object, object, coordsEnd[2]);
            getWorldPoint(&polygonPointFour, object, object, coordsEnd[3]);
            polygonSize = 4;
            polyData.f4++;
            break;
        case TypeG4:
            coordsEnd = polyData.g4->coords;
            getWorldPoint(&p1Edge, object, object, coordsEnd[0]);
            getWorldPoint(&p2World, object, object, coordsEnd[1]);
            getWorldPoint(&lowP3, object, object, coordsEnd[2]);
            getWorldPoint(&polygonPointFour, object, object, coordsEnd[3]);
            polygonSize = 4;
            polyData.g4++;
            break;
        case TypeFT4:
            coordsEnd = polyData.ft4->coords;
            getWorldPoint(&p1Edge, object, object, coordsEnd[0]);
            getWorldPoint(&p2World, object, object, coordsEnd[1]);
            getWorldPoint(&lowP3, object, object, coordsEnd[2]);
            getWorldPoint(&polygonPointFour, object, object, coordsEnd[3]);
            polygonSize = 4;
            polyData.ft4++;
            break;
        case TypeGT4:
            coordsEnd = polyData.gt4->coords;
            getWorldPoint(&p1Edge, object, object, coordsEnd[0]);
            getWorldPoint(&p2World, object, object, coordsEnd[1]);
            getWorldPoint(&lowP3, object, object, coordsEnd[2]);
            getWorldPoint(&polygonPointFour, object, object, coordsEnd[3]);
            polygonSize = 4;
            polyData.gt4++;
            break;
        case TypeF3:
            coordsEnd = polyData.f3->coords;
            getWorldPoint(&p1Edge, object, object, coordsEnd[0]);
            getWorldPoint(&p2World, object, object, coordsEnd[1]);
            getWorldPoint(&lowP3, object, object, coordsEnd[2]);
            polygonSize = 3;
            polyData.f3++;
            break;
        case TypeG3:
            coordsEnd = polyData.g3->coords;
            getWorldPoint(&p1Edge, object, object, coordsEnd[0]);
            getWorldPoint(&p2World, object, object, coordsEnd[1]);
            getWorldPoint(&lowP3, object, object, coordsEnd[2]);
            polygonSize = 3;
            polyData.g3++;
            break;
        case TypeFT3:
            coordsEnd = polyData.ft3->coords;
            getWorldPoint(&p1Edge, object, object, coordsEnd[0]);
            getWorldPoint(&p2World, object, object, coordsEnd[1]);
            getWorldPoint(&lowP3, object, object, coordsEnd[2]);
            polygonSize = 3;
            polyData.ft3++;
            break;
        case TypeGT3:
            coordsEnd = polyData.gt3->coords;
            getWorldPoint(&p1Edge, object, object, coordsEnd[0]);
            getWorldPoint(&p2World, object, object, coordsEnd[1]);
            getWorldPoint(&lowP3, object, object, coordsEnd[2]);
            polygonSize = 3;
            polyData.gt3++;
            break;
        default:
            break;
        }
        MinusVector(p1Edge, p2World, &polygonEdge12);
        MinusVector(p1Edge, lowP3, &edge13Edge);

        CrossProduct(&plane1, polygonEdge12, edge13Edge);

        for (edgeCount = 0; edgeCount < 6; edgeCount++) {
            switch (edgeCount) {
            case 0:
                assign(&sourceVector, bValue);
                assign(&pos_vecEnd, vertexDifferenceBC);
                break;
            case 1:
                assign(&sourceVector, highAValue);
                assign(&pos_vecEnd, vertexDifferenceAC);
                break;
            case 2:
                assign(&sourceVector, d);
                assign(&pos_vecEnd, DCEnd);
                break;
            case 3:
                assign(&sourceVector, highAValue);
                assign(&pos_vecEnd, firstAB);
                break;
            case 4:
                assign(&sourceVector, highAValue);
                assign(&pos_vecEnd, ADPlane);
                break;
            case 5:
                assign(&sourceVector, bValue);
                assign(&pos_vecEnd, shipBD);
                break;
            }

            MinusVector(sourceVector, p1Edge, &vectorFactor);
            collisionAlpha = (ScalarProduct(vectorFactor, plane1) << 12);
            factor2Second = ScalarProduct(pos_vecEnd, plane1);
            if (factor2Second != 0) {
                collisionAlpha = collisionAlpha / factor2Second;

                stepComponent = (collisionAlpha * pos_vecEnd.vx) >> 12;
                yStep = (collisionAlpha * pos_vecEnd.vy) >> 12;
                deltaDepth = (collisionAlpha * pos_vecEnd.vz) >> 12;

                planeResult.vx = stepComponent + sourceVector.vx;
                planeResult.vy = yStep + sourceVector.vy;
                planeResult.vz = deltaDepth + sourceVector.vz;

                if (collisionAlpha >= 0 && collisionAlpha <= 4096) {
                    edgeVectorZero.vx = planeResult.vx - p1Edge.vx;
                    edgeVectorZero.vy = planeResult.vy - p1Edge.vy;
                    edgeVectorZero.vz = planeResult.vz - p1Edge.vz;

                    secondAngleVector.vx = planeResult.vx - p2World.vx;
                    secondAngleVector.vy = planeResult.vy - p2World.vy;
                    secondAngleVector.vz = planeResult.vz - p2World.vz;

                    vec2End.vx = planeResult.vx - lowP3.vx;
                    vec2End.vy = planeResult.vy - lowP3.vy;
                    vec2End.vz = planeResult.vz - lowP3.vz;

                    vec3End.vx = planeResult.vx - polygonPointFour.vx;
                    vec3End.vy = planeResult.vy - polygonPointFour.vy;
                    vec3End.vz = planeResult.vz - polygonPointFour.vz;
                    if (polygonSize == 3) {
                        angle = GetAng(edgeVectorZero, secondAngleVector);
                        angle += GetAng(secondAngleVector, vec2End);
                        angle += GetAng(vec2End, edgeVectorZero);
                    } else {
                        angle = GetAng(edgeVectorZero, secondAngleVector);
                        angle += GetAng(secondAngleVector, vec2End);
                        angle += GetAng(vec2End, edgeVectorZero);
                        angle += GetAng(vec2End, vec3End);
                    }
                    if (polygonSize == 4) {
                        if (angle >= 33000)
                            return 1;
                    } else {
                        if (angle >= 30000)
                            return 1;
                    }
                }
            }
        }
    }
    return 0;
}
#endif
#endif
