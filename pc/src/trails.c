#include "functions.h"
#include "weapon_state.h"
#include "ship_state.h"
#include "trails.h"
#include "d3dpoly.h"
#include <math.h>
#include <string.h>
#ifdef NETPLAY
#pragma intrinsic(sin, cos, abs, memset)
#endif
#include "mathinline.h"

extern short ownShip;
extern ConfigData *gameData;
extern ShipData *shipIndex;
#include "heap_state.h"
extern POLY_G4 flare[19][12];
extern struct Trail *shipTrails;



extern struct Trail Plasma[4];
extern int plasmaPhaseA;
extern int plasmaPhaseB;
extern int plasmaPhaseC;


#ifndef NETPLAY
void DrawFlare(int x, int y, int distance, int ship, float z)
{
    int speed, value, half, quarter, size;
    ShipData *s;
    POLY_G4 *p;
    if (ship < 15) {
        if (!gameData->trailDetail)
            return;
        s = &shipIndex[ship];
        if (s->combatAttr & DESTROYED)
            return;
        if (ship == ownShip && (shipIndex[ownShip].attr & RACING))
            speed = shipIndex[ownShip].thrust * 2;
        else
            speed = s->speed;
    } else
        speed = 6000;
    if (distance == 0) distance = 1;
    value = (speed * 8) / distance;
    if (value > 80)
        value = 80;
    half = value >> 1;
    size = value / 32;
    quarter = value >> 2;
    if (size < 1)
        size = 1;

    flare[ship][4].x0 = x + size;
    flare[ship][4].y0 = y + size;
    flare[ship][4].x1 = x;
    flare[ship][4].y1 = y;
    flare[ship][4].x2 = x + half + 1;
    flare[ship][4].y2 = y - half + 1;
    flare[ship][4].x3 = x + half;
    flare[ship][4].y3 = y - half;
    flare[ship][5].x0 = x - size;
    flare[ship][5].y0 = y - size;
    flare[ship][5].x1 = x;
    flare[ship][5].y1 = y;
    flare[ship][5].x2 = x + half - 1;
    flare[ship][5].y2 = y - half - 1;
    flare[ship][5].x3 = x + half;
    flare[ship][5].y3 = y - half;
    flare[ship][6].x0 = x + size;
    flare[ship][6].y0 = y + size;
    flare[ship][6].x1 = x;
    flare[ship][6].y1 = y;
    flare[ship][6].x2 = x - half + 1;
    flare[ship][6].y2 = y + half + 1;
    flare[ship][6].x3 = x - half;
    flare[ship][6].y3 = y + half;
    flare[ship][7].x0 = x - size;
    flare[ship][7].y0 = y - size;
    flare[ship][7].x1 = x;
    flare[ship][7].y1 = y;
    flare[ship][7].x2 = x - half - 1;
    flare[ship][7].y2 = y + half - 1;
    flare[ship][7].x3 = x - half;
    flare[ship][7].y3 = y + half;
    flare[ship][8].x0 = x - size;
    flare[ship][8].y0 = y + size;
    flare[ship][8].x1 = x;
    flare[ship][8].y1 = y;
    flare[ship][8].x2 = x - quarter + 1;
    flare[ship][8].y2 = y - quarter + 1;
    flare[ship][8].x3 = x - quarter;
    flare[ship][8].y3 = y - quarter;
    flare[ship][9].x0 = x + size;
    flare[ship][9].y0 = y - size;
    flare[ship][9].x1 = x;
    flare[ship][9].y1 = y;
    flare[ship][9].x2 = x - quarter + 1;
    flare[ship][9].y2 = y - quarter - 1;
    flare[ship][9].x3 = x - quarter;
    flare[ship][9].y3 = y - quarter;
    flare[ship][10].x0 = x - size;
    flare[ship][10].y0 = y + size;
    flare[ship][10].x1 = x;
    flare[ship][10].y1 = y;
    flare[ship][10].x2 = x + quarter - 1;
    flare[ship][10].y2 = y + quarter + 1;
    flare[ship][10].x3 = x + quarter;
    flare[ship][10].y3 = y + quarter;
    flare[ship][11].x0 = x + size;
    flare[ship][11].y0 = y - size;
    flare[ship][11].x1 = x;
    flare[ship][11].y1 = y;
    flare[ship][11].x2 = x + quarter + 1;
    flare[ship][11].y2 = y + quarter - 1;
    flare[ship][11].x3 = x + quarter;
    flare[ship][11].y3 = y + quarter;
    if (distance < 1000) {
        flare[ship][0].x0 = x;
        flare[ship][0].y0 = y;
        flare[ship][0].x1 = x;
        flare[ship][0].y1 = y;
        flare[ship][0].x2 = x + quarter;
        flare[ship][0].y2 = y;
        flare[ship][0].x3 = x;
        flare[ship][0].y3 = y - quarter;
        flare[ship][1].x0 = x;
        flare[ship][1].y0 = y;
        flare[ship][1].x1 = x;
        flare[ship][1].y1 = y;
        flare[ship][1].x2 = x;
        flare[ship][1].y2 = y - quarter;
        flare[ship][1].x3 = x - quarter;
        flare[ship][1].y3 = y;
        flare[ship][2].x0 = x;
        flare[ship][2].y0 = y;
        flare[ship][2].x1 = x;
        flare[ship][2].y1 = y;
        flare[ship][2].x2 = x;
        flare[ship][2].y2 = y + quarter;
        flare[ship][2].x3 = x + quarter;
        flare[ship][2].y3 = y;
        flare[ship][3].x0 = x;
        flare[ship][3].y0 = y;
        flare[ship][3].x1 = x;
        flare[ship][3].y1 = y;
        flare[ship][3].x2 = x - quarter;
        flare[ship][3].y2 = y;
        flare[ship][3].x3 = x;
        flare[ship][3].y3 = y + quarter;
        DrawPolyG4(z, &flare[ship][0], 1);
        DrawPolyG4(z, &flare[ship][1], 1);
        DrawPolyG4(z, &flare[ship][2], 1);
        DrawPolyG4(z, &flare[ship][3], 1);
    }
    DrawPolyG4(z, &flare[ship][4], 1);
    DrawPolyG4(z, &flare[ship][5], 1);
    DrawPolyG4(z, &flare[ship][6], 1);
    DrawPolyG4(z, &flare[ship][7], 1);
    DrawPolyG4(z, &flare[ship][8], 1);
    DrawPolyG4(z, &flare[ship][9], 1);
    DrawPolyG4(z, &flare[ship][10], 1);
    DrawPolyG4(z, &flare[ship][11], 1);
}
#endif
#ifdef EQUIV
#ifdef NETPLAY
/* NET build: the cross arms (flares 0-3) use armLength and the diagonals (8-11) quarterLength, and
   the racing speed is thrust + thrust; MSVC /Od gives the exe this frame and operand order. */
void DrawFlare(int x, int y, int distance, int ship, float z)
{
    int halfLength, size, quarterLength, armLength, shipSpeed;
    if (ship < 15) {
        if (!gameData->trailDetail || (shipIndex[ship].combatAttr & DESTROYED))
            return;
        if (!(ship == ownShip && (shipIndex[ownShip].attr & RACING)))
            shipSpeed = shipIndex[ship].speed;
        else
            shipSpeed = shipIndex[ownShip].thrust + shipIndex[ownShip].thrust;
    } else
        shipSpeed = 6000;
    if (distance == 0) distance = 1;
    shipSpeed = (shipSpeed * 8) / distance;
    if (shipSpeed > 80)
        shipSpeed = 80;
    halfLength = shipSpeed >> 1;
    size = shipSpeed / 32;
    quarterLength = shipSpeed >> 2;
    armLength = shipSpeed >> 2;
    if (size < 1)
        size = 1;

    flare[ship][4].x0 = x + size;
    flare[ship][4].y0 = y + size;
    flare[ship][4].x1 = x;
    flare[ship][4].y1 = y;
    flare[ship][4].x2 = x + halfLength + 1;
    flare[ship][4].y2 = y - halfLength + 1;
    flare[ship][4].x3 = x + halfLength;
    flare[ship][4].y3 = y - halfLength;
    flare[ship][5].x0 = x - size;
    flare[ship][5].y0 = y - size;
    flare[ship][5].x1 = x;
    flare[ship][5].y1 = y;
    flare[ship][5].x2 = x + halfLength - 1;
    flare[ship][5].y2 = y - halfLength - 1;
    flare[ship][5].x3 = x + halfLength;
    flare[ship][5].y3 = y - halfLength;
    flare[ship][6].x0 = x + size;
    flare[ship][6].y0 = y + size;
    flare[ship][6].x1 = x;
    flare[ship][6].y1 = y;
    flare[ship][6].x2 = x - halfLength + 1;
    flare[ship][6].y2 = y + halfLength + 1;
    flare[ship][6].x3 = x - halfLength;
    flare[ship][6].y3 = y + halfLength;
    flare[ship][7].x0 = x - size;
    flare[ship][7].y0 = y - size;
    flare[ship][7].x1 = x;
    flare[ship][7].y1 = y;
    flare[ship][7].x2 = x - halfLength - 1;
    flare[ship][7].y2 = y + halfLength - 1;
    flare[ship][7].x3 = x - halfLength;
    flare[ship][7].y3 = y + halfLength;
    flare[ship][8].x0 = x - size;
    flare[ship][8].y0 = y + size;
    flare[ship][8].x1 = x;
    flare[ship][8].y1 = y;
    flare[ship][8].x2 = x - quarterLength + 1;
    flare[ship][8].y2 = y - quarterLength + 1;
    flare[ship][8].x3 = x - quarterLength;
    flare[ship][8].y3 = y - quarterLength;
    flare[ship][9].x0 = x + size;
    flare[ship][9].y0 = y - size;
    flare[ship][9].x1 = x;
    flare[ship][9].y1 = y;
    flare[ship][9].x2 = x - quarterLength + 1;
    flare[ship][9].y2 = y - quarterLength - 1;
    flare[ship][9].x3 = x - quarterLength;
    flare[ship][9].y3 = y - quarterLength;
    flare[ship][10].x0 = x - size;
    flare[ship][10].y0 = y + size;
    flare[ship][10].x1 = x;
    flare[ship][10].y1 = y;
    flare[ship][10].x2 = x + quarterLength - 1;
    flare[ship][10].y2 = y + quarterLength + 1;
    flare[ship][10].x3 = x + quarterLength;
    flare[ship][10].y3 = y + quarterLength;
    flare[ship][11].x0 = x + size;
    flare[ship][11].y0 = y - size;
    flare[ship][11].x1 = x;
    flare[ship][11].y1 = y;
    flare[ship][11].x2 = x + quarterLength + 1;
    flare[ship][11].y2 = y + quarterLength - 1;
    flare[ship][11].x3 = x + quarterLength;
    flare[ship][11].y3 = y + quarterLength;
    if (distance < 1000) {
        flare[ship][0].x0 = x;
        flare[ship][0].y0 = y;
        flare[ship][0].x1 = x;
        flare[ship][0].y1 = y;
        flare[ship][0].x2 = x + armLength;
        flare[ship][0].y2 = y;
        flare[ship][0].x3 = x;
        flare[ship][0].y3 = y - armLength;
        flare[ship][1].x0 = x;
        flare[ship][1].y0 = y;
        flare[ship][1].x1 = x;
        flare[ship][1].y1 = y;
        flare[ship][1].x2 = x;
        flare[ship][1].y2 = y - armLength;
        flare[ship][1].x3 = x - armLength;
        flare[ship][1].y3 = y;
        flare[ship][2].x0 = x;
        flare[ship][2].y0 = y;
        flare[ship][2].x1 = x;
        flare[ship][2].y1 = y;
        flare[ship][2].x2 = x;
        flare[ship][2].y2 = y + armLength;
        flare[ship][2].x3 = x + armLength;
        flare[ship][2].y3 = y;
        flare[ship][3].x0 = x;
        flare[ship][3].y0 = y;
        flare[ship][3].x1 = x;
        flare[ship][3].y1 = y;
        flare[ship][3].x2 = x - armLength;
        flare[ship][3].y2 = y;
        flare[ship][3].x3 = x;
        flare[ship][3].y3 = y + armLength;
        DrawPolyG4(z, &flare[ship][0], 1);
        DrawPolyG4(z, &flare[ship][1], 1);
        DrawPolyG4(z, &flare[ship][2], 1);
        DrawPolyG4(z, &flare[ship][3], 1);
    }
    DrawPolyG4(z, &flare[ship][4], 1);
    DrawPolyG4(z, &flare[ship][5], 1);
    DrawPolyG4(z, &flare[ship][6], 1);
    DrawPolyG4(z, &flare[ship][7], 1);
    DrawPolyG4(z, &flare[ship][8], 1);
    DrawPolyG4(z, &flare[ship][9], 1);
    DrawPolyG4(z, &flare[ship][10], 1);
    DrawPolyG4(z, &flare[ship][11], 1);
}
#endif
#endif

extern LINE_F2 targetPrims[8][4];
extern char targetFlags[8];
extern char targetvalid;
extern POLY_G3 muzzletop[2];
extern POLY_G3 muzzleleft[2];
extern POLY_G3 muzzleright[2];
extern POLY_G3 muzzlebot[2];
extern short missileLockTimer;

