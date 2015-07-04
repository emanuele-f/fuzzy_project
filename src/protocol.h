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

#ifndef __FUZZY_PROTOCOL_H
#define __FUZZY_PROTOCOL_H

#include "server.h"

/* Return codes */
#define FUZZY_NETERROR_CHARS 256
typedef enum FUZZY_NETCODES {
    FUZZY_NETCODE_OK,
    FUZZY_NETCODE_ERROR
} FUZZY_NETCODES;

/* Message types */
typedef enum FUZZY_MESSAGE_TYPES {
    FUZZY_COMMAND_SHUTDOWN,
    FUZZY_COMMAND_AUTHENTICATE,
    FUZZY_COMMAND_GAME_CREATE,
    FUZZY_COMMAND_GAME_JOIN,
    FUZZY_COMMAND_GAME_START,
    FUZZY_COMMAND_GAME_FINISH,

    FUZZY_COMMAND_PLAYER_STEP,
    FUZZY_COMMAND_PLAYER_MOVE,
    FUZZY_COMMAND_PLAYER_ATTACK
} FUZZY_MESSAGE_TYPES;

/* Command specific data */
struct FuzzyCommandAuth {
    char key[FUZZY_SERVERKEY_LEN];
};
struct FuzzyCommandRoom {
    ulong id;
    char name[FUZZY_NET_ROOM_LEN];
};
union FuzzyCommandData {
    struct FuzzyCommandAuth auth;
    struct FuzzyCommandRoom room;
};

typedef struct FuzzyCommand {
    FUZZY_MESSAGE_TYPES type;
    union FuzzyCommandData data;
} FuzzyCommand;


/* Functions */
bool fuzzy_protocol_decode_message(FuzzyMessage * msg, FuzzyCommand * cmd);
bool fuzzy_protocol_server_shutdown(int svsock, FuzzyMessage * msg);
bool fuzzy_protocol_authenticate(int svsock, FuzzyMessage * msg, char * key);
ulong fuzzy_protocol_create_room(int svsock, FuzzyMessage * msg, char * name);
bool fuzzy_protocol_join(int svsock, FuzzyMessage * msg, ulong roomid);

#endif
