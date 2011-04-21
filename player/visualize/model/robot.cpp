#include "robot.h"
#include <QApplication>
#include <QTimer>

using namespace amos;

Robot::Robot(const char *hostname,
	const int port,
	const int position2d_index,
	const int planner_index) :
	QThread(),
	client(new PlayerCc::PlayerClient(hostname, port)),
	position2d(new PlayerCc::Position2dProxy(client, position2d_index)),
	planner(planner_index >= 0 ? new PlayerCc::PlannerProxy(client, planner_index) : 0),
	x(0.0), y(0.0), yaw(0.0)
{
	connect(QApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(quit()));
	this->moveToThread(this);
	this->start();
}

Robot::~Robot()
{
	wait();
	if (position2d)
	{
		delete position2d;
		position2d = 0;
	}
	if (planner)
	{
		delete planner;
		planner = 0;
	}
	if (client)
	{
		delete client;
		client = 0;
	}
}

void Robot::run()
{
	QTimer timer(this);
	connect(&timer, SIGNAL(timeout()), this, SLOT(update()));
	timer.start(100);
	exec();
}

void Robot::update()
{
	static uint32_t i = 0;

	
	client->Read();
	x = position2d->GetXPos();
	y = position2d->GetYPos();
	yaw = position2d->GetYaw();

	if (!planner) return;
	if (!(i++ % 10))
	{
		planner->RequestWaypoints();
		std::vector<player_pose2d_t> path;
		path.resize(planner->GetWaypointCount());
		for (unsigned j = 0; j < planner->GetWaypointCount(); j++)
		{
			path[j] = planner->GetWaypoint(j);
		}
		mutex.lock();
		this->path = path;
		mutex.unlock();
	}
	
}
