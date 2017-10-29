#ifndef USERS_H
#define USERS_H

#include "message.h"
#define USER_NUM 2
#define MAX_PASS 256

struct session;

struct user {
  char name[MAX_NAME];
  char pass[MAX_PASS];
  struct session* cur_session;
  int sockfd; // connection between server and client
  int active; // whether is user is active
};

extern struct user users[USER_NUM];

int auth_user(int sockfd);
int logout_user(struct user* user);
int join_session(struct user* user, struct session* s);
int leave_session(struct user* user);
#endif
