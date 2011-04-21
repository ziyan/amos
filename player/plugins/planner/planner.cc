#include "planner.h"
#include <limits>	

#define PLANNER_PATH_DEVIATION_LIMIT 3.0
#define PLANNER_NEXT_WAYPOINT_DISTANCE 1.5
#define PLANNER_GOAL_DISTANCE 0.75

using namespace amos;

PlannerDriver::PlannerDriver(ConfigFile* cf, int section) :
	ThreadedDriver(cf, section),
	map(0),
	astar(0),
	enabled(false),
	input_position2d_dev(0),
	output_position2d_dev(0)
{
	// read settings
	int map_servers_count = cf->GetTupleCount(section, "maphosts");
	if (map_servers_count > 0)
	{
		assert(map_servers_count == cf->GetTupleCount(section, "mapports"));
		for (int i = 0; i < map_servers_count; i++)
		{
			map_servers.push_back(make_pair(
				std::string(cf->ReadTupleString(section, "maphosts", i, "localhost")),
				(uint16_t)cf->ReadTupleInt(section, "mapports", i, 11211)
			));
		}
	}
	else
	{
		// default map server
		map_servers.push_back(make_pair(std::string("localhost"), (uint16_t)11211));
	}


    // set up the devices we provide
	if (cf->ReadDeviceAddr(&planner_addr, section, "provides", PLAYER_PLANNER_CODE, -1, NULL))
	{
		PLAYER_ERROR("planner: cannot find planner addr");
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(planner_addr))
	{
		PLAYER_ERROR("planner: cannot add planner interface");
		this->SetError(-1);
		return;
	}

	// set up the devices we require
	if (cf->ReadDeviceAddr(&input_position2d_addr, section, "requires", PLAYER_POSITION2D_CODE, -1, "input"))
	{
		PLAYER_ERROR("planner: cannot find input position2d addr");
		this->SetError(-1);
		return;
	}
	
	if (cf->ReadDeviceAddr(&output_position2d_addr, section, "requires", PLAYER_POSITION2D_CODE, -1, "output"))
	{
		PLAYER_WARN("planner: not using output position2d device");
	}

	PLAYER_MSG0(3, "planner: initialized");
}

PlannerDriver::~PlannerDriver()
{
}

int PlannerDriver::MainSetup()
{
	PLAYER_MSG0(3, "planner: setup started");

	// first create the map client
	map = new Map(map_servers);
	if (!map->isOpen())
	{
		delete map;
		map = 0;
		PLAYER_ERROR("planner: unable to create map");
		return -1;
	}

	// start up the AStar thread
	astar = new AStarThread(map, PLANNER_NEXT_WAYPOINT_DISTANCE);
	astar->start();

	// subscribe to input position2d
	if (!(input_position2d_dev = deviceTable->GetDevice(input_position2d_addr)))
	{
		PLAYER_ERROR("planner: unable to locate suitable input position2d device");
		return -1;
	}
	else if (input_position2d_dev->Subscribe(this->InQueue))
	{
		PLAYER_ERROR("planner: input position2d device cannot be subscribed");
		input_position2d_dev = 0;
		return -1;
	}
	
	if (output_position2d_addr.interf)
	{
		if (!(output_position2d_dev = deviceTable->GetDevice(output_position2d_addr)))
		{
			PLAYER_ERROR("planner: unable to locate suitable output position2d device");
			return -1;
		}
		else if (output_position2d_dev->Subscribe(this->InQueue))
		{
			PLAYER_ERROR("planner: output position2d device cannot be subscribed");
			output_position2d_dev = 0;
			return -1;
		}
	}

	PLAYER_MSG0(3, "planner: setup complete");
	return 0;
}

void PlannerDriver::MainQuit()
{
	PLAYER_MSG0(3, "planner: shutting down");

	if (astar)
	{
		astar->stop();
		delete astar;
		astar = 0;
	}

	if (map)
	{
		delete map;
		map = 0;
	}

	if (input_position2d_dev)
	{
		input_position2d_dev->Unsubscribe(this->InQueue);
		input_position2d_dev = 0;
	}
	
	if (output_position2d_dev)
	{
		output_position2d_dev->Unsubscribe(this->InQueue);
		output_position2d_dev = 0;
	}
	
	enabled = false;

	PLAYER_MSG0(3, "planner: shut down");
}

void PlannerDriver::Main()
{
	PLAYER_MSG0(3, "planner: thread started");

	assert(!enabled);

	for(;;)
	{
		this->TestCancel();
		this->ProcessMessages();
		this->Plan();

		// command local planner
		if (output_position2d_dev)
		{
			player_position2d_cmd_pos_t cmd;
			memset(&cmd, 0, sizeof(player_position2d_cmd_pos_t));
			if (planner.valid && !planner.done)
			{
				// tell local planner to go towards next waypoint
				cmd.pos = planner.waypoint;
				cmd.state = true;
			}
			else
			{
				// tell local planner to stop
				cmd.pos = planner.pos;
				cmd.state = false;
			}
			output_position2d_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_POS, &cmd, sizeof(player_position2d_cmd_pos_t), NULL);
		}

		this->Publish(planner_addr, PLAYER_MSGTYPE_DATA, PLAYER_PLANNER_DATA_STATE, &planner);
		usleep(10000);
	}

}

