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
 * A fake allegro function implementation, used for tests
 *
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

#define BOGUS_PTR (void *)(1000)

AL_FUNC(ALLEGRO_COLOR, al_map_rgb, (unsigned char r, unsigned char g, unsigned char b)) {
    ALLEGRO_COLOR a;
    return a;
}

AL_FUNC(ALLEGRO_COLOR, al_map_rgb_f, (float r, float g, float b)) {
    ALLEGRO_COLOR a;
    return a;
}

AL_FUNC(void, al_unmap_rgb_f, (ALLEGRO_COLOR color, float *r, float *g, float *b)) {}

AL_FUNC(ALLEGRO_COLOR, al_map_rgba, (unsigned char r, unsigned char g, unsigned char b, unsigned char a)) {
    ALLEGRO_COLOR c;
    return c;
}

AL_FUNC(ALLEGRO_COLOR, al_map_rgba_f, (float r, float g, float b, float a)) {
    ALLEGRO_COLOR c;
    return c;
}

AL_FUNC(ALLEGRO_BITMAP*, al_create_bitmap, (int w, int h)) { return BOGUS_PTR; }
AL_FUNC(void, al_destroy_bitmap, (ALLEGRO_BITMAP *bitmap)) {}
AL_FUNC(void, al_clear_to_color, (ALLEGRO_COLOR color)) {}
AL_FUNC(void, al_draw_bitmap, (ALLEGRO_BITMAP *bitmap, float dx, float dy, int flags)) {}
AL_FUNC(void, al_draw_tinted_bitmap, (ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint, float dx, float dy, int flags)) {}
AL_FUNC(void, al_draw_tinted_bitmap_region, (ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint, float sx, float sy, float sw, float sh, float dx, float dy, int flags)) {}
AL_FUNC(void, al_set_new_bitmap_format, (int format)) {}
AL_FUNC(ALLEGRO_DISPLAY*, al_get_current_display, (void)) {return BOGUS_PTR;}
AL_FUNC(void,            al_set_target_backbuffer, (ALLEGRO_DISPLAY *display)) {}
AL_FUNC(ALLEGRO_PATH*, al_create_path, (const char *str)) {return BOGUS_PTR;}
AL_FUNC(void, al_destroy_path, (ALLEGRO_PATH *path)) {}
AL_FUNC(const char*, al_path_cstr, (const ALLEGRO_PATH *path, char delim)) {return BOGUS_PTR;}
ALLEGRO_PRIM_FUNC(void, al_draw_rectangle, (float x1, float y1, float x2, float y2, ALLEGRO_COLOR color, float thickness)) {}
ALLEGRO_PRIM_FUNC(void, al_draw_line, (float x1, float y1, float x2, float y2, ALLEGRO_COLOR color, float thickness)) {}
ALLEGRO_PRIM_FUNC(void, al_draw_ellipse, (float cx, float cy, float rx, float ry, ALLEGRO_COLOR color, float thickness)) {}
AL_FUNC(ALLEGRO_BITMAP *, al_load_bitmap, (const char *filename)) {return BOGUS_PTR;}
AL_FUNC(void,            al_set_target_bitmap, (ALLEGRO_BITMAP *bitmap)) {}
