#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/stat.h>
//#include<sys/types.h>
#include<unistd.h>
#include<limits.h>
#include<signal.h>
#include"pipelist.c"
#include"messagequeue.c"

void graceful_exit() {
	cleanup_pipes();
	mq_cleanup();
}

void sigint_handler() {
	fprintf(stderr, "freeing memory\n");
	graceful_exit();
	exit(0);
}

int main() {
	char buf[PIPE_BUF];
	fd_set rfds;
	int retval;
	pipelist *pipeindex;

	signal(SIGINT, sigint_handler);

	if(create_pipes() != 0) {
		return 1;
	}


	while(1) {
		FD_ZERO(&rfds);
		pipeindex = head;
		while(pipeindex != NULL) {
			FD_SET(pipeindex->fd, &rfds);
			pipeindex = pipeindex->next;
		}

		retval = select(maxfd+1, &rfds, NULL, NULL, NULL);

		if(retval == -1) {
			fprintf(stderr, "select() error\n");
		} else if(retval) {
			pipeindex = head;
			while(pipeindex != NULL) {
				if(FD_ISSET(pipeindex->fd, &rfds)) {
					break;
				}
				pipeindex = pipeindex->next;
			}

			read(pipeindex->fd, buf, PIPE_BUF);
			printf("read: %s\n", buf);
		}
		sleep(1);
	}

	graceful_exit();
	return 0;
}
