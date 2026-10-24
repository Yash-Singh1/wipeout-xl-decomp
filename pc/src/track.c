#include "functions.h"
#include "track.h"
#include "camera_facing.h"

extern TrackData *track;


/* Byte-swaps every loaded vertex to the PC byte order. */
void IntelVertex(void)
{
    short i;
    VECTOR *vertex;
    vertex = track->vertices;
    for (i = 0; i < track->vertexCount; i++) {
        IntelLong(&vertex->vx);
        IntelLong(&vertex->vy);
        IntelLong(&vertex->vz);
        vertex++;
    }
}

/* Byte-swaps every loaded face to the PC byte order. */
void IntelFace(void)
{
    Face *face;
    short i;
    face = track->faces;
    for (i = 0; i < track->faceCount; i++) {
        IntelShort(&face->vertex[0]);
        IntelShort(&face->vertex[1]);
        IntelShort(&face->vertex[2]);
        IntelShort(&face->vertex[3]);
        IntelShort(&face->normal.vx);
        IntelShort(&face->normal.vy);
        IntelShort(&face->normal.vz);
        face++;
    }
}

/* Byte-swaps every loaded section to the PC byte order. */
void IntelSection(void)
{
    TrackSection *section;
    short i;
    short j;
    section = track->sections;
    for (i = 0; i < track->sectionCount; i++) {
        IntelLong(&section->junction.index);
        IntelLong(&section->prevSection.index);
        IntelLong(&section->nextSection.index);
        IntelLong(&section->centre.vx);
        IntelLong(&section->centre.vy);
        IntelLong(&section->centre.vz);
        IntelShort(&section->version);
        IntelShort(&section->pad);
        for (j = 0; j < 3; j++) {
            IntelShort(&section->northViewCount[j]);
            IntelShort(&section->southViewCount[j]);
            IntelShort(&section->eastViewCount[j]);
            IntelShort(&section->westViewCount[j]);
            IntelShort(&section->allViewCount[j]);
        }
        for (j = 0; j < 4; j++) {
            IntelShort(&section->high[j]);
            IntelShort(&section->med[j]);
        }
        IntelShort(&section->faceStart);
        IntelShort(&section->faceCount);
        IntelShort(&section->globalRadius);
        IntelShort(&section->localRadius);
        IntelShort(&section->flags);
        IntelShort(&section->secNo);
        section++;
    }
}

/* Rejects a track whose sections are not in the version 8 format this build reads. */
void CheckVersion(void)
{
    TrackSection *section;
    short i;
    section = track->sections;
    for (i = 0; i < track->sectionCount; i++) {
        if (section->version != 8)
            Error("Convert track with track10", Fatal);
        section++;
    }
}

/* Resolves section indices into track section pointers. */
void IndexToPointers(void)
{
    TrackSection *section;
    short i;
    section = track->sections;
    for (i = 0; i < track->sectionCount; i++) {
        section->prevSection.ptr = track->sections + section->prevSection.index;
        section->nextSection.ptr = track->sections + section->nextSection.index;
        if (section->junction.index != -1)
            section->junction.ptr = track->sections + section->junction.index;
        section++;
    }
}

/* Loads the vertex array from a WAD track file. */
void WADInitVertices(WADFILE *wad)
{
    long length;
    length = wad->sizes[wad->current];
    track->vertices = DAlloc(heap, length + 32);
    if (!track->vertices)
        Error("Track.c:InitVertices: Failed to allocate memory for track vertices", Fatal);
    readnextfile(wad, (char *)track->vertices);
    track->vertexCount = length / sizeof(VECTOR);
    IntelVertex();
}

