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
 *  LAYER_FLR: the floor, such as grass or cement
 *  LAYER_BEL: layer below the sprites
 *  LAYER_SPR: sprites layer
 *  LAYER_OVR: layer over the sprites
 *  LAYER_SKY: above everything
 */

#include <stdio.h>
#include <stdbool.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <tmx.h>
#include "fuzzy.h"
#include "tiles.h"

#define LINE_THICKNESS 2.5

static char LayerNames[][10] = {
    "LAYER_FLR",
    "LAYER_BEL",
    "LAYER_SPR",
    "LAYER_ABO",
    "LAYER_SKY"
};

typedef unsigned long ulong;

/** Holds animation frame description */
struct _AnimationFrame {
    ulong fid;                              /* frame id within the animation group */
    double transtime;                       /* time before next frame */
    uint tx;                                /* x offset within tileset */
    uint ty;                                /* y offset within tileset */
    struct _AnimationFrame * next;          /* next frame in group */
};

/** Holds a group of related animation frames */
struct _AnimationGroup {
    ulong id;                               /* id of the animation group */
    struct _AnimationFrame * frames;        /* list of animation frames */
    struct _AnimationGroup * next;
};

/** An animation instance object */
struct _AnimatedSprite {
    ulong grp;                              /* the group id used */
    ulong x;                                /* x position in layer */
    ulong y;                                /* y position in layer */
    ulong curframe;                         /* fid of the current frame */
    double difftime;                        /* tracks time for transition */
    struct _AnimatedSprite * next;
};

/** Contains the list of coordinates which hold an animated sprites */
struct _AnimatedLayer {
    uint lid;                              /* layer ID */
    struct _AnimatedSprite * sprites;       /* list of animated tiles in layer */
    ALLEGRO_BITMAP * bitmap;                /* cached static bitmap for layer */
};

/** Describes a tile information */
struct _TileInfo {
    uint gid;
    tmx_tileset * ts;
    tmx_tile * tile;        /* could contain tile specific info */
    uint tx;                /* x offset inside tileset */
    uint ty;                /* y offset inside tileset */
};

/*-------------------------- UTILITY METHODS -----------------------------*/

/** Get special flags from gid */
static int _gid_extract_flags(unsigned int gid) {
	int res = 0;

	if (gid & TMX_FLIPPED_HORIZONTALLY) res |= ALLEGRO_FLIP_HORIZONTAL;
	if (gid & TMX_FLIPPED_VERTICALLY)   res |= ALLEGRO_FLIP_VERTICAL;
	/* FIXME allegro has no diagonal flip */
	return res;
}

/** Find a property of the tile
    \retval string property on success
    \retval NULL on not found
 */
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

/** Get a numeric unsigned long property of the tile
    \retval property value

    \note if property is not found, program exits with error
 */
static ulong _get_tile_ulong_property(tmx_tile * tile, char * prop)
{
    char * val;

    fuzzy_iz_error(val = _get_tile_property(tile, prop),
      fuzzy_sformat("Missing '%s' mandatory property", prop)
    );

    return atol(val);
}

/** Computes the gid for the given coords on the layer */
static uint _get_gid_in_layer(tmx_map *map, tmx_layer *layer, ulong x, ulong y)
{
    if (x >= map->width || y >= map->height)
        fuzzy_critical(fuzzy_sformat("Map coords outside bounds [%d,%d]: %d,%d", map->width, map->height, x, y));
    return layer->content.gids[(y*map->width)+x];
}

static struct _AnimatedLayer * _get_animation_layer(FuzzyMap * fmap, uint lid)
{
    if (lid >= fmap->nlayers)
        fuzzy_critical(fuzzy_sformat("Layer '%d' out of available layers (%d)", lid, fmap->nlayers));

    return fmap->elayers[lid];
}

/* get layer by id or die */
static tmx_layer * _get_tmx_layer(tmx_map * map, uint lid)
{
    uint i;
    tmx_layer * layer;

    layer = map->ly_head;
    for(i=0; i<=lid; i++) {
        if (layer == NULL)
            fuzzy_critical(fuzzy_sformat("Layer '%d' out of available layers (%d)", lid, i));
        if (i == lid)
            return layer;
        layer = layer->next;
    }

    return NULL;
}

/** Get the sprite animation group.

    \retval animation group

    \note if group is not found, program exits with error
 */
