#include "map.h"

#include <string.h>
#include <algorithm>
#include <stdio.h>
#include <assert.h>
#include <limits>

#include "memcached.h"

using namespace amos;

Map::Map(const std::vector< std::pair<std::string, uint16_t> > &servers, uint32_t pages) : pages(pages), memc(0)
{
	memcached_return mr = MEMCACHED_SUCCESS;
	memcached_server_st *server_list = 0;
	memcachedmap_key_t info_key;
	size_t info_size = 0;
	uint32_t info_flags = 0;
	char *info_data = 0;

	memc = memcached_create(0);
	assert(memc);

	// create the list of server
	for (std::vector< std::pair<std::string, uint16_t> >::const_iterator i = servers.begin(); i != servers.end(); i++)
	{
		server_list = memcached_server_list_append(server_list, i->first.c_str(), i->second, &mr);
		if (mr != MEMCACHED_SUCCESS)
			goto error;
	}

	// connect to all servers
	mr = memcached_server_push(memc, server_list);
	if (mr != MEMCACHED_SUCCESS)
		goto error;
	memcached_server_free(server_list);
	server_list = 0;

	// now find out if there is a map on the server
	memset(&info_key, 0, sizeof(memcachedmap_key_t));
	info_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	info_key.type = MEMCACHEDMAP_KEY_TYPE_INFO;
	info_data = memcached_get(memc, (const char*)&info_key, MEMCACHEDMAP_KEY_SIZE, &info_size, &info_flags, &mr);
	if (!info_data)
		goto error;
	if (info_size != sizeof(map_info_t))
		goto error;
	memcpy(&info, info_data, sizeof(map_info_t));
	free(info_data);
	info_data = 0;

	// a little sanity check
	if (info.scale <= 0.0)
		goto error;

	return;

error:
	if (mr != MEMCACHED_SUCCESS)
	{
		fprintf(stderr, "memcachedmap: error: %s\n", memcached_strerror(memc, mr));
	}

	if (info_data)
	{
		free(info_data);
		info_data = 0;
	}

	if (memc)
	{
		memcached_quit(memc);
		memcached_free(memc);
		memc = 0;
	}
}

Map::~Map()
{
	if (memc)
	{
		memcached_quit(memc);
		memcached_free(memc);
		memc = 0;
	}

	for (std::map<map_tile_id_t, map_data_t*>::iterator i = data.begin(); i != data.end(); ++i)
	{
		if (i->second)
		{
			delete [] i->second;
			i->second = 0;
		}
	}
	for (std::map<map_tile_id_t, map_data_t*>::iterator i = addition.begin(); i != addition.end(); ++i)
	{
		if (i->second)
		{
			delete [] i->second;
			i->second = 0;
		}
	}
	for (std::map<map_tile_id_t, map_data_t*>::iterator i = multiplication.begin(); i != multiplication.end(); ++i)
	{	
		if (i->second)
		{
			delete [] i->second;
			i->second = 0;
		}
	}
}


uint32_t Map::getTileLength() const
{
	static uint32_t tile_length = 0;
	if (!tile_length) tile_length = info.tile_width * info.tile_height * info.tile_depth;
	return tile_length;
}

uint32_t Map::getTileSize() const
{
	static uint32_t tile_size = 0;
	if (!tile_size) tile_size = sizeof(map_data_t) * getTileLength();
	return tile_size;
}

map_data_t Map::get(uint32_t channel, double x, double y, double z)
{
	return get(channel, (int32_t)floor(x/info.scale), (int32_t)floor(y/info.scale), (int32_t)floor(z/info.scale));
}

void Map::set(uint32_t channel, double x, double y, double z, map_data_t value)
{
	set(channel, (int32_t)floor(x/info.scale), (int32_t)floor(y/info.scale), (int32_t)floor(z/info.scale), value);
}

void Map::update(uint32_t channel, double x, double y, double z, map_data_t multiplication, map_data_t addition)
{
	update(channel, (int32_t)floor(x/info.scale), (int32_t)floor(y/info.scale), (int32_t)floor(z/info.scale), multiplication, addition);
}