/* Not byte-exact, but equivalent: the ship-trail counter lives in its own variable (the exe keeps it in eax); the target-line register allocation still differs. */
#ifdef EQUIV
#ifdef NETPLAY
/* NET build: the flare colours are chosen with if/else on texture16Bit (the exe tests it again for each channel). */
void InitTrails(void)
{
    int bank, i, segment, j, k, ship;
    unsigned char shadeR, shadeG, shadeB;

    shipTrails = DAlloc(reserveHeap, 15 * sizeof(struct Trail));
    targetvalid = 0;
    for (bank = 0; bank < 8; bank++) {
        for (j = 0; j < 4; j++) {
            SetLineF2(&targetPrims[bank][j]);
            SetSemiTrans(&targetPrims[bank][j], 1);
            targetPrims[bank][j].r0 = 128;
            targetPrims[bank][j].g0 = 128;
            targetPrims[bank][j].b0 = 0;
        }
        targetFlags[bank] = 0;
    }
    for (ship = 0; ship < 15; ship++) {
        shipTrails[ship].current = 0;
        shipTrails[ship].colour[0][0] = 32;
        shipTrails[ship].colour[0][1] = 64;
        shipTrails[ship].colour[0][2] = 255;
        shipTrails[ship].colour[1][0] = 64;
        shipTrails[ship].colour[1][1] = 128;
        shipTrails[ship].colour[1][2] = 255;
        shipTrails[ship].colour[2][0] = 255;
        shipTrails[ship].colour[2][1] = 255;
        shipTrails[ship].colour[2][2] = 255;
        for (segment = 0; segment < TRAIL_SEGMENTS; segment++)
            shipTrails[ship].segment[segment].active = 0;
    }
    for (i = 0; i < 2; i++) {
        SetPolyG3(&muzzletop[i]);
        SetSemiTrans(&muzzletop[i], 1);
        muzzletop[i].r0 = 255;
        muzzletop[i].g0 = 8;
        muzzletop[i].b0 = 8;
        muzzletop[i].r1 = 0;
        muzzletop[i].g1 = 0;
        muzzletop[i].b1 = 0;
        muzzletop[i].r2 = 0;
        muzzletop[i].g2 = 0;
        muzzletop[i].b2 = 0;
        SetPolyG3(&muzzleleft[i]);
        SetSemiTrans(&muzzleleft[i], 1);
        muzzleleft[i].r0 = 255;
        muzzleleft[i].g0 = 8;
        muzzleleft[i].b0 = 8;
        muzzleleft[i].r1 = 0;
        muzzleleft[i].g1 = 0;
        muzzleleft[i].b1 = 0;
        muzzleleft[i].r2 = 0;
        muzzleleft[i].g2 = 0;
        muzzleleft[i].b2 = 0;
        SetPolyG3(&muzzleright[i]);
        SetSemiTrans(&muzzleright[i], 1);
        muzzleright[i].r0 = 255;
        muzzleright[i].g0 = 8;
        muzzleright[i].b0 = 8;
        muzzleright[i].r1 = 0;
        muzzleright[i].g1 = 0;
        muzzleright[i].b1 = 0;
        muzzleright[i].r2 = 0;
        muzzleright[i].g2 = 0;
        muzzleright[i].b2 = 0;
        SetPolyG3(&muzzlebot[i]);
        SetSemiTrans(&muzzlebot[i], 1);
        muzzlebot[i].r0 = 255;
        muzzlebot[i].g0 = 8;
        muzzlebot[i].b0 = 8;
        muzzlebot[i].r1 = 0;
        muzzlebot[i].g1 = 0;
        muzzlebot[i].b1 = 0;
        muzzlebot[i].r2 = 0;
        muzzlebot[i].g2 = 0;
        muzzlebot[i].b2 = 0;
    }
    for (j = 0; j < 19; j++) {
        for (k = 0; k < 12; k++) {
            SetPolyG4(&flare[j][k]);
            SetSemiTrans(&flare[j][k], 1);
            if (j < 15) {
                if (texture16Bit)
                    flare[j][k].r0 = 32;
                else
                    flare[j][k].r0 = 50;
                flare[j][k].g0 = 64;
                flare[j][k].b0 = 255;
                if (texture16Bit)
                    flare[j][k].r1 = 32;
                else
                    flare[j][k].r1 = 50;
                flare[j][k].g1 = 64;
                flare[j][k].b1 = 255;
            } else {
                if (texture16Bit)
                    flare[j][k].r0 = 224;
                else
                    flare[j][k].r0 = 50;
                flare[j][k].g0 = 128;
                flare[j][k].b0 = 32;
                if (texture16Bit)
                    flare[j][k].r1 = 224;
                else
                    flare[j][k].r1 = 50;
                flare[j][k].g1 = 128;
                flare[j][k].b1 = 32;
            }
            if (texture16Bit)
                shadeR = 0;
            else
                shadeR = 50;
            shadeG = 0;
            shadeB = 0;
            if (texture16Bit)
                flare[j][k].r2 = shadeR;
            else
                flare[j][k].r2 = 50;
            flare[j][k].g2 = shadeG;
            flare[j][k].b2 = shadeB;
            if (texture16Bit)
                flare[j][k].r3 = shadeR;
            else
                flare[j][k].r3 = 50;
            flare[j][k].g3 = shadeG;
            flare[j][k].b3 = shadeB;
            if (k > 3) {
                if (texture16Bit)
                    flare[j][k].r0 = shadeR;
                else
                    flare[j][k].r0 = 50;
                flare[j][k].g0 = shadeG;
                flare[j][k].b0 = shadeB;
            }
        }
    }
}
#else
void InitTrails(void)
{
    int bank, i, segment, j, k, ship;

    shipTrails = DAlloc(reserveHeap, 15 * sizeof(struct Trail));
    targetvalid = 0;
    for (bank = 0; bank < 8; bank++) {
        for (j = 0; j < 4; j++) {
            SetLineF2(&targetPrims[bank][j]);
            SetSemiTrans(&targetPrims[bank][j], 1);
            targetPrims[bank][j].r0 = 128;
            targetPrims[bank][j].g0 = 128;
            targetPrims[bank][j].b0 = 0;
        }
        targetFlags[bank] = 0;
    }
    for (ship = 0; ship < 15; ship++) {
        shipTrails[ship].current = 0;
        shipTrails[ship].colour[0][0] = 32;
        shipTrails[ship].colour[0][1] = 64;
        shipTrails[ship].colour[0][2] = 255;
        shipTrails[ship].colour[1][0] = 64;
        shipTrails[ship].colour[1][1] = 128;
        shipTrails[ship].colour[1][2] = 255;
        shipTrails[ship].colour[2][0] = 255;
        shipTrails[ship].colour[2][1] = 255;
        shipTrails[ship].colour[2][2] = 255;
        for (segment = 0; segment < TRAIL_SEGMENTS; segment++)
            shipTrails[ship].segment[segment].active = 0;
    }
    for (i = 0; i < 2; i++) {
        SetPolyG3(&muzzletop[i]);
        SetSemiTrans(&muzzletop[i], 1);
        muzzletop[i].r0 = 255;
        muzzletop[i].g0 = 8;
        muzzletop[i].b0 = 8;
        muzzletop[i].r1 = 0;
        muzzletop[i].g1 = 0;
        muzzletop[i].b1 = 0;
        muzzletop[i].r2 = 0;
        muzzletop[i].g2 = 0;
        muzzletop[i].b2 = 0;
        SetPolyG3(&muzzleleft[i]);
        SetSemiTrans(&muzzleleft[i], 1);
        muzzleleft[i].r0 = 255;
        muzzleleft[i].g0 = 8;
        muzzleleft[i].b0 = 8;
        muzzleleft[i].r1 = 0;
        muzzleleft[i].g1 = 0;
        muzzleleft[i].b1 = 0;
        muzzleleft[i].r2 = 0;
        muzzleleft[i].g2 = 0;
        muzzleleft[i].b2 = 0;
        SetPolyG3(&muzzleright[i]);
        SetSemiTrans(&muzzleright[i], 1);
        muzzleright[i].r0 = 255;
        muzzleright[i].g0 = 8;
        muzzleright[i].b0 = 8;
        muzzleright[i].r1 = 0;
        muzzleright[i].g1 = 0;
        muzzleright[i].b1 = 0;
        muzzleright[i].r2 = 0;
        muzzleright[i].g2 = 0;
        muzzleright[i].b2 = 0;
        SetPolyG3(&muzzlebot[i]);
        SetSemiTrans(&muzzlebot[i], 1);
        muzzlebot[i].r0 = 255;
        muzzlebot[i].g0 = 8;
        muzzlebot[i].b0 = 8;
        muzzlebot[i].r1 = 0;
        muzzlebot[i].g1 = 0;
        muzzlebot[i].b1 = 0;
        muzzlebot[i].r2 = 0;
        muzzlebot[i].g2 = 0;
        muzzlebot[i].b2 = 0;
    }
    for (j = 0; j < 19; j++) {
        for (k = 0; k < 12; k++) {
            SetPolyG4(&flare[j][k]);
            SetSemiTrans(&flare[j][k], 1);
            if (j < 15) {
                flare[j][k].r0 = 32;
                if (!texture16Bit) flare[j][k].r0 = 50;
                flare[j][k].g0 = 64;
                flare[j][k].b0 = 255;
                flare[j][k].r1 = 32;
                if (!texture16Bit) flare[j][k].r1 = 50;
                flare[j][k].g1 = 64;
                flare[j][k].b1 = 255;
            } else {
                flare[j][k].r0 = 224;
                if (!texture16Bit) flare[j][k].r0 = 50;
                flare[j][k].g0 = 128;
                flare[j][k].b0 = 32;
                flare[j][k].r1 = 224;
                if (!texture16Bit) flare[j][k].r1 = 50;
                flare[j][k].g1 = 128;
                flare[j][k].b1 = 32;
            }
            flare[j][k].r2 = texture16Bit ? 0 : 50;
            if (texture16Bit == 0)
                flare[j][k].r2 = 50;
            flare[j][k].g2 = 0;
            flare[j][k].b2 = 0;
            flare[j][k].r3 = texture16Bit ? 0 : 50;
            if (texture16Bit == 0)
                flare[j][k].r3 = 50;
            flare[j][k].g3 = 0;
            flare[j][k].b3 = 0;
            if (k > 3) {
                flare[j][k].r0 = texture16Bit ? 0 : 50;
                if (texture16Bit == 0)
                    flare[j][k].r0 = 50;
                flare[j][k].g0 = 0;
                flare[j][k].b0 = 0;
            }
        }
    }
}
#endif

#endif

/* Drops a new segment into a ship's trail and fades the older ones. */
void NewTrailSegment(int ship, VECTOR *left, VECTOR *right)
{
    int           i;
    short         intensity;
#ifndef NETPLAY
    int           current;
#endif
    TrailSegment *segment;

    intensity = shipIndex[ship].speed >> 1;
    if (intensity > 255)
        intensity = 255;
    for (i = 0; i < TRAIL_SEGMENTS; i++) {
        segment = &shipTrails[ship].segment[i];
        segment->r -= 10;
        segment->g -= 10;
        segment->b -= 10;
        if (segment->r < 0) segment->r = 0;
        if (segment->g < 0) segment->g = 0;
        if (segment->b < 0) segment->b = 0;
    }
#ifdef NETPLAY
    shipTrails[ship].current = (shipTrails[ship].current + (TRAIL_SEGMENTS - 1)) % TRAIL_SEGMENTS;
    segment = &shipTrails[ship].segment[shipTrails[ship].current];
#else
    current = (shipTrails[ship].current + (TRAIL_SEGMENTS - 1)) % TRAIL_SEGMENTS;
    shipTrails[ship].current = current;
    segment = &shipTrails[ship].segment[current];
#endif
    segment->active = 1;
    segment->left.vx = left->vx;
    segment->left.vy = left->vy;
    segment->left.vz = left->vz;
    segment->right.vx = right->vx;
    segment->right.vy = right->vy;
    segment->right.vz = right->vz;
    if (!(newRand() & 7)) {
        segment->r = (shipTrails[ship].colour[1][0] * intensity) >> 8;
        segment->g = (shipTrails[ship].colour[1][1] * intensity) >> 8;
        segment->b = (shipTrails[ship].colour[1][2] * intensity) >> 8;
    } else {
        segment->r = (shipTrails[ship].colour[0][0] * intensity) >> 8;
        segment->g = (shipTrails[ship].colour[0][1] * intensity) >> 8;
        segment->b = (shipTrails[ship].colour[0][2] * intensity) >> 8;
    }
}

