// *************************************************************************************************
// include section

#include <cstdlib>
#include <iostream>
#include <string.h>

#include "maptool.h"

using namespace std;

// *************************************************************************************************
// usage printing section

void usage(const char *name)
{
	cerr << "Usage: " << name << " <create|show|load|save|flush> ..." << endl;
	cerr << endl;
}

void usage_create(const char *name)
{
	cerr << "Usage: " << name << " create <db-path> [resolution] [tile-width] [tile-height] [tile-depth]" << endl;
	cerr << endl;
	cerr << "[resolution]: meter per pixel" << endl;
	cerr << "[tile-width]: in pixels" << endl;
	cerr << "[tile-height]: in pixels" << endl;
	cerr << "[tile-depth]: in pixels" << endl;
	cerr << endl;
}

void usage_show(const char *name)
{
	cerr << "Usage: " << name << " show [dbpath] [host:port] ..." << endl;
	cerr << endl;
}

void usage_load(const char *name)
{
	cerr << "Usage: " << name << " load <dbpath> [host:port] ..." << endl;
	cerr << endl;
}

void usage_save(const char *name)
{
	cerr << "Usage: " << name << " save <dbpath> [host:port] ..." << endl;
	cerr << endl;
}

void usage_flush(const char *name)
{
	cerr << "Usage: " << name << " flush [host:port] ..." << endl;
	cerr << endl;
}


// *************************************************************************************************
// action execution

int action_create(int argc, char ** argv)
{
	map_info_t info = {0.05, 256, 256, 1};

	if (argc < 3)
	{
		usage_create(argv[0]);
		return -1;
	}
	
	if (argc > 3) info.scale = atof(argv[3]);
	if (argc > 4) info.tile_width = atoi(argv[4]);
	if (argc > 5) info.tile_height = atoi(argv[5]);
	if (argc > 6) info.tile_depth = atoi(argv[6]);
	
	if (info.scale <= 0.0f)
	{
		usage_create(argv[0]);
		return -1;
	}

	return db_create(argv[2], info) ? 0 : -2;
}

int action_show(int argc, char ** argv)
{
	if (argc == 3 && !strchr(argv[2], ':'))
	{
		return db_show(argv[2]) ? 0 : -2;
	}
	else
	{
		vector< pair<string, uint16_t> > servers;
		for (int i = 2; i < argc; i++)
		{
			if (!strchr(argv[i], ':'))
			{
				usage_show(argv[0]);
				return -1;
			}
			
			servers.push_back(make_pair(
				string(argv[i], strchr(argv[i], ':')),
				(uint16_t)atoi(strchr(argv[i], ':') + 1)
			));
		}
		
		if (servers.empty())
		{
			servers.push_back(make_pair(
				string("localhost"),
				(uint16_t)11211
			));
		}
		
		return memcached_show(servers) ? 0 : -2;	
	}
}

int action_load(int argc, char ** argv)
{
	if (argc < 3)
	{
		usage_load(argv[0]);
		return -1;
	}
	
	vector< pair<string, uint16_t> > servers;
	for (int i = 3; i < argc; i++)
	{
		if (!strchr(argv[i], ':'))
		{
			usage_load(argv[0]);
			return -1;
		}
		
		servers.push_back(make_pair(
			string(argv[i], strchr(argv[i], ':')),
			(uint16_t)atoi(strchr(argv[i], ':') + 1)
		));
	}
	
	if (servers.empty())
	{
		servers.push_back(make_pair(
			string("localhost"),
			(uint16_t)11211
		));
	}
	
	return db_load(argv[2], servers) ? 0 : -2;
}

int action_save(int argc, char ** argv)
{
	if (argc < 3)
	{
		usage_save(argv[0]);
		return -1;
	}
	
	vector< pair<string, uint16_t> > servers;
	for (int i = 3; i < argc; i++)
	{
		if (!strchr(argv[i], ':'))
		{
			usage_save(argv[0]);
			return -1;
		}
		
		servers.push_back(make_pair(
			string(argv[i], strchr(argv[i], ':')),
			(uint16_t)atoi(strchr(argv[i], ':') + 1)
		));
	}
	
	if (servers.empty())
	{
		servers.push_back(make_pair(
			string("localhost"),
			(uint16_t)11211
		));
	}
	
	return memcached_save(argv[2], servers) ? 0 : -2;
}

int action_flush(int argc, char ** argv)
{
	vector< pair<string, uint16_t> > servers;
	for (int i = 2; i < argc; i++)
	{
		if (!strchr(argv[i], ':'))
		{
			usage_flush(argv[0]);
			return -1;
		}
		
		servers.push_back(make_pair(
			string(argv[i], strchr(argv[i], ':')),
			(uint16_t)atoi(strchr(argv[i], ':') + 1)
		));
	}
	
	if (servers.empty())
	{
		servers.push_back(make_pair(
			string("localhost"),
			(uint16_t)11211
		));
	}
	
	return memcached_flush(servers) ? 0 : -2;
}

// *************************************************************************************************
// main

int main(int argc, char ** argv)
{
	if (argc < 2)
	{
		usage(argv[0]);
		return -1;
	}

	string action = argv[1];
	if (action == "create")
	{
		return action_create(argc, argv);
	}
	else if (action == "show")
	{
		return action_show(argc, argv);
	}
	else if (action == "load")
	{
		return action_load(argc, argv);
	}
	else if (action == "save")
	{
		return action_save(argc, argv);
	}
	else if (action == "flush")
	{
		return action_flush(argc, argv);
	}
	else
	{
		usage(argv[0]);
		return -1;
	}
	return 0;
}

