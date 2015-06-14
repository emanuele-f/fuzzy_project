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

/*
    A library to abstract network/calculator subsystem to perform network
    operations.

    MESSAGE: the container for the data to send
    NETWORK TYPES: a conversion must be performed to convert standard types
        to network types. Supported types: ubyte8 ubyte16 ubyte32

    Endianess is respected

    Ordering: network MSB first, network LSB after
    Size: size is encoded in 32bit storage

    \note cursor always points to a new insertion byte
 */

#ifndef __FUZZY_NETWORK_H
#define __FUZZY_NETWORK_H

#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>

#define FUZZY_DEFAULT_MESSAGE_SIZE 256

typedef unsigned char ubyte8;
typedef uint16_t ubyte16;
typedef uint32_t ubyte32;

typedef struct FuzzyMessage {
    ubyte8 * buffer;
    ssize_t buflen;
    ssize_t cursor;
}FuzzyMessage;

/* Misc */
FuzzyMessage * fuzzy_message_new();
void fuzzy_message_del(FuzzyMessage * msg);
bool fuzzy_message_poll(int sock);

/* Push message data */
void fuzzy_message_push8(FuzzyMessage * msg, ubyte8 data);
void fuzzy_message_push16(FuzzyMessage * msg, ubyte16 data);
void fuzzy_message_push32(FuzzyMessage * msg, ubyte32 data);
void fuzzy_message_pushstr(FuzzyMessage * msg, const char * data, ssize_t len);

/* Pop message data */
ubyte8 fuzzy_message_pop8(FuzzyMessage * msg);
ubyte16 fuzzy_message_pop16(FuzzyMessage * msg);
ubyte32 fuzzy_message_pop32(FuzzyMessage * msg);
void fuzzy_message_popstr(FuzzyMessage * msg, char * out, ssize_t len);

/* Push fitting uint */
void fuzzy_message_push8uint(FuzzyMessage * msg, uint data);
void fuzzy_message_push16uint(FuzzyMessage * msg, uint data);
void fuzzy_message_push32uint(FuzzyMessage * msg, uint data);

/* Exchange routines */
void fuzzy_message_send(int sock, FuzzyMessage * msg);
bool fuzzy_message_recv(int sock, FuzzyMessage * msg);

#endif
