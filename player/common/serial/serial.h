#ifndef AMOS_COMMON_SERIAL_H
#define AMOS_COMMON_SERIAL_H

#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open serial port
 */
int serial_open(const char* port, speed_t baudrate);

/**
 * Convert integer to baudrate
 */
speed_t serial_baudrate(int baudrate, speed_t default_value);

/**
 * Clear received buffer in serial port
 */
void serial_clear(int fd);

/**
 * Wait util data arrives
 */
int serial_wait_input(int fd, const long int usecs);

/**
 * Wait until ready to output
 */
int serial_wait_output(int fd, const long int usecs);

/**
 * Blocking serial read
 */
int serial_read(int fd, char* buf, const int size);

/**
 * Blocking serial write
 */
int serial_write(int fd, const char* buf, const int size);

/**
 * Blocking but timed serial read
 */
int serial_timed_read(int fd, char* buf, const int size, const long int usecs);

/**
 * Blocking but timed serial write
 */
int serial_timed_write(int fd, const char* buf, const int size, const long int usecs);

/**
 * Blocking and timed STX detection
 */
int serial_wait_mark(int fd, const char mark, const long int usecs);

/**
 * Close serial port
 */
void serial_close(int fd);

#ifdef __cplusplus
}
#endif


#endif // AMOS_COMMON_SERIAL_H