/* Lays out a fresh trail behind the player's ship, one step back per segment. */
void BuildOwnShipTrail(int ship, VECTOR *left, VECTOR *right)
{
    int           i;
    short         intensity;
    TrailSegment *segment;

    intensity = shipIndex[ship].speed >> 6;
    if (intensity > 255)
        intensity = 255;
    shipTrails[ship].current = 0;
    for (i = 0; i < TRAIL_SEGMENTS; i++) {
        shipTrails[ship].segment[i].active = 1;
        shipTrails[ship].segment[i].left.vx = left->vx;
        shipTrails[ship].segment[i].left.vy = left->vy;
        shipTrails[ship].segment[i].left.vz = left->vz;
        shipTrails[ship].segment[i].right.vx = right->vx;
        shipTrails[ship].segment[i].right.vy = right->vy;
        shipTrails[ship].segment[i].right.vz = right->vz;
        left->vx -= shipIndex[ownShip].unitVecNose.vx >> 5;
        left->vy -= shipIndex[ownShip].unitVecNose.vy >> 5;
        left->vz -= shipIndex[ownShip].unitVecNose.vz >> 5;
        right->vx -= shipIndex[ownShip].unitVecNose.vx >> 5;
        right->vy -= shipIndex[ownShip].unitVecNose.vy >> 5;
        right->vz -= shipIndex[ownShip].unitVecNose.vz >> 5;
    }
    segment = &shipTrails[ship].segment[0];
    if (!(newRand() & 7)) {
        segment->r = (shipTrails[ship].colour[1][0] * intensity) >> 8;
        segment->g = (shipTrails[ship].colour[1][1] * intensity) >> 8;
        segment->b = (shipTrails[ship].colour[1][2] * intensity) >> 8;
    } else {
        segment->r = (shipTrails[ship].colour[0][0] * intensity) >> 8;
        segment->g = (shipTrails[ship].colour[0][1] * intensity) >> 8;
        segment->b = (shipTrails[ship].colour[0][2] * intensity) >> 8;
    }
    for (i = TRAIL_SEGMENTS - 1; i > 0; i--) {
        shipTrails[ship].segment[i].r = shipTrails[ship].segment[i - 1].r - 10;
        shipTrails[ship].segment[i].g = shipTrails[ship].segment[i - 1].g - 10;
        shipTrails[ship].segment[i].b = shipTrails[ship].segment[i - 1].b - 10;
        if (shipTrails[ship].segment[i].r < 0) shipTrails[ship].segment[i].r = 0;
        if (shipTrails[ship].segment[i].g < 0) shipTrails[ship].segment[i].g = 0;
        if (shipTrails[ship].segment[i].b < 0) shipTrails[ship].segment[i].b = 0;
    }
}

#include "pc_particles.h"
#include "pc_projection.h"
extern float trailMaxBright;

