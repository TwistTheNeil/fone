#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<limits.h>
#include<pthread.h>
#include<signal.h>
#include"client_protocol.h"

static int ringing = 0;
static int in_fd, out_fd;
static int listening = 0;
static int ongoing_call = 0;
static int quit_called = 0;
static pthread_t thread_subscription;

void *listen_for_subscription(void *arg);
void *answer_call(void *arg);
void hangup_call();
void quit();

void quit() {
	printf("[Info] Stopping client (any ongoing calls will continue)\n");
	pthread_cancel(thread_subscription);
	quit_called = 1;
}

void *answer_call(void *arg) {
	char *buf = calloc(PIPE_BUF, sizeof(char));
	int in_fd, out_fd;

	if(send_hello(&in_fd, &out_fd) != 0) {
		fprintf(stderr, "[Error] Failed to converse with server\n");
		return NULL;
	}

	write(out_fd, "ATA\r\n", 5);
	read(in_fd, buf, PIPE_BUF);
	printf("%s\n", buf);

	free(buf);
	send_finish(&in_fd, &out_fd);
}

void hangup_call() {
	char *buf = calloc(PIPE_BUF, sizeof(char));
	int in_fd, out_fd;

	if(send_hello(&in_fd, &out_fd) != 0) {
		fprintf(stderr, "[Error] Failed to converse with server\n");
		return;
	}

	write(out_fd, "ATH\r\n", 5);
	read(in_fd, buf, PIPE_BUF);
	printf("%s\n", buf);

	ongoing_call = 0;
	ringing = 0;

	free(buf);
	send_finish(&in_fd, &out_fd);
}

void *listen_for_subscription(void *arg) {
	char *buf = calloc(PIPE_BUF, sizeof(char));
	char *found;
	char *tofree = buf;

	/* Subscribe to "NO CARRIER", and "CLIP" */
	if(send_subscribe(&in_fd, &out_fd) != 0) {
		fprintf(stderr, "[Fatal] Unable to subscribe to serial\n");
		return NULL;
	}
	write(out_fd, "NO CARRIER", 10);
	sleep(1);
	write(out_fd, "CLIP", 4);

	while(1) {
		listening = 1;
		if(read(in_fd, buf, PIPE_BUF) == 0) {
			fprintf(stderr, "[INFO] Lost connection with server\n");
			break;
		}
		if(strstr(buf, "NO CARRIER") != NULL) {
			printf("call dropped\n");
			ongoing_call = 0;
			ringing = 0;
		}
		if(strstr(buf, "CLIP") != NULL && ringing != 1) {
			ringing = 1;
			found = strsep(&buf, "\"");
			if(found == NULL) {
				memset(tofree, 0, PIPE_BUF);
				continue;
			}
			found = strsep(&buf, "\"");
			if(found != NULL) {
				printf("%s is calling..\n", found);
			}
		}
		memset(tofree, 0, PIPE_BUF);
	}
	listening = 0;
	free(tofree);
}

int main() {
	size_t response_size = 2;
	char *response = NULL;
	pthread_t thread_answer_call;

	signal(SIGINT, hangup_call);
	signal(SIGQUIT, quit);

	init_fone_client();

	if(pthread_create(&thread_subscription, NULL, listen_for_subscription, NULL)) {
		fprintf(stderr, "[Fatal] Cannot create pthread for listening\n");
		return 1;
	}

	while(!quit_called) {
		if(listening == 1 && ringing == 1 && ongoing_call == 0) {
			ongoing_call = 1;
			printf("Answer call? (y/n)\n> ");
			getline(&response, &response_size, stdin);
			if(response[0] == 'y') {
				printf("Answering call..\n");
				if(pthread_create(&thread_answer_call, NULL, answer_call, NULL)) {
					fprintf(stderr, "[Error] Couldn't answer call\n");
					continue;
				}
				pthread_join(thread_answer_call, NULL);
			} else {
				hangup_call();
			}
		}
		sleep(1);
	}

	pthread_join(thread_subscription, NULL);
	return send_finish(&in_fd, &out_fd);
}
