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
// [WARNING] Does not resist mal formated packet
void parse_message(const char* buf, struct message* m) {
  char* strs[4];
  int start_i = 0;
  int field_count = 0;
  for (size_t i = 0; i < sizeof(struct message); i++) {
    if (buf[i] == ':') {
      int len = i - start_i;
      strs[field_count] = malloc(sizeof(char) * len);
      strncpy(strs[field_count], buf + start_i, len);
      start_i = i + 1; // the char after ":"
      field_count++;
      if (field_count == 4) break; // there should only be 4 colons according to packet structure
    }
  }

  m->type = atoi(strs[0]);
  m->size = atoi(strs[1]);
  strcpy(m->source, strs[2]);
  strncpy(m->data, buf + start_i, m->size);
  for (int i = 0; i < 4; i++)
    free(strs[i]); // free memory
}

int response(int sockfd, message_t type, const char* data) {
  char buf[MAX_MESSAGE];
  sprintf(buf, "%d:%s", type, data);
  int err = send(sockfd, buf, strlen(buf), 0);
  if (err) {
    printf("response: failed to response to client sockfd%d\n", sockfd);
  }
  return err;
}
