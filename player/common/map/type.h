#ifndef AMOS_COMMON_MAP_MAPTYPE_H
#define AMOS_COMMON_MAP_MAPTYPE_H

#include <stdint.h>

typedef struct map_tile_id {
	uint32_t channel;
	int32_t x, y, z;
} map_tile_id_t;

typedef struct map_info {
	double scale;
	uint32_t tile_width, tile_height, tile_depth;
} map_info_t;

typedef float map_data_t;

void map_tile_id_to_hex(const map_tile_id_t &id, uint8_t hex[sizeof(map_tile_id_t) + sizeof(map_tile_id_t)]);
bool operator<(const map_tile_id_t &a, const map_tile_id_t &b);
bool operator==(const map_tile_id_t &a, const map_tile_id_t &b);

#endif

