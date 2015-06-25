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

#ifndef __FUZZY_TILES_H
#define __FUZZY_TILES_H

#include <allegro5/allegro.h>
#include "fuzzy.h"

#define FUZZY_LAYERS_N 5

/* Map properties names */
#define FUZZY_TILEPROP_ANIMATION_GROUP "g"
#define FUZZY_TILEPROP_FRAME_ID "f"
#define FUZZY_TILEPROP_TRANSITION_TIME "t"

typedef enum FUZZY_LAYERS {
    FUZZY_LAYER_SUB = 0,
    FUZZY_LAYER_BGD = 1,
    FUZZY_LAYER_OBJ = 2,
    FUZZY_LAYER_OVR = 3,
    FUZZY_LAYER_SKY = 4
} FUZZY_LAYERS;

#define FUZZY_LAYER_SPRITES 1

/** Holds map status and data. */
typedef struct FuzzyMap {
    tmx_map * map;                          /* the map data */
    ALLEGRO_BITMAP * bitmap;                /* rendered map */
    struct _AnimatedLayer ** elayers;       /* animation layers */
    struct _AnimationGroup * groups;        /* animation groups */
    uint nlayers;                           /* number of layers */
    double curtime;
    ulong tot_width;
    ulong tot_height;

    /* map information duplication */
    ulong width;
    ulong height;
    ulong tile_width;
    ulong tile_height;
} FuzzyMap;

/** Initialize map module. */
void fuzzy_map_setup();

/** Load a map from a file.

    \param mapfile data source

    \retval a loaded map

    \note on error, program exits with error
 */
FuzzyMap * fuzzy_map_load(char * mapfile);

/** Releases map resources.

    \param map to release
 */
void fuzzy_map_unload(FuzzyMap * map);

/** Render the current map status to a bitmap.

    \param map to render
    \param target to blit
 */
void fuzzy_map_render(FuzzyMap * map, ALLEGRO_BITMAP * target);

/** Updates map internal animation counters and renders internal map.

    \param map to update
    \param time current time in seconds

    \note The rendered map can be accessed through the map->bitmap field
 */
void fuzzy_map_update(FuzzyMap * map, double time);

/** Create a new sprite at (x, y)

    \param map object
    \param grp the animation group the sprite belongs to
    \param x where to create
    \param y where to create

    \note an error is raised if given position is not empty
 */
void fuzzy_sprite_create(FuzzyMap * map, ulong grp, ulong x, ulong y);

/** Moves a sprite to (ox, oy) to (nx, ny)

    \param map object
    \param ox old x
    \param oy old y
    \param nx new x
    \param ny new y

    \note an error is raised if sprite does exist at coords (ox, oy)
 */
void fuzzy_sprite_move(FuzzyMap * map, ulong ox, ulong oy, ulong nx, ulong ny);

#endif
