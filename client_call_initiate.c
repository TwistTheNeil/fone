#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<limits.h>
#include<pthread.h>
#include<signal.h>
#include"client_protocol.h"

int in_fd, out_fd;
int ongoing_call = 0;
static pthread_t thread_subscription;

void *listen_for_subscription(void *arg);
void hangup_sender();

void hangup_sender() {
	write(out_fd, "ATH\r\n", 5);
	ongoing_call = 2;
	pthread_cancel(thread_subscription);
}

void *listen_for_subscription(void *arg) {
	char *buf = calloc(PIPE_BUF, sizeof(char));
	int in_fd, out_fd;

	/* Subscribe to "NO CARRIER" */
	if(send_subscribe(&in_fd, &out_fd) != 0) {
		fprintf(stderr, "[Fatal] Unable to subscribe to serial\n");
		return NULL;
	}
	write(out_fd, "NO CARRIER", 10);

	while(ongoing_call == 0 || ongoing_call == 1) {
		if(read(in_fd, buf, PIPE_BUF) == 0) {
			fprintf(stderr, "[INFO] Lost connection with server\n");
			break;
		}
		if(strstr(buf, "NO CARRIER") != NULL) {
			printf("call dropped\n");
			ongoing_call = 2;
			break;
		}
		memset(buf, 0, PIPE_BUF);
		sleep(1);
	}
	free(buf);
	send_finish(&in_fd, &out_fd);
	return NULL;
}

int main(int argc, char **argv) {
	char *serial_cmd = calloc(17, sizeof(char));
	char *buf = calloc(PIPE_BUF, sizeof(char));

	if(argc != 2) {
		printf("Usage: %s <phone number>\n", argv[0]);
		return 1;
	}

	signal(SIGINT, hangup_sender);

	init_fone_client();

	if(pthread_create(&thread_subscription, NULL, listen_for_subscription, NULL)) {
		fprintf(stderr, "[Fatal] Cannot create pthread for listening\n");
		return 1;
	}

	sleep(1);

	if(send_hello(&in_fd, &out_fd) != 0) {
		fprintf(stderr, "[Fatal] Can't connect to server\n");
	} else {
		write(out_fd, "AT+CMGF=0\r\n", 11);
		sleep(1);
		read(in_fd, buf, PIPE_BUF);
		if(strstr(buf, "OK") != NULL) {
			memset(buf, 0, PIPE_BUF);
			snprintf(serial_cmd, 16, "ATD%s;\r\n", argv[1]);
			write(out_fd, serial_cmd, strlen(serial_cmd));
			sleep(1);
			read(in_fd, buf, PIPE_BUF);
			if(strstr(buf, "OK") != NULL) {
				ongoing_call = 1;
			}
		} else {
			fprintf(stderr, "[Error] Couldn't set PDU mode\n");
			pthread_cancel(thread_subscription);
		}
	}

	pthread_join(thread_subscription, NULL);
	return send_finish(&in_fd, &out_fd);
}
