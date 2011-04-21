#ifndef AMOS_COMMON_MAP_MEMCACHEDMAPDEF_H
#define AMOS_COMMON_MAP_MEMCACHEDMAPDEF_H

#include "type.h"
#include <stdint.h>

#define MEMCACHEDMAP_KEY_NAMESPACE			((uint8_t)'M')
#define MEMCACHEDMAP_KEY_TYPE_INFO			((uint8_t)'i')
#define MEMCACHEDMAP_KEY_TYPE_LIST			((uint8_t)'s')
#define MEMCACHEDMAP_KEY_TYPE_TILE_DATA		((uint8_t)'t')
#define MEMCACHEDMAP_KEY_TYPE_TILE_LOCK		((uint8_t)'l')
#define MEMCACHEDMAP_KEY_TYPE_TILE_REVISION	((uint8_t)'r')

typedef struct memcachedmap_key
{
	uint8_t ns;
	uint8_t type;
	uint8_t id[sizeof(map_tile_id_t) + sizeof(map_tile_id_t)];
} memcachedmap_key_t;

#define MEMCACHEDMAP_KEY_SIZE				(sizeof(uint8_t) + sizeof(uint8_t))
#define MEMCACHEDMAP_KEY_ID_SIZE			(sizeof(map_tile_id_t) + sizeof(map_tile_id_t))
#define MEMCACHEDMAP_KEY_WITH_ID_SIZE		(MEMCACHEDMAP_KEY_SIZE + MEMCACHEDMAP_KEY_ID_SIZE)

#endif // AMOS_COMMON_MAP_MEMCACHEDMAPDEF_H

