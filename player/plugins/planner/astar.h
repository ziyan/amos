#ifndef AMOS_PLUGINS_PLANNER_ASTAR_H
#define AMOS_PLUGINS_PLANNER_ASTAR_H

#include <vector>
#include <libplayercore/playercore.h>
#include "map/map.h"
#include "thread/thread.h"
#include "thread/mutex.h"

namespace amos
{
	class AStarThread: public Thread
	{
	public:
		AStarThread(Map *map, const double accuracy = 0.0);
		virtual ~AStarThread();
		virtual void set(const player_pose2d_t &begin, const player_pose2d_t &end);
		virtual void get(std::vector<player_pose2d_t> *path);

	protected:
		virtual void run();
		
		Map *map;
		const double accuracy;
		player_pose2d_t begin, end;
		std::vector<player_pose2d_t> path;
		Mutex mutex;
	};
}
#endif
