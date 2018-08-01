#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<limits.h>
#include"application_protocol.h"

int main() {
	char *buf = calloc(PIPE_BUF, sizeof(char));
	int in_fd, out_fd;
	int retval;

	if(send_hello(&in_fd, &out_fd) != 0) {
		return 1;
	}

	write(out_fd, "AT+CCID\r\n", 9);
	read(in_fd, buf, PIPE_BUF);
	printf("Returned:  %s\n", buf);

	return 0;
}
