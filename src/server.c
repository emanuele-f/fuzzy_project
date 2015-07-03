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

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>
#include "fuzzy.h"
#include "server.h"
#include "network.h"
#include "protocol.h"

static int ServerSocket = -1;
static char ServerKey[FUZZY_SERVERKEY_LEN];
static bool ServerRun;
static FuzzyClient * ServerClients = NULL;

static bool _verify_auth(FuzzyClient * client, FUZZY_MESSAGE_TYPES cmdtype)
{
    if (! client->auth) {
        fuzzy_warning(fuzzy_sformat("Client %d is not authorized to perform command %d", client->socket, cmdtype));
        return false;
    }
    return true;
}

static FuzzyClient * _client_connected(int clsock, struct sockaddr_in * sa_addr)
{
    FuzzyClient * cl;

    cl = fuzzy_new(FuzzyClient);
    cl->socket = clsock;
    strncpy(cl->ip, inet_ntoa(sa_addr->sin_addr), sizeof(cl->ip));
    cl->port = sa_addr->sin_port;
    cl->auth = false;
    cl->next = NULL;

    // append to clients
    cl->next = ServerClients;
    ServerClients = cl;

    return cl;
}

static void _client_disconnected(int clsock)
{
    FuzzyClient *cl, *prev;

    prev = NULL;
    cl = ServerClients;
    while(cl) {
        if (cl->socket == clsock) {
            if (prev == NULL)
                ServerClients = cl->next;
            else
                prev->next = cl->next;
            break;
        }
        prev = cl;
        cl = cl->next;
    }

    if (cl == NULL)
        fuzzy_critical(fuzzy_sformat("Cannot find client for socket #%d", clsock));

    free(cl);
}

// get or die
static FuzzyClient * _get_client_by_socket(int clsock)
{
    FuzzyClient *cl;

    cl = ServerClients;
    while(cl) {
        if (cl->socket == clsock)
            return cl;
        cl = cl->next;
    }

    fuzzy_critical(fuzzy_sformat("Cannot find client for socket #%d", clsock));
}

static void _fuzzy_net_error(FuzzyMessage * msg, char * err, FuzzyClient * cl)
{

    fuzzy_message_clear(msg);
    fuzzy_message_pushstr(msg, err, FUZZY_NETERROR_CHARS);
    fuzzy_message_push8(msg, FUZZY_NETCODE_ERROR);
    fuzzy_message_send(cl->socket, msg);
}

static void _fuzzy_net_ok(FuzzyMessage * msg, FuzzyClient * cl)
{
    fuzzy_message_clear(msg);
    fuzzy_message_push8(msg, FUZZY_NETCODE_OK);
    fuzzy_message_send(cl->socket, msg);
}

static void _fuzzy_process_message(FuzzyMessage * msg, FuzzyClient * client)
{
    FuzzyCommand cmd;

    if (! fuzzy_protocol_decode_message(msg, &cmd)) {
        /* bad message */
        _fuzzy_net_error(msg, "Malformed message", client);
        return;
    }
    fuzzy_debug(fuzzy_sformat("Message[%d bytes] from socket %d", msg->buflen, client->socket));

    switch(cmd.type) {
        case FUZZY_COMMAND_AUTHENTICATE:
            if(strncmp(cmd.data.auth.key, ServerKey, FUZZY_SERVERKEY_LEN) != 0) {
                fuzzy_error(fuzzy_sformat("Bad key: %s", cmd.data.auth.key));
                _fuzzy_net_error(msg, "Bad key", client);
                return;
            } else {
                fuzzy_debug(fuzzy_sformat("Client %d authenticated", client->socket));
                client->auth = true;
            }
            break;

        case FUZZY_COMMAND_SHUTDOWN:
            if (_verify_auth(client, FUZZY_COMMAND_SHUTDOWN)) {
                fuzzy_debug("Server shutdown command received");
                ServerRun = false;
            }
            break;

        default:
            fuzzy_critical(fuzzy_sformat("Unknown command type '0x%02x'", cmd.type));
            return;
    }

    _fuzzy_net_ok(msg, client);
}

