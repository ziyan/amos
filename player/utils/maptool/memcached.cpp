// *************************************************************************************************
// include section

#include <iostream>
#include <assert.h>
#include <string.h>

#include "maptool.h"
#include "map/memcached.h"

using namespace std;

// *************************************************************************************************
// implementation section

memcached_st *memcached_connect(const vector< pair<string, uint16_t> > &servers)
{
	memcached_st *memc = 0;
	memcached_return mr = MEMCACHED_SUCCESS;
	memcached_server_st *server_list = 0;

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
	
	return memc;
	
error:
	if (mr != MEMCACHED_SUCCESS)
	{
		cerr << "ERROR: memcached_connect: " << memcached_strerror(memc, mr) << endl << endl;
	}
	if (memc)
	{
		memcached_quit(memc);
		memcached_free(memc);
		memc = 0;
	}
	return 0;
}

bool memcached_info(memcached_st *memc, map_info_t &info)
{
	memcached_return mr = MEMCACHED_SUCCESS;
	memcachedmap_key_t info_key;
	size_t info_size = 0;
	uint32_t info_flags = 0;
	char *info_data = 0;
	
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
	return true;

error:
	if (info_data)
	{
		free(info_data);
		info_data = 0;
	}
	return false;
}

bool memcached_list(memcached_st *memc, set<map_tile_id_t> &tiles)
{
	memcached_return mr = MEMCACHED_SUCCESS;
	memcachedmap_key_t list_key;
	size_t list_size = 0;
	uint32_t list_flags = 0;
	char *list_data = 0;
	map_tile_id_t *list_ptr = 0;
	unsigned int i = 0;

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
			tiles.insert(*list_ptr);
		free(list_data);
		list_data = 0;
	}
	else if (mr != MEMCACHED_NOTFOUND)
	{
		goto error;
	}
	return true;

error:
	if (mr != MEMCACHED_SUCCESS)
	{
		cerr << "ERROR: memcached_list: " << memcached_strerror(memc, mr) << endl << endl;
	}

	if (list_data)
	{
		free(list_data);
		list_data = 0;
	}
	return false;
}

bool memcached_show(const vector< pair<string, uint16_t> > &servers)
{
	memcached_st *memc = 0;
	map_info_t info = {0};
	set<map_tile_id_t> tiles;

	memc = memcached_connect(servers);
	if (!memc) goto error;
	
	cout << "Map Server:" << endl;
	for (vector< pair<string, uint16_t> >::const_iterator i = servers.begin(); i != servers.end(); i++)
		cout << "\t" << (i->first) << ":" << (i->second) << endl;
	cout << endl;

	if (!memcached_info(memc, info))
	{
		cerr << "ERROR: Map not found on server" << endl << endl;
		goto error;
	}

	cout << "Map Information:" << endl;
	cout << "\tResolution: " << info.scale << " (meters/pixel)" << endl;
	cout << "\tTile Size: [" << info.tile_width << "," << info.tile_height << "," << info.tile_depth << "] (pixels)" << endl;
	cout << endl;
	
	if (!memcached_list(memc, tiles)) goto error;
	
	cout << "Map Tiles:" << endl;
	for (set<map_tile_id_t>::const_iterator i = tiles.begin(); i != tiles.end(); i++)
		cout << "\t[" << i->channel << "," << i->x << "," << i->y << "," << i->z << "]" << endl;
	cout << "\t(Total number of tiles: " << tiles.size() << ")" << endl;
	cout << endl;
	
	memcached_quit(memc);
	memcached_free(memc);
	memc = 0;
	return true;
	
error:
	if (memc)
	{
		memcached_quit(memc);
		memcached_free(memc);
		memc = 0;
	}
	return false;
}


