#ifndef CD_H
#define CD_H

typedef struct {
    unsigned char minute;
    unsigned char second;
    unsigned char sector;
    unsigned char track;
} CdlLOC;

#endif
