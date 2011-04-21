#ifndef AMOS_COMMON_ASTAR_H
#define AMOS_COMMON_ASTAR_H

#include <libplayercore/playercore.h>
#include <vector>
#include "map/map.h"

bool astar_search(
	amos::Map *map,
	const player_pose2d_t &begin,
	const player_pose2d_t &end,
	std::vector<player_pose2d_t> *path = 0,
	double *cost = 0,
	const double accuracy = 0.0
);

#endif

