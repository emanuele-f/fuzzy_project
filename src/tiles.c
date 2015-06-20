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

#include <stdbool.h>
#include "fuzzy.h"
#include "tiles.h"

static char LayerName[][10] = {
    "LAYER_SUB",
    "LAYER_BGD",
    "LAYER_OBJ",
    "LAYER_OVR",
    "LAYER_SKY"
};

typedef unsigned long ulong;

/* rappresents a single animation frame */
struct _AnimationFrame {
    ulong fid;                              /* frame id within the animation group */
    double transtime;                       /* time before next frame */
    uint tx;                                /* x offset within tileset */
    uint ty;                                /* y offset within tileset */
    struct _AnimationFrame * next;          /* next frame in group */
};

/* holds a group of frames */
struct _AnimationGroup {
    ulong id;                               /* id of the animation group */
    struct _AnimationFrame * frames;        /* list of animation frames */
    struct _AnimationGroup * next;
};

/* an animated tile */
struct _AnimatedObject {
    ulong x;                                /* x position in layer */
    ulong y;                                /* y position in layer */
    ulong grp;                              /* the group id used */
    ulong curframe;                         /* fid of the current frame */
    double difftime;                        /* tracks time */
    struct _AnimatedObject * next;
};

/* holds layer information in map engine */
struct _AnimatedLayer {
    ulong lid;                              /* layer ID */
    struct _AnimatedObject * sprites;       /* list of animated tiles in layer */
    ALLEGRO_BITMAP * bitmap;                /* cached bitmap */
};

static struct _AnimatedLayer * _new_map_layer(ulong id)
{
    struct _AnimatedLayer * elayer;

    elayer = (struct _AnimatedLayer *) malloc(sizeof(struct _AnimatedLayer));
    fuzzy_iz_perror(elayer);

    elayer->lid = id;
    elayer->sprites = NULL;
    elayer->bitmap = NULL;

    return elayer;
}

static void _unload_group_frames(struct _AnimationFrame * frames)
{
    struct _AnimationFrame *d, *frame;

    frame = frames;
    while(frame) {
        d = frame;
        frame = frame->next;
        free(d);
    }
}

static void _unload_animation_groups(struct _AnimationGroup * groups)
{
    struct _AnimationGroup *d, *group;

    group = groups;
    while (group) {
        _unload_group_frames(group->frames);
        d = group;
        group = group->next;
        free(d);
    }
}

static void _unload_map_layer(struct _AnimatedLayer * elayer)
{
    struct _AnimatedObject *sprite, *d;

    sprite = elayer->sprites;
    while (sprite) {
        d = sprite;
        sprite = sprite->next;
        free(d);
    }
}

static void _unload_map_layers(FuzzyMap * fmap) {
    ulong i;

    for (i=0; i < fmap->nlayers; i++) {
        _unload_map_layer(fmap->elayers[i]);
        free(fmap->elayers[i]);
    }
    _unload_animation_groups(fmap->groups);

    free(fmap->elayers);
}

/* Get a tile property. Null if not found */
static char * _get_tile_property(tmx_tile * tile, char * prop)
{
    tmx_property * properties = tile->properties;

    while (properties) {
        if(strcmp(prop, properties->name) == 0)
            return properties->value;
        properties = properties->next;
    }
    return NULL;
}

/* get it or die */
static ulong _get_tile_ulong_property(tmx_tile * tile, char * prop)
{
    char * val;

    fuzzy_iz_error(val = _get_tile_property(tile, prop),
      fuzzy_sformat("Missing '%s' mandatory property", prop)
    );

    return atol(val);
}

/* compute a gid for the given position */
static uint _fuzzy_layer_gid(tmx_map *map, tmx_layer *layer, ulong x, ulong y)
{
    return layer->content.gids[(y*map->width)+x];
}

