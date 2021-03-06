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

#ifndef __FUZZY_SERVER_H
#define __FUZZY_SERVER_H

#include "fuzzy.h"
#include "list.h"

#define FUZZY_DEFAULT_SERVER_PORT 7557
#define FUZZY_DEFAULT_SERVER_ADDRESS "127.0.0.1"
#define FUZZY_SERVER_BACKLOG 1
#define FUZZY_SERVERKEY_LEN 37
#define FUZZY_NET_ROOM_LEN 64

typedef struct FuzzyClient {
    char ip[16];
    ubyte port;
    int socket;
    bool auth;
    struct FuzzyRoom * room;

    fuzzy_list_link(struct FuzzyClient);
}FuzzyClient;

typedef struct FuzzyRoom {
    ulong id;
    char name[FUZZY_NET_ROOM_LEN];
    FuzzyClient * owner;
    FuzzyClient * clients;
    fuzzy_list_link(struct FuzzyRoom);
}FuzzyRoom;

void fuzzy_server_create(int port, char * keyout);
void fuzzy_server_destroy();
void * fuzzy_server_loop(void * args);
int fuzzy_server_connect(char * addr, int port);
void fuzzy_server_stop(int svsock);

#endif
