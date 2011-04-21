#ifndef AMOS_PLUGINS_CAMERA_H
#define AMOS_PLUGINS_CAMERA_H

#include <string>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <libplayercore/playercore.h>

namespace amos
{
	class CameraDriver : public ThreadedDriver
	{
	public:
		CameraDriver(ConfigFile*, int);
		virtual ~CameraDriver();
		virtual int MainSetup();
		virtual void MainQuit();
		virtual int ProcessMessage(QueuePointer&, player_msghdr*, void*);

	protected:
		virtual void Main();

		player_devaddr_t camera_addr;
		player_camera_data_t camera_data;

		CvCapture *capture;
		int camera;
		int width, height;
		std::string show;
	};
}

#endif

