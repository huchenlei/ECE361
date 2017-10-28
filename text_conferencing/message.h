#ifndef MESSAGE_H
#define MESSAGE_H

#define MAX_NAME 256
#define MAX_DATA 2048

struct lab3message {
  unsigned int type;
  unsigned int size;
  unsigned char source[MAX_NAME];
  unsigned char data[MAX_DATA];
};

#define LOGIN 1
#define LO_ACK 2
#define LO_NAK 3
#define EXIT 4
#define JOIN 5
#define JN_ACK 6
#define JN_NAK 7
#define LEAVE_SESS 8
#define NEW_SESS 9
#define NS_ACK 10
#define MESSAGE 11
#define QUERY 12
#define QU_ACK 13

#endif
