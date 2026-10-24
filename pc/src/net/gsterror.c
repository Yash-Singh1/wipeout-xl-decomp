#include "gst.h"

/* Name of a GST error code. */
const char *gstErrorString(int error)
{
    if ((error & 0xffff0000) == 0x80100000 && (error & 0xffff) < 10)
        return gstGpiErrorNames[error & 0xffff];
    else if ((error & 0xffff0000) == 0x8f000000 && (error & 0xffff) < 17)
        return gstDpErrorNames[error & 0xffff];
    return gstSdkErrorString(error);
}
