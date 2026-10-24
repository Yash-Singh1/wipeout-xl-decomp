#include "malloc.h"

void PrintWarning(char *msg, long unused);

short IsBadBlock(DynBlock *block)
{
    long magic;

    if ((long)block >= 0) {
        return 1;
    }
    if ((u_long)block > DYN_ADDRESS_LIMIT) {
        return 1;
    }
    magic = block->magic;
    if (magic != BLOCK_FREE) {
        if (magic != BLOCK_ALLOCATED) {
            return 1;
        }
    }
    return 0;
}

short IsBadHeapBlock(DynBlock *block)
{
    long magic;

    if ((long)block >= 0) {
        return 1;
    }
    if ((u_long)block > DYN_ADDRESS_LIMIT) {
        return 1;
    }
    magic = block->magic;
    if (magic != BLOCK_FREE) {
        if (magic != BLOCK_ALLOCATED) {
            return 1;
        }
    }
    return 0;
}

short BinIndex(long size)
{
    short bin = 0;

    while (size >= 2) {
        size >>= 1;
        bin++;
    }
    return bin;
}

void InitDynamicMem(DynHeap *heap, long size)
{
    long i;
    DynBlock *first;

    for (i = DYN_BUCKET_COUNT - 1; i >= 0; i--) {
        heap->bucket[i] = 0;
    }
    size -= (char *)&heap->first - (char *)heap + DYN_HEADER;
    first = &heap->first;
    heap->bucket[BinIndex(size)] = first;
    first->magic = BLOCK_FREE;
    first->size = size;
    first->prev = 0;
    first->next = 0;
    first->freePrev = 0;
    first->freeNext = 0;
    heap->size = size;
    heap->free = size;
}

void DJoin(DynBlock *one, DynBlock *two)
{
    DynBlock *next;

    if (IsBadBlock(one)) {
        PrintWarning("Malloc.c:DJoin: Block one is not valid", 0);
        return;
    }
    if (IsBadBlock(two)) {
        PrintWarning("Malloc.c:DJoin: Block two is not valid", 0);
        return;
    }
    if (one->next != two) {
        PrintWarning("Malloc.c:DJoin: Blocks are not consecutive", 0);
        return;
    }
    next = two->next;
    one->next = next;
    if (next != 0) {
        if (IsBadBlock(next)) {
            PrintWarning("Malloc.c:DJoin: Next block is not valid", 0);
            return;
        }
        one->next->prev = one;
    }
    one->size += DYN_HEADER + two->size;
}

DynBlock *DSplit(DynBlock *block, long size)
{
    DynBlock *rest;
    long remaining;

    if (IsBadBlock(block)) {
        PrintWarning("Malloc.c:DSplit: Free block is not valid", 0);
        return 0;
    }
    remaining = block->size - size - DYN_HEADER;
    if (remaining >= DYN_MIN_SPLIT) {
        rest = (DynBlock *)((char *)block + DYN_HEADER + size);
        rest->prev = block;
        rest->next = block->next;
        block->next = rest;
        if (rest->next != 0) {
            if (IsBadBlock(rest->next)) {
                PrintWarning("Malloc.c:DSplit: Next block is not valid", 0);
                return 0;
            }
            rest->next->prev = rest;
        }
        block->size = size;
        rest->size = remaining;
        return rest;
    }
    return 0;
}

void DLink(DynHeap *head, DynBlock *block)
{
    short bin;

    if (IsBadBlock(block)) {
        PrintWarning("Malloc.c:DLink: Free block is not valid", 0);
        return;
    }
    bin = BinIndex(block->size);
    block->freePrev = 0;
    block->freeNext = head->bucket[bin];
    head->bucket[bin] = block;
    if (block->freeNext != 0) {
        if (IsBadBlock(block->freeNext)) {
            PrintWarning("Malloc.c:DLink: Next free block is not valid", 0);
        } else {
            block->freeNext->freePrev = block;
        }
    }
}

void DUnlink(DynHeap *head, DynBlock *block)
{
    if (IsBadBlock(block)) {
        PrintWarning("Malloc.c:DUnlink: Free block is not valid", 0);
        return;
    }
    if (block->freePrev) {
        if (IsBadBlock(block->freePrev)) {
            PrintWarning("Malloc.c:DUnlink: Previous block is not valid", 0);
            return;
        }
        block->freePrev->freeNext = block->freeNext;
    } else {
        head->bucket[BinIndex(block->size)] = block->freeNext;
    }
    if (block->freeNext) {
        if (IsBadBlock(block->freeNext)) {
            PrintWarning("Malloc.c:DUnlink: Next block is not valid", 0);
            return;
        }
        block->freeNext->freePrev = block->freePrev;
    }
}

void DCoalesce(DynHeap *head, DynBlock *block)
{
    DynBlock *prev;
    DynBlock *next;

    if (IsBadBlock(block)) {
        PrintWarning("Malloc.c:DCoalesce: Free block is not valid", 0);
        return;
    }
    prev = block->prev;
    next = block->next;
    if (prev != 0 && IsBadBlock(prev)) {
        PrintWarning("Malloc.c:DCoalesce: Previous free block is not valid", 0);
        return;
    }
    if (next != 0 && IsBadBlock(next)) {
        PrintWarning("Malloc.c:DCoalesce: Next free block is not valid", 0);
        return;
    }
    if (prev != 0 && prev->magic == BLOCK_FREE && next != 0 && next->magic == BLOCK_FREE) {
        DUnlink(head, prev);
        DUnlink(head, block);
        DUnlink(head, next);
        DJoin(prev, block);
        DJoin(prev, next);
        DLink(head, prev);
        head->free += 2 * DYN_HEADER;
    } else if (prev != 0 && prev->magic == BLOCK_FREE) {
        DUnlink(head, prev);
        DUnlink(head, block);
        DJoin(prev, block);
        DLink(head, prev);
        head->free += DYN_HEADER;
    } else if (next != 0 && next->magic == BLOCK_FREE) {
        DUnlink(head, block);
        DUnlink(head, next);
        DJoin(block, next);
        DLink(head, block);
        head->free += DYN_HEADER;
    }
}

