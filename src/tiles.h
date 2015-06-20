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

typedef struct FuzzyMap {
    tmx_map * map;                          /* the actual map */
    ALLEGRO_BITMAP * bitmap;                /* rendered image */
    struct _AnimatedLayer ** elayers;       /* the engine working data */
    struct _AnimationGroup * groups;        /* animation groups */
    uint nlayers;                          /* number of layers */
    double curtime;

    /* map information duplication */
    ulong width;
    ulong height;
    ulong tile_width;
    ulong tile_height;
} FuzzyMap;

//~ bool fuzzy_map_validate(ALLEGRO_MAP * map);
void fuzzy_map_setup();
FuzzyMap * fuzzy_map_load(char * mapfile);
void fuzzy_map_unload(FuzzyMap * map);
void fuzzy_map_render(FuzzyMap * map, ALLEGRO_BITMAP * target);
void fuzzy_map_update(FuzzyMap * map, double time);

#endif