static struct _AnimationGroup * _get_sprite_group(FuzzyMap * fmap, struct _AnimatedSprite * sprite)
{
    struct _AnimationGroup * group;

    group = fmap->groups;
    while (group) {
        if (group->id == sprite->grp)
            break;
        group = group->next;
    }
    if (group == NULL)
        fuzzy_critical(fuzzy_sformat("Animation group #%d not loaded!", sprite->grp));

    return group;
}

/** Get the current frame for the sprite.

    \param fmap
    \param sprite
    \param group sprite animation group

    \retval the sprite frame descriptor

    \note if frame is not found, program exits with error
 */
static struct _AnimationFrame * _get_sprite_frame(
  FuzzyMap * fmap,
  struct _AnimatedSprite * sprite,
  struct _AnimationGroup * group
) {
    struct _AnimationFrame * frame;

    /* get the frame */
    frame = group->frames;
    while (frame) {
        if (frame->fid == sprite->curframe)
            break;
        frame = frame->next;
    }
    if (frame == NULL)
        fuzzy_critical(fuzzy_sformat("Cannot find animation frame '%d' for group #%d at %d,%d",
          sprite->curframe, sprite->grp, sprite->x, sprite->y));

    return frame;
}

/** Converts an integer rgb color to an ALLEGRO_COLOR */
static ALLEGRO_COLOR int_to_al_color(int color)
{
	unsigned char r, g, b;

	r = (color >> 16) & 0xFF;
	g = (color >>  8) & 0xFF;
	b = (color)       & 0xFF;

	return al_map_rgb(r, g, b);
}

/** Loads an ALLEGRO_BITMAP. Used as the image load hook. */
static void* al_img_loader(const char *path)
{
	ALLEGRO_BITMAP *res    = NULL;
	ALLEGRO_PATH   *alpath = NULL;

	if (!(alpath = al_create_path(path))) return NULL;

	al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
	res = al_load_bitmap(al_path_cstr(alpath, ALLEGRO_NATIVE_PATH_SEP));

	al_destroy_path(alpath);

	return (void*)res;
}

/** Appends a sprite to layer list */
void _layer_sprite_append(struct _AnimatedLayer * layer, struct _AnimatedSprite * sp)
{
    struct _AnimatedSprite * last = layer->sprites;

    if (last == NULL) {
        layer->sprites = sp;
    } else {
        while(last->next)
            last = last->next;

        last->next = sp;
    }

    fuzzy_nz_error(sp->next, "Sprite next pointer not null");
}

/** Look for a tileset containing the specified animation group */
tmx_tileset * _get_tileset_for_group(FuzzyMap * fmap, ulong grp)
{
    tmx_tile * tile;
    tmx_tileset * ts;
    char * grp_s;
    ulong grp_l;

    ts = fmap->map->ts_head;
    while (ts) {
        /* todo find grp */
        tile = ts->tiles;
        while(tile) {
            grp_s = _get_tile_property(tile, FUZZY_TILEPROP_ANIMATION_GROUP);
            if (grp_s) {
                grp_l = atol(grp_s);
                if (grp_l == grp)
                    return ts;
            }
            tile = tile->next;
        }
        ts = ts->next;
    }

    fuzzy_critical(fuzzy_sformat("Tileset for group %d not found", grp));
}

/** Find an animated sprite at coords in layer.
    \param precsprite if not NULL, will point to preceding sprite

    \retval sprite on success
    \retval NULL on not found
 */
static struct _AnimatedSprite * _get_sprite_at(struct _AnimatedLayer * elayer, ulong x, ulong y, struct _AnimatedSprite ** precsprite)
{
    struct _AnimatedSprite * sprite;
    struct _AnimatedSprite * prec;

    prec = NULL;
    sprite = elayer->sprites;
    while(sprite) {
        if (sprite->x == x && sprite->y == y) {
            if (precsprite != NULL)
                *precsprite = prec;
            return sprite;
        }
        prec = sprite;
        sprite = sprite->next;
    }
    return NULL;
}

/* nb real tile information is not saved into ts->tiles list, it is
 * rather stored into layer->content.gids where 0 indicates empty tile
 * 
 * if info != NULL, save tile information inside
 * Returns true: tile found, false:not found
 */
