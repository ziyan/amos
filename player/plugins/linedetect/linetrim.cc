#include "linetrim.h"

#include <math.h>

using namespace std;

vector<CvPoint*> trimLines(CvPoint** lines, int num_lines, double width, double height, player_laser_data_t laser_data) {
	
	vector<CvPoint*> trimmed_lines;
	player_pose3d_t laser_pose, camera_pose;
	double camera_hfov, camera_vfov;
	
	// read settings
	laser_pose.px = 0.0f;
	laser_pose.py = 0.0f;
	laser_pose.pz = 0.47f;
	laser_pose.proll = 0.0f;
	laser_pose.ppitch = 0.0f;
	laser_pose.pyaw = 0.0f;
	
	camera_pose.px = 0.0f;
	camera_pose.py = 0.0f;
	camera_pose.pz = 1.0f;
	camera_pose.proll = 0.0f;
	camera_pose.ppitch = -DTOR(30.0f);
	camera_pose.pyaw = 0.0f;
	
	camera_hfov = DTOR(70.0);
	camera_vfov = DTOR(60.0);

	// add all lines
	for( int i = 0; i < num_lines; i++ ) {
		trimmed_lines.push_back(lines[i]);
	}
	
	
	for( int i = 0; i < laser_data.ranges_count; i++ ) {
		
		// see if lidar ray is an obstacle
		if( laser_data.ranges[i] >= OBSTACLE_DIST ) continue;
		
		const double t = laser_data.ranges[i];
		const double a = laser_data.min_angle + i*laser_data.resolution;

		// is cell within horizontal field of view?
		const double ha = asin(t * sin(a) / sqrt(t * t + camera_pose.pz * camera_pose.pz));
		if (fabs(ha) > camera_hfov * 0.5f) continue;
		
		// is cell within vertical field of view?
		const double va = atan2(0.0 - camera_pose.pz, t * cos(a)) - camera_pose.ppitch;
		if(fabs(va) > camera_vfov * 0.5f) continue;

		// determine horizontal frame coordinates
		const int fxc = width * (-ha / camera_hfov + 0.5);
		if (fxc < 0 || fxc >= (int)width) continue;
		
		// determine vertical frame coordinates
		const int fyc = height * (-va / camera_vfov + 0.5);
		if (fyc < 0 || fyc >= (int)height) continue;
		
		
		// obstacle and within fov
		// remove all lines from trimmed lines that pass through the line, (fxc, fyc), (fxc, height)
		vector<CvPoint*> fill;
		for( int j = 0; j < trimmed_lines.size(); j++ ) {
			double x_min, y_min, x_max, y_max;
			int min_index;
			if( trimmed_lines[j][0].x > trimmed_lines[j][1].x ) {
				x_min = trimmed_lines[j][1].x;
				x_max = trimmed_lines[j][0].x;
				min_index = 1;
			}else {
				x_min = trimmed_lines[j][0].x;
				x_max = trimmed_lines[j][1].x;
				min_index = 0;
			}
			
			if( trimmed_lines[j][0].y > trimmed_lines[j][1].y ) {
				y_min = trimmed_lines[j][1].y;
				y_max = trimmed_lines[j][0].y;
			}else {
				y_min = trimmed_lines[j][0].y;
				y_max = trimmed_lines[j][1].y;
			}
			
			if( x_min <= fxc && x_max >= fxc && y_max >= fyc ) {
				// possibility of intercept
				if( y_min > fyc ) {
					// always above obstacle remove it
					continue;
				}else {
					double m = (trimmed_lines[j][0].y - trimmed_lines[j][1].y)/(trimmed_lines[j][0].x - trimmed_lines[j][1].x);
					
					// get y value at fxc
					double delta_x, delta_y;
					delta_x = fxc - x_min;
					delta_y = m * delta_x;
					
					if( trimmed_lines[j][min_index].y + delta_y >= fyc ) continue;		// intersect
				}
				
			}
			fill.push_back(trimmed_lines[i]);
		}
		trimmed_lines = fill;		// copy over new set of lines
	}
	
	return trimmed_lines;
}
