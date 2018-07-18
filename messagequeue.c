#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef enum State {
	IDLE,
	WORKING
} State;

typedef struct Message {
	char *msg;
	int pipe_fd;
	struct Message *next;
} Message;

typedef struct MessageQueue {
	Message *head;
	Message *tail;
	State state;
	int length;
} MessageQueue;

MessageQueue mq;

/*
 *
 * Helper Functions
 *
 */

void mq_init() {
	mq.head = NULL;
	mq.tail = NULL;
	mq.state = IDLE;
	mq.length = 0;
}

void mq_push(char *cmd, int fd) {
	// Cope the command
	Message *new_msg = calloc(1, sizeof(Message));
	new_msg->msg = calloc(strlen(cmd) + 1, sizeof(char));
	strncpy(new_msg->msg, cmd, strlen(cmd));
	new_msg->msg[strlen(cmd)] = 0;

	//Copy the pipe fd
	new_msg->pipe_fd = fd;

	new_msg->next = NULL;

	if(mq.head == NULL) {
		mq.head = new_msg;
		mq.tail = new_msg;
	} else {
		mq.tail->next = new_msg;
		mq.tail = new_msg;
	}

	mq.length++;
}

void mq_pop() {
	Message *popped;

	if(mq.head != NULL) {
		popped = mq.head;
		mq.head = mq.head->next;

		free(popped->msg);
		free(popped);

		mq.length--;
	} else {
		mq.head = NULL;
		mq.tail = NULL;
	}
}

void print_mq() {
	Message *i = mq.head;

	printf("state: %d\nlength: %d\n", mq.state, mq.length);
	printf("Contents:\n");


	while(i != NULL) {
		printf("[%d] %s\n", i->pipe_fd, i->msg);
		i = i->next;
	}
}
