#ifndef AMOS_PLUGINS_PLANNER_H
#define AMOS_PLUGINS_PLANNER_H

#include <libplayercore/playercore.h>
#include <vector>

#include "map/map.h"
#include "astar.h"

namespace amos
{
	class PlannerDriver : public ThreadedDriver
	{
	public:
		PlannerDriver(ConfigFile*, int section);
		virtual ~PlannerDriver();
		virtual int MainSetup();
		virtual void MainQuit();
		virtual int ProcessMessage(QueuePointer&, player_msghdr*, void*);

	private:
		virtual void Main();
		virtual void Plan();
		
		// map
		std::vector< std::pair<std::string, uint16_t> > map_servers;
		Map *map;
		AStarThread *astar;
	
		// devices we provide
		player_devaddr_t planner_addr;
		player_planner_data_t planner;
		bool enabled;
		std::vector<player_pose2d_t> path;

		// devices we require
		player_devaddr_t input_position2d_addr;
		Device *input_position2d_dev;
		
		player_devaddr_t output_position2d_addr;
		Device *output_position2d_dev;
	};
}

#endif

