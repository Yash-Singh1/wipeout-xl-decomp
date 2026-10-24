#ifndef WIPEOUT_H
#define WIPEOUT_H

#include "psyq.h"
#include "pc.h"

#ifndef BASIC_TYPES_DEFINED
#define BASIC_TYPES_DEFINED
typedef signed char        s8;
typedef unsigned char      u8;
typedef short              s16;
typedef unsigned short     u16;
typedef int                s32;
typedef unsigned int       u32;
typedef __int64            s64;
typedef unsigned __int64   u64;
typedef float              f32;
typedef double             f64;
#endif

/* Basic game types */

typedef struct {
    long vx, vy, vz;
} Vector;

typedef struct {
    short vx, vy, vz;
} SVector;

/* skeleton.c */

/* One joint of an articulated model: its relative and absolute transforms, and the links
   super (parent), sub (child) and next (sibling). */
typedef struct Skeleton {
    MATRIX           relative;
    MATRIX           absolute;
    short            update;
    short            pad;
    struct Skeleton *super;
    struct Skeleton *sub;
    struct Skeleton *next;
} Skeleton;

/* malloc.c */

#include "malloc.h"

/* wadfile.c */

typedef struct WADFILE {
    long  handle;
    short count;
    short current;
    long  sizes[1];
} WADFILE;

/* tim.c */

/* An uploaded texture: where its CLUT and pixels live in VRAM, and the UVs of its quad. */
typedef struct Texture {
    short type;
    short tsb;
    short cba;
    short clutX, clutY, clutW, clutH;
    short textureX, textureY, textureW, textureH;
    u_char u0, v0, u1, v1, u2, v2, u3, v3;
} Texture;

typedef struct TIMlist {
    char *memBase;
    long  memOffset[400];
} TIMlist;

/* object.c: object (PRM) format */

typedef struct Primitive {
    short type;
} Primitive;

typedef struct F3 {
    short   type;
    short   flag;
    short   coords[3];
    short   pad1;
    CVECTOR colour;
} F3;

typedef struct FT3 {
    short   type;
    short   flag;
    short   coords[3];
    short   texture;
    short   cba;
    short   tsb;
    char    u0, v0;
    char    u1, v1;
    char    u2, v2;
    short   pad1;
    CVECTOR colour;
} FT3;

typedef struct F4 {
    short   type;
    short   flag;
    short   coords[4];
    CVECTOR colour;
} F4;

typedef struct FT4 {
    short   type;
    short   flag;
    short   coords[4];
    short   texture;
    short   cba;
    short   tsb;
    char    u0, v0;
    char    u1, v1;
    char    u2, v2;
    char    u3, v3;
    short   pad1;
    CVECTOR colour;
} FT4;

typedef struct G3 {
    short   type;
    short   flag;
    short   coords[3];
    short   pad1;
    CVECTOR colour[3];
} G3;

typedef struct GT3 {
    short   type;
    short   flag;
    short   coords[3];
    short   texture;
    short   cba;
    short   tsb;
    char    u0, v0;
    char    u1, v1;
    char    u2, v2;
    short   pad1;
    CVECTOR colour[3];
} GT3;

typedef struct G4 {
    short   type;
    short   flag;
    short   coords[4];
    CVECTOR colour[4];
} G4;

typedef struct GT4 {
    short   type;
    short   flag;
    short   coords[4];
    short   texture;
    short   cba;
    short   tsb;
    char    u0, v0;
    char    u1, v1;
    char    u2, v2;
    char    u3, v3;
    short   pad1;
    CVECTOR colour[4];
} GT4;

typedef struct LSF3 {
    short   type;
    short   flag;
    short   coords[3];
    short   normal;
    CVECTOR colour;
} LSF3;

typedef struct LSFT3 {
    short   type;
    short   flag;
    short   coords[3];
    short   normal;
    short   texture;
    short   cba;
    short   tsb;
    char    u0, v0;
    char    u1, v1;
    char    u2, v2;
    CVECTOR colour;
} LSFT3;

typedef struct LSF4 {
    short   type;
    short   flag;
    short   coords[4];
    short   normal;
    short   pad1;
    CVECTOR colour;
} LSF4;

