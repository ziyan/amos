#ifndef AMOS_PLUGINS_BASE_PACKET_H
#define AMOS_PLUGINS_BASE_PACKET_H

#include <stdint.h>

#define PACKET_DATA_MAX_LEN 24

#define PACKET_STX 0x02
#define PACKET_CMD_NCK '!'

#define PACKET_ERR_NOT_IMPLEMENTED 1
#define PACKET_ERR_INVALID_FORMAT 2

typedef struct packet
{
	uint8_t cmd;
	uint8_t len;
	uint8_t data[PACKET_DATA_MAX_LEN];
} packet_t;

#endif // AMOS_PLUGINS_BASE_PACKET_H

