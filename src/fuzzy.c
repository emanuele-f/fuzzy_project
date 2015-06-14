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