typedef struct LSFT4 {
    short   type;
    short   flag;
    short   coords[4];
    short   normal;
    short   texture;
    short   cba;
    short   tsb;
    char    u0, v0;
    char    u1, v1;
    char    u2, v2;
    char    u3, v3;
    CVECTOR colour;
} LSFT4;

typedef struct LSG3 {
    short   type;
    short   flag;
    short   coords[3];
    short   normals[3];
    CVECTOR colour[3];
} LSG3;

typedef struct LSGT3 {
    short   type;
    short   flag;
    short   coords[3];
    short   normals[3];
    short   texture;
    short   cba;
    short   tsb;
    char    u0, v0;
    char    u1, v1;
    char    u2, v2;
    CVECTOR colour[3];
} LSGT3;

typedef struct LSG4 {
    short   type;
    short   flag;
    short   coords[4];
    short   normals[4];
    CVECTOR colour[4];
} LSG4;

typedef struct LSGT4 {
    short   type;
    short   flag;
    short   coords[4];
    short   normals[4];
    short   texture;
    short   cba;
    short   tsb;
    char    u0, v0;
    char    u1, v1;
    char    u2, v2;
    char    u3, v3;
    short   pad1;
    CVECTOR colour[4];
} LSGT4;

typedef struct SPR {
    short   type;
    short   flag;
    short   coord;
    short   width;
    short   height;
    short   texture;
    CVECTOR colour;
} SPR;

typedef struct Spline {
    short   type;
    short   flag;
    VECTOR  control1;
    VECTOR  position;
    VECTOR  control2;
    CVECTOR colour;
} Spline;

typedef struct PointLight {
    short   type;
    short   flag;
    VECTOR  position;
    CVECTOR colour;
    short   startFalloff;
    short   endFalloff;
} PointLight;

typedef struct SpotLight {
    short   type;
    short   flag;
    VECTOR  position;
    SVECTOR direction;
    CVECTOR colour;
    short   startFalloff;
    short   endFalloff;
    short   coneAngle;
    short   spreadAngle;
} SpotLight;

typedef struct InfiniteLight {
    short   type;
    short   flag;
    SVECTOR direction;
    CVECTOR colour;
} InfiniteLight;

/* A pointer to one primitive of an object's list, viewed as any of its primitive types. */
typedef union Poly {
    char          *ptr;
    Primitive     *prim;
    F3            *f3;
    FT3           *ft3;
    F4            *f4;
    FT4           *ft4;
    G3            *g3;
    GT3           *gt3;
    G4            *g4;
    GT4           *gt4;
    LSF3          *lsf3;
    LSFT3         *lsft3;
    LSF4          *lsf4;
    LSFT4         *lsft4;
    LSG3          *lsg3;
    LSGT3         *lsgt3;
    LSG4          *lsg4;
    LSGT4         *lsgt4;
    SPR           *spr;
    Spline        *spline;
    InfiniteLight *infiniteLight;
    PointLight    *pointLight;
    SpotLight     *spotLight;
} Poly;

/* Primitive.type */
#define TypeF3            1
#define TypeFT3           2
#define TypeF4            3
#define TypeFT4           4
#define TypeG3            5
#define TypeGT3           6
#define TypeG4            7
#define TypeGT4           8
#define TypeLF2           9
#define TypeTSPR          10
#define TypeBSPR          11
#define TypeLSF3          12
#define TypeLSFT3         13
#define TypeLSF4          14
#define TypeLSFT4         15
#define TypeLSG3          16
#define TypeLSGT3         17
#define TypeLSG4          18
#define TypeLSGT4         19
#define TypeSpline        20
#define TypeInfiniteLight 21
#define TypePointLight    22
#define TypeSpotLight     23

/* Primitive flag */
#define SingleSided 0x0001
#define ShipEngine  0x0002
#define Translucent 0x0004

/* Binary space partition node of an object: its primitives and the front and back children. */
typedef struct BspTree {
    short           primitiveCount;
    Primitive      *primitives;
    struct BspTree *front;
    struct BspTree *back;
} BspTree;

struct Skeleton;

