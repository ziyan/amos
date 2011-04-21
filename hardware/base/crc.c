#include "crc.h"

uint16_t crc_update(uint16_t crc, uint8_t data)
{
    data ^= crc & 0xff;
    data ^= data << 4;
    return ((((uint16_t)data << 8) | ((crc >> 8) & 0xff)) ^ (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));
}

uint16_t crc_compute(const uint8_t data[], uint16_t length)
{
    const uint8_t *p = data;
    uint16_t i = 0, crc = 0xffff;
    for (i = 0; i < length; ++i, ++p)
    {
        crc = crc_update(crc, *p);
    }
    return crc;
}
