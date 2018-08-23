#include<termios.h>
#include"messagequeue.c"
#include"fone_subscribers.h"

/* Prototypes */
static int uart_init();
static void uart_close();
static void *write_serial(void* arg);
static void *read_serial(void* arg);

/* Globals */
static int fona_fd_r = -1;
static int fona_fd_w = -1;

static int uart_init() {
	struct termios options_r, options_w;

	fona_fd_r = open("/dev/serial0", O_RDONLY | O_NOCTTY | O_NDELAY);
	fona_fd_w = open("/dev/serial0", O_WRONLY | O_NOCTTY | O_NDELAY);

	if(fona_fd_r < 0) {
		fprintf(stderr, "[Error] Can't open serial connection on /dev/serial0\n");
		return 1;
	}
	if(fona_fd_w < 0) {
		fprintf(stderr, "[Error] Can't open serial connection on /dev/serial0\n");
		return 1;
	}

	memset(&options_w, 0, sizeof(options_w));
	memset(&options_r, 0, sizeof(options_r));
	if(tcgetattr(fona_fd_r, &options_r) !=  0) {
		fprintf(stderr, "[Error] Getting tcgetattr\n");
		return 2;
	}
	if(tcgetattr(fona_fd_w, &options_w) !=  0) {
		fprintf(stderr, "[Error] Getting tcgetattr\n");
		return 2;
	}

	cfsetospeed(&options_w, 115200);
	cfsetispeed(&options_r, 115200);

	options_r.c_cflag = (options_r.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	options_r.c_iflag &= ~IGNBRK;         // disable break processing
	options_r.c_lflag = 0;                // no signaling chars, no echo,
	options_r.c_oflag = 0;                // no remapping, no delays
	//options_r.c_cc[VMIN]  = 0;            // read doesn't block
	//options_r.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
	options_r.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
	options_r.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
	options_r.c_cflag &= ~(PARENB | PARODD);      // no parity
	options_r.c_cflag |= 0;
	options_r.c_cflag &= ~CSTOPB;
	options_r.c_cflag &= ~CRTSCTS;

	options_w.c_cflag = (options_w.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	options_w.c_iflag &= ~IGNBRK;         // disable break processing
	options_w.c_lflag = 0;                // no signaling chars, no echo,
	options_w.c_oflag = 0;                // no remapping, no delays
	options_w.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
	options_w.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
	options_w.c_cflag &= ~(PARENB | PARODD);      // no parity
	options_w.c_cflag |= 0;
	options_w.c_cflag &= ~CSTOPB;
	options_w.c_cflag &= ~CRTSCTS;

	if (tcsetattr(fona_fd_r, TCSANOW, &options_r) != 0) {
		fprintf(stderr, "[Error] Setting tcsetattr\n");
		return 3;
	}
	if (tcsetattr(fona_fd_w, TCSANOW, &options_w) != 0) {
		fprintf(stderr, "[Error] Setting tcsetattr\n");
		return 3;
	}

	return 0;
}

static void uart_close() {
	close(fona_fd_r);
	fona_fd_r = -1;
	close(fona_fd_w);
	fona_fd_w = -1;
}

static void *write_serial(void* arg) {
	while(1) {
		if(mq.head != NULL && mq.last != mq.tail && fona_fd_w > 0) {
			if(mq.last == NULL) {
				write(fona_fd_w, (mq.head)->msg, strlen((mq.head)->msg));
				mq.last = mq.head;
			} else {
				write(fona_fd_w, (mq.last->next)->msg, strlen((mq.last->next)->msg));
				mq.last = mq.last->next;
			}
		}
		sleep(1);
	}
	return NULL;
}

static void *read_serial(void* arg) {
	char buf[PIPE_BUF];
	int sent_to_subscriber = 0;
	Subscriber *subscriber = NULL;

	memset(buf, 0, PIPE_BUF);

	while(1) {
		if((mq.head != NULL || sq.head != NULL) && fona_fd_r > 0) {
			read(fona_fd_r, buf, PIPE_BUF);
			/* First check if there are any clients subscribing to the message */
			subscriber = sq.head;
			while(subscriber != NULL) {
				if(strstr(buf, subscriber->keyword) != NULL) {
					write(subscriber->fs2a_fd, buf, strlen(buf));
					sent_to_subscriber = 1;
					//sleep(1);
				}
				subscriber = subscriber->next;
			}
			if(sent_to_subscriber == 1) {
				memset(buf, 0, PIPE_BUF);
				sent_to_subscriber = 0;
				sleep(1);
				continue;
			}

			/*
			 * since the read() call above blocks, we need to make sure that
			 * the head wasn't popped
			 */
			if(mq.head != NULL) {
				write(mq.head->fs2a_fd, buf, strlen(buf));
				memset(buf, 0, PIPE_BUF);
			}
		}

		// TODO: Sane sleep time? perhaps in nanoseconds?
		sleep(1);
	}
	return NULL;
}
