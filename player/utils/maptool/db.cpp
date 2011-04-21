// *************************************************************************************************
// include section

#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <assert.h>

#include "maptool.h"

using namespace std;

// *************************************************************************************************
// implementation section

sqlite3 *db_open(const char* path)
{
	sqlite3 *db = 0;
	if (sqlite3_open(path, &db))
	{
		cerr << "ERROR: Failed to open map database: " << sqlite3_errmsg(db) << endl;
		cerr << endl;
		sqlite3_close(db);
		db = 0;
		return 0;
	}
	
	//
	// Create inital tables, including info table for readonly
	// map information, tiles table for map tiles.
	//
	if (sqlite3_exec(db, "BEGIN TRANSACTION; \
			CREATE TABLE IF NOT EXISTS `tiles` (`channel` integer, `x` integer, `y` integer, `z` integer, `data` blob, `revision` integer, primary key (`channel`, `x`, `y`, `z`)); \
			CREATE TABLE IF NOT EXISTS `info` (`scale` real, `tile_width` integer, `tile_height` integer, `tile_depth` integer); \
			COMMIT;", 0, 0, 0) != SQLITE_OK)
	{
		cerr << "ERROR: Failed to open map database: " << sqlite3_errmsg(db) << endl;
		sqlite3_close(db);
		db = 0;
		return 0;
	}
	return db;
}

int db_size(const char* path)
{
	struct stat file_stat;
	if (stat(path, &file_stat))
	{
		return -1;
	}
	return file_stat.st_size;
}

