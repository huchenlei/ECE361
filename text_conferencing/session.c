#include "session.h"
#include "users.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

fd_set server_fds;
struct session* sessions[MAX_SESSION];

int new_session(const char* session_id, struct user* creator) {
  for (size_t i = 0; i < MAX_SESSION; i++) {
    if (sessions[i] == NULL) {
      sessions[i] = malloc(sizeof(struct session));
      (sessions[i])->sid = i;
      (sessions[i])->user_num = 0;
      (sessions[i])->creator = creator;
      bzero(sessions[i]->session_id, MAX_SESSION_ID);
      bzero(sessions[i]->users, MAX_SESSION_ID);
      strncpy(sessions[i]->session_id, session_id, MAX_SESSION_ID);
      // Add creator to session list
      sessions[i]->users[0] = creator;
      sessions[i]->user_num++;

      _user_join_session(creator, sessions[i]);
      printf("User %s create new session %s\n", creator->name, sessions[i]->session_id);
      return response(creator->sockfd, NS_ACK, sessions[i]->session_id);
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

int session_send(struct session* s, const char* source, const char* msg) {
  for (size_t i = 0; i < MAX_USER_SESSION; ++i) {
    if (s->users[i] != NULL) {
      send_through(s->users[i]->sockfd, MESSAGE, source, s->session_id, msg);
    }
  }
  return 0;
}

int session_remove_user(struct session* s, struct user* user) {
  int err = 0;
  assert(user->joined_sessions[s->sid] == s);
  for (size_t i = 0; i < MAX_USER_SESSION; i++) {
    if (s->users[i] == user) {
      s->users[i] = NULL;
      s->user_num--;
      if (s->user_num == 0) {
        // Destory session when there are no user in it
        err = session_destory(s);
      }
      return err;
    }
  }
  return 1;
}

int session_destory(struct session* s) {
  if (s->user_num != 0) {
    printf("Illegal deletion of session that has more than one user\n");
    return 1; // TODO user will be able to delete session
  } // illegal destory when there are user in the session
  assert(sessions[s->sid] != NULL);
  sessions[s->sid] = NULL;
  free(s);
  return 0;
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
