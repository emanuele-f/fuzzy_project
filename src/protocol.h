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
