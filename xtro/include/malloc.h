#ifndef MALLOC_H
#define MALLOC_H

#include "common.h"

/* Magic in the header of an allocated block, and of a free one. */
#define BLOCK_ALLOCATED 0x414C4F43 /* "ALOC" */
#define BLOCK_FREE 0x46524545      /* "FREE" */

/* Block header in front of the data of every block of a dynamic heap. A free block
   also links itself into its size bucket through the first two data words. */
#define DYN_HEADER 0x10

/* A free block is split only if the piece left over is at least this many bytes. */
#define DYN_MIN_SPLIT 0x40
#define DYN_MIN_ALLOC 16
#define DYN_BUCKET_COUNT 32
#define DYN_ADDRESS_LIMIT 0x80800000

typedef struct DynBlock {
    long magic;
    long size;                 /* bytes of data after the header */
    struct DynBlock *prev;     /* neighbour before this block in memory */
    struct DynBlock *next;     /* neighbour after this block in memory */
    struct DynBlock *freePrev; /* bucket link, first data word (free blocks only) */
    struct DynBlock *freeNext; /* bucket link, second data word (free blocks only) */
} DynBlock;

/* The heap object. Free blocks hang off 32 buckets, indexed by size class. The heap
   starts out as one free block, stored at the end of the object. */
typedef struct DynHeap {
    long size;
    long free; /* bytes still available */
    DynBlock *bucket[DYN_BUCKET_COUNT];
    DynBlock first;
} DynHeap;

/* A chunked allocator: a run of chunks taken from a heap, filled by bumping cursor.
   Each chunk's first word links to the chunk before it. */
typedef struct DynArena {
    DynHeap *heap;
    char *chunks;  /* previous chunks, newest first */
    char *chunk;   /* current chunk */
    char *cursor;  /* next free byte in the current chunk */
    u_long remain; /* bytes left in the current chunk */
    long chunkSize;
} DynArena;

int printf(const char *format, ...);

void DFree(DynHeap *head, void *ptr);
void InitDynamicMem(DynHeap *heap, long size);
void DJoin(DynBlock *one, DynBlock *two);
DynBlock *DSplit(DynBlock *block, long size);
void *DAlloc(DynHeap *head, long size);
short BinIndex(long size);
void DLink(DynHeap *head, DynBlock *block);     /* put a free block into its bucket */
void DUnlink(DynHeap *head, DynBlock *block);   /* take a free block out of its bucket */
void DCoalesce(DynHeap *heap, DynBlock *block); /* merge a freed block with free neighbours */

#endif /* MALLOC_H */
