#include <opencv/cv.h>
#include <vector>
#include <libplayercore/playercore.h>

#define OBSTACLE_DIST 6.0

std::vector<CvPoint*> trimLines(CvPoint** lines, int num_lines, double width, double height, player_laser_data_t laser_data);