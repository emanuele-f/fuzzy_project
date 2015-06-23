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
 * A test for fuzzy tile engine
 *
 */
 
#include <mcheck.h>
#include "fuzzy.h"
#include "tiles.h"

static char fake_image_buffer[1];

/* do not call fuzzy_map_setup; use custom loaders instead */
static void* image_loader(const char *path)
{
	return fake_image_buffer;
}

static void* image_freer(void * m)
{
}

static void _setup_map_module()
{
    tmx_img_load_func = image_loader;
	tmx_img_free_func = image_freer;
}

int main()
{
    FuzzyMap * map;    
    
    mtrace();
    _setup_map_module();
    map = fuzzy_map_load("level000.tmx");
    
    fuzzy_map_update(map, 0);
    
    fuzzy_map_unload(map);
    return 0;
}
