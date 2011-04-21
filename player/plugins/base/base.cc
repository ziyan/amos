#include "base.h"
#include "comm.h"
#include "serial/serial.h"

#include <cmath>
#include <unistd.h>

using namespace std;
using namespace amos;

#define SPEED_TTL 1.0f

BaseDriver::BaseDriver(ConfigFile* cf, int section) :
	ThreadedDriver(cf, section),
	fd(-1), speed_left(0.0f), speed_right(0.0f), speed_active(false)
{
	memset(&position2d_data, 0, sizeof(player_position2d_data_t));
	memset(&position1d_data_left, 0, sizeof(player_position1d_data_t));
	memset(&position1d_data_right, 0, sizeof(player_position1d_data_t));

	// read configurations
	port = cf->ReadString(section, "port", "/dev/motor");
	baudrate = serial_baudrate(cf->ReadInt(section, "baudrate", 115200), B115200);

	p_left = cf->ReadTupleFloat(section, "pid", 0, 0.1f);
	i_left = cf->ReadTupleFloat(section, "pid", 1, 0.0f);
	d_left = cf->ReadTupleFloat(section, "pid", 2, 0.0f);
	p_right = cf->ReadTupleFloat(section, "pid", 3, 0.1f);
	i_right = cf->ReadTupleFloat(section, "pid", 4, 0.0f);
	d_right = cf->ReadTupleFloat(section, "pid", 5, 0.0f);

	a_left = fabs(cf->ReadTupleFloat(section, "acceleration", 0, 0.0f));
	a_right = fabs(cf->ReadTupleFloat(section, "acceleration", 1, 0.0f));

	acceleration_limit_linear = fabs(cf->ReadTupleFloat(section, "accelerationlimit", 0, 1.5f));
	acceleration_limit_angular = fabs(cf->ReadTupleFloat(section, "accelerationlimit", 1, M_PI / 3.0));

	speed_limit_linear_forward = fabs(cf->ReadTupleFloat(section, "speedlimit", 0, 1.5f));
	speed_limit_linear_reverse = fabs(cf->ReadTupleFloat(section, "speedlimit", 1, 0.25f));
	speed_limit_angular = fabs(cf->ReadTupleFloat(section, "speedlimit", 2, M_PI / 4.0));
	speed_limit_angular_scaling = fabs(cf->ReadTupleFloat(section, "speedlimit", 3, 0.5));

	geom.size.sw = fabs(cf->ReadTupleFloat(section, "size", 0, 0.725));
	geom.size.sl = fabs(cf->ReadTupleFloat(section, "size", 1, 1.3));
	geom.size.sh = fabs(cf->ReadTupleFloat(section, "size", 2, 1.0));


	// providing position2d interface
	if (cf->ReadDeviceAddr(&position2d_addr, section, "provides", PLAYER_POSITION2D_CODE, -1, NULL))
	{
		PLAYER_ERROR("base: cannot find position2d addr in provides setting");
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(position2d_addr))
	{
		PLAYER_ERROR("base: cannot add position2d interface");
		this->SetError(-1);
		return;
	}

	if (!cf->ReadDeviceAddr(&position1d_left_addr, section, "provides", PLAYER_POSITION1D_CODE, -1, "left"))
	{
		if (this->AddInterface(position1d_left_addr))
		{
			PLAYER_ERROR("base: cannot add left motor interface");
			this->SetError(-1);
			return;
		}
	}


	if (!cf->ReadDeviceAddr(&position1d_right_addr, section, "provides", PLAYER_POSITION1D_CODE, -1, "right"))
	{
		if (this->AddInterface(position1d_right_addr))
		{
			PLAYER_ERROR("base: cannot add right motor interface");
			this->SetError(-1);
			return;
		}
	}

	if (!cf->ReadDeviceAddr(&power_left_addr, section, "provides", PLAYER_POWER_CODE, -1, "left"))
	{
		if (this->AddInterface(power_left_addr))
		{
			PLAYER_ERROR("base: cannot add left power interface");
			this->SetError(-1);
			return;
		}
	}
	if (!cf->ReadDeviceAddr(&power_right_addr, section, "provides", PLAYER_POWER_CODE, -1, "right"))
	{
		if (this->AddInterface(power_right_addr))
		{
			PLAYER_ERROR("base: cannot add right power interface");
			this->SetError(-1);
			return;
		}
	}

	PLAYER_MSG0(3,"base: initialized");
}

BaseDriver::~BaseDriver()
{
}

