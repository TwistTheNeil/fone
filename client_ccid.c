#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<limits.h>
#include"application_protocol.h"

int main() {
	/*fd = open(buf, O_WRONLY);
	write(fd, "AT+CCID\r\n", 9);
	close(fd);*/

	send_hello();

	return 0;
}
