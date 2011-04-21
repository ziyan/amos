#include "camera.h"

#include <time.h>

using namespace std;
using namespace amos;

CameraDriver::CameraDriver(ConfigFile* cf, int section) : ThreadedDriver(cf, section) {
	camera = cf->ReadInt(section, "camera", 0);
	width = cf->ReadTupleInt(section, "size", 0, 0);
	height = cf->ReadTupleInt(section, "size", 1, 0);
	show = string(cf->ReadString(section, "show", ""));
	PLAYER_MSG2(8,"camera: config: camera = %i, show = %s", camera, show.c_str());
	if (cf->ReadDeviceAddr(&camera_addr, section, "provides", PLAYER_CAMERA_CODE, -1, NULL) != 0) {
		PLAYER_ERROR("camera: cannot find camera addr");
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(camera_addr)) {
		PLAYER_ERROR("camera: cannot add camera interface");
		this->SetError(-1);
		return;
	}
	// some default setting
	camera_data.compression = PLAYER_CAMERA_COMPRESS_RAW;
	camera_data.bpp = 24;
	camera_data.format = PLAYER_CAMERA_FORMAT_RGB888;
	camera_data.fdiv = 1;
	camera_data.image = 0;
	PLAYER_MSG0(8,"camera: initialized");
}

CameraDriver::~CameraDriver() {
}

int CameraDriver::MainSetup() {
	if(show!="") cvNamedWindow(show.c_str(),1);
	capture = cvCaptureFromCAM(camera);
	if(!capture) {
		PLAYER_ERROR("camera: failed to capture from camera");
		return -1;
	}
	PLAYER_MSG0(8,"camera: setup");
	return 0;
}

void CameraDriver::MainQuit() {
	PLAYER_MSG0(8,"camera: shutting down");
	cvReleaseCapture(&capture);
	if(camera_data.image)
		delete [] camera_data.image;
	camera_data.image = 0;
	PLAYER_MSG0(8,"camera: shutted down");
}

int CameraDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data) {
	return -1;
}

void CameraDriver::Main()
{
	PLAYER_MSG0(8,"camera: thread started");
	IplImage *frame = 0, *small = 0;
	time_t t = time(NULL);
	int frame_counter = 0;
	for(;;)
	{
		this->TestCancel();
		this->ProcessMessages();

		if (!cvGrabFrame(capture)) {
			PLAYER_ERROR("camera: cannot grab frame");
			return;
		}
		frame = cvRetrieveFrame(capture);
		if (!frame) {
			PLAYER_ERROR("camera: cannot retrieve frame");
			return;
		}
		if (!camera_data.image) {
			if (frame->nChannels != 3 || frame->depth != IPL_DEPTH_8U) {
				PLAYER_ERROR("camera: cannot handle frames that do not have 3 channels and 8-bit depth");
				return;
			}
			camera_data.width = width <= 0 ? frame->width : width;
			camera_data.height = height <= 0 ? frame->height : height;
			camera_data.image_count = camera_data.width * camera_data.height * 3;
			camera_data.image = new uint8_t[camera_data.image_count];
		}

		// shrink?
		if(width > 0 || height > 0) {
			if(!small) small = cvCreateImage(cvSize(camera_data.width, camera_data.height), IPL_DEPTH_8U, 3);
			cvResize(frame, small, CV_INTER_LINEAR);
		}

		// convert back to rgb from bgr
		for (uint i = 0; i < camera_data.width; i++) {
			for (uint j = 0; j < camera_data.height; j++) {
				int jj = j*3*camera_data.width + i*3;
				camera_data.image[jj + 2] = (small ? small : frame)->imageData[jj + 0];
				camera_data.image[jj + 1] = (small ? small : frame)->imageData[jj + 1];
				camera_data.image[jj + 0] = (small ? small : frame)->imageData[jj + 2];
			}
		}

		this->Publish(camera_addr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, &camera_data);

		// calculate framerate
		frame_counter++;
		if(frame_counter % 100 == 0) {
			PLAYER_MSG1(9, "camera: fps = %f", (double)frame_counter/(double)(time(NULL)-t));
			frame_counter = 0;
			t = time(NULL);
		}
		if(show!="") {
			cvShowImage(show.c_str(),frame);
			cvWaitKey(10);
		}

	}
	cvReleaseImage(&frame);
	cvReleaseImage(&small);
}

Driver* driver_init(ConfigFile* cf, int section) {
	return new CameraDriver(cf, section);
}

void driver_register(DriverTable* table) {
	table->AddDriver("amoscamera", driver_init);
}

extern "C" {
	int player_driver_init(DriverTable* table) {
		driver_register(table);
		return 0;
	}
}

