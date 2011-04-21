#ifndef AMOS_PLUGINS_MD_H
#define AMOS_PLUGINS_MD_H

#include <libplayercore/playercore.h>
#include <string>

namespace amos
{
	class MagneticDeclinationDriver : public ThreadedDriver
	{
	public:
		MagneticDeclinationDriver(ConfigFile*, int);
		virtual ~MagneticDeclinationDriver();
		virtual int MainSetup();
		virtual void MainQuit();
		virtual int ProcessMessage(QueuePointer&, player_msghdr*, void*);
		virtual bool UpdateDeclination(double latitude, double longitude, double elevation);

	protected:
		virtual void Main();

		player_devaddr_t position2d_addr;
		player_position2d_data_t position2d_data;

		player_devaddr_t gps_addr;
		Device *gps_dev;

		float declination;
		std::string filename;
	};
}

#endif // AMOS_PLUGINS_MD_H

