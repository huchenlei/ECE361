#ifndef USERS_H
#define USERS_H

#include "message.h"
#define USER_NUM 2
#define MAX_PASS 256

struct user {
  char name[MAX_NAME];
  char pass[MAX_PASS];
};

extern const struct user users[USER_NUM];

// Verify the user through data string
// @param data data string in the format 'username,password'
// @returns bool
// @server
int verify_user(char* data);

#endif
