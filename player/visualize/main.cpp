#include <iostream>
#include <string>
#include <QApplication>
#include "model/robot.h"
#include "model/world.h"
#include "ui/glwidget.h"

using namespace std;
using namespace amos;

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	vector< pair<string, uint16_t> > servers;
	string robot_host = "";
	uint16_t robot_port = 6665;
	int robot_position2d = 0;
	int robot_planner = -1;

	int flag = 0;
	while ((flag = getopt (argc, argv, "hr:m:l:p:")) != -1)
	{
		switch(flag)
		{
		case 'm':
			if (!strchr(optarg, ':'))
			{
				servers.push_back(make_pair(string(optarg), (uint16_t)11211));
			}
			else
			{
				servers.push_back(make_pair(
					string(optarg, strchr(optarg, ':')),
					(uint16_t)atoi(strchr(optarg, ':') + 1)
				));
			}
			break;
		case 'r':
			if (!strchr(optarg, ':'))
			{
				robot_host = optarg;
			}
			else
			{
				robot_host = string(optarg, strchr(optarg, ':'));
				robot_port = atoi(strchr(optarg, ':') + 1);
			}
			break;
		case 'p':
			robot_planner = atoi(optarg);
			break;
		case 'l':
			robot_position2d = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Usage: %s -m host[:port] -r host[:port] -l index -p index -h\n", argv[0]);
			return -1;
		}
	}

	if (servers.empty())
	{
		servers.push_back(make_pair(string("localhost"), (uint16_t)11211));
	}

	World world(servers);

	if (!robot_host.empty())
	{
		Robot robot(robot_host.c_str(), robot_port, robot_position2d, robot_planner);
		GLWidget gl(&world, &robot);
		gl.show();
		return a.exec();
	}
	else
	{
		GLWidget gl(&world);
		gl.show();
		return a.exec();
	}
}

