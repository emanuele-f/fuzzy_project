#ifndef __FUZZY_SERVER_H
#define __FUZZY_SERVER_H

#define FUZZY_DEFAULT_SERVER_PORT 7557
#define FUZZY_DEFAULT_SERVER_ADDRESS "127.0.0.1"
#define FUZZY_SERVER_BACKLOG 1
#define FUZZY_SERVERKEY_LEN 37

void fuzzy_server_create(int port, char * keyout);
void fuzzy_server_destroy();
void * fuzzy_server_loop(void * args);
int fuzzy_server_connect(char * addr, int port);
void fuzzy_server_stop(int svsock);

#endif
