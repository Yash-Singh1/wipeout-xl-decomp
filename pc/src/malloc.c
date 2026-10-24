#include "functions.h"
#include "pc.h"

#define HEAP_BLOCK_FREE 0
#define HEAP_BLOCK_ALLOCATED 1
#define HEAP_ALIGNMENT_BYTES 4
#define HEAP_ALIGNMENT_MASK 3
#define HEAP_SEARCH_LIMIT 0x800000

extern char msgInsufficientMemory[];
extern char msgDFreeNotFound[];
extern HeapBlock *mainheapcopy;

long lowTideMark;

void *memset(void *dest, int c, unsigned int count);
#pragma intrinsic(memset)

/* Clears a whole heap and makes its one free block span everything after the heap header. */
void InitDynamicMem(HeapBlock *head, long size)
{
#ifdef NETPLAY
    memset(head, 0, size);
    head->flags = HEAP_BLOCK_FREE;
    head->size = size - sizeof(HeapBlock);
    head->data = (char *)(head + 1);
    head->next = 0;
    head->free = size - sizeof(HeapBlock);
#else
    long avail;

    memset(head, 0, size);
    avail = size - sizeof(HeapBlock);
    head->flags = HEAP_BLOCK_FREE;
    head->size = avail;
    head->data = (char *)(head + 1);
    head->next = 0;
    head->free = avail;
#endif
}

/* Allocates an aligned block using a best-fit search and splits its free block. */
void *DAlloc(HeapBlock *head, long size)
{
#ifdef NETPLAY
    HeapBlock *block;
    HeapBlock *bestBlock;
    HeapBlock *newBlock;
    long       smallest;

    bestBlock = 0;
    smallest = HEAP_SEARCH_LIMIT;
    size += HEAP_ALIGNMENT_BYTES;
    size = (size | HEAP_ALIGNMENT_MASK) ^ HEAP_ALIGNMENT_MASK;
    block = head;
    while (block) {
        if (block->size >= size + (long)sizeof(HeapBlock) && block->size < smallest && block->flags == HEAP_BLOCK_FREE) {
            smallest = block->size;
            bestBlock = block;
        }
        if (block->size == size && block->flags == HEAP_BLOCK_FREE) {
            block->flags = HEAP_BLOCK_ALLOCATED;
            head->free -= size;
            return block->data;
        }
        block = block->next;
    }
    if (!bestBlock) {
        OutputDebugString(msgInsufficientMemory);
        return 0;
    }
    block = bestBlock;
    block->size -= size + sizeof(HeapBlock);
    newBlock = (HeapBlock *)(block->data + block->size);
    newBlock->flags = HEAP_BLOCK_ALLOCATED;
    newBlock->size = size;
    newBlock->data = (char *)(newBlock + 1);
    newBlock->next = block->next;
    block->next = newBlock;
    head->free -= size + sizeof(HeapBlock);
    if (head == mainheapcopy && head->free < lowTideMark) {
        lowTideMark = head->free;
    }
    return newBlock->data;
#else
    HeapBlock *block;
    HeapBlock *best;
    HeapBlock *allocated;
    long       smallest;
    long       remaining;
    long       leftover;

    best = 0;
    smallest = HEAP_SEARCH_LIMIT;
    size += HEAP_ALIGNMENT_BYTES;
    size = (size | HEAP_ALIGNMENT_MASK) ^ HEAP_ALIGNMENT_MASK;
    block = head;
    while (block) {
        if (block->size >= size + (long)sizeof(HeapBlock) && block->size < smallest && block->flags == HEAP_BLOCK_FREE) {
            smallest = block->size;
            best = block;
        }
        if (block->size == size && block->flags == HEAP_BLOCK_FREE) {
            block->flags = HEAP_BLOCK_ALLOCATED;
            head->free -= size;
            return block->data;
        }
        block = block->next;
    }
    if (!best) {
        OutputDebugString(msgInsufficientMemory);
        return 0;
    }
    block = best;
    leftover = block->size - sizeof(HeapBlock);
    leftover = leftover - size;
    block->size = leftover;
    allocated = (HeapBlock *)(block->data + leftover);
    allocated->flags = HEAP_BLOCK_ALLOCATED;
    allocated->size = size;
    allocated->data = (char *)(allocated + 1);
    allocated->next = block->next;
    block->next = allocated;
    remaining = head->free - sizeof(HeapBlock) - size;
    head->free = remaining;
    if (head == mainheapcopy && head->free < lowTideMark) {
        lowTideMark = head->free;
    }
    return allocated->data;
#endif
}

/* Frees a block, merging adjacent free blocks and reporting unknown pointers. */
void DFree(HeapBlock *head, char *ptr)
{
#ifdef NETPLAY
    HeapBlock *previousBlock;
    HeapBlock *nextBlock;
    HeapBlock *current;

    current = head->next;
    previousBlock = head;
    while (current != 0) {
        if (current->data == ptr) {
            nextBlock = current->next;
            head->free += current->size;
            if (nextBlock != 0 && nextBlock->flags == HEAP_BLOCK_FREE) {
                current->size += nextBlock->size + sizeof(HeapBlock);
                current->next = nextBlock->next;
                head->free += sizeof(HeapBlock);
            }
            if (previousBlock->flags == HEAP_BLOCK_ALLOCATED) {
                current->flags = HEAP_BLOCK_FREE;
            } else {
                previousBlock->size += current->size + sizeof(HeapBlock);
                previousBlock->next = current->next;
                head->free += sizeof(HeapBlock);
            }
            return;
        }
        previousBlock = current;
        current = current->next;
    }
    OutputDebugString(msgDFreeNotFound);
#else
    HeapBlock *block;
    HeapBlock *prevBlock;
    HeapBlock *nextBlock;

    block = head->next;
    prevBlock = head;
    while (block != 0) {
        if (block->data == ptr) {
            nextBlock = block->next;
            head->free += block->size;
            if (nextBlock != 0 && nextBlock->flags == HEAP_BLOCK_FREE) {
                block->size += nextBlock->size + sizeof(HeapBlock);
                block->next = nextBlock->next;
                head->free += sizeof(HeapBlock);
            }
            if (prevBlock->flags == HEAP_BLOCK_ALLOCATED) {
                block->flags = HEAP_BLOCK_FREE;
                return;
            }
            prevBlock->size += block->size + sizeof(HeapBlock);
            prevBlock->next = block->next;
            head->free += sizeof(HeapBlock);
            return;
        }
        prevBlock = block;
        block = block->next;
    }
    OutputDebugString(msgDFreeNotFound);
#endif
}
