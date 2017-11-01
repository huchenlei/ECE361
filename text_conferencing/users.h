#ifndef USERS_H
#define USERS_H

#include "message.h"
#define USER_NUM 2
#define MAX_PASS 256
#define MAX_JOINED_SESSION 128 // This number is the same as MAX_POSSIBLE session

struct session;

struct user {
  char name[MAX_NAME];
  char pass[MAX_PASS];
  // Current session user can send message to
  struct session* cur_session;
  struct session* joined_sessions[MAX_JOINED_SESSION];
  int sockfd; // connection between server and client
  int active; // whether is user is active
};

extern struct user users[USER_NUM];

void init_users();
int auth_user(int sockfd);
int logout_user(struct user* user);
int user_join_session(struct user* user, struct session* s);
int user_leave_session(struct user* user, struct session* s);
int user_send_msg(struct user* user, struct session* s, const char* msg);
int user_switch_session(struct user* user, struct session* s);
// private helper function that encapsulate the process user joining a session
// Join a new session
void _user_join_session(struct user* user, struct session* s);

#endif
