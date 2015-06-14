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

bool fuzzy_protocol_decode_message(FuzzyMessage * msg, FuzzyMessageInfo * info)
{
    #define BAD_MSG "Bad message: "
    #define _fuzzy_bad_message(err)\
    do {\
        fuzzy_error(err);\
        return false;\
    } while(0)

    if(msg->buflen < 1)
        _fuzzy_bad_message(BAD_MSG "missing message type");

    info->type = fuzzy_message_pop8(msg);
    switch (info->type) {
    case FUZZY_MESSAGE_TYPE_COMMAND:
        if(msg->buflen < 1 + FUZZY_SERVERKEY_LEN + 1)
            _fuzzy_bad_message(BAD_MSG "missing auth key or command type");

        info->type = FUZZY_MESSAGE_TYPE_COMMAND;
        fuzzy_message_popstr(msg, info->data.cmd.authkey, FUZZY_SERVERKEY_LEN);
        info->data.cmd.cmdtype = fuzzy_message_pop8(msg);

        switch(info->data.cmd.cmdtype) {
        case FUZZY_COMMAND_SHUTDOWN:
            break;
        default:
            _fuzzy_bad_message(fuzzy_sformat(BAD_MSG "unknown command type '0x%02x'", info->data.cmd.cmdtype));
        }
        break;
    default:
        _fuzzy_bad_message(fuzzy_sformat(BAD_MSG "unknown message type '0x%02x'", info->type));
    }

    return true;
}

void fuzzy_protocol_server_shutdown(int svsock, FuzzyMessage * msg, char * key)
{
    fuzzy_message_push8(msg, FUZZY_COMMAND_SHUTDOWN);
    fuzzy_message_pushstr(msg, key, FUZZY_SERVERKEY_LEN);
    fuzzy_message_push8(msg, FUZZY_MESSAGE_TYPE_COMMAND);
    fuzzy_message_send(svsock, msg);
}
