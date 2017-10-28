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

// Configuable constants

#define WINDOW_SIZE 32 // listening queue size
/* #define */
/* #define */
/* #define */
/* #define */
#define DEBUG

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET)
    return &(((struct sockaddr_in*)sa)->sin_addr);
  else
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

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
  int new_fd; // new connection on new_fd
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


  struct sockaddr_storage their_addr;
  char s[INET6_ADDRSTRLEN];
  while(1) { // main accept() loop
    socklen_t sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }
    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("server: got connection from %s\n", s);
    if (!fork()) { // this is the child process
      close(sockfd); // child doesn't need the listener
      if (send(new_fd, "Hello, world!", 13, 0) == -1)
        perror("send");
      close(new_fd);
      exit(0);
    }
    close(new_fd); // parent doesn't need this
  }
  return 0;
}
