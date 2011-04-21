#ifndef AMOS_PLUGINS_LINE_DETECT_H
#define AMOS_PLUGINS_LINE_DETECT_H

#include "routines.h"

#include <vector>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <libplayercore/playercore.h>

using namespace std;

namespace amos{
	class LineDetectDriver : public ThreadedDriver{
		public:
			LineDetectDriver(ConfigFile*, int);
			virtual ~LineDetectDriver();
			virtual int MainSetup();
			virtual void MainQuit();
			virtual int ProcessMessage(QueuePointer&, player_msghdr*, void*);

			//void setDebug(bool val);
			//void setLaneCallback(void (*funcPtr)(vector<Line>));

			//void setSeedPoints(vector<Point> points);
			//void clearSeedPoints();


			/*double radPerPxH;
			double radPerPxV;
			double maxPitch;
			double minPitch;
			double maxYDist;
			double minYDist;
			double yDelta;
			double maxYaw;
			double maxXDist;
			double xDelta;
			int frameWidth;
			int frameHeight;
			double posDeltaX;
			double posDeltaY;
			double posDeltaYaw;
			double posPitch;*/


		protected:
			enum MediaType {Unknown=-1, Video=1, Photo=2, Camera=3, PlayerCamera=4};

			virtual void Main();

			void loadFrame();
			void parseFrame(uint8_t* img, int width, int height);
			string cvScalarToString(CvScalar val, int channels);
			void processImage();

			inline Point transformToGlobal(CvPoint& pt);

		private:
			bool debug;
			bool updated;
			int lastFrameId;

			//Mutex seedMutex;
			CvCapture* capture;
			//PlayerCc::PlayerClient* robot;
			//PlayerCc::CameraProxy* playerCam;
			//Mutex* readMutex;
			IplImage* frame;
			IplImage* input;
			IplImage* colorSpace;
			IplImage* greyImg;
			IplImage* edgeImg;
			bool showWindows;
			char inputMedia;
			vector<CvPoint*> seedPoints;

			int cannyAperture;
			int cannyThresh1;
			int cannyThresh2;
			int lineThresh;
			int minLength;
			int maxGap;
			double radPerPxH;
			double radPerPxV;

			Point origin;

			//Subscribed Devices
			player_devaddr_t camera_addr;
			Device* camera_dev;

			player_devaddr_t laser_addr;
			Device* laser_dev;

			//Publish data
			player_devaddr_t line_laser_addr;
			player_laser_data_t line_laser;

			float laser_ranges[361];
			uint8_t laser_intensity[361];
	};
};

#endif //AMOS_PLUGINS_LINE_DETECT_H

