#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<limits.h>
#include<signal.h>
#include"client_protocol.h"

/*
 * We are fetching SMS the simple way - asking FONA for all messages and then
 * deleting the ones recently read. The application will have to handle the
 * messages once read (like print them out).
 * I do this instead of subscribing to +CMTI because I do not want to miss one
 * due to client downtime. Fetching messages which may have come in during
 * downtime would be the same - by simply asking for all outstanding unread
 * messages.
 */
static int in_fd, out_fd;
static char *buf;
static char *qbuf;
static int query = 0;

void stop_sms_query();
int fetch_sms();

void stop_sms_query() {
	query = 0;
}

int fetch_sms() {
	char *temp;
	char *found, *tofree;
	int messages = 0;
	int i, j;

	if(buf != NULL) {
		free(buf);
		buf = NULL;
	}
	if(qbuf != NULL) {
		free(qbuf);
		qbuf = NULL;
	}
	buf = calloc(PIPE_BUF, sizeof(char));
	qbuf = calloc(PIPE_BUF, sizeof(char));

	/* Set text mode */
	write(out_fd, "AT+CMGF=1\r\n", 11);
	sleep(1);
	read(in_fd, buf, PIPE_BUF);
	if(strstr(buf, "OK") == NULL) {
		fprintf(stderr, "[Error] Couldn't set text mode (%s)\n", buf);
		return 1;
	}

	/* Get number of messages in storage */
	write(out_fd, "AT+CPMS?\r\n", 10);
	read(in_fd, buf, PIPE_BUF);
	temp = strndup(buf, strlen(buf));
	tofree = temp;
	found = strsep(&temp, ",");
	found = strsep(&temp, ",");
	if(found != NULL) {
		messages = atoi(found);
		for(i=0; i<messages; i++) {
			memset(buf, 0, PIPE_BUF);
			sprintf(qbuf, "AT+CMGR=%d\r\n", i+1);
			write(out_fd, qbuf, strlen(qbuf));
			sleep(1);
			read(in_fd, buf, PIPE_BUF);
			if(strstr(buf, "CMGR") != NULL) {
				free(tofree);
				temp = strndup(buf, strlen(buf));
				tofree = temp;
				// Contact number is #4
				for(j=0; j<4; j++) {
					found = strsep(&temp, "\"");
				}
				printf("Contact: %s\n", found);

				// Date/Time is #8
				for(j=0; j<4; j++) {
					found = strsep(&temp, "\"");
				}
				printf("Time: %s\n", found);

				// Message is #9
				found = strsep(&temp, "\"");
				printf("%s\n", found);

				// Delete the message from FONA
				memset(qbuf, 0, PIPE_BUF);
				memset(buf, 0, PIPE_BUF);
				sprintf(qbuf, "AT+CMGD=%d\r\n", i+1);
				write(out_fd, qbuf, strlen(qbuf));
				sleep(1);
				read(in_fd, buf, PIPE_BUF);
				if(strstr(buf, "OK") == NULL) {
					fprintf(stderr, "[Error] Deleting message from FONA\n");
				}
			}
		}
	}

	memset (buf, 0, PIPE_BUF);
	/* Set PDU mode */
	write(out_fd, "AT+CMGF=0\r\n", 11);
	read(in_fd, buf, PIPE_BUF);
	if(strstr(buf, "OK") == NULL) {
		fprintf(stderr, "[Error] Couldn't set pdu mode\n");
		return 1;
	}

	free(tofree);
	free(qbuf);
	free(buf);
	buf = NULL;
	qbuf = NULL;
	return 0;
}

int main() {
	signal(SIGINT, stop_sms_query);

	init_fone_client();
	query = 1;

	while(1) {
		if(send_hello(&in_fd, &out_fd) != 0) {
			fprintf(stderr, "[Error] Failed to converse with server\n");
			break;
		}
		fetch_sms();
		if(send_finish(&in_fd, &out_fd) != 0) {
			fprintf(stderr, "[Error] Failed to finish conversation with server\n");
			break;
		}
		if(query == 0) {
			break;
		}
		sleep(10);
	}
}
