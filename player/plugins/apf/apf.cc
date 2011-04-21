#include "apf.h"

#include <cmath>
#include <limits>
#include <unistd.h>

#define LASER_RANGE 4.0

#define GOAL_FORCE_WEIGHT 20.0
#define LASER_FORCE_WEIGHT 1.0

#define GOAL_FORCE_X_WEIGHT GOAL_FORCE_WEIGHT
#define GOAL_FORCE_Y_WEIGHT GOAL_FORCE_WEIGHT
#define LASER_FORCE_X_WEIGHT LASER_FORCE_WEIGHT
#define LASER_FORCE_Y_WEIGHT LASER_FORCE_WEIGHT

#define MAX_SPEED 1.5
#define MAX_TURNRATE (M_PI/6.0)
#define SAFE_TURNRATE (M_PI/9.0)

#define CLEARANCE_DISTANCE 0.1
#define SAFE_DISTANCE 1.5

#define SPEED_TURN_WEIGHT 1.0
#define SPEED_DIST_WEIGHT 1.0

#define GOAL_DISTANCE_THRESHOLD 0.5

using namespace amos;

APFDriver::APFDriver(ConfigFile* cf, int section) :
	ThreadedDriver(cf, section),
	active(false), position2d_dev(0), laser_dev(0)
{
	memset(&position2d_data, 0, sizeof(player_position2d_data_t));

	if (cf->ReadDeviceAddr(&position2d_addr, section, "requires", PLAYER_POSITION2D_CODE, -1, NULL))
	{
		PLAYER_ERROR("apf: cannot find position2d addr");
		this->SetError(-1);
		return;
	}
	if (cf->ReadDeviceAddr(&laser_addr, section, "requires", PLAYER_LASER_CODE, -1, NULL))
	{
		PLAYER_ERROR("apf: cannot find laser addr");
		this->SetError(-1);
		return;
	}

	if (cf->ReadDeviceAddr(&interface_position2d_addr, section, "provides", PLAYER_POSITION2D_CODE, -1, NULL))
	{
		PLAYER_ERROR("apf: cannot find position2d interface");
		this->SetError(-1);
		return;
	}

	if (this->AddInterface(interface_position2d_addr))
	{
		PLAYER_ERROR("apf: cannot add position2d interface");
		this->SetError(-1);
		return;
	}

	PLAYER_MSG0(3,"apf: initialized");
}

APFDriver::~APFDriver()
{
}

int APFDriver::MainSetup()
{
	PLAYER_MSG0(3,"apf: setup begin");
	
	if (!(position2d_dev = deviceTable->GetDevice(position2d_addr)))
	{
		PLAYER_ERROR("apf: unable to locate suitable position2d device");
		return -1;
	}
	if (position2d_dev->Subscribe(InQueue))
	{
		PLAYER_ERROR("apf: unable to subscribe to position2d device");
		position2d_dev = 0;
		return -1;
	}

	if (!(laser_dev = deviceTable->GetDevice(laser_addr)))
	{
		PLAYER_ERROR("apf: unable to locate suitable laser device");
		return -1;
	}
	if (laser_dev->Subscribe(InQueue))
	{
		PLAYER_ERROR("apf: unable to subscribe to laser device");
		laser_dev = 0;
		return -1;
	}
	PLAYER_MSG0(3,"apf: setup completed");
	return 0;
}

void APFDriver::MainQuit()
{
	PLAYER_MSG0(3,"apf: shutting down");
	active = false;
	if (position2d_dev)
	{
		// stop motors
		player_position2d_cmd_vel cmd = {{0.0, 0.0, 0.0}, false};
		position2d_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, &cmd, sizeof(player_position2d_cmd_vel), NULL);
		position2d_dev->Unsubscribe(this->InQueue);
		position2d_dev = 0;
	}
	if (laser_dev)
	{
		laser_dev->Unsubscribe(this->InQueue);
		laser_dev = 0;
	}
	PLAYER_MSG0(3,"apf: shutted down");
}


void APFDriver::Main()
{
	PLAYER_MSG0(3,"apf: thread started");
	
	assert(!active);
	
	for(;;)
	{
		this->TestCancel();
		this->ProcessMessages();
		usleep(10000);
	}
}


int APFDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data)
{
	// received position2d update
	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, position2d_addr))
	{
		if (!data || hdr->size != sizeof(player_position2d_data_t)) return -1;
		position2d_data = *((player_position2d_data_t*)data);
		this->Publish(interface_position2d_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, &position2d_data, sizeof(player_position2d_data_t), NULL);
		
		// have we reached the goal yet?
		if (hypot(goal.px - position2d_data.pos.px, goal.py - position2d_data.pos.py) < GOAL_DISTANCE_THRESHOLD)
		{
			active = false;
			
			// stop motors
			player_position2d_cmd_vel cmd = {{0.0, 0.0, 0.0}, false};
			position2d_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, &cmd, sizeof(player_position2d_cmd_vel), NULL);
			return 0;
		}
		return 0;
	}
	// received laser scan
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN, laser_addr))
	{
		if (!data) return -1;
		if (!active) return 0;
		player_laser_data_t *d = (player_laser_data_t*)data;
		if (d->ranges_count != 361 || d->max_range != 8.0f)
		{
			PLAYER_ERROR("apf: laser data format not supported.");
			return -1;
		}
		
		this->APF(d->ranges);
		return 0;
	}
	else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_POS, interface_position2d_addr))
	{
		if (!data || hdr->size != sizeof(player_position2d_cmd_pos_t)) return -1;

		// TODO: right now we ignore vel in PLAYER_POSITION2D_CMD_POS, it is supposed to control
		// how fast we approach our goal
		goal = ((player_position2d_cmd_pos_t*)data)->pos;

		if (!((player_position2d_cmd_pos_t*)data)->state)
		{
			active = false;

			// stop motors
			player_position2d_cmd_vel cmd = {{0.0, 0.0, 0.0}, false};
			position2d_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, &cmd, sizeof(player_position2d_cmd_vel), NULL);
			return 0;
		}
		else
		{
			active = true;
			return 0;
		}
	}
	else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, -1, interface_position2d_addr))
	{
		// any other command stops apf
		active = false;

		// pass thru commands
		player_msghdr_t newhdr = *hdr;
		newhdr.addr = interface_position2d_addr;
		position2d_dev->PutMsg(this->InQueue, &newhdr, data);
		return 0;
	}
	else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, interface_position2d_addr))
	{
		// pass thru requests
		Message* msg = position2d_dev->Request(this->InQueue, hdr->type, hdr->subtype, data, hdr->size, &hdr->timestamp);
		if (!msg) return -1;
		player_msghdr_t* rephdr = msg->GetHeader();
		void* repdata = msg->GetPayload();
		rephdr->addr = interface_position2d_addr;
		this->Publish(resp_queue, rephdr, repdata);
		delete msg;
		return 0;
	}

	return -1;
}

