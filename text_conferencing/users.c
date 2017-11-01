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
  {.name = "Chenlei", .pass = "chenlei"},
  {.name = "Alex", .pass = "alex"}
};

void _user_join_session(struct user* user, struct session* s) {
  if (user->joined_sessions[s->sid] == NULL) {
    user->joined_sessions[s->sid] = s;
    user->cur_session = s;
  } else {
    printf("[Fatal] User %s already joined session %s\n", user->name, s->session_id);
  }
}

void init_users() {
  for (size_t i = 0; i < USER_NUM; i++) {
    users[i].active = 0;
    users[i].sockfd = -1;
    users[i].cur_session = NULL;
    bzero(users[i].joined_sessions, MAX_JOINED_SESSION);
  }
}

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
  assert(user != NULL);
  for (size_t i = 0; i < MAX_JOINED_SESSION; i++) {
    if (user->joined_sessions[i] != NULL) { // remove user in all sessions joined
      user_leave_session(user, user->joined_sessions[i]);
    }
  }
  user->active = 0;
  FD_CLR(user->sockfd, &server_fds);
  close(user->sockfd);
  user->sockfd = -1;
  user->cur_session = NULL;
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
  _user_join_session(user, s);
  response(user->sockfd, JN_ACK, s->session_id);
  return 0;
}

int user_leave_session(struct user* user, struct session* s) {
  if (s == NULL) {
    response(user->sockfd, MESSAGE, "[Server] Session does not exist");
    return 1;
  }
  if (user->joined_sessions[s->sid] == NULL) {
    response(user->sockfd, MESSAGE, "[Server] Not in the session specified");
    return 1;
  }
  size_t cur_sid = s->sid;
  char buf_all[MAX_DATA];
  char buf_user[MAX_DATA];
  snprintf(buf_all, MAX_DATA, "%s has left session %s", user->name, s->session_id);
  // send response to user
  snprintf(buf_user, MAX_DATA, "[Server] leaving session %s...", s->session_id);

  // remove user in session
  if (session_remove_user(s, user)) {
    response(user->sockfd, MESSAGE, "500 server error!");
    return 1; // remove failed(should never happen)
  }
  // remove session in user
  user->joined_sessions[cur_sid] = NULL;
  if (user->cur_session == s) {
      user->cur_session = NULL;
  }
  if (sessions[cur_sid] != NULL) {
      // boardcast to users left in current session if the session still exist
      // after the user quit the session
      session_send(sessions[cur_sid], buf_all);
  }
  response(user->sockfd, MESSAGE, buf_user);

  user->joined_sessions[cur_sid] = NULL;
  return 0;
}

int user_send_msg(struct user* user, struct session* s, const char* msg) {
  assert(user != NULL);
  if (user->joined_sessions[s->sid] == NULL) {
    response(user->sockfd, MESSAGE, "[Server] Please join that session first");
    return 1;
  }
  if (s == NULL) {
    response(user->sockfd, MESSAGE, "[Server] Session does not exist");
    return 1;
  }
  return session_send(s, msg);
}

// TODO
int user_switch_session(struct user* user, struct session* s) {
  assert(user != NULL);
  if (user->joined_sessions[s->sid] != NULL) {
    user->cur_session = user->joined_sessions[s->sid];
    response(user->sockfd, MESSAGE, "[Server] Successfully switch");
  }
}