bool memcached_flush(const vector< pair<string, uint16_t> > &servers)
{
	memcached_return mr = MEMCACHED_SUCCESS;
	memcached_st *memc = 0;

	memc = memcached_connect(servers);
	if (!memc) goto error;

	mr = memcached_flush(memc, (time_t)0);
	if (mr != MEMCACHED_SUCCESS) goto error;

	memcached_quit(memc);
	memcached_free(memc);
	memc = 0;
	return true;
	
error:
	if (mr != MEMCACHED_SUCCESS)
	{
		cerr << "ERROR: memcached_flush: " << memcached_strerror(memc, mr) << endl << endl;
	}
	if (memc)
	{
		memcached_quit(memc);
		memcached_free(memc);
		memc = 0;
	}
	return false;
}

bool memcached_load(memcached_st *memc, const map_tile_id_t &id, const map_data_t *data, const uint32_t &length, const uint32_t &revision)
{
	memcached_return mr = MEMCACHED_SUCCESS;
	memcachedmap_key_t tile_data_key, tile_revision_key, tile_lock_key, list_key;
	
	// lock up the tile
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
	
	// set data
	memset(&tile_data_key, 0, sizeof(memcachedmap_key_t));
	tile_data_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	tile_data_key.type = MEMCACHEDMAP_KEY_TYPE_TILE_DATA;
	memcpy(tile_data_key.id, tile_lock_key.id, MEMCACHEDMAP_KEY_ID_SIZE);
	mr = memcached_set(memc, (const char*)&tile_data_key, MEMCACHEDMAP_KEY_WITH_ID_SIZE, (const char*)data, sizeof(map_data_t) * length, (time_t)0, 0);
	if (mr != MEMCACHED_SUCCESS)
		goto error;
	
	// set revision
	memset(&tile_revision_key, 0, sizeof(memcachedmap_key_t));
	tile_revision_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	tile_revision_key.type = MEMCACHEDMAP_KEY_TYPE_TILE_REVISION;
	memcpy(tile_revision_key.id, tile_lock_key.id, MEMCACHEDMAP_KEY_ID_SIZE);
	mr = memcached_set(memc, (const char*)&tile_revision_key, MEMCACHEDMAP_KEY_WITH_ID_SIZE, (const char*)&revision, sizeof(uint32_t), (time_t)0, 0);
	if (mr != MEMCACHED_SUCCESS)
		goto error;

	// add to list of tiles
	memset(&list_key, 0, sizeof(memcachedmap_key_t));
	list_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	list_key.type = MEMCACHEDMAP_KEY_TYPE_LIST;
	mr = memcached_append(memc, (const char*)&list_key, MEMCACHEDMAP_KEY_SIZE, (const char*)&id, sizeof(map_tile_id_t), (time_t)0, 0);
	if (mr != MEMCACHED_SUCCESS)
		goto error;

	// unlock
	mr = memcached_delete(memc, (const char*)&tile_lock_key, MEMCACHEDMAP_KEY_WITH_ID_SIZE, (time_t)0);
	if (mr != MEMCACHED_SUCCESS)
		goto error;

	return true;

error:
	if (mr != MEMCACHED_SUCCESS)
	{
		cerr << "ERROR: memcached_load_tile: " << memcached_strerror(memc, mr) << endl << endl;
	}
	return false;
}

bool memcached_init(memcached_st *memc, const map_info_t &info)
{
	memcached_return mr = MEMCACHED_SUCCESS;
	memcachedmap_key_t info_key, list_key;
	
	memset(&info_key, 0, sizeof(memcachedmap_key_t));
	info_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	info_key.type = MEMCACHEDMAP_KEY_TYPE_INFO;
	mr = memcached_set(memc, (const char*)&info_key, MEMCACHEDMAP_KEY_SIZE, (const char*)&info, sizeof(map_info_t), (time_t)0, 0);
	if (mr != MEMCACHED_SUCCESS)
		goto error;

	memset(&list_key, 0, sizeof(memcachedmap_key_t));
	list_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	list_key.type = MEMCACHEDMAP_KEY_TYPE_LIST;
	mr = memcached_set(memc, (const char*)&list_key, MEMCACHEDMAP_KEY_SIZE, "", 0, (time_t)0, 0);
	if (mr != MEMCACHED_SUCCESS)
		goto error;

	return true;

error:
	if (mr != MEMCACHED_SUCCESS)
	{
		cerr << "ERROR: memcached_init: " << memcached_strerror(memc, mr) << endl << endl;
	}
	return false;
}


