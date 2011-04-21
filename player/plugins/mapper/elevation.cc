#include "mapper.h"

#include <cmath>

#define ELEVATION_OBSTACLE_HEIGHT 0.1f
#define ELEVATION_RUNNING_WEIGHT 0.3f

using namespace amos;

void MapperDriver::MapElevation(const float *ranges, const float max)
{
	// static constants section
	static double laser_angle[361], laser_angle_cos[361], laser_angle_sin[361];
	static double laser_tilt[361], laser_tilt_cos[361], laser_tilt_sin[361];
	static map_data_t laser_last_var[361] = {0};

	static bool init = false;
	if (!init)
	{
		// build a table of laser tilt angle
		for(int i = 0; i < 361; i++)
		{
			laser_angle[i] = DTOR((double)i * 0.5 - 90.0);
			laser_angle_cos[i] = cos(laser_angle[i]);
			laser_angle_sin[i] = sin(laser_angle[i]);
			
			// negative tilt due to negative pitch
			laser_tilt[i] = atan(tan(elevation_laser_pose.ppitch) * cos(laser_angle[i]));
			laser_tilt_cos[i] = cos(laser_tilt[i]);
			laser_tilt_sin[i] = sin(laser_tilt[i]);
		}
		init = true;
	}
	
	const double scale = map->getInfo().scale;
	const player_pose2d_t pose2d = this->position2d_data.pos;
	
	double lx, ly, lz;
	double l, a;
	double gx, gy;
	int32_t x, y;
	int i, j;
	map_data_t avg, var;

	// convert to 3d space
	for(j = 0; j < 361; j++)
	{
		// start from center and work the way around
		i = 180 + ((j % 2) ? -j/2 : j/2);
		
		if (ranges[i] >= max)
		{
			virtual_laser_ranges[i] = max;
			elevation_laser_previous[i].px = 0.0f;
			elevation_laser_previous[i].py = 0.0f;
			elevation_laser_previous[i].pz = 0.0f;
			continue;
		}
	
		// robot local coordinates
		lx = ranges[i] * laser_tilt_cos[i] * laser_angle_cos[i];
		ly = ranges[i] * laser_tilt_cos[i] * laser_angle_sin[i];
		lz = ranges[i] * laser_tilt_sin[i] + elevation_laser_pose.pz;
		
		// virtual laser
		virtual_laser_ranges[i] = (fabs(lz) < ELEVATION_OBSTACLE_HEIGHT) ? max : ranges[i] * laser_tilt_cos[i];

		// global coordinates
		l = sqrt(lx*lx + ly*ly);
		a = atan2(ly, lx);
		gx = pose2d.px + l * cos(a + pose2d.pa);
		gy = pose2d.py + l * sin(a + pose2d.pa);
		
		// record latest scan
		elevation_laser_previous[i].px = gx;
		elevation_laser_previous[i].py = gy;
		elevation_laser_previous[i].pz = lz;
		
		// map to pixel
		x = gx / scale;
		y = gy / scale;
		

		// update current cell
		avg = map->get(MAP_CHANNEL_E_AVG, x, y, 0);
		var = map->get(MAP_CHANNEL_E_VAR, x, y, 0);
	
		if (avg == 0.0f && var == 0.0f)
		{
			map->set(MAP_CHANNEL_E_AVG, x, y, 0, lz);
			map->set(MAP_CHANNEL_E_VAR, x, y, 0, laser_last_var[i]);
		}
		else
		{
			avg = ELEVATION_RUNNING_WEIGHT * lz + (1.0f - ELEVATION_RUNNING_WEIGHT) * avg;
			var = ELEVATION_RUNNING_WEIGHT * (lz - avg) * (lz - avg) + (1.0f - ELEVATION_RUNNING_WEIGHT) * var;
			map->update(MAP_CHANNEL_E_AVG, x, y, 0, 1.0f - ELEVATION_RUNNING_WEIGHT, ELEVATION_RUNNING_WEIGHT * lz);
			map->update(MAP_CHANNEL_E_VAR, x, y, 0, 1.0f - ELEVATION_RUNNING_WEIGHT, ELEVATION_RUNNING_WEIGHT * (lz - avg) * (lz - avg));
			laser_last_var[i] = var;
		}
	}
}

