#ifndef CLIENT_MENU_H
#define CLIENT_MENU_H

#include "message.h"


#define MAX_COMMAND_LEN 128
#define MAX_FIELD 32

struct user;
extern struct user* cur_user;
extern int client_sock;

//helper functions
int isloggedin();
int request(message_t type, const char* source, const char* data);
int recv_ack(message_t ack_type, message_t nak_type, int* retval, char** body);

// menu
int menu();
// menu options
int login(const char* name, const char* pass, const char* server_ip, const char* server_port);
int logout();
int join_session(const char* session_id);
int leave_session();
int create_session(const char* session_id);
int list();
int quit();
int send_message(const char* text);

#endif
