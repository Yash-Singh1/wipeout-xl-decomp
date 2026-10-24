#include "gst.h"
#include "gpi_api.h"


/* Opens the SDK in the given mode. */
int gstLobbyGPI(GstQueryFn mode)
{
    int result;

    if (mode == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0xffff);
    if (gstIsInitialised())
        return gstReportError(0x80000001, 0xffff);
    result = gpiConnectLobby(mode);
    if (result == 3 || result == 4) {
        return gstReportError(result, 0xffff);
    } else if (result < 1) {
        return gstReportError(result, 0xffff);
    }
    return gstReportError(gstReturnSuccess(mode), 0xffff);
}