static void _load_group_frames(struct _AnimationGroup * group, tmx_tileset * ts)
{
    struct _AnimationFrame *frame, *last;
    tmx_tile * tile;
    char * grp_s;
    ulong grp, fid, msec;

    printf("Loading frames for group %ld\n", group->id);

    last = NULL;
    tile = ts->tiles;
    while(tile) {
        grp_s = _get_tile_property(tile, FUZZY_TILEPROP_ANIMATION_GROUP);
        if (grp_s) {
            grp = atol(grp_s);
            if (grp == group->id) {
                fid = _get_tile_ulong_property(tile, FUZZY_TILEPROP_FRAME_ID);
                msec = _get_tile_ulong_property(tile, FUZZY_TILEPROP_TRANSITION_TIME);

                /* compute tile offset in tileset */
                uint id = tile->id;
                uint ts_w = ts->image->width  - 2 * (ts->margin) + ts->spacing;
				uint tiles_x_count = ts_w / (ts->tile_width  + ts->spacing);
                uint tx = id % tiles_x_count;
                uint ty = id / tiles_x_count;

                frame = (struct _AnimationFrame *) malloc(sizeof(struct _AnimationFrame));
                fuzzy_iz_perror(frame);
                frame->fid = fid;
                frame->tx = ts->margin + (tx * ts->tile_width)  + (tx * ts->spacing);
                frame->ty = ts->margin + (ty * ts->tile_height) + (ty * ts->spacing);
                frame->transtime = (msec * 1.) / 1000;
                frame->next = NULL;

                /* add to frame list */
                if (last != NULL)
                    last->next = frame;
                else
                    group->frames = frame;
                last = frame;
            }
        }
        tile = tile->next;
    }
}

/* Ensure arg groups contains the needed group.
    If it was not already loaded, load it.

    Returns the group
 */
static struct _AnimationGroup * _load_animation_group(FuzzyMap * fmap, ulong grp, tmx_tileset * ts)
{
    struct _AnimationGroup *group, *last;

    /* look for existing group */
    group = fmap->groups;
    while (group) {
        if (group->id == grp)
            break;
        group = group->next;
    }

    if (group == NULL) {
        /* load a new one */
        group = (struct _AnimationGroup *) malloc(sizeof(struct _AnimationGroup));
        fuzzy_iz_perror(group);
        group->id = grp;
        group->frames = NULL;
        group->next = NULL;
        _load_group_frames(group, ts);

        /* add to list */
        if (fmap->groups == NULL) {
            fmap->groups = group;
        } else {
            last = fmap->groups;
            while (last->next != NULL)
                last = last->next;
            last->next = group;
        }
    }

    return group;
}

/* analize a layer to find animated objects */
static struct _AnimatedLayer * _discover_layer_objects(FuzzyMap * fmap, tmx_layer *layer, ulong lid)
{
    ulong i, j;
    struct _AnimatedLayer * elayer;
    struct _AnimatedObject *obj, *last;
    tmx_tile * tile;
    tmx_tileset * ts;
    char * grp_s;
    ulong grp;
    tmx_map * map = fmap->map;
    uint _x, _y;

    elayer = _new_map_layer(lid);
    last = NULL;

    for (i=0; i<map->height; i++) {
		for (j=0; j<map->width; j++) {
            uint gid = _fuzzy_layer_gid(map, layer, j, i);
            tile = tmx_get_tile(map, gid);
            if (tile) {
                grp_s = _get_tile_property(tile, FUZZY_TILEPROP_ANIMATION_GROUP);
                if (grp_s) {
                    grp = atol(grp_s);
                    printf("Tile: tid=%d group=%ld @%d,%d\n", tile->id, grp, j, i);

                    fuzzy_iz_error(ts = tmx_get_tileset(map, gid, &_x, &_y), "Tileset cannot be Null here!");
                    _load_animation_group(fmap, grp, ts);

                    /* create a sprite descriptor */
                    obj = (struct _AnimatedObject *) malloc(sizeof(struct _AnimatedObject));
                    fuzzy_iz_perror(obj);
                    obj->x = j;
                    obj->y = i;
                    obj->grp = grp;
                    obj->curframe = 0;
                    obj->difftime = 0;
                    obj->next = NULL;

                    /* add to layer list */
                    if (last != NULL)
                        last->next = obj;
                    else
                        elayer->sprites = obj;
                    last = obj;
                }
            }
        }
    }

