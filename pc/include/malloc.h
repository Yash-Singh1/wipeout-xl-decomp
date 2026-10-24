#ifndef MALLOC_H
#define MALLOC_H

/* Header in front of every block of a dynamic heap. The heap object itself has the
   same header; its free field counts the bytes still available. */
typedef struct HeapBlock {
    long              flags;
    long              size;
    long              free;
    char             *data;
    struct HeapBlock *next;
} HeapBlock;

#endif
