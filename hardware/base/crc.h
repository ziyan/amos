#ifndef _CRC_H_
#define _CRC_H_

#include <WProgram.h>


#ifdef __cplusplus
extern "C" {
#endif

uint16_t crc_update(uint16_t crc, uint8_t data);
uint16_t crc_compute(const uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif // _CRC_H_

