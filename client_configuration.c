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

	/* Disable Echo  */
	write(out_fd, "ATE 0\r\n", 7);
	sleep(1);
	read(in_fd, buf, PIPE_BUF);
	printf("Turning Echo off:  %s\n", buf);
	memset(buf, 0, PIPE_BUF);

	/* Enable Caller ID  */
	write(out_fd, "AT+CLIP=1\r\n", 11);
	sleep(1);
	read(in_fd, buf, PIPE_BUF);
	printf("Enabling Caller ID:  %s\n", buf);
	memset(buf, 0, PIPE_BUF);

	/* Set volume = 20  */
	write(out_fd, "AT+CLVL=20\r\n", 12);
	sleep(1);
	read(in_fd, buf, PIPE_BUF);
	printf("Setting speaker volume:  %s\n", buf);
	memset(buf, 0, PIPE_BUF);

	/* Disable SMS notifications  */
	write(out_fd, "AT+CNMI=2,0\r\n", 13);
	sleep(1);
	read(in_fd, buf, PIPE_BUF);
	printf("Disable notifications on SMS events:  %s\n", buf);
	memset(buf, 0, PIPE_BUF);

	free(buf);
	return send_finish(&in_fd, &out_fd);
}
