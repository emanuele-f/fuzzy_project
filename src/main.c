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

#include <stdio.h>
#include <pthread.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include "fuzzy.h"
#include "server.h"
#include "network.h"
#include "protocol.h"
#include "tiles.h"
#include "area.h"
#include "gids.h"

#define FPS 30
#define LEFT_BUTTON 1
#define RIGHT_BUTTON 2
#define GRID_ON
#define WINDOW_TITLE "FUZZY Tales!"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

/* Seconds before soul points boost */
#define SOUL_TIME_INTERVAL 3.0
#define SOUL_POINTS_BOOST 8
#define SOUL_POINTS_INITIAL 20

/* Action requirements */
#define SP_MOVE 1
#define SP_ATTACK 5

typedef struct Chess {
    ulong x;
    ulong y;
    bool target;
    FuzzyArea * atkarea;
}Chess;

FuzzyMap * map;
uint player_sp = SOUL_POINTS_INITIAL;

static bool _pay_sp_requirement(uint sp_req)
{
    if (player_sp < sp_req) {
        fuzzy_debug("Not enough SP!");
        return false;
    }

    player_sp -= sp_req;
    return true;
}

static bool _chess_move(Chess * chess, ulong nx, ulong ny)
{
    if (! _pay_sp_requirement(SP_MOVE))
        // not enough APs
        return false;

    if (fuzzy_map_spy(map, FUZZY_LAYER_SPRITES, nx, ny) != FUZZY_CELL_EMPTY)
        // collision
        return false;

    if (chess->target)
        fuzzy_sprite_move(map, FUZZY_LAYER_BELOW, chess->x, chess->y, nx, ny);
    fuzzy_sprite_move(map, FUZZY_LAYER_SPRITES, chess->x, chess->y, nx, ny);
    chess->x = nx;
    chess->y = ny;
    return true;
}

static Chess * _chess_new(ulong x, ulong y, FuzzyArea * atkarea)
{
    char * grp = GID_LINK;
    Chess * chess = fuzzy_new(Chess);
    chess->x = x;
    chess->y = y;
    chess->target = false;
    chess->atkarea = atkarea;

    fuzzy_sprite_create(map, FUZZY_LAYER_SPRITES, grp, x, y);

    return chess;
}

static void _chess_free(Chess * chess)
{
    const ulong x = chess->x;
    const ulong y = chess->y;

    if (chess->target)
        fuzzy_sprite_destroy(map, FUZZY_LAYER_BELOW, x, y);
    fuzzy_sprite_destroy(map, FUZZY_LAYER_SPRITES, x, y);
    free(chess);
}

static void _chess_show_attack_area(Chess * chess)
{
    FuzzyAreaIterator iterator;
    FuzzyPoint limit, pt;
    bool val;

    limit.x = map->width;
    limit.y = map->height;
    pt.x = chess->x;
    pt.y = chess->y;

    fuzzy_area_iter_begin(chess->atkarea, &iterator, &pt, &limit);
    while(fuzzy_area_iter(chess->atkarea, &iterator))
        if (iterator.value)
            fuzzy_sprite_create(map, FUZZY_LAYER_BELOW, GID_ATTACK_AREA, iterator.pos.x, iterator.pos.y);
}

static void _chess_hide_attack_area(Chess * chess)
{
    FuzzyAreaIterator iterator;
    FuzzyPoint limit, pt;

    limit.x = map->width;
    limit.y = map->height;
    pt.x = chess->x;
    pt.y = chess->y;

    fuzzy_area_iter_begin(chess->atkarea, &iterator, &pt, &limit);
    while(fuzzy_area_iter(chess->atkarea, &iterator))
        if (iterator.value)
            fuzzy_sprite_destroy(map, FUZZY_LAYER_BELOW, iterator.pos.x, iterator.pos.y);
}

