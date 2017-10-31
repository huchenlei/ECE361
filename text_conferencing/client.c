#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "client_menu.h"

#define VERSION "1.0"
#define PROMPT "\n>"

int main(int argc, char *argv[]) {
  printf("Welcome to Text Conferencing Pro v%s\n", VERSION);
  // set of file descriptors
  fd_set fds;

  for (;;) {
    FD_ZERO(&fds);
    FD_SET(fileno(stdin), &fds);

    if (client_sock > 0) {
      FD_SET(client_sock, &fds);
      select(client_sock + 1, &fds, NULL, NULL, NULL);
    } else {
      select(fileno(stdin) + 1, &fds, NULL, NULL, NULL);
    }

    // Receive message
    if (isloggedin() && FD_ISSET(client_sock, &fds)) {
      char buf[MAX_MESSAGE];
      recv(client_sock, buf, MAX_MESSAGE, 0);
      buf[MAX_MESSAGE - 1] = '\0'; // Avoid overflow
      printf("client recevicing: %s\n", buf);
      // TODO to be removed
      /* scanf("%s", buf); // block the print */
    } else if (FD_ISSET(fileno(stdin), &fds)) {
      // TODO currently ignoring err generated by menu
      menu();
    }
  }
  return 0;
}
