CC = gcc
CFLAGS=-g -c
LIBS=-lm -lpthread
SRCS=server.c client.c users.c
OBJS=$(SRCS:.c=.o)

all:
	server client

server: server.o users.o
	$(CC) -o server server.o users.o $(LIBS)

client: client.o
	$(CC) -o client client.o  $(LIBS)

$(OBJS):$(SRCS)
	$(CC) $(CFLAGS) $(SRCS)

clean:
	rm -f *.o server client