void fuzzy_server_create(int port, char * keyout)
{
    int yes = 1;
    struct sockaddr_in sa_srv;
    uuid_t key;

    sa_srv.sin_family = AF_INET;
    sa_srv.sin_port = htons(port);
    sa_srv.sin_addr.s_addr = htonl(INADDR_ANY);

    fuzzy_lz_perror(ServerSocket = socket(AF_INET, SOCK_STREAM, 0));
    fuzzy_lz_perror(setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)));
    uuid_generate_random(key);
    uuid_unparse_upper(key, ServerKey);
    fuzzy_debug(fuzzy_sformat("Server key: %s", ServerKey));

    fuzzy_lz_perror(bind(ServerSocket, (struct sockaddr *)&sa_srv, sizeof(sa_srv)));
    fuzzy_lz_perror(listen(ServerSocket, FUZZY_SERVER_BACKLOG));
    fuzzy_debug(fuzzy_sformat("Server listening on port %i", port));

    strncpy(keyout, ServerKey, FUZZY_SERVERKEY_LEN);
}

void fuzzy_server_destroy()
{
    if (ServerSocket == -1) {
        fuzzy_error("Server not running");
    } else {
        close(ServerSocket);
        ServerSocket = -1;
        fuzzy_debug("Server shutdown completed");
    }
}

void * fuzzy_server_loop(void * args)
{
    int i;
    int clsock;
    struct sockaddr_in sa_addr;
    fd_set active_fd_set, read_fd_set;
    socklen_t sa_size;
    FuzzyMessage * msg;
    FuzzyClient * client;

    if (ServerSocket == -1)
        fuzzy_critical("Server not running");

    /* Initialize the set of active sockets */
    FD_ZERO(&active_fd_set);
    FD_SET(ServerSocket, &active_fd_set);
    msg = fuzzy_message_new();

    ServerRun = 1;
    while(ServerRun) {
        read_fd_set = active_fd_set;
        fuzzy_lz_perror(select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL));

        for (i = 0; i<FD_SETSIZE; ++i) {
            if (FD_ISSET(i, &read_fd_set)) {
                if (i == ServerSocket) {
                    /* connection request */
                    sa_size = sizeof(sa_addr);
                    fuzzy_lz_perror(clsock = accept(ServerSocket, (struct sockaddr *)&sa_addr, &sa_size));
                    client = _client_connected(clsock, &sa_addr);
                    fuzzy_debug(fuzzy_sformat("Client %s:%d connected -> socket #%d", client->ip, client->port, client->socket));

                    FD_SET(clsock, &active_fd_set);
                } else {
                    client = _get_client_by_socket(i);
                    if ((fuzzy_message_recv(i, msg)) == false) {
                        /* disconnection */
                        fuzzy_debug(fuzzy_sformat("Client %s:%d disconnected", client->ip, client->port));
                        close(client->socket);
                        FD_CLR (client->socket, &active_fd_set);
                        _client_disconnected(client->socket);
                    } else {
                        /* incoming data */
                        bool more = 1;

                        while (more) {
                            _fuzzy_process_message(msg, client);

                            if (! fuzzy_message_poll(i))
                                more = false;
                            else
                                fuzzy_message_recv(i, msg);
                        }
                    }
                }
            }
        }
    }

    fuzzy_message_del(msg);
    return 0;
}

int fuzzy_server_connect(char * host, int port)
{
    struct sockaddr_in serv_addr;
    int sock;

    bzero((char *) &serv_addr, sizeof(serv_addr));
    fuzzy_iz_perror(inet_aton(host, &serv_addr.sin_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    fuzzy_lz_perror(sock = socket(AF_INET, SOCK_STREAM, 0));
    fuzzy_lz_perror(connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));
    fuzzy_debug(fuzzy_sformat("Connected to server %s:%d", host, port));

    return sock;
}
