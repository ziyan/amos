#include <libplayerc++/playerc++.h>
#include <iostream>
#include <sstream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <time.h>
#include <assert.h>

using namespace std;
using namespace PlayerCc;

int main(int argc, char** argv)
{
	IplImage *img = 0;
	CvVideoWriter* video = 0;
	uint32_t frame_count = 0;
	time_t t = time(NULL);

	if(argc < 2)
	{
		cerr << "Usage: " << argv[0] << " <hostname> [port] [id] [vlog-path] " <<endl;
		return -1;
	}
	
	// determine arguments
	int port = (argc > 2) ? atoi(argv[2]) : 6665;
	const char *logging = argc > 3 ? argv[3] : 0;

	// connect to robot and camera
	PlayerClient robot(argv[1], port);
	CameraProxy camera(&robot, argc>3?atoi(argv[3]):0);

	// create a display window with a descriptive name
	ostringstream os;
	os<<"camviewer: " << argv[1] << ":" << port << ":" << camera.GetIndex();
	string windowName = os.str();
	cvNamedWindow(windowName.c_str(),1);
	
	for(;;)
	{
		// read from remote robot
		robot.Read();
		
		// make sure format is supported
		if (camera.GetFormat() != PLAYER_CAMERA_FORMAT_RGB888)
		{
			cerr << "camviewer: format not supported" << endl;
			break;
		}

		// check if the image requires decompression
		if (camera.GetCompression() == PLAYER_CAMERA_COMPRESS_JPEG)
			camera.Decompress();

		// initialize frame and video if not already
		if (!img)
		{
			img = cvCreateImage(cvSize(camera.GetWidth(), camera.GetHeight()), IPL_DEPTH_8U, 3);
		}
		
		if(logging && !video)
		{
			video = cvCreateVideoWriter(logging, CV_FOURCC('M','J','P','G'), 5.0, cvSize(camera.GetWidth(),camera.GetHeight()));
		}
		
		// make sure the image format we have is consistent
		assert(img->width == (int)camera.GetWidth() &&
			img->height == (int)camera.GetHeight() &&
			img->nChannels == 3 &&
			img->depth == IPL_DEPTH_8U);

		// retrieve pixel data
		camera.GetImage((uint8_t*)img->imageData);

		// convert from RGB to OpenCV's BGR
		for (int i = 0; i<img->width; i++) {
			for (int j = 0; j<img->height; j++) {
				int jj = img->width * j * 3 + i * 3;
				// swap color
				char temp = img->imageData[jj + 0];
				img->imageData[jj + 0] = img->imageData[jj + 2];
				img->imageData[jj + 2] = temp;
			}
		}

		// display image
		cvShowImage(windowName.c_str(), img);
		
		// log video
		if (logging)
			cvWriteFrame(video, img);
			
		// OpenCV gui stuff
		if (cvWaitKey(10) != -1)
			break;
		
		// stats
		if (frame_count++ % 20 == 0)
			cout << "camviewer: fps = " << ((double)frame_count/(double)(time(NULL)-t)) << endl;
	}
	cvReleaseImage(&img);
	return 0;
}

