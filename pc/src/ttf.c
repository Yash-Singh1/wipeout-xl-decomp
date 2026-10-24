#include "functions.h"

extern char errorString[];


/* The default register allocation differs; the entropy build matches. */
#ifdef EQUIV
#ifndef NETPLAY
/* Loads and byte-swaps texture templates from a standalone file. */
TexTemplate *LoadTtfFile(char *file, short *count, short libraryTextures)
{
    long length;
    char *ttfFile;
    short *tex;
    short i;

    length = FileLength(file);
    if (length <= 0) {
        FileAccessError(file);
        wsprintf(errorString, "Ttf.c:LoadTtfFile: Ttf file %s not found\n", file);
        Error(errorString, Fatal);
    }

    ttfFile = DAlloc(heap, length + 32);
    if (!ttfFile) {
        /* the original passes no output buffer here (the format string is written to) */
        wsprintf("Ttf.c:LoadTtfFile: Failed to allocate memory for file %s\n", file);
        Error(errorString, Fatal);
    }

    if (LoadFile(file, ttfFile) != length) {
        wsprintf(errorString, "Ttf.c:LoadTtfFile: Failed to load file %s\n", file);
        DFree(heap, ttfFile);
        ttfFile = 0;
        Error(errorString, Fatal);
    }

    tex = (short *)ttfFile;
    while (tex < (short *)(ttfFile + length)) {
        for (i = 0; i < 21; i++) {
            *tex = ((*tex & 0x00ff) << 8) + ((*tex & 0xff00) >> 8);
            tex += 1;
        }
        (*count)++;
    }

    return (TexTemplate *)ttfFile;
}
#endif
#endif

#ifdef NETPLAY
/* Loads and byte-swaps texture templates from a standalone file. */
TexTemplate *LoadTtfFile(char *file, short *count, short libraryTextures)
{
    char *data;
    short *cursor;
    long length;
    short i;

    length = FileLength(file);
    if (length <= 0) {
        FileAccessError(file);
        wsprintf(errorString, "Ttf.c:LoadTtfFile: Ttf file %s not found\n", file);
        Error(errorString, Fatal);
    }

    data = DAlloc(heap, length + 32);
    if (!data) {
        /* the original passes no output buffer here (the format string is written to) */
        wsprintf("Ttf.c:LoadTtfFile: Failed to allocate memory for file %s\n", file);
        Error(errorString, Fatal);
    }

    if (LoadFile(file, data) != length) {
        wsprintf(errorString, "Ttf.c:LoadTtfFile: Failed to load file %s\n", file);
        DFree(heap, data);
        data = 0;
        Error(errorString, Fatal);
    }

    cursor = (short *)data;
    while (cursor < (short *)(data + length)) {
        for (i = 0; i < 21; i++) {
            *cursor = ((*cursor & 0x00ff) << 8) + ((*cursor & 0xff00) >> 8);
            cursor += 1;
        }
        (*count)++;
    }

    return (TexTemplate *)data;
}
#endif

/* Loads and byte-swaps texture templates from the current WAD entry. */
#ifdef NETPLAY
TexTemplate *WADLoadTtfFile(WADFILE *wad, short *count)
{
    char *data;
    short *cursor;
    long length;
    short i;

    length = wad->sizes[wad->current];
    if (!rgbTextures) {
        data = DAlloc(heap, length + length / 2 + 32);
    } else {
        data = DAlloc(heap, length + 32);
    }
    readnextfile(wad, data);

    cursor = (short *)data;
    while (cursor < (short *)(data + length)) {
        for (i = 0; i < 21; i++) {
            *cursor = ((*cursor & 0x00ff) << 8) + ((*cursor & 0xff00) >> 8);
            cursor += 1;
        }
        (*count)++;
    }

    return (TexTemplate *)data;
}
#else
TexTemplate *WADLoadTtfFile(WADFILE *wad, short *count)
{
    long length;
    char *ttfFile;
    short *tex;
    short i;

    length = wad->sizes[wad->current];
    if (!rgbTextures) {
        ttfFile = DAlloc(heap, length + length / 2 + 32);
    } else {
        ttfFile = DAlloc(heap, length + 32);
    }
    readnextfile(wad, ttfFile);

    tex = (short *)ttfFile;
    while (tex < (short *)(ttfFile + length)) {
        for (i = 0; i < 21; i++) {
            *tex = ((*tex & 0x00ff) << 8) + ((*tex & 0xff00) >> 8);
            tex += 1;
        }
        (*count)++;
    }

    return (TexTemplate *)ttfFile;
}
#endif
