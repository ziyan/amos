#ifndef AMOS_PLUGINS_BASE_H
#define AMOS_PLUGINS_BASE_H

#include <libplayercore/playercore.h>
#include <termios.h>
#include <string>

namespace amos
{
	class BaseDriver : public ThreadedDriver
	{
	public:
		BaseDriver(ConfigFile*, int);
		virtual ~BaseDriver();
		virtual int MainSetup();
		virtual void MainQuit();
		virtual int ProcessMessage(QueuePointer&, player_msghdr*, void*);

	protected:
		virtual void Main();
		virtual int Drive(const float, const float);
		virtual int SetPID(const float, const float, const float, const float, const float, const float);
		virtual int SetAcceleration(const float, const float);
		virtual void UpdateSpeed();
		virtual void UpdateOdometry();
		virtual void UpdatePower();

		player_devaddr_t position2d_addr, position1d_left_addr, position1d_right_addr, power_left_addr, power_right_addr;
		player_position2d_data_t position2d_data;
		player_position1d_data_t position1d_data_left, position1d_data_right;

		int fd; // file descriptor to open serial port
		std::string port; // location of the serial port, read from config
		speed_t baudrate; // baudrate to connect with, read from config

		// holds configured and current p, i, d parameters
		float p_left, i_left, d_left, p_right, i_right, d_right;
		// left and right motor acceleration limit, sets on motor controller
		float a_left, a_right;

		float acceleration_limit_linear, acceleration_limit_angular;
		float speed_limit_linear_forward, speed_limit_linear_reverse;
		float speed_limit_angular, speed_limit_angular_scaling;

		player_position2d_geom_t geom; // dimension of robot

		float speed_left, speed_right; // last set speed on each motor using position1d interface
		bool speed_active;
		float speed_linear, speed_angular, speed_ttl;
	};
}
#endif

