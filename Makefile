default: server deliver
# compile server program
server: server.c
	gcc server.c -o server
# compile the client program
deliver: deliver.c
	gcc deliver.c -lm -o deliver

clean:
	rm deliver server
