#ifndef __FUZZY_GLOBAL_H
#define __FUZZY_GLOBAL_H

#define _DSEP "/"
#define DATA_FOLDER "." _DSEP "data"
#define PICTURE_FOLDER DATA_FOLDER _DSEP "pictures"
#define FONT_FOLDER DATA_FOLDER _DSEP "fonts"
#define MAP_FOLDER DATA_FOLDER _DSEP "maps"

/* Get a resource */
#define fuzzy_res(folder, resource) folder _DSEP resource

/* Helper macros */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <stdbool.h>
#include <tmx.h>

#define fuzzy_min(x, y) (x < y ? x : y)
#define fuzzy_max(x, y) (x > y ? x : y)
#define fuzzy_str(s) #s

#define fuzzy_log(ticket, msg) fprintf(stderr, "[%s] %s:%d -- %s\n", ticket, basename(__FILE__), __LINE__, msg);
#define fuzzy_debug(msg) fuzzy_log("DEBUG", msg);
#define fuzzy_warning(msg) fuzzy_log("WARNING", msg);
#define fuzzy_error(msg) fuzzy_log("ERROR", msg);
#define fuzzy_critical(msg)\
do{\
    fuzzy_log("CRITICAL", msg);\
    exit(EXIT_FAILURE);\
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

/* FUNCTIONS */

/* internal string format */
#define FUZZY_FORMAT_SIZE 256
char * fuzzy_sformat(char * fmt, ...);

#endif