/* PC model vertex: a float copy of an Object vertex (0x20 bytes). */
typedef struct FloatVertex {
    float x, y, z;
    float rhw;
    unsigned long color;
    unsigned long specular;
    float tu, tv;
} FloatVertex;

/* A model (PRM object): vertices, normals, primitives, an optional skeleton and the next object in a list. */
typedef struct Object {
    char             name[16];
    short            vertexCount;
    SVECTOR         *vertices;
    short            normalCount;
    union {
        SVECTOR         *normals;
        struct TriList  *triLists;  /* PC: one triangle list per texture set (normalCount is 12345 then) */
    };
    short            primitiveCount;
    Primitive       *primitives;
    struct SpriteQuad *spriteQuads; /* PC: billboard records allocated by the scene loader */
    union {
        BspTree     *bspTree;
        FloatVertex *floatVerts;    /* PC: float vertices, 0x20 bytes each (the shark code reads these) */
    };
    struct Skeleton *skeleton;
    long             extent;
    short            spriteQuadCount;
    short            index;
    struct Object   *next;
} Object;

/* track.c */

struct TrackSection;

/* A link between track sections: a pointer once the track is loaded, or an index before that. */
typedef union SectionPtr {
    struct TrackSection *ptr;
    long                 index;
} SectionPtr;

/* Face.flags */
#define TRACK_BASE       0x1
#define WEAPON_GRID_LEFT 0x2
#define FLIP             0x4
#define WEAPON_GRID_RIGHT 0x8
#define START_GRID       0x10
#define SPEED_UP         0x20
#define CHECKPOINT       0x80

/* One track face: four vertex indices, its normal, texture number, flags (see Face.flags) and colour. */
typedef struct Face {
    short   vertex[4];
    SVector normal;
    char    texture;
    u_char  flags;
    char    red;
    char    green;
    char    blue;
} Face;

/* One section of the track: links to its neighbours and junction, view lists for each direction,
   level-of-detail triangle counts, the face range, radii and flags. */
typedef struct TrackSection {
    SectionPtr junction;
    SectionPtr prevSection;
    SectionPtr nextSection;
    Vector     centre;
    short      version;
    short      pad;
    short     *objects;
    short      objectCount;
    short     *northViewSection[3];
    short     *southViewSection[3];
    short     *eastViewSection[3];
    short     *westViewSection[3];
    short     *allViewSection[3];
    short      northViewCount[3];
    short      southViewCount[3];
    short      eastViewCount[3];
    short      westViewCount[3];
    short      allViewCount[3];
    short      high[4];
    short      med[4];
    short      faceStart;
    short      faceCount;
    short      globalRadius;
    short      localRadius;
    short      flags;
    short      secNo;
    signed char bump;
    char       snow;
} TrackSection;

/* The loaded track mesh: vertex and face arrays, the track sections and the view list. */
typedef struct TrackData {
    long          vertexCount;
    long          faceCount;
    long          sectionCount;
    VECTOR       *vertices;
    Face         *faces;
    TrackSection *sections;
    short        *viewList;
    struct Colors *colCopy;
} TrackData;

/* Racing line: one point per track section; distance is how far along the track it lies. */
typedef struct RaceLine {
    short   distance;
    u_short pad;
    long    x, y, z;
} RaceLine;

/* camera.c */

typedef struct TrackCamera {
    Skeleton     *camPos;
    TrackSection *section;
    short         hdg, pitch, roll;
    short         vhdg, vpitch, vroll;
    SVECTOR       vel;
} TrackCamera;

/* ships.c */

struct Object;
struct WeaponData;

/* ShipData.attr */
#define IN_TOW        0x1
#define REMOTE_VIEW   0x2
#define VIEW_IN       0x4
#define DIRECTION     0x8
#define FLYING        0x10
#define LSIDE         0x20
#define RACING        0x40
#define SHIP_COLL     0x80
#define ON_JUNCTION   0x100
#define VISIBLE       0x200
#define RESCUE        0x400
#define OVERTAKE      0x800
#define JUST_IN_FRONT 0x1000
#define JUNC_LEFT     0x2000
#define SHIELDED      0x4000
#define ELECTROED     0x8000

