#include "serial.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


int serial_open(const char* port, speed_t baudrate) {
	int fd = open(port, O_RDWR | O_SYNC | O_NONBLOCK, S_IRUSR | S_IWUSR);
	if (fd < 0) return -1;
	if (tcflush(fd, TCIFLUSH)) return -1;
	struct termios options;
	tcgetattr(fd, &options);
	cfmakeraw(&options);
	cfsetispeed(&options, baudrate);
	cfsetospeed(&options, baudrate);
	tcsetattr(fd, TCSAFLUSH, &options);
	int flags;
	if ((flags = fcntl(fd, F_GETFL)) < 0) return -1;
	if (fcntl(fd, F_SETFL, flags ^ O_NONBLOCK) < 0) return -1;
	return fd;
}

speed_t serial_baudrate(int baudrate, speed_t default_value) {
	switch(baudrate) {
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
#ifdef B230400
	case 230400:
		return B230400;
#endif
	}
	return default_value;
}

void serial_clear(int fd) {
	char buf[10];
	fcntl(fd, F_SETFL, O_NONBLOCK);
	while(read(fd, buf, 10) > 0) ;
	fcntl(fd, F_SETFL, 0);
}

int serial_wait_input(int fd, const long int usecs) {
	fd_set input;
	struct timeval timer;
	FD_ZERO(&input);
	FD_SET(fd, &input);
	timer.tv_sec = usecs / 1000000;
	timer.tv_usec = usecs % 1000000;
	return select(fd+1, &input, NULL, NULL, &timer);
}

int serial_wait_output(int fd, const long int usecs) {
	fd_set output;
	struct timeval timer;
	FD_ZERO(&output);
	FD_SET(fd, &output);
	timer.tv_sec = usecs / 1000000;
	timer.tv_usec = usecs % 1000000;
	return select(fd+1, NULL, &output, NULL, &timer);
}

int serial_read(int fd, char* buf, const int size) {
	fcntl(fd, F_SETFL, 0);
	int s, cur = 0;
	while(cur < size) {
		s = read(fd, buf + cur, size - cur);
		if(s <= 0) return -1;
		cur += s;
	}
	return 0;
}

int serial_timed_read(int fd, char* buf, const int size, const long int usecs) {
	fd_set input;
	struct timeval timer;
	FD_ZERO(&input);
	FD_SET(fd, &input);
	timer.tv_sec = usecs / 1000000;
	timer.tv_usec = usecs % 1000000;
	fcntl(fd, F_SETFL, 0);
	int s, cur = 0;
	while(cur < size) {
		if (select(fd+1, &input, NULL, NULL, &timer) <= 0) return -1;
		s = read(fd, buf + cur, size - cur);
		if(s <= 0) return -1;
		cur += s;
	}
	return 0;
}


int serial_write(int fd, const char* buf, const int size) {
	fcntl(fd, F_SETFL, 0);
	int s, cur = 0;
	while(cur < size) {
		s = write(fd, buf + cur, size - cur);
		if(s < 0) return -1;
		cur += s;
	}
	return 0;
}

int serial_timed_write(int fd, const char* buf, const int size, const long int usecs) {
	fd_set output;
	struct timeval timer;
	FD_ZERO(&output);
	FD_SET(fd, &output);
	timer.tv_sec = usecs / 1000000;
	timer.tv_usec = usecs % 1000000;
	fcntl(fd, F_SETFL, 0);
	int s, cur = 0;
	while(cur < size) {
		if (select(fd+1, NULL, &output, NULL, &timer) <= 0) return -1;
		s = write(fd, buf + cur, size - cur);
		if (s < 0) return -1;
		cur += s;
	}
	return 0;
}

int serial_wait_mark(int fd, const char mark, const long int usecs) {
	char buf[1];
	buf[0] = 0;
	while(buf[0]!=mark)
		if(serial_timed_read(fd, buf, 1, usecs) != 0)
			return -1;
	return 0;
}

void serial_close(int fd) {
	if(fd>=0) close(fd);
}

