#include "comm.h"
#include "packet.h"
#include "crc/crc.h"
#include "serial/serial.h"

#include <string.h>
#include <stdio.h>

#define COMM_SERIAL_TIMEOUT 1000000

int comm_send_packet(int fd, const packet_t *pkt, const long int usec)
{
	if (!pkt || pkt->len > PACKET_DATA_MAX_LEN) return -1;

	// large enough buffer to always work
	static uint8_t buf[sizeof(packet_t) + 3] = {0};
	uint16_t crc = crc_compute((uint8_t*)pkt, pkt->len + 2);

	// clear the line
	serial_clear(fd);

	// stx
	buf[0] = PACKET_STX;

	// content of packet
	memcpy(buf+1, pkt, pkt->len + 2);

	// crc
	buf[pkt->len + 3] = (crc >> 8) & 0xff;
	buf[pkt->len + 4] = crc & 0xff;

	// send
	if(serial_timed_write(fd, (char*)buf, pkt->len + 5, usec)) return -1;
	return 0;
}

int comm_receive_packet(int fd, packet_t *pkt, const long int usec)
{
	static uint8_t buf[2] = {0};
	uint16_t crc = 0;

	if (serial_wait_mark(fd, PACKET_STX, usec)) return -1;
	if (serial_timed_read(fd, (char*)pkt, 2, usec)) return -1;
	if (pkt->len > PACKET_DATA_MAX_LEN) return -1;
	if (serial_timed_read(fd, (char*)pkt->data, pkt->len, usec)) return -1;
	if (serial_timed_read(fd, (char*)buf, 2, usec)) return -1;
	crc = buf[0] << 8 | buf[1];
	if (crc_compute((uint8_t*)pkt, pkt->len + 2) != crc) return -2;
	return 0;
}

int comm_reset(int fd)
{
	packet_t pkt = {0};
	pkt.cmd = 'R';
	pkt.len = 0;
	if (comm_send_packet(fd, &pkt, COMM_SERIAL_TIMEOUT)) return -1;
	if (comm_receive_packet(fd, &pkt, COMM_SERIAL_TIMEOUT)) return -1;
	if (pkt.cmd != 'R' || pkt.len != 0) return -2;
	return 0;
}

int comm_set_speed(int fd, const float left, const float right)
{
	typedef struct {
		float left, right;
	} data_t;

	packet_t pkt = {0};
	data_t *data = (data_t*)pkt.data;

	pkt.cmd = 'S';
	pkt.len = sizeof(data_t);

	data->left = left;
	data->right = right;

	if (comm_send_packet(fd, &pkt, COMM_SERIAL_TIMEOUT)) return -1;
	if (comm_receive_packet(fd, &pkt, COMM_SERIAL_TIMEOUT)) return -1;
	if (pkt.cmd != 'S' || pkt.len != 0) return -2;
	return 0;
}


int comm_set_pid(int fd, const float p_left, const float i_left, const float d_left, const float p_right, const float i_right, const float d_right)
{
	typedef struct {
		float p_left, i_left, d_left;
		float p_right, i_right, d_right;
	} data_t;

	packet_t pkt = {0};
	data_t *data = (data_t*)pkt.data;

	pkt.cmd = 'P';
	pkt.len = sizeof(data_t);

	data->p_left = p_left;
	data->i_left = i_left;
	data->d_left = d_left;
	data->p_right = p_right;
	data->i_right = i_right;
	data->d_right = d_right;

	if (comm_send_packet(fd, &pkt, COMM_SERIAL_TIMEOUT)) return -1;
	if (comm_receive_packet(fd, &pkt, COMM_SERIAL_TIMEOUT)) return -1;
	if (pkt.cmd != 'P' || pkt.len != 0) return -2;
	return 0;
}

int comm_set_acceleration(int fd, const float a_left, const float a_right)
{
	typedef struct {
		float a_left, a_right;
	} data_t;

	packet_t pkt = {0};
	data_t *data = (data_t*)pkt.data;

	pkt.cmd = 'A';
	pkt.len = sizeof(data_t);

	data->a_left = a_left;
	data->a_right = a_right;

	if (comm_send_packet(fd, &pkt, COMM_SERIAL_TIMEOUT)) return -1;
	if (comm_receive_packet(fd, &pkt, COMM_SERIAL_TIMEOUT)) return -1;
	if (pkt.cmd != 'A' || pkt.len != 0) return -2;
	return 0;
}

int comm_get_odometry(int fd, float *left, float *left_t, float *right, float *right_t)
{
	typedef struct {
		float left, left_t, right, right_t;
	} data_t;

	packet_t pkt = {0};
	data_t *data = (data_t*)pkt.data;

	pkt.cmd = 'o';
	pkt.len = 0;

	if (comm_send_packet(fd, &pkt, COMM_SERIAL_TIMEOUT)) return -1;
	if (comm_receive_packet(fd, &pkt, COMM_SERIAL_TIMEOUT)) return -1;
	if (pkt.cmd != 'o' || pkt.len != sizeof(data_t)) return -2;

	*left = data->left;
	*left_t = data->left_t;
	*right = data->right;
	*right_t = data->right_t;

	return 0;
}

int comm_get_power(int fd, float *voltage_left, float *current_left, float *voltage_right, float *current_right)
{
	typedef struct {
		float voltage_left, current_left, voltage_right, current_right;
	} data_t;

	packet_t pkt = {0};
	data_t *data = (data_t*)pkt.data;

	pkt.cmd = 'p';
	pkt.len = 0;

	if (comm_send_packet(fd, &pkt, COMM_SERIAL_TIMEOUT)) return -1;
	if (comm_receive_packet(fd, &pkt, COMM_SERIAL_TIMEOUT)) return -1;
	if (pkt.cmd != 'p' || pkt.len != sizeof(data_t)) return -2;

	*voltage_left = data->voltage_left;
	*current_left = data->current_left;
	*voltage_right = data->voltage_right;
	*current_right = data->current_right;

	return 0;
}


