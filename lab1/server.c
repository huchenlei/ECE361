#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#define BUFFER_SIZE 256
#define FTP_RES "yes"
#define OTHER_RES "no"

int main(int argc, char const * argv[]) {
  if (argc != 2) {
    printf("Wrong number of argument! Must has exactly one argument\n");
    exit(1);
  }

  int port = atoi(argv[1]);
  struct sockaddr_in server_addr, client_addr;

  int sockfd = socket(PF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    printf("Failed to open socket at server\n");
    exit(1);
  }

  server_addr.sin_family = AF_INET; // Use ipv4
  /* the sin_port must be in Network Byte Order (by using htons()!) */
  server_addr.sin_port = htons(port); // port number
  server_addr.sin_addr.s_addr = INADDR_ANY;
  /* Note that sin_zero (which is included to pad the structure to the length of a struct sockaddr) should be set to all zeros with the function memset() */
  memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

  /* possible other option to config socket
    inet_pton(AF_INET, , &(server_addr.sin_addr)); // IPv4 */
  if ((bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr))) == -1) {
    printf("Failed to bind socket\n");
    exit(1);
  }

  for (;;) {
    socklen_t client_len = sizeof(client_addr);
    char buf[BUFFER_SIZE];
    bzero(buf, BUFFER_SIZE);

    // receive message from client
    if (recvfrom(sockfd, buf, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len) == -1) {
      printf("Failed to receive from client\n");
      exit(1);
    }

    // send message to client
    if (!strcmp(buf, "ftp")) {
      if ((sendto(sockfd, FTP_RES, strlen(FTP_RES), 0, (struct sockaddr*)&client_addr, sizeof(server_addr))) == -1) {
        printf("Failed to send message back to client\n");
      }
    } else {
      if ((sendto(sockfd, OTHER_RES, strlen(OTHER_RES), 0, (struct sockaddr*)&client_addr, sizeof(server_addr))) == -1) {
        printf("Failed to send message back to client\n");
      }
    }
    printf("Successful send message back to client\n");
  }
  close(sockfd);
  return 0;
}
