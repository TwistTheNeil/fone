#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<limits.h>
#include"client_protocol.h"

int main(int argc, char **argv) {
	char *buf = calloc(PIPE_BUF, sizeof(char));
	char *cmdbuf;
	char *msgbuf;
	char ctrlz[4];
	int in_fd, out_fd;

	if(argc != 3) {
		printf("Usage: %s phone-number \"message\"\n", argv[0]);
		return 1;
	}

	/* Construct AT command to initiate and send sms */
	cmdbuf = calloc(9+strlen(argv[1])+3+1, sizeof(char));
	sprintf(cmdbuf, "AT+CMGS=\"%s\"\r\n", argv[1]);

	/* Construct message how fona expects */
	msgbuf = calloc(strlen(argv[2])+2+1, sizeof(char));
	sprintf(msgbuf, "%s\r\n", argv[2]);

	/* We need ctrl+z character to signal the end of a message */
	ctrlz[0] = 0x1A;
	ctrlz[1] = '\r';
	ctrlz[2] = '\n';
	ctrlz[3] = 0;

	init_fone_client();

	if(send_hello(&in_fd, &out_fd) != 0) {
		return 1;
	}

	write(out_fd, "AT+CMGF=1\r\n", 11);
	read(in_fd, buf, PIPE_BUF);
	if(strstr(buf, "OK") == NULL) {
		fprintf(stderr, "[Error] Couldn't set text mode\n");
		send_finish(&in_fd, &out_fd);
		return 1;
	}

	memset(buf, 0, PIPE_BUF);

	write(out_fd, cmdbuf, strlen(cmdbuf));
	free(cmdbuf);
	read(in_fd, buf, PIPE_BUF);
	if(strncmp(buf, "\r\n>", 3) == 0) {
		memset(buf, 0, PIPE_BUF);
		write(out_fd, msgbuf, strlen(msgbuf));
		write(out_fd, ctrlz, strlen(ctrlz));
		read(in_fd, buf, PIPE_BUF);
		printf("%s\n", buf);
	} else {
		fprintf(stderr, "[Info] Wasn't prompted for message input. Exiting.\n");
		send_finish(&in_fd, &out_fd);
		return 1;
	}
	free(msgbuf);

	/* Set PDU mode */
	memset(buf, 0, PIPE_BUF);
	write(out_fd, "AT+CMGF=0\r\n", 11);
	read(in_fd, buf, PIPE_BUF);
	if(strstr(buf, "OK") == NULL) {
		fprintf(stderr, "[Error] Couldn't set PDU mode\n");
		send_finish(&in_fd, &out_fd);
		return 1;
	}

	free(buf);
	return send_finish(&in_fd, &out_fd);
}
