#include "functions.h"


extern TrackData *track;
extern Object **trackObjects;

#define ObjectListMax 128
#define Jump 1

#ifndef NETPLAY
/* Works out, for every track section, which objects of the scene can be seen from it. */
void SceneRadiusCheck(Object *objects)
{
    TrackSection *section;
    TrackSection *visSection;
    short i, j;
    Object *object;
    MATRIX *matrix;
    Vector *sectionCentre;
    short objectCount;
    long vx, vz, vy;
    long distance;
    short objectList[ObjectListMax];
    short withinLocal;
    short withinGlobal;
    short allCount;

    object = objects;
    objectCount = 0;
    while (object) {
        object = object->next;
        objectCount++;
    }
    trackObjects = DAlloc(heap, (objectCount + 1) * sizeof(Object *));
    if (!trackObjects) {
        Error("radscn.c::SceneRadiusCheck(): No memory for global object table", Fatal);
    }
    object = objects;
    for (i = 0; i < objectCount; i++) {
        trackObjects[i] = object;
        object = object->next;
    }
    section = track->sections;
    for (i = 0; i < track->sectionCount; i++) {
        object = objects;
        objectCount = 0;
        allCount = 0;
        while (object) {
            withinLocal = 0;
            withinGlobal = 0;
            matrix = &object->skeleton->relative;
            sectionCentre = &section->centre;
            vx = matrix->t[0] - sectionCentre->vx;
            vy = matrix->t[1] - sectionCentre->vy;
            vz = matrix->t[2] - sectionCentre->vz;
            if (section->flags & Jump) {
                if ((abs(vx) < (section->globalRadius * 2)) && (abs(vy) < (section->globalRadius * 2)) && (abs(vz) < (section->globalRadius * 2))) {
                    distance = SquareRoot0((vx * vx) + (vy * vy) + (vz * vz));
                    distance = distance - object->extent;
                    distance -= 1024;
                    if (distance < section->globalRadius) {
                        withinGlobal = 1;
                    }
                }
            } else {
                if ((abs(vx) < section->globalRadius) && (abs(vy) < section->globalRadius) && (abs(vz) < section->globalRadius)) {
                    distance = SquareRoot0((vx * vx) + (vy * vy) + (vz * vz));
                    distance = distance - object->extent;
                    distance -= 1024;
                    if (distance < section->globalRadius) {
                        withinGlobal = 1;
                    }
                }
            }
            if (withinGlobal) {
                visSection = section;
                for (j = 0; (!withinLocal) && (j < 30); j++) {
                    matrix = &object->skeleton->relative;
                    sectionCentre = &visSection->centre;
                    vx = matrix->t[0] - sectionCentre->vx;
                    vy = matrix->t[1] - sectionCentre->vy;
                    vz = matrix->t[2] - sectionCentre->vz;
                    if ((abs(vx) < section->localRadius) && (abs(vy) < section->localRadius) && (abs(vz) < section->localRadius)) {
                        distance = SquareRoot0((vx * vx) + (vy * vy) + (vz * vz));
                        distance = distance - object->extent;
                        distance -= 1024;
                        if (distance < section->localRadius) {
                            withinLocal = 1;
                        }
                    }
                    visSection = visSection->nextSection.ptr;
                }
                visSection = section;
                for (j = 0; (!withinLocal) && (j < 30); j++) {
                    matrix = &object->skeleton->relative;
                    sectionCentre = &visSection->centre;
                    vx = matrix->t[0] - sectionCentre->vx;
                    vy = matrix->t[1] - sectionCentre->vy;
                    vz = matrix->t[2] - sectionCentre->vz;
                    if ((abs(vx) < section->localRadius) && (abs(vy) < section->localRadius) && (abs(vz) < section->localRadius)) {
                        distance = SquareRoot0((vx * vx) + (vy * vy) + (vz * vz));
                        distance = distance - object->extent;
                        distance -= 1024;
                        if (distance < section->localRadius) {
                            withinLocal = 1;
                        }
                    }
                    visSection = visSection->prevSection.ptr;
                }
            }
            if (withinLocal) {
                if (objectCount < ObjectListMax) {
                    objectList[objectCount++] = allCount;
                } else {
                    Error("Radius.c:SceneRadiusCheck: Maximum number of objects exceeded", Fatal);
                }
            }
            object = object->next;
            allCount++;
        }
        section->objects = DAlloc(heap, objectCount * sizeof(short));
        if (!section->objects) {
            Error("Error:: radius.c: Not enough memory for section object list", Fatal);
        }
        section->objectCount = objectCount;
        for (j = 0; j < objectCount; j++) {
            section->objects[j] = objectList[j];
        }
        section = NextIndexSection(section);
    }
}
#endif
#ifdef EQUIV
#ifdef NETPLAY
/* NET-WOXL body. Its local names are chosen so the /Od frame places each local where the exe does
   (tools/odslots.py); the declaration order below is the one that gives that layout. `spare` is a
   local the original declared and never used. */
