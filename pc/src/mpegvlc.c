#include "mpeg.h"
#include <string.h>

/* Builds the VLC lookup tables. The coded block pattern values are stored bit-reversed in the
   source table; they are flipped to the order the decoder uses before the tables are built. */
void mpegInitializeVlcTables(VidStream *vs)
{
    unsigned int i;
    int j;
    int k;

    for (i = 0; i < 63; i++) {
        k = 0;
        for (j = 0; j < 6; j++) {
            if (codedBlockPatternCodes[i].value & (1 << j))
                k += 1 << (5 - j);
        }
        codedBlockPatternCodes[i].value = k;
    }
    mpegBuildVlcLookupTable(vs, &mbAddrIncTable, 35, mbAddrIncCodes, 11);
    mpegBuildVlcLookupTable(vs, &mbTypePTable, 7, mbTypePCodes, 6);
    mpegBuildVlcLookupTable(vs, &mbTypeBTable, 11, mbTypeBCodes, 6);
    mpegBuildVlcLookupTable(vs, &codedBlockPatternTable, 63, codedBlockPatternCodes, 9);
    mpegBuildVlcLookupTable(vs, &motionTable, 33, motionCodes, 11);
}

/* Expands a list of "0 1 ..." code strings into a direct lookup table of 1 << bits entries. */
#ifndef NETPLAY
void mpegBuildVlcLookupTable(VidStream *vs, VlcEntry **table, short count, VlcSource *codes, short bits)
{
    short b;
    short nbits;
    short code;
    short m;
    short idx;

    *table = mpegHeapAlloc(&vs->heap, (1 << bits) * sizeof(VlcEntry));
    for (idx = 0; (1 << bits) > idx; idx++)
        (*table)[idx].value = -1;
    for (idx = 0; idx < count; idx++) {
        code = 0;
        m = 1 << (bits - 1);
        nbits = 0;
        for (b = 0; strlen(codes[idx].code) > b; b++) {
            if (m == 0)
                mpegDebugMessage("Too many bits in string");
            switch (codes[idx].code[b]) {
            case '1':
                code += m;
            case '0':
                m >>= 1;
                nbits++;
                break;
            case ' ':
                break;
            default:
                mpegDebugMessage("Invalid char in string: '%c'", codes[idx].code[b]);
                break;
            }
        }
        for (b = 0; (1 << (bits - nbits)) > b; b++) {
            if ((*table)[code + b].value != -1)
                mpegDebugMessage("VLC probs i=%d '%s' j=%d val %d bits %d pre %x\n",
                            idx, codes[idx].code, b, (*table)[code + b].value, (*table)[code + b].numBits, code);
            (*table)[code + b].value = codes[idx].value;
            (*table)[code + b].numBits = nbits;
        }
    }
}
#endif

#ifdef NETPLAY
void mpegBuildVlcLookupTable(VidStream *vs, VlcEntry **table, short count, VlcSource *codes, short bits)
{
    short b;
    short nbits;
    short code;
    short m;
    short idx;

    *table = mpegHeapAlloc(&vs->heap, (1 << bits) * sizeof(VlcEntry));
    for (idx = 0; (1 << bits) > idx; idx++)
        (*table)[idx].value = -1;
    for (idx = 0; count > idx; idx++) {
        code = 0;
        m = 1 << (bits - 1);
        nbits = 0;
        for (b = 0; strlen(codes[idx].code) > b; b++) {
            if (m == 0)
                mpegDebugMessage("Too many bits in string");
            switch (codes[idx].code[b]) {
            case '1':
                code = m + code;
            case '0':
                m >>= 1;
                nbits++;
                break;
            case ' ':
                break;
            default:
                mpegDebugMessage("Invalid char in string: '%c'", codes[idx].code[b]);
                break;
            }
        }
        for (b = 0; (1 << (bits - nbits)) > b; b++) {
            if ((*table)[code + b].value != -1)
                mpegDebugMessage("VLC probs i=%d '%s' j=%d val %d bits %d pre %x\n",
                            idx, codes[idx].code, b, (*table)[code + b].value, (*table)[code + b].numBits, code);
            (*table)[code + b].value = codes[idx].value;
            (*table)[code + b].numBits = nbits;
        }
    }
}
#endif

/* Restores the source ordering of coded-block-pattern code values. This is the same 6-bit flip
   as mpegInitializeVlcTables, so applying it again undoes the first flip. */
void mpegRestoreCodedBlockPatternCodes(void)
{
    unsigned int i;
    int j;
    int k;

    for (i = 0; i < 63; i++) {
        k = 0;
        for (j = 0; j < 6; j++) {
            if (codedBlockPatternCodes[i].value & (1 << j))
                k += 1 << (5 - j);
        }
        codedBlockPatternCodes[i].value = k;
    }
}
