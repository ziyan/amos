#ifndef AMOS_PLUGINS_COMPASS_H
#define AMOS_PLUGINS_COMPASS_H

#include <libplayercore/playercore.h>
#include <termios.h>
#include <string>

namespace amos
{
	class CompassDriver : public ThreadedDriver
	{
	public:
		CompassDriver(ConfigFile*, int);
		virtual ~CompassDriver();
		virtual int MainSetup();
		virtual void MainQuit();
		virtual int ProcessMessage(QueuePointer&, player_msghdr*, void*);

	protected:
		virtual void Main();

		player_devaddr_t position2d_addr;

		int fd;
		speed_t baudrate;
		std::string port;
	};
}

#endif // AMOS_PLUGINS_COMPASS_H

