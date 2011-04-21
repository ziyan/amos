#include "cspace.h"
#include <unistd.h>
#include <cmath>
#include <cassert>

using namespace amos;

CSpaceDriver::CSpaceDriver(ConfigFile* cf, int section) : ThreadedDriver(cf, section), map(0)
{
	// read configuration
	radius = cf->ReadFloat(section, "radius", 0.3f);
	buffer = cf->ReadFloat(section, "buffer", 0.5f);

	int map_servers_count = cf->GetTupleCount(section, "maphosts");
	if (map_servers_count > 0)
	{
		assert(map_servers_count == cf->GetTupleCount(section, "mapports"));
		for (int i = 0; i < map_servers_count; i++)
		{
			map_servers.push_back(make_pair(
				std::string(cf->ReadTupleString(section, "maphosts", i, "localhost")),
				(uint16_t)cf->ReadTupleInt(section, "mapports", i, 11211)
			));
		}
	}
	else
	{
		// default map server
		map_servers.push_back(make_pair(std::string("localhost"), (uint16_t)11211));
	}

	// set up the dummy opaque device
	player_devaddr_t dummy_opaque_addr;
	if (cf->ReadDeviceAddr(&dummy_opaque_addr, section, "provides", PLAYER_OPAQUE_CODE, -1, "dummy"))
	{
		PLAYER_ERROR("cspace: cannot find dummy opaque device addr");
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(dummy_opaque_addr))
	{
		PLAYER_ERROR("cspace: cannot add dummy opaque interface");
		this->SetError(-1);
		return;
	}

	PLAYER_MSG0(3, "cspace: initialized");
}

CSpaceDriver::~CSpaceDriver()
{
}

int CSpaceDriver::MainSetup()
{
	PLAYER_MSG0(3, "cspace: setup started");

	map = new Map(map_servers);
	if (!map->isOpen())
	{
		delete map;
		map = 0;
		PLAYER_ERROR("cspace: unable to create map");
		return -1;
	}

	PLAYER_MSG0(3, "cspace: setup complete");

	return 0;
}

void CSpaceDriver::MainQuit()
{
	PLAYER_MSG0(3, "cspace: shutting down");
	// clean-up code goes here
	if (map)
	{
		delete map;
		map = 0;
	}
	PLAYER_MSG0(3, "cspace: shut down");
}


void CSpaceDriver::Main()
{
	PLAYER_MSG0(3, "cspace: thread started");
	for(;;)
	{
		this->TestCancel();
		this->ProcessMessages();

		map->refresh();
		std::set<map_tile_id_t> tiles = map->list(true);
		for (std::set<map_tile_id_t>::const_iterator i = tiles.begin(); i != tiles.end(); i++)
		{
			if (i->channel != MAP_CHANNEL_P) continue;
			this->ProcessTile(*i);
		}
		map->commit();

		usleep(250000);
	}
}


int CSpaceDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data)
{
	return -1;
}

