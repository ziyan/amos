#include "gf.h"

#include <cmath>
#include <limits>
#include <unistd.h>

#define DEAD_ZONE 30
#define ROBOT_WIDTH 0.3
#define GAP_BUFFER 0.1

#define MAX_SPEED 0.4
#define MAX_TURNRATE (M_PI/4.0)
#define SAFE_TURNRATE (M_PI/6.0)

#define CLEARANCE_DISTANCE 0.1
#define SAFE_DISTANCE 1.0

#define SPEED_TURN_WEIGHT 1.0
#define SPEED_DIST_WEIGHT 1.0

#define GOAL_DISTANCE_THRESHOLD 0.5

using namespace amos;

GapFinderDriver::GapFinderDriver(ConfigFile* cf, int section) :
	ThreadedDriver(cf, section),
	active(false), position2d_dev(0), laser_dev(0)
{
	memset(&position2d_data, 0, sizeof(player_position2d_data_t));

	if (cf->ReadDeviceAddr(&position2d_addr, section, "requires", PLAYER_POSITION2D_CODE, -1, NULL))
	{
		PLAYER_ERROR("gf: cannot find position2d addr");
		this->SetError(-1);
		return;
	}
	if (cf->ReadDeviceAddr(&laser_addr, section, "requires", PLAYER_LASER_CODE, -1, NULL))
	{
		PLAYER_ERROR("gf: cannot find laser addr");
		this->SetError(-1);
		return;
	}

	if (cf->ReadDeviceAddr(&interface_position2d_addr, section, "provides", PLAYER_POSITION2D_CODE, -1, NULL))
	{
		PLAYER_ERROR("gf: cannot find position2d interface");
		this->SetError(-1);
		return;
	}

	if (this->AddInterface(interface_position2d_addr))
	{
		PLAYER_ERROR("gf: cannot add position2d interface");
		this->SetError(-1);
		return;
	}

	PLAYER_MSG0(3,"gf: initialized");
}

GapFinderDriver::~GapFinderDriver()
{
}

int GapFinderDriver::MainSetup()
{
	PLAYER_MSG0(3,"gf: setup begin");
	
	if (!(position2d_dev = deviceTable->GetDevice(position2d_addr)))
	{
		PLAYER_ERROR("gf: unable to locate suitable position2d device");
		return -1;
	}
	if (position2d_dev->Subscribe(InQueue))
	{
		PLAYER_ERROR("gf: unable to subscribe to position2d device");
		position2d_dev = 0;
		return -1;
	}

	if (!(laser_dev = deviceTable->GetDevice(laser_addr)))
	{
		PLAYER_ERROR("gf: unable to locate suitable laser device");
		return -1;
	}
	if (laser_dev->Subscribe(InQueue))
	{
		PLAYER_ERROR("gf: unable to subscribe to laser device");
		laser_dev = 0;
		return -1;
	}
	PLAYER_MSG0(3,"gf: setup completed");
	return 0;
}

void GapFinderDriver::MainQuit()
{
	PLAYER_MSG0(3,"gf: shutting down");
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
	PLAYER_MSG0(3,"gf: shutted down");
}


void GapFinderDriver::Main()
{
	PLAYER_MSG0(3,"gf: thread started");
	
	assert(!active);
	
	for(;;)
	{
		this->TestCancel();
		this->ProcessMessages();
		usleep(10000);
	}
}


int GapFinderDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data)
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
			PLAYER_ERROR("gf: laser data format not supported.");
			return -1;
		}
		
		this->FindGap(d->ranges, d->max_range);
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

void GapFinderDriver::FindGap(const float *ranges, const float max_range)
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
	

	// find min range
	float min_range = max_range;
	for(uint16_t i = 0; i < 361; i++)
	{
		if (ranges[i] < max_range && fabs(laser_angle[i]) < DTOR(90 - DEAD_ZONE))
		{			
			if (ranges[i] < min_range)
				min_range = ranges[i];
		}
	}
	
	// calculate goal heading
	const double goal_dist = hypot(goal.px - position2d_data.pos.px, goal.py - position2d_data.pos.py);
	const double min_dist = goal_dist < min_range ? goal_dist : min_range;
	
	double goal_angle = atan2(goal.py - position2d_data.pos.py, goal.px - position2d_data.pos.px) - position2d_data.pos.pa;
	while(goal_angle >= M_PI) goal_angle -= M_PI + M_PI;
	while(goal_angle < -M_PI) goal_angle += M_PI + M_PI;

	// find gap
	double best_angle = -M_PI;
	bool gap_started = false;
	const float gap_range = (min_range + 0.5f >= max_range) ? max_range : min_range + 0.5f; // where to look for gap
	double gap_start_angle = 0.0, gap_stop_angle = 0.0;
	const double buffer_angle = fabs(asin((ROBOT_WIDTH + GAP_BUFFER) / 2.0 / gap_range) * 2.0);

	for(int i = 361 - DEAD_ZONE * 2; i >= DEAD_ZONE * 2; i--)
	{
		if(ranges[i] >= gap_range && !gap_started)
		{
			//start angle
			gap_start_angle = laser_angle[i];
			gap_started = true;
		}
		else if((ranges[i] < gap_range || i == DEAD_ZONE * 2) && gap_started)
		{
			//stop angle
			gap_stop_angle = laser_angle[i];
			gap_started = false;
			const double gap_dx = cos(gap_start_angle) * gap_range - cos(gap_stop_angle) * gap_range;
			const double gap_dy = sin(gap_start_angle) * gap_range - sin(gap_stop_angle) * gap_range;
			const double gap_open = hypot(gap_dx, gap_dy);

			// good gap?
			if (gap_open > ROBOT_WIDTH + GAP_BUFFER)
			{
				double angle = (gap_start_angle + gap_stop_angle) / 2.0;
				if (gap_start_angle - buffer_angle <= gap_stop_angle + buffer_angle)
				{
					// can only go in middle when buffer overlaps
				}
				else if (goal_angle <= gap_start_angle - buffer_angle && goal_angle >= gap_stop_angle + buffer_angle)
				{
					//the gap allows up to go straight towards target
					angle = goal_angle;
				}
				else if (goal_angle <= gap_stop_angle + buffer_angle)
				{
					angle = gap_stop_angle + buffer_angle;
				}
				else
				{
					angle = gap_start_angle - buffer_angle;
				}

				// use the best heading so far
				if (fabs(angle - goal_angle) < fabs(best_angle - goal_angle))
				{
					best_angle = angle;
				}
			}
		}
	}


	// find turn
	double turn = best_angle;

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
	
	if (turn > MAX_TURNRATE) turn = MAX_TURNRATE;
	if (turn < -MAX_TURNRATE) turn = -MAX_TURNRATE;
	
	const double speed = (speed_dist < speed_turn) ? speed_dist : speed_turn;
	
	// command motors
	player_position2d_cmd_vel cmd = {{speed, 0.0, turn}, true};
	position2d_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, &cmd, sizeof(player_position2d_cmd_vel), NULL);
	
	PLAYER_MSG2(9, "gf: speed = %f, turn = %f", speed, turn);
}

Driver* driver_init(ConfigFile* cf, int section) {
	return new GapFinderDriver(cf, section);
}

void driver_register(DriverTable* table) {
	table->AddDriver("amosgf", driver_init);
}

extern "C" {
	int player_driver_init(DriverTable* table) {
		driver_register(table);
		return 0;
	}
}

