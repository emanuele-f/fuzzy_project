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

typedef enum FUZZY_LAYERS {
    FUZZY_LAYER_SUB = 0,
    FUZZY_LAYER_BGD = 1,
    FUZZY_LAYER_OBJ = 2,
    FUZZY_LAYER_OVR = 3,
    FUZZY_LAYER_SKY = 4
} FUZZY_LAYERS;

//~ bool fuzzy_map_validate(ALLEGRO_MAP * map);
void fuzzy_map_setup();
tmx_map * fuzzy_map_load(char * mapfile);
ALLEGRO_BITMAP * fuzzy_map_render(tmx_map * map);

#endif
