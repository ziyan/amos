#ifndef AMOS_COMMON_CRC_H
#define AMOS_COMMON_CRC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t crc_update(uint16_t crc, uint8_t data);
uint16_t crc_compute(const uint8_t data[], uint16_t length);

#ifdef __cplusplus
}
#endif

#endif // AMOS_COMMON_CRC_H
