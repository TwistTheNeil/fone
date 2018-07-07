#include<stdlib.h>
#include<termios.h>
#include<unistd.h>
#include<stdio.h>
#include<fcntl.h>
#include<string.h>

int main() {
	int fd = -1;
	unsigned char tx_buffer[20];
	unsigned char *p_tx_buffer;

	fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);

	if(fd < 0) {
		printf("Error: Can't open serial connection\n");
		return 1;
	}

	// Configure UART
	struct termios options;
	memset(&options, 0, sizeof options);
	if ( tcgetattr(fd, &options) !=  0 )    {
		printf("error from tcgetattr\n");
		return -1;
	}

	cfsetospeed (&options, 115200);
	cfsetispeed (&options, 115200);

	options.c_cflag = (options.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	options.c_iflag &= ~IGNBRK;         // disable break processing
	options.c_lflag = 0;                // no signaling chars, no echo,
	options.c_oflag = 0;                // no remapping, no delays
	options.c_cc[VMIN]  = 0;            // read doesn't block
	options.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	options.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	options.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
	options.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	options.c_cflag |= 0;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CRTSCTS;

	if ( tcsetattr(fd, TCSANOW, &options) != 0 )    {
		printf("error from tcsetattr");
		return -1;
	}

	p_tx_buffer = &tx_buffer[0];
	*p_tx_buffer++ = 'A';
	*p_tx_buffer++ = 'T';
	*p_tx_buffer++ = '\r';
	*p_tx_buffer++ = '\n';

	if (fd != -1) {
		int count = write(fd, &tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));
		if (count < 0) {
			printf("UART TX error\n");
		}
	}

	printf("SENT!\n");
	if (fd != -1) {
		for(;;) {
			unsigned char rx_buffer[256];
			int rx_length = read(fd, (void*)rx_buffer, 255);
			if (rx_length < 0) {
				//	printf("RX_length < 0\n");
			}
			else if (rx_length == 0) {
				//	printf("RX_length = 0\n");
			}
			else {
				rx_buffer[rx_length] = '\0';
				printf("%i bytes read : %s\n", rx_length, rx_buffer);
			}
		}
	}

	close(fd);
	return 0;
}