void PlannerDriver::Plan()
{
	// reset all data
	planner.waypoint_idx = -1;
	planner.valid = false;
	planner.done = false;
	planner.waypoint = planner.pos;

	// are we enabled?
	if (!enabled)
	{
		// stop astar
		astar->set(planner.pos, planner.pos);
		
		// no path
		path.clear();
		planner.waypoints_count = 0;
		return;
	}

	const double goal_distance = hypot(planner.goal.px - planner.pos.px, planner.goal.py - planner.pos.py);

	// are we done?
	if ((planner.done = goal_distance < PLANNER_GOAL_DISTANCE))
	{
		// stop astar
		astar->set(planner.pos, planner.pos);

		// no path
		path.clear();
		planner.waypoints_count = 0;
		return;
	}
	
	// are we really close to our goal ready?
	/*if (goal_distance < PLANNER_NEXT_WAYPOINT_DISTANCE)
	{
		// stop astar
		astar->set(planner.pos, planner.pos);
		
		// now the only thing in the path is our goal
		path.clear();
		path.push_back(planner.goal);
		planner.waypoints_count = 1;
		planner.waypoint_idx = 0;
		planner.waypoint = planner.goal;
		planner.valid = true;
		return;
	}*/

	// talk to the astar thread
	astar->set(planner.pos, planner.goal);
	astar->get(&path);
	planner.waypoints_count = path.size();
	
	// do we have a path?
	if (!path.size())
	{
		// no path found or not ready yet
		return;
	}
	
	// find closest waypoint to our location
	double min = std::numeric_limits<double>::infinity();
	for (uint32_t i = 0; i < planner.waypoints_count; i++)
	{
		const double d = hypot(path[i].px - planner.pos.px, path[i].py - planner.pos.py);
		if (d <= min)
		{
			min = d;
			planner.waypoint_idx = i;
		}
	}

	// are we deviated from path?
	if (min > PLANNER_PATH_DEVIATION_LIMIT)
	{
		// too much deviation
		PLAYER_WARN("planner: path deviation detected");
		return;
	}
	
	// then find a waypoint that is far away enough
	//uint32_t i;
	/*for (i = planner.waypoint_idx; i < planner.waypoints_count; i++)
	{
		// current waypoint is set to some waypoint ahead of the current position
		if (hypot(path[i].px - planner.pos.px, path[i].py - planner.pos.py) > PLANNER_NEXT_WAYPOINT_DISTANCE)
		{
			break;
		}
	}*/
	const uint32_t next_waypoint_dist = (uint32_t)ceil(PLANNER_NEXT_WAYPOINT_DISTANCE / map->getInfo().scale);
	planner.waypoint_idx += next_waypoint_dist;
	if (planner.waypoint_idx >= (int)planner.waypoints_count)
		planner.waypoint_idx = planner.waypoints_count - 1;
	planner.waypoint = path[planner.waypoint_idx];
	planner.valid = true;

	PLAYER_MSG2(9, "planner: next waypoint is (%f, %f)", planner.waypoint.px, planner.waypoint.py);
}

int PlannerDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data)
{
	// received input_position2d update
	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, input_position2d_addr))
	{
		if (!data || hdr->size != sizeof(player_position2d_data_t)) return -1;
		planner.pos = ((player_position2d_data_t*)data)->pos;
		return 0;
	}
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, output_position2d_addr))
	{
		// ignored
		return 0;
	}
	// enable/disable planner
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_PLANNER_REQ_ENABLE, planner_addr))
	{
		if (!data || hdr->size != sizeof(player_planner_enable_req_t)) return -1;
		enabled = ((player_planner_enable_req_t*)data)->state;
		this->Publish(planner_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_PLANNER_REQ_ENABLE, NULL);
		return 0;
	}
	// set goal
	else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_PLANNER_CMD_GOAL, planner_addr))
	{
		if (!data || hdr->size != sizeof(player_planner_cmd_t)) return -1;
		planner.goal = ((player_planner_cmd_t*)data)->goal;
		return 0;
    }
    // request for waypoints
	else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_PLANNER_REQ_GET_WAYPOINTS, planner_addr))
	{
		if (hdr->size != 0) return -1;
		player_planner_waypoints_req_t resp;
		resp.waypoints_count = path.size();
		resp.waypoints = (resp.waypoints_count > 0) ? &path[0] : 0;
		this->Publish(planner_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_PLANNER_REQ_GET_WAYPOINTS, &resp);
		return 0;
	}
	return -1;
}

Driver* driver_init(ConfigFile* cf, int section)
{
	return new PlannerDriver(cf, section);
}

void driver_register(DriverTable* table)
{
	table->AddDriver("amosplanner", driver_init);
}

extern "C"
{
	int player_driver_init(DriverTable* table)
	{
		driver_register(table);
		return 0;
	}
}

