#include "session.h"
#include "users.h"
#include <assert.h>
#include <string.h>

fd_set server_fds;
struct session* sessions[MAX_SESSION];

int new_session(const char* session_id, struct user* creator) {
  for (size_t i = 0; i < MAX_SESSION; i++) {
    if (sessions[i] == NULL) {
      struct session* new_s = sessions[i];
      new_s = malloc(sizeof(struct session)); // TODO free session
      new_s->sid = i;
      new_s->user_num = 0;
      new_s->creator = creator;
      bzero(new_s->session_id, MAX_SESSION_ID);
      bzero(new_s->users, MAX_SESSION_ID);

      return response(creator->sockfd, NS_ACK, session_id);
    }
  }
  response(creator->sockfd, UNKNOWN, "[Server] max session num reached");
  return 1; // Max session reached
}

int session_is_full(struct session* s) {
  return s->user_num == MAX_USER_SESSION;
}

struct session* find_session(const char* session_id) {
  for (size_t i = 0; i < MAX_SESSION; i++) {
    if (sessions[i] != NULL &&
        (strcmp(sessions[i]->session_id, session_id) == 0)) {
      return sessions[i]; // true
    }
  }
  return NULL; // false
}

int session_send(struct session* s, const char* msg) {
  for (size_t i = 0; i < MAX_USER_SESSION; ++i) {
    if (s->users[i] != NULL) {
      response(s->users[i]->sockfd, MESSAGE, msg);
    }
  }
  return 0;
}

int session_remove_user(struct session* s, struct user* user) {
  assert(user->cur_session == s);
  for (size_t i = 0; i < MAX_USER_SESSION; i++) {
    if (s->users[i] == user) {
      s->users[i] = NULL;
      s->user_num--;
      return 0;
    }
  }
  return 1;
}

int session_add_user(struct session* s, struct user* user) {
  for (size_t i = 0; i < MAX_USER_SESSION; i++) {
    if (s->users[i] == NULL) {
      s->users[i] = user;
      s->user_num++;
      return 0;
    }
  }
  return 1; // Session full
}
