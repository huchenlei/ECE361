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
// whether user in a session
int is_in_session = 0;
char cur_session[MAX_SESSION_ID];
int client_sock = -1; // current socket

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
      scanf(" %s", session_id);
      err = leave_session(session_id);
    }
  } else if (strcmp(command, "/createsession") == 0) {
    LOGIN_CHECK {
      scanf(" %s", session_id);
      err = create_session(session_id);
    }
  } else if (strcmp(command, "/switchsession") == 0) {
    LOGIN_CHECK {
      scanf(" %s", session_id);
      err = switch_session(session_id);
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

int request(message_t type, const char* source, const char* session_id, const char* data) {
  return send_through(client_sock, type, source, session_id, data);
}

// Remember to free body since it's malloced
int recv_ack(message_t ack_type, message_t nak_type, int* retval, char** body) {
  char msg_buf[sizeof(struct message)];
  int err = recv(client_sock, msg_buf, sizeof(struct message), 0);
  if (err == -1) {
    printf("Failed receiving ack/nak!...\n");
    return 1;
  }
  struct message m;
  parse_message(msg_buf, &m);

  *body = malloc(sizeof(char) * (m.size + 1));
  strcpy(*body, m.data);

  if (m.type == ack_type) {
    *retval = 1;
  } else if (m.type == nak_type) {
    *retval = 0;
  } else {
    printf("unexpected ack/nak type %d\n", m.type);
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
  request(LOGIN, name, "", pass);

  // Receive LO_ACK or LO_NAK from server
  int isack;
  char* result = NULL;
  recv_ack(LO_ACK, LO_NAK, &isack, &result);
  if (!isack) {
    printf("Login failed: %s\n", result);
    return 3;
  }
  free(result);
  cur_user = malloc(sizeof(struct user));
  strcpy(cur_user->name, name);
  strcpy(cur_user->pass, pass);
  printf("Successfully loggged in as %s\n", cur_user->name);
  return 0;
}

int logout() {
  int err = request(EXIT, cur_user->name, "", "");
  close(client_sock);
  client_sock = -1;
  if (err) {
    printf("Failed to logout\n");
  } else {
    printf("Logged out %s\n", cur_user->name);
    free(cur_user);
    cur_user = NULL;
  }
  return err;
}

int join_session(const char* session_id) {
  assert(strlen(session_id) < MAX_FIELD);
  int err = request(JOIN, cur_user->name, session_id, "");
  if (err) return err;
  int isack;
  char* result = NULL;
  err = recv_ack(JN_ACK, JN_NAK, &isack, &result);
  if (err) {
    free(result);
    return err;
  }
  if(isack) {
    strncpy(cur_session, session_id, MAX_SESSION_ID);
    is_in_session = 1;
    printf("Successfully joined session %s\n", cur_session);
    err = 0;
  } else {
    printf("failed to join session: %s\n", result);
    err = 1;
  }
  free(result);
  return err;
}

int leave_session(const char* session_id) {
  assert(session_id != NULL);
  int err = request(LEAVE_SESS, cur_user->name, session_id, "");
  if (err) return err;
  printf("Leave session %s\n", session_id);
  if (strcmp(cur_session, session_id) == 0) {
    is_in_session = 0;
  }
  return 0;
}

int create_session(const char* session_id) {
  int err = request(NEW_SESS, cur_user->name, session_id, "");
  if (err) return err;
  int isack;
  char* result = NULL;
  err = recv_ack(NS_ACK, UNKNOWN, &isack, &result);
  if (err) {
    printf("Failed to create session\n");
  } else {
    strncpy(cur_session, session_id, MAX_SESSION_ID);
    is_in_session = 1;
    printf("Successfully created session %s\n", result);
  }
  free(result);
  return err;
}

int switch_session(const char* session_id) {
  int err = request(SW_SESS, cur_user->name, session_id, "");
  if (err) return err;
  int isack;
  char* result = NULL;
  err = recv_ack(SW_ACK, UNKNOWN, &isack, &result);
  if (err) {
    printf("Failed to switch session %s\n", result);
  } else {
    is_in_session = 1;
    strncpy(cur_session, session_id, MAX_SESSION_ID);
    printf("Successfully switching to %s\n", session_id);
  }
  free(result);
  return err;
}

int list() {
  int err = request(QUERY, "", "", "");
  if (err) return err;
  int isack;
  char* result = NULL;
  err = recv_ack(QU_ACK, UNKNOWN, &isack, &result);
  if (err) {
    printf("Failed to list sessions\n");
  } else {
    // TODO format the list string
    printf("Sessions: %s\n", result);
  }
  free(result);
  return err;
}

int quit() {
  int err = 0;
  if (cur_user != NULL) {
    err = logout();
  }
  if (err) {
    printf("failed to quit\n");
    return err;
  }
  printf("\nQuiting Text Conferencing Pro v1.0\n");
  exit(0);
}

int send_message(const char* text) {
  if (!is_in_session) {
    printf("Please join a session first\n");
    return 1;
  }
  int err = request(MESSAGE, cur_user->name, cur_session, text);
  if (err) return err;
  printf("message sent...\n");
  return 0;
}