map_data_t Map::get(uint32_t channel, int32_t x, int32_t y, int32_t z)
{
	return get(
			(map_tile_id_t){channel,
							(int32_t)floor((double)x / (double)info.tile_width),
							(int32_t)floor((double)y / (double)info.tile_height),
							(int32_t)floor((double)z / (double)info.tile_depth)},
			(((z % info.tile_depth) + info.tile_depth) % info.tile_depth) * info.tile_height * info.tile_width +
			(((y % info.tile_height) + info.tile_height) % info.tile_height) * info.tile_width +
			(((x % info.tile_width) + info.tile_width)  % info.tile_width)
	);
}

void Map::set(uint32_t channel, int32_t x, int32_t y, int32_t z, map_data_t value)
{
	return set(
			(map_tile_id_t){channel,
							(int32_t)floor((double)x / (double)info.tile_width),
							(int32_t)floor((double)y / (double)info.tile_height),
							(int32_t)floor((double)z / (double)info.tile_depth)},
			(((z % info.tile_depth) + info.tile_depth) % info.tile_depth) * info.tile_height * info.tile_width +
			(((y % info.tile_height) + info.tile_height) % info.tile_height) * info.tile_width +
			(((x % info.tile_width) + info.tile_width)  % info.tile_width),
			value
	);
}

void Map::update(uint32_t channel, int32_t x, int32_t y, int32_t z, map_data_t multiplication, map_data_t addition)
{
	return update(
			(map_tile_id_t){channel,
							(int32_t)floor((double)x / (double)info.tile_width),
							(int32_t)floor((double)y / (double)info.tile_height),
							(int32_t)floor((double)z / (double)info.tile_depth)},
			(((z % info.tile_depth) + info.tile_depth) % info.tile_depth) * info.tile_height * info.tile_width +
			(((y % info.tile_height) + info.tile_height) % info.tile_height) * info.tile_width +
			(((x % info.tile_width) + info.tile_width)  % info.tile_width),
			multiplication,
			addition
	);
}


map_data_t Map::get(const map_tile_id_t &id, uint32_t index)
{
	access(id);
	if (!data[id])
		return ((map_data_t[])MAP_CHANNEL_DEFAULTS)[id.channel];
	return data[id][index];
}

void Map::set(const map_tile_id_t &id, uint32_t index, map_data_t value)
{
	update(id, index, 0.0f, value);
}

void Map::update(const map_tile_id_t &id, uint32_t index, map_data_t m, map_data_t a)
{
	static const float inf = std::numeric_limits<float>::infinity();

	assert(fabs(m) != inf);

	access(id);	

	if (!data[id])
	{
		data[id] = new map_data_t[getTileLength()];
		for (int j = 0; j < (int)getTileLength(); j++)
			data[id][j] = ((map_data_t[])MAP_CHANNEL_DEFAULTS)[id.channel];
	}
	
	if (!multiplication[id])
	{
		multiplication[id] = new map_data_t[getTileLength()];
		for (int j = 0; j < (int)getTileLength(); j++)
			multiplication[id][j] = 1.0f;
	}
	
	if (!addition[id])
	{
		addition[id] = new map_data_t[getTileLength()];
		memset(addition[id], 0, getTileSize());
	}

	multiplication[id][index] *= m;

	if (m == 0.0f && fabs(addition[id][index]) == inf)
		addition[id][index] = 0.0f;
	else
		addition[id][index] *= m;
	addition[id][index] += a;
	
	if (m == 0.0f && fabs(data[id][index]) == inf)
		data[id][index] = 0.0f;
	else
		data[id][index] *= m;
	data[id][index] += a;
}

const map_data_t* Map::get(const map_tile_id_t& id, uint32_t *rev)
{
	access(id);
	if (rev) *rev = revision[id];
	return data[id];
}