    return elayer;
}

static void _fuzzy_map_analize(FuzzyMap * fmap)
{
    tmx_layer * layer;
    struct _AnimatedLayer ** elayers;
    ulong i, nlayers;

    /* count layers number */
    i = 0;
    layer = fmap->map->ly_head;
    while (layer) {
        i++;
        layer = layer->next;
    }
    nlayers = i;

    /* allocate structure */
    elayers = (struct _AnimatedLayer **) malloc(sizeof(struct _AnimatedLayer **) * nlayers);
    fuzzy_iz_perror(elayers);

    /* fill engine layers */
    layer = fmap->map->ly_head;
    for (i=0; i<nlayers; i++) {
        elayers[i] = _discover_layer_objects(fmap, layer, i);
        layer = layer->next;
    }

    /* fill input structure */
    fmap->nlayers = nlayers;
    fmap->elayers = elayers;
}

/* Get an animation object if present, otherwise NULL */
static struct _AnimatedObject * _get_tile_animation(FuzzyMap * fmap, uint lid, ulong x, ulong y)
{
    struct _AnimatedObject * sprite;

    if (lid >= fmap->nlayers) {
        fuzzy_error(fuzzy_sformat("Layer '%d' out of available layers (%d)", lid, fmap->nlayers));
        return NULL;
    }

    sprite = fmap->elayers[lid]->sprites;
    while(sprite) {
        if (sprite->x == x && sprite->y==y)
            return sprite;
        sprite = sprite->next;
    }

    return NULL;
}

static struct _AnimationFrame * _get_sprite_frame(
  FuzzyMap * fmap,
  struct _AnimatedObject * sprite,
  struct _AnimationGroup ** resgroup
) {
    struct _AnimationGroup * group;
    struct _AnimationFrame * frame;

    /* find animation group */
    group = fmap->groups;
    while (group) {
        if (group->id == sprite->grp)
            break;
        group = group->next;
    }
    if (group == NULL)
        fuzzy_critical(fuzzy_sformat("Animation group #%d not loaded!", sprite->grp));

    /* get the frame */
    frame = group->frames;
    while (frame) {
        if (frame->fid == sprite->curframe)
            break;
        frame = frame->next;
    }
    if (frame == NULL)
        fuzzy_critical(fuzzy_sformat("Cannot find animation frame '%d' for group #%d at %d,%d", sprite->curframe, sprite->grp, sprite->x, sprite->y));

    /* also return the group */
    if (resgroup != NULL)
        *resgroup = group;
    return frame;
}

static int gid_extract_flags(unsigned int gid) {
	int res = 0;

	if (gid & TMX_FLIPPED_HORIZONTALLY) res |= ALLEGRO_FLIP_HORIZONTAL;
	if (gid & TMX_FLIPPED_VERTICALLY)   res |= ALLEGRO_FLIP_VERTICAL;
	/* FIXME allegro has no diagonal flip */
	return res;
}

static void _draw_layer(FuzzyMap *fmap, tmx_layer *layer, uint lid) {
	unsigned long i, j;
	unsigned int x, y, w, h, flags;
	float op;
	tmx_tileset *ts;
	ALLEGRO_BITMAP *tileset;
    tmx_map * map = fmap->map;
    struct _AnimatedObject * sprite;
    struct _AnimationFrame * frame;
    uint tx, ty;

	op = layer->opacity;

	for (i=0; i<map->height; i++) {
		for (j=0; j<map->width; j++) {
			ts = tmx_get_tileset(map, layer->content.gids[(i*map->width)+j], &x, &y);
			if (ts) {
                w = ts->tile_width; h = ts->tile_height;
                tileset = (ALLEGRO_BITMAP*)ts->image->resource_image;
                flags = gid_extract_flags(layer->content.gids[(i*map->width)+j]);

                sprite = _get_tile_animation(fmap, lid, j, i);
                if (! sprite) {
                    /* standard tile*/
                    tx = x;
                    ty = y;
                } else {
                    /* animated tile */
                    frame = _get_sprite_frame(fmap, sprite, NULL);
                    tx = frame->tx;
                    ty = frame->ty;
                }

                al_draw_tinted_bitmap_region(tileset, al_map_rgba_f(op, op, op, op), tx, ty, w, h, j*ts->tile_width, i*ts->tile_height, flags);
			}
		}
	}
}

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