void APFDriver::APF(const float *ranges)
{
	// static constants section
	static const double speed_turn_e = exp(-SPEED_TURN_WEIGHT);
	static const double speed_turn_d = 1.0 - speed_turn_e;
	static const double speed_dist_d = exp(SPEED_DIST_WEIGHT) - 1.0;

	static double laser_angle[361];
	static bool init = false;
	if (!init)
	{
		// build a table of laser tilt angle
		for(int i = 0; i < 361; i++)
		{
			laser_angle[i] = DTOR((double)i * 0.5 - 90.0);
		}
		init = true;
	}
	
	// laser force
	uint16_t laser_force_count = 0;
	double laser_force_x = 0.0, laser_force_y = 0.0;
	float min_dist = std::numeric_limits<float>::infinity();

	for(uint16_t i = 0; i < 361; i++)
	{
		if (ranges[i] < LASER_RANGE && fabs(laser_angle[i]) < M_PI / 3.0)
		{
			// calculate laser force
			if (fabs(laser_angle[i]) < M_PI / 12.0 && ranges[i] < min_dist)
				min_dist = ranges[i];

			double force = 1.0 / ((ranges[i] + 0.00000001) / LASER_RANGE);
			laser_force_y += sin(laser_angle[i]) * force;
			laser_force_x += cos(laser_angle[i]) * force;
			laser_force_count ++;
		}
	}

	// average laser force
	if(laser_force_count > 0)
	{
		laser_force_x /= (double)laser_force_count;
		laser_force_y /= (double)laser_force_count;
	}
	
	// calculate goal force
	const double goal_dist = hypot(goal.px - position2d_data.pos.px, goal.py - position2d_data.pos.py);
	if (min_dist > goal_dist) min_dist = goal_dist;
	
	const double goal_force = 1.0;// / (goal_dist + 1.0);
	double goal_force_angle = atan2(goal.py - position2d_data.pos.py,
									goal.px - position2d_data.pos.px) - position2d_data.pos.pa;
	while(goal_force_angle >= M_PI) goal_force_angle -= M_PI + M_PI;
	while(goal_force_angle < -M_PI) goal_force_angle += M_PI + M_PI;
	const double goal_force_x = goal_force * cos(goal_force_angle);
	const double goal_force_y = goal_force * sin(goal_force_angle);

	// sum up forces
	const double total_force_x = goal_force_x * GOAL_FORCE_X_WEIGHT - laser_force_x * LASER_FORCE_X_WEIGHT;
	const double total_force_y = goal_force_y * GOAL_FORCE_Y_WEIGHT - laser_force_y * LASER_FORCE_Y_WEIGHT;

	PLAYER_MSG6(9, "apf: goal force = (%f, %f), laser force = (%f, %f), total force = (%f, %f)",
				goal_force_x, goal_force_y,
				laser_force_x, laser_force_y,
				total_force_x, total_force_y);


	// find turn
	double turn = atan2(total_force_y, total_force_x);
	while(turn >= M_PI) turn -= M_PI + M_PI;
	while(turn < -M_PI) turn += M_PI + M_PI;

	// heuristic speed
	double speed_dist = 0.0;
	double speed_turn = 0.0;
	
	if (fabs(turn) > MAX_TURNRATE)
	{
		// turning in place
		speed_turn = 0.0;
	}
	else if (fabs(turn) > SAFE_TURNRATE)
	{
		// if turn rate is bigger than safe turn rate, scale down speed
		speed_turn = (fabs(turn) - SAFE_TURNRATE) / (MAX_TURNRATE - SAFE_TURNRATE);
		speed_turn = (exp(-SPEED_TURN_WEIGHT * speed_turn) - speed_turn_e) / speed_turn_d;
		speed_turn *= MAX_SPEED;
	}
	else
	{
		speed_turn = MAX_SPEED;
	}
	
	
	if (min_dist < CLEARANCE_DISTANCE)
	{
		// stop, getting too close
		speed_dist = 0.0;
	}
	else if (min_dist < SAFE_DISTANCE)
	{
		// if within safe distance, scale down the speed
		speed_dist = (min_dist - CLEARANCE_DISTANCE) / (SAFE_DISTANCE - CLEARANCE_DISTANCE);
		speed_dist = (exp(SPEED_DIST_WEIGHT * speed_dist) - 1.0) / speed_dist_d;
		speed_dist *= MAX_SPEED;
	}
	else
	{
		speed_dist = MAX_SPEED;
	}
	
	PLAYER_MSG2(9, "apf: speed_turn = %f, speed_dist = %f", speed_turn, speed_dist);
	
	if (turn > MAX_TURNRATE) turn = MAX_TURNRATE;
	if (turn < -MAX_TURNRATE) turn = -MAX_TURNRATE;
	
	const double speed = (speed_dist < speed_turn) ? speed_dist : speed_turn;
	
	// command motors
	player_position2d_cmd_vel cmd = {{speed, 0.0, turn}, true};
	position2d_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, &cmd, sizeof(player_position2d_cmd_vel), NULL);
	
	PLAYER_MSG2(9, "apf: speed = %f, turn = %f", speed, turn);
}

Driver* driver_init(ConfigFile* cf, int section) {
	return new APFDriver(cf, section);
}

void driver_register(DriverTable* table) {
	table->AddDriver("amosapf", driver_init);
}

extern "C" {
	int player_driver_init(DriverTable* table) {
		driver_register(table);
		return 0;
	}
}