void Map::set(const map_tile_id_t &id, const map_data_t* tile)
{
	assert(tile);

	access(id);

	if (!data[id])
		data[id] = new map_data_t[getTileLength()];
	if (!multiplication[id])
		multiplication[id] = new map_data_t[getTileLength()];
	if (!addition[id])
		addition[id] = new map_data_t[getTileLength()];
	
	memcpy(data[id], tile, getTileSize());
	memset(multiplication[id], 0, getTileSize());
	memcpy(data[id], tile, getTileSize());
}

void Map::update(const map_tile_id_t &id, const map_data_t* m, const map_data_t* a)
{
	static const float inf = std::numeric_limits<float>::infinity();

	assert(m && a);

	access(id);

	if (!data[id])
	{
		data[id] = new map_data_t[getTileLength()];
		for (int j = 0; j < (int)getTileLength(); j++)
			data[id][j] = ((map_data_t[])MAP_CHANNEL_DEFAULTS)[id.channel];
	}
	
	if (!multiplication[id])
	{
		multiplication[id] = new map_data_t[getTileLength()];
		for (int j = 0; j < (int)getTileLength(); j++)
			multiplication[id][j] = 1.0f;
	}
	
	if (!addition[id])
	{
		addition[id] = new map_data_t[getTileLength()];
		memset(addition[id], 0, getTileSize());
	}

	for (int j = 0; j < (int)getTileLength(); j++)
	{
		assert(fabs(m[j]) != inf);
	
		multiplication[id][j] *= m[j];
		
		if (m[j] == 0.0f && fabs(addition[id][j]) == inf)
			addition[id][j] = 0.0f;
		else
			addition[id][j] *= m[j];
		addition[id][j] += a[j];
		
		if (m[j] == 0.0f && fabs(data[id][j]) == inf)
			data[id][j] = 0.0f;
		else
			data[id][j] *= m[j];
		data[id][j] += a[j];
	}
}

std::set<map_tile_id_t> Map::list(bool refresh)
{
	if (!refresh) return tiles;
	tiles.clear();
	list(tiles);
	for(std::list<map_tile_id_t>::iterator i = lru.begin(); i != lru.end(); i++)
	{
		if (data[*i]) tiles.insert(*i);
	}
	return tiles;
}

void Map::commit()
{
	static const float inf = std::numeric_limits<float>::infinity();

	for(std::list<map_tile_id_t>::iterator i = lru.begin(); i != lru.end(); ++i)
	{
		// save it if dirty
		if (!((addition[*i] || multiplication[*i]) && data[*i])) continue;

		// first lock tile up to make sure no one else is updating it
		lock(*i);
		
		// load the newest tile if available
		if (load(*i, &data[*i], &revision[*i]))
		{
			// if tile has new revision, apply update again
			for (int j = 0; j < (int)getTileLength(); j++)
			{
				if (multiplication[*i])
				{
					if (multiplication[*i][j] == 0.0f && fabs(data[*i][j]) == inf)
						data[*i][j] = 0.0f;
					else
						data[*i][j] *= multiplication[*i][j];
				}
				if (addition[*i])
					data[*i][j] += addition[*i][j];
			}
		}

		// save tile
		save(*i, data[*i], &revision[*i]);
		
		// unlock tile to allow others to update
		unlock(*i);

		// clear dirty
		if (multiplication[*i])
		{
			delete [] multiplication[*i];
			multiplication[*i] = 0;
		}
		
		if (addition[*i])
		{
			delete [] addition[*i];
			addition[*i] = 0;
		}
		multiplication.erase(*i);
		addition.erase(*i);
		stale[*i] = false;
	}
}

void Map::refresh()
{
	for(std::list<map_tile_id_t>::iterator i = lru.begin(); i != lru.end(); ++i)
	{
		stale[*i] = true;
	}
}

