#ifndef __FUZZY_GLOBAL_H
#define __FUZZY_GLOBAL_H

#define DSEP "/"
#define DATA_FOLDER "." DSEP "data"
#define PICTURE_FOLDER DATA_FOLDER DSEP "pictures"
#define FONT_FOLDER DATA_FOLDER DSEP "fonts"
#define MAP_FOLDER DATA_FOLDER DSEP "maps"

/* Get a resource */
#define fuzzy_res(folder, resource) folder DSEP resource

/* Helper macros */
#include <stdio.h>

#define fuzzy_nz_error(fnret, errmsg)\
do{\
    if((fnret) == 0) {\
        fprintf(stderr, errmsg "\n");\
        exit(EXIT_FAILURE);\
    }\
} while(0)

#define fuzzy_load_addon(addon, fn) fuzzy_nz_error(fn, "Cannot initialize addon '" addon "'")

#endif
