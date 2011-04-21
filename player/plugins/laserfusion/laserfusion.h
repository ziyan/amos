#ifndef AMOS_PLUGINS_LASERFUSION_H
#define AMOS_PLUGINS_LASERFUSION_H

#include <libplayercore/playercore.h>

namespace amos
{
	class LaserFusionDriver : public ThreadedDriver
	{
	public:
		LaserFusionDriver(ConfigFile*, int);
		virtual ~LaserFusionDriver();
		virtual int MainSetup();
		virtual void MainQuit();
		virtual int ProcessMessage(QueuePointer&, player_msghdr*, void*);

	protected:
		virtual void Main();
		virtual void Fuse();
		
		player_devaddr_t laser_addr;
		player_laser_data_t laser_data;
		float laser_ranges[361];
		uint8_t laser_intensity[361];

		int lasers_count;
		player_devaddr_t *lasers_addr;
		Device **lasers_dev;
		float *lasers_ranges;
	};
}
#endif

