#include <string.h>
#include "gst.h"

/* Queues a request on the SDK's buffer list and starts it. */
int gstAddRequest(int owner, const char *arg, int id, const GstGuid *guid, GstProviderFn start)
{
    GstBuffer *buffer;
    int result;

    buffer = gstDefaultAlloc(0x1c);
    if (buffer == 0)
        return 0x80000002;
    buffer->owner = owner;
    buffer->id = id;
    if (guid)
        buffer->guid = *guid;
    else
        memset(&buffer->guid, 0, sizeof(GstGuid));
    buffer->next = gstBufferList;
    gstBufferList = buffer;
    result = start(buffer->owner, arg);
    if (result < 1)
        return result;
    return 1;
}
