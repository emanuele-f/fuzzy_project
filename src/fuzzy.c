#include <stdarg.h>
#include <pthread.h>
#include "fuzzy.h"

static __thread char FmtBuffer[FUZZY_FORMAT_SIZE];

char * fuzzy_sformat(char * fmt, ...)
{
    int size;
    va_list ap;

    va_start(ap, fmt);
    size = vsnprintf(FmtBuffer, FUZZY_FORMAT_SIZE, fmt, ap);
    va_end(ap);

    if (size == sizeof(FmtBuffer)-1)
        fuzzy_warning("Error message truncated to " fuzzy_str(sizeof(FmtBuffer)) " bytes");

    return FmtBuffer;
}
