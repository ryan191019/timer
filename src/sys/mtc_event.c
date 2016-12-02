
#include "include/mtc.h"
#include "include/mtc_event.h"

static void __sendEvent(EventCode code, int scode, void *data, int size, unsigned char blockmode, const char *file, const int line, const char *func)
{
#if 0
    EventBlock eb;

    eb.code = code;
    eb.scode = scode;
    eb.size = size;
#endif
    switch(code) {
        case EventKey:
        {
        // doEventWinKey(&eb, data);
            ;
        }
        break;

        default:
        {}
        break;
    }

    return;
}

void sendEvent(EventCode code, int scode, void *data, int size, unsigned char blockmode)
{
    __sendEvent(code, scode, data, size, blockmode, __FILE__ , __LINE__, __FUNCTION__);
}
