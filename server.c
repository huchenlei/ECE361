#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include "packet.h"

#define BUFFER_SIZE 4096 // Max packet size
#define FTP_RES "yes"
#define OTHER_RES "no"
#define FILE_RECV_PATH "./recv/"

/* Parse received string to packet struct */
void parse_packet(const char* buf, struct packet* p);

int main(int argc, char const * argv[]) {
  if (argc != 2) {
    printf("Wrong number of argument! Must has exactly one argument\n");
    printf("Usage: server <server port num>");
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

  printf("Start listing at port %d\n", port);

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
    if (!strncmp(buf, "ftp", 4)) {
      if ((sendto(sockfd, FTP_RES, strlen(FTP_RES), 0, (struct sockaddr*)&client_addr, sizeof(server_addr))) == -1) {
        printf("Failed to send message back to client\n");
      } else {
        printf("Successful send message back to client. Now listening for packet transfering...\n");

        // Start listening for file transfer
        FILE *fp = NULL;
        char* filepath;
        int *received_nos; // Array to keep track of packet received
        int first_packet_flag = 1;
        int prev_frag_no = -1; // frag number starts from 0
        for (;;) {
          struct packet p_re;
          if (recvfrom(sockfd, buf, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len) == -1) {
            printf("failed to recive datagram...\n");
            // Not exiting. Continue listing for next packet
          } else {
            // Parse packet
            parse_packet(buf, &p_re);
            // Process first packet
            if (first_packet_flag) {
              first_packet_flag = 0;
              // Create file recv
              filepath = malloc(sizeof(char) * (strlen(FILE_RECV_PATH) + strlen(p_re.filename)));
              strcpy(filepath, FILE_RECV_PATH);
              strcat(filepath, p_re.filename);
              fp = fopen(filepath, "w");
              if (!fp) {
                printf("Unable to create received file name: %s\n", filepath);
                break; // Stop listing for packet
              }
              received_nos = calloc(p_re.total_frag, sizeof(int));
            }
            // ACK/NACK
            // ACK if the received packet has been received before or
            // its the direct subsequent packet of the previous packet
            // ELSE NACK
            if (p_re.frag_no - prev_frag_no == 1 || received_nos[p_re.frag_no]) {
              // ACK
              if(sendto(sockfd, ACK_RES, strlen(ACK_RES), 0, (struct sockaddr*)&client_addr, sizeof(server_addr)) == -1) {
                printf("Failed to ack the client packet received\n");
              }
              if (received_nos[p_re.frag_no])
                continue; // Continue listening since the packet has already been processed
              else
                prev_frag_no++;
            } else {
              // NACK
              if (sendto(sockfd, NACK_RES, strlen(NACK_RES), 0, (struct sockaddr*)&client_addr, sizeof(server_addr) == -1)) {
                printf("Failed to nack the client packet is not accepted\n");
                continue; // Continue listening for packet
              }
            }

            // Add frag no to recevied_nos
            received_nos[p_re.frag_no] = 1;
            // Write to file
            long int offset = p_re.frag_no * DATA_LEN * sizeof(char);
            fseek(fp, offset, SEEK_SET);
            fwrite(p_re.filedata, sizeof(char), p_re.size, fp);
            // Free packet memory
            free(p_re.filename);
            // Break if it's the last packet
            if (p_re.frag_no + 1 == p_re.total_frag) break;
          }
        }
        printf("file has been saved as %s\n", filepath);
        fclose(fp);
      }
    } else {
      // Handle the case when first ack does not start with "ftp"
      if ((sendto(sockfd, OTHER_RES, strlen(OTHER_RES), 0, (struct sockaddr*)&client_addr, sizeof(server_addr))) == -1) {
        printf("Failed to send message back to client\n");
      } else {
        printf("Successful send message back to client\n");
      }
    }
  }
  close(sockfd);
  return 0;
}

// [WARNING] Does not resist mal formated packet
void parse_packet(const char* buf, struct packet* p) {
  char* strs[4];
  int start_i = 0;
  int field_count = 0;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    if (buf[i] == ':') {
      int len = i - start_i;
      strs[field_count] = malloc(sizeof(char) * len);
      memcpy(strs[field_count], buf + start_i, len);
      start_i = i + 1; // the char after ":"
      field_count++;
      if (field_count == 4) break; // there should only be 4 colons according to packet structure
    }
  }

  p->total_frag = atoi(strs[0]);
  p->frag_no = atoi(strs[1]);
  p->size = atoi(strs[2]);
  for (int i = 0; i < 3; i++) free(strs[i]); // free memory
  p->filename = strs[3];

  memcpy(&(p->filedata), buf + start_i, p->size);
}
