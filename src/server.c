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

static void _fuzzy_process_message(FuzzyMessage * msg, int clsock)
{
    FuzzyMessageInfo minfo;

    if (! fuzzy_protocol_decode_message(msg, &minfo))
        /* bad message */
        return;
    fuzzy_debug(fuzzy_sformat("Message[%d bytes] from fd %d", msg->buflen, clsock));

    switch(minfo.type) {
    case FUZZY_MESSAGE_TYPE_COMMAND:
        if(strncmp(minfo.data.cmd.authkey, ServerKey, FUZZY_SERVERKEY_LEN) != 0) {
            fuzzy_error(fuzzy_sformat("Wrong key: %s", minfo.data.cmd.authkey));
            return;
        }

        switch(minfo.data.cmd.cmdtype) {
            case FUZZY_COMMAND_SHUTDOWN:
                fuzzy_debug("Server shutdown command received");
                ServerRun = false;
                break;
            default:
                fuzzy_critical(fuzzy_sformat("Unknown command type '0x%02x'", minfo.data.cmd.cmdtype));
        }
        break;
    default:
        fuzzy_critical(fuzzy_sformat("Unknown message type '0x%02x'", minfo.type));
    }
}

void fuzzy_server_create(int port, char * keyout)
{
    struct sockaddr_in sa_srv;
    uuid_t key;

    sa_srv.sin_family = AF_INET;
    sa_srv.sin_port = htons(port);
    sa_srv.sin_addr.s_addr = htonl(INADDR_ANY);

    fuzzy_lz_perror(ServerSocket = socket(AF_INET, SOCK_STREAM, 0));
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
                    fuzzy_debug(fuzzy_sformat("Client %s:%d connected -> fd %d", inet_ntoa(sa_addr.sin_addr), sa_addr.sin_port, clsock));

                    FD_SET(clsock, &active_fd_set);
                } else {
                    if ((fuzzy_message_recv(i, msg)) == false) {
                        /* disconnection */
                        sa_size = sizeof(sa_addr);
                        fuzzy_lz_perror(getpeername(i, (struct sockaddr *)&sa_addr, &sa_size));
                        fuzzy_debug(fuzzy_sformat("Client %s:%d disconnected", inet_ntoa(sa_addr.sin_addr), sa_addr.sin_port));
                        close(i);
                        FD_CLR (i, &active_fd_set);
                    } else {
                        /* incoming data */
                        bool more = 1;

                        while (more) {
                            _fuzzy_process_message(msg, i);

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
