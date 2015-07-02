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

#include "fuzzy.h"
#include "area.h"
#include "tiles.h"

/* Seconds before soul points boost */
#define SOUL_TIME_INTERVAL 3.0
#define SOUL_POINTS_BOOST 8
#define SOUL_POINTS_INITIAL 20
#define RAY_TIME_INTERVAL 1.0

/* Action requirements */
#define SP_MOVE 1
#define SP_ATTACK 5

typedef struct Chess {
    ulong x;
    ulong y;
    FuzzyArea * atkarea;
    struct Player * owner;
    struct Chess * next;
}Chess;

typedef struct Player {
    ubyte id;
    char name[32];
    Chess * chess_l;
    struct FuzzyMap * map;
    struct Player * next;
}Player;

typedef struct LocalPlayer {
    Player * player;
    double soul_time;
    uint soul_points;
}LocalPlayer;

/* player related */
Player * fuzzy_player_new(Player ** plist, char * name);
void fuzzy_player_free();
LocalPlayer * fuzzy_localplayer_new(LocalPlayer ** lplist, Player ** plist, char * name);
void fuzzy_localplayer_free();

/* chess related */
Chess * fuzzy_chess_add(Player * pg, ulong x, ulong y, FuzzyArea * atkarea);
Chess * fuzzy_chess_at(Player * player, ulong x, ulong y);
bool fuzzy_chess_move(Chess * chess, ulong nx, ulong ny);
void fuzzy_chess_attack(Chess * chess, ulong tx, ulong ty);
void fuzzy_chess_show_attack_area(Chess * chess);
void fuzzy_chess_hide_attack_area(Chess * chess);
bool fuzzy_chess_inside_target_area(Chess * chess, ulong tx, ulong ty);
bool fuzzy_chess_local_attack(LocalPlayer * player, Chess * chess, ulong tx, ulong ty);
bool fuzzy_chess_local_move(LocalPlayer * player, Chess * chess, ulong nx, ulong ny);
