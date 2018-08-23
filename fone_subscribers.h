#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef struct Subscriber {
	char *keyword;
	int fs2a_fd;
	int fa2s_fd;
	struct Subscriber *next;
} Subscriber;

typedef struct SubscriberQueue {
	Subscriber *head;
	Subscriber *tail;
} SubscriberQueue;

SubscriberQueue sq;

/*
 *
 * Helper Functions
 *
 */

void sq_init() {
	sq.head = NULL;
	sq.tail = NULL;
}

void sq_push(char *keyword, int fs2a_fd, int fa2s_fd) {
	Subscriber *new_subscriber = calloc(1, sizeof(Subscriber));
	new_subscriber->keyword = calloc(strlen(keyword) + 1, sizeof(char));

	strncpy(new_subscriber->keyword, keyword, strlen(keyword));
	new_subscriber->keyword[strlen(keyword)] = 0;

	new_subscriber->fs2a_fd = fs2a_fd;
	new_subscriber->fa2s_fd = fa2s_fd;
	new_subscriber->next = NULL;

	if(sq.head == NULL) {
		sq.head = new_subscriber;
		sq.tail = new_subscriber;
	} else {
		sq.tail->next = new_subscriber;
		sq.tail = new_subscriber;
	}
}

void subscription_remove_by_fd(int fs2a_fd) {
	Subscriber *s = sq.head;
	Subscriber *p = NULL;

	while(s != NULL) {
		if(fs2a_fd == s->fs2a_fd) {
			free(s->keyword);
			if(p == NULL) {
				sq.head = s->next;
			} else {
				p->next = s->next;
			}
			free(s);
			break;
		}
		p = s;
		s = s->next;
	}
}

void sq_cleanup() {
	Subscriber *i = sq.head;
	Subscriber *j = NULL;

	while(i != NULL) {
		j = i;
		free(i->keyword);
		i = i->next;
		free(j);
	}
	sq_init();
}

void sq_print() {
	Subscriber *i = sq.head;

	printf("Subscriber Queue:\n");
	while(i != NULL) {
		printf("[%d, %d] %s\n", i->fs2a_fd, i->fa2s_fd, i->keyword);
		i = i->next;
	}

}