void SceneRadiusCheck(Object *objects)
{
    Object *sceneObject;
    short objCount;
    short closeLocal;
    short objectIndices[ObjectListMax];
    TrackSection *currentSection;
    long distance;
    short allIndex;
    long yOffset;
    long zOffset;
    long spare;
    short walk;
    short outerHit;
    TrackSection *viewSection;
    short trackIndex;
    MATRIX *relative;
    long vx;
    Vector *sectionCentre;

    sceneObject = objects;
    objCount = 0;
    while (sceneObject) {
        sceneObject = sceneObject->next;
        objCount++;
    }
    trackObjects = DAlloc(heap, (objCount + 1) * sizeof(Object *));
    if (!trackObjects) {
        Error("radscn.c::SceneRadiusCheck(): No memory for global object table", Fatal);
    }
    sceneObject = objects;
    for (trackIndex = 0; trackIndex < objCount; trackIndex++) {
        trackObjects[trackIndex] = sceneObject;
        sceneObject = sceneObject->next;
    }
    currentSection = track->sections;
    for (trackIndex = 0; trackIndex < track->sectionCount; trackIndex++) {
        sceneObject = objects;
        objCount = 0;
        allIndex = 0;
        while (sceneObject) {
            closeLocal = 0;
            outerHit = 0;
            relative = &sceneObject->skeleton->relative;
            sectionCentre = &currentSection->centre;
            vx = relative->t[0] - sectionCentre->vx;
            yOffset = relative->t[1] - sectionCentre->vy;
            zOffset = relative->t[2] - sectionCentre->vz;
            if (currentSection->flags & Jump) {
                if ((abs(vx) < (currentSection->globalRadius * 2)) && (abs(yOffset) < (currentSection->globalRadius * 2)) && (abs(zOffset) < (currentSection->globalRadius * 2))) {
                    distance = SquareRoot0((yOffset * yOffset) + (zOffset * zOffset) + (vx * vx));
                    distance = distance - sceneObject->extent;
                    distance -= 1024;
                    if (distance < currentSection->globalRadius) {
                        outerHit = 1;
                    }
                }
            } else {
                if ((abs(vx) < currentSection->globalRadius) && (abs(yOffset) < currentSection->globalRadius) && (abs(zOffset) < currentSection->globalRadius)) {
                    distance = SquareRoot0((yOffset * yOffset) + (zOffset * zOffset) + (vx * vx));
                    distance = distance - sceneObject->extent;
                    distance -= 1024;
                    if (distance < currentSection->globalRadius) {
                        outerHit = 1;
                    }
                }
            }
            if (outerHit) {
                viewSection = currentSection;
                for (walk = 0; (!closeLocal) && (walk < 30); walk++) {
                    relative = &sceneObject->skeleton->relative;
                    sectionCentre = &viewSection->centre;
                    vx = relative->t[0] - sectionCentre->vx;
                    yOffset = relative->t[1] - sectionCentre->vy;
                    zOffset = relative->t[2] - sectionCentre->vz;
                    if ((abs(vx) < currentSection->localRadius) && (abs(yOffset) < currentSection->localRadius) && (abs(zOffset) < currentSection->localRadius)) {
                        distance = SquareRoot0((yOffset * yOffset) + (zOffset * zOffset) + (vx * vx));
                        distance = distance - sceneObject->extent;
                        distance -= 1024;
                        if (distance < currentSection->localRadius) {
                            closeLocal = 1;
                        }
                    }
                    viewSection = viewSection->nextSection.ptr;
                }
                viewSection = currentSection;
                for (walk = 0; (!closeLocal) && (walk < 30); walk++) {
                    relative = &sceneObject->skeleton->relative;
                    sectionCentre = &viewSection->centre;
                    vx = relative->t[0] - sectionCentre->vx;
                    yOffset = relative->t[1] - sectionCentre->vy;
                    zOffset = relative->t[2] - sectionCentre->vz;
                    if ((abs(vx) < currentSection->localRadius) && (abs(yOffset) < currentSection->localRadius) && (abs(zOffset) < currentSection->localRadius)) {
                        distance = SquareRoot0((yOffset * yOffset) + (zOffset * zOffset) + (vx * vx));
                        distance = distance - sceneObject->extent;
                        distance -= 1024;
                        if (distance < currentSection->localRadius) {
                            closeLocal = 1;
                        }
                    }
                    viewSection = viewSection->prevSection.ptr;
                }
            }
            if (closeLocal) {
                if (objCount < ObjectListMax) {
                    objectIndices[objCount++] = allIndex;
                } else {
                    Error("Radius.c:SceneRadiusCheck: Maximum number of objects exceeded", Fatal);
                }
            }
            sceneObject = sceneObject->next;
            allIndex++;
        }
        currentSection->objects = DAlloc(heap, objCount * sizeof(short));
        if (!currentSection->objects) {
            Error("Error:: radius.c: Not enough memory for section object list", Fatal);
        }
        currentSection->objectCount = objCount;
        for (walk = 0; walk < objCount; walk++) {
            currentSection->objects[walk] = objectIndices[walk];
        }
        currentSection = NextIndexSection(currentSection);
    }
}
#endif
#endif
