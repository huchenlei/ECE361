#include "session.h"
#include "users.h"
#include <assert.h>

fd_set server_fds;

int session_is_full(struct session* s) {
  return s->user_num == MAX_USER_SESSION;
}

int session_send(struct session* s, const char* msg) {

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