static int gid_clear_flags(unsigned int gid) {
	return gid & TMX_FLIP_BITS_REMOVAL;
}

/*
	Render map
*/
void fuzzy_map_render(FuzzyMap *fmap, ALLEGRO_BITMAP * target) {
    tmx_map * map = fmap->map;
	tmx_layer *layers = map->ly_head;
    uint i;

	if (map->orient != O_ORT)
        fuzzy_critical("Only orthogonal orientation currently supported");

	al_set_target_bitmap(target);
	al_clear_to_color(int_to_al_color(map->backgroundcolor));

    i=0;
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
				_draw_layer(fmap, layers, i);
			}
		}
		layers = layers->next;
        i++;
	}

	al_set_target_backbuffer(al_get_current_display());
}

/* updates internal map state. Internal map bitmap is also rendered. */
void fuzzy_map_update(FuzzyMap * fmap, double time)
{
    struct _AnimatedLayer * elayer;
    struct _AnimatedObject * sprite;
    struct _AnimationGroup * group;
    struct _AnimationFrame * frame;
    double tdiff;
    uint i;

    tdiff = time - fmap->curtime;
    fmap->curtime = time;

    /* update frames */
    for (i=0; i<fmap->nlayers; i++) {
        elayer = fmap->elayers[i];
        sprite = elayer->sprites;

        while(sprite) {
            frame = _get_sprite_frame(fmap, sprite, &group);

            sprite->difftime += tdiff;
            while (sprite->difftime >= frame->transtime) {
                /* next frame */
                sprite->difftime -= frame->transtime;

                if (frame->next != NULL) {
                    sprite->curframe++;
                    frame = frame->next;
                } else {
                    /* back to the first */
                    sprite->curframe = 0;
                    frame = group->frames;
                }
            }

            sprite = sprite->next;
        }
    }

    fuzzy_map_render(fmap, fmap->bitmap);
}
/* END FROM SAMPLE */

void fuzzy_map_setup()
{
    tmx_img_load_func = al_img_loader;
	tmx_img_free_func = (void (*)(void*))al_destroy_bitmap;
}

FuzzyMap * fuzzy_map_load(char * mapfile)
{
    tmx_map * map;
    FuzzyMap * fmap;
    char * fname;
    unsigned long w, h;

    fname = fuzzy_sformat("%s%s%s", MAP_FOLDER, _DSEP, mapfile);
    fuzzy_iz_tmxerror(map = tmx_load(fname));

    fuzzy_iz_perror(fmap = (FuzzyMap *) malloc(sizeof(FuzzyMap)));
    fmap->map = map;
    fmap->elayers = NULL;
    fmap->groups = NULL;
    fmap->width = map->width;
    fmap->height = map->height;
    fmap->tile_width = map->tile_width;
    fmap->tile_height = map->tile_height;
    fmap->curtime = 0;

    /* create map bitmap */

	w = map->width  * map->tile_width;
	h = map->height * map->tile_height;
	if (! (fmap->bitmap = al_create_bitmap(w, h)) )
        fuzzy_critical("Failed to create map bitmap");

    _fuzzy_map_analize(fmap);
    return fmap;
}

void fuzzy_map_unload(FuzzyMap * fmap)
{
    _unload_map_layers(fmap);
    tmx_map_free(fmap->map);
    al_destroy_bitmap(fmap->bitmap);
    free(fmap);
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