void Map::access(const map_tile_id_t &id)
{
	static const float inf = std::numeric_limits<float>::infinity();

	std::list<map_tile_id_t>::iterator i = find(lru.begin(), lru.end(), id);
	if (i == lru.end())
	{
		// get rid of least recently used lru
		while (lru.size() >= pages && lru.size() > 0)
		{
			map_tile_id_t least_used = lru.back();

			// save it
			if ((addition[least_used] || multiplication[least_used]) && data[least_used])
			{
				// first lock tile up to make sure no one else is updating it
				lock(least_used);
				
				// load the newest tile if available
				if (load(least_used, &data[least_used], &revision[least_used]))
				{
					// if tile has new revision, apply update again
					for (int j = 0; j < (int)getTileLength(); j++)
					{
						if (multiplication[least_used])
						{
							if (multiplication[least_used][j] == 0.0f && fabs(data[least_used][j]) == inf)
								data[least_used][j] = 0.0f;
							else
								data[least_used][j] *= multiplication[least_used][j];
						}
						if (addition[least_used])
							data[least_used][j] += addition[least_used][j];
					}
				}

				// save tile
				save(least_used, data[least_used], &revision[least_used]);
				
				// unlock tile to allow others to update
				unlock(least_used);
			}

			// remove it
			if (data[least_used])
			{
				delete [] data[least_used];
				data[least_used] = 0;
			}
			if (multiplication[least_used])
			{
				delete [] multiplication[least_used];
				multiplication[least_used] = 0;
			}
			
			if (addition[least_used])
			{
				delete [] addition[least_used];
				addition[least_used] = 0;
			}
			data.erase(least_used);
			multiplication.erase(least_used);
			addition.erase(least_used);
			stale.erase(least_used);
			revision.erase(least_used);
			lru.pop_back();
		}

		load(id, &data[id], &revision[id]);
		assert(!multiplication[id]);
		assert(!addition[id]);

		stale[id] = false;
		lru.push_front(id);
	}
	else
	{
		if (stale[id])
		{
			if (load(id, &data[id], &revision[id]) && (multiplication[id] || addition[id]))
			{
				// if tile has new revision, apply update again
				for (int i = 0; i < (int)getTileLength(); i++)
				{
					if (multiplication[id])
					{
						if (multiplication[id][i] == 0.0f && fabs(data[id][i]) == inf)
							data[id][i] = 0.0f;
						else
							data[id][i] *= multiplication[id][i];
					}
					if (addition[id])
						data[id][i] += addition[id][i];
				}
			}
			stale[id] = false;
		}
		
		// update LRU
		if (i != lru.begin())
		{
			lru.erase(i);
			lru.push_front(id);
		}
	}
}


void Map::lock(const map_tile_id_t& id)
{
	memcached_return mr = MEMCACHED_SUCCESS;
	memcachedmap_key_t tile_lock_key;

	if (!isOpen()) return;

	memset(&tile_lock_key, 0, sizeof(memcachedmap_key_t));
	tile_lock_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	tile_lock_key.type = MEMCACHEDMAP_KEY_TYPE_TILE_LOCK;
	map_tile_id_to_hex(id, tile_lock_key.id);

	for(;;)
	{
		// the lock has some bogus data in it, because we don't really care
		mr = memcached_add(memc, (const char*)&tile_lock_key, MEMCACHEDMAP_KEY_WITH_ID_SIZE, "L", 1, (time_t)0, 0);
		// if we see a collision, it means someone has a lock on the tile already, so we wait
		if (mr != MEMCACHED_NOTSTORED) break;
		usleep(100); // make sure we don't use up 100% CPU when the network is fast
	}

	if (mr != MEMCACHED_SUCCESS)
		goto error;
	return;

error:
	if (mr != MEMCACHED_SUCCESS)
	{
		fprintf(stderr, "memcachedmap: lock: error: %s\n", memcached_strerror(memc, mr));
	}
}

void Map::unlock(const map_tile_id_t& id)
{
	memcached_return mr = MEMCACHED_SUCCESS;
	memcachedmap_key_t tile_lock_key;

	if (!isOpen()) return;

	memset(&tile_lock_key, 0, sizeof(memcachedmap_key_t));
	tile_lock_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	tile_lock_key.type = MEMCACHEDMAP_KEY_TYPE_TILE_LOCK;
	map_tile_id_to_hex(id, tile_lock_key.id);

	mr = memcached_delete(memc, (const char*)&tile_lock_key, MEMCACHEDMAP_KEY_WITH_ID_SIZE, (time_t)0);
	if (mr != MEMCACHED_SUCCESS)
		goto error;
	return;

error:
	if (mr != MEMCACHED_SUCCESS)
	{
		fprintf(stderr, "memcachedmap: unlock: error: %s\n", memcached_strerror(memc, mr));
	}
}

