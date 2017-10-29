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

#include "users.h"
#include "message.h"
#include "client_menu.h"

#define DEBUG

// Global variables
struct user* cur_user = NULL; // current user
char* cur_session = NULL; // current session id
int client_sock; // current socket

#define LOGIN_CHECK                                             \
  if(!isloggedin())                                             \
    { printf("Not yet logged in; Please login first\n"); }      \
  else

int menu() {
  int err = 0;
  char session_id[MAX_FIELD];
  char command[MAX_COMMAND_LEN];
  scanf("%s", command);

  if (strcmp(command, "/login") == 0) {
    if (!isloggedin()) {
      char name[MAX_NAME];
      char pass[MAX_PASS];
      char server_ip[MAX_FIELD];
      char server_port[MAX_FIELD];
      scanf(" %s %s %s %s", name, pass, server_ip, server_port);
      err = login(name, pass, server_ip, server_port);
    } else {
      printf("Already logged in as %s\n", cur_user->name);
    }
  } else if (strcmp(command, "/logout") == 0) {
    LOGIN_CHECK {
      err = logout();
    }
  } else if (strcmp(command, "/joinsession") == 0) {
    LOGIN_CHECK {
      scanf(" %s", session_id);
      err = join_session(session_id);
    }
  } else if (strcmp(command, "/leavesession") == 0) {
    LOGIN_CHECK {
      err = leave_session();
    }
  } else if (strcmp(command, "/createsession") == 0) {
    LOGIN_CHECK {
      scanf(" %s", session_id);
      err = create_session(session_id);
    }
  } else if (strcmp(command, "/list") == 0) {
    LOGIN_CHECK{
      err = list();
    }
  } else if (strcmp(command, "/quit") == 0) {
    err = quit();
  } else {
    LOGIN_CHECK {
      // get all text in terminl and send
      char msg_buf[MAX_DATA];
      strcpy(msg_buf, command);
      int offset = strlen(command);
      fgets(msg_buf + offset, MAX_DATA - offset, stdin);
      err = send_message(msg_buf);
    }
  }
  return err;
}

int isloggedin() {
  return cur_user != NULL;
}

int request(message_t type, const char* source, const char* data) {
  char msg_buf[sizeof(struct message)];
  bzero(msg_buf, sizeof(struct message));
  sprintf(msg_buf, "%d:%lu:%s%s", type, strlen(data), source, data);
#ifdef DEBUG
  printf("message buffer:\n%s\n", msg_buf);
#endif
  int err = send(client_sock, msg_buf, strlen(msg_buf), 0);
  if (err) {
    printf("Failed to send request: %s\n", msg_buf);
  }
  return err;
}

int recv_ack(message_t ack_type, message_t nak_type, int* retval, char* body) {
  char msg_buf[sizeof(struct message)];
  int err = recv(client_sock, msg_buf, sizeof(struct message), 0);
  if (err) {
    printf("Failed receiving ack/nak!...\n");
    return err;
  }
  char* iter = msg_buf;
  size_t type_len = 0;
  while (*iter != ':') {
    type_len++;
    iter++;
  }
  char type_str[type_len + 1]; // + 1 for '\0'
  strncpy(type_str, msg_buf, type_len);
  message_t msg_type = atoi(type_str);
  if (msg_type == ack_type) {
    *retval = 1;
    body = NULL;
  } else if (msg_type == nak_type) {
    *retval = 0;
    body = malloc(sizeof(char) * (strlen(msg_buf) + 1));
    strcpy(body, msg_buf);
  } else {
    printf("unexpected ack/nak type %d\n", msg_type);
    return 1;
  }
  return 0;
}

int login(const char* name, const char* pass, const char* server_ip, const char* server_port) {
  // Create the socket
  struct addrinfo hints, *servinfo, *p;
  int rv;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if ((rv = getaddrinfo(server_ip, server_port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((client_sock = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }
    if (connect(client_sock, p->ai_addr, p->ai_addrlen) == -1) {
      close(client_sock);
      perror("client: connect");
      continue;
    }
    break;
  }
  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }
  // Connected, now send the message
  request(LOGIN, name, pass);

  // Receive LO_ACK or LO_NAK from server
  int isack;
  char* result = NULL;
  recv_ack(LO_ACK, LO_NAK, &isack, result);
  if (!isack) {
    printf("Login failed: %s\n", result);
    return 3;
  }
  cur_user = malloc(sizeof(struct user));
  strcpy(cur_user->name, name);
  strcpy(cur_user->pass, pass);
  return 0;
}

int logout() {
  int err = request(EXIT, cur_user->name, "");
  close(client_sock);
  client_sock = -1;
  if (err) {
    printf("Failed to logout\n");
  } else {
    printf("Logged out%s\n", cur_user->name);
    free(cur_user);
    cur_user = NULL;
  }
  return err;
}

int join_session(const char* session_id) {
  assert(strlen(session_id) < MAX_FIELD);
  int err = request(JOIN, cur_user->name, session_id);
  if (err) return err;
  int isack;
  char* result = NULL;
  err = recv_ack(JN_ACK, JN_NAK, &isack, result);
  if (err) return err;
  if(isack) {
    printf("Successfully joined session %s\n", session_id);
    cur_session = malloc((strlen(session_id) + 1) * sizeof(char));
    strcpy(cur_session, session_id);
    return 0;
  } else {
    printf("failed to join session: %s\n", result);
    return 1;
  }
}

int leave_session() {
  int err = request(LEAVE_SESS, cur_user->name, cur_session);
  if (err) return err;
  printf("Leave session %s\n", cur_session);
  free(cur_session);
  cur_session = NULL;
  return 0;
}

int create_session(const char* session_id) {
  int err = request(NEW_SESS, cur_user->name, session_id);
  if (err) return err;
  int isack;
  char* result = NULL;
  err = recv_ack(NS_ACK, UNKNOWN, &isack, result);
  if (err) {
    printf("Failed to create session\n");
  } else {
    printf("Successfully created session %s\n", result);
  }
  return err;
}

int list() {
  int err = request(QUERY, "", "");
  if (err) return err;
  int isack;
  char* result = NULL;
  err = recv_ack(QU_ACK, UNKNOWN, &isack, result);
  if (err) {
    printf("Failed to list sessions\n");
  } else {
    // TODO format the list string
    printf("Sessions: %s\n", result);
  }
  return err;
}

int quit() {
  int err = request(EXIT, "", "");
  if (err) return err;
  printf("\nQuiting Text Conferencing Pro v1.0\n");
  close(client_sock);
  exit(0);
}

int send_message(const char* text) {
  if (cur_session == NULL) {
    printf("Please join a session first\n");
    return 1;
  }
  int err = request(MESSAGE, cur_user->name, text);
  if (err) return err;
  printf("message sent...\n");
  return 0;
}
