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

#include "network.h"
#include "server.h"
#include "protocol.h"
#include "fuzzy.h"

/* checks command return code, printing error. Returns true on ok. */
static bool _check_return_netcode(FuzzyMessage * msg, int svsock)
{
    ubyte8 netcode;

    fuzzy_message_recv(svsock, msg);
    netcode = fuzzy_message_pop8(msg);

    if (netcode != FUZZY_NETCODE_OK) {
        if (netcode == FUZZY_NETCODE_ERROR) {
            char err[FUZZY_NETERROR_CHARS];

            fuzzy_message_popstr(msg, err, FUZZY_NETERROR_CHARS);
            fuzzy_error(fuzzy_sformat("Net error: %s", err));
        }
        return false;
    }
    return true;
}

bool fuzzy_protocol_decode_message(FuzzyMessage * msg, FuzzyCommand * cmd)
{
    #define BAD_MSG "Bad message: "
    #define _fuzzy_bad_message(err)\
    do {\
        fuzzy_error(err);\
        return false;\
    } while(0)

    if(msg->buflen < 1)
        _fuzzy_bad_message(BAD_MSG "missing command type");

    cmd->type = fuzzy_message_pop8(msg);
    switch(cmd->type) {
        case FUZZY_COMMAND_AUTHENTICATE:
            if(msg->buflen < 1 + FUZZY_SERVERKEY_LEN)
                _fuzzy_bad_message(BAD_MSG "missing authentication key");

            fuzzy_message_popstr(msg, cmd->data.auth.key, FUZZY_SERVERKEY_LEN);
            break;
        case FUZZY_COMMAND_SHUTDOWN:
            break;
        case FUZZY_COMMAND_GAME_CREATE:
            if(msg->buflen < 1 + FUZZY_NET_ROOM_LEN)
                _fuzzy_bad_message(BAD_MSG "missing room name");

            fuzzy_message_popstr(msg, cmd->data.room.name, FUZZY_NET_ROOM_LEN);
            break;
        case FUZZY_COMMAND_GAME_JOIN:
            if(msg->buflen < 1 + 4)
                _fuzzy_bad_message(BAD_MSG "missing room id");

            cmd->data.room.id = fuzzy_message_pop32(msg);
            break;
        case FUZZY_COMMAND_GAME_START:
            if (msg->buflen != 1)
                _fuzzy_bad_message(BAD_MSG);
            break;
        default:
            _fuzzy_bad_message(fuzzy_sformat(BAD_MSG "unknown command type '0x%02x'", cmd->type));
    }
    return true;
}

bool fuzzy_protocol_server_shutdown(int svsock, FuzzyMessage * msg)
{
    fuzzy_message_push8(msg, FUZZY_COMMAND_SHUTDOWN);
    fuzzy_message_send(svsock, msg);

    return _check_return_netcode(msg, svsock);
}

bool fuzzy_protocol_authenticate(int svsock, FuzzyMessage * msg, char * key)
{
    fuzzy_message_pushstr(msg, key, FUZZY_SERVERKEY_LEN);
    fuzzy_message_push8(msg, FUZZY_COMMAND_AUTHENTICATE);
    fuzzy_message_send(svsock, msg);

    return _check_return_netcode(msg, svsock);
}

// 0 on error, >0 roomid on success
ulong fuzzy_protocol_create_room(int svsock, FuzzyMessage * msg, char * name)
{
    ulong roomid;

    fuzzy_message_pushstr(msg, name, FUZZY_NET_ROOM_LEN);
    fuzzy_message_push8(msg, FUZZY_COMMAND_GAME_CREATE);
    fuzzy_message_send(svsock, msg);

    if (! _check_return_netcode(msg, svsock))
        return 0;

    roomid = fuzzy_message_pop32(msg);
    return roomid;
}

bool fuzzy_protocol_join(int svsock, FuzzyMessage * msg, ulong roomid)
{
    fuzzy_message_push32(msg, roomid);
    fuzzy_message_push8(msg, FUZZY_COMMAND_GAME_JOIN);
    fuzzy_message_send(svsock, msg);

    return _check_return_netcode(msg, svsock);
}

bool fuzzy_protocol_game_start(int svsock, FuzzyMessage * msg)
{
    fuzzy_message_push8(msg, FUZZY_COMMAND_GAME_START);
    fuzzy_message_send(svsock, msg);

    return _check_return_netcode(msg, svsock);
}
