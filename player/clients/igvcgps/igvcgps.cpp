#include <libplayerc++/playerc++.h>
#include <math.h>
#include <signal.h>
#include <fstream>
#include <algorithm>
#include "tsp.h"

using namespace std;
using namespace amos;
using namespace PlayerCc;

#define WAYPOINT_DISTANCE 0.9	// accuracy that we need to reach
#define ASTAR_ACCURACY 1.0

// global killed flag
bool killed = false;

// kill signal handler
void kill(int i = 0)
{
	killed = true;
}

// program main entry
int main(int argc, char** argv)
{
	// register kill signals
	signal(SIGINT, kill);
	signal(SIGTERM, kill);
	signal(SIGABRT, kill);
	
	// check arguments
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <waypoints> [host] [port] [host:port]\n", argv[0]);
		return -1;
	}

	vector< pair<string, uint16_t> > servers;
	for (int i = 4; i < argc; i++)
	{
		if (!strchr(argv[i], ':'))
		{
			fprintf(stderr, "Usage: %s <waypoints> [host] [port] [host:port]\n", argv[0]);
			return -1;
		}
		
		servers.push_back(make_pair(
			string(argv[i], strchr(argv[i], ':')),
			(uint16_t)atoi(strchr(argv[i], ':') + 1)
		));
	}
	
	if (servers.empty())
	{
		servers.push_back(make_pair(
			string("localhost"),
			(uint16_t)11211
		));
	}
	
	// read goals from file
	vector<player_pose2d_t> goals;
	fstream file(argv[1]);
	if (!file)
	{
		fprintf(stderr, "ERROR: failed to open waypoints file for reading\n");
		return -2;
	}
	while (file)
	{
		double x, y;
		file >> x >> y;
		if (file)
		{
			goals.push_back((player_pose2d_t){x, y, 0.0});
			printf("igvcgps: waypoint: easting = %f, northing = %f\n", x, y);
		}
	}
	if (!goals.size())
	{
		fprintf(stderr, "ERROR: no waypoint found\n");
		return -2;
	}

	// connect to robot
	PlayerClient robot(argc > 2 ? argv[2] : "localhost", argc > 3 ? atoi(argv[3]) : 6665);
	PlannerProxy planner(&robot, 0);

	// connect to map
	Map map(servers);
	if (!map.isOpen())
	{
		fprintf(stderr, "ERROR: failed to open map\n");
		return -2;
	}

	// start tsp thread
	TSPThread tsp(&map, ASTAR_ACCURACY);
	tsp.start();

	// initialize and read end point
	robot.Read();
	goals.push_back(planner.GetPose());
	planner.SetEnable(false);

	while(!killed)
	{
		// robot communication
		robot.Read();
		
		// current position
		player_pose2d_t pose = planner.GetPose();
		
		//
		// step 1: see if we reached any goals
		//

		if (goals.size() > 1)
		{
			for (vector<player_pose2d_t>::iterator i = goals.begin(); i != goals.end() - 1; i++)
			{
				// assume we will reach only one waypoint at a time
				// TODO: is this assumption correct?
				if (hypot(i->py - pose.py, i->px - pose.px) < WAYPOINT_DISTANCE)
				{
					goals.erase(i);
					printf("igvcgps: we reached 1 waypoint, %i left\n", (int)goals.size());
					break;
				}
			}
		}
		else if (goals.size() == 1)
		{
			if (hypot(goals[0].py - pose.py, goals[0].px - pose.px) < WAYPOINT_DISTANCE)
			{
				goals.clear();
				printf("igvcgps: we reached last waypoint\n");
				break;
			}
		}
		else
		{
			// this should never happen
			assert(false);
			break;
		}

		//
		// step 2: find path / next waypoint
		//

		player_pose2d_t next;
		// if we have more than one goal to go
		if (goals.size() > 1)
		{
			// provide current position as a start
			vector<player_pose2d_t> waypoints(goals);
			waypoints.insert(waypoints.begin(), pose);
			
			// talk to the tsp thread
			tsp.set(waypoints);
			
			// get path
			vector<player_pose2d_t> path;
			tsp.get(&path, 0);

			if (!path.size())
			{
				// tsp is not done yet
				planner.SetEnable(false);
				continue;
			}
			assert(path.size() >= 2);
			
			// get rid of start location since we don't care
			path.erase(path.begin());

			// now filter out all waypoints that has already been visited
			for (vector<player_pose2d_t>::iterator i = path.begin(); i != path.end() - 1; i++)
			{
				// waypoint has been visited
				if (find(goals.begin(), goals.end() - 1, *i) == goals.end() - 1)
					path.erase(i);
			}
			
			assert(path.size() >= 1);
			next = path[0];
		}
		// we have only one goal left (return to start position)
		else
		{
			// tell tsp to stop
			tsp.set(vector<player_pose2d_t>());
			next = goals[0];
		}
		
		//
		// Step 3: command planner
		//
		planner.SetGoalPose(next.px, next.py, 0.0);
		planner.SetEnable(true);
		usleep(10000);
	}

	planner.SetEnable(false);
	tsp.stop();
}

bool operator==(const player_pose2d_t &a, const player_pose2d_t &b)
{
	return a.px == b.px && a.py == b.py;
}