int BaseDriver::MainSetup()
{
	PLAYER_MSG0(3,"base: setup begin");
	// setup serial port
	fd = serial_open(port.c_str(), baudrate);
	if(fd < 0)
	{
		PLAYER_ERROR1("base: failed to connect to motor at %s", port.c_str());
		return -1;
	}
	PLAYER_MSG0(3,"base: port opened, baudrate set");

	// sleep 2 seconds to wait for arduino reset
	sleep(2);

	// wait for reset
	int i;
	for (i = 0; i < 20; i++)
	{
		if (!comm_reset(fd)) break;
		usleep(250000);
	}
	if (i == 20)
	{
		PLAYER_ERROR("base: failed to talk to motor, give up");
		return -1;
	}
	// set pid
	this->SetPID(p_left, i_left, d_left, p_right, i_right, d_right);
	// set acceleration
	this->SetAcceleration(a_left, a_right);

	PLAYER_MSG0(3,"base: setup complete");
	return 0;
}

void BaseDriver::MainQuit()
{
	PLAYER_MSG0(3,"base: unsubscribing");
	// rest
	comm_reset(fd);
	// close serial port
	serial_close(fd);
	fd = -1;
	speed_active = false;
	speed_left = 0.0f;
	speed_right = 0.0f;
	PLAYER_MSG0(3,"base: shut down");
}

void BaseDriver::Main()
{
	PLAYER_MSG0(3,"base: main thread started");

	time_t timestamp = time(NULL);

	for(;;)
	{
		this->TestCancel();
		this->ProcessMessages();

		// speed update
		this->UpdateSpeed();

		// odometry reading
		this->UpdateOdometry();

		// power reading does not need to be often
		if (time(NULL) > timestamp)
		{
			timestamp = time(NULL);
			this->UpdatePower();
		}
		usleep(10000);
	}
}

int BaseDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data)
{
	//DATA
	//CMD
	//PLAYER_POSITION2D_CMD_VEL
	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, position2d_addr))
	{
		if (!data || hdr->size != sizeof(player_position2d_cmd_vel_t)) return -1;
		player_position2d_cmd_vel_t* cmd = (player_position2d_cmd_vel_t*)data;
		if (cmd->state)
			this->Drive(cmd->vel.px, cmd->vel.pa);
		else
			this->Drive(0.0f, 0.0f);
		return 0;
	}
	//PLAYER_POSITION2D_CMD_POS
	//PLAYER_POSITION2D_CMD_CAR
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_CAR, position2d_addr))
	{
		if (!data || hdr->size != sizeof(player_position2d_cmd_car_t)) return -1;
		player_position2d_cmd_car_t* cmd = (player_position2d_cmd_car_t*)data;
		this->Drive(cmd->velocity, cmd->angle);
		return 0;
	}
	//PLAYER_POSITION2D_CMD_VEL_HEAD

	//REQ
	//PLAYER_POSITION2D_REQ_GET_GEOM
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_GET_GEOM, position2d_addr))
	{
		if (hdr->size != 0) return -1;
		this->Publish(position2d_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_GET_GEOM, &geom);
		return 0;
	}
	//PLAYER_POSITION2D_REQ_MOTOR_POWER
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_MOTOR_POWER, position2d_addr))
	{
		if (!data || hdr->size != sizeof(player_position2d_power_config_t)) return -1;
		player_position2d_power_config_t* req = (player_position2d_power_config_t*)data;
		if (!req->state)
			this->Drive(0.0f, 0.0f);
		this->Publish(position2d_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_MOTOR_POWER);
		return 0;
	}

	//PLAYER_POSITION2D_REQ_VELOCITY_MODE
	//PLAYER_POSITION2D_REQ_POSITION_MODE
	//PLAYER_POSITION2D_REQ_SET_ODOM
	else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_SET_ODOM, position2d_addr))
	{
		if (!data || hdr->size != sizeof(player_position2d_set_odom_req_t)) return -1;
		player_position2d_set_odom_req_t* req = (player_position2d_set_odom_req_t*)data;
		position2d_data.pos = req->pose;
		this->Publish(position2d_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_SET_ODOM, NULL);
		return 0;
	}
	//PLAYER_POSITION2D_REQ_RESET_ODOM
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_RESET_ODOM, position2d_addr))
	{
		if (hdr->size != 0) return -1;
		position2d_data.pos.px = 0.0;
		position2d_data.pos.py = 0.0;
		position2d_data.pos.pa = 0.0;
		this->Publish(position2d_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_RESET_ODOM, NULL);
		return 0;
	}
	//PLAYER_POSITION2D_REQ_SPEED_PID
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_SPEED_PID, position2d_addr))
	{
		if (!data || hdr->size != sizeof(player_position2d_speed_pid_req_t)) return -1;
		player_position2d_speed_pid_req_t* req = (player_position2d_speed_pid_req_t*)data;

		p_left = req->kp;
		i_left = req->ki;
		d_left = req->kd;
		p_right = req->kp;
		i_right = req->ki;
		d_right = req->kd;

		if (this->SetPID(p_left, i_left, d_left, p_right, i_right, d_right))
		{
			this->Publish(position2d_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_POSITION2D_REQ_SPEED_PID);
			return 0;
		}
		this->Publish(position2d_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_SPEED_PID);
		return 0;
	}
	//PLAYER_POSITION2D_REQ_POSITION_PID
	//PLAYER_POSITION2D_REQ_SPEED_PROF
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_SPEED_PROF, position2d_addr))
	{
		if (!data || hdr->size != sizeof(player_position2d_speed_prof_req_t)) return -1;
		player_position2d_speed_prof_req_t* req = (player_position2d_speed_prof_req_t*)data;

		acceleration_limit_linear = req->acc;
		speed_limit_linear_forward = req->speed;
		speed_limit_linear_reverse = req->speed;

		if (this->SetAcceleration(a_left, a_right))
		{
			this->Publish(position2d_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_POSITION2D_REQ_SPEED_PROF);
			return 0;
		}
		this->Publish(position2d_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_SPEED_PROF);
		return 0;
	}


	//PLAYER_POSITION1D_REQ_GET_GEOM
	//PLAYER_POSITION1D_REQ_MOTOR_POWER
	//PLAYER_POSITION1D_REQ_VELOCITY_MODE
	//PLAYER_POSITION1D_REQ_POSITION_MODE
	//PLAYER_POSITION1D_REQ_SET_ODOM
	//PLAYER_POSITION1D_REQ_RESET_ODOM
	//PLAYER_POSITION1D_REQ_SPEED_PID
	else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_SPEED_PID, position1d_left_addr))
	{
		if (!data || hdr->size != sizeof(player_position1d_speed_pid_req_t)) return -1;
		player_position1d_speed_pid_req_t* req = (player_position1d_speed_pid_req_t*)data;

		p_left = req->kp;
		i_left = req->ki;
		d_left = req->kd;

		if (this->SetPID(p_left, i_left, d_left, p_right, i_right, d_right))
		{
			this->Publish(position1d_left_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_POSITION1D_REQ_SPEED_PID);
			return 0;
		}
		this->Publish(position1d_left_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION1D_REQ_SPEED_PID);
		return 0;
	}
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_SPEED_PID, position1d_right_addr))
	{
		if (!data || hdr->size != sizeof(player_position1d_speed_pid_req_t)) return -1;
		player_position1d_speed_pid_req_t* req = (player_position1d_speed_pid_req_t*)data;

		p_right = req->kp;
		i_right = req->ki;
		d_right = req->kd;

		if (this->SetPID(p_left, i_left, d_left, p_right, i_right, d_right))
		{
			this->Publish(position1d_right_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_POSITION1D_REQ_SPEED_PID);
			return 0;
		}
		this->Publish(position1d_right_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION1D_REQ_SPEED_PID);
		return 0;
	}
	//PLAYER_POSITION1D_REQ_POSITION_PID
	//PLAYER_POSITION1D_REQ_SPEED_PROF
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_SPEED_PROF, position1d_left_addr))
	{
		if (!data || hdr->size != sizeof(player_position1d_speed_prof_req_t)) return -1;
		player_position1d_speed_prof_req_t* req = (player_position1d_speed_prof_req_t*)data;

		a_left = req->acc;

		if (this->SetAcceleration(a_left, a_right))
		{
			this->Publish(position1d_left_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_POSITION1D_REQ_SPEED_PROF);
			return 0;
		}
		this->Publish(position1d_left_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION1D_REQ_SPEED_PROF);
		return 0;
	}
	else  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_SPEED_PROF, position1d_right_addr))
	{
		if (!data || hdr->size != sizeof(player_position1d_speed_prof_req_t)) return -1;
		player_position1d_speed_prof_req_t* req = (player_position1d_speed_prof_req_t*)data;

		a_right = req->acc;

		if (this->SetAcceleration(a_left, a_right))
		{
			this->Publish(position1d_right_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_POSITION1D_REQ_SPEED_PROF);
			return 0;
		}
		this->Publish(position1d_right_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION1D_REQ_SPEED_PROF);
		return 0;
	}

	//PLAYER_POSITION1D_CMD_VEL
	else if(position1d_left_addr.interf && Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION1D_CMD_VEL, position1d_left_addr))
	{
		if (!data || hdr->size != sizeof(player_position1d_cmd_vel_t)) return -1;
		player_position1d_cmd_vel_t* vel_cmd = (player_position1d_cmd_vel_t*)data;
		if (vel_cmd->state)
			speed_left = vel_cmd->vel;
		else
			speed_left = 0.0f;
		speed_active = false;
		if (comm_set_speed(fd, speed_left, speed_right))
			PLAYER_ERROR("base: failed to set speed");
		PLAYER_MSG1(9,"base: left motor speed = %f", speed_left);
		return 0;
	}
	else if(position1d_left_addr.interf && Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION1D_CMD_VEL, position1d_right_addr))
	{
		if (!data || hdr->size != sizeof(player_position1d_cmd_vel_t)) return -1;
		player_position1d_cmd_vel_t* vel_cmd = (player_position1d_cmd_vel_t*)data;
		if (vel_cmd->state)
			speed_right = vel_cmd->vel;
		else
			speed_right = 0.0f;
		speed_active = false;
		if (comm_set_speed(fd, speed_left, speed_right))
			PLAYER_ERROR("base: failed to set speed");
		PLAYER_MSG1(9,"base: right motor speed = %f", speed_right);
		return 0;
	}
	//PLAYER_POSITION1D_CMD_POS
	return -1;
}

