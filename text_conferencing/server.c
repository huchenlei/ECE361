#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "message.h"
#include "users.h"

// Configuable constants

/* #define */
/* #define */
/* #define */
/* #define */
/* #define */
#define DEBUG

int main(int argc, char const * argv[]) {
  /* API testcases */
  /* if (verify_user("Chenlei, chenlei")) { */
  /*   printf("verify user pass!\n"); */
  /* } */

  if (argc != 2) {
    printf("usage: server <port number>\n>");
    exit(1);
  }

  while (1) {

  }
  return 0;
}
