foneserver: foneserver.c
	gcc foneserver.c -o server.out -lpthread

client_status: client_status.c
	gcc client_status.c -o status.out
