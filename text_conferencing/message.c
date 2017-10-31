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
#include <assert.h>

#include "message.h"

#define DEBUG

#define NUM_COL 3
// [WARNING] Does not resist mal formated packet
int parse_message(const char* buf, struct message* m) {
  char* strs[NUM_COL];
  int start_i = 0;
  int field_count = 0;
  for (size_t i = 0; i < sizeof(struct message); i++) {
    if (buf[i] == ':') {
      int len = i - start_i;
      strs[field_count] = malloc(sizeof(char) * len + 1);
      strncpy(strs[field_count], buf + start_i, len);
      start_i = i + 1; // the char after ":"
      field_count++;
      if (field_count == NUM_COL) break; // there should only be 3 colons according to packet structure
    }
  }
  if (field_count != NUM_COL) { // packet not in the right format
    printf("Mal-formatted packet %s\n", buf);
    return 1;
  }

  m->type = atoi(strs[0]);
  m->size = atoi(strs[1]);
  strcpy(m->source, strs[2]);
  strncpy(m->data, buf + start_i, m->size);
  m->data[m->size] = '\0';
#ifdef DEBUG
  printf("parsing message as: %d %d %s %s\n", m->type, m->size, m->source, m->data);
#endif
  for (int i = 0; i < NUM_COL; i++)
    free(strs[i]); // free memory
  return 0;
}

int response(int sockfd, message_t type, const char* data) {
  char buf[MAX_MESSAGE];
  sprintf(buf, "%d:%s", type, data);
  int err = send(sockfd, buf, strlen(buf) + 1, 0);
  if (err == -1) {
    printf("response: failed to response to client sockfd %d\n", sockfd);
    return 1;
  }
  return 0;
}
