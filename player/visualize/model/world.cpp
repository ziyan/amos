#include "world.h"
#include <QApplication>
#include <QTimer>
#include <QtDebug>

using namespace amos;

World::World(const std::vector< std::pair<std::string, uint16_t> > &servers) :
	QThread(),
	map(new Map(servers))
{
	Q_ASSERT(map->isOpen());
	if (!map->isOpen())
	{
		delete map;
		map = 0;
	}

	connect(QApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(quit()));
	this->moveToThread(this);
	this->start();
}

World::~World()
{
	wait();
	if (map)
	{
		delete map;
		map = 0;
	}
}

void World::run()
{
	QTimer timer(this);
	connect(&timer, SIGNAL(timeout()), this, SLOT(update()));
	timer.start(100);

	exec();
}

void World::update()
{

}