static bool _get_tile_at(tmx_map * map, uint lid, ulong x, ulong y, struct _TileInfo * info)
{
    tmx_tileset * ts;
    tmx_layer * layer;
    uint gid;
    uint tx, ty;
    
    layer = _get_tmx_layer(map, lid);
    gid = _get_gid_in_layer(map, layer, x, y);
    
    ts = tmx_get_tileset(map, gid, &tx, &ty);
    if (ts) {
        /* tile found */
        if (info != NULL) {
            info->gid = gid;
            info->ts = ts;
            info->tx = tx;
            info->ty = ty;
            
            /* try to get tile specific info */
            info->tile = tmx_get_tile(map, gid);
        }
        return true;
    }
    return false;
}

FUZZY_CELL_TYPE fuzzy_map_spy(FuzzyMap * fmap, uint lid, ulong x, ulong y)
{
    tmx_map * map = fmap->map;
    
    if (_get_sprite_at(_get_animation_layer(fmap, lid), x, y, NULL))
        return FUZZY_CELL_SPRITE;
    else if (_get_tile_at(fmap->map, lid, x, y, NULL))
        return FUZZY_CELL_TILE;
    else
        return FUZZY_CELL_EMPTY;
}

/*---------------------------- DRAW METHODS ------------------------------*/

/** Draws a line with multiple points. */
static void _draw_polyline(int **points, int x, int y, int pointsc, ALLEGRO_COLOR color)
{
	int i;
	for (i=1; i<pointsc; i++) {
		al_draw_line(x+points[i-1][0], y+points[i-1][1], x+points[i][0],
          y+points[i][1], color, LINE_THICKNESS);
	}
}

/** Draws a closed polygone. */
static void _draw_polygone(int **points, int x, int y, int pointsc, ALLEGRO_COLOR color)
{
	_draw_polyline(points, x, y, pointsc, color);
	if (pointsc > 2) {
		al_draw_line(x+points[0][0], y+points[0][1], x+points[pointsc-1][0],
          y+points[pointsc-1][1], color, LINE_THICKNESS);
	}
}

/** Draws different shapes. */
static void _draw_objects(tmx_object *head, ALLEGRO_COLOR color)
{
	while (head) {
		if (head->visible) {
			if (head->shape == S_SQUARE) {
				al_draw_rectangle(head->x, head->y, head->x+head->width,
                  head->y+head->height, color, LINE_THICKNESS);
			} else if (head->shape  == S_POLYGON) {
				_draw_polygone(head->points, head->x, head->y, head->points_len, color);
			} else if (head->shape == S_POLYLINE) {
				_draw_polyline(head->points, head->x, head->y, head->points_len, color);
			} else if (head->shape == S_ELLIPSE) {
				al_draw_ellipse(head->x + head->width/2.0,
                  head->y + head->height/2.0, head->width/2.0,
                  head->height/2.0, color, LINE_THICKNESS);
			}
		}
		head = head->next;
	}
}

/** Renders a static layer onto the current target bitmap. */
static void _render_layer(FuzzyMap *fmap, tmx_layer *layer, struct _AnimatedLayer * elayer) {
	ulong i, j;
	uint x, y, w, h, flags;
	float op;
	tmx_tileset *ts;
	ALLEGRO_BITMAP *tileset;
    tmx_map * map = fmap->map;

    op = layer->opacity;

	for (i=0; i<map->height; i++) {
		for (j=0; j<map->width; j++) {
			ts = tmx_get_tileset(map, _get_gid_in_layer(map, layer, j, i), &x, &y);
			if (ts) {
                w = ts->tile_width; h = ts->tile_height;
                tileset = (ALLEGRO_BITMAP*)ts->image->resource_image;
                flags = _gid_extract_flags(_get_gid_in_layer(map, layer, j, i));

                if (! _get_sprite_at(elayer, j, i, NULL)) {
                    /* standard tile*/
                    al_draw_tinted_bitmap_region(tileset, al_map_rgba_f(op, op, op, op),
                      x, y, w, h, j*ts->tile_width, i*ts->tile_height, flags);
                }
			}
		}
	}
}

