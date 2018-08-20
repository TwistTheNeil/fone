#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef enum State {
	STARTED,
	FINISHED,
} State;

typedef struct Message {
	char *msg;
	int fs2a_fd;
	int fa2s_fd;
	struct Message *next;
} Message;

typedef struct MessageQueue {
	Message *head;
	Message *tail;
	Message *last; // Last command in the queue which was read
	State state;
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
	mq.last = NULL;
	mq.state = FINISHED;
}

void mq_push(char *cmd, int fs2a_fd, int fa2s_fd) {
	Message *new_msg = calloc(1, sizeof(Message));
	new_msg->msg = calloc(strlen(cmd) + 1, sizeof(char));
	strncpy(new_msg->msg, cmd, strlen(cmd));
	new_msg->msg[strlen(cmd)] = 0;

	new_msg->fs2a_fd = fs2a_fd;
	new_msg->fa2s_fd = fa2s_fd;
	new_msg->next = NULL;

	if(mq.head == NULL) {
		mq.head = new_msg;
		mq.tail = new_msg;
	} else {
		mq.tail->next = new_msg;
		mq.tail = new_msg;
	}
}

void mq_pop() {
	Message *popped;

	if(mq.head != NULL) {
		popped = mq.head;
		mq.head = mq.head->next;

		free(popped->msg);
		free(popped);

	} else {
		mq.head = NULL;
		mq.tail = NULL;
		mq.last = NULL;
	}
}

void mq_cleanup() {
	while(mq.head != NULL) {
		mq_pop();
	}
	mq_init();
}

void mq_print() {
	Message *i = mq.head;

	printf("state: %d\n", mq.state);
	printf("Contents:\n");

	while(i != NULL) {
		printf("[%d, %d] %s\n", i->fs2a_fd, i->fa2s_fd, i->msg);
		i = i->next;
	}
}