bool db_info(sqlite3 *db, map_info_t &info)
{
	int rc = 0;
	sqlite3_stmt *stmt = 0;
	
	rc = sqlite3_prepare_v2(db, "SELECT `scale`, `tile_width`, `tile_height`, `tile_depth` FROM `info` LIMIT 1", -1, &stmt, 0);
	if (rc != SQLITE_OK || stmt == 0)
	{
		cerr << "ERROR: Failed to query map information: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}

	rc = sqlite3_step(stmt);
	if (rc == SQLITE_ROW)
	{
		info.scale = sqlite3_column_double(stmt, 0);
		info.tile_width = sqlite3_column_int(stmt, 1);
		info.tile_height = sqlite3_column_int(stmt, 2);
		info.tile_depth = sqlite3_column_int(stmt, 3);

		while (rc == SQLITE_ROW)
		{
			rc = sqlite3_step(stmt);
		}

		if (rc != SQLITE_DONE)
		{
			cerr << "ERROR: Failed to query map information: " << sqlite3_errmsg(db) << endl << endl;
			goto error;
		}
		sqlite3_finalize(stmt);
		stmt = 0;
	}
	else
	{
		cerr << "ERROR: Failed to query map information: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}
	
	return true;

error:
	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = 0;
	}
	return false;
}

bool db_init(sqlite3 *db, const map_info_t &info)
{
	int rc = 0;
	ostringstream oss;
	
	if (!db) goto error;
	
	oss << "BEGIN TRANSACTION; \
			DELETE FROM `info`; \
			INSERT INTO `info` (`scale`, `tile_width`, `tile_height`, `tile_depth`) VALUES (" << info.scale << ", " << info.tile_width << ", " << info.tile_height << ", " << info.tile_depth << "); \
			COMMIT;";
	rc = sqlite3_exec(db, oss.str().c_str(), 0, 0, 0);
	if (rc != SQLITE_OK)
	{
		cerr << "ERROR: Failed to initialize map information: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}
	
	return true;

error:
	return false;
}

bool db_create(const char* path, const map_info_t &info)
{
	sqlite3 *db = 0;
	
	//
	// Check if file exists.
	//
	if (db_size(path) >= 0)
	{
		cerr << "ERROR: File already exists." << endl << endl;
		goto error;
	}
	
	//
	// Open/create database.
	//
	db = db_open(path);
	if (!db) goto error;
	if (!db_init(db, info)) goto error;
	
	sqlite3_close(db);
	db = 0;
	return true;
	
error:
	if (db)
	{
		sqlite3_close(db);
		db = 0;
	}
	return false;
}

bool db_show(const char* path)
{
	int rc = 0;
	sqlite3 *db = 0;
	sqlite3_stmt *stmt = 0;
	int size = 0;
	map_info_t info = {0};
	int count = 0;
	
	//
	// Check if file exists.
	//
	size = db_size(path);
	if (size < 0)
	{
		cerr << "ERROR: Database file does not exist." << endl << endl;
		goto error;
	}
	
	//
	// Open/create database.
	//
	db = db_open(path);
	if (!db)
	{
		goto error;
	}
	
	cout << "Map Database:" << endl;
	cout << "\t" << path << " (Size: " << size << " bytes)" << endl;
	cout << endl;
	
	//
	// Query map information, including scale (meters per pixel)
	// tile_width, tile_height.
	//

	if (!db_info(db, info)) goto error;

	cout << "Map Information:" << endl;
	cout << "\tResolution: " << info.scale << " (meters/pixel)" << endl;
	cout << "\tTile Size: [" << info.tile_width << "," << info.tile_height << "," << info.tile_depth << "] (pixels)" << endl;
	cout << endl;
	
	//
	//
	// Query tiles
	//
	cout << "Map Tiles:" << endl;

	rc = sqlite3_prepare_v2(db, "SELECT `channel`, `x`, `y`, `z` FROM `tiles`", -1, &stmt, 0);
	if (rc != SQLITE_OK || stmt == 0)
	{
		cerr << "ERROR: Failed to query map tiles: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}

	rc = sqlite3_step(stmt);
	if (rc == SQLITE_ROW)
	{
		while (rc == SQLITE_ROW)
		{
			count++;
			cout << "\t[" << sqlite3_column_int(stmt, 0) << "," << sqlite3_column_int(stmt, 1) << "," << sqlite3_column_int(stmt, 2) << "," << sqlite3_column_int(stmt, 3) << "]" << endl;	
			rc = sqlite3_step(stmt);
		}
		
		if (rc != SQLITE_DONE) {
			cerr << "ERROR: Failed to query map tiles: " << sqlite3_errmsg(db) << endl << endl;
			goto error;
		}
		sqlite3_finalize(stmt);
		stmt = 0;
	}
	else if (rc == SQLITE_DONE)
	{
		sqlite3_finalize(stmt);
		stmt = 0;
	}
	else
	{
		cerr << "ERROR: Failed to query map tiles: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}
	
	cout << "\t(Total number of tiles: " << count << ")" << endl;
	cout << endl;
	
	sqlite3_close(db);
	db = 0;
	return true;
	
error:
	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = 0;
	}
	if (db)
	{
		sqlite3_close(db);
		db = 0;
	}
	return false;
}

bool db_load(const char* path, const vector< pair<string, uint16_t> > &servers)
{
	int rc = 0;
	sqlite3 *db = 0;
	sqlite3_stmt *stmt = 0;
	memcached_st *memc = 0;
	map_info_t info = {0};

	if (db_size(path) < 0)
	{
		cerr << "ERROR: Database file does not exist." << endl << endl;
		goto error;
	}
	
	db = db_open(path);
	if (!db) goto error;
	
	memc = memcached_connect(servers);
	if (!memc) goto error;
	if (memcached_info(memc, info))
	{
		cerr << "ERROR: Map already loaded on the server." << endl << endl;
		goto error;
	}
	
	if (!db_info(db, info)) goto error;
	if (!memcached_init(memc, info)) goto error;
	
	rc = sqlite3_prepare_v2(db, "SELECT `channel`, `x`, `y`, `z`, `data`, `revision` FROM `tiles`", -1, &stmt, 0);
	if (rc != SQLITE_OK || stmt == 0)
	{
		cerr << "ERROR: Failed to query map tiles: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}

	rc = sqlite3_step(stmt);
	if (rc == SQLITE_ROW)
	{
		while (rc == SQLITE_ROW)
		{
			if (!memcached_load(memc,
					(map_tile_id_t) { sqlite3_column_int(stmt, 0),
						sqlite3_column_int(stmt, 1),
						sqlite3_column_int(stmt, 2),
						sqlite3_column_int(stmt, 3) },
					(const map_data_t*)sqlite3_column_blob(stmt, 4),
					info.tile_width * info.tile_height * info.tile_depth,
					sqlite3_column_int(stmt, 5)
				)) goto error;
			
			rc = sqlite3_step(stmt);
		}
		
		if (rc != SQLITE_DONE)
		{
			cerr << "ERROR: Failed to query map tiles: " << sqlite3_errmsg(db) << endl << endl;
			goto error;
		}
		sqlite3_finalize(stmt);
		stmt = 0;
	}
	else if (rc == SQLITE_DONE)
	{
		sqlite3_finalize(stmt);
		stmt = 0;
	}
	else
	{
		cerr << "ERROR: Failed to query map tiles: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}
	
	sqlite3_close(db);
	db = 0;

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
	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = 0;
	}
	if (db)
	{
		sqlite3_close(db);
		db = 0;
	}
	return false;
}

bool db_save(sqlite3 *db, const map_tile_id_t &id, const map_data_t *data, const uint32_t &length, const uint32_t &revision)
{
	int rc = 0;
	sqlite3_stmt *stmt = 0;
	
	if (!db) goto error;

	rc = sqlite3_prepare_v2(db, "REPLACE INTO `tiles` (`channel`, `x`, `y`, `z`, `data`, `revision`) VALUES (?, ?, ?, ?, ?, ?);", -1, &stmt, 0);
	if (rc != SQLITE_OK || stmt == 0)
	{
		cerr << "ERROR: Failed to save map tiles: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}
	
	
	rc = sqlite3_bind_int(stmt, 1, id.channel);
	if (rc != SQLITE_OK)
	{
		cerr << "ERROR: Failed to save map tiles: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}
	
	rc = sqlite3_bind_int(stmt, 2, id.x);
	if (rc != SQLITE_OK)
	{
		cerr << "ERROR: Failed to save map tiles: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}

	rc = sqlite3_bind_int(stmt, 3, id.y);
	if (rc != SQLITE_OK)
	{
		cerr << "ERROR: Failed to save map tiles: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}
	
	rc = sqlite3_bind_int(stmt, 4, id.z);
	if (rc != SQLITE_OK)
	{
		cerr << "ERROR: Failed to save map tiles: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}
	
	rc = sqlite3_bind_blob(stmt, 5, data, sizeof(map_data_t) * length, 0);
	if (rc != SQLITE_OK)
	{
		cerr << "ERROR: Failed to save map tiles: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}
	
	rc = sqlite3_bind_int(stmt, 6, revision);
	if (rc != SQLITE_OK)
	{
		cerr << "ERROR: Failed to save map tiles: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE)
	{
		cerr << "ERROR: Failed to save map tiles: " << sqlite3_errmsg(db) << endl << endl;
		goto error;
	}
	
	sqlite3_finalize(stmt);
	stmt = 0;
	return true;
	
error:
	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = 0;
	}
	return false;
}


