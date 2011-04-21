#ifndef AMOS_COMMON_MAP_H
#define AMOS_COMMON_MAP_H

#include <list>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <cmath>
#include <stdint.h>
#include <libmemcached/memcached.h>

#include "define.h"
#include "type.h"

namespace amos
{
	class Map
	{
	public:
		Map(const std::vector< std::pair<std::string, uint16_t> > &servers, uint32_t pages = 500);
		virtual ~Map();

		bool isOpen() const { return memc; }
		virtual map_info_t getInfo() const { return info; }

		virtual uint32_t getTileLength() const;
		virtual uint32_t getTileSize() const;

		virtual map_data_t get(uint32_t channel, double x, double y, double z);
		virtual void set(uint32_t channel, double x, double y, double z, map_data_t value);
		virtual void update(uint32_t channel, double x, double y, double z, map_data_t multiplication, map_data_t addition);

		virtual map_data_t get(uint32_t channel, int32_t x, int32_t y, int32_t z);
		virtual void set(uint32_t channel, int32_t x, int32_t y, int32_t z, map_data_t value);
		virtual void update(uint32_t channel, int32_t x, int32_t y, int32_t z, map_data_t multiplication, map_data_t addition);

		virtual map_data_t get(const map_tile_id_t &id, uint32_t index);
		virtual void set(const map_tile_id_t &id, uint32_t index, map_data_t value);
		virtual void update(const map_tile_id_t &id, uint32_t index, map_data_t multiplication, map_data_t addition);
		
		virtual const map_data_t* get(const map_tile_id_t &id, uint32_t *rev = 0);
		virtual void set(const map_tile_id_t &id, const map_data_t* tile);
		virtual void update(const map_tile_id_t &id, const map_data_t* multiplication, const map_data_t* addition);

		virtual std::set<map_tile_id_t> list(bool refresh = false);
		virtual void commit();
		virtual void refresh();

	protected:
		virtual void access(const map_tile_id_t &id);
		
		virtual void lock(const map_tile_id_t& id);
		virtual void unlock(const map_tile_id_t& id);
		virtual bool load(const map_tile_id_t& id, map_data_t **data, uint32_t *revision);
		virtual void save(const map_tile_id_t& id, const map_data_t *data,  uint32_t *revision);
		virtual void list(std::set<map_tile_id_t> &list);

		uint32_t pages;
		map_info_t info;

		std::list<map_tile_id_t> lru; // a queue for LRU, last element is last used
		std::map<map_tile_id_t, map_data_t*> data; // tile data map
		std::map<map_tile_id_t, map_data_t*> addition;
		std::map<map_tile_id_t, map_data_t*> multiplication;
		std::map<map_tile_id_t, bool> stale; // tile stale bit map
		std::map<map_tile_id_t, uint32_t> revision; // revision of the map
		std::set<map_tile_id_t> tiles; // cached tile list
		
		memcached_st *memc;
	};
}

#endif // AMOS_COMMON_MAP_H

