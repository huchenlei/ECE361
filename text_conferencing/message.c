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

#define NUM_COL 4

int parse_message(const char* buf, struct message* m) {
  char* strs[NUM_COL];
  int start_i = 0;
  int field_count = 0;
  for (size_t i = 0; i < sizeof(struct message); i++) {
    if (buf[i] == ':') {
      int len = i - start_i;
      strs[field_count] = malloc(sizeof(char) * len + 1);
      strncpy(strs[field_count], buf + start_i, len);
      strs[field_count][len] = '\0';
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
  strcpy(m->session_id, strs[3]);
  strncpy(m->data, buf + start_i, m->size);
  m->data[m->size] = '\0';
#ifdef DEBUG
  printf("parsing message as: %d %d ", m->type, m->size);
  printf("%s ", m->source);
  printf("%s ", m->session_id);
  printf("%s\n", m->data);
#endif
  for (int i = 0; i < NUM_COL; i++)
    free(strs[i]); // free memory
  return 0;
}

int response(int sockfd, message_t type, const char* data) {
    return send_through(sockfd, type, "Server", "Server", data);
}

int send_through(int sockfd, message_t type, const char* source, const char* session_id, const char* data) {
  assert(data != NULL);
  assert(session_id != NULL);
  assert(source != NULL);
  assert(sockfd > 0);
  char msg_buf[MAX_MESSAGE];
  bzero(msg_buf, MAX_MESSAGE);
  snprintf(msg_buf, MAX_MESSAGE, "%d:%lu:%s:%s:%s", type, strlen(data), source, session_id, data);

#ifdef DEBUG
  printf("[send through] %s \n", msg_buf);
#endif
  
  int err = send(sockfd, msg_buf, strlen(msg_buf) + 1, 0);
  if (err == -1) {
    printf("[send through] Failed to send message: %s\n", msg_buf);
    return 1;
  }
  return 0;
}

void print_message(const struct message* m) {
  printf("[%s] %s says: %s\n", m->session_id, m->source, m->data);
}
