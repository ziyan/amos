#ifndef AMOS_PLUGINS_LOCALIZE_H
#define AMOS_PLUGINS_LOCALIZE_H

#include <libplayercore/playercore.h>

namespace amos
{
	class LocalizeDriver : public ThreadedDriver
	{
	public:
		LocalizeDriver(ConfigFile*, int);
		virtual ~LocalizeDriver();
		virtual int MainSetup();
		virtual void MainQuit();
		virtual int ProcessMessage(QueuePointer&, player_msghdr*, void*);

	protected:
		virtual void Main();

		// output devices
		player_devaddr_t position2d_addr;
		player_position2d_data_t position2d_data;

		// input devices
		player_devaddr_t encoder_addr;
		Device *encoder_dev;
		player_position2d_data_t encoder_data;

		player_devaddr_t gps_addr;
		Device *gps_dev;
		player_gps_data_t gps_data;

		player_devaddr_t compass_addr;
		Device *compass_dev;
		player_position2d_data_t compass_data;

		player_devaddr_t md_addr;
		Device *md_dev;
		player_position2d_data_t md_data;
	};
}

#endif

