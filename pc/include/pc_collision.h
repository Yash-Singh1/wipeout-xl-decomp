#ifndef PC_COLLISION_H
#define PC_COLLISION_H

/* (plane - point) . normal divided by -(normal . normal) / 4096 (the squares are shifted per
   component), which is -4096 times the t where point + t * normal meets the plane. No callers yet. */
static __inline int pcPlaneLineCollision(VECTOR plane, VECTOR point, SVector normal)
{
    int product;
    int normalProduct;
    int distance;

    product = (plane.vx - point.vx) * normal.vx;
    product += (plane.vy - point.vy) * normal.vy;
    product += (plane.vz - point.vz) * normal.vz;
    normalProduct = ((-normal.vx * normal.vx) >> 12) +
                    ((-normal.vy * normal.vy) >> 12) +
                    ((-normal.vz * normal.vz) >> 12);
    distance = product / normalProduct;
    return distance;
}

#endif
