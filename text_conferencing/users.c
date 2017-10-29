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
#include "users.h"
#include "message.h"
#include "session.h"

#define DEBUG

struct user users[USER_NUM] = {
  {.name = "Chenlei", .pass = "chenlei", .cur_session = NULL, .sockfd = -1, .active = 0},
  {.name = "Alex", .pass = "alex", .cur_session = NULL, .sockfd = -1, .active = 0}
};

int auth_user(int sockfd) {
  struct timeval timeout = {.tv_sec = 10, .tv_usec = 0};
  int err = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  if (err) {
    printf("auth_user: failed to config socket\n");
    return err;
  }
  char buf[MAX_MESSAGE];
  err = recv(sockfd, buf, MAX_MESSAGE, 0);
  if (errno == EAGAIN) {
    // TODO handle timeout
    return 0;
  }
  struct message m;
  parse_message(buf, &m);
#ifdef DEBUG
  printf("recevicing: %s\n", buf);
  printf("name: %s\npass: %s\n", m.source, m.data);
#endif
  if (m.type == LOGIN) {
    for (size_t i = 0; i < USER_NUM; i++) {
      if ((strcmp(users[i].name, m.source) == 0) && (strcmp(users[i].pass, m.data) == 0)) {
        if (users[i].active) {
          response(sockfd, LO_NAK, "user already logged in");
          return 0;
        } else {
          response(sockfd, LO_ACK, "");
          users[i].active = 1;
          users[i].sockfd = sockfd;
          printf("User %s connect to server\n", m.source);
          return 1; // return true
        }
      }
    }
    response(sockfd, LO_NAK, "wrong username or password");
  } else {
    response(sockfd, LO_NAK, "wrong message type for login");
  }
  return 0; // return false
}

int logout_user(struct user* user) {
  if (user->cur_session != NULL) { // remove user in current session
    user_leave_session(user);
  }
  user->active = 0;
  FD_CLR(user->sockfd, &server_fds);
  close(user->sockfd);
  user->sockfd = -1;
  // TODO reset_max_sock
  // Might not need since it's done for each loop
  printf("Successfully logged out %s\n", user->name);
  return 0;
}

int user_join_session(struct user* user, struct session* s) {
  char msg[MAX_DATA];
  if (s == NULL) {
    sprintf(msg, "%s:session does not exist", s->session_id);
    response(user->sockfd, JN_NAK , msg);
    return 1;
  }
  if (session_is_full(s)) {
    sprintf(msg, "%s:session already full %d", s->session_id, MAX_USER_SESSION);
    response(user->sockfd, JN_NAK, msg);
    return 1;
  }
  int err = session_add_user(s, user);
  if (err) return err; // TODO handle other errors
  user->cur_session = s;
  response(user->sockfd, JN_ACK, s->session_id);
  return 0;
}

int user_leave_session(struct user* user) {
  if (user->cur_session == NULL) {
    response(user->sockfd, MESSAGE, "No in a session");
    return 1;
  }
  if (session_remove_user(user->cur_session, user)) {
    response(user->sockfd, MESSAGE, "500 server error!");
    return 1; // remove failed(should never happen)
  }

  char buf[MAX_DATA];
  sprintf(buf, "%s has left session %s", user->name, user->cur_session->session_id);
  session_send(user->cur_session, buf);
  user->cur_session = NULL;
  return 0;
}

int user_send_msg(struct user* user, const char* msg) {
  if (user->cur_session == NULL) {
    response(user->sockfd, MESSAGE, "[Server] Please join a session first");
    return 1;
  }
  return session_send(user->cur_session, msg);
}