int BaseDriver::Drive(const float linear, const float angular)
{
	speed_linear = linear;
	speed_angular = angular;

	// cap linear speed
	if (speed_linear > speed_limit_linear_forward)
		speed_linear = speed_limit_linear_forward;
	else if (speed_linear < -speed_limit_linear_reverse)
		speed_linear = -speed_limit_linear_reverse;

	// scale down angular speed
	if (fabs(speed_linear) > speed_limit_angular_scaling)
		speed_angular *= speed_limit_angular_scaling / fabs(speed_linear);

	// cap angular speed
	if (speed_angular > speed_limit_angular)
		speed_angular = speed_limit_angular;
	else if (speed_angular > speed_limit_angular)
		speed_angular = speed_limit_angular;

	speed_active = true;
	speed_ttl = SPEED_TTL;
	speed_left = 0.0f;
	speed_right = 0.0f;
	return 0;
}

int BaseDriver::SetPID(const float p_left, const float i_left, const float d_left, const float p_right, const float i_right, const float d_right)
{
	PLAYER_MSG3(6,"base: p_left = %f, i_left = %f, d_left = %f", p_left, i_left, d_left);
	PLAYER_MSG3(6,"base: p_right = %f, i_right = %f, d_right = %f", p_right, i_right, d_right);
	if (comm_set_pid(fd, p_left, i_left, d_left, p_right, i_right, d_right))
	{
		PLAYER_ERROR("base: failed to set pid");
		return -1;
	}
	return 0;
}

int BaseDriver::SetAcceleration(const float a_left, const float a_right)
{
	assert(a_left >= 0.0f && a_right >= 0.0f);
	PLAYER_MSG2(6,"base: a_left = %f, a_right = %f", a_left, a_right);
	if (comm_set_acceleration(fd, a_left, a_right))
	{
		PLAYER_ERROR("base: failed to set acceleration");
		return -1;
	}
	return 0;
}

void BaseDriver::UpdateSpeed()
{
	// here we command speed based on acceleration profile

	// static section
	static bool init = false;
	static struct timespec t;
	static float current_speed_linear = 0.0f;
	static float current_speed_angular = 0.0f;
	if (!init)
	{
		clock_gettime(CLOCK_REALTIME, &t);
		init = true;
	}

	// calculate elapsed time using high res clock
	struct timespec t_now;
	clock_gettime(CLOCK_REALTIME, &t_now);
	const double dt = ((double)t_now.tv_sec + (double)t_now.tv_nsec / 1000000000.0) -
				((double)t.tv_sec + (double)t.tv_nsec / 1000000000.0);
	clock_gettime(CLOCK_REALTIME, &t);
	if (dt <= 0.0) return;

	if (speed_ttl > dt)
		speed_ttl -= dt;
	else
	{
		speed_ttl = 0.0f;
		speed_linear = 0.0f;
		speed_angular = 0.0f;
	}

	// is position1d interface being used?
	if (!speed_active)
	{
		current_speed_linear = 0.0f;
		current_speed_angular = 0.0f;
		return;
	}

	// trapezoidal speed control
	if (current_speed_linear != speed_linear)
	{
		// default to no acceleration limit
		if (acceleration_limit_linear <= 0.0f)
		{
			current_speed_linear = speed_linear;
		}
		else if (current_speed_linear < speed_linear)
		{
			if (speed_linear - current_speed_linear > acceleration_limit_linear * dt)
				current_speed_linear += acceleration_limit_linear * dt;
			else
				current_speed_linear = speed_linear;
		}
		else
		{
		   if (current_speed_linear - speed_linear > acceleration_limit_linear * dt)
				current_speed_linear -= acceleration_limit_linear * dt;
			else
				current_speed_linear = speed_linear;
		}
	}

	if (current_speed_angular != speed_angular)
	{
		// default to no acceleration limit
		if (acceleration_limit_angular <= 0.0f)
		{
			current_speed_angular = speed_angular;
		}
		else if (current_speed_angular < speed_angular)
		{
			if (speed_angular - current_speed_angular > acceleration_limit_angular * dt)
				current_speed_angular += acceleration_limit_angular * dt;
			else
				current_speed_angular = speed_angular;
		}
		else
		{
		   if (current_speed_angular - speed_angular > acceleration_limit_angular * dt)
				current_speed_angular -= acceleration_limit_angular * dt;
			else
				current_speed_angular = speed_angular;
		}
	}

	// differential drive
	const float left = -0.5 * geom.size.sw * current_speed_angular + current_speed_linear;
	const float right = 0.5 * geom.size.sw * current_speed_angular + current_speed_linear;

	// command speed on motors
	if (comm_set_speed(fd, left, right))
	{
		PLAYER_ERROR("base: failed to set speed");
	}
}

