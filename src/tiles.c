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

/* Access tiled functionalities and exploits it to build a dynamic environment
 *
 * Definition: each tile map for this project is structured into 5 layers:
 *  LAYER_FLR: the floar
 *  LAYER_COL: collision objects
 *  LAYER_OBJ: where the game sprites move
 *  LAYER_OVR: anything over the game sprites, like the fortess
 *  LAYER_SKY: up above
 */

#include "fuzzy.h"
#include "tiles.h"

static char LayerName[][10] = {
    "LAYER_SUB",
    "LAYER_BGD",
    "LAYER_OBJ",
    "LAYER_OVR",
    "LAYER_SKY"
};

/* FROM SAMPLE */
#include <stdio.h>
#include <tmx.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

#define LINE_THICKNESS 2.5

static ALLEGRO_COLOR int_to_al_color(int color) {
	unsigned char r, g, b;

	r = (color >> 16) & 0xFF;
	g = (color >>  8) & 0xFF;
	b = (color)       & 0xFF;

	return al_map_rgb(r, g, b);
}

static void* al_img_loader(const char *path) {
	ALLEGRO_BITMAP *res    = NULL;
	ALLEGRO_PATH   *alpath = NULL;

	if (!(alpath = al_create_path(path))) return NULL;

	al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
	res = al_load_bitmap(al_path_cstr(alpath, ALLEGRO_NATIVE_PATH_SEP));

	al_destroy_path(alpath);

	return (void*)res;
}

/*
	Draw objects
*/
static void draw_polyline(int **points, int x, int y, int pointsc, ALLEGRO_COLOR color) {
	int i;
	for (i=1; i<pointsc; i++) {
		al_draw_line(x+points[i-1][0], y+points[i-1][1], x+points[i][0], y+points[i][1], color, LINE_THICKNESS);
	}
}

static void draw_polygone(int **points, int x, int y, int pointsc, ALLEGRO_COLOR color) {
	draw_polyline(points, x, y, pointsc, color);
	if (pointsc > 2) {
		al_draw_line(x+points[0][0], y+points[0][1], x+points[pointsc-1][0], y+points[pointsc-1][1], color, LINE_THICKNESS);
	}
}

static void draw_objects(tmx_object *head, ALLEGRO_COLOR color) {
	while (head) {
		if (head->visible) {
			if (head->shape == S_SQUARE) {
				al_draw_rectangle(head->x, head->y, head->x+head->width, head->y+head->height, color, LINE_THICKNESS);
			} else if (head->shape  == S_POLYGON) {
				draw_polygone(head->points, head->x, head->y, head->points_len, color);
			} else if (head->shape == S_POLYLINE) {
				draw_polyline(head->points, head->x, head->y, head->points_len, color);
			} else if (head->shape == S_ELLIPSE) {
				al_draw_ellipse(head->x + head->width/2.0, head->y + head->height/2.0, head->width/2.0, head->height/2.0, color, LINE_THICKNESS);
			}
		}
		head = head->next;
	}
}

/*
	Draw tiled layers
*/
static int gid_extract_flags(unsigned int gid) {
	int res = 0;

	if (gid & TMX_FLIPPED_HORIZONTALLY) res |= ALLEGRO_FLIP_HORIZONTAL;
	if (gid & TMX_FLIPPED_VERTICALLY)   res |= ALLEGRO_FLIP_VERTICAL;
	/* FIXME allegro has no diagonal flip */
	return res;
}

static int gid_clear_flags(unsigned int gid) {
	return gid & TMX_FLIP_BITS_REMOVAL;
}

static void draw_layer(tmx_map *map, tmx_layer *layer) {
	unsigned long i, j;
	unsigned int x, y, w, h, flags;
	float op;
	tmx_tileset *ts;
	ALLEGRO_BITMAP *tileset;
	op = layer->opacity;
	for (i=0; i<map->height; i++) {
		for (j=0; j<map->width; j++) {
			ts = tmx_get_tileset(map, layer->content.gids[(i*map->width)+j], &x, &y);
			if (ts) {
				w = ts->tile_width; h = ts->tile_height;
				tileset = (ALLEGRO_BITMAP*)ts->image->resource_image;
				flags = gid_extract_flags(layer->content.gids[(i*map->width)+j]);
				al_draw_tinted_bitmap_region(tileset, al_map_rgba_f(op, op, op, op), x, y, w, h, j*ts->tile_width, i*ts->tile_height, flags);
			}
		}
	}
}

/*
	Render map
*/
ALLEGRO_BITMAP* fuzzy_map_render(tmx_map *map) {
	ALLEGRO_BITMAP *res = NULL;
	tmx_layer *layers = map->ly_head;
	unsigned long w, h;

	if (map->orient != O_ORT)
        fuzzy_critical("Only orthogonal orientation currently supported");

	w = map->width  * map->tile_width;  /* Bitmap's width and height */
	h = map->height * map->tile_height;
	if (!(res = al_create_bitmap(w, h)))
        fuzzy_critical("failed to create bitmap");

	al_set_target_bitmap(res);

	al_clear_to_color(int_to_al_color(map->backgroundcolor));

	while (layers) {
		if (layers->visible) {
			if (layers->type == L_OBJGR) {
				draw_objects(layers->content.head, int_to_al_color(layers->color));
			} else if (layers->type == L_IMAGE) {
				if (layers->opacity < 1.) {
					float op = layers->opacity;
					al_draw_tinted_bitmap((ALLEGRO_BITMAP*)layers->content.image->resource_image, al_map_rgba_f(op, op, op, op), 0, 0, 0);
				}
				al_draw_bitmap((ALLEGRO_BITMAP*)layers->content.image->resource_image, 0, 0, 0);
			} else if (layers->type == L_LAYER) {
				draw_layer(map, layers);
			}
		}
		layers = layers->next;
	}

	al_set_target_backbuffer(al_get_current_display());

	return res;
}
/* END FROM SAMPLE */

void fuzzy_map_setup()
{
    tmx_img_load_func = al_img_loader;
	tmx_img_free_func = (void (*)(void*))al_destroy_bitmap;
}

tmx_map * fuzzy_map_load(char * mapfile)
{
    tmx_map * map;
    char * fname;

    fname = fuzzy_sformat("%s%s%s", MAP_FOLDER, _DSEP, mapfile);
    fuzzy_iz_tmxerror(map = tmx_load(fname));

    return map;
}

/* true if maps has correct structure */
/*
bool fuzzy_map_validate(ALLEGRO_MAP * map)
{
    int i;

    for (i=0; i<FUZZY_LAYERS_N; i++) {
        if (al_get_map_layer(map, LayerName[i]) == NULL) {
            fuzzy_warning(fuzzy_sformat("Missing layer '%s'", LayerName[i]));
            return false;
        }
    }
    return true;
}
*/
