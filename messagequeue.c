#include<stdio.h>
#include<stdlib.h>
#include<string.h>

// TODO: Do i really need MessageQueue:length?

typedef enum State {
	IDLE,		// Not doing anything
	PROCESSING,	// Processing AT command
	WORKING		// Working with client
} State;

typedef enum msgtype {
	CMD,
	COM
} msgtype;

typedef struct Message {
	char *msg;
	int pipe_fd;
	struct Message *next;
} Message;

typedef struct MessageQueue {
	Message *head;
	Message *tail;
	Message *com_head;
	Message *com_tail;
	State state;
//	int cmd_length;
//	int com_length;
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
	mq.com_head = NULL;
	mq.com_tail = NULL;
	mq.state = IDLE;
//	mq.cmd_length = 0;
//	mq.com_length = 0;
}

void mq_push(char *cmd, msgtype mtype, int fd) {
	Message *new_msg = calloc(1, sizeof(Message));
	new_msg->msg = calloc(strlen(cmd) + 1, sizeof(char));
	strncpy(new_msg->msg, cmd, strlen(cmd));
	new_msg->msg[strlen(cmd)] = 0;

	new_msg->pipe_fd = fd;
	new_msg->next = NULL;

	if(mtype == CMD) {
		if(mq.head == NULL) {
			mq.head = new_msg;
			mq.tail = new_msg;
		} else {
			mq.tail->next = new_msg;
			mq.tail = new_msg;
		}
	} else if (mtype == COM) {
		if(mq.com_head == NULL) {
			mq.com_head = new_msg;
			mq.com_tail = new_msg;
		} else {
			mq.com_tail->next = new_msg;
			mq.com_tail = new_msg;
		}
	}
}

void mq_pop(msgtype mtype) {
	Message *popped;

	if(mtype == CMD) {
		if(mq.head != NULL) {
			popped = mq.head;
			mq.head = mq.head->next;

			free(popped->msg);
			free(popped);

			//mq.length--;
		} else {
			mq.head = NULL;
			mq.tail = NULL;
		}
	} else if (mtype == COM) {
		if(mq.com_head != NULL) {
			popped = mq.com_head;
			mq.com_head = mq.com_head->next;

			free(popped->msg);
			free(popped);

			//mq.length--;
		} else {
			mq.com_head = NULL;
			mq.com_tail = NULL;
		}
	}

}

void mq_cleanup() {
	while(mq.head != NULL) {
		mq_pop(CMD);
	}
	while(mq.com_head != NULL) {
		mq_pop(COM);
	}
}

void mq_print() {
	Message *i = mq.head;

	printf("state: %d\n", mq.state);
	printf("CMD Contents:\n");

	while(i != NULL) {
		printf("[%d] %s\n", i->pipe_fd, i->msg);
		i = i->next;
	}

	printf("COM Contents:\n");

	i = mq.com_head;
	while(i != NULL) {
		printf("[%d] %s\n", i->pipe_fd, i->msg);
		i = i->next;
	}
}