/* Not byte-exact, but equivalent: the trail vertices go to trailStage[vertexCount + k], the same addresses the executable reaches with its stage pointer, advanced by six per segment. */
#ifdef EQUIV
#ifndef NETPLAY
void DrawTrail(TrackCamera *camera, struct Trail *trail, int ship)
{
    SVECTOR midpoint;
    u_short set;
    int next;
    long leftX, leftY, leftZ, rightY, rightZ;
    long screen, p, flag, depth;
    float matrix[16];
    long *cam;
    float distX, distY, distZ;
    ParticleTriangleBuffer *buffer;
    TrailSegment *seg;
    long rightX;
    int skip, i, vertexCount;
    u_char r, g, b, brightest;

    cam = camera->camPos->relative.t;
    distX = (shipIndex[ship].ppivot.vx + cam[0]) * 0.125f;
    distY = (shipIndex[ship].ppivot.vy + cam[1]) * 0.125f;
    distZ = (shipIndex[ship].ppivot.vz + cam[2]) * 0.125f;
    if (GetMagnitude(distX, distY, distZ) < 0)
        return;
    /* This path tests the low combat byte. */
    if (ship < 15 && (shipIndex[ship].combatAttr & 0x80)) return;
    vertexCount = 0;
    set = normalSetNo;
    skip = (trail->current + 29) % TRAIL_SEGMENTS;
    buffer = quadBuckets;
    for (i = 0; i <= set; i++) {
        buffer->count = 0;
        buffer++;
    }
    i = 0;
    do {
        if (i == skip) continue;
        next = (i + 1) % TRAIL_SEGMENTS;
        if (!trail->segment[i].active || !trail->segment[next].active) continue;
        seg = &trail->segment[i];
        leftX = seg->left.vx;
        leftY = seg->left.vy;
        leftZ = seg->left.vz;
        rightX = seg->right.vx;
        rightY = seg->right.vy;
        rightZ = seg->right.vz;
        if (trail->current == i) {
            midpoint.vx = ((leftX + rightX) >> 1) + cam[0];
            midpoint.vy = ((leftY + rightY) >> 1) + cam[1];
            midpoint.vz = ((leftZ + rightZ) >> 1) + cam[2];
        }
        if (gameData->trailDetail == 2) {
            r = seg->r;
            g = seg->g;
            b = seg->b;
            if (r + g + b >= 32) {
                trailStage[vertexCount].x = (leftX + cam[0]) * 0.000244140625f;
                trailStage[vertexCount].y = (leftY + cam[1]) * 0.000244140625f;
                trailStage[vertexCount].z = (leftZ + cam[2]) * 0.000244140625f;
                trailStage[vertexCount].u = 0;
                trailStage[vertexCount].v = 0;
                trailStage[vertexCount].a = translucentAlpha;
                trailStage[vertexCount].r = r >> 4;
                trailStage[vertexCount].g = g >> 4;
                trailStage[vertexCount].b = b >> 4;
                brightest = b > g ? b : g;
                brightest = brightest > r ? brightest : r;
                if ((float)(int)brightest > trailMaxBright) trailMaxBright = (int)brightest;
                trailStage[vertexCount + 1].x = (((leftX + rightX) >> 1) + cam[0]) * 0.000244140625f;
                trailStage[vertexCount + 1].y = (((leftY + rightY) >> 1) + cam[1]) * 0.000244140625f;
                trailStage[vertexCount + 1].z = (((leftZ + rightZ) >> 1) + cam[2]) * 0.000244140625f;
                trailStage[vertexCount + 1].u = 0;
                trailStage[vertexCount + 1].v = 0;
                trailStage[vertexCount + 1].a = opaqueAlpha;
                trailStage[vertexCount + 1].r = r;
                trailStage[vertexCount + 1].g = g;
                trailStage[vertexCount + 1].b = b;
                trailStage[vertexCount + 2].x = (rightX + cam[0]) * 0.000244140625f;
                trailStage[vertexCount + 2].y = (rightY + cam[1]) * 0.000244140625f;
                trailStage[vertexCount + 2].z = (rightZ + cam[2]) * 0.000244140625f;
                trailStage[vertexCount + 2].u = 0;
                trailStage[vertexCount + 2].v = 0;
                trailStage[vertexCount + 2].a = translucentAlpha;
                trailStage[vertexCount + 2].r = r >> 4;
                trailStage[vertexCount + 2].g = g >> 4;
                trailStage[vertexCount + 2].b = b >> 4;
                leftX = trail->segment[next].left.vx;
                leftY = trail->segment[next].left.vy;
                leftZ = trail->segment[next].left.vz;
                rightX = trail->segment[next].right.vx;
                rightY = trail->segment[next].right.vy;
                rightZ = trail->segment[next].right.vz;
                r = trail->segment[next].r;
                g = trail->segment[next].g;
                b = trail->segment[next].b;
                trailStage[vertexCount + 3].x = (leftX + cam[0]) * 0.000244140625f;
                trailStage[vertexCount + 3].y = (leftY + cam[1]) * 0.000244140625f;
                trailStage[vertexCount + 3].z = (leftZ + cam[2]) * 0.000244140625f;
                trailStage[vertexCount + 3].u = 0;
                trailStage[vertexCount + 3].v = 0;
                trailStage[vertexCount + 3].a = translucentAlpha;
                trailStage[vertexCount + 3].r = r >> 4;
                trailStage[vertexCount + 3].g = g >> 4;
                trailStage[vertexCount + 3].b = b >> 4;
                brightest = b > g ? b : g;
                brightest = brightest > r ? brightest : r;
                if ((float)(int)brightest > trailMaxBright) trailMaxBright = (int)brightest;
                trailStage[vertexCount + 4].x = (((leftX + rightX) >> 1) + cam[0]) * 0.000244140625f;
                trailStage[vertexCount + 4].y = (((leftY + rightY) >> 1) + cam[1]) * 0.000244140625f;
                trailStage[vertexCount + 4].z = (((leftZ + rightZ) >> 1) + cam[2]) * 0.000244140625f;
                trailStage[vertexCount + 4].u = 0;
                trailStage[vertexCount + 4].v = 0;
                trailStage[vertexCount + 4].a = opaqueAlpha;
                trailStage[vertexCount + 4].r = r;
                trailStage[vertexCount + 4].g = g;
                trailStage[vertexCount + 4].b = b;
                trailStage[vertexCount + 5].x = (rightX + cam[0]) * 0.000244140625f;
                trailStage[vertexCount + 5].y = (rightY + cam[1]) * 0.000244140625f;
                trailStage[vertexCount + 5].z = (rightZ + cam[2]) * 0.000244140625f;
                trailStage[vertexCount + 5].u = 0;
                trailStage[vertexCount + 5].v = 0;
                trailStage[vertexCount + 5].a = translucentAlpha;
                trailStage[vertexCount + 5].r = r >> 4;
                trailStage[vertexCount + 5].g = g >> 4;
                trailStage[vertexCount + 5].b = b >> 4;
                set = normalSetNo;
                quadBuckets[set].triangles[quadBuckets[set].count].v1 = vertexCount + 4;
                quadBuckets[set].triangles[quadBuckets[set].count].v2 = vertexCount + 1;
                quadBuckets[set].triangles[quadBuckets[set].count].v3 = vertexCount;
                quadBuckets[set].count++;
                quadBuckets[set].triangles[quadBuckets[set].count].v1 = vertexCount + 3;
                quadBuckets[set].triangles[quadBuckets[set].count].v2 = vertexCount + 4;
                quadBuckets[set].triangles[quadBuckets[set].count].v3 = vertexCount;
                quadBuckets[set].count++;
                quadBuckets[set].triangles[quadBuckets[set].count].v1 = vertexCount + 5;
                quadBuckets[set].triangles[quadBuckets[set].count].v2 = vertexCount + 2;
                quadBuckets[set].triangles[quadBuckets[set].count].v3 = vertexCount + 1;
                quadBuckets[set].count++;
                quadBuckets[set].triangles[quadBuckets[set].count].v1 = vertexCount + 4;
                quadBuckets[set].triangles[quadBuckets[set].count].v2 = vertexCount + 5;
                quadBuckets[set].triangles[quadBuckets[set].count].v3 = vertexCount + 1;
                quadBuckets[set].count++;
                vertexCount += 6;
            }
        }
        if (trail->current == i) {
            float x, y, z, tx, ty, tz, sx, sy, denominator, projectedDepth;
            x = midpoint.vx * 0.000244140625f;
            y = midpoint.vy * 0.000244140625f;
            z = midpoint.vz * 0.000244140625f;
            depth = RotTransPers(&midpoint, &screen, &p, &flag);
            ConvertMatrix(matrix, GetSkeletonMatrix(camera->camPos->super));
            tx = (matrix[0] * x + matrix[4] * y) + matrix[8] * z + matrix[12];
            ty = (matrix[1] * x + matrix[9] * z) + matrix[5] * y + matrix[13];
            tz = (matrix[2] * x + matrix[10] * z) + matrix[6] * y + matrix[14];
            sx = viewMatrixFlipped.m[2][0] * tz + viewMatrixFlipped.m[1][0] * ty + viewMatrixFlipped.m[0][0] * tx + viewMatrixFlipped.m[3][0];
            sy = viewMatrixFlipped.m[2][1] * tz + viewMatrixFlipped.m[1][1] * ty + viewMatrixFlipped.m[0][1] * tx + viewMatrixFlipped.m[3][1];
            denominator = viewMatrixFlipped.m[2][2] * tz + viewMatrixFlipped.m[1][2] * ty + viewMatrixFlipped.m[0][2] * tx + viewMatrixFlipped.m[3][2];
            sx = ((sx / denominator * projMatrix.m[0][0]) * 2.0f + 1.0f) * 160.0f;
            projectedDepth = projMatrix.m[3][2] / denominator + projMatrix.m[2][2];
            sy = sy * (projMatrix.m[1][1] / denominator) * 2.0f;
            sy = (1.0f - sy) * 120.0f;
            if ((!sillyShips || ship > 15) && denominator > 0.1)
                DrawFlare((int)sx, (int)sy, depth, ship, projectedDepth * projectedDepth);
            UpdateTarget((int)sx, (int)sy, depth, ship);
        }
    } while (++i < TRAIL_SEGMENTS);
    if (gameData->trailDetail == 2 && (!sillyShips || ship > 15)) {
        PCVertex *vertices;
        ParticleTriangleList *lists;
        char *cursor;
        int count;

        vertices = (PCVertex *)trackVertexCursor;
        memcpy(vertices, trailStage, vertexCount * sizeof(PCVertex));
        lists = (ParticleTriangleList *)(vertices + vertexCount);
        cursor = (char *)(lists + 23);
        trackVertexCursor = cursor;
        set = normalSetNo;
        for (i = 0; i <= set; i++) {
            count = quadBuckets[i].count;
            if (count) {
                lists[i].tris = (D3DTRIANGLE *)cursor;
                lists[i].count = count;
                /* Read again after the header stores, as the original does. */
                count = quadBuckets[i].count;
                memcpy(cursor, quadBuckets[i].triangles, count * sizeof(D3DTRIANGLE));
                cursor += count * sizeof(D3DTRIANGLE);
            } else {
                lists[i].tris = 0;
                lists[i].count = 0;
            }
            trackVertexCursor = cursor;
        }
        AddRenderSkeleton(camera->camPos->super, vertices, vertexCount, lists, 0, 0, 1);
    }
}
#endif
#endif
#ifdef EQUIV
#ifdef NETPLAY
#pragma intrinsic(memcpy)
/* Equivalent with the original transform grouping; a few integer operand loads differ. */
void DrawTrail(TrackCamera *camera, struct Trail *trail, int ship)
{
    float currentTx;
    float cameraX;
    PCVertex *segmentVertices;
    int vertexCount;
    float deltaZ;
    long *cameraVector;
    int trailNextSeg;
    float segmentTy;
    u_char currentGreenColour;
    float transform[16];
    float x;
    long trailLeftX;
    long currentRightSideX;
    long depthCue;
    int trailISeg;
    float zCoord;
    SVECTOR segmentCenter;
    long trailLeftY;
    int segmentCameraDistance;
    int trailSetNumber;
    float currentCenterY;
    float segmentViewZ;
    float currentColourBrightness;
    long segmentScreenPoint;
    long segmentFlag;
    long trailDepth;
    long leftEdgeZ;
    long rightZCoordinate;
    ParticleTriangleList *outputLists;
    u_char trailColorRed;
    u_char currentBlueValue;
    float currentRelativeY;
    long rightSideY;
    int currentSkipIndex;

    cameraVector = camera->camPos->relative.t;
    cameraX = (shipIndex[ship].ppivot.vx + cameraVector[0]) / 8.0f;
    currentRelativeY = (shipIndex[ship].ppivot.vy + cameraVector[1]) / 8.0f;
    deltaZ = (shipIndex[ship].ppivot.vz + cameraVector[2]) / 8.0f;
    segmentCameraDistance = GetMagnitude(cameraX, currentRelativeY, deltaZ);
    if (segmentCameraDistance < 0)
        return;
    /* This path tests the low combat byte. */
    if (ship < 15 && (shipIndex[ship].combatAttr & 0x80)) return;
    currentSkipIndex = (trail->current + 29) % TRAIL_SEGMENTS;
    vertexCount = 0;
    for (trailSetNumber = 0; trailSetNumber <= normalSetNo; trailSetNumber++)
        quadBuckets[trailSetNumber].count = 0;
    for (trailISeg = 0; trailISeg < TRAIL_SEGMENTS; trailISeg++) {
        if (trailISeg != currentSkipIndex) {
            trailNextSeg = (trailISeg + 1) % TRAIL_SEGMENTS;
            if (trail->segment[trailISeg].active && trail->segment[trailNextSeg].active) {
                trailLeftX = trail->segment[trailISeg].left.vx;
                trailLeftY = trail->segment[trailISeg].left.vy;
                leftEdgeZ = trail->segment[trailISeg].left.vz;
                currentRightSideX = trail->segment[trailISeg].right.vx;
                rightSideY = trail->segment[trailISeg].right.vy;
                rightZCoordinate = trail->segment[trailISeg].right.vz;
                if (trail->current == trailISeg) {
                    segmentCenter.vx = ((currentRightSideX + trailLeftX) >> 1) + cameraVector[0];
                    segmentCenter.vy = ((rightSideY + trailLeftY) >> 1) + cameraVector[1];
                    segmentCenter.vz = ((rightZCoordinate + leftEdgeZ) >> 1) + cameraVector[2];
                }
                if (gameData->trailDetail == 2) {
                    trailColorRed = trail->segment[trailISeg].r;
                    currentGreenColour = trail->segment[trailISeg].g;
                    currentBlueValue = trail->segment[trailISeg].b;
                    if (currentBlueValue + currentGreenColour + trailColorRed >= 32) {
                        trailStage[vertexCount].x = (cameraVector[0] + trailLeftX) / 4096.0f;
                        trailStage[vertexCount].y = (cameraVector[1] + trailLeftY) / 4096.0f;
                        trailStage[vertexCount].z = (cameraVector[2] + leftEdgeZ) / 4096.0f;
                        trailStage[vertexCount].u = 0;
                        trailStage[vertexCount].v = 0;
                        trailStage[vertexCount].a = translucentAlpha;
                        trailStage[vertexCount].r = trailColorRed >> 4;
                        trailStage[vertexCount].g = currentGreenColour >> 4;
                        trailStage[vertexCount].b = currentBlueValue >> 4;
                        if ((currentColourBrightness = (float)(int)((currentBlueValue > currentGreenColour ? currentBlueValue : currentGreenColour) > trailColorRed ? (currentBlueValue > currentGreenColour ? currentBlueValue : currentGreenColour) : trailColorRed)) > trailMaxBright) trailMaxBright = currentColourBrightness;
                        trailStage[vertexCount + 1].x = (((currentRightSideX + trailLeftX) >> 1) + cameraVector[0]) / 4096.0f;
                        trailStage[vertexCount + 1].y = (((rightSideY + trailLeftY) >> 1) + cameraVector[1]) / 4096.0f;
                        trailStage[vertexCount + 1].z = (((rightZCoordinate + leftEdgeZ) >> 1) + cameraVector[2]) / 4096.0f;
                        trailStage[vertexCount + 1].u = 0.0f / trailMaxBright;
                        trailStage[vertexCount + 1].v = 0;
                        trailStage[vertexCount + 1].a = opaqueAlpha;
                        trailStage[vertexCount + 1].r = trailColorRed;
                        trailStage[vertexCount + 1].g = currentGreenColour;
                        trailStage[vertexCount + 1].b = currentBlueValue;
                        trailStage[vertexCount + 2].x = (cameraVector[0] + currentRightSideX) / 4096.0f;
                        trailStage[vertexCount + 2].y = (cameraVector[1] + rightSideY) / 4096.0f;
                        trailStage[vertexCount + 2].z = (cameraVector[2] + rightZCoordinate) / 4096.0f;
                        trailStage[vertexCount + 2].u = 0;
                        trailStage[vertexCount + 2].v = 0;
                        trailStage[vertexCount + 2].a = translucentAlpha;
                        trailStage[vertexCount + 2].r = trailColorRed >> 4;
                        trailStage[vertexCount + 2].g = currentGreenColour >> 4;
                        trailStage[vertexCount + 2].b = currentBlueValue >> 4;
                        trailLeftX = trail->segment[trailNextSeg].left.vx;
                        trailLeftY = trail->segment[trailNextSeg].left.vy;
                        leftEdgeZ = trail->segment[trailNextSeg].left.vz;
                        currentRightSideX = trail->segment[trailNextSeg].right.vx;
                        rightSideY = trail->segment[trailNextSeg].right.vy;
                        rightZCoordinate = trail->segment[trailNextSeg].right.vz;
                        trailColorRed = trail->segment[trailNextSeg].r;
                        currentGreenColour = trail->segment[trailNextSeg].g;
                        currentBlueValue = trail->segment[trailNextSeg].b;
                        trailStage[vertexCount + 3].x = (cameraVector[0] + trailLeftX) / 4096.0f;
                        trailStage[vertexCount + 3].y = (cameraVector[1] + trailLeftY) / 4096.0f;
                        trailStage[vertexCount + 3].z = (cameraVector[2] + leftEdgeZ) / 4096.0f;
                        trailStage[vertexCount + 3].u = 0;
                        trailStage[vertexCount + 3].v = 0;
                        trailStage[vertexCount + 3].a = translucentAlpha;
                        trailStage[vertexCount + 3].r = trailColorRed >> 4;
                        trailStage[vertexCount + 3].g = currentGreenColour >> 4;
                        trailStage[vertexCount + 3].b = currentBlueValue >> 4;
                        if ((currentColourBrightness = (float)(int)((currentBlueValue > currentGreenColour ? currentBlueValue : currentGreenColour) > trailColorRed ? (currentBlueValue > currentGreenColour ? currentBlueValue : currentGreenColour) : trailColorRed)) > trailMaxBright) trailMaxBright = currentColourBrightness;
                        trailStage[vertexCount + 4].x = (((currentRightSideX + trailLeftX) >> 1) + cameraVector[0]) / 4096.0f;
                        trailStage[vertexCount + 4].y = (((rightSideY + trailLeftY) >> 1) + cameraVector[1]) / 4096.0f;
                        trailStage[vertexCount + 4].z = (((rightZCoordinate + leftEdgeZ) >> 1) + cameraVector[2]) / 4096.0f;
                        trailStage[vertexCount + 4].u = 0.0f / trailMaxBright;
                        trailStage[vertexCount + 4].v = 0;
                        trailStage[vertexCount + 4].a = opaqueAlpha;
                        trailStage[vertexCount + 4].r = trailColorRed;
                        trailStage[vertexCount + 4].g = currentGreenColour;
                        trailStage[vertexCount + 4].b = currentBlueValue;
                        trailStage[vertexCount + 5].x = (cameraVector[0] + currentRightSideX) / 4096.0f;
                        trailStage[vertexCount + 5].y = (cameraVector[1] + rightSideY) / 4096.0f;
                        trailStage[vertexCount + 5].z = (cameraVector[2] + rightZCoordinate) / 4096.0f;
                        trailStage[vertexCount + 5].u = 0;
                        trailStage[vertexCount + 5].v = 0;
                        trailStage[vertexCount + 5].a = translucentAlpha;
                        trailStage[vertexCount + 5].r = trailColorRed >> 4;
                        trailStage[vertexCount + 5].g = currentGreenColour >> 4;
                        trailStage[vertexCount + 5].b = currentBlueValue >> 4;
                        quadBuckets[normalSetNo].triangles[quadBuckets[normalSetNo].count].v1 = vertexCount + 4;
                        quadBuckets[normalSetNo].triangles[quadBuckets[normalSetNo].count].v2 = vertexCount + 1;
                        quadBuckets[normalSetNo].triangles[quadBuckets[normalSetNo].count].v3 = vertexCount;
                        quadBuckets[normalSetNo].count++;
                        quadBuckets[normalSetNo].triangles[quadBuckets[normalSetNo].count].v1 = vertexCount + 3;
                        quadBuckets[normalSetNo].triangles[quadBuckets[normalSetNo].count].v2 = vertexCount + 4;
                        quadBuckets[normalSetNo].triangles[quadBuckets[normalSetNo].count].v3 = vertexCount;
                        quadBuckets[normalSetNo].count++;
                        quadBuckets[normalSetNo].triangles[quadBuckets[normalSetNo].count].v1 = vertexCount + 5;
                        quadBuckets[normalSetNo].triangles[quadBuckets[normalSetNo].count].v2 = vertexCount + 2;
                        quadBuckets[normalSetNo].triangles[quadBuckets[normalSetNo].count].v3 = vertexCount + 1;
                        quadBuckets[normalSetNo].count++;
                        quadBuckets[normalSetNo].triangles[quadBuckets[normalSetNo].count].v1 = vertexCount + 4;
                        quadBuckets[normalSetNo].triangles[quadBuckets[normalSetNo].count].v2 = vertexCount + 5;
                        quadBuckets[normalSetNo].triangles[quadBuckets[normalSetNo].count].v3 = vertexCount + 1;
                        quadBuckets[normalSetNo].count++;
                        vertexCount += 6;
                    }
                }
                if (trail->current == trailISeg) {
                    x = segmentCenter.vx / 4096.0f;
                    currentCenterY = segmentCenter.vy / 4096.0f;
                    zCoord = segmentCenter.vz / 4096.0f;
                    trailDepth = RotTransPers(&segmentCenter, &segmentScreenPoint, &depthCue, &segmentFlag);
                    ConvertMatrix(transform, GetSkeletonMatrix(camera->camPos->super));
                    currentTx = ((transform[8] * zCoord) + transform[4] * currentCenterY) + transform[0] * x + transform[12];
                    segmentTy = ((transform[9] * zCoord) + transform[5] * currentCenterY) + transform[1] * x + transform[13];
                    segmentViewZ = ((transform[10] * zCoord) + transform[6] * currentCenterY) + transform[2] * x + transform[14];
                    x = ((viewMatrixFlipped.m[1][0] * segmentTy) + viewMatrixFlipped.m[0][0] * currentTx) + viewMatrixFlipped.m[2][0] * segmentViewZ + viewMatrixFlipped.m[3][0];
                    currentCenterY = ((viewMatrixFlipped.m[1][1] * segmentTy) + viewMatrixFlipped.m[0][1] * currentTx) + viewMatrixFlipped.m[2][1] * segmentViewZ + viewMatrixFlipped.m[3][1];
                    zCoord = ((viewMatrixFlipped.m[2][2] * segmentViewZ) + viewMatrixFlipped.m[1][2] * segmentTy) + viewMatrixFlipped.m[0][2] * currentTx + viewMatrixFlipped.m[3][2];
                    currentTx = projMatrix.m[0][0] * x / zCoord * 2.0f;
                    segmentTy = projMatrix.m[1][1] * currentCenterY / zCoord * 2.0f;
                    segmentViewZ = projMatrix.m[3][2] / zCoord + projMatrix.m[2][2];
                    x = (currentTx + 1.0f) * 160.0f;
                    currentCenterY = (-segmentTy + 1.0f) * 120.0f;
                    if ((!sillyShips || ship > 15) && zCoord > 0.1)
                        DrawFlare((int)x, (int)currentCenterY, trailDepth, ship, segmentViewZ * segmentViewZ);
                    UpdateTarget((int)x, (int)currentCenterY, trailDepth, ship);
                }
            }
        }
    }
    if (gameData->trailDetail == 2 && (!sillyShips || ship > 15)) {
        segmentVertices = (PCVertex *)trackVertexCursor;
        trackVertexCursor += vertexCount * sizeof(PCVertex);
        memcpy(segmentVertices, trailStage, vertexCount * sizeof(PCVertex));
        outputLists = (ParticleTriangleList *)trackVertexCursor;
        trackVertexCursor += 23 * sizeof(ParticleTriangleList);
        for (trailSetNumber = 0; trailSetNumber <= normalSetNo; trailSetNumber++) {
            if (quadBuckets[trailSetNumber].count) {
                outputLists[trailSetNumber].count = quadBuckets[trailSetNumber].count;
                outputLists[trailSetNumber].tris = (D3DTRIANGLE *)trackVertexCursor;
                trackVertexCursor += quadBuckets[trailSetNumber].count * sizeof(D3DTRIANGLE);
                memcpy(outputLists[trailSetNumber].tris, quadBuckets[trailSetNumber].triangles, quadBuckets[trailSetNumber].count * sizeof(D3DTRIANGLE));
            } else {
                outputLists[trailSetNumber].count = 0;
                outputLists[trailSetNumber].tris = 0;
            }
        }
        AddRenderSkeleton(camera->camPos->super, segmentVertices, vertexCount, outputLists, 0, 0, 1);
    }
}
#endif
#endif

