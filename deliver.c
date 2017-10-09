#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include "packet.h"

#define MAX_PACK_LEN 4096

int main(int argc, char const *argv[]) {
  if (argc != 3) {
    printf("Wrong number of argument\n");
    printf("Usage: deliver <server address> <server port num>");
    exit(1);
  }

  int sockfd;

  struct sockaddr_in serv_addr;

  struct addrinfo hints;
  struct addrinfo *serverinfo;

  // config structs
  memset(&hints, 0 , sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;

  // DNS lookup
  int status;
  if ((status = getaddrinfo(argv[1], argv[2], &hints, &serverinfo )) == -1) {
    printf("DNS lookup failed\n");
    exit(1);
  }
  socklen_t serv_addr_size = sizeof(serv_addr);
  if ((sockfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol)) == -1) {
    printf("Can't open socket at client\n");
    exit(1);
  }

  // Get user input
  printf("Please specify transfer protocol and file name:\n");
  char buf[256];
  bzero(buf, 256);
  fgets(buf, 256, stdin);

  char *newline = strchr(buf, '\n');
  if (newline) {
    *newline = 0;
  }

  char *proto_name;
  proto_name = strtok(buf, " ");

  char file_name[MAX_PACK_LEN];
  strcpy(file_name, strtok(NULL, ""));

  // Checking if the file exists.
  if(access(file_name, F_OK) == -1) {
    printf("File \"%s\" doesn't exist.\n", file_name);
    exit(1);
  }

  // get file size (also opens file)
  FILE *fp = fopen(file_name, "rb");
  fseek(fp, 0, SEEK_END);
  unsigned int file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  unsigned int total_frag = (unsigned int)ceil(((double)file_size) / DATA_LEN);

  // Send initial protocol name to server to init a file transfer
  if((sendto(sockfd, proto_name, strlen(proto_name), 0, serverinfo->ai_addr, serverinfo->ai_addrlen)) == -1){
    printf("talker: sendto\n");
    exit(1);
  };
  // Acknoledgement from server
  bzero(buf, 256);
  if ((recvfrom(sockfd, buf, 256, 0, (struct sockaddr *) &serv_addr, &serv_addr_size)) == -1) {
    printf("No bytes recevied at client\n");
    exit(1);
  }
  if (!strcmp(buf, "yes")) {
    printf("Got 'yes' from server, a file transfer can initiate\n");
  } else if (!strcmp(buf, "no")) {
    printf("Got 'no' from server, quiting...\n");
    exit(1);
  }

  // Start file transfer
  char file_buf[DATA_LEN];
  char pack_buf[MAX_PACK_LEN];
  unsigned int frag_no = 0;
  while (!feof(fp)) {
    // read data from file
    int size = fread(file_buf, sizeof(char), DATA_LEN, fp);
    // send the packet
    unsigned int header_offset = sprintf(pack_buf, "%d:%d:%d:%s:", total_frag, frag_no, size, file_name);
    /* printf("confirm file name is %s\n", file_name); */
    /* printf("pack header is %s\n", pack_buf); */
    memcpy(pack_buf + header_offset, file_buf, size);
    if ((sendto(sockfd, pack_buf, header_offset + size, 0, serverinfo->ai_addr, serverinfo->ai_addrlen)) == -1) {
      printf("Error sending packet %d\n", frag_no);
    }

    if ((recvfrom(sockfd, buf, 256, 0, (struct sockaddr *) &serv_addr, &serv_addr_size)) == -1) {
      if (errno == EAGAIN) {
        // Timeout occours
        printf("Timeout ACK/NACK: resending the packet %d...\n", frag_no);
        // Resend the packet
        fseek(fp, -size, SEEK_CUR);
        continue;
      } else {
        printf("Unknown error receiving ACK for packet %d\n", frag_no);
      }
    }

    if (!strcmp(buf, NACK_RES)) {
      fseek(fp, -size, SEEK_CUR);
      printf("NACK! resending the packet %d...\n", frag_no);
      continue;
    }
    if (strcmp(buf, ACK_RES) != 0) {
      printf("Unknown server response: %s, quiting...\n", buf);
      exit(1);
    }
    frag_no++;
  }

  printf("File send report: %d bytes sent\n", file_size);

  freeaddrinfo(serverinfo);
  close(sockfd);
  fclose(fp);
  return 0;
}
