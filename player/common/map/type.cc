#include "type.h"

#include <stdio.h>
#include <string.h>

void map_tile_id_to_hex(const map_tile_id_t &id, uint8_t hex[sizeof(map_tile_id_t) + sizeof(map_tile_id_t)])
{
	char buffer[sizeof(map_tile_id_t) + sizeof(map_tile_id_t) + 1];
	char *ptr = &buffer[0];
	for (unsigned i = 0; i < sizeof(map_tile_id_t); i++, ptr += 2)
		sprintf(ptr, "%02x", ((uint8_t*)&id)[i]);
	memcpy(hex, buffer, sizeof(map_tile_id_t) + sizeof(map_tile_id_t));
}

bool operator<(const map_tile_id_t &a, const map_tile_id_t &b)
{
	if (a.channel != b.channel) return a.channel < b.channel;
	if (a.x != b.x) return a.x < b.x;
	if (a.y != b.y) return a.y < b.y;
	return a.z < b.z;
}

bool operator==(const map_tile_id_t &a, const map_tile_id_t &b)
{
	return a.channel == b.channel && a.x == b.x && a.y == b.y && a.z == b.z;
}
