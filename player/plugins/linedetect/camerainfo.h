#ifndef CAM_INFO_H
#define CAM_INFO_H

#define DegToRad(x) ((x)*(M_PI/180.0))
#define RadToDeg(x) ((x)*(180.0/M_PI))



#define HFIELD_OF_VIEW DegToRad(62.7495)	//Horizontal field of view
#define VFIELD_OF_VIEW DegToRad(48.5677)	//Vertical field of view

#define CAM_HEIGHT 1.3335	//Meters from ground to camera
#define CAM_PITCH DegToRad(50.52058266)

#define LIDAR_CAM_DELTA_Y -0.1	//Distance camera is from lidar center
#define LIDAR_CAM_DELTA_X 0.05	//Distance camera is from lidar center

//static double LIDAR_CAM_DELTA=0.1;

#endif