int CSpaceDriver::ProcessTile(const map_tile_id_t &id)
{
	assert(id.channel == MAP_CHANNEL_P);

	uint32_t revision_lt = 0, revision_mt = 0, revision_rt = 0,
			 revision_lm = 0, revision_mm = 0, revision_rm = 0,
			 revision_lb = 0, revision_mb = 0, revision_rb = 0;

	// retrieve relevant tiles
	const map_data_t *tile_lt = map->get((map_tile_id_t) { MAP_CHANNEL_P, id.x - 1,	id.y + 1,	id.z }, &revision_lt);
	const map_data_t *tile_mt = map->get((map_tile_id_t) { MAP_CHANNEL_P, id.x,		id.y + 1,	id.z }, &revision_mt);
	const map_data_t *tile_rt = map->get((map_tile_id_t) { MAP_CHANNEL_P, id.x + 1,	id.y + 1,	id.z }, &revision_rt);
	const map_data_t *tile_lm = map->get((map_tile_id_t) { MAP_CHANNEL_P, id.x - 1,	id.y,		id.z }, &revision_lm);
	const map_data_t *tile_mm = map->get((map_tile_id_t) { MAP_CHANNEL_P, id.x,		id.y,		id.z }, &revision_mm);
	const map_data_t *tile_rm = map->get((map_tile_id_t) { MAP_CHANNEL_P, id.x + 1,	id.y,		id.z }, &revision_rm);
	const map_data_t *tile_lb = map->get((map_tile_id_t) { MAP_CHANNEL_P, id.x - 1,	id.y - 1,	id.z }, &revision_lb);
	const map_data_t *tile_mb = map->get((map_tile_id_t) { MAP_CHANNEL_P, id.x,		id.y - 1,	id.z }, &revision_mb);
	const map_data_t *tile_rb = map->get((map_tile_id_t) { MAP_CHANNEL_P, id.x + 1,	id.y - 1,	id.z }, &revision_rb);

	// just a sanity check
	assert(!((revision_lt && !tile_lt) ||
			(revision_mt && !tile_mt) ||
			(revision_rt && !tile_rt) ||
			(revision_lm && !tile_lm) ||
			(revision_mm && !tile_mm) ||
			(revision_rm && !tile_rm) ||
			(revision_lb && !tile_lb) ||
			(revision_mb && !tile_mb) ||
			(revision_rb && !tile_rb)));

	// a sum of tile revisions will guarantee that if any of the tile changes, the cspace
	// for the center tile will be stale
	uint32_t revision = revision_lt + revision_mt + revision_rt +
				   revision_lm + revision_mm + revision_rm +
				   revision_lb + revision_mb + revision_rb;

	map_tile_id_t cspace_id = { MAP_CHANNEL_P_CSPACE, id.x, id.y, id.z };

	// check revision
	if (revision <= revisions[cspace_id]) return -1;

	PLAYER_MSG3(9, "cspace: process changed tile (%i, %i, %i)", id.x, id.y, id.z);

	// create one larger working copy of the tile
	const uint32_t tile_width = map->getInfo().tile_width;
	const uint32_t tile_height = map->getInfo().tile_height;
	const uint32_t radius_in_pixel = floor(radius / map->getInfo().scale);
	const uint32_t buffer_in_pixel = floor(buffer / map->getInfo().scale);
	const uint32_t total_in_pixel = radius_in_pixel + buffer_in_pixel;
	const uint32_t copy_width = total_in_pixel + map->getInfo().tile_width + total_in_pixel;
	const uint32_t copy_height = total_in_pixel + map->getInfo().tile_height + total_in_pixel;
	map_data_t *original = new map_data_t[copy_width * copy_height];
	memset(original, 0, sizeof(map_data_t) * copy_width * copy_height);


	// copy data over, row by row, faster than regular double loop
	for (uint32_t y = 0; y < total_in_pixel; ++y)
	{
		// left-top
		if (tile_lt)
		{
			memcpy(original + (copy_width * (copy_height - total_in_pixel + y)),
					tile_lt + (tile_width * y + (tile_width - total_in_pixel)),
			sizeof(map_data_t) * total_in_pixel);
		}

		// middle-top
		if (tile_mt)
		{
			memcpy(original + (copy_width * (copy_height - total_in_pixel + y) + total_in_pixel),
					tile_mt + (tile_width * y),
			sizeof(map_data_t) * tile_width);
		}

		// right-top
		if (tile_rt)
		{
			memcpy(original + (copy_width * (copy_height - total_in_pixel + y) + (copy_width - total_in_pixel)),
					tile_rt + (tile_width * y),
			sizeof(map_data_t) * total_in_pixel);
		}

		// left-bottom
		if (tile_lb)
		{
			memcpy(original + (copy_width * y),
					tile_lb + (tile_width * (tile_height - total_in_pixel + y) + (tile_width - total_in_pixel)),
			sizeof(map_data_t) * total_in_pixel);
		}

		// middle-bottom
		if (tile_mb)
		{
			memcpy(original + (copy_width * y + total_in_pixel),
					tile_mb + (tile_width * (tile_height - total_in_pixel + y)),
			sizeof(map_data_t) * tile_width);
		}

		// right-bottom
		if (tile_rb)
		{
			memcpy(original + (copy_width * y + (copy_width - total_in_pixel)),
					tile_rb + (tile_width * (tile_height - total_in_pixel + y)),
			sizeof(map_data_t) * total_in_pixel);
		}
	}

	for (uint32_t y = 0; y < tile_height; ++y)
	{
		// left-middle
		if (tile_lm)
		{
			memcpy(original + (copy_width * (y + total_in_pixel)),
					tile_lm + (tile_width * y + (tile_width - total_in_pixel)),
			sizeof(map_data_t) * total_in_pixel);
		}

		// middle-middle
		if (tile_mm)
		{
			memcpy(original + (copy_width * (y + total_in_pixel) + total_in_pixel),
					tile_mm + (tile_width * y),
			sizeof(map_data_t) * tile_width);
		}

		// right-middle
		if (tile_rm)
		{
			memcpy(original + (copy_width * (y + total_in_pixel) + (copy_width - total_in_pixel)),
					tile_rm + (tile_width * y),
			sizeof(map_data_t) * total_in_pixel);
		}
	}

	// make a copy
	map_data_t *working = new map_data_t[copy_width * copy_height];
	memcpy(working, original, sizeof(map_data_t) * copy_width * copy_height);

	// work on it

	//
	// First we need to build cspace by growing obstacles by radius.
	//
	for (uint32_t i = 0; i < radius_in_pixel; ++i)
	{
		for (int y = 0; y < (int)copy_height; ++y)
		{
			for (int x = 0; x < (int)copy_width; ++x)
			{
				// we make all cells that have a neighbour who has a value > MAP_P_OBSTACLE_THRESHOLD to MAP_CELL_MAX
				if (original[y * copy_width + x] >= MAP_P_OBSTACLE_THRESHOLD ||
					(x + 1 < (int)copy_width && original[y * copy_width + x + 1] >= MAP_P_OBSTACLE_THRESHOLD) ||
					(x - 1 >= 0 && original[y * copy_width + x - 1] >= MAP_P_OBSTACLE_THRESHOLD) ||
					(y + 1 < (int)copy_height && original[(y + 1) * copy_width + x] >= MAP_P_OBSTACLE_THRESHOLD) ||
					(y - 1 >= 0 && original[(y - 1) * copy_width + x] >= MAP_P_OBSTACLE_THRESHOLD))
				{
					working[y * copy_width + x] = MAP_P_MAX;
				}
			}
		}

		// swap original copy with work copy
		map_data_t * temp = original;
		original = working;
		working = temp;
		temp = 0;
	}

	//
	// Now we need to build buffer area.
	//
	for (uint32_t i = 0; i < buffer_in_pixel; ++i)
	{
		for (int y = 0; y < (int)copy_height; ++y)
		{
			for (int x = 0; x < (int)copy_width; ++x)
			{
				// do your magic
				working[y * copy_width + x] = original[y * copy_width + x];

				if (x + 1 < (int)copy_width &&
					original[y * copy_width + x + 1] > MAP_P_OBSTACLE_THRESHOLD &&
					original[y * copy_width + x + 1] > working[y * copy_width + x])
				{
					working[y * copy_width + x] = original[y * copy_width + x + 1] - (MAP_P_MAX - MAP_P_OBSTACLE_THRESHOLD) / (double)buffer_in_pixel;
				}

				if (x - 1 >= 0 &&
					original[y * copy_width + x - 1] > MAP_P_OBSTACLE_THRESHOLD &&
					original[y * copy_width + x - 1] > working[y * copy_width + x])
				{
					working[y * copy_width + x] = original[y * copy_width + x - 1] - (MAP_P_MAX - MAP_P_OBSTACLE_THRESHOLD) / (double)buffer_in_pixel;
				}

				if (y + 1 < (int)copy_height &&
					original[(y + 1) * copy_width + x] > MAP_P_OBSTACLE_THRESHOLD &&
					original[(y + 1) * copy_width + x] > working[y * copy_width + x])
				{
					working[y * copy_width + x] = original[(y + 1) * copy_width + x] - (MAP_P_MAX - MAP_P_OBSTACLE_THRESHOLD) / (double)buffer_in_pixel;
				}

				if (y - 1 >= 0 &&
					original[(y - 1) * copy_width + x] > MAP_P_OBSTACLE_THRESHOLD &&
					original[(y - 1) * copy_width + x] > working[y * copy_width + x])
				{
					working[y * copy_width + x] = original[(y - 1) * copy_width + x] - (MAP_P_MAX - MAP_P_OBSTACLE_THRESHOLD) / (double)buffer_in_pixel;
				}
			}
		}

		// swap original copy with work copy
		map_data_t * temp = original;
		original = working;
		working = temp;
		temp = 0;
	}

	// get the cspace in the middle
	map_data_t *cspace = new map_data_t[map->getTileLength()];
	for (uint32_t y = 0; y < tile_height; ++y)
	{
		memcpy(cspace + (tile_width * y),
				original + (copy_width * (total_in_pixel + y) + total_in_pixel),
				sizeof(map_data_t) * tile_width);
	}

	// update the map
	map->set(cspace_id, cspace);

	// remember the revision so that we don't do this all over again unless there has been a change
	revisions[cspace_id] = revision;

	// clean up resources
	delete [] cspace;
	cspace = 0;

	delete [] original;
	original = 0;

	delete [] working;
	working = 0;

	return 0;
}


Driver* driver_init(ConfigFile* cf, int section) {
	return new CSpaceDriver(cf, section);
}

void driver_register(DriverTable* table) {
	table->AddDriver("amoscspace", driver_init);
}

extern "C" {
	int player_driver_init(DriverTable* table) {
		driver_register(table);
		return 0;
	}
}

