#ifndef SESSION_H
#define SESSION_H

#define MAX_SESSION 10

struct session {
  int sockfd; // connection between server and client
  int user_id;
  int session_id;
  struct session *prev;
  struct session *next;
};

extern struct session* sessions[MAX_SESSION];

#endif
