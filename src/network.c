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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "network.h"
#include "fuzzy.h"

static FuzzyMessage * _fuzzy_message_allocate(ssize_t buflen)
{
    FuzzyMessage * msg;

    fuzzy_iz_perror(msg = (FuzzyMessage *) malloc(sizeof(FuzzyMessage)));
    fuzzy_iz_perror(msg->buffer = (ubyte8 *) malloc(buflen));
    msg->buflen = buflen;
    msg->cursor = 0;
    return msg;
}

static void _fuzzy_message_expand(FuzzyMessage * msg, ssize_t nbuflen)
{
    ubyte8 * newbuf;

    if (msg->buflen == nbuflen)
        return;
    else if (msg->buflen > nbuflen)
        fuzzy_critical("Message resize to lower size");

    if (msg->cursor > nbuflen)
        fuzzy_critical(fuzzy_sformat("Cursor %d bytes outside valid area", msg->cursor-nbuflen));

    fuzzy_iz_perror(newbuf = (ubyte8 *) malloc(nbuflen));
    memcpy(newbuf, msg->buffer, msg->cursor);
    free(msg->buffer);
    msg->buffer = newbuf;
    msg->buflen = nbuflen;
}

FuzzyMessage * fuzzy_message_new()
{
    return _fuzzy_message_allocate(FUZZY_DEFAULT_MESSAGE_SIZE);
}

void fuzzy_message_del(FuzzyMessage * msg)
{
    free(msg->buffer);
    free(msg);
}

void fuzzy_message_push8(FuzzyMessage * msg, ubyte8 data)
{
    if ((msg->buflen - msg->cursor) < 1)
        _fuzzy_message_expand(msg, fuzzy_max(msg->buflen * 2, 1));

    msg->buffer[msg->cursor] = data;
    msg->cursor++;
}

void fuzzy_message_push16(FuzzyMessage * msg, ubyte16 data)
{
    if ((msg->buflen - msg->cursor) < 2)
        _fuzzy_message_expand(msg, fuzzy_max(msg->buflen*2, 2));

    data = htons(data);
    msg->buffer[msg->cursor+0] = data >> 8;
    msg->buffer[msg->cursor+1] = data >> 0;
    msg->cursor += 2;
}

void fuzzy_message_push32(FuzzyMessage * msg, ubyte32 data)
{
    if ((msg->buflen - msg->cursor) < 4)
        _fuzzy_message_expand(msg, fuzzy_max(msg->buflen*2, 4));

    data = htonl(data);
    msg->buffer[msg->cursor+0] = data >> 24;
    msg->buffer[msg->cursor+1] = data >> 16;
    msg->buffer[msg->cursor+2] = data >> 8;
    msg->buffer[msg->cursor+3] = data >> 0;
    msg->cursor += 4;
}

void fuzzy_message_pushstr(FuzzyMessage * msg, const char * data, ssize_t len)
{
    ssize_t slen;

    if((msg->buflen - msg->cursor) < len)
        _fuzzy_message_expand(msg, len + msg->cursor);

    slen = strnlen(data, len);
    memcpy(&msg->buffer[msg->cursor], data, slen);

    /* if it's a null terminated string, bzero to avoid data leaks */
    bzero(&msg->buffer[msg->cursor+slen], len-slen);
    msg->cursor += len;
}

ubyte8 fuzzy_message_pop8(FuzzyMessage * msg)
{
    ubyte8 data;

    if (msg->cursor < 1)
        fuzzy_critical("Message buffer does not hold a 8 bit value");

    data = msg->buffer[msg->cursor-1];
    msg->cursor--;

    return data;
}

ubyte16 fuzzy_message_pop16(FuzzyMessage * msg)
{
    ubyte16 data;

    if (msg->cursor < 2)
        fuzzy_critical("Message buffer does not hold a 16 bit value");

    data = msg->buffer[msg->cursor-1];
    data |= msg->buffer[msg->cursor-2] << 8;
    data = ntohs(data);
    msg->cursor-=2;

    return data;
}

ubyte32 fuzzy_message_pop32(FuzzyMessage * msg)
{
    ubyte32 data;

    if (msg->cursor < 4)
        fuzzy_critical("Message buffer does not hold a 32 bit value");

    data = msg->buffer[msg->cursor-1];
    data |= msg->buffer[msg->cursor-2] << 8;
    data |= msg->buffer[msg->cursor-3] << 16;
    data |= msg->buffer[msg->cursor-4] << 24;
    data = ntohl(data);
    msg->cursor-=4;

    return data;
}

/* strings are sent entirely! only use with short data */
void fuzzy_message_popstr(FuzzyMessage * msg, char * out, ssize_t len)
{
    if (msg->cursor < len)
        fuzzy_critical(fuzzy_sformat("Message buffer does not hold a string[%d]", len));

    //~ fuzzy_debug(fuzzy_sformat("KEY: %s", msg->buffer[msg->cursor-len]));

    memcpy(out, &msg->buffer[msg->cursor-len], len);
    //~ bzero(out, 1);
    msg->cursor-=len;
}

void fuzzy_message_push8uint(FuzzyMessage * msg, uint data)
{
    if (data > 255)
        fuzzy_critical("Data does not fit 8 bit storage");

    fuzzy_message_push8(msg, (ubyte8)data);
}

void fuzzy_message_push16uint(FuzzyMessage * msg, uint data)
{
    if (data > 65535)
        fuzzy_critical("Data does not fit 16 bit storage");

    fuzzy_message_push16(msg, (ubyte16)data);
}

void fuzzy_message_push32uint(FuzzyMessage * msg, uint data)
{
    if (data > ((ubyte32)4294967295LL))
        fuzzy_critical("Data does not fit 32 bit storage");

    fuzzy_message_push32(msg, (ubyte32)data);
}

void fuzzy_message_clear(FuzzyMessage * msg)
{
    msg->cursor = 0;
}

/* send entire message */
void fuzzy_message_send(int sock, FuzzyMessage * msg)
{
    const ssize_t realen = msg->cursor;
    ssize_t sent;

    fuzzy_lz_perror(send(sock, &realen, 4, MSG_NOSIGNAL));
    fuzzy_lz_perror(sent = send(sock, msg->buffer, realen, MSG_NOSIGNAL));
    if (sent != realen)
        fuzzy_critical("Partial message send");
}

bool fuzzy_message_poll(int sock)
{
    fd_set single;
    struct timeval tv;
    int retval;

    FD_ZERO(&single);
    FD_SET(sock, &single);

    /* do not wait */
    bzero(&tv, sizeof(tv));
    fuzzy_lz_perror(retval = select(FD_SETSIZE, &single, NULL, NULL, &tv));
    if (retval == 1)
        return 1;
    return 0;
}

/* blocks and recvs a entire message
    \param sock socket to recv from
    \param msg data container; it's overwritten and could be resized

    \retval TRUE read
    \retval FALSE connection closed
 */
bool fuzzy_message_recv(int sock, FuzzyMessage * msg)
{
    ssize_t recved;
    ubyte32 len;

    recved = recv(sock, &len, 4, 0);
    if (recved == 0)
        // disconnected
        return false;
    else if (recved < 0)
        fuzzy_critical(fuzzy_strerror(errno));

    if (msg->buflen < len)
        _fuzzy_message_expand(msg, len);

    fuzzy_lz_perror(recved = recv(sock, msg->buffer, len, 0));
    if (recved != len)
        fuzzy_critical("Partial message receive");

    msg->buflen = len;
    msg->cursor = len;
    return true;
}