static bool _is_inside_target_area(Chess * chess, ulong tx, ulong ty)
{
    FuzzyPoint pivot, pt;

    if (tx == chess->x && ty == chess->y)
        return false;

    pivot.x = chess->x;
    pivot.y = chess->y;
    pt.x = tx;
    pt.y = ty;

    if (fuzzy_area_inside(chess->atkarea, &pivot, &pt))
        return true;
    return false;
}

/* pre: target is in attack area and there is a target */
static bool _do_attack(Chess * chess, ulong tx, ulong ty)
{
    if (! _pay_sp_requirement(SP_ATTACK))
        return false;
    fuzzy_sprite_destroy(map, FUZZY_LAYER_SPRITES, tx, ty);
    return true;
}

#define _attack_area_on() do {\
    if (!showing_area && chess->target) {\
        _chess_show_attack_area(chess);\
        showing_area = true;\
    }\
}while(0)

#define _attack_area_off() do{\
    if(showing_area) {\
        _chess_hide_attack_area(chess);\
        showing_area = false;\
    }\
}while(0)

int main(int argc, char *argv[])
{
	ALLEGRO_DISPLAY *display = NULL;
	ALLEGRO_EVENT_QUEUE *evqueue = NULL;
	ALLEGRO_TIMER *timer = NULL;
	ALLEGRO_KEYBOARD_STATE keyboard_state;
    ALLEGRO_EVENT event;
    ALLEGRO_FONT *font;
    ALLEGRO_BITMAP *clock_hand, *clock_quadrant;
    float clock_angle = 0;

	bool running = true;
	bool redraw = true;

	int map_x = 13*16, map_y = 5*16;
	int screen_width = WINDOW_WIDTH;
	int screen_height = WINDOW_HEIGHT;
    double curtime, soul_time;

	/* Initialization */
    fuzzy_iz_error(al_init(), "Failed to initialize allegro");
    fuzzy_load_addon("image", al_init_image_addon());
    fuzzy_load_addon("primitives", al_init_primitives_addon());
    fuzzy_load_addon("keyboard", al_install_keyboard());
    fuzzy_load_addon("mouse", al_install_mouse());
    al_init_font_addon();

	fuzzy_iz_error(timer = al_create_timer(1.0 / FPS), "Cannot create FPS timer");
    fuzzy_iz_error(evqueue = al_create_event_queue(), "Cannot create event queue");
	fuzzy_iz_error(display = al_create_display(screen_width, screen_height),
      "Cannot initialize display");
    al_set_window_title(display, WINDOW_TITLE);
    fuzzy_iz_error(font = al_load_font(fuzzy_res(FONT_FOLDER, "fixed_font.tga"), 0, 0), "Cannot load 'fixed_font.tga'");
    clock_hand = al_load_bitmap(fuzzy_res(PICTURE_FOLDER, "clock_hand.png"));
    fuzzy_iz_error(clock_hand, "Cannot load clock handle");
    clock_quadrant = al_load_bitmap(fuzzy_res(PICTURE_FOLDER, "clock_quadrant.png"));
    fuzzy_iz_error(clock_hand, "Cannot load clock quadrant");

	/* Queue setup */
	al_register_event_source(evqueue, al_get_display_event_source(display));
	al_register_event_source(evqueue, al_get_timer_event_source(timer));
	al_register_event_source(evqueue, al_get_keyboard_event_source());
    al_register_event_source(evqueue, al_get_mouse_event_source());

    /* Map load */
    fuzzy_areadb_init();
    fuzzy_map_setup();
    map = fuzzy_map_load("level000.tmx");
    fuzzy_map_update(map, 0);
    Chess * chess = _chess_new(34, 30, &FuzzyMeleeMan);
    bool showing_area = false;

	al_clear_to_color(al_map_rgb(0, 0, 0));
    al_draw_bitmap(map->bitmap, -map_x, -map_y, 0);
	al_flip_display();

#if DEBUG
	ALLEGRO_BITMAP *icon;
    int fps, fps_accum;
    double fps_time;

    icon = al_load_bitmap(fuzzy_res(PICTURE_FOLDER, "icon.tga"));
    if (icon)
        al_set_display_icon(display, icon);
    fps_accum = fps_time = 0;
    fps = FPS;
#endif

    /* Server setup */
    pthread_t srv_thread;
    int svsock;
    char srvkey[FUZZY_SERVERKEY_LEN];
    FuzzyMessage * sendmsg = fuzzy_message_new();

    fuzzy_server_create(FUZZY_DEFAULT_SERVER_PORT, srvkey);
    fuzzy_nz_rerror(pthread_create(&srv_thread, NULL, fuzzy_server_loop, NULL));
    svsock = fuzzy_server_connect(FUZZY_DEFAULT_SERVER_ADDRESS, FUZZY_DEFAULT_SERVER_PORT);

	/* MAIN loop */
    soul_time = al_get_time();
    al_start_timer(timer);
	while (running) {
        /* wait until an event happens */
		al_wait_for_event(evqueue, &event);

        switch (event.type) {
        case ALLEGRO_EVENT_TIMER:
            /* check soul ticks */
            curtime = al_get_time();
            while (curtime - soul_time >= SOUL_TIME_INTERVAL) {
                //~ fuzzy_debug("Soul tick!");
                soul_time += SOUL_TIME_INTERVAL;
                player_sp += SOUL_POINTS_BOOST;
            }
            clock_angle = (curtime - soul_time)/SOUL_TIME_INTERVAL * FUZZY_2PI;

            al_get_keyboard_state(&keyboard_state);
            if (al_key_down(&keyboard_state, ALLEGRO_KEY_RIGHT)) {
                map_x += 5;
                if (map_x > (map->tot_width - screen_width))
                    map_x = map->tot_width - screen_width;
            }
            else if (al_key_down(&keyboard_state, ALLEGRO_KEY_LEFT)) {
                map_x -= 5;
                if (map_x < 0)
                    map_x = 0;
            }
            else if (al_key_down(&keyboard_state, ALLEGRO_KEY_UP)) {
                map_y -= 5;
                if (map_y < 0)
                    map_y = 0;
            }
            else if (al_key_down(&keyboard_state, ALLEGRO_KEY_DOWN)) {
                map_y += 5;
                if (map_y > (map->tot_height - screen_height))
                    map_y = map->tot_height - screen_height;
            }

            redraw = true;
            break;
        case ALLEGRO_EVENT_KEY_DOWN:
            if(! chess->target)
                break;

            if (showing_area)
                /* abort attack */
                _attack_area_off();

            switch(event.keyboard.keycode) {
                case ALLEGRO_KEY_W:
                    _chess_move(chess, chess->x, chess->y-1);
                    break;
                case ALLEGRO_KEY_A:
                    _chess_move(chess, chess->x-1, chess->y);
                    break;
                case ALLEGRO_KEY_S:
                    _chess_move(chess, chess->x, chess->y+1);
                    break;
                case ALLEGRO_KEY_D:
                    _chess_move(chess, chess->x+1, chess->y);
                    break;

                case ALLEGRO_KEY_K:
                    _attack_area_on();
                    break;

                case ALLEGRO_KEY_SPACE:
                    /* switch attack type */
                    if (chess->atkarea == &FuzzyMeleeMan)
                        chess->atkarea = &FuzzyRangedMan;
                    else
                        chess->atkarea = &FuzzyMeleeMan;
                    break;
            }
            break;
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            running = false;
            break;
        case ALLEGRO_EVENT_KEY_UP:
            break;
        case ALLEGRO_EVENT_KEY_CHAR:
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            if(event.mouse.button == RIGHT_BUTTON) {
                _attack_area_on();
            } else if(event.mouse.button == LEFT_BUTTON) {
                /* world to tile coords */
                int tx = (event.mouse.x+map_x) / map->tile_width;
                int ty = (event.mouse.y+map_y) / map->tile_height;
#ifdef DEBUG
                printf("SELECT %d %d\n", tx, ty);
#endif
                if(showing_area && _is_inside_target_area(chess, tx, ty)) {
                    /* select attack target */
                    if (fuzzy_map_spy(map, FUZZY_LAYER_SPRITES, tx, ty) == FUZZY_CELL_SPRITE) {
                        if (_do_attack(chess, tx, ty))
                            _attack_area_off();
                    }
                } else {
                    /* select chess */
                    if (chess->x == tx && chess->y == ty) {
                        if (! chess->target) {
                            fuzzy_sprite_create(map, FUZZY_LAYER_BELOW, GID_TARGET, tx, ty);
                            chess->target = true;
                        }
                    } else if (chess->target) {
                        if (showing_area) {
                            /* abort attack */
                            _attack_area_off();
                        } else {
                            /* remove target */
                            fuzzy_sprite_destroy(map, FUZZY_LAYER_BELOW, chess->x, chess->y);
                            chess->target = false;
                        }
                    }
                }
            }
            break;
        default:
#ifdef DEBUG
            //~ fprintf(stderr, "Unknown event received: %d\n", event.type);
#endif
            break;
        }

        if (redraw && al_is_event_queue_empty(evqueue)) {
            curtime = al_get_time();
            fuzzy_map_update(map, curtime);

            // Clear the screen
            al_clear_to_color(al_map_rgb(0, 0, 0));
            al_draw_bitmap(map->bitmap, -map_x, -map_y, 0);

#ifdef GRID_ON
            /* Draw the grid */
            int tw = map->tile_width;
            int ty = map->tile_height;
            int x, y;
            for (x=(tw-map_x)%tw; x<screen_width; x+=tw)
                al_draw_line(x, 0, x, screen_height, al_map_rgba(7,7,7,100), 1);
            for (y=(ty-map_y)%ty; y<screen_height; y+=ty)
                al_draw_line(0, y, screen_width, y, al_map_rgba(7,7,7,100), 1);
#endif
#if DEBUG
            al_draw_filled_rounded_rectangle(screen_width-100, 4, screen_width, 30,
                8, 8, al_map_rgba(0, 0, 0, 200));
            al_draw_textf(font, al_map_rgb(255, 255, 255),
                screen_width-50, 8, ALLEGRO_ALIGN_CENTRE, "FPS: %d", fps);
#endif
            /* draw SP count */
            al_draw_filled_rounded_rectangle(4, screen_height-170, 175, screen_height-4,
                8, 8, al_map_rgba(0, 0, 0, 200));
            al_draw_textf(font, al_map_rgb(255, 70, 70),
                15, screen_height-163, ALLEGRO_ALIGN_LEFT, "SP: %d", player_sp);

            /* draw Soul Clock */
            al_draw_scaled_bitmap(clock_quadrant, 0, 0, 301, 301, 20, screen_height-80-139/2, 139, 139, 0);
            al_draw_scaled_rotated_bitmap(clock_hand, 160, 607, 90, screen_height-80, 0.11, 0.11, clock_angle, 0);

            al_flip_display();
#if DEBUG
            fps_accum++;
            if (curtime - fps_time >= 1) {
                fps = fps_accum;
                fps_accum = 0;
                fps_time = curtime;
            }
#endif
            redraw = false;
        }
    }

	/* Cleanup */
    void * retval;
    fuzzy_protocol_server_shutdown(svsock, sendmsg, srvkey);
    fuzzy_nz_rerror(pthread_join(srv_thread, &retval));
    fuzzy_server_destroy();
    fuzzy_message_del(sendmsg);

    _chess_free(chess);
	fuzzy_map_unload(map);
    al_destroy_event_queue(evqueue);
	al_destroy_display(display);
    al_destroy_timer(timer);
	return 0;
}
