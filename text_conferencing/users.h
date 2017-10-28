#ifndef USERS_H
#define USERS_H

#include "message.h"
#define USER_NUM 2
#define MAX_PASS 256

typedef struct {
  char name[MAX_NAME];
  char pass[MAX_PASS];
} user;

extern const user users[USER_NUM];

// Verify the user through data string
// @param data data string in the format 'username,password'
// @returns bool
int verify_user(char* data);

#endif