bool Map::load(const map_tile_id_t& id, map_data_t **data, uint32_t *revision)
{
	memcached_return mr = MEMCACHED_SUCCESS;

	uint32_t tile_revision = 0;
	memcachedmap_key_t tile_revision_key;
	size_t tile_revision_size = 0;
	uint32_t tile_revision_flags = 0;
	char *tile_revision_data = 0;

	memcachedmap_key_t tile_data_key;
	size_t tile_data_size = 0;
	uint32_t tile_data_flags = 0;
	char *tile_data_data = 0;

	if (!isOpen()) return false;

	//
	// first find out revision
	//

	memset(&tile_revision_key, 0, sizeof(memcachedmap_key_t));
	tile_revision_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	tile_revision_key.type = MEMCACHEDMAP_KEY_TYPE_TILE_REVISION;
	map_tile_id_to_hex(id, tile_revision_key.id);
	tile_revision_data = memcached_get(memc, (const char*)&tile_revision_key, MEMCACHEDMAP_KEY_WITH_ID_SIZE, &tile_revision_size, &tile_revision_flags, &mr);

	// does tile exist on server?
	if (!tile_revision_data) return false;

	if (tile_revision_size != sizeof(uint32_t))
		goto error;
	tile_revision = *((uint32_t*)tile_revision_data);
	free(tile_revision_data);
	tile_revision_data = 0;

	// if no new revision is available, we don't need to fetch the tile again
	if (tile_revision <= *revision) return false;

	//
	// new revision, we do need to fetch tile
	//
	memset(&tile_data_key, 0, sizeof(memcachedmap_key_t));
	tile_data_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	tile_data_key.type = MEMCACHEDMAP_KEY_TYPE_TILE_DATA;
	memcpy(tile_data_key.id, tile_revision_key.id, MEMCACHEDMAP_KEY_ID_SIZE);
	tile_data_data = memcached_get(memc, (const char*)&tile_data_key, MEMCACHEDMAP_KEY_WITH_ID_SIZE, &tile_data_size, &tile_data_flags, &mr);
	// if revision key exists but not the tile data, something is terribly wrong
	if (!tile_data_data)
		goto error;
	if (tile_data_size != getTileSize())
		goto error;

	// trust you have the correct size if data is already allocated
	if (!(*data))
		*data = new map_data_t[getTileLength()];
	memcpy(*data, tile_data_data, getTileSize());

	free(tile_data_data);
	tile_data_data = 0;

	*revision = tile_revision;

	return true; // new tile loaded, hence return true

error:
	if (mr != MEMCACHED_SUCCESS)
	{
		fprintf(stderr, "memcachedmap: load: error: %s\n", memcached_strerror(memc, mr));
	}

	if (tile_revision_data)
	{
		free(tile_revision_data);
		tile_revision_data = 0;
	}

	if (tile_data_data)
	{
		free(tile_data_data);
		tile_data_data = 0;
	}
	return false;
}

