CC = gcc
CFLAGS=-g -c
LIBS=-lm -lpthread
SRCS=server.c client.c users.c client_menu.c message.c
OBJS=$(SRCS:.c=.o)

all:
	server client

server: server.o users.o message.o
	$(CC) -o server server.o users.o message.o $(LIBS)

client: client.o users.o client_menu.o
	$(CC) -o client client.o users.o client_menu.o $(LIBS)

$(OBJS):$(SRCS)
	$(CC) $(CFLAGS) $(SRCS)

clean:
	rm -f *.o server client