/* ShipData.combatAttr */
#define DESTROYED     0x8000

#ifdef NETPLAY
/* A 16-byte asynchronous message from another machine (weapon fire and the like). */
typedef struct NetAsynch {
    u_char data[16];
} NetAsynch;
#endif

/* One ship in a race (player, computer drone or ghost): position, attitude, speed and thrust,
   weapons, race progress and lap times. */
typedef struct ShipData {
    short         highResTableIndex;
    short         lowResTableIndex;
    TrackSection *nearTrkSect;
    TrackSection *prevShipSect;
    int           attr;
    VECTOR        unitVecNose;
    VECTOR        unitVecWing;
    VECTOR        unitVecFin;
    VECTOR        ppivot;
    VECTOR        vpivot;
    VECTOR        apivot;
    short         hdg, pitch, roll;
    short         vhdg, vpitch, vroll;
    short         ahdg;
    short         mass;
    VECTOR        thrustVector;
    short         r_brake_rot;
    short         l_brake_rot;
    u_short       speed;
    short         reserved96;
    short         thrust;
    short         maxThrust;
    short         thrustCount;
    short         noAttitudeControl;
    short         headingInc;
    short         maxHeading;
    short         resistance;
    short         skid;
    short         lateralOffset;
    short         moveDist;
    u_short       combatAttr;
    short         boostCount;
    union {
        struct {
            short distance;
            short trackDistance;
        };
        struct {
            short distance;
            short trackDistance;
        } raceProgress;
    };
    u_short       remoteMagnitude;
    u_short       remoteMax;
    u_char        weapon;
    u_char        lastWeapon;
    u_char        rearFire;
    signed char   targetShip;
    signed char   rearTarget;
    u_char        padBD;
    short         electroCount;
    short         autopilotCount;
    short         stallCount;
    signed char   lap;
    signed char   reverseLap;
    short         nearSecNo;
    union {
        struct {
            short prevSecNo;
            short pad_CA;
            int lapTimes[5];
        };
        int raceLapTimes[6];
    };
    int           updateCount;
    int           reservedE4;
    int           startY;       /* hover height: ppivot.vy at race start, the ship bobs around it */
#ifdef NETPLAY
    /* NET-WOXL ShipData is 1028 bytes and update sits at 0x400; the fields in between are not all identified yet. */
    signed char   netPlace;     /* 0xec: finishing place, -1 while racing */
    signed char   netEd;        /* 0xed: set to -1 at the start of a network race */
    u_char        netPad0[2];
    int           netProximity; /* 0xf0: proximity mask the network code maintains per ship */
    u_char        netF4, netF5, netF6, netF7;   /* cleared at the start of a network race */
    u_char        netF8;
    u_char        netF9, netFA;
    u_char        netFB;
    int           netFC;
    u_char        asynchIn;     /* queue of asynchronous messages from the player: write index */
    u_char        asynchOut;    /* read index */
    u_char        netPad2[2];
    int           asynchTime[32];       /* event number each message was received at */
    NetAsynch     asynchQueue[32];
    u_char        netActive;    /* 1 while the player is in the race */
    u_char        netSlot;      /* index of the player in the lockstep player list */
    u_char        netPad1[2];
    int           netId;        /* the player's GST id */
    char          netName[100];
    char          netDescription[10];
    u_short       net3FA, net3FC, net3FE;       /* cleared at the start of a network race */
#endif
    void        (*update)();
} ShipData;

/* droid.c */

typedef struct RescueData {
    TrackSection *section;
    short         modelID;
    short         pad;
    VECTOR        pos;
    VECTOR        vel;
    VECTOR        acc;
    short         hdg, pitch, roll;
    short         vhdg, vpitch, vroll;
    short         updateCount;
    short         pad2;
    void        (*droidUpdate)(struct RescueData *droid, ShipData *ships, struct Object **shipShapes);
} RescueData;

typedef void (*CameraUpdateFunc)(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);

/* menus.c */

#include "config.h"

/* C library */
int sprintf(char *buf, const char *fmt, ...);
int printf(const char *fmt, ...);

#define NonFatal 1
#define Fatal    2

#endif
