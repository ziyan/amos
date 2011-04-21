#include "astar.h"
#include <map>
#include <queue>
#include <limits>
#include <unordered_map>

using namespace amos;

// pose2d in map coordinate representation
typedef struct astar_pose2d
{
	int32_t x, y;
} astar_pose2d_t;

bool operator<(const astar_pose2d_t &a, const astar_pose2d_t &b)
{
	return (a.x != b.x) ? a.x < b.x : a.y < b.y;
}

bool operator==(const astar_pose2d_t &a, const astar_pose2d_t &b)
{
	return a.x == b.x && a.y == b.y;
}

uint64_t astar_pose2d_hash(const astar_pose2d_t &p)
{
	return (((uint64_t)p.x << 32) | (uint64_t)p.y);
}


// node on the map that stores cost and heuristic information
typedef struct astar_node
{
	astar_pose2d_t pose; // node location
	double weight;
} astar_node_t;

// this comparison is used by priority_queue to find which node has the lowest cost
bool operator<(const astar_node_t &a, const astar_node_t &b)
{
	return a.weight > b.weight;
}

bool astar_search(Map *map,
	const player_pose2d_t &begin,
	const player_pose2d_t &end,
	std::vector<player_pose2d_t> *path,
	double *cost,
	const double accuracy)
{
	//
	// Define a list of possible successors that we would like to consider
	//

	#define ASTAR_NODE_SUCCESSORS 8
	static const astar_node_t successors[ASTAR_NODE_SUCCESSORS] = {
		{{ 1,  0}, 1.0},
		{{-1,  0}, 1.0},
		{{ 0,  1}, 1.0},
		{{ 0, -1}, 1.0},
		{{ 1,  1}, sqrt(2.0)},
		{{ 1, -1}, sqrt(2.0)},
		{{-1,  1}, sqrt(2.0)},
		{{-1, -1}, sqrt(2.0)},
	};

	assert(map);
	assert(accuracy >= 0.0);

	std::unordered_map<uint64_t, bool> seen; // path that has been evaluated already
	std::unordered_map<uint64_t, double> costs; // real cost accumulated
	std::priority_queue<astar_node_t> queue; // queue of nodes that need to be explored

	std::unordered_map<uint64_t, astar_pose2d_t> parents; // node parent to back trace the optimal path

	const double scale = map->getInfo().scale;
	const double goal_accuracy = accuracy / scale;

	const astar_pose2d_t init = {(int32_t)floor(begin.px / scale), (int32_t)floor(begin.py / scale)}; // start position node
	const astar_pose2d_t goal = {(int32_t)floor(end.px / scale), (int32_t)floor(end.py / scale)}; // goal pose

	// insert first node which is the start pose
	costs[astar_pose2d_hash(init)] = 1.0;
	queue.push((astar_node_t){init, 1.0 + hypot(goal.x - init.x, goal.y - init.y)});

	astar_node_t head, child;
	double head_cost, child_cost;
	int i;
	map_data_t p;

	for(;;)
	{
		// no path found?
		if (queue.empty())
		{
			if (path) path->clear();
			if (cost) *cost = std::numeric_limits<double>::infinity();
			return false;
		}

		// copy the head of the queue
		head = queue.top();
		queue.pop();

		// found the goal yet?
		if (head.pose == goal) break;
		if (goal_accuracy > 0.0 && hypot((double)(goal.x - head.pose.x), (double)(goal.y - head.pose.y)) <= goal_accuracy) break;
		
		// mark it as already seen
		if (seen[astar_pose2d_hash(head.pose)]) continue;
		seen[astar_pose2d_hash(head.pose)] = true;

		head_cost = costs[astar_pose2d_hash(head.pose)];

		// find sucessors
		for (i = 0; i < ASTAR_NODE_SUCCESSORS; i++)
		{
			// compute successor
			child.pose.x = head.pose.x + successors[i].pose.x;
			child.pose.y = head.pose.y + successors[i].pose.y;

			if (seen[astar_pose2d_hash(child.pose)]) continue;

			// calculate cost and heuristic
			p = map->get(MAP_CHANNEL_P_CSPACE, child.pose.x, child.pose.y, 0);
			if (p >= MAP_P_MAX)
			{
				// don't consider obstacles at all
				seen[astar_pose2d_hash(child.pose)] = true;
				continue;
			}
			if (p < MAP_P_MIN) p = MAP_P_MIN;

			// accumulate cost
			if (p <= 0.5)
				child_cost = head_cost + successors[i].weight;
			else
				child_cost = head_cost + successors[i].weight / (double)(1.0 - p);

			// if the cell is already in the tentative list,
			// we need to make sure we don't have a higher cost here
			if (costs[astar_pose2d_hash(child.pose)] > 0.0 && costs[astar_pose2d_hash(child.pose)] < child_cost) continue;
			
			// store the parent only if we are interested in knowing the path
			if (path) parents[astar_pose2d_hash(child.pose)] = head.pose;
			costs[astar_pose2d_hash(child.pose)] = child_cost;

			// weight is cost + heuristic
			// put the sucessor into the queue
			child.weight = child_cost + hypot((double)(goal.x - child.pose.x), (double)(goal.y - child.pose.y));
			queue.push(child);
		}
	}

	// cost output
	if (cost) *cost = costs[astar_pose2d_hash(head.pose)];

	// path output
	if (path)
	{
		// reconstruct the path based on the tree
		std::vector<player_pose2d_t> rpath;
		astar_pose2d_t pose = head.pose;
		i = 0;
		
		if (goal_accuracy > 0.0)
			rpath.push_back((player_pose2d_t){ goal.x * scale + 0.5 * scale, goal.y * scale + 0.5 * scale, 0.0 });

		for(;;)
		{
			// convert coordinate back to real world coordinate
			rpath.push_back((player_pose2d_t){ pose.x * scale + 0.5 * scale, pose.y * scale + 0.5 * scale, 0.0 });
			if (pose == init) break;
			pose = parents[astar_pose2d_hash(pose)];
		}

		*path = std::vector<player_pose2d_t>(rpath.rbegin(), rpath.rend());
	}
	return true;
}


