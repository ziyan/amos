#ifndef AMOS_UTILS_MAPTOOL_H
#define AMOS_UTILS_MAPTOOL_H

#include <string>
#include <map>
#include <vector>
#include <set>
#include <stdint.h>

#include <libmemcached/memcached.h>
#include <sqlite3.h>

#include "map/type.h"

extern memcached_st *memcached_connect(const std::vector< std::pair<std::string, uint16_t> > &servers);
extern bool memcached_info(memcached_st *memc, map_info_t &info);
extern bool memcached_show(const std::vector< std::pair<std::string, uint16_t> > &servers);
extern bool memcached_flush(const std::vector< std::pair<std::string, uint16_t> > &servers);
extern bool memcached_load(memcached_st *memc, const map_tile_id_t &id, const map_data_t *data, const uint32_t &length, const uint32_t &revision);
extern bool memcached_init(memcached_st *memc, const map_info_t &info);
extern bool memcached_save(const char* path, const std::vector< std::pair<std::string, uint16_t> > &servers);

extern sqlite3 *db_open(const char* path);
extern int db_size(const char* path);
extern bool db_info(sqlite3 *db, map_info_t &info);
extern bool db_init(sqlite3 *db, const map_info_t &info);
extern bool db_create(const char* path, const map_info_t &info);
extern bool db_show(const char* path);
extern bool db_load(const char* path, const std::vector< std::pair<std::string, uint16_t> > &servers);
extern bool db_save(sqlite3 *db, const map_tile_id_t &id, const map_data_t *data, const uint32_t &length, const uint32_t &revision);

#endif // AMOS_UTILS_MAPTOOL_H

