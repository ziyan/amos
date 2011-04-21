// *************************************************************************************************
// include section

#include <cstdlib>
#include <iostream>
#include <string.h>

#include "map/map.h"

using namespace std;
using namespace amos;

// *************************************************************************************************
// usage printing section


void usage(const char *name)
{
	cerr << "Usage: " << name << " <box|line> ..." << endl;
	cerr << endl;
}

void usage_box(const char *name)
{
	cerr << "Usage: " << name << " box <x-min> <x-max> <y-min> <y-max> <value> [host:port] ..." << endl;
	cerr << endl;
}

void usage_line(const char *name)
{
	cerr << "Usage: " << name << " line <origin-x> <origin-y> <direction> <length> <value> [host:port] ..." << endl;
	cerr << endl;
}


// *************************************************************************************************
// action execution

int action_box(int argc, char ** argv)
{
	if (argc < 7)
	{
		usage_box(argv[0]);
		return -1;
	}
	
	const double x_min = atof(argv[2]);
	const double x_max = atof(argv[3]);
	const double y_min = atof(argv[4]);
	const double y_max = atof(argv[5]);
	const float value = atof(argv[6]);
	
	if (x_min >= x_max || y_min >= y_max)
	{
		usage_box(argv[0]);
		return -1;
	}
	
	vector< pair<string, uint16_t> > servers;
	for (int i = 7; i < argc; i++)
	{
		if (!strchr(argv[i], ':'))
		{
			usage_box(argv[0]);
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
	
	// connect to map
	Map map(servers);
	if (!map.isOpen())
	{
		cerr << "ERROR: failed to open map" << endl;
		cerr << endl;
		return -2;
	}
	
	const double scale = map.getInfo().scale;
	
	// draw box
	const int32_t x0 = (int32_t)floor(x_min / scale);
	const int32_t x1 = (int32_t)ceil(x_max / scale);
	const int32_t y0 = (int32_t)floor(y_min / scale);
	const int32_t y1 = (int32_t)ceil(y_max / scale);

	for (int32_t x = x0; x <= x1; x++)
	{
		map.set(MAP_CHANNEL_P, x, y0, 0, value);
		map.set(MAP_CHANNEL_P, x, y1, 0, value);
	}
	
	for (int32_t y = y0; y <= y1; y++)
	{
		map.set(MAP_CHANNEL_P, x0, y, 0, value);
		map.set(MAP_CHANNEL_P, x1, y, 0, value);
	}
	
	map.commit();
	
	return 0;
}

int action_line(int argc, char ** argv)
{
	if (argc < 7)
	{
		usage_line(argv[0]);
		return -1;
	}
	
	const double origin_x = atof(argv[2]);
	const double origin_y = atof(argv[3]);
	const double direction = atof(argv[4]);
	const double length = atof(argv[5]);
	const float value = atof(argv[6]);
	
	if (direction >= M_PI || direction < -M_PI || length <= 0.0)
	{
		usage_line(argv[0]);
		return -1;
	}

	vector< pair<string, uint16_t> > servers;
	for (int i = 7; i < argc; i++)
	{
		if (!strchr(argv[i], ':'))
		{
			usage_line(argv[0]);
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
	
	// connect to map
	Map map(servers);
	if (!map.isOpen())
	{
		cerr << "ERROR: failed to open map" << endl;
		cerr << endl;
		return -2;
	}

	const double scale = map.getInfo().scale;
	
	// draw line
	for (double l = 0.0; l <= length; l += scale / 2.0)
	{
		map.set(MAP_CHANNEL_P, origin_x + l * cos(direction), origin_y + l * sin(direction), 0.0, value);
	}

	map.commit();
	
	return 0;
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
	if (action == "box")
	{
		return action_box(argc, argv);
	}
	else if (action == "line")
	{
		return action_line(argc, argv);
	}
	else
	{
		usage(argv[0]);
		return -1;
	}
	return 0;
}

