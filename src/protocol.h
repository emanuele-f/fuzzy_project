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

/* Message types */
typedef enum FUZZY_MESSAGE_TYPE {
    FUZZY_MESSAGE_TYPE_COMMAND,
    FUZZY_MESSAGE_TYPE_GAME
} FUZZY_MESSAGE_TYPE;
typedef enum FUZZY_MESSAGE_COMMAND {
    FUZZY_COMMAND_SHUTDOWN
} FUZZY_MESSAGE_COMMAND;
typedef enum FUZZY_MESSAGE_GAME {
    FUZZY_GAME_STEP
} FUZZY_MESSAGE_GAME;

/* A command */
typedef struct FuzzyCommandData {
    FUZZY_MESSAGE_COMMAND cmdtype;
    char authkey[FUZZY_SERVERKEY_LEN];
} FuzzyCommandData;

/* Message data */
typedef union FuzzyMessageData {
    FuzzyCommandData cmd;
} FuzzyMessageData;

/* Message info: group attributes */
typedef struct FuzzyMessageInfo {
    FUZZY_MESSAGE_TYPE type;
    FuzzyMessageData data;
}FuzzyMessageInfo;

/* Functions */
void fuzzy_protocol_server_shutdown(int svsock, FuzzyMessage * msg, char * key);
bool fuzzy_protocol_decode_message(FuzzyMessage * msg, FuzzyMessageInfo * info);

#endif