void BaseDriver::UpdateOdometry()
{
	float left, left_t, right, right_t;
	double dt, forward;

	if (comm_get_odometry(fd, &left, &left_t, &right, &right_t))
	{
		PLAYER_ERROR("base: failed to read odometry");
		return;
	}

	if (left_t == 0.0f && right_t == 0.0f) return;
	PLAYER_MSG2(9, "base: left = %f, right = %f", left, right);

	dt = ((double)left_t + (double)right_t) / 2.0;
	forward = ((double)left + (double)right) / 2.0;

	position1d_data_left.vel = left / left_t;
	position1d_data_left.pos += left;
	position1d_data_right.vel = right / left_t;
	position1d_data_right.pos += right;

	position2d_data.vel.pa = (double)(right - left) / geom.size.sw / dt;
	position2d_data.pos.pa += position2d_data.vel.pa * dt;
	position2d_data.vel.px = forward / dt;
	position2d_data.vel.py = 0.0;

	position2d_data.pos.px += position2d_data.vel.px * dt;
	position2d_data.pos.py += position2d_data.vel.py * dt;

	while(position2d_data.pos.pa >= M_PI) position2d_data.pos.pa -= M_PI + M_PI;
	while(position2d_data.pos.pa < -M_PI) position2d_data.pos.pa += M_PI + M_PI;

	this->Publish(position2d_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, &position2d_data);
	if (position1d_left_addr.interf)
		this->Publish(position1d_left_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION1D_DATA_STATE, &position1d_data_left);
	if (position1d_left_addr.interf)
		this->Publish(position1d_right_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION1D_DATA_STATE, &position1d_data_right);
}

void BaseDriver::UpdatePower()
{
	static player_power_data_t power_left_data = {0}, power_right_data = {0};
	static float current_left = 0.0f, current_right = 0.0f, voltage_left = 0.0f, voltage_right = 0.0f;

	if (comm_get_power(fd, &voltage_left, &current_left, &voltage_right, &current_right))
	{
		PLAYER_ERROR("base: failed to read power");
		return;
	}

	PLAYER_MSG4(9, "base: voltage_left = %f, current_left = %f, voltage_right = %f, current_right = %f",
				voltage_left, current_left, voltage_right, current_right);

	power_left_data.volts = voltage_left;
	power_left_data.watts = voltage_left * current_left;
	power_left_data.percent = 0;
	power_left_data.joules = 0;

	power_right_data.volts = voltage_right;
	power_right_data.watts = voltage_right * current_right;
	power_right_data.percent = 0;
	power_right_data.joules = 0;

	if (power_left_addr.interf)
		this->Publish(power_left_addr, PLAYER_MSGTYPE_DATA, PLAYER_POWER_DATA_STATE, &power_left_data);
	if (power_right_addr.interf)
		this->Publish(power_right_addr, PLAYER_MSGTYPE_DATA, PLAYER_POWER_DATA_STATE, &power_right_data);
}

Driver* driver_init(ConfigFile* cf, int section) {
	return new BaseDriver(cf, section);
}

void driver_register(DriverTable* table) {
	table->AddDriver("amosbase", driver_init);
}

extern "C" {
	int player_driver_init(DriverTable* table) {
		driver_register(table);
		return 0;
	}
}



