#include "game.h"
#include "gids.h"

Chess * fuzzy_chess_add(Player * pg, ulong x, ulong y, FuzzyArea * atkarea)
{
    char * grp = GID_LINK;
    Chess * chess = fuzzy_new(Chess);
    chess->x = x;
    chess->y = y;
    chess->target = false;
    chess->atkarea = atkarea;
    chess->owner = pg;

    fuzzy_sprite_create(chess->owner->map, FUZZY_LAYER_SPRITES, grp, x, y);

    return chess;
}

static bool _pay_sp_requirement(LocalPlayer * player, uint sp_req)
{
    if (player->soul_points < sp_req) {
        fuzzy_debug("Not enough SP!");
        return false;
    }

    player->soul_points -= sp_req;
    return true;
}

bool fuzzy_chess_move(Chess * chess, ulong nx, ulong ny)
{
    if (fuzzy_map_spy(chess->owner->map, FUZZY_LAYER_SPRITES, nx, ny) != FUZZY_CELL_EMPTY)
        // collision
        return false;

    if (chess->target)
        fuzzy_sprite_move(chess->owner->map, FUZZY_LAYER_BELOW, chess->x, chess->y, nx, ny);
    fuzzy_sprite_move(chess->owner->map, FUZZY_LAYER_SPRITES, chess->x, chess->y, nx, ny);
    chess->x = nx;
    chess->y = ny;
    return true;
}

/* pre: target is in attack area and there is a target */
void fuzzy_chess_attack(Chess * chess, ulong tx, ulong ty)
{
    fuzzy_sprite_destroy(chess->owner->map, FUZZY_LAYER_SPRITES, tx, ty);
}

static void _fuzzy_chess_free(Chess * chess)
{
    const ulong x = chess->x;
    const ulong y = chess->y;

    if (chess->target)
        fuzzy_sprite_destroy(chess->owner->map, FUZZY_LAYER_BELOW, x, y);
    fuzzy_sprite_destroy(chess->owner->map, FUZZY_LAYER_SPRITES, x, y);
    free(chess);
}

void fuzzy_chess_show_attack_area(Chess * chess)
{
    FuzzyAreaIterator iterator;
    FuzzyPoint limit, pt;

    limit.x = chess->owner->map->width;
    limit.y = chess->owner->map->height;
    pt.x = chess->x;
    pt.y = chess->y;

    fuzzy_area_iter_begin(chess->atkarea, &iterator, &pt, &limit);
    while(fuzzy_area_iter(chess->atkarea, &iterator))
        if (iterator.value)
            fuzzy_sprite_create(chess->owner->map, FUZZY_LAYER_BELOW, GID_ATTACK_AREA, iterator.pos.x, iterator.pos.y);
}

void fuzzy_chess_hide_attack_area(Chess * chess)
{
    FuzzyAreaIterator iterator;
    FuzzyPoint limit, pt;

    limit.x = chess->owner->map->width;
    limit.y = chess->owner->map->height;
    pt.x = chess->x;
    pt.y = chess->y;

    fuzzy_area_iter_begin(chess->atkarea, &iterator, &pt, &limit);
    while(fuzzy_area_iter(chess->atkarea, &iterator))
        if (iterator.value)
            fuzzy_sprite_destroy(chess->owner->map, FUZZY_LAYER_BELOW, iterator.pos.x, iterator.pos.y);
}

bool fuzzy_chess_inside_target_area(Chess * chess, ulong tx, ulong ty)
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

static uint _PlayerCtr = 0;

/* player related */
Player * fuzzy_player_new(Player ** plist, char * name)
{
    Player * player, * p;

    player = fuzzy_new(Player);
    player->id = _PlayerCtr++;
    player->chess_l = NULL;
    player->map = NULL;
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

void fuzzy_player_free()
{
}

LocalPlayer * fuzzy_localplayer_new(LocalPlayer ** lplist, Player ** plist, char * name)
{
    LocalPlayer * lp;

    lp = fuzzy_new(LocalPlayer);
    lp->player = fuzzy_player_new(plist, name);
    lp->soul_time = 0;
    lp->soul_points = SOUL_POINTS_INITIAL;
    return lp;
}

void fuzzy_localplayer_free()
{
}

/* Local player actions */

bool fuzzy_chess_local_attack(LocalPlayer * player, Chess * chess, ulong tx, ulong ty)
{
    if (! _pay_sp_requirement(player, SP_ATTACK))
        return false;
    fuzzy_chess_attack(chess, tx, ty);
    return true;
}

bool fuzzy_chess_local_move(LocalPlayer * player, Chess * chess, ulong nx, ulong ny)
{
    if (! _pay_sp_requirement(player, SP_MOVE))
        // not enough APs
        return false;

    return fuzzy_chess_move(chess, nx, ny);
}
