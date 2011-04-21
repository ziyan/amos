#include "mapper.h"
#include <limits>

#define PROBABILITY_CHANGE_RATE 0.5f
#define PROBABILITY_RUNNING_WEIGHT 0.5f

using namespace amos;

double normalize_angle(double a)
{
	while(a < -M_PI) a += M_PI + M_PI;
	while(a >= M_PI) a -= M_PI + M_PI;
	return a;
}

double laser_probability_model(double range, double d)
{
	#define D1 0.2
	if ( d < range - D1 )
	{
		return 0.0;
	}
	else if ( d < range + D1 )
	{
		return 1.0;
	}
	return 0.5;
}

void MapperDriver::MapProbability(const float *ranges, const float max)
{
	// static constants section
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

	const double scale = map->getInfo().scale;
	const player_pose2d_t pose2d = this->position2d_data.pos;
	
	// first, find a bounding box
	const int32_t x_min = (int32_t)floor((pose2d.px - max) / scale);
	const int32_t x_max = (int32_t)ceil((pose2d.px + max) / scale);
	const int32_t y_min = (int32_t)floor((pose2d.py - max) / scale);
	const int32_t y_max = (int32_t)ceil((pose2d.py + max) / scale);
	
	// check each cell inside that box
	for (int32_t y = y_min; y <= y_max; y++)
	{
		for (int32_t x = x_min; x <= x_max; x++)
		{
			// find center of such cell
			const double cx = scale * x + scale * 0.5;
			const double cy = scale * y + scale * 0.5;
			
			// distance from center
			const double d = sqrt((cx - pose2d.px) * (cx - pose2d.px) + (cy - pose2d.py) * (cy - pose2d.py));
			if (d > max) continue;
			
			// angle
			const double a = normalize_angle(atan2(cy - pose2d.py, cx - pose2d.px) - pose2d.pa);
			const int i = 180 + (int)floor(a / DTOR(0.5) + 0.5);
			if (i < 0 || i >= 361) continue;
			
			// need to take all laser beams landed in this cell into account
			const double x0 = scale * x;
			const double y0 = scale * y;
			const double x1 = scale * x + scale;
			const double y1 = scale * y + scale;
			const double a0 = normalize_angle(atan2(y0 - pose2d.py, x0 - pose2d.px) - pose2d.pa);
			const double a1 = normalize_angle(atan2(y0 - pose2d.py, x1 - pose2d.px) - pose2d.pa);
			const double a2 = normalize_angle(atan2(y1 - pose2d.py, x0 - pose2d.px) - pose2d.pa);
			const double a3 = normalize_angle(atan2(y1 - pose2d.py, x1 - pose2d.px) - pose2d.pa);
			
			double a_max = a0;
			if (a1 > a_max) a_max = a1;
			if (a2 > a_max) a_max = a2;
			if (a3 > a_max) a_max = a3;
			
			double a_min = a0;
			if (a1 < a_min) a_min = a1;
			if (a2 < a_min) a_min = a2;
			if (a3 < a_min) a_min = a3;
			
			int i_max = 180 + (int)ceil(a_max / DTOR(0.5));
			if (i_max > 360) i_max = 360;
			if (i_max < 0) i_max = 0; 
			int i_min = 180 + (int)floor(a_max / DTOR(0.5));
			if (i_min > 360) i_min = 360;
			if (i_min < 0) i_min = 0;
			
			// laser model
			double r = ranges[i];
			for (int j = i_min; j <= i_max; j++)
			{
				if (ranges[j] < r) r = ranges[j];
			}
			if (ranges[i] >= max) r = std::numeric_limits<double>::infinity();
			
			double dp = laser_probability_model(r, d) - 0.5;
			map_data_t prob = map->get(MAP_CHANNEL_P, x, y, 0);
			prob += PROBABILITY_CHANGE_RATE * dp;
			if (prob > 1.0f) prob = 1.0f;
			if (prob < 0.0f) prob = 0.0f;
			map->update(MAP_CHANNEL_P, x, y, 0, 1.0f - PROBABILITY_RUNNING_WEIGHT, PROBABILITY_RUNNING_WEIGHT * prob);
		}
	}
}

