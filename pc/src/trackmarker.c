#include "functions.h"
#include "race_globals.h"
#include "mathinline.h"

typedef union TrackMarkerPrefix {
    struct {
        unsigned char reserved[0x10];
        POLY_FT4 primitive;
    } render;
    struct {
        unsigned char reserved[0x8e];
        short faceStart;
    } track;
} TrackMarkerPrefix;

typedef struct TrackMarker {
    TrackMarkerPrefix prefix;
    unsigned char reserved[0xfb0 - sizeof(TrackMarkerPrefix)];
    Skeleton *skeleton;
} TrackMarker;

extern Texture *trackMarkerTexture;

#ifdef EQUIV
/* Equivalent: the exe interleaves the texture byte loads with the stores; this build's scheduling differs. */
void InitTrackMarker(TrackMarker *marker, Skeleton *parent)
{
    Texture *texture;

    marker->skeleton = NewSkeleton();
    marker->skeleton->super = parent;
    marker->prefix.render.primitive.r0 = 0x7f;
    marker->prefix.render.primitive.g0 = 0x7f;
    marker->prefix.render.primitive.b0 = 0x7f;
    texture = trackMarkerTexture;
    marker->prefix.render.primitive.u0 = texture->u0;
    marker->prefix.render.primitive.v0 = texture->v0;
    marker->prefix.render.primitive.u1 = texture->u1;
    marker->prefix.render.primitive.v1 = texture->v1;
    marker->prefix.render.primitive.u2 = texture->u2;
    marker->prefix.render.primitive.v2 = texture->v2;
    marker->prefix.render.primitive.u3 = texture->u3;
    marker->prefix.render.primitive.v3 = texture->v3;
    marker->prefix.render.primitive.tpage = texture->tsb;
    marker->prefix.render.primitive.clut = texture->cba;
    SetPolyFT4(&marker->prefix.render.primitive);
}

#endif

#ifdef EQUIV
/* Projects the marker onto the screen and lays out its billboard quad.
   Equivalent with a relocated frame; projection outputs share the quad scratch. */
void UpdateTrackMarker(TrackMarker *quad, TrackMarker *marker, short angle)
{
    union {
        DVECTOR screen;
        long leftY;
    } vertical;
    SVECTOR origin;
    union {
        long flags;
        long leftX;
    } side;
    union {
        long projection;
        int turnAngle;
    } rotation;
    long scale, depth;
    short vertexIndex;
    VECTOR *position;
    short halfWidth;
    int centerX, centerY;
    short left, right, top, bottom;
    long shiftX, shiftY;

    /* the marker sits on a vertex of the face after the section's first face */
    vertexIndex = track->faces[marker->prefix.track.faceStart + 1].vertex[2];
    position = &track->vertices[vertexIndex];
    SetSkeletonPosition(quad->skeleton,
                        position->vx, position->vy, position->vz);
    SetSkeletonDirection(quad->skeleton, 0, 0, 0);
    quad->skeleton->update = 1;
    SetSkeletonTransform(quad->skeleton);

    origin.vx = 0;
    origin.vy = 0;
    origin.vz = 0;
    depth = RotTransPers(&origin, (long *)&vertical.screen, &rotation.projection, &side.flags);
    centerX = vertical.screen.vx;
    centerY = vertical.screen.vy;
    if (!(side.flags & 0x80460000)) {
        scale = 0xa000 / depth;
        halfWidth = (short)(scale >> 1);

        /* turn the marker, then turn the projection a quarter turn the other
           way: both angles are 4096 units to the full circle */
        rotation.turnAngle = angle;
        side.leftX = (rcos(rotation.turnAngle) * halfWidth) >> 12;
        vertical.leftY = (rsin(rotation.turnAngle) * halfWidth) >> 12;
        shiftX = (rcos(1024 - rotation.turnAngle) * scale) >> 12;
        shiftY = (rsin(1024 - rotation.turnAngle) * scale) >> 12;

        left = centerX - side.leftX;
        right = centerX + side.leftX;
        top = centerY - vertical.leftY;
        bottom = centerY + vertical.leftY;

        quad->prefix.render.primitive.x0 = left + shiftX;
        quad->prefix.render.primitive.y0 = top - shiftY;
        quad->prefix.render.primitive.x1 = right + shiftX;
        quad->prefix.render.primitive.y1 = bottom - shiftY;
        quad->prefix.render.primitive.x2 = left;
        quad->prefix.render.primitive.y2 = top;
        quad->prefix.render.primitive.x3 = right;
        quad->prefix.render.primitive.y3 = bottom;
    }
}
#endif