extern short lastTargetShip;
extern char targetBank;
extern int targetSpin;
extern int pauseRequested;
extern int flashNozzleShip;


/* Not byte-exact, but equivalent: corner calculations agree; integer scheduling and register choices differ. */
#ifdef EQUIV
#ifdef NETPLAY
/* NET build: each store to the box primitives sits behind NET_TARGET_BOX_SET (MSVC /Od keeps a jump over the empty else). */
#define NET_TARGET_BOX_SET 1
/* NET build: the box size is clamped in place and three scaled sizes are kept (the exe stores them and never reads them back). */
void UpdateTarget(int x, int y, int distance, int target)
{
    int boxGap;
    int unitSize;
    int halfExtent;
    int radius;
    int strength;

    if (target < 15) {
        if (shipIndex[target].combatAttr & DESTROYED)
            return;
        if (ownShip != target || !(shipIndex[ownShip].attr & RACING))
            strength = shipIndex[target].speed >> 1;
        else
            strength = shipIndex[ownShip].thrust;
    } else
        strength = 6000;
    if (distance == 0)
        distance = 1;
    strength = (strength << 3) / distance;
    if (strength > 80)
        strength = 80;
    radius = strength >> 1;
    unitSize = strength / 32;
    halfExtent = strength >> 2;
    boxGap = strength >> 2;
    if (unitSize < 1)
        unitSize = 1;
    if (shipIndex[ownShip].targetShip == target && (shipIndex[ownShip].attr & RACING) &&
        (shipIndex[ownShip].weapon == 2 || shipIndex[ownShip].weapon == 5)) {
        radius += missileLockTimer << 3;
        if (radius < 8)
            radius = 8;
        if (NET_TARGET_BOX_SET) {
            targetFlags[targetBank] = 1;
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][0].x0 = x + (short)((radius * rsin(targetSpin)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][0].y0 = y + (short)((radius * rcos(targetSpin)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][0].x1 = x + (short)((radius * rsin(targetSpin + 0x400)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][0].y1 = y + (short)((radius * rcos(targetSpin + 0x400)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][1].x0 = x + (short)((radius * rsin(targetSpin + 0x400)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][1].y0 = y + (short)((radius * rcos(targetSpin + 0x400)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][1].x1 = x + (short)((radius * rsin(targetSpin + 0x800)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][1].y1 = y + (short)((radius * rcos(targetSpin + 0x800)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][2].x1 = x + (short)((radius * rsin(targetSpin + 0x800)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][2].y1 = y + (short)((radius * rcos(targetSpin + 0x800)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][2].x0 = x + (short)((radius * rsin(targetSpin + 0xc00)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][2].y0 = y + (short)((radius * rcos(targetSpin + 0xc00)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][3].x1 = x + (short)((radius * rsin(targetSpin + 0xc00)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][3].y1 = y + (short)((radius * rcos(targetSpin + 0xc00)) >> 12);
        } else {
        }
        if (NET_TARGET_BOX_SET) {
            targetPrims[targetBank][3].x0 = x + (short)((radius * rsin(targetSpin)) >> 12);
        } else {
        }
        targetPrims[targetBank][3].y0 = y + (short)((radius * rcos(targetSpin)) >> 12);
        if (!rgbTextures) {
            targetPrims[targetBank][0].r0 = 10;
            targetPrims[targetBank][1].r0 = 10;
            targetPrims[targetBank][2].r0 = 10;
            targetPrims[targetBank][3].r0 = 10;
        }
        targetPrims[targetBank][0].g0 = 128;
        targetPrims[targetBank][1].g0 = 128;
        targetPrims[targetBank][2].g0 = 128;
        targetPrims[targetBank][3].g0 = 128;
        targetvalid = 1;
    }
}
#else
/* Places the missile lock-on box around the target ship at screen position x, y. */
void UpdateTarget(int x, int y, int distance, int target)
{
    int radius, value, clamped;

    if (target < 15) {
        if (shipIndex[target].combatAttr & DESTROYED)
            return;
        if (ownShip == target && (shipIndex[ownShip].attr & RACING))
            value = shipIndex[ownShip].thrust;
        else
            value = shipIndex[target].speed >> 1;
    } else
        value = 6000;
    if (distance == 0)
        distance = 1;
    value = (value << 3) / distance;
    clamped = value;
    if (clamped > 80)
        clamped = 80;
    radius = clamped >> 1;
    if (shipIndex[ownShip].targetShip == target && (shipIndex[ownShip].attr & RACING) &&
        (shipIndex[ownShip].weapon == 2 || shipIndex[ownShip].weapon == 5)) {
        radius += missileLockTimer << 3;
        if (radius < 8)
            radius = 8;
        targetFlags[targetBank] = 1;
        targetPrims[targetBank][0].x0 = x + ((radius * rsin(targetSpin)) >> 12);
        targetPrims[targetBank][0].y0 = y + ((radius * rcos(targetSpin)) >> 12);
        targetPrims[targetBank][0].x1 = x + ((radius * rsin(targetSpin + 0x400)) >> 12);
        targetPrims[targetBank][0].y1 = y + ((radius * rcos(targetSpin + 0x400)) >> 12);
        targetPrims[targetBank][1].x0 = x + ((radius * rsin(targetSpin + 0x400)) >> 12);
        targetPrims[targetBank][1].y0 = y + ((radius * rcos(targetSpin + 0x400)) >> 12);
        targetPrims[targetBank][1].x1 = x + ((radius * rsin(targetSpin + 0x800)) >> 12);
        targetPrims[targetBank][1].y1 = y + ((radius * rcos(targetSpin + 0x800)) >> 12);
        targetPrims[targetBank][2].x1 = x + ((radius * rsin(targetSpin + 0x800)) >> 12);
        targetPrims[targetBank][2].y1 = y + ((radius * rcos(targetSpin + 0x800)) >> 12);
        targetPrims[targetBank][2].x0 = x + ((radius * rsin(targetSpin + 0xc00)) >> 12);
        targetPrims[targetBank][2].y0 = y + ((radius * rcos(targetSpin + 0xc00)) >> 12);
        targetPrims[targetBank][3].x1 = x + ((radius * rsin(targetSpin + 0xc00)) >> 12);
        targetPrims[targetBank][3].y1 = y + ((radius * rcos(targetSpin + 0xc00)) >> 12);
        targetPrims[targetBank][3].x0 = x + ((radius * rsin(targetSpin)) >> 12);
        targetPrims[targetBank][3].y0 = y + ((radius * rcos(targetSpin)) >> 12);
        if (!rgbTextures) {
            targetPrims[targetBank][0].r0 = 10;
            targetPrims[targetBank][1].r0 = 10;
            targetPrims[targetBank][2].r0 = 10;
            targetPrims[targetBank][3].r0 = 10;
        }
        targetvalid = 1;
        targetPrims[targetBank][0].g0 = 128;
        targetPrims[targetBank][1].g0 = 128;
        targetPrims[targetBank][2].g0 = 128;
        targetPrims[targetBank][3].g0 = 128;
    }
}
#endif
#endif

/* Not byte-exact, but equivalent: the exe keeps the ship and trail offsets in ebx/ebp and the camera in esi; the register allocation of the ship loop still differs. */
#ifdef EQUIV
/* Draws the missile target box, then the trail of every visible ship, the plasma trails
   and the nozzle flash. */
void DrawShipTrails(TrackCamera *camera)
{
#ifdef NETPLAY

    /* Retain the unused compatibility argument of the PC line submission. */
    int i;

    if (shipIndex[ownShip].weapon != 0) {
        if (lastTargetShip != shipIndex[ownShip].targetShip) {
            lastTargetShip = shipIndex[ownShip].targetShip;
#ifdef NETPLAY
            for (i = 0; i < 8; i++)
                targetFlags[i] = 0;
#else
            memset(targetFlags, 0, sizeof(targetFlags));
#endif
        }
        if ((shipIndex[ownShip].attr & RACING) && targetvalid != 0 &&
            (shipIndex[ownShip].weapon == 2 || shipIndex[ownShip].weapon == 5) &&
            shipIndex[ownShip].targetShip < 15) {
            if (missileLockTimer > 0) {
                for (i = 0; i < 8; i++) {
                    if (targetPrims[i][0].g0 > 0) {
                        /* Each corner's green fades twice per frame, as in the original. */
                        targetPrims[i][0].g0 -= 14;
                        targetPrims[i][0].g0 -= 14;
                        targetPrims[i][1].g0 -= 14;
                        targetPrims[i][1].g0 -= 14;
                        targetPrims[i][2].g0 -= 14;
                        targetPrims[i][2].g0 -= 14;
                        targetPrims[i][3].g0 -= 14;
                        targetPrims[i][3].g0 -= 14;
                    }
                    if (targetFlags[i] != 0) {
                        AddLinePrim(0.11f, &targetPrims[i][0], 1);
                        AddLinePrim(0.11f, &targetPrims[i][1], 1);
                        AddLinePrim(0.11f, &targetPrims[i][2], 1);
                        AddLinePrim(0.11f, &targetPrims[i][3], 1);
                    }
                }
            } else {
                AddLinePrim(0.11f, &targetPrims[targetBank][0], 1);
                AddLinePrim(0.11f, &targetPrims[targetBank][1], 1);
                AddLinePrim(0.11f, &targetPrims[targetBank][2], 1);
                AddLinePrim(0.11f, &targetPrims[targetBank][3], 1);
            }
        } else {
#ifdef NETPLAY
            for (i = 0; i < 8; i++)
                targetFlags[i] = 0;
#else
            memset(targetFlags, 0, sizeof(targetFlags));
#endif
        }
        if (!pauseRequested) {
            targetBank = (targetBank + 1) & 7;
            targetSpin += 0x80;
        }
        targetvalid = 0;
        if (missileLockTimer > 0)
            missileLockTimer--;
    }
    for (i = 0; i < 15; i++) {
        if ((shipIndex[i].attr & VISIBLE) && !(shipIndex[i].combatAttr & DESTROYED) &&
            (i != ownShip || !(shipIndex[ownShip].attr & VIEW_IN)))
            DrawTrail(camera, &shipTrails[i], i);
    }
    if (plasmaShotBusy) {
        DrawTrail(camera, &Plasma[0], 0xF);
        DrawTrail(camera, &Plasma[1], 0x10);
        DrawTrail(camera, &Plasma[2], 0x11);
        DrawTrail(camera, &Plasma[3], 0x12);
    }
    if (flashNozzleShip > -1)
        DrawFlashNozzle(flashNozzleShip, camera);
    flashNozzleShip = -1;

#else

    /* Retain the unused compatibility argument of the PC line submission. */
    signed char target;
    ShipData *own;
    u_char weapon;
    ShipData *ship;
    int i;

    own = &shipIndex[ownShip];
    weapon = own->weapon;
    if (weapon != 0) {
        target = own->targetShip;
        if (lastTargetShip != target) {
            lastTargetShip = target;
            memset(targetFlags, 0, sizeof(targetFlags));
        }
        if ((own->attr & RACING) && targetvalid != 0 &&
            (weapon == 2 || weapon == 5) && target < 15) {
            if (missileLockTimer > 0) {
                for (i = 0; i < 8; i++) {
                    if (targetPrims[i][0].g0 != 0) {
                        /* Each corner's green fades twice per frame, as in the original. */
                        targetPrims[i][0].g0 -= 14;
                        targetPrims[i][0].g0 -= 14;
                        targetPrims[i][1].g0 -= 14;
                        targetPrims[i][1].g0 -= 14;
                        targetPrims[i][2].g0 -= 14;
                        targetPrims[i][2].g0 -= 14;
                        targetPrims[i][3].g0 -= 14;
                        targetPrims[i][3].g0 -= 14;
                    }
                    if (targetFlags[i] != 0) {
                        AddLinePrim(0.11f, &targetPrims[i][0], 1);
                        AddLinePrim(0.11f, &targetPrims[i][1], 1);
                        AddLinePrim(0.11f, &targetPrims[i][2], 1);
                        AddLinePrim(0.11f, &targetPrims[i][3], 1);
                    }
                }
            } else {
                AddLinePrim(0.11f, &targetPrims[targetBank][0], 1);
                AddLinePrim(0.11f, &targetPrims[targetBank][1], 1);
                AddLinePrim(0.11f, &targetPrims[targetBank][2], 1);
                AddLinePrim(0.11f, &targetPrims[targetBank][3], 1);
            }
        } else {
            memset(targetFlags, 0, sizeof(targetFlags));
        }
        if (!pauseRequested) {
            targetBank = (targetBank + 1) & 7;
            targetSpin += 0x80;
        }
        targetvalid = 0;
        if (missileLockTimer > 0)
            missileLockTimer--;
    }
    for (i = 0; i < 15; i++) {
        ship = &shipIndex[i];
        if ((ship->attr & VISIBLE) && !(ship->combatAttr & DESTROYED) &&
            (i != ownShip || !(shipIndex[ownShip].attr & VIEW_IN)))
            DrawTrail(camera, &shipTrails[i], i);
    }
    if (plasmaShotBusy != 0) {
        DrawTrail(camera, &Plasma[0], 0xF);
        DrawTrail(camera, &Plasma[1], 0x10);
        DrawTrail(camera, &Plasma[2], 0x11);
        DrawTrail(camera, &Plasma[3], 0x12);
    }
    if (flashNozzleShip > -1)
        DrawFlashNozzle(flashNozzleShip, camera);
    flashNozzleShip = -1;

#endif
}

#endif

/* Extends the trail of every ship that is flying, or clears the trail of one that is not. */
void UpdateShipTrails(ShipData *ships, int allShips)
{
    int    i;
    int    j;
    VECTOR left;
    VECTOR right;

    for (i = 0; i < 15; i++) {
        if (ships[i].attr & VISIBLE) {
            left.vx = ships[i].ppivot.vx - ((ships[i].unitVecNose.vx * 420) >> 12);
            left.vy = ships[i].ppivot.vy - ((ships[i].unitVecNose.vy * 420) >> 12);
            left.vz = ships[i].ppivot.vz - ((ships[i].unitVecNose.vz * 420) >> 12);
            left.vy -= 15;
            right.vx = left.vx;
            right.vy = left.vy;
            right.vz = left.vz;
            left.vx -= (ships[i].unitVecWing.vx * 40) >> 12;
            left.vy -= (ships[i].unitVecWing.vy * 40) >> 12;
            left.vz -= (ships[i].unitVecWing.vz * 40) >> 12;
            right.vx += (ships[i].unitVecWing.vx * 40) >> 12;
            right.vy += (ships[i].unitVecWing.vy * 40) >> 12;
            right.vz += (ships[i].unitVecWing.vz * 40) >> 12;
            if (i != ownShip || allShips) {
                NewTrailSegment(i, &left, &right);
            } else if (!(ships[ownShip].attr & VIEW_IN)) {
                BuildOwnShipTrail(i, &left, &right);
            }
        } else if (shipTrails[i].segment[0].active) {
            for (j = 0; j < TRAIL_SEGMENTS; j++) {
                shipTrails[i].segment[j].active = 0;
            }
        }
    }
}

void InitPlasmaShot(void)
{
    int i;

    Plasma[0].current = 0;
    Plasma[0].colour[0][0] = 0xA0;
    Plasma[0].colour[0][1] = 0x60;
    Plasma[0].colour[0][2] = 0;
    Plasma[0].colour[1][0] = 0xFF;
    Plasma[0].colour[1][1] = 0xFF;
    Plasma[0].colour[1][2] = 0xFF;
    Plasma[1].current = 0;
    Plasma[1].colour[0][0] = 0xA0;
    Plasma[1].colour[0][1] = 0x60;
    Plasma[1].colour[0][2] = 0;
    Plasma[1].colour[1][0] = 0xFF;
    Plasma[1].colour[1][1] = 0xFF;
    Plasma[1].colour[1][2] = 0xFF;
    Plasma[2].current = 0;
    Plasma[2].colour[0][0] = 0xA0;
    Plasma[2].colour[0][1] = 0x60;
    Plasma[2].colour[0][2] = 0;
    Plasma[2].colour[1][0] = 0xFF;
    Plasma[2].colour[1][1] = 0xFF;
    Plasma[2].colour[1][2] = 0xFF;
    Plasma[3].current = 0;
    Plasma[3].colour[0][0] = 0xA0;
    Plasma[3].colour[0][1] = 0x60;
    Plasma[3].colour[0][2] = 0;
    Plasma[3].colour[1][0] = 0xFF;
    Plasma[3].colour[1][1] = 0xFF;
    Plasma[3].colour[1][2] = 0xFF;
    for (i = 0; i < TRAIL_SEGMENTS; i++) {
        Plasma[0].segment[i].active = 0;
        Plasma[1].segment[i].active = 0;
        Plasma[2].segment[i].active = 0;
        Plasma[3].segment[i].active = 0;
    }
    plasmaShotBusy = 1;
    plasmaPhaseA = 0;
    plasmaPhaseB = 0;
    plasmaPhaseC = 0;
}

void NewPlasmaSeg(VECTOR *pos, struct Trail *trail)
{
    int i;
    short intensity;

    intensity = 255;
    for (i = 0; i < TRAIL_SEGMENTS; i++) {
        if (trail->segment[i].active) {
            trail->segment[i].r -= 10;
            trail->segment[i].g -= 10;
            trail->segment[i].b -= 10;
            if (trail->segment[i].r < 0) trail->segment[i].r = 0;
            if (trail->segment[i].g < 0) trail->segment[i].g = 0;
            if (trail->segment[i].b < 0) trail->segment[i].b = 0;
        }
    }
    trail->current = (trail->current + (TRAIL_SEGMENTS - 1)) % TRAIL_SEGMENTS;
    trail->segment[trail->current].active = 1;
    trail->segment[trail->current].left.vx = pos->vx - 40;
    trail->segment[trail->current].left.vy = pos->vy;
    trail->segment[trail->current].left.vz = pos->vz - 40;
    trail->segment[trail->current].right.vx = pos->vx + 40;
    trail->segment[trail->current].right.vy = pos->vy;
    trail->segment[trail->current].right.vz = pos->vz + 40;
    if (!(newRand() & 7)) {
        trail->segment[trail->current].r = (trail->colour[1][0] * intensity) >> 8;
        trail->segment[trail->current].g = (trail->colour[1][1] * intensity) >> 8;
        trail->segment[trail->current].b = (trail->colour[1][2] * intensity) >> 8;
    } else {
        trail->segment[trail->current].r = (trail->colour[0][0] * intensity) >> 8;
        trail->segment[trail->current].g = (trail->colour[0][1] * intensity) >> 8;
        trail->segment[trail->current].b = (trail->colour[0][2] * intensity) >> 8;
    }
}

/* Plasma phases are in 4096ths of a turn. */
#define PLASMA_SIN(angle) ((int)(sin((angle) * 6.28318530718) * 4096.0) >> 4)
#define PLASMA_COS(angle) ((int)(cos((angle) * 6.28318530718) * 4096.0) >> 4)

#ifdef NETPLAY
/* NET build: the steps marked NET_PLASMA_STEP sit behind a constant test with an empty else (MSVC /Od keeps a jump over the else). */
#define NET_PLASMA_STEP 1
/* NET build: the phase angles are passed to the sin/cos macros directly (no angle local), and the drifts are all computed before the points. */
void NewPlasmaSegment(VECTOR *pos)
{
    VECTOR drift;
    VECTOR point;

    plasmaPhaseA += 300;
    plasmaPhaseB += 400;
    if (NET_PLASMA_STEP) {
        plasmaPhaseC += 500;
    } else {
    }

    if (NET_PLASMA_STEP) {
        drift.vx = PLASMA_SIN(plasmaPhaseA / 4096.0);
    } else {
    }
    drift.vy = PLASMA_SIN(plasmaPhaseB / 4096.0);
    point.vx = pos->vx + drift.vx;
    point.vy = pos->vy + drift.vy;
    point.vz = pos->vz;
    if (NET_PLASMA_STEP) {
        NewPlasmaSeg(&point, &Plasma[0]);
    } else {
    }

    if (NET_PLASMA_STEP) {
        drift.vx = PLASMA_SIN(plasmaPhaseB / 4096.0);
    } else {
    }
    drift.vz = PLASMA_SIN(plasmaPhaseC / 4096.0);
    point.vx = pos->vx + drift.vx;
    point.vy = pos->vy;
    point.vz = pos->vz + drift.vz;
    if (NET_PLASMA_STEP) {
        NewPlasmaSeg(&point, &Plasma[1]);
    } else {
    }

    if (NET_PLASMA_STEP) {
        drift.vy = PLASMA_SIN(plasmaPhaseB / 4096.0);
    } else {
    }
    drift.vz = PLASMA_COS(plasmaPhaseC / 4096.0);
    point.vx = pos->vx;
    point.vy = pos->vy + drift.vy;
    point.vz = pos->vz + drift.vz;
    if (NET_PLASMA_STEP) {
        NewPlasmaSeg(&point, &Plasma[2]);
    } else {
    }

    if (NET_PLASMA_STEP) {
        drift.vx = PLASMA_SIN(plasmaPhaseC / 4096.0);
    } else {
    }
    drift.vz = PLASMA_COS(plasmaPhaseA / 4096.0);
    point.vx = pos->vx + drift.vx;
    point.vy = pos->vy;
    point.vz = pos->vz + drift.vz;
    NewPlasmaSeg(&point, &Plasma[3]);
}
#else
void NewPlasmaSegment(VECTOR *pos)
{
    VECTOR offset;
    VECTOR point;
    double angle;

    plasmaPhaseA += 300;
    plasmaPhaseB += 400;
    plasmaPhaseC += 500;

    angle = plasmaPhaseA / 4096.0;
    offset.vx = PLASMA_SIN(angle);
    point.vx = pos->vx + offset.vx;
    angle = plasmaPhaseB / 4096.0;
    offset.vy = PLASMA_SIN(angle);
    point.vy = pos->vy + offset.vy;
    point.vz = pos->vz;
    NewPlasmaSeg(&point, &Plasma[0]);

    angle = plasmaPhaseB / 4096.0;
    offset.vx = PLASMA_SIN(angle);
    point.vx = pos->vx + offset.vx;
    angle = plasmaPhaseC / 4096.0;
    offset.vz = PLASMA_SIN(angle);
    point.vy = pos->vy;
    point.vz = pos->vz + offset.vz;
    NewPlasmaSeg(&point, &Plasma[1]);

    point.vx = pos->vx;
    angle = plasmaPhaseB / 4096.0;
    offset.vy = PLASMA_SIN(angle);
    point.vy = pos->vy + offset.vy;
    angle = plasmaPhaseC / 4096.0;
    offset.vz = PLASMA_COS(angle);
    point.vz = pos->vz + offset.vz;
    NewPlasmaSeg(&point, &Plasma[2]);

    angle = plasmaPhaseC / 4096.0;
    offset.vx = PLASMA_SIN(angle);
    point.vx = pos->vx + offset.vx;
    angle = plasmaPhaseA / 4096.0;
    offset.vz = PLASMA_COS(angle);
    point.vy = pos->vy;
    point.vz = pos->vz + offset.vz;
    NewPlasmaSeg(&point, &Plasma[3]);
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Not byte-exact, but equivalent: the screen points are declared in reverse order so their stack slots match the executable's. */
void DrawFlashNozzle(int shipNum, TrackCamera *camera)
{
    float matrix[16];
    SVECTOR vertex0;
    int yOffset;
    SVECTOR vertex1, vertex2, vertex3;
    DVECTOR screen3, screen2, screen1, screen0;
    long p;
    long flag;
    float x, y, z;
    float tx, ty, tz;
    float depth;
    ShipData *ship;
    long *cam;
    VECTOR *pivot;
    VECTOR *wing;
    VECTOR *nose;
    VECTOR *fin;

    ship = &shipIndex[shipNum];
    yOffset = 0;
    if (ship->attr & VIEW_IN) yOffset = 96;
    cam = camera->camPos->relative.t;
    pivot = &ship->ppivot;
    wing = &ship->unitVecWing;
    nose = &ship->unitVecNose;
    fin = &ship->unitVecFin;
    vertex0.vx = (nose->vx >> 3) - (nose->vx >> 5) + pivot->vx + cam[0];
    vertex0.vy = (nose->vy >> 3) - (nose->vy >> 5) + pivot->vy + cam[1];
    vertex0.vz = (nose->vz >> 3) - (nose->vz >> 5) + pivot->vz + cam[2];
    x = vertex0.vx * 0.000244140625f;
    y = vertex0.vy * 0.000244140625f;
    z = vertex0.vz * 0.000244140625f;
    ConvertMatrix(matrix, GetSkeletonMatrix(camera->camPos->super));
    tz = matrix[10] * z + matrix[6] * y + matrix[2] * x + matrix[14];
    ty = matrix[9] * z + matrix[5] * y + matrix[1] * x + matrix[13];
    tx = matrix[8] * z + matrix[4] * y + matrix[0] * x + matrix[12];
    depth = projMatrix.m[3][2] / (viewMatrixFlipped.m[2][2] * tz + viewMatrixFlipped.m[1][2] * ty + viewMatrixFlipped.m[0][2] * tx + viewMatrixFlipped.m[3][2]) + projMatrix.m[2][2];
    vertex1.vx = (nose->vx >> 3) - (nose->vx >> 5) - (wing->vx >> 7) - (fin->vx >> 6) + pivot->vx + cam[0];
    vertex1.vy = (nose->vy >> 3) - (nose->vy >> 5) - (wing->vy >> 7) - (fin->vy >> 6) + pivot->vy + cam[1];
    vertex1.vz = (nose->vz >> 3) - (nose->vz >> 5) - (wing->vz >> 7) - (fin->vz >> 6) + pivot->vz + cam[2];
    vertex2.vx = (nose->vx >> 3) - (nose->vx >> 5) - (wing->vx >> 6) - (fin->vx >> 7) + pivot->vx + cam[0];
    vertex2.vy = (nose->vy >> 3) - (nose->vy >> 5) - (wing->vy >> 6) - (fin->vy >> 7) + pivot->vy + cam[1];
    vertex2.vz = (nose->vz >> 3) - (nose->vz >> 5) - (wing->vz >> 6) - (fin->vz >> 7) + pivot->vz + cam[2];
    vertex3.vx = (nose->vx >> 3) - (nose->vx >> 5) - (wing->vx >> 5) - (fin->vx >> 5) + pivot->vx + cam[0];
    vertex3.vy = (nose->vy >> 3) - (nose->vy >> 5) - (wing->vy >> 5) - (fin->vy >> 5) + pivot->vy + cam[1];
    vertex3.vz = (nose->vz >> 3) - (nose->vz >> 5) - (wing->vz >> 5) - (fin->vz >> 5) + pivot->vz + cam[2];
    RotTransPers(&vertex3, (long *)&screen3, &p, &flag);
    RotTransPers(&vertex2, (long *)&screen2, &p, &flag);
    RotTransPers(&vertex1, (long *)&screen1, &p, &flag);
    RotTransPers(&vertex0, (long *)&screen0, &p, &flag);
    if (!(flag & 0x80460000)) {
        muzzletop[0].x0 = screen0.vx;
        muzzletop[0].y0 = screen0.vy + yOffset;
        muzzletop[0].x1 = screen1.vx;
        muzzletop[0].y1 = screen1.vy + yOffset;
        muzzletop[0].x2 = screen3.vx;
        muzzletop[0].y2 = screen3.vy + yOffset;
        muzzletop[1].x0 = screen0.vx;
        muzzletop[1].y0 = screen0.vy + yOffset;
        muzzletop[1].x1 = screen2.vx;
        muzzletop[1].y1 = screen2.vy + yOffset;
        muzzletop[1].x2 = screen3.vx;
        muzzletop[1].y2 = screen3.vy + yOffset;
        DrawPolyG3(depth, &muzzletop[0], 1);
        DrawPolyG3(depth, &muzzletop[1], 1);
    }
    vertex1.vx = (nose->vx >> 3) - (nose->vx >> 5) + (wing->vx >> 7) - (fin->vx >> 6) + pivot->vx + cam[0];
    vertex1.vy = (nose->vy >> 3) - (nose->vy >> 5) + (wing->vy >> 7) - (fin->vy >> 6) + pivot->vy + cam[1];
    vertex1.vz = (nose->vz >> 3) - (nose->vz >> 5) + (wing->vz >> 7) - (fin->vz >> 6) + pivot->vz + cam[2];
    vertex2.vx = (nose->vx >> 3) - (nose->vx >> 5) + (wing->vx >> 6) - (fin->vx >> 7) + pivot->vx + cam[0];
    vertex2.vy = (nose->vy >> 3) - (nose->vy >> 5) + (wing->vy >> 6) - (fin->vy >> 7) + pivot->vy + cam[1];
    vertex2.vz = (nose->vz >> 3) - (nose->vz >> 5) + (wing->vz >> 6) - (fin->vz >> 7) + pivot->vz + cam[2];
    vertex3.vx = (nose->vx >> 3) - (nose->vx >> 5) + (wing->vx >> 5) - (fin->vx >> 5) + pivot->vx + cam[0];
    vertex3.vy = (nose->vy >> 3) - (nose->vy >> 5) + (wing->vy >> 5) - (fin->vy >> 5) + pivot->vy + cam[1];
    vertex3.vz = (nose->vz >> 3) - (nose->vz >> 5) + (wing->vz >> 5) - (fin->vz >> 5) + pivot->vz + cam[2];
    RotTransPers(&vertex3, (long *)&screen3, &p, &flag);
    RotTransPers(&vertex2, (long *)&screen2, &p, &flag);
    RotTransPers(&vertex1, (long *)&screen1, &p, &flag);
    if (!(flag & 0x80460000)) {
        muzzlebot[0].x0 = screen0.vx;
        muzzlebot[0].y0 = screen0.vy + yOffset;
        muzzlebot[0].x1 = screen1.vx;
        muzzlebot[0].y1 = screen1.vy + yOffset;
        muzzlebot[0].x2 = screen3.vx;
        muzzlebot[0].y2 = screen3.vy + yOffset;
        muzzlebot[1].x0 = screen0.vx;
        muzzlebot[1].y0 = screen0.vy + yOffset;
        muzzlebot[1].x1 = screen2.vx;
        muzzlebot[1].y1 = screen2.vy + yOffset;
        muzzlebot[1].x2 = screen3.vx;
        muzzlebot[1].y2 = screen3.vy + yOffset;
        DrawPolyG3(depth, &muzzlebot[0], 1);
        DrawPolyG3(depth, &muzzlebot[1], 1);
    }
    vertex1.vx = (nose->vx >> 3) - (nose->vx >> 5) - (wing->vx >> 7) + (fin->vx >> 6) + pivot->vx + cam[0];
    vertex1.vy = (nose->vy >> 3) - (nose->vy >> 5) - (wing->vy >> 7) + (fin->vy >> 6) + pivot->vy + cam[1];
    vertex1.vz = (nose->vz >> 3) - (nose->vz >> 5) - (wing->vz >> 7) + (fin->vz >> 6) + pivot->vz + cam[2];
    vertex2.vx = (nose->vx >> 3) - (nose->vx >> 5) - (wing->vx >> 6) + (fin->vx >> 7) + pivot->vx + cam[0];
    vertex2.vy = (nose->vy >> 3) - (nose->vy >> 5) - (wing->vy >> 6) + (fin->vy >> 7) + pivot->vy + cam[1];
    vertex2.vz = (nose->vz >> 3) - (nose->vz >> 5) - (wing->vz >> 6) + (fin->vz >> 7) + pivot->vz + cam[2];
    vertex3.vx = (nose->vx >> 3) - (nose->vx >> 5) - (wing->vx >> 5) + (fin->vx >> 5) + pivot->vx + cam[0];
    vertex3.vy = (nose->vy >> 3) - (nose->vy >> 5) - (wing->vy >> 5) + (fin->vy >> 5) + pivot->vy + cam[1];
    vertex3.vz = (nose->vz >> 3) - (nose->vz >> 5) - (wing->vz >> 5) + (fin->vz >> 5) + pivot->vz + cam[2];
    RotTransPers(&vertex3, (long *)&screen3, &p, &flag);
    RotTransPers(&vertex2, (long *)&screen2, &p, &flag);
    RotTransPers(&vertex1, (long *)&screen1, &p, &flag);
    if (!(flag & 0x80460000)) {
        muzzleleft[0].x0 = screen0.vx;
        muzzleleft[0].y0 = screen0.vy + yOffset;
        muzzleleft[0].x1 = screen1.vx;
        muzzleleft[0].y1 = screen1.vy + yOffset;
        muzzleleft[0].x2 = screen3.vx;
        muzzleleft[0].y2 = screen3.vy + yOffset;
        muzzleleft[1].x0 = screen0.vx;
        muzzleleft[1].y0 = screen0.vy + yOffset;
        muzzleleft[1].x1 = screen2.vx;
        muzzleleft[1].y1 = screen2.vy + yOffset;
        muzzleleft[1].x2 = screen3.vx;
        muzzleleft[1].y2 = screen3.vy + yOffset;
        DrawPolyG3(depth, &muzzleleft[0], 1);
        DrawPolyG3(depth, &muzzleleft[1], 1);
    }
    vertex1.vx = (nose->vx >> 3) - (nose->vx >> 5) + (wing->vx >> 7) + (fin->vx >> 6) + pivot->vx + cam[0];
    vertex1.vy = (nose->vy >> 3) - (nose->vy >> 5) + (wing->vy >> 7) + (fin->vy >> 6) + pivot->vy + cam[1];
    vertex1.vz = (nose->vz >> 3) - (nose->vz >> 5) + (wing->vz >> 7) + (fin->vz >> 6) + pivot->vz + cam[2];
    vertex2.vx = (nose->vx >> 3) - (nose->vx >> 5) + (wing->vx >> 6) + (fin->vx >> 7) + pivot->vx + cam[0];
    vertex2.vy = (nose->vy >> 3) - (nose->vy >> 5) + (wing->vy >> 6) + (fin->vy >> 7) + pivot->vy + cam[1];
    vertex2.vz = (nose->vz >> 3) - (nose->vz >> 5) + (wing->vz >> 6) + (fin->vz >> 7) + pivot->vz + cam[2];
    vertex3.vx = (nose->vx >> 3) - (nose->vx >> 5) + (wing->vx >> 5) + (fin->vx >> 5) + pivot->vx + cam[0];
    vertex3.vy = (nose->vy >> 3) - (nose->vy >> 5) + (wing->vy >> 5) + (fin->vy >> 5) + pivot->vy + cam[1];
    vertex3.vz = (nose->vz >> 3) - (nose->vz >> 5) + (wing->vz >> 5) + (fin->vz >> 5) + pivot->vz + cam[2];
    RotTransPers(&vertex3, (long *)&screen3, &p, &flag);
    RotTransPers(&vertex2, (long *)&screen2, &p, &flag);
    RotTransPers(&vertex1, (long *)&screen1, &p, &flag);
    if (!(flag & 0x80460000)) {
        muzzleright[0].x0 = screen0.vx;
        muzzleright[0].y0 = screen0.vy + yOffset;
        muzzleright[0].x1 = screen1.vx;
        muzzleright[0].y1 = screen1.vy + yOffset;
        muzzleright[0].x2 = screen3.vx;
        muzzleright[0].y2 = screen3.vy + yOffset;
        muzzleright[1].x0 = screen0.vx;
        muzzleright[1].y0 = screen0.vy + yOffset;
        muzzleright[1].x1 = screen2.vx;
        muzzleright[1].y1 = screen2.vy + yOffset;
        muzzleright[1].x2 = screen3.vx;
        muzzleright[1].y2 = screen3.vy + yOffset;
        DrawPolyG3(depth, &muzzleright[0], 1);
        DrawPolyG3(depth, &muzzleright[1], 1);
    }
}
#endif
#endif
#ifdef EQUIV
#ifdef NETPLAY
/* The final vertex sums use a different operand order, but compute the same values. */
/* The frame names place NET locals in the executable's slots. */
void DrawFlashNozzle(int shipNum, TrackCamera *camera)
{
    VECTOR *noseVec;
    VECTOR *wingPt;
    int offsetYScr;
    float txOff;
    long cue;
    DVECTOR screen3Off;
    float yView;
    long *camPos;
    VECTOR *pivotVec;
    float xform[16];
    float x;
    SVECTOR tip0;
    float y;
    float viewZ;
    SVECTOR tip1;
    float z;
    DVECTOR screen2;
    VECTOR *finDir;
    DVECTOR scr1;
    DVECTOR sc0;
    SVECTOR vertex3Pos;
    long outcode;
    int rtpResult;
    SVECTOR tip2;

    offsetYScr = 0;
    if (shipIndex[shipNum].attr & VIEW_IN)
        offsetYScr = 96;
    camPos = camera->camPos->relative.t;
    pivotVec = &shipIndex[shipNum].ppivot;
    noseVec = &shipIndex[shipNum].unitVecNose;
    wingPt = &shipIndex[shipNum].unitVecWing;
    finDir = &shipIndex[shipNum].unitVecFin;
    tip0.vx = (noseVec->vx >> 3) + pivotVec->vx - (noseVec->vx >> 5) + camPos[0];
    tip0.vy = (noseVec->vy >> 3) + pivotVec->vy - (noseVec->vy >> 5) + camPos[1];
    tip0.vz = (noseVec->vz >> 3) + pivotVec->vz - (noseVec->vz >> 5) + camPos[2];
    x = tip0.vx / 4096.0f;
    y = tip0.vy / 4096.0f;
    z = tip0.vz / 4096.0f;
    ConvertMatrix(xform, GetSkeletonMatrix(camera->camPos->super));
    txOff = ((xform[0] * x) + xform[8] * z) + xform[4] * y + xform[12];
    yView = ((xform[9] * z) + xform[5] * y) + xform[1] * x + xform[13];
    viewZ = ((xform[10] * z) + xform[6] * y) + xform[2] * x + xform[14];
    x = ((viewMatrixFlipped.m[1][0] * yView) + viewMatrixFlipped.m[0][0] * txOff) + viewMatrixFlipped.m[2][0] * viewZ + viewMatrixFlipped.m[3][0];
    y = ((viewMatrixFlipped.m[2][1] * viewZ) + viewMatrixFlipped.m[1][1] * yView) + viewMatrixFlipped.m[0][1] * txOff + viewMatrixFlipped.m[3][1];
    z = ((viewMatrixFlipped.m[1][2] * yView) + viewMatrixFlipped.m[0][2] * txOff) + viewMatrixFlipped.m[2][2] * viewZ + viewMatrixFlipped.m[3][2];
    txOff = projMatrix.m[0][0] * x / z * 2.0f;
    yView = projMatrix.m[1][1] * y / z * 2.0f;
    viewZ = projMatrix.m[3][2] / z + projMatrix.m[2][2];
    x = (txOff + 1.0f) * 160.0f;
    y = (-yView + 1.0f) * 120.0f;
    tip1.vx = (noseVec->vx >> 3) + pivotVec->vx - (noseVec->vx >> 5) - (wingPt->vx >> 7) - (finDir->vx >> 6) + camPos[0];
    tip1.vy = (noseVec->vy >> 3) + pivotVec->vy - (noseVec->vy >> 5) - (wingPt->vy >> 7) - (finDir->vy >> 6) + camPos[1];
    tip1.vz = (noseVec->vz >> 3) + pivotVec->vz - (noseVec->vz >> 5) - (wingPt->vz >> 7) - (finDir->vz >> 6) + camPos[2];
    tip2.vx = (noseVec->vx >> 3) + pivotVec->vx - (noseVec->vx >> 5) - (wingPt->vx >> 6) - (finDir->vx >> 7) + camPos[0];
    tip2.vy = (noseVec->vy >> 3) + pivotVec->vy - (noseVec->vy >> 5) - (wingPt->vy >> 6) - (finDir->vy >> 7) + camPos[1];
    tip2.vz = (noseVec->vz >> 3) + pivotVec->vz - (noseVec->vz >> 5) - (wingPt->vz >> 6) - (finDir->vz >> 7) + camPos[2];
    vertex3Pos.vx = (noseVec->vx >> 3) + pivotVec->vx - (noseVec->vx >> 5) - (finDir->vx >> 5) - (wingPt->vx >> 5) + camPos[0];
    vertex3Pos.vy = (noseVec->vy >> 3) + pivotVec->vy - (noseVec->vy >> 5) - (finDir->vy >> 5) - (wingPt->vy >> 5) + camPos[1];
    vertex3Pos.vz = (noseVec->vz >> 3) + pivotVec->vz - (noseVec->vz >> 5) - (finDir->vz >> 5) - (wingPt->vz >> 5) + camPos[2];
    rtpResult = RotTransPers(&vertex3Pos, (long *)&screen3Off, &cue, &outcode);
    RotTransPers(&tip2, (long *)&screen2, &cue, &outcode);
    RotTransPers(&tip1, (long *)&scr1, &cue, &outcode);
    RotTransPers(&tip0, (long *)&sc0, &cue, &outcode);
    if (!(outcode & 0x80460000)) {
        muzzletop[0].x0 = sc0.vx;
        muzzletop[0].y0 = sc0.vy + offsetYScr;
        muzzletop[0].x1 = scr1.vx;
        muzzletop[0].y1 = scr1.vy + offsetYScr;
        muzzletop[0].x2 = screen3Off.vx;
        muzzletop[0].y2 = screen3Off.vy + offsetYScr;
        muzzletop[1].x0 = sc0.vx;
        muzzletop[1].y0 = sc0.vy + offsetYScr;
        muzzletop[1].x1 = screen2.vx;
        muzzletop[1].y1 = screen2.vy + offsetYScr;
        muzzletop[1].x2 = screen3Off.vx;
        muzzletop[1].y2 = screen3Off.vy + offsetYScr;
        DrawPolyG3(viewZ, &muzzletop[0], 1);
        DrawPolyG3(viewZ, &muzzletop[1], 1);
    }
    tip1.vx = (noseVec->vx >> 3) + pivotVec->vx - (noseVec->vx >> 5) + (wingPt->vx >> 7) - (finDir->vx >> 6) + camPos[0];
    tip1.vy = (noseVec->vy >> 3) + pivotVec->vy - (noseVec->vy >> 5) + (wingPt->vy >> 7) - (finDir->vy >> 6) + camPos[1];
    tip1.vz = (noseVec->vz >> 3) + pivotVec->vz - (noseVec->vz >> 5) + (wingPt->vz >> 7) - (finDir->vz >> 6) + camPos[2];
    tip2.vx = (noseVec->vx >> 3) + pivotVec->vx - (noseVec->vx >> 5) + (wingPt->vx >> 6) - (finDir->vx >> 7) + camPos[0];
    tip2.vy = (noseVec->vy >> 3) + pivotVec->vy - (noseVec->vy >> 5) + (wingPt->vy >> 6) - (finDir->vy >> 7) + camPos[1];
    tip2.vz = (noseVec->vz >> 3) + pivotVec->vz - (noseVec->vz >> 5) + (wingPt->vz >> 6) - (finDir->vz >> 7) + camPos[2];
    vertex3Pos.vx = (noseVec->vx >> 3) + pivotVec->vx - (noseVec->vx >> 5) - (finDir->vx >> 5) + (wingPt->vx >> 5) + camPos[0];
    vertex3Pos.vy = (noseVec->vy >> 3) + pivotVec->vy - (noseVec->vy >> 5) - (finDir->vy >> 5) + (wingPt->vy >> 5) + camPos[1];
    vertex3Pos.vz = (noseVec->vz >> 3) + pivotVec->vz - (noseVec->vz >> 5) - (finDir->vz >> 5) + (wingPt->vz >> 5) + camPos[2];
    RotTransPers(&vertex3Pos, (long *)&screen3Off, &cue, &outcode);
    RotTransPers(&tip2, (long *)&screen2, &cue, &outcode);
    RotTransPers(&tip1, (long *)&scr1, &cue, &outcode);
    if (!(outcode & 0x80460000)) {
        muzzlebot[0].x0 = sc0.vx;
        muzzlebot[0].y0 = sc0.vy + offsetYScr;
        muzzlebot[0].x1 = scr1.vx;
        muzzlebot[0].y1 = scr1.vy + offsetYScr;
        muzzlebot[0].x2 = screen3Off.vx;
        muzzlebot[0].y2 = screen3Off.vy + offsetYScr;
        muzzlebot[1].x0 = sc0.vx;
        muzzlebot[1].y0 = sc0.vy + offsetYScr;
        muzzlebot[1].x1 = screen2.vx;
        muzzlebot[1].y1 = screen2.vy + offsetYScr;
        muzzlebot[1].x2 = screen3Off.vx;
        muzzlebot[1].y2 = screen3Off.vy + offsetYScr;
        DrawPolyG3(viewZ, &muzzlebot[0], 1);
        DrawPolyG3(viewZ, &muzzlebot[1], 1);
    }
    tip1.vx = (noseVec->vx >> 3) + pivotVec->vx - (noseVec->vx >> 5) - (wingPt->vx >> 7) + (finDir->vx >> 6) + camPos[0];
    tip1.vy = (noseVec->vy >> 3) + pivotVec->vy - (noseVec->vy >> 5) - (wingPt->vy >> 7) + (finDir->vy >> 6) + camPos[1];
    tip1.vz = (noseVec->vz >> 3) + pivotVec->vz - (noseVec->vz >> 5) - (wingPt->vz >> 7) + (finDir->vz >> 6) + camPos[2];
    tip2.vx = (noseVec->vx >> 3) + pivotVec->vx - (noseVec->vx >> 5) - (wingPt->vx >> 6) + (finDir->vx >> 7) + camPos[0];
    tip2.vy = (noseVec->vy >> 3) + pivotVec->vy - (noseVec->vy >> 5) - (wingPt->vy >> 6) + (finDir->vy >> 7) + camPos[1];
    tip2.vz = (noseVec->vz >> 3) + pivotVec->vz - (noseVec->vz >> 5) - (wingPt->vz >> 6) + (finDir->vz >> 7) + camPos[2];
    vertex3Pos.vx = (noseVec->vx >> 3) + pivotVec->vx - (noseVec->vx >> 5) + (finDir->vx >> 5) - (wingPt->vx >> 5) + camPos[0];
    vertex3Pos.vy = (noseVec->vy >> 3) + pivotVec->vy - (noseVec->vy >> 5) + (finDir->vy >> 5) - (wingPt->vy >> 5) + camPos[1];
    vertex3Pos.vz = (noseVec->vz >> 3) + pivotVec->vz - (noseVec->vz >> 5) + (finDir->vz >> 5) - (wingPt->vz >> 5) + camPos[2];
    RotTransPers(&vertex3Pos, (long *)&screen3Off, &cue, &outcode);
    RotTransPers(&tip2, (long *)&screen2, &cue, &outcode);
    RotTransPers(&tip1, (long *)&scr1, &cue, &outcode);
    if (!(outcode & 0x80460000)) {
        muzzleleft[0].x0 = sc0.vx;
        muzzleleft[0].y0 = sc0.vy + offsetYScr;
        muzzleleft[0].x1 = scr1.vx;
        muzzleleft[0].y1 = scr1.vy + offsetYScr;
        muzzleleft[0].x2 = screen3Off.vx;
        muzzleleft[0].y2 = screen3Off.vy + offsetYScr;
        muzzleleft[1].x0 = sc0.vx;
        muzzleleft[1].y0 = sc0.vy + offsetYScr;
        muzzleleft[1].x1 = screen2.vx;
        muzzleleft[1].y1 = screen2.vy + offsetYScr;
        muzzleleft[1].x2 = screen3Off.vx;
        muzzleleft[1].y2 = screen3Off.vy + offsetYScr;
        DrawPolyG3(viewZ, &muzzleleft[0], 1);
        DrawPolyG3(viewZ, &muzzleleft[1], 1);
    }
    tip1.vx = (noseVec->vx >> 3) + pivotVec->vx - (noseVec->vx >> 5) + (wingPt->vx >> 7) + (finDir->vx >> 6) + camPos[0];
    tip1.vy = (noseVec->vy >> 3) + pivotVec->vy - (noseVec->vy >> 5) + (finDir->vy >> 6) + (wingPt->vy >> 7) + camPos[1];
    tip1.vz = (noseVec->vz >> 3) + pivotVec->vz - (noseVec->vz >> 5) + (finDir->vz >> 6) + (wingPt->vz >> 7) + camPos[2];
    tip2.vx = (noseVec->vx >> 3) + pivotVec->vx - (noseVec->vx >> 5) + (wingPt->vx >> 6) + (finDir->vx >> 7) + camPos[0];
    tip2.vy = (noseVec->vy >> 3) + pivotVec->vy - (noseVec->vy >> 5) + (wingPt->vy >> 6) + (finDir->vy >> 7) + camPos[1];
    tip2.vz = (noseVec->vz >> 3) + pivotVec->vz - (noseVec->vz >> 5) + (wingPt->vz >> 6) + (finDir->vz >> 7) + camPos[2];
    vertex3Pos.vx = (noseVec->vx >> 3) + pivotVec->vx - (noseVec->vx >> 5) + (finDir->vx >> 5) + (wingPt->vx >> 5) + camPos[0];
    vertex3Pos.vy = (noseVec->vy >> 3) + pivotVec->vy - (noseVec->vy >> 5) + (finDir->vy >> 5) + (wingPt->vy >> 5) + camPos[1];
    vertex3Pos.vz = (noseVec->vz >> 3) + pivotVec->vz - (noseVec->vz >> 5) + (finDir->vz >> 5) + (wingPt->vz >> 5) + camPos[2];
    RotTransPers(&vertex3Pos, (long *)&screen3Off, &cue, &outcode);
    RotTransPers(&tip2, (long *)&screen2, &cue, &outcode);
    RotTransPers(&tip1, (long *)&scr1, &cue, &outcode);
    if (!(outcode & 0x80460000)) {
        muzzleright[0].x0 = sc0.vx;
        muzzleright[0].y0 = sc0.vy + offsetYScr;
        muzzleright[0].x1 = scr1.vx;
        muzzleright[0].y1 = scr1.vy + offsetYScr;
        muzzleright[0].x2 = screen3Off.vx;
        muzzleright[0].y2 = screen3Off.vy + offsetYScr;
        muzzleright[1].x0 = sc0.vx;
        muzzleright[1].y0 = sc0.vy + offsetYScr;
        muzzleright[1].x1 = screen2.vx;
        muzzleright[1].y1 = screen2.vy + offsetYScr;
        muzzleright[1].x2 = screen3Off.vx;
        muzzleright[1].y2 = screen3Off.vy + offsetYScr;
        DrawPolyG3(viewZ, &muzzleright[0], 1);
        DrawPolyG3(viewZ, &muzzleright[1], 1);
    }
}
#endif
#endif
