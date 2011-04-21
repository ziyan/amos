#include "mapper.h"

#include <cmath>

#define VISUAL_RUNNING_WEIGHT 0.3f

using namespace amos;

void MapperDriver::MapVisual(const uint8_t *image, const uint32_t &width, const uint32_t &height)
{
	// copy current location
	const player_pose2d_t pose2d = this->position2d_data.pos;
	const double scale = map->getInfo().scale;
	
	for(int i = 0; i < 361; i++)
	{
		if (elevation_laser_previous[i].px == 0.0f &&
			elevation_laser_previous[i].py == 0.0f &&
			elevation_laser_previous[i].pz == 0.0f) continue;
		
		const int32_t x = elevation_laser_previous[i].px / scale;
		const int32_t y = elevation_laser_previous[i].py / scale;

		// find center of the cell
		const double cx = scale * x + scale * 0.5;
		const double cy = scale * y + scale * 0.5;
		const double e = elevation_laser_previous[i].pz;

		const double t = sqrt((cx - pose2d.px) * (cx - pose2d.px) + (cy - pose2d.py) * (cy - pose2d.py));
		const double a = atan2(cy - pose2d.py, cx - pose2d.px) - pose2d.pa;
		const double d = t * cos(a);

		// is cell within horizontal field of view?
		const double ha = asin(t * sin(a) / sqrt(t * t + visual_camera_pose.pz * visual_camera_pose.pz));
		if (fabs(ha) > visual_camera_hfov * 0.5f) continue;
		
		// is cell within vertical field of view?
		const double va = atan2(e - visual_camera_pose.pz, d) - visual_camera_pose.ppitch;
		if(fabs(va) > visual_camera_vfov * 0.5f) continue;

		// determine horizontal frame coordinates
		const int fxc = width * (-ha / visual_camera_hfov + 0.5);
		if (fxc < 0 || fxc >= (int)width) continue;
		
		// determine vertical frame coordinates
		const int fyc = height * (-va / visual_camera_vfov + 0.5);
		if (fyc < 0 || fyc >= (int)height) continue;

#if 0
		const float r = (float)image[fyc * width * 3 + fxc * 3 + 0] / 255.0f;
		const float g = (float)image[fyc * width * 3 + fxc * 3 + 1] / 255.0f;
		const float b = (float)image[fyc * width * 3 + fxc * 3 + 2] / 255.0f;

		// TODO: configurable weight
		map->update(MAP_CHANNEL_R, x, y, 0, 1.0f - VISUAL_RUNNING_WEIGHT, VISUAL_RUNNING_WEIGHT * r);
		map->update(MAP_CHANNEL_G, x, y, 0, 1.0f - VISUAL_RUNNING_WEIGHT, VISUAL_RUNNING_WEIGHT * g);
		map->update(MAP_CHANNEL_B, x, y, 0, 1.0f - VISUAL_RUNNING_WEIGHT, VISUAL_RUNNING_WEIGHT * b);
#else
		// need to take neighbouring pixels into account, cell is larger than a pixel on a frame
		const double l = sqrt(d * d + visual_camera_pose.pz * visual_camera_pose.pz);
		const double ha1 = atan(tan(ha) - scale * 0.5 / l);
		const double ha2 = atan(tan(ha) + scale * 0.5 / l);
		const double va1 = atan2(e - visual_camera_pose.pz, d - scale * 0.5) - visual_camera_pose.ppitch;
		const double va2 = atan2(e - visual_camera_pose.pz, d + scale * 0.5) - visual_camera_pose.ppitch;

		const int fx1 = width * (-ha1 / visual_camera_hfov + 0.5);
		const int fx2 = width * (-ha2 / visual_camera_hfov + 0.5);
		const int fy1 = height * (-va1 / visual_camera_vfov + 0.5);
		const int fy2 = height * (-va2 / visual_camera_vfov + 0.5);
		const int fxmin = fx1 < fx2 ? (fx1 < 0 ? 0 : fx1) : (fx2 < 0 ? 0 : fx2);
		const int fxmax = fx1 > fx2 ? (fx1 >= (int)width ? width - 1 : fx1) : (fx2 >= (int)width ? width - 1 : fx2);
		const int fymin = fy1 < fy2 ? (fy1 < 0 ? 0 : fy1) : (fy2 < 0 ? 0 : fy2);
		const int fymax = fy1 > fy2 ? (fy1 >= (int)height ? height - 1 : fy1) : (fy2 >= (int)height ? height - 1 : fy2);

		for (int fx = fxmin; fx <= fxmax; ++fx)
		{
			for (int fy = fymin; fy <= fymax; ++fy)
			{
				const float r = (float)image[fy * width * 3 + fx * 3 + 0] / 255.0f;
				const float g = (float)image[fy * width * 3 + fx * 3 + 1] / 255.0f;
				const float b = (float)image[fy * width * 3 + fx * 3 + 2] / 255.0f;

				// TODO: configurable weight
				map->update(MAP_CHANNEL_R, x, y, 0, 1.0f - VISUAL_RUNNING_WEIGHT, VISUAL_RUNNING_WEIGHT * r);
				map->update(MAP_CHANNEL_G, x, y, 0, 1.0f - VISUAL_RUNNING_WEIGHT, VISUAL_RUNNING_WEIGHT * g);
				map->update(MAP_CHANNEL_B, x, y, 0, 1.0f - VISUAL_RUNNING_WEIGHT, VISUAL_RUNNING_WEIGHT * b);
			}
		}
#endif
	}
}

