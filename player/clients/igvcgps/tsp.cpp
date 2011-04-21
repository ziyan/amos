#include "tsp.h"
#include <assert.h>
#include <algorithm>
#include <limits>
#include <map>
#include "astar/astar.h"

using namespace amos;

bool operator<(const player_pose2d_t &a, const player_pose2d_t &b)
{
	return (a.px != b.px) ? a.px < b.px : a.py < b.py;
}

TSPThread::TSPThread(Map *map, const double accuracy)
	: Thread(), map(map), accuracy(accuracy)
{
	assert(map);
}

TSPThread::~TSPThread()
{
}

void TSPThread::set(const std::vector<player_pose2d_t> &waypoints)
{
	mutex.lock();
	this->waypoints = waypoints;	
	mutex.unlock();
}

void TSPThread::get(std::vector<player_pose2d_t> *path, double *cost)
{
	mutex.lock();
	if (path) *path = this->path;
	if (cost) *cost = this->cost;
	mutex.unlock();
}

void TSPThread::run()
{
	std::map<std::pair<player_pose2d_t, player_pose2d_t>, double> costs;
	std::vector<player_pose2d_t> path, candidate_path;
	double cost, candidate_cost;
	std::vector<player_pose2d_t> waypoints;


	for(;;)
	{
		this->testcancel();

		// make a copy of the inputs so that they don't get modified when we are calculating
		mutex.lock();
		waypoints = this->waypoints;
		mutex.unlock();
		
		if (waypoints.size() >= 2)
		{
			// initialize
			map->refresh();
			costs.clear();
			path.clear();
			cost = std::numeric_limits<double>::infinity();
			candidate_path = std::vector<player_pose2d_t>(waypoints);
			sort(candidate_path.begin() + 1, candidate_path.end() - 1);

			// evaluate each permutation
			do
			{
				candidate_cost = 0.0;
				for(std::vector<player_pose2d_t>::const_iterator i = candidate_path.begin(); i != candidate_path.end() - 1; i++)
				{	
					std::pair<player_pose2d_t, player_pose2d_t> key = (*i < *(i + 1)) ? std::make_pair(*i, *(i + 1)) : std::make_pair(*(i + 1), *i);

					// if no cost is calculated for this path yet, use astar to calculate it
					if (!costs[key])
					{
						printf("igvcgps: cost from (%f, %f) to (%f, %f) is calculated to be ... ",  i->px, i->py, (i+1)->px, (i+1)->py);
						fflush(stdout);
						if (!astar_search(map, *i, *(i+1), NULL, &costs[key], accuracy))
						{
							// no path found, try another permutation
							printf("IMPOSSIBLE!\n");
							candidate_cost = std::numeric_limits<double>::infinity();
							break;
						}
						printf("%f\n", costs[key]);
					}
					candidate_cost += costs[key];

					// if candidate cost already larger than best, then we can quit calculating the rest
					if (candidate_cost >= cost) break;
				}

				// save best
				if (candidate_cost < cost)
				{
					path = candidate_path;
					cost = candidate_cost;
				}
			}
			while(next_permutation(candidate_path.begin() + 1, candidate_path.end() - 1));
			
			if (path.size() != 0)
			{
				printf("igvcgps: optimal path found with cost = %f\n",  cost);
				for (std::vector<player_pose2d_t>::const_iterator i = path.begin(); i != path.end(); i++)
				{
					printf("\t(%f, %f)\n", i->px, i->py);
				}
			}
			else
			{
				printf("igvcgps: optimal path not found\n");
			}

			mutex.lock();
			this->path = path;
			this->cost = cost;
			mutex.unlock();
		}

		usleep(100000);
	}
}



