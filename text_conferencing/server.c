#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>

#include <arpa/inet.h>

#include "message.h"
#include "users.h"
#include "session.h"

// Configuable constants
#define WINDOW_SIZE 32 // listening queue size

#define DEBUG

// Handles user operations
int handle_user_req();

int main(int argc, char const * argv[]) {
  if (argc != 2) {
    printf("usage: server <port number>\n>");
    exit(1);
  }

  unsigned int port = atoi(argv[1]);
  struct addrinfo hints, *res;
  bzero(&hints, sizeof(hints));
  hints.ai_flags = AI_PASSIVE; // Use my IP
  hints.ai_family = AF_INET; // Use IPv4
  hints.ai_socktype = SOCK_STREAM; // Use TCP

  int err = getaddrinfo(NULL, argv[1], &hints, &res);
  if (err == -1) {
    fprintf(stderr, "failed in getaddrinfo %s\n", gai_strerror(err));
    exit(1);
  }

  int sockfd; // Listening on sockfd
  struct addrinfo *iter;
  for(iter = res; iter != NULL; iter = res->ai_next) {
    sockfd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
    if (sockfd == -1) {
#ifdef DEBUG
      printf("Attempt to find available sock failed ... \n");
#endif
      continue;
    }
    err = bind(sockfd, iter->ai_addr, iter->ai_addrlen);
    if (err == -1) {
      close(sockfd);
#ifdef DEBUG
      printf("Attempt to bind sock failed ...\n");
#endif
      continue;
    }
    break;
  }

  if (iter == NULL) {
    fprintf(stderr, "Tried all possible addr can't find any sock available\n");
    exit(1);
  }

  freeaddrinfo(res);

  if (listen(sockfd, WINDOW_SIZE)) {
    close(sockfd);
    fprintf(stderr, "Failed to listen at sockfd %d ...\n", sockfd);
    exit(1);
  }

  printf("server: start listening at %d ...\n", port);

  int main_sockfd = sockfd;
  int max_sockfd;
  for (;;) {
    FD_ZERO(&server_fds);
    FD_SET(main_sockfd, &server_fds);
    max_sockfd = main_sockfd;
    for (size_t i = 0; i < USER_NUM; i++) {
      if (users[i].active) {
        assert(users[i].sockfd != -1);
        FD_SET(users[i].sockfd, &server_fds);
        if (users[i].sockfd > max_sockfd)
          max_sockfd = users[i].sockfd;
      }
    }

    // always handle max sockfd first
    int err = select(max_sockfd + 1, &server_fds, NULL, NULL, NULL);
    if (err < 0) perror("select");

    if (FD_ISSET(main_sockfd, &server_fds)) {
      // Login request
      struct sockaddr new_addr;
      socklen_t new_addrlen;

      int new_sockfd = accept(sockfd, &new_addr, &new_addrlen);
      if (new_sockfd < 0) {
        perror("accept");
        continue;
      }

      if (!auth_user(new_sockfd))
        close(new_sockfd);
    } else {
      // Other request
      handle_user_req();
    }
  }
  return 0;
}

int handle_user_req() {
  for (size_t i = 0; i < USER_NUM; i++) {
    struct user* cur_user = &users[i];
    if (!cur_user->active) continue;
    assert(cur_user->sockfd > 0);
    if (FD_ISSET(cur_user->sockfd, &server_fds)) {
      char buf[MAX_MESSAGE];
      struct message m;
      /* bzero(buf, MAX_MESSAGE); */
      buf[MAX_MESSAGE - 1] = '\0'; // avoid overflow
      int err = recv(cur_user->sockfd, buf, MAX_MESSAGE, 0);
      if (err == -1) {
        printf("Failed to receive message from sockfd %d\n", cur_user->sockfd);
        continue;
      }
#ifdef DEBUG
      printf("Receving from %s: %s\n", cur_user->name, buf);
#endif
      if (parse_message(buf, &m)) {
        // If the packet is mal formed the user might be lost
        // logout the user on server
        logout_user(cur_user);
        return 1;
      }

      // Sanity check
      if (strcmp(m.source, cur_user->name) != 0) {
        printf("Fatal: sockfd does not match username! sock %d user %s source %s\n", cur_user->sockfd, cur_user->name, m.source);
        return 1;
      }
      assert(cur_user->active);

      // Process each situation
      switch (m.type) {
      case EXIT:
        err = logout_user(cur_user);
        break;
      case JOIN:
        err = user_join_session(cur_user, find_session(m.data));
        break;
      case LEAVE_SESS:
        err = user_leave_session(cur_user);
        break;
      case NEW_SESS:
        err = new_session(m.data, cur_user);
        break;
      case QUERY:
        // TODO
        printf("received a query\n");
        break;
      case MESSAGE:
        err = user_send_msg(cur_user, m.data);
        break;
      default:
        printf("Unknown type of message received...\n");
        err = 1;
      }
      return err;
    } // if
  } // for loop
}