static void _render_sprites(FuzzyMap *fmap, tmx_layer * layer, struct _AnimatedLayer * elayer)
{
    struct _AnimatedSprite * sprite;
    struct _AnimationFrame * frame;
    tmx_tileset *ts;
    uint w, h, flags;
    ALLEGRO_BITMAP * tileset;
    float op;

    op = layer->opacity;

    sprite = elayer->sprites;
    while(sprite) {
        frame = _get_sprite_frame(fmap, sprite, _get_sprite_group(fmap, sprite));
        ts = _get_tileset_for_group(fmap, sprite->grp);

        w = ts->tile_width; h = ts->tile_height;
        tileset = (ALLEGRO_BITMAP*)ts->image->resource_image;
        flags = _gid_extract_flags(_get_gid_in_layer(fmap->map, layer, sprite->x, sprite->y));
        al_draw_tinted_bitmap_region(tileset, al_map_rgba_f(op, op, op, op),
          frame->tx, frame->ty, w, h, sprite->x*ts->tile_width, sprite->y*ts->tile_height, flags);

        sprite = sprite->next;
    }
}

void fuzzy_map_update(FuzzyMap * fmap, double time)
{
    struct _AnimatedLayer * elayer;
    struct _AnimatedSprite * sprite;
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
            group = _get_sprite_group(fmap, sprite);
            frame = _get_sprite_frame(fmap, sprite, group);

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

void fuzzy_map_render(FuzzyMap *fmap, ALLEGRO_BITMAP * target) {
    struct _AnimatedLayer * elayer;
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
				_draw_objects(layers->content.head, int_to_al_color(layers->color));
			} else if (layers->type == L_IMAGE) {
				if (layers->opacity < 1.) {
					float op = layers->opacity;
					al_draw_tinted_bitmap(
                      (ALLEGRO_BITMAP*)layers->content.image->resource_image,
                      al_map_rgba_f(op, op, op, op), 0, 0, 0);
				}
				al_draw_bitmap((ALLEGRO_BITMAP*)layers->content.image->resource_image, 0, 0, 0);
			} else if (layers->type == L_LAYER) {
                elayer = _get_animation_layer(fmap, i);
                al_draw_bitmap(elayer->bitmap, 0, 0, 0);
                _render_sprites(fmap, layers, elayer);
			}
		}
		layers = layers->next;
        i++;
	}

	al_set_target_backbuffer(al_get_current_display());
}

/*---------------------------- LOAD METHODS ------------------------------*/

void fuzzy_map_setup()
{
    tmx_img_load_func = al_img_loader;
	tmx_img_free_func = (void (*)(void*))al_destroy_bitmap;
}

/** Allocate a new _AnimatedLayer */
static struct _AnimatedLayer * _new_map_layer(FuzzyMap * fmap, tmx_layer * layer, ulong id)
{
    struct _AnimatedLayer * elayer;

    elayer = fuzzy_new(struct _AnimatedLayer);
    fuzzy_iz_perror(elayer);

    elayer->lid = id;
    elayer->sprites = NULL;
    elayer->bitmap = NULL;
    return elayer;
}

/** Allocate a new _AnimatedSprite */
static struct _AnimatedSprite * _new_sprite(ulong group)
{
    struct _AnimatedSprite * sp;

    sp = fuzzy_new(struct _AnimatedSprite);
    fuzzy_iz_perror(sp);
    sp->x = 0;
    sp->y = 0;
    sp->grp = group;
    sp->curframe = 0;
    sp->difftime = 0;
    sp->next = NULL;
}

/** Load frames information for an animation group. */
static void _load_group_frames(struct _AnimationGroup * group, tmx_tileset * ts)
{
    struct _AnimationFrame *frame, *cur;
    tmx_tile * tile;
    char * grp_s;
    ulong grp, fid, msec;

    fuzzy_debug(fuzzy_sformat("Loading animation group %ld", group->id));

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

                frame = fuzzy_new(struct _AnimationFrame);
                fuzzy_iz_perror(frame);
                frame->fid = fid;
                frame->tx = ts->margin + (tx * ts->tile_width)  + (tx * ts->spacing);
                frame->ty = ts->margin + (ty * ts->tile_height) + (ty * ts->spacing);
                frame->transtime = (msec * 1.) / 1000;
                frame->next = NULL;

                /* add to frame list */
                if (group->frames == NULL)
                    group->frames = frame;
                else {
                    /* find a place */
                    cur = group->frames;
                    while (cur->next != NULL && cur->fid <= fid) {
                        if (cur->fid == fid)
                            fuzzy_critical(fuzzy_sformat("Frame '%d' for group #%d already loaded!",
                                fid, grp)
                            );
                        cur = cur->next;
                    }
                    frame->next = cur->next;
                    cur->next = frame;
                }
            }
        }
        tile = tile->next;
    }
}

