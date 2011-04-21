#ifndef AMOS_PLUGINS_PLANNER_ASTAR_H
#define AMOS_PLUGINS_PLANNER_ASTAR_H

#include <vector>
#include <libplayerc++/playerc++.h>
#include "map/map.h"
#include "thread/thread.h"
#include "thread/mutex.h"

namespace amos
{
	class TSPThread: public Thread
	{
	public:
		TSPThread(Map *map, const double accuracy = 0.0);
		virtual ~TSPThread();
		virtual void set(const std::vector<player_pose2d_t> &waypoints);
		virtual void get(std::vector<player_pose2d_t> *path, double *cost);

	protected:
		virtual void run();

		Map *map;
		const double accuracy;
		std::vector<player_pose2d_t> waypoints;
		std::vector<player_pose2d_t> path;
		double cost;
		Mutex mutex;
	};
}
#endif
