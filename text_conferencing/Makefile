CC = gcc
#CFLAGS=-g -c -D DEBUG
CFLAGS=-g -c
LIBS=-lm -lpthread
SRCS=server.c client.c users.c client_menu.c message.c session.c
OBJS=$(SRCS:.c=.o)

default:
	make server
	make client

server: server.o users.o message.o session.o
	$(CC) -o server server.o users.o message.o session.o $(LIBS)

client: client.o users.o client_menu.o message.o session.o
	$(CC) -o client client.o users.o message.o client_menu.o session.o $(LIBS)

$(OBJS):$(SRCS)
	$(CC) $(CFLAGS) $(SRCS)

clean:
	rm -f *.o server client

test:
	make
	./server 5000
	# perl test.pl
