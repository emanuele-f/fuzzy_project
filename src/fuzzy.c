/*
 * Emanuele Faranda         black.silver@hotmail.it
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdarg.h>
#include <pthread.h>
#include "fuzzy.h"

static __thread char FmtBuffer[FUZZY_FORMAT_SIZE];
static __thread FILE * NullLog = NULL;
static __thread FILE * Logfile = NULL;
static __thread FILE * OldLogfile = NULL;
static __thread bool TestFlag = false;
static __thread bool ErrorFlag = false;

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

void fuzzy_log_to(char * logf)
{
    FILE * f;

    /* cleanup */
    if (Logfile != NULL && Logfile != NullLog) {
        fclose(Logfile);
    }

    /* only open FUZZY_LOG_DISCARD once */
    if (strncmp(logf, FUZZY_LOG_DISCARD, sizeof(FUZZY_LOG_DISCARD)) == 0) {
        if(NullLog == NULL)
            fuzzy_iz_perror(NullLog = fopen(FUZZY_LOG_DISCARD, "r"));
        f = NullLog;
    } else {
        fuzzy_iz_perror(f = fopen(logf, "r"));
    }
    Logfile = f;
}

FILE * fuzzy_log_get()
{
    if (Logfile == NULL)
        Logfile = stderr;
    return Logfile;
}

void fuzzy_test_prepare()
{
    TestFlag = true;
    ErrorFlag = false;
    OldLogfile = Logfile;
    fuzzy_log_to(FUZZY_LOG_DISCARD);
}

void _fuzzy_test_error()
{
    ErrorFlag = true;
}

bool _fuzzy_test_is_enabled()
{
    return TestFlag;
}

bool fuzzy_test_result()
{
    bool val;

    if (! TestFlag)
        fuzzy_critical("Test has not been prepared");
    TestFlag = false;
    val = ErrorFlag;
    ErrorFlag = false;
    Logfile = OldLogfile;

    return val;
}

void * fuzzy_alloc(ssize_t size)
{
    void * ptr;

    fuzzy_iz_perror(ptr = malloc(size));
    return ptr;
}
