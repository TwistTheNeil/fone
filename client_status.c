#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<limits.h>
#include"client_protocol.h"

int main() {
	char *buf = calloc(PIPE_BUF, sizeof(char));
	int in_fd, out_fd;

	init_fone_client();

	if(send_hello(&in_fd, &out_fd) != 0) {
		return 1;
	}

	write(out_fd, "AT\r\n", 4);
	sleep(1);
	read(in_fd, buf, PIPE_BUF);
	printf("Returned:  %s\n", buf);

	free(buf);
	return send_finish(&in_fd, &out_fd);
}
