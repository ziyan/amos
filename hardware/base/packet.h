#ifndef _PACKET_H_
#define _PACKET_H_

#include <WProgram.h>

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

typedef int8_t(*packet_handler_t)(packet_t*);

#endif // _PACKET_H_

