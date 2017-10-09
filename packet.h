#define ACK_RES "ACK"
#define NACK_RES "NACK"
#define DATA_LEN 1000
/* Defines the packet for file fransfer */
struct packet {
  unsigned int total_frag;
  unsigned int frag_no; // Start from 0
  unsigned int size;
  char* filename;
  char filedata[DATA_LEN];
};
