#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include "allegro5/allegro_tiled.h"
#include "global.h"

#define FPS 30
#define LEFT_BUTTON 1
#define GRID_ON
#define WINDOW_TITLE "FUZZY Tales!"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

int main(int argc, char *argv[])
{
	ALLEGRO_DISPLAY *display = NULL;
	ALLEGRO_EVENT_QUEUE *evqueue = NULL;
	ALLEGRO_TIMER *timer = NULL;
	ALLEGRO_KEYBOARD_STATE keyboard_state;
	ALLEGRO_MAP *map;
    ALLEGRO_EVENT event;

	bool running = true;
	bool redraw = true;
	bool reload = false;

	int map_x = 0, map_y = 0;
	int screen_width = WINDOW_WIDTH;
	int screen_height = WINDOW_HEIGHT;
    int map_total_width, map_total_height;

	/* Initialization */
    fuzzy_nz_error(al_init(), "Failed to initialize allegro");
    fuzzy_load_addon("image", al_init_image_addon());
    fuzzy_load_addon("primitives", al_init_primitives_addon());
    fuzzy_load_addon("keyboard", al_install_keyboard());
    fuzzy_load_addon("mouse", al_install_mouse());
    al_init_font_addon();

	fuzzy_nz_error(timer = al_create_timer(1.0 / FPS), "Cannot create FPS timer");
    fuzzy_nz_error(evqueue = al_create_event_queue(), "Cannot create event queue");
	fuzzy_nz_error(display = al_create_display(screen_width, screen_height),
      "Cannot initialize display");
    al_set_window_title(display, WINDOW_TITLE);

	/* Queue setup */
	al_register_event_source(evqueue, al_get_display_event_source(display));
	al_register_event_source(evqueue, al_get_timer_event_source(timer));
	al_register_event_source(evqueue, al_get_keyboard_event_source());
    al_register_event_source(evqueue, al_get_mouse_event_source());

    /* Map load */
	map = al_open_map(MAP_FOLDER, "level1.tmx");
	map_total_width = al_get_map_width(map) * al_get_tile_width(map);
	map_total_height = al_get_map_height(map) * al_get_tile_height(map);
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_draw_map_region(map, map_x, map_y, screen_width, screen_height, 0, 0, 0);
	al_flip_display();

#if DEBUG
	ALLEGRO_BITMAP *icon;
    ALLEGRO_FONT *font;
    int fps, fps_accum;
    double fps_time, t;

    font = al_load_font(fuzzy_res(FONT_FOLDER, "fixed_font.tga"), 0, 0);
    icon = al_load_bitmap(fuzzy_res(PICTURE_FOLDER, "icon.tga"));
    if (icon)
        al_set_display_icon(display, icon);
    fps_accum = fps_time = t = 0;
    fps = FPS;
#endif

	/* MAIN loop */
    al_start_timer(timer);
	while (running) {
        /* wait until an event happens */
		al_wait_for_event(evqueue, &event);

        switch (event.type) {
        case ALLEGRO_EVENT_TIMER:
            // is an arrow key being held?
            al_get_keyboard_state(&keyboard_state);
            if (al_key_down(&keyboard_state, ALLEGRO_KEY_RIGHT)) {
                map_x += 5;
                if (map_x > (map_total_width - screen_width))
                    map_x = map_total_width - screen_width;
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
                if (map_y > (map_total_height - screen_height))
                    map_y = map_total_height - screen_height;
            }

            redraw = true;
            break;
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            running = false;
            break;
        case ALLEGRO_EVENT_KEY_DOWN:
            break;
        case ALLEGRO_EVENT_KEY_UP:
            break;
        case ALLEGRO_EVENT_KEY_CHAR:
            if (event.keyboard.keycode == ALLEGRO_KEY_SPACE)
                reload = true;
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            if(event.mouse.button == LEFT_BUTTON) {
                /* world to tile coords */
                int tx = (event.mouse.x+map_x) / al_get_tile_width(map);
                int ty = (event.mouse.y+map_y) / al_get_tile_height(map);
#ifdef DEBUG
                printf("SELECT %d %d\n", tx, ty);
#endif
            }
            break;
        default:
#ifdef DEBUG
            //~ fprintf(stderr, "Unknown event received: %d\n", event.type);
#endif
            break;
        }

        if (redraw && al_is_event_queue_empty(evqueue)) {
#ifdef DEBUG
            t = al_get_time();
#endif

            // Clear the screen
            al_clear_to_color(al_map_rgb(0, 0, 0));

            // If we need to reload, do it
            if (reload) {
                int x = map_x;
                int y = map_y;
                al_free_map(map);
                map = al_open_map(MAP_FOLDER, "level1.tmx");
                map_x = x;
                map_y = y;
                reload = false;
            }
            al_draw_map_region(map, map_x, map_y, screen_width, screen_height, 0, 0, 0);

#ifdef GRID_ON
            /* Draw the grid */
            int tw = al_get_tile_width(map);
            int ty = al_get_tile_height(map);
            int x, y;
            for (x=(tw-map_x)%tw; x<screen_width; x+=tw)
                al_draw_line(x, 0, x, screen_height, al_map_rgba(7,7,7,100), 1);
            for (y=(ty-map_y)%ty; y<screen_height; y+=ty)
                al_draw_line(0, y, screen_width, y, al_map_rgba(7,7,7,100), 1);
#endif
#if DEBUG
            if (font) {
                al_draw_filled_rounded_rectangle(4, 4, 100, 30,
                    8, 8, al_map_rgba(0, 0, 0, 200));
                al_draw_textf(font, al_map_rgb(255, 255, 255),
                    54, 8, ALLEGRO_ALIGN_CENTRE, "FPS: %d", fps);
            }
#endif
            al_flip_display();
#if DEBUG
            fps_accum++;
            if (t - fps_time >= 1) {
                fps = fps_accum;
                fps_accum = 0;
                fps_time = t;
            }
#endif
            redraw = false;
        }
    }

	/* Cleanup */
	al_free_map(map);
    al_destroy_event_queue(evqueue);
	al_destroy_display(display);
    al_destroy_timer(timer);
	return 0;
}
