#include "astar.h"
#include <assert.h>
#include "astar/astar.h"

using namespace amos;

bool operator==(const player_pose2d_t &a, const player_pose2d_t &b)
{
	return a.px == b.px && a.py == b.py;
}

bool operator!=(const player_pose2d_t &a, const player_pose2d_t &b)
{
	return a.px != b.px || a.py != b.py;
}

AStarThread::AStarThread(Map *map, const double accuracy)
	: Thread(), map(map), accuracy(accuracy)
{
	assert(map);
}

AStarThread::~AStarThread()
{
}

void AStarThread::set(const player_pose2d_t &begin, const player_pose2d_t &end)
{
	mutex.lock();
	if (this->end != end || begin == end)
	{
		this->path.clear();
	}
	this->begin = begin;
	this->end = end;
	mutex.unlock();
}

void AStarThread::get(std::vector<player_pose2d_t> *path)
{
	assert(path);
	mutex.lock();
	*path = this->path;
	mutex.unlock();
}

void AStarThread::run()
{
	std::vector<player_pose2d_t> path;
	player_pose2d_t begin, end;

	for(;;)
	{
		this->testcancel();
		
		map->refresh();
		
		mutex.lock();
		begin = this->begin;
		end = this->end;
		mutex.unlock();

		if (begin != end)
		{
			if (!astar_search(map, begin, end, &path, NULL, this->accuracy))
			{
				PLAYER_WARN4("planner: no path found between (%f, %f) and (%f, %f)", begin.px, begin.py, end.px, end.py);
			}

			mutex.lock();
			// Is this the goal you asked for? Or are we too late already?
			if (this->end == end && this->begin != this->end)
			{
				this->path = path;
			}
			else
			{
				this->path.clear();
			}
			mutex.unlock();
		}

		usleep(50000);
	}
}

