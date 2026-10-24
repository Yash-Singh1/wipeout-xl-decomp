#include "gst.h"

extern const char *gstGpiFunctionNames[];
extern const char *gstDpFunctionNames[];
extern const char *gstDpsFunctionNames[];

/* Name of a GST call id (as reported with an error). */
const char *gstFunctionName(int code)
{
    if ((code & 0xffff0000) == 0x100000 && (code & 0xffff) < 11)
        return gstGpiFunctionNames[code & 0xffff];
    else if ((code & 0xffff0000) == 0xf000000 && (code & 0xffff) < 37)
        return gstDpFunctionNames[code & 0xffff];
    else if ((code & 0xffff0000) == 0xf010000 && (code & 0xffff) < 2)
        return gstDpsFunctionNames[code & 0xffff];
    return gstSdkFunctionName(code);
}