/* Loads the face array from a WAD track file. */
void WADInitFaces(WADFILE *wad)
{
    long length;
    short i;
    length = wad->sizes[wad->current];
    track->faces = DAlloc(heap, length + 32);
    if (!track->faces)
        Error("Track.c:InitFaces: Failed to allocate memory for track faces", Fatal);
    readnextfile(wad, (char *)track->faces);
    track->faceCount = length / sizeof(Face);
    IntelFace();
    track->colCopy = DAlloc(heap, track->faceCount * sizeof(Colors));
    /* The original tests track->faces here, not the colour copy it has just allocated. */
    if (!track->faces)
        Error("Track.c:InitFaces: Failed to allocate memory for copy of colors", Fatal);
    for (i = 0; i < track->faceCount; i++) {
        track->colCopy[i].red = track->faces[i].red;
        track->colCopy[i].green = track->faces[i].green;
        track->colCopy[i].blue = track->faces[i].blue;
    }
}

/* Loads and links track sections from a WAD file. */
void WADInitSections(WADFILE *wad)
{
    long length;
    length = wad->sizes[wad->current];
    track->sections = DAlloc(heap, length + 32);
    if (!track->sections)
        Error("Track.c:InitSections: Failed to allocate memory for track sections", Fatal);
    readnextfile(wad, (char *)track->sections);
    track->sectionCount = length / sizeof(TrackSection);
    IntelSection();
    IndexToPointers();
    CheckVersion();
}

/* Loads the visibility list for a WAD track. */
void WADInitViewList(WADFILE *wad)
{
    TrackSection *section;
    long length;
    long total;
    short i;
    short j;
    length = wad->sizes[wad->current];
    track->viewList = DAlloc(heap, length + 32);
    if (!track->viewList)
        Error("Track.c:InitViewList: Failed to allocate memory for view list", Fatal);
    readnextfile(wad, (char *)track->viewList);
    for (i = 0; i < length / 2; i++)
        IntelShort(&track->viewList[i]);
    section = track->sections;
    total = 0;
    for (i = 0; i < track->sectionCount; i++) {
        for (j = 0; j < 3; j++) {
            section->northViewSection[j] = track->viewList + total;
            total += section->northViewCount[j];
            section->southViewSection[j] = track->viewList + total;
            total += section->southViewCount[j];
            section->eastViewSection[j] = track->viewList + total;
            total += section->eastViewCount[j];
            section->westViewSection[j] = track->viewList + total;
            total += section->westViewCount[j];
            section->allViewSection[j] = track->viewList + total;
            total += section->allViewCount[j];
        }
        section++;
    }
    if (total * 2 != length)
        Error("track.c::InitViewList():problem with view list", Fatal);
}

TrackSection *NextIndexSection(TrackSection *section)
{
    section++;
    if (section > track->sections + track->sectionCount - 1)
        section = track->sections;
    return section;
}

extern ConfigData *gameData;
extern u_short viewDetailLimit[];
void AsmNextFrame(void);
TrackSection *DrawTrackSection(TrackSection *section, Skeleton *camPos, short count);

