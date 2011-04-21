#ifndef AMOS_PLUGINS_MAPPER_H
#define AMOS_PLUGINS_MAPPER_H

#include <libplayercore/playercore.h>
#include <vector>

#include "map/map.h"

namespace amos
{
	class MapperDriver : public ThreadedDriver
	{
	public:
		MapperDriver(ConfigFile*, int);
		virtual ~MapperDriver();
		virtual int MainSetup();
		virtual void MainQuit();
		virtual int ProcessMessage(QueuePointer&, player_msghdr*, void*);

	protected:
		virtual void Main();

		virtual void MapElevation(const float *ranges, const float max);
		virtual void MapVisual(const uint8_t *image, const uint32_t &width, const uint32_t &height);
		virtual void MapProbability(const float *ranges, const float max);

		std::vector< std::pair<std::string, uint16_t> > map_servers;
		Map *map;

		// current position
		player_devaddr_t position2d_addr;
		Device *position2d_dev;
		player_position2d_data_t position2d_data;

		// laser used for elevation mapping
		player_devaddr_t elevation_laser_addr;
		player_pose3d_t elevation_laser_pose;
		Device *elevation_laser_dev;
		player_pose3d_t elevation_laser_previous[361];

		// camera used for visual mapping
		player_devaddr_t visual_camera_addr;
		player_pose3d_t visual_camera_pose;
		Device *visual_camera_dev;
		double visual_camera_hfov, visual_camera_vfov;
		
		// laser used for probability mapping
		player_devaddr_t probability_laser_addr;
		player_pose3d_t probability_laser_pose;
		Device *probability_laser_dev;
		
		// virtual horizontal laser
		player_devaddr_t virtual_laser_addr;
		Device *virtual_laser_dev;
		player_laser_data_t virtual_laser;
		float virtual_laser_ranges[361];
		uint8_t virtual_laser_intensity[361];

		bool ready;
	};
}

#endif // AMOS_PLUGINS_MAPPER_H

