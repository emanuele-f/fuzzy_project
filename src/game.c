#include "game.h"
#include "gids.h"
#include "tiles.h"

FuzzyPlayer * fuzzy_game_player_by_id(FuzzyGame * game, ubyte id)
{
    FuzzyPlayer * plist = game->players;

    while(plist) {
        if (plist->id == id)
            return plist;
        plist = plist->next;
    }
    return NULL;
}

FuzzyGame * fuzzy_game_new(char * mapname)
{
    FuzzyGame * game;

    fuzzy_areadb_init();
    fuzzy_map_setup();

    game = fuzzy_new(FuzzyGame);
    game->players = NULL;
    game->_pctr = 0;
    game->map = fuzzy_map_load(mapname);
    fuzzy_map_update(game->map, 0);

    return game;
}

void fuzzy_game_free(FuzzyGame * game)
{
    // TODO implement
}

FuzzyChess * fuzzy_chess_add(FuzzyGame * game, FuzzyPlayer * pg, FuzzyFooes foo, ulong x, ulong y)
{
    char * grp = GID_LINK;
    FuzzyArea * atkarea;
    FuzzyChess * chess, * l;

    // Select foo asset
    switch (foo) {
    case FUZZY_FOO_LINK:
        grp = GID_LINK;
        atkarea = &FuzzyMeleeMan;
        break;

    }

    chess = fuzzy_new(FuzzyChess);
    chess->x = x;
    chess->y = y;
    chess->atkarea = atkarea;
    chess->owner = pg;
    chess->next = NULL;

    fuzzy_sprite_create(game->map, FUZZY_LAYER_SPRITES, grp, x, y);

    if (! pg->chess_l) {
        pg->chess_l = chess;
    } else {
        l = pg->chess_l;
        while (l->next)
            l = l->next;
        l->next = chess;
    }

    return chess;
}

FuzzyChess * fuzzy_chess_at(FuzzyGame * game, FuzzyPlayer * player, ulong x, ulong y)
{
    FuzzyChess * chess;

    chess = player->chess_l;
    while(chess) {
        if (chess->x == x && chess->y==y)
            return chess;
        chess = chess->next;
    }

    return NULL;
}

static bool _pay_sp_requirement(FuzzyPlayer * player, uint sp_req)
{
    if (player->soul_points < sp_req) {
        fuzzy_debug("Not enough SP!");
        return false;
    }

    player->soul_points -= sp_req;
    return true;
}

bool fuzzy_chess_move(FuzzyGame * game, FuzzyChess * chess, ulong nx, ulong ny)
{
    if (fuzzy_map_spy(game->map, FUZZY_LAYER_SPRITES, nx, ny) != FUZZY_CELL_EMPTY)
        // collision
        return false;

    fuzzy_sprite_move(game->map, FUZZY_LAYER_SPRITES, chess->x, chess->y, nx, ny);
    chess->x = nx;
    chess->y = ny;
    return true;
}

bool fuzzy_chess_attack(FuzzyGame * game, FuzzyChess * chess, ulong tx, ulong ty)
{
    FuzzyPlayer * player;

    player = game->players;
    while (player) {
        if (player->id != chess->owner->id) {
            if (fuzzy_chess_at(game, player, tx, ty)) {
                fuzzy_sprite_destroy(game->map, FUZZY_LAYER_SPRITES, tx, ty);
                return true;
            }
        }

        player = player->next;
    }

    return false;
}

static void _fuzzy_chess_free(FuzzyGame * game, FuzzyChess * chess)
{
    const ulong x = chess->x;
    const ulong y = chess->y;

    fuzzy_sprite_destroy(game->map, FUZZY_LAYER_SPRITES, x, y);
    free(chess);
    // TODO check and implement
}

void fuzzy_chess_show_attack_area(FuzzyGame * game, FuzzyChess * chess)
{
    FuzzyAreaIterator iterator;
    FuzzyPoint limit, pt;

    limit.x = game->map->width;
    limit.y = game->map->height;
    pt.x = chess->x;
    pt.y = chess->y;

    fuzzy_area_iter_begin(chess->atkarea, &iterator, &pt, &limit);
    while(fuzzy_area_iter(chess->atkarea, &iterator))
        if (iterator.value)
            fuzzy_sprite_create(game->map, FUZZY_LAYER_BELOW, GID_ATTACK_AREA, iterator.pos.x, iterator.pos.y);
}

void fuzzy_chess_hide_attack_area(FuzzyGame * game, FuzzyChess * chess)
{
    FuzzyAreaIterator iterator;
    FuzzyPoint limit, pt;

    limit.x = game->map->width;
    limit.y = game->map->height;
    pt.x = chess->x;
    pt.y = chess->y;

    fuzzy_area_iter_begin(chess->atkarea, &iterator, &pt, &limit);
    while(fuzzy_area_iter(chess->atkarea, &iterator))
        if (iterator.value)
            fuzzy_sprite_destroy(game->map, FUZZY_LAYER_BELOW, iterator.pos.x, iterator.pos.y);
}

bool fuzzy_chess_inside_target_area(FuzzyGame * game, FuzzyChess * chess, ulong tx, ulong ty)
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

/* player related */
FuzzyPlayer * fuzzy_player_new(FuzzyGame * game, FuzzyFuzzyPlayerType type, char * name)
{
    FuzzyPlayer * player, * p;
    FuzzyPlayer ** plist = &game->players;

    player = fuzzy_new(FuzzyPlayer);
    player->id = (game->_pctr)++;
    player->chess_l = NULL;
    player->type = type;
    player->soul_time = 0;
    player->soul_points = SOUL_POINTS_INITIAL;
    player->next = NULL;
    strncpy(player->name, name, sizeof(player->name));

    if (! *plist)
        *plist = player;
    else {
        p = *plist;
        while(p->next)
            p = p->next;
        p->next = player;
    }

    return player;
}

void fuzzy_player_free(FuzzyPlayer * player)
{
    // TODO implement me
}

/* Local player actions */

bool fuzzy_chess_local_attack(FuzzyGame * game, FuzzyPlayer * player, FuzzyChess * chess, ulong tx, ulong ty)
{
    if (! _pay_sp_requirement(player, SP_ATTACK))
        return false;
    return fuzzy_chess_attack(game, chess, tx, ty);
}

bool fuzzy_chess_local_move(FuzzyGame * game, FuzzyPlayer * player, FuzzyChess * chess, ulong nx, ulong ny)
{
    if (! _pay_sp_requirement(player, SP_MOVE))
        // not enough APs
        return false;

    return fuzzy_chess_move(game, chess, nx, ny);
}
