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

typedef enum FuzzyFuzzyPlayerType {
    FUZZY_PLAYER_LOCAL,          // a local human player
    FUZZY_PLAYER_REMOTE,         // a remote human player
    FUZZY_PLAYER_CPU,            // a local non human player
}FuzzyFuzzyPlayerType;

typedef struct FuzzyChess {
    ulong x;
    ulong y;
    FuzzyArea * atkarea;
    struct FuzzyPlayer * owner;
    struct FuzzyChess * next;
}FuzzyChess;

typedef struct FuzzyPlayer {
    // meta
    ubyte id;
    FuzzyFuzzyPlayerType type;
    char name[32];

    // data
    double soul_time;
    uint soul_points;
    FuzzyChess * chess_l;
    struct FuzzyPlayer * next;
}FuzzyPlayer;

typedef struct FuzzyGame {
    FuzzyMap * map;
    FuzzyPlayer * players;
    uint _pctr;
}FuzzyGame;

/* Available fooes */
typedef enum FuzzyFooes {
    FUZZY_FOO_LINK
}FuzzyFooes;

/* game related */
FuzzyGame * fuzzy_game_new(char * mapname);
void fuzzy_game_free(FuzzyGame * game);
FuzzyPlayer * fuzzy_game_player_by_id(FuzzyGame * game, ubyte id);

/* player related */
FuzzyPlayer * fuzzy_player_new(FuzzyGame * game, FuzzyFuzzyPlayerType type, char * name);
void fuzzy_player_free(FuzzyPlayer * player);

/* chess related */
FuzzyChess * fuzzy_chess_add(FuzzyGame * game, FuzzyPlayer * pg, FuzzyFooes foo, ulong x, ulong y);
FuzzyChess * fuzzy_chess_at(FuzzyGame * game, FuzzyPlayer * player, ulong x, ulong y);
bool fuzzy_chess_move(FuzzyGame * game, FuzzyChess * chess, ulong nx, ulong ny);
bool fuzzy_chess_attack(FuzzyGame * game, FuzzyChess * chess, ulong tx, ulong ty);
void fuzzy_chess_show_attack_area(FuzzyGame * game, FuzzyChess * chess);
void fuzzy_chess_hide_attack_area(FuzzyGame * game, FuzzyChess * chess);
bool fuzzy_chess_inside_target_area(FuzzyGame * game, FuzzyChess * chess, ulong tx, ulong ty);
bool fuzzy_chess_local_attack(FuzzyGame * game, FuzzyPlayer * player, FuzzyChess * chess, ulong tx, ulong ty);
bool fuzzy_chess_local_move(FuzzyGame * game, FuzzyPlayer * player, FuzzyChess * chess, ulong nx, ulong ny);
