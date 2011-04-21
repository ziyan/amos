#ifndef AMOS_PLUGINS_APF_H
#define AMOS_PLUGINS_APF_H

#include <libplayercore/playercore.h>

namespace amos
{
	class APFDriver : public ThreadedDriver
	{
	public:
		APFDriver(ConfigFile*, int);
		virtual ~APFDriver();
		virtual int MainSetup();
		virtual void MainQuit();
		virtual int ProcessMessage(QueuePointer&, player_msghdr*, void*);

	protected:
		virtual void Main();
		virtual void APF(const float ranges[361]);

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