bool memcached_save(const char* path, const std::vector< std::pair<std::string, uint16_t> > &servers)
{
	memcached_return mr = MEMCACHED_SUCCESS;
	memcached_st *memc = 0;
	sqlite3 *db = 0;
	map_info_t info = {0}, info2 = {0};
	set<map_tile_id_t> tiles;
	memcachedmap_key_t tile_revision_key, tile_data_key;
	size_t tile_revision_size = 0, tile_data_size = 0;
	uint32_t tile_revision_flags = 0, tile_data_flags = 0;
	char *tile_revision_data = 0, *tile_data_data = 0;

	memc = memcached_connect(servers);
	if (!memc) goto error;
	if (!memcached_info(memc, info))
	{
		cerr << "ERROR: Map not found on server" << endl << endl;
		goto error;
	}
	
	if (db_size(path) < 0)
	{
		db = db_open(path);
		if (!db) goto error;
		if (!db_init(db, info)) goto error;
	}
	else
	{
		db = db_open(path);
		if (!db) goto error;
		if (!db_info(db, info2)) goto error;
	
		if (info.scale != info2.scale ||
			info.tile_width != info2.tile_width ||
			info.tile_height != info2.tile_height ||
			info.tile_depth != info2.tile_depth)
		{
			cerr << "ERROR: Map not compatible, cannot save into an incompatible database" << endl << endl;
			goto error;
		}
	}
	
	if (!memcached_list(memc, tiles)) goto error;

	memset(&tile_revision_key, 0, sizeof(memcachedmap_key_t));
	tile_revision_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	tile_revision_key.type = MEMCACHEDMAP_KEY_TYPE_TILE_REVISION;
	memset(&tile_data_key, 0, sizeof(memcachedmap_key_t));
	tile_data_key.ns = MEMCACHEDMAP_KEY_NAMESPACE;
	tile_data_key.type = MEMCACHEDMAP_KEY_TYPE_TILE_DATA;

	for (set<map_tile_id_t>::const_iterator i = tiles.begin(); i != tiles.end(); i++)
	{
		// get revision number
		map_tile_id_to_hex(*i, tile_revision_key.id);
		tile_revision_data = memcached_get(memc, (const char*)&tile_revision_key, MEMCACHEDMAP_KEY_WITH_ID_SIZE, &tile_revision_size, &tile_revision_flags, &mr);
		if (!tile_revision_data) goto error;
		if (tile_revision_size != sizeof(uint32_t)) goto error;

		// get tile data
		memcpy(tile_data_key.id, tile_revision_key.id, MEMCACHEDMAP_KEY_ID_SIZE);
		tile_data_data = memcached_get(memc, (const char*)&tile_data_key, MEMCACHEDMAP_KEY_WITH_ID_SIZE, &tile_data_size, &tile_data_flags, &mr);
		if (!tile_data_data) goto error;
		if (tile_data_size != sizeof(map_data_t) * info.tile_width * info.tile_height * info.tile_depth) goto error;
		if (!db_save(db, *i, (const map_data_t*)tile_data_data, info.tile_width * info.tile_height * info.tile_depth, *((uint32_t*)tile_revision_data))) goto error;

		free(tile_revision_data);
		tile_revision_data = 0;
		free(tile_data_data);
		tile_data_data = 0;
	}
	
	sqlite3_close(db);
	db = 0;
	memcached_quit(memc);
	memcached_free(memc);
	memc = 0;
	return true;

error:
	if (mr != MEMCACHED_SUCCESS)
	{
		cerr << "ERROR: memcached_load_info: " << memcached_strerror(memc, mr) << endl << endl;
	}
	if (memc)
	{
		memcached_quit(memc);
		memcached_free(memc);
		memc = 0;
	}
	if (db)
	{
		sqlite3_close(db);
		db = 0;
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

