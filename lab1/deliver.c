#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {
  if (argc != 3) {
    printf("Wrong number of argument");
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
  char buf[256];
  bzero(buf, 256);
  fgets(buf, 256, stdin);

  char *newline = strchr(buf, '\n');
  if (newline) {
    *newline = 0;
  }

  char *proto_name;
  proto_name = strtok(buf, " ");

  char *file_name;
  file_name = strtok(NULL, "");

  /* printf("file name is %s\n", file_name); */
  /* int i = 3; */
  /* while (buf[i] == ' ') { */
  /*   i++; */
  /* } */
  /* int file_name_char = 0; */
  /* while (buf[i] != ' ' && buf[i] != '\n') { */
  /*   file_name[file_name_char] = buf[i]; */
  /*   file_name_char++; */
  /*   i++; */
  /* } */
  /* file_name[file_name_char] ='\0'; */

  /* else { */
  /*   printf("unrecognized protocol name\n"); */
  /*   exit(1); */
  /* } */

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
  unsigned int total_frags = (file_size % 1000 == 0) ? (file_size/1000) : (file_size/1000+1);
  int numbytes;
  if((numbytes = sendto(sockfd, proto_name, strlen(proto_name), 0, serverinfo->ai_addr, serverinfo->ai_addrlen)) == -1){
    printf("talker: sendto\n");
    exit(1);
  };
  bzero(buf, 256);
  if ((numbytes = recvfrom(sockfd, buf, 256, 0, (struct sockaddr *) &serv_addr, &serv_addr_size)) == -1) {
    printf("No bytes recevied at client\n");
    exit(1);
  }
  if (!strcmp(buf, "yes")) {
    printf("Got 'yes' from server, a file transfer can initiate\n");
  }
  if (!strcmp(buf, "no")) {
    // This should never happen
    printf("Got 'no' from server, inproper protocol name\n");
  }
  freeaddrinfo(serverinfo);
  close(sockfd);
  fclose(fp);

  return 0;
}