/** Ensures the animation loop for [grp] id is loaded into FuzzyMap groups.
    Returns the group.
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
        group = fuzzy_new(struct _AnimationGroup);
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

/** Finds any sprite in layer and loads its animation group. */
static struct _AnimatedLayer * _discover_layer_sprites(FuzzyMap * fmap, tmx_layer *layer, uint lid)
{
    ulong i, j;
    struct _AnimatedLayer * elayer;
    struct _AnimatedSprite *obj, *last;
    tmx_tile * tile;
    tmx_tileset * ts;
    char * grp_s;
    ulong grp;
    tmx_map * map = fmap->map;
    uint _x, _y;

    elayer = _new_map_layer(fmap, layer, lid);
    if (layer->type != L_LAYER)
        /* this layer does not hold any sprite */
        return elayer;

    last = NULL;
    for (i=0; i<map->height; i++) {
		for (j=0; j<map->width; j++) {
            uint gid = _get_gid_in_layer(map, layer, j, i);
            tile = tmx_get_tile(map, gid);
            if (tile) {
                grp_s = _get_tile_property(tile, FUZZY_TILEPROP_ANIMATION_GROUP);
                if (grp_s) {
                    grp = atol(grp_s);

                    fuzzy_iz_error(ts = tmx_get_tileset(map, gid, &_x, &_y), "Tileset cannot be Null here!");
                    _load_animation_group(fmap, grp, ts);

                    /* create a sprite descriptor */
                    obj = _new_sprite(grp);
                    obj->x = j;
                    obj->y = i;

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

    /* create the static layer bitmap */
    if (! (elayer->bitmap = al_create_bitmap(fmap->tot_width, fmap->tot_height)) )
        fuzzy_critical("Failed to create layer bitmap");
    al_set_target_bitmap(elayer->bitmap);
    al_clear_to_color(al_map_rgba(0,0,0,0));
    _render_layer(fmap, layer, elayer);
    al_set_target_backbuffer(al_get_current_display());

    return elayer;
}

/** Loads map animation data structures. */
static void _fuzzy_map_initialize(FuzzyMap * fmap)
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
    elayers = fuzzy_newarr(struct _AnimatedLayer *, nlayers);
    fuzzy_iz_perror(elayers);

    /* fill sprite layers */
    layer = fmap->map->ly_head;
    for (i=0; i<nlayers; i++) {
        elayers[i] = _discover_layer_sprites(fmap, layer, i);
        layer = layer->next;
    }

    /* fill input structure */
    fmap->nlayers = nlayers;
    fmap->elayers = elayers;
}

static void _map_validate(tmx_map * map)
{
    tmx_layer * layer;
    int i;
    
    layer = map->ly_head;
    for (i=0; i<FUZZY_LAYERS_N; i++) {
        if (layer->type != L_LAYER)
            fuzzy_critical("Only L_LAYER layers are supported!");
        
        if (layer == NULL)
            fuzzy_critical(fuzzy_sformat("Missing layer '%s'", LayerNames[i]));
            
        if (strcmp(layer->name, LayerNames[i]) != 0)
            fuzzy_critical(fuzzy_sformat("Expected layer '%s' but got '%s'", LayerNames[i], layer->name));
        layer = layer->next;
    }
}

FuzzyMap * fuzzy_map_load(char * mapfile)
{
    tmx_map * map;
    FuzzyMap * fmap;
    char * fname;

    fname = fuzzy_sformat("%s%s%s", MAP_FOLDER, _DSEP, mapfile);
    fuzzy_iz_tmxerror(map = tmx_load(fname));
    _map_validate(map);

    fmap = fuzzy_new(FuzzyMap);
    fmap->map = map;
    fmap->elayers = NULL;
    fmap->groups = NULL;
    fmap->width = map->width;
    fmap->height = map->height;
    fmap->tile_width = map->tile_width;
    fmap->tile_height = map->tile_height;
    fmap->tot_width = map->width  * map->tile_width;
    fmap->tot_height = map->height * map->tile_height;
    fmap->curtime = 0;

    /* create map bitmap */
	if (! (fmap->bitmap = al_create_bitmap(fmap->tot_width, fmap->tot_height)) )
        fuzzy_critical("Failed to create map bitmap");

    _fuzzy_map_initialize(fmap);
    return fmap;
}

/*--------------------------- CLEAUP METHODS -----------------------------*/

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
    struct _AnimatedSprite *sprite, *d;

    sprite = elayer->sprites;
    while (sprite) {
        d = sprite;
        sprite = sprite->next;
        free(d);
    }

    if (elayer->bitmap)
        al_destroy_bitmap(elayer->bitmap);
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

/** Deallocates a FuzzyMap an related structures */
void fuzzy_map_unload(FuzzyMap * fmap)
{
    _unload_map_layers(fmap);
    tmx_map_free(fmap->map);
    al_destroy_bitmap(fmap->bitmap);
    free(fmap);
}

void fuzzy_sprite_create(FuzzyMap * map, uint lid, ulong grp, ulong x, ulong y)
{
    struct _AnimatedSprite * sprite;
    tmx_tileset * ts;

    sprite = _new_sprite(grp);
    sprite->x = x;
    sprite->y = y;

    /* ensure sprite resources are loaded */
    ts = _get_tileset_for_group(map, grp);
    _load_animation_group(map, grp, ts);

    /* register sprite descriptor */
    _layer_sprite_append(map->elayers[lid], sprite);
}

/* from tmx.c */
static void free_props(tmx_property *p) {
	if (p) {
		free_props(p->next);
		free(p->name);
		free(p->value);
		free(p);
    }
}
static void free_image(tmx_image *i) {
	if (i) {
		free(i->source);
		if (tmx_img_free_func)
			tmx_img_free_func(i->resource_image);
		free(i);
	}
}
static void free_tile(tmx_tile *t) {
    free_props(t->properties);
    free_image(t->image);
    free(t);
}

/* Remove a tile from tmx data structures
 * 
 * pre: tile is present
 * post: tinfo->tile is NULL
 */
static void _remove_tile_at(tmx_map * map, uint lid, ulong x, ulong y, struct _TileInfo * tinfo)
{
    tmx_layer * layer;
    tmx_tile * tile;
    uint id;
    
    layer = _get_tmx_layer(map, lid);
    
    /* clear the tile gid */
    layer->content.gids[(y*map->width)+x] = 0;
        
    if (tinfo->tile != NULL) {
        /* also remove special tile information */
        id = (tinfo->gid & TMX_FLIP_BITS_REMOVAL) - tinfo->ts->firstgid;
            
        tile = tinfo->ts->tiles;
        while(tile->next) {             // pre: tileset is not empty
            if (tile->next->id == id) {
                tile->next = tinfo->tile->next;
                free_tile(tinfo->tile);
                tinfo->tile = NULL;
                break;
            }
            tile = tile->next;
        }
        
        if(tinfo->tile != NULL)
            fuzzy_critical(fuzzy_sformat("Cannot find tile '%d' in tileset!", id));
    }
}

void fuzzy_sprite_destroy(FuzzyMap * fmap, uint lid, ulong x, ulong y)
{
    struct _AnimatedSprite * sprite, * prec;
    struct _AnimatedLayer * elayer = fmap->elayers[lid];
    struct _TileInfo tinfo;
    tmx_map * map = fmap->map;
    
    sprite = _get_sprite_at(elayer, x, y, &prec);
    if (sprite == NULL)
        fuzzy_critical("Target position is empty");
        
    /* do delete from list */
    if (prec == NULL)
        elayer->sprites = sprite->next;
    else
        prec->next = sprite->next;
        
    /* check if it's also loaded into layer list */
    if (_get_tile_at(map, lid, x, y, &tinfo))
        _remove_tile_at(map, lid, x, y, &tinfo);
        
    free(sprite);
}

void fuzzy_sprite_move(FuzzyMap * map, uint lid, ulong ox, ulong oy, ulong nx, ulong ny)
{
    struct _AnimatedSprite * sprite;
    struct _AnimatedLayer * elayer = map->elayers[lid];

    sprite = _get_sprite_at(elayer, nx, ny, NULL);
    if (sprite != NULL)
        fuzzy_critical(fuzzy_sformat("Destination position %d,%d is not empty, contains one in group %d",
          nx, ny, sprite->grp));
    sprite = _get_sprite_at(elayer, ox, oy, NULL);
    if (sprite == NULL)
        fuzzy_critical(fuzzy_sformat("Source position %d,%d does not contain a sprite",
          ox, oy));

    sprite->x = nx;
    sprite->y = ny;
}
