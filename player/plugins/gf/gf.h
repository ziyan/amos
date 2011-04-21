#ifndef AMOS_PLUGINS_GF_H
#define AMOS_PLUGINS_GF_H

#include <libplayercore/playercore.h>

namespace amos
{
	class GapFinderDriver : public ThreadedDriver
	{
	public:
		GapFinderDriver(ConfigFile*, int);
		virtual ~GapFinderDriver();
		virtual int MainSetup();
		virtual void MainQuit();
		virtual int ProcessMessage(QueuePointer&, player_msghdr*, void*);

	protected:
		virtual void Main();
		virtual void FindGap(const float ranges[361], const float max_range);

		player_devaddr_t interface_position2d_addr;
		player_pose2d_t goal;
		bool active;

		player_devaddr_t position2d_addr;
		Device *position2d_dev;
		player_position2d_data_t position2d_data;

		player_devaddr_t laser_addr;
		Device *laser_dev;
	};
}

#endif