void Map::save(const map_tile_id_t& id, const map_data_t *data, uint32_t *revision)
{
	memcached_return mr = MEMCACHED_SUCCESS;

	uint32_t tile_revision = 0;
	memcachedmap_key_t tile_revision_key;
	size_t tile_revision_size = 0;
	uint32_t tile_revision_flags = 0;
	char *tile_revision_data = 0;

	memcachedmap_key_t tile_data_key;

	memcachedmap_key_t list_key;
	
	if (!isOpen()) return;

	//
	// first find out revision
	//
	memset(&tile_revision_key, 0, sizeof(memcachedmap_key_t));
	tile_revision_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	tile_revision_key.type = MEMCACHEDMAP_KEY_TYPE_TILE_REVISION;
	map_tile_id_to_hex(id, tile_revision_key.id);
	tile_revision_data = memcached_get(memc, (const char*)&tile_revision_key, MEMCACHEDMAP_KEY_WITH_ID_SIZE, &tile_revision_size, &tile_revision_flags, &mr);

	// does tile exist on server?
	if (tile_revision_data)
	{
		if (tile_revision_size != sizeof(uint32_t))
			goto error;
		tile_revision = *((uint32_t*)tile_revision_data);
		free(tile_revision_data);
		tile_revision_data = 0;
	}
	else if (mr != MEMCACHED_NOTFOUND)
		goto error;
	
	// the revision on the server should be the same as the one we have
	assert(*revision == tile_revision);

	//
	// new revision, we do need to upload tile
	//
	memset(&tile_data_key, 0, sizeof(memcachedmap_key_t));
	tile_data_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	tile_data_key.type = MEMCACHEDMAP_KEY_TYPE_TILE_DATA;
	memcpy(tile_data_key.id, tile_revision_key.id, MEMCACHEDMAP_KEY_ID_SIZE);
	mr = memcached_set(memc, (const char*)&tile_data_key, MEMCACHEDMAP_KEY_WITH_ID_SIZE, (const char*)data, getTileSize(), (time_t)0, 0);
	if (mr != MEMCACHED_SUCCESS)
		goto error;

	//
	// now we need to update revision number
	//
	tile_revision ++;
	mr = memcached_set(memc, (const char*)&tile_revision_key, MEMCACHEDMAP_KEY_WITH_ID_SIZE, (const char*)&tile_revision, sizeof(uint32_t), (time_t)0, 0);
	if (mr != MEMCACHED_SUCCESS)
		goto error;
	*revision = tile_revision;

	//
	// now we need to add this tile into the list of tiles if it does not exists in there yet
	//
	if (tile_revision != 1) return;
	memset(&list_key, 0, sizeof(memcachedmap_key_t));
	list_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	list_key.type = MEMCACHEDMAP_KEY_TYPE_LIST;
	mr = memcached_append(memc, (const char*)&list_key, MEMCACHEDMAP_KEY_SIZE, (const char*)&id, sizeof(map_tile_id_t), (time_t)0, 0);
	if (mr != MEMCACHED_SUCCESS)
		goto error;

	return;

error:
	if (mr != MEMCACHED_SUCCESS)
	{
		fprintf(stderr, "memcachedmap: save: error: %s\n", memcached_strerror(memc, mr));
	}

	if (tile_revision_data)
	{
		free(tile_revision_data);
		tile_revision_data = 0;
	}
}

void Map::list(std::set<map_tile_id_t> &output)
{
	memcached_return mr = MEMCACHED_SUCCESS;

	memcachedmap_key_t list_key;
	size_t list_size = 0;
	uint32_t list_flags = 0;
	char *list_data = 0;
	map_tile_id_t *list_ptr = 0;
	unsigned int i = 0;

	if (!isOpen()) return;
	memset(&list_key, 0, sizeof(memcachedmap_key_t));
	list_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	list_key.type = MEMCACHEDMAP_KEY_TYPE_LIST;
	list_data = memcached_get(memc, (const char*)&list_key, MEMCACHEDMAP_KEY_SIZE, &list_size, &list_flags, &mr);

	// it might be the case that no tile exists yet
	if (list_data)
	{
		if (list_size % sizeof(map_tile_id_t) != 0)
			goto error;

		list_ptr = (map_tile_id_t*)list_data;
		for (i = 0; i < list_size / sizeof(map_tile_id_t); i++, list_ptr++)
			output.insert(*list_ptr);
		free(list_data);
		list_data = 0;
	}
	else if (mr != MEMCACHED_NOTFOUND)
	{
		goto error;
	}
	return;

error:
	if (mr != MEMCACHED_SUCCESS)
	{
		fprintf(stderr, "memcachedmap: list: error: %s\n", memcached_strerror(memc, mr));
	}

	if (list_data)
	{
		free(list_data);
		list_data = 0;
	}
}


