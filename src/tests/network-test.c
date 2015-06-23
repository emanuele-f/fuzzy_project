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
 * A test for fuzzy network library
 *
 */

#include <stdio.h>
#include <string.h>
#include <mcheck.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/limits.h>
#include <sys/un.h>
#include "fuzzy.h"
#include "network.h"

#define TEST_STRING "TEST STRING !!!"

#define push_n_pop(pfunc, popfunc, val)\
do{\
    int _rval;\
    pfunc(msg, val);\
    if ((_rval = popfunc(msg)) != val)\
        fuzzy_critical(fuzzy_sformat("Chain '%s(%d)' but %s returned %d", fuzzy_str(pfunc), val, fuzzy_str(popfunc), _rval));\
} while(0)

#define pop_n_cmpstr(tstr)\
do{\
    fuzzy_message_popstr(msg, tstr, sizeof(tstr));\
    if (strncmp(tstr, TEST_STRING, sizeof(teststr)) != 0)\
        fuzzy_critical(fuzzy_sformat("Strings differ: sent '%s' but got '%s'", TEST_STRING, teststr));\
} while(0)

#define test_message_function(fn)\
do{\
    fuzzy_test_prepare();\
    fn(msg);\
    if (! fuzzy_test_result())\
        fuzzy_critical(fuzzy_sformat("Test for function '%s' failed", fuzzy_str(fn)));\
} while(0)

static void _bogus_push(FuzzyMessage * msg, uint data) {}

int main()
{
    FuzzyMessage * msg;
    char teststr[FUZZY_DEFAULT_MESSAGE_SIZE*2];
    char template[] = "fuzzy_XXXXXX";
    struct sockaddr_un address;
    int fd, svfd, clfd;
    socklen_t addrlen;
    mtrace();
    
    strncpy(teststr, TEST_STRING, sizeof(teststr));
    
    /* Simple push&pop */
    msg = fuzzy_message_new();
    push_n_pop(fuzzy_message_push8uint, fuzzy_message_pop8, 7);
    push_n_pop(fuzzy_message_push16uint, fuzzy_message_pop16, 547);
    push_n_pop(fuzzy_message_push32uint, fuzzy_message_pop32, 890000);
    fuzzy_message_del(msg);
    
    /* Bad pops */
    msg = fuzzy_message_new();
    test_message_function(fuzzy_message_pop8);
    fuzzy_message_push8(msg, 55);
    test_message_function(fuzzy_message_pop16);
    fuzzy_message_push16(msg, 55);
    test_message_function(fuzzy_message_pop32);
    fuzzy_message_del(msg);
    
    /* Free non empty message */
    msg = fuzzy_message_new();
    fuzzy_message_push32(msg, 777);
    fuzzy_message_push32(msg, 777);
    fuzzy_message_push32(msg, 777);
    fuzzy_message_del(msg);
    
    /* Multiple pushes / pops */
    msg = fuzzy_message_new();
    fuzzy_message_push8uint(msg, 7);
    fuzzy_message_push16uint(msg, 70);
    fuzzy_message_push32uint(msg, 700);
    fuzzy_message_pushstr(msg, teststr, sizeof(teststr));
    push_n_pop(fuzzy_message_push32uint, fuzzy_message_pop32, 7000);
    pop_n_cmpstr(teststr);
    push_n_pop(_bogus_push, fuzzy_message_pop32, 700);
    push_n_pop(_bogus_push, fuzzy_message_pop16, 70);
    push_n_pop(_bogus_push, fuzzy_message_pop8, 7);
    fuzzy_message_del(msg);
    
    /* Socket send/receive */
    fuzzy_lz_perror(fd = socket(AF_UNIX, SOCK_STREAM, 0));
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    mktemp(template);                       // I know, this is dangerous
    snprintf(address.sun_path, PATH_MAX, template);
    fuzzy_lz_perror(bind(fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)));
    fuzzy_lz_perror(listen(fd, 1));
    fuzzy_lz_perror(svfd = socket(AF_UNIX, SOCK_STREAM, 0));
    fuzzy_lz_perror(connect(svfd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)));
    unlink(template);
    addrlen = sizeof(struct sockaddr_un);
    fuzzy_lz_perror(clfd = accept(fd, (struct sockaddr *) &address, &addrlen));
    
    msg = fuzzy_message_new();
    fuzzy_message_push16uint(msg, 123);
    fuzzy_message_pushstr(msg, teststr, sizeof(teststr));
    fuzzy_message_push32uint(msg, 4444);
    fuzzy_message_send(svfd, msg);
    fuzzy_message_del(msg);
    
    if (! fuzzy_message_poll(clfd) )
        fuzzy_critical("Message poll failed");
    
    msg = fuzzy_message_new();
    fuzzy_message_recv(clfd, msg);
    push_n_pop(_bogus_push, fuzzy_message_pop32, 4444);
    pop_n_cmpstr(teststr);
    push_n_pop(_bogus_push, fuzzy_message_pop16, 123);
    fuzzy_message_del(msg);
    close(fd);
    
    return EXIT_SUCCESS;
}
