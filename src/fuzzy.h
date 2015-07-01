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

#ifndef __FUZZY_GLOBAL_H
#define __FUZZY_GLOBAL_H

#define _DSEP "/"
#ifndef FUZZY_DATA_FOLDER
    #define FUZZY_DATA_FOLDER "." _DSEP "data"
#endif
#define PICTURE_FOLDER FUZZY_DATA_FOLDER _DSEP "pictures"
#define FONT_FOLDER FUZZY_DATA_FOLDER _DSEP "fonts"
#define MAP_FOLDER FUZZY_DATA_FOLDER _DSEP "maps"

/* Get a resource */
#define fuzzy_res(folder, resource) folder _DSEP resource

/* Helper macros */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>
#include <stdbool.h>
#include <tmx.h>

typedef unsigned int uint;
typedef unsigned char ubyte;
typedef unsigned long int ulong;

typedef struct FuzzyPoint {
    ulong x;
    ulong y;
} FuzzyPoint;

#define FUZZY_PI 3.14159265358979323846
#define FUZZY_2PI (2*FUZZY_PI)

#define fuzzy_min(x, y) (x < y ? x : y)
#define fuzzy_max(x, y) (x > y ? x : y)
#define fuzzy_abs(x) ((x) >= 0 ? (x) : -(x))
#define fuzzy_str(s) #s

#define fuzzy_log(ticket, msg) fprintf(fuzzy_log_get(), "[%s] %s:%d -- %s\n", ticket, basename(__FILE__), __LINE__, msg)
#ifndef FUZZY_SUPPRESS_DEBUG
    #define fuzzy_debug(msg) fuzzy_log("DEBUG", msg)
#else
    #define fuzzy_debug(msg)
#endif
#define fuzzy_warning(msg) fuzzy_log("WARNING", msg)
#define fuzzy_error(msg) fuzzy_log("ERROR", msg)
#define fuzzy_critical(msg)\
do{\
    fuzzy_log("CRITICAL", msg);\
    if(! _fuzzy_test_is_enabled()) {\
        exit(EXIT_FAILURE);\
    } else {\
        _fuzzy_test_error();\
        _Pragma("GCC diagnostic push");\
        /* Not ignored currently (gcc 5.1)...*/\
        _Pragma("GCC diagnostic ignored \"-Wreturn-type\"");\
        return;\
        _Pragma("GCC diagnostic pop");\
    }\
}while(0)

#define fuzzy_iz_error(fnret, errmsg)\
do{\
    if((fnret) == 0)\
        fuzzy_critical(errmsg);\
}while(0)

#define fuzzy_nz_error(fnret, errmsg)\
do{\
    if((fnret) != 0)\
        fuzzy_critical(errmsg);\
}while(0)

#define fuzzy_lz_error(fnret, errmsg)\
do{\
    if((fnret) < 0)\
        fuzzy_critical(errmsg);\
}while(0)

/* TODO use strerror_r thread safe version */
#define fuzzy_strerror(error) strerror(error)
#define fuzzy_nz_perror(fnret) fuzzy_nz_error(fnret, fuzzy_strerror(errno))
#define fuzzy_lz_perror(fnret) fuzzy_lz_error(fnret, fuzzy_strerror(errno))
#define fuzzy_iz_perror(fnret) fuzzy_iz_error(fnret, fuzzy_strerror(errno))
#define fuzzy_iz_tmxerror(fnret) fuzzy_iz_error(fnret, tmx_strerr())
#define fuzzy_nz_rerror(fnret) fuzzy_nz_error(fnret, fuzzy_strerror(fnret))
#define fuzzy_lz_rerror(fnret) fuzzy_lz_error(fnret, fuzzy_strerror(fnret))

#define fuzzy_load_addon(addon, fn) fuzzy_iz_error(fn, "Cannot initialize addon '" addon "'")
#define fuzzy_new(tp) ((tp *) fuzzy_alloc(sizeof(tp)))
#define fuzzy_newarr(tp, len) ((tp *) fuzzy_alloc(sizeof(tp) * len))

/* FUNCTIONS */

/* internal string format */
#define FUZZY_FORMAT_SIZE 256
#define FUZZY_LOG_DISCARD "/dev/null"
// TODO define sformat policy
char * fuzzy_sformat(char * fmt, ...);
void fuzzy_log_to(char * logf);
FILE * fuzzy_log_get();

// Disable critical messages and errors, preparing a test function call
void fuzzy_test_prepare();
// Signal a test error
void _fuzzy_test_error();
// If test has been prepared
bool _fuzzy_test_is_enabled();
// Get test result
bool fuzzy_test_result();
// Error checked malloc
void * fuzzy_alloc(ssize_t size);

#endif
