#include "mpeg.h"

/* Debug trace hook: the printing was compiled out. */
void mpegDebugMessage(const char *msg, ...)
{
}

/* Pushes a function identifier onto the decoder's diagnostic call stack. */
void mpegDebugFunctionEnter(int fn)
{
    if (funcDepth == 16)
        mpegDebugMessage("In %d functions", funcDepth);
    funcStack[funcDepth] = fn;
    funcDepth++;
}

/* Pops one function identifier from the decoder's diagnostic call stack. */
void mpegDebugFunctionLeave(void)
{
    if (funcDepth == 0)
        mpegDebugMessage("Leaving when not in function");
    funcDepth--;
}
