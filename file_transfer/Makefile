default: server deliver cleanDir
# compile server program
server: server.c
	gcc server.c -o server
# compile the client program
deliver: deliver.c
	gcc deliver.c -lm -o deliver
# clean the reciving dir
cleanDir:
	rm -rf ./recv
	mkdir recv
clean:
	rm deliver server
