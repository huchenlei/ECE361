#include <stdio.h>
#include <string.h>
#include "users.h"

const user users[USER_NUM] = {
  {.name = "Chenlei", .pass = "chenlei"},
  {.name = "Alex", .pass = "alex"}
};

int verify_user(char* data) {
  char* original_data = data;
  unsigned name_len = 0;
  while (*data != ',') {
    name_len++;
    data++;
  }
  char name[MAX_NAME];
  char pass[MAX_PASS];
  strncpy(name, original_data, name_len);
  strncpy(pass, original_data+name_len+1, // +1 to ignore ','
          strlen(original_data) - name_len);
#ifdef DEBUG
  printf("user: %s, pass: %s\n", name, pass);
#endif

  for (unsigned int i; i < USER_NUM; i++) {
    if ((strcmp(users[i].name, name) == 0) && (strcmp(users[i].pass, pass) == 0)) {
      return 1; // return true
    }
  }
  return 0; // return false if no matching
}