/* Draws the track sections in the view list for the camera's facing direction. */
void DrawNewTrack(TrackCamera *camera)
{
    short i;
    short loCount = 0;
    short medCount = 0;
    short hiCount = 0;
    short dir;
    int beforeVcount = 0;
    u_short count;

    AsmNextFrame();
    dir = GetCameraFacing(camera);
    if (dir == 1) {
        for (i = 0; i < camera->section->northViewCount[2]; i += 2) {
            count = camera->section->northViewSection[2][i + 1];
            if (viewDetailLimit[gameData->viewDetail] < count)
                count = viewDetailLimit[gameData->viewDetail];
            DrawTrackSection(track->sections + camera->section->northViewSection[2][i], camera->camPos, count);
            loCount += camera->section->northViewSection[2][i + 1];
        }
        for (i = 0; i < camera->section->northViewCount[1]; i += 2) {
            count = camera->section->northViewSection[1][i + 1];
            DrawTrackSection(track->sections + camera->section->northViewSection[1][i], camera->camPos, count);
            medCount += camera->section->northViewSection[1][i + 1];
        }
        for (i = 0; i < camera->section->northViewCount[0]; i += 2) {
            count = camera->section->northViewSection[0][i + 1];
            DrawTrackSection(track->sections + camera->section->northViewSection[0][i], camera->camPos, count);
            hiCount += camera->section->northViewSection[0][i + 1];
        }
    } else if (dir == 2) {
        for (i = 0; i < camera->section->southViewCount[2]; i += 2) {
            count = camera->section->southViewSection[2][i + 1];
            DrawTrackSection(track->sections + camera->section->southViewSection[2][i], camera->camPos, count);
            loCount += camera->section->southViewSection[2][i + 1];
        }
        for (i = 0; i < camera->section->southViewCount[1]; i += 2) {
            count = camera->section->southViewSection[1][i + 1];
            DrawTrackSection(track->sections + camera->section->southViewSection[1][i], camera->camPos, count);
            medCount += camera->section->southViewSection[1][i + 1];
        }
        for (i = 0; i < camera->section->southViewCount[0]; i += 2) {
            count = camera->section->southViewSection[0][i + 1];
            DrawTrackSection(track->sections + camera->section->southViewSection[0][i], camera->camPos, count);
            hiCount += camera->section->southViewSection[0][i + 1];
        }
    } else if (dir == 4) {
        for (i = 0; i < camera->section->westViewCount[2]; i += 2) {
            count = camera->section->westViewSection[2][i + 1];
            DrawTrackSection(track->sections + camera->section->westViewSection[2][i], camera->camPos, count);
            loCount += camera->section->westViewSection[2][i + 1];
        }
        for (i = 0; i < camera->section->westViewCount[1]; i += 2) {
            count = camera->section->westViewSection[1][i + 1];
            DrawTrackSection(track->sections + camera->section->westViewSection[1][i], camera->camPos, count);
            medCount += camera->section->westViewSection[1][i + 1];
        }
        for (i = 0; i < camera->section->westViewCount[0]; i += 2) {
            count = camera->section->westViewSection[0][i + 1];
            DrawTrackSection(track->sections + camera->section->westViewSection[0][i], camera->camPos, count);
            hiCount += camera->section->westViewSection[0][i + 1];
        }
    } else if (dir == 8) {
        for (i = 0; i < camera->section->eastViewCount[2]; i += 2) {
            count = camera->section->eastViewSection[2][i + 1];
            DrawTrackSection(track->sections + camera->section->eastViewSection[2][i], camera->camPos, count);
            loCount += camera->section->eastViewSection[2][i + 1];
        }
        for (i = 0; i < camera->section->eastViewCount[1]; i += 2) {
            count = camera->section->eastViewSection[1][i + 1];
            DrawTrackSection(track->sections + camera->section->eastViewSection[1][i], camera->camPos, count);
            medCount += camera->section->eastViewSection[1][i + 1];
        }
        for (i = 0; i < camera->section->eastViewCount[0]; i += 2) {
            count = camera->section->eastViewSection[0][i + 1];
            DrawTrackSection(track->sections + camera->section->eastViewSection[0][i], camera->camPos, count);
            hiCount += camera->section->eastViewSection[0][i + 1];
        }
    }
}

/* Draws the complete track from the supplied camera. */
void DrawAllTrack(TrackCamera *camera)
{
    short i;
    short loCount = 0;
    short medCount = 0;
    short hiCount = 0;
    short dir;

    AsmNextFrame();
    dir = GetCameraFacing(camera);
    for (i = 0; i < camera->section->allViewCount[2]; i += 2) {
        DrawTrackSection(track->sections + camera->section->allViewSection[2][i], camera->camPos, camera->section->allViewSection[2][i + 1]);
        loCount += camera->section->allViewSection[2][i + 1];
    }
    for (i = 0; i < camera->section->allViewCount[1]; i += 2) {
        DrawTrackSection(track->sections + camera->section->allViewSection[1][i], camera->camPos, camera->section->allViewSection[1][i + 1]);
        medCount += camera->section->allViewSection[1][i + 1];
    }
    for (i = 0; i < camera->section->allViewCount[0]; i += 2) {
        DrawTrackSection(track->sections + camera->section->allViewSection[0][i], camera->camPos, camera->section->allViewSection[0][i + 1]);
        hiCount += camera->section->allViewSection[0][i + 1];
    }
}