void *DAlloc(DynHeap *head, long size)
{
    DynBlock *block;
    DynBlock *rest;
    short bin;
    long allocatedSize;
    long freeBytes;

    size = (size + 3) & ~3;
    if (size < DYN_MIN_ALLOC) {
        size = DYN_MIN_ALLOC;
    }
    bin = BinIndex(size);
    block = 0;
    while (block == 0) {
        if (bin >= DYN_BUCKET_COUNT - 1) {
            break;
        }
        block = head->bucket[bin];
        if (block != 0) {
            if (IsBadBlock(block)) {
                PrintWarning("Malloc.c:DAlloc: Heap header corrupted", 0);
                return 0;
            }
            while (1) {
                if (block->size >= size) {
                    break;
                }
                block = block->freeNext;
                if (block == 0) {
                    break;
                }
                if (IsBadBlock(block)) {
                    PrintWarning("Malloc.c:DAlloc: Block header corrupted", 0);
                    return 0;
                }
            }
        }
        if (block != 0) {
            break;
        }
        bin++;
    }
    if (block != 0) {
        rest = DSplit(block, size);
        if (rest != 0) {
            block->magic = BLOCK_ALLOCATED;
            rest->magic = BLOCK_FREE;
            if (block->freePrev != 0) {
                block->freePrev->freeNext = block->freeNext;
            } else {
                head->bucket[bin] = block->freeNext;
            }
            if (block->freeNext != 0) {
                block->freeNext->freePrev = block->freePrev;
            }
            bin = BinIndex(rest->size);
            rest->freePrev = 0;
            rest->freeNext = head->bucket[bin];
            head->bucket[bin] = rest;
            if (rest->freeNext != 0) {
                rest->freeNext->freePrev = rest;
            }
            allocatedSize = block->size;
            head->free -= DYN_HEADER;
            head->free -= allocatedSize;
            return (char *)block + DYN_HEADER;
        } else {
            block->magic = BLOCK_ALLOCATED;
            if (block->freePrev != 0) {
                block->freePrev->freeNext = block->freeNext;
            } else {
                head->bucket[bin] = block->freeNext;
            }
            if (block->freeNext != 0) {
                block->freeNext->freePrev = block->freePrev;
            }
            freeBytes = head->free;
            head->free = freeBytes - block->size;
            return (char *)block + DYN_HEADER;
        }
    }
    return 0;
}

void DFree(DynHeap *head, void *ptr)
{
    DynBlock *block;
    short bucket;

    if (ptr == 0) {
        return;
    }
    block = (DynBlock *)((char *)ptr - DYN_HEADER);
    if (block->magic != BLOCK_ALLOCATED) {
        printf("Malloc.c:DFree(): Block header not valid!\n");
        return;
    }
    bucket = BinIndex(block->size);
    block->freePrev = 0;
    block->freeNext = head->bucket[bucket];
    if (block->freeNext != 0 && block->freeNext->magic != BLOCK_FREE) {
        printf("Malloc.c:DFree(): Free block list not valid!");
    }
    head->bucket[bucket] = block;
    if (block->freeNext != 0) {
        block->freeNext->freePrev = block;
    }
    head->free += block->size;
    block->magic = BLOCK_FREE;
    DCoalesce(head, block);
}

DynArena *ArenaCreate(DynHeap *heap, long size)
{
    DynArena *arena;

    arena = DAlloc(heap, sizeof(DynArena));
    if (arena == 0) {
        return 0;
    }
    arena->chunk = DAlloc(heap, size);
    if (arena->chunk == 0) {
        DFree(heap, arena);
        return 0;
    }
    arena->heap = heap;
    arena->chunks = 0;
    arena->cursor = arena->chunk;
    arena->remain = size;
    arena->chunkSize = size;
    return arena;
}

void *ArenaAlloc(DynArena *arena, long size)
{
    u_long aligned;
    char *chunk;
    char *data;

    aligned = (size + 3) & ~3;
    if (arena->remain < aligned + 4) {
        chunk = arena->chunk;
        *(char **)chunk = arena->chunks;
        arena->chunks = chunk;
        arena->chunk = DAlloc(arena->heap, arena->chunkSize);
        arena->cursor = arena->chunk;
        if (arena->chunk == 0) {
            arena->remain = 0;
            return 0;
        }
        arena->remain = arena->chunkSize;
    }
    data = arena->cursor;
    arena->cursor += aligned;
    arena->remain -= aligned;
    return data;
}

void ArenaDestroy(DynArena *arena)
{
    char *chunk;
    char *dead;

    chunk = arena->chunks;
    while (chunk != 0) {
        dead = chunk;
        chunk = *(char **)dead;
        DFree(arena->heap, dead);
    }
    DFree(arena->heap, arena->chunk);
    DFree(arena->heap, arena);
}

void ArenaReset(DynArena *arena)
{
    char *chunk;
    char *dead;

    chunk = arena->chunks;
    while (chunk != 0) {
        dead = chunk;
        chunk = *(char **)dead;
        DFree(arena->heap, dead);
    }
    arena->chunks = 0;
    arena->cursor = arena->chunk;
    arena->remain = arena->chunkSize;
}