/* Draws the track from each of the camera's four view directions. */
void DrawAllDirections(TrackCamera *camera)
{
    short i;
    short loCount = 0;
    short medCount = 0;
    short hiCount = 0;
    short dir;
    int beforeVcount = 0;
    u_short count;

    AsmNextFrame();
    for (i = 0; i < camera->section->northViewCount[2]; i += 2) {
        count = camera->section->northViewSection[2][i + 1];
        if (viewDetailLimit[gameData->viewDetail] < count)
            count = viewDetailLimit[gameData->viewDetail];
        DrawTrackSection(track->sections + camera->section->northViewSection[2][i], camera->camPos, count);
        loCount += camera->section->northViewSection[2][i + 1];
    }
    for (i = 0; i < camera->section->northViewCount[1]; i += 2) {
        count = camera->section->northViewSection[1][i + 1];
        DrawTrackSection(track->sections + camera->section->northViewSection[1][i], camera->camPos, count);
        medCount += camera->section->northViewSection[1][i + 1];
    }
    for (i = 0; i < camera->section->northViewCount[0]; i += 2) {
        count = camera->section->northViewSection[0][i + 1];
        DrawTrackSection(track->sections + camera->section->northViewSection[0][i], camera->camPos, count);
        hiCount += camera->section->northViewSection[0][i + 1];
    }
    for (i = 0; i < camera->section->southViewCount[2]; i += 2) {
        count = camera->section->southViewSection[2][i + 1];
        DrawTrackSection(track->sections + camera->section->southViewSection[2][i], camera->camPos, count);
        loCount += camera->section->southViewSection[2][i + 1];
    }
    for (i = 0; i < camera->section->southViewCount[1]; i += 2) {
        count = camera->section->southViewSection[1][i + 1];
        DrawTrackSection(track->sections + camera->section->southViewSection[1][i], camera->camPos, count);
        medCount += camera->section->southViewSection[1][i + 1];
    }
    for (i = 0; i < camera->section->southViewCount[0]; i += 2) {
        count = camera->section->southViewSection[0][i + 1];
        DrawTrackSection(track->sections + camera->section->southViewSection[0][i], camera->camPos, count);
        hiCount += camera->section->southViewSection[0][i + 1];
    }
    for (i = 0; i < camera->section->westViewCount[2]; i += 2) {
        count = camera->section->westViewSection[2][i + 1];
        DrawTrackSection(track->sections + camera->section->westViewSection[2][i], camera->camPos, count);
        loCount += camera->section->westViewSection[2][i + 1];
    }
    for (i = 0; i < camera->section->westViewCount[1]; i += 2) {
        count = camera->section->westViewSection[1][i + 1];
        DrawTrackSection(track->sections + camera->section->westViewSection[1][i], camera->camPos, count);
        medCount += camera->section->westViewSection[1][i + 1];
    }
    for (i = 0; i < camera->section->westViewCount[0]; i += 2) {
        count = camera->section->westViewSection[0][i + 1];
        DrawTrackSection(track->sections + camera->section->westViewSection[0][i], camera->camPos, count);
        hiCount += camera->section->westViewSection[0][i + 1];
    }
    for (i = 0; i < camera->section->eastViewCount[2]; i += 2) {
        count = camera->section->eastViewSection[2][i + 1];
        DrawTrackSection(track->sections + camera->section->eastViewSection[2][i], camera->camPos, count);
        loCount += camera->section->eastViewSection[2][i + 1];
    }
    for (i = 0; i < camera->section->eastViewCount[1]; i += 2) {
        count = camera->section->eastViewSection[1][i + 1];
        DrawTrackSection(track->sections + camera->section->eastViewSection[1][i], camera->camPos, count);
        medCount += camera->section->eastViewSection[1][i + 1];
    }
    for (i = 0; i < camera->section->eastViewCount[0]; i += 2) {
        count = camera->section->eastViewSection[0][i + 1];
        DrawTrackSection(track->sections + camera->section->eastViewSection[0][i], camera->camPos, count);
        hiCount += camera->section->eastViewSection[0][i + 1];
    }
}
