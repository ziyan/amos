#include "linedetect.h"
#include "camerainfo.h"
#include <vector>
#include <sstream>

#define MAX_INPUT_WIDTH 512
#define MAX_INPUT_HEIGHT 512

using namespace std;
using namespace amos;

LineDetectDriver::LineDetectDriver(ConfigFile* cf, int section) : ThreadedDriver(cf, section) {
	debug=false;
	frame=NULL;
	input=NULL;
	colorSpace=NULL;
	greyImg=NULL;
	edgeImg=NULL;
	capture=NULL;
	camera_dev=NULL;
	showWindows=false;

	origin.x=0.0;
	origin.y=0.0;

	lineThresh=20;
	minLength=30;
	maxGap=10;
	cannyThresh1=10;
	cannyThresh2=80;
	cannyAperture=3;

	PLAYER_MSG0(3,"linedetect: constructor called");

	/*
	if(cf->ReadDeviceAddr(&laser_addr, section, "requires", PLAYER_LASER_CODE, -1, NULL)) {
		PLAYER_ERROR("linedetect: cannot find laser addr");
		this->SetError(-1);
		return;
	}*/

	string input_source=cf->ReadString(section, "input_source", "");
	if(input_source=="player"){
		inputMedia=PlayerCamera;
		if(cf->ReadDeviceAddr(&camera_addr, section, "requires", PLAYER_CAMERA_CODE, -1, NULL)) {
			PLAYER_ERROR("linedetect: cannot find camera addr");
			this->SetError(-1);
			return;
		}
	}
	else if(input_source=="video"){
		string fileName=cf->ReadString(section, "video_path", "");
		if(fileName.size()<1){
			PLAYER_ERROR("linedetect: video selected as input source, video_path must be valid!");
			this->SetError(-1);
			return;
		}

		inputMedia=Video;
		capture = cvCaptureFromAVI(fileName.c_str());
		if(!capture){
			PLAYER_ERROR1("linedetect: could not open video file[%s]", fileName.c_str());
			this->SetError(-1);
			return;
		}
		PLAYER_MSG1(6, "linedetect: Reading video file [%s]", fileName.c_str());
		loadFrame();
	}
	else if(input_source=="camera" || input_source.size()==0){
		int camera_index=cf->ReadInt(section, "camera_index", 0);
		if(input_source.size()==0){
			PLAYER_WARN1("linedetect: Defaulting to OpenCV camera index=%i", camera_index);
		}

		inputMedia=Camera;
		capture = cvCaptureFromCAM(camera_index);
		if(!capture){
			PLAYER_ERROR1("linedetect: Could not open camera %i", camera_index);
			this->SetError(-1);
			return;
		}
		PLAYER_MSG1(6, "linedetect: Reading from camera ", camera_index);
		loadFrame();
	}
	else{
		PLAYER_ERROR1("linedetect: Unsupported input source[%s] selected!", input_source.c_str());
		this->SetError(-1);
		return;
	}

	showWindows=cf->ReadBool(section, "show_windows", false);
	if(showWindows){
		PLAYER_MSG0(6, "linedetect: Showing windows");
	}


	// set up a laser interface
	if (cf->ReadDeviceAddr(&line_laser_addr, section, "provides", PLAYER_LASER_CODE, -1, NULL))
	{
		PLAYER_ERROR("linedetect: cannot find dummy opaque device addr");
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(line_laser_addr))
	{
		PLAYER_ERROR("linedetect: cannot add interface");
		this->SetError(-1);
		return;
	}

	line_laser.min_angle = DTOR(-90.0);
	line_laser.max_angle = DTOR(90.0);
	line_laser.resolution = DTOR(0.5);
	line_laser.max_range = 8.0f;
	line_laser.ranges_count = 361;
	line_laser.ranges = &laser_ranges[0];
	line_laser.intensity_count = 361;
	line_laser.intensity = &laser_intensity[0];
	line_laser.id = 0;

	PLAYER_MSG0(3,"linedetect: initialized");
}

LineDetectDriver::~LineDetectDriver(){
	if (laser_dev!=NULL){
		delete laser_dev;
		laser_dev = 0;
	}

	if(camera_dev!=NULL){
		delete camera_dev;
		camera_dev=0;
	}

	while(!seedPoints.empty()){
		delete seedPoints.back();
		seedPoints.pop_back();
	}
}

int LineDetectDriver::MainSetup(){
	PLAYER_MSG0(6,"linedetect: setup begin");

	/*if(!(laser_dev=deviceTable->GetDevice(laser_addr))){
		PLAYER_ERROR("linedetect: unable to locate suitable laser device");
		return -1;
	}
	else{
		if(laser_dev->Subscribe(InQueue) != 0)
		{
			PLAYER_ERROR("linedetect: unable to subscribe to laser device");
			laser_dev = 0;
			return -1;
		}
	}*/

	if(inputMedia==PlayerCamera){
		if(!(camera_dev=deviceTable->GetDevice(camera_addr))){
			PLAYER_ERROR("linedetect: unable to locate suitable camera device");
			return -1;
		}
		else{
			if(camera_dev->Subscribe(InQueue) != 0)
			{
				PLAYER_ERROR("linedetect: unable to subscribe to camera device");
				camera_dev = 0;
				return -1;
			}
		}
	}

	PLAYER_MSG0(6,"linedetect: setup complete");

	if(showWindows){
		cvNamedWindow("Input", CV_WINDOW_AUTOSIZE);
		cvNamedWindow("ColorSpace", CV_WINDOW_AUTOSIZE);
		cvNamedWindow("Grey", CV_WINDOW_AUTOSIZE);
		cvNamedWindow("Canny", CV_WINDOW_AUTOSIZE);
	}

	return 0;
}

void LineDetectDriver::MainQuit(){
	PLAYER_MSG0(6,"linedetect: unsubscribing");
	if(laser_dev!=NULL){
		laser_dev->Unsubscribe(this->InQueue);
		laser_dev=0;
	}

	if(camera_dev!=NULL){
		camera_dev->Unsubscribe(this->InQueue);
		camera_dev=0;
	}

	if(showWindows){ cvDestroyAllWindows(); }

	if(input!=NULL && input!=frame){ cvReleaseImage(&input); }
	if(colorSpace!=NULL){ cvReleaseImage(&colorSpace); }
	if(greyImg!=NULL){ cvReleaseImage(&greyImg); }
	if(edgeImg!=NULL){ cvReleaseImage(&edgeImg); }
	if(inputMedia==PlayerCamera){
		if(frame!=NULL){
			cvReleaseImage(&frame);
		}
	}
	else if(capture!=NULL){
		cvReleaseCapture(&capture);
	}

	PLAYER_MSG0(6,"linedetect: shut down");
}


void LineDetectDriver::Main(){
	PLAYER_MSG0(3,"linedetect: thread started");
	updated=false;
	lastFrameId=0;
	while(true){
		this->TestCancel();
		this->ProcessMessages();

		if(inputMedia!=PlayerCamera){
			loadFrame();
			updated=true;
		}

		if(updated && frame!=NULL){
			processImage();
			updated=false;
		}
		usleep(5000);
	}
}

int LineDetectDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data){
	//Process messages and stuff

	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, camera_addr) && data)
	{
		player_camera_data_t *d = (player_camera_data_t*)data;
		if (d->bpp != 24 ||
			d->format != PLAYER_CAMERA_FORMAT_RGB888 ||
			d->compression != PLAYER_CAMERA_COMPRESS_RAW ||
			d->image_count != d->width * d->height * 3)
		{
			PLAYER_WARN("linedetect: camera image format not supported.");
			return -1;
		}

		this->parseFrame(d->image, d->width, d->height);
		updated=true;
		lastFrameId++;

		return 0;
	}
	return -1;
}

void LineDetectDriver::loadFrame(){
	if(capture!=NULL && inputMedia!=PlayerCamera){
		if(!cvGrabFrame(capture)){
			cerr<<"Warning: Could not grab frame"<<endl;
			frame=NULL;
		}
		frame=cvRetrieveFrame(capture);
	}
}

void LineDetectDriver::parseFrame(uint8_t* img, int width, int height){
	if(frame==NULL){	//Construct frame
		frame=cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
	}

	// convert from RGB to OpenCV's BGR
	for (int i = 0; i<width; i++) {
		for (int j = 0; j<height; j++) {
			int jj = frame->width * j * 3 + i * 3;
			// swap color
			frame->imageData[jj + 0]=img[jj + 2];
			frame->imageData[jj + 1]=img[jj + 1];
			frame->imageData[jj + 2]=img[jj + 0];
		}
	}
}

string LineDetectDriver::cvScalarToString(CvScalar val, int channels=3){
	stringstream stream;

	stream<<"[";
	for(int i=0; i<channels; i++){
		stream<<val.val[i];
		if(i+1!=channels){
			stream<<", ";
		}
	}
	stream<<"]";
	return stream.str();
}

void LineDetectDriver::processImage(){

	if(frame==NULL){
		return;
	}

	if(frame->width > MAX_INPUT_WIDTH || frame->height > MAX_INPUT_HEIGHT){
		int width = frame->width/2;
		int height = frame->height/2;
		if(input==frame || input==NULL){
			input = cvCreateImage( cvSize(width, height), frame->depth, frame->nChannels );
			radPerPxH=HFIELD_OF_VIEW/((double)width);
			radPerPxV=VFIELD_OF_VIEW/((double)height);
		}
		cvResize(frame, input);
	}
	else{
		input=frame;
		radPerPxH=HFIELD_OF_VIEW/((double)input->width);
		radPerPxV=VFIELD_OF_VIEW/((double)input->height);
	}

	if(greyImg==NULL){
		greyImg=cvCreateImage(cvGetSize(input), IPL_DEPTH_8U, 1);
	}
	if(edgeImg==NULL){
		edgeImg=cvCreateImage(cvGetSize(input), IPL_DEPTH_8U, 1);
	}
	if(colorSpace==NULL){
		colorSpace=cvCreateImage(cvGetSize(input), IPL_DEPTH_8U, 3);
	}

	cvCvtColor(input, colorSpace, CV_RGB2HSV);

	//Copy hue color channel into buffer
	cvSplit(colorSpace, greyImg, NULL, NULL, NULL);

	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* lines = 0;


	cvCanny(greyImg, edgeImg, cannyThresh1, cannyThresh2, cannyAperture);



	lines = cvHoughLines2( edgeImg,
						   storage,
						   CV_HOUGH_PROBABILISTIC,
						   1,
						   CV_PI/180,
						   lineThresh+1,
						   minLength,
						   maxGap );

	//Delete old points
	while(!seedPoints.empty()){
		delete seedPoints.back();
		seedPoints.pop_back();
	}

	for(int row=colorSpace->height/2; row<colorSpace->height; row+=10){
		for(int col=0; col < colorSpace->width; col+=10){
			CvPoint* pt=new CvPoint;
			pt->x=col;
			pt->y=row;
			seedPoints.push_back(pt);
		}
	}

	PixelStats stats;
	calcStats(colorSpace, seedPoints, &stats);
	vector<CvPoint*>::iterator iter=seedPoints.begin();
	while(iter!=seedPoints.end()){
		CvPoint* pt=*iter;
		double chan0Delta=fabs(((uint8_t)GetPixelPtrD8(colorSpace, pt->y, pt->x, 0)) - stats.avgVal.val[0]);
		double chan1Delta=fabs(((uint8_t)GetPixelPtrD8(colorSpace, pt->y, pt->x, 1)) - stats.avgVal.val[1]);
		double chan2Delta=fabs(((uint8_t)GetPixelPtrD8(colorSpace, pt->y, pt->x, 2)) - stats.avgVal.val[2]);
		if(chan0Delta > stats.stdDev.val[0]*1 /*||
			chan1Delta > stats.stdDev.val[1]*1.5 ||
			chan2Delta > stats.stdDev.val[2]*1.5*/){
			delete (*iter);
			iter=seedPoints.erase(iter);
			continue;
		}
		iter++;
	}
	calcStats(colorSpace, seedPoints, &stats);

	int removed=0;
	vector<CvPoint*> acceptedLines;

	for(int i=0; i<lines->total; i++){
		CvPoint* line = (CvPoint*)cvGetSeqElem(lines,i);
		CvLineIterator iterator;
		int count=cvInitLineIterator( colorSpace, line[0], line[1], &iterator, 8, 0 );

		CvScalar avgVal=cvScalarAll(0);
		CvScalar delta=cvScalarAll(0);
		CvScalar variance=cvScalarAll(0);
		CvScalar stdDev=cvScalarAll(0);


		for(int p=0; p<count; p++){	//Loop over pixels in line
			avgVal.val[0]+=iterator.ptr[0];
			avgVal.val[1]+=iterator.ptr[1];
			avgVal.val[2]+=iterator.ptr[2];
			CV_NEXT_LINE_POINT(iterator);
		}

		avgVal.val[0]=avgVal.val[0]/count;
		avgVal.val[1]=avgVal.val[1]/count;
		avgVal.val[2]=avgVal.val[2]/count;

		count=cvInitLineIterator( colorSpace, line[0], line[1], &iterator, 8, 0 );

		for(int p=0; p<count; p++){	//Loop over pixels in line
			variance.val[0]+=(iterator.ptr[0]-avgVal.val[0])*(iterator.ptr[0]-avgVal.val[0]);
			variance.val[1]+=(iterator.ptr[1]-avgVal.val[1])*(iterator.ptr[1]-avgVal.val[1]);
			variance.val[2]+=(iterator.ptr[2]-avgVal.val[2])*(iterator.ptr[2]-avgVal.val[2]);
			CV_NEXT_LINE_POINT(iterator);
		}

		variance.val[0]/=count;
		variance.val[1]/=count;
		variance.val[2]/=count;

		delta.val[0]=fabs(avgVal.val[0]-stats.avgVal.val[0]);
		delta.val[1]=fabs(avgVal.val[1]-stats.avgVal.val[1]);
		delta.val[2]=fabs(avgVal.val[2]-stats.avgVal.val[2]);

		stdDev.val[0]=sqrt(variance.val[0]);
		stdDev.val[1]=sqrt(variance.val[1]);
		stdDev.val[2]=sqrt(variance.val[2]);

					int failCount=0;

					if(delta.val[0] < 10*stats.stdDev.val[0]){
						failCount++;
						removed++;
						continue;
					}

					if(delta.val[1] < 2*stats.stdDev.val[1]){
						failCount++;
					}

					if(delta.val[2] < 4*stats.stdDev.val[2]){
						failCount++;
					}

					if(failCount>=2){
						removed++;
						continue;
					}

					//Dark grass Checking for HSV
					/*if(avgVal.val[0] > stats.avgVal.val[0]){
						removed++;
						continue;
					}
					else if(avgVal.val[1] > stats.avgVal.val[1]){
						removed++;
						continue;
					}
					else if(avgVal.val[2] < stats.avgVal.val[2]){
						removed++;
						continue;
					}*/

		//cout<<"Keeping deltaAvg="<<cvScalarToString(delta,3)<<" stdDev="<<cvScalarToString(stdDev,3)<<endl;
		//cvLine( input, line[0], line[1], CV_RGB(0, 255, 0), 1);
		acceptedLines.push_back(line);
	}

	cout<<"Found "<<lines->total<<" lines rejected "<<removed<<" lines in frame"<<lastFrameId<<endl;

	//cout<<"\tAvg="<<cvScalarToString(stats.avgVal,3)<<endl;
	//cout<<"\tVariance="<<cvScalarToString(stats.variance,3)<<endl;
	//cout<<"\tStdDev="<<cvScalarToString(stats.stdDev,3)<<endl;


	//Clear old lidar scan
	for(unsigned int i=0; i<=line_laser.ranges_count; i++){
		laser_ranges[i]=line_laser.max_range;
	}

	//PLAYER_MSG2(2, "Accepted %i lines out of %i detected", acceptedLines.size(), lines->total);

	//Build lidar scan
	for(unsigned int i=0; i<acceptedLines.size(); i++){
		CvPoint* line=acceptedLines[i];
		Point pt1=transformToGlobal(line[0]);
		Point pt2=transformToGlobal(line[1]);

		double angle1=atan2(pt1.y, pt1.x);
		double angle2=atan2(pt2.y, pt2.x);

		int angle1Deg=RadToDeg(angle1)*2;
		int angle2Deg=RadToDeg(angle2)*2;

		double angleStep=DegToRad(0.25);
		Point current;
		Line cLine(pt1, pt2);
		if(angle1Deg < angle2Deg){
					for(int angle=angle1Deg; angle < angle2Deg; angle++){
						cLine.getIntercept(tan(DegToRad(((double)angle/2.0))), current);
						double dist=hypot(current.x, current.y);
						//int i=(180-(int)(RadToDeg(angle)))*2;
						laser_ranges[angle]=fminf(laser_ranges[angle], (float)dist);
					}
				}
				else{
					for(int angle=angle2Deg; angle < angle1Deg; angle++){
						cLine.getIntercept(tan(DegToRad(((double)angle/2.0))), current);
						double dist=hypot(current.x, current.y);
						//int i=(180-(int)(RadToDeg(angle)))*2;
						laser_ranges[angle]=fminf(laser_ranges[angle], (float)dist);
					}
				}

		/*if(angle1 < angle2){
			for(double angle=angle1; angle < angle2; angle+=angleStep){
				cLine.getIntercept(tan(angle), current);
				double dist=hypot(current.x, current.y);
				int i=(180-(int)(RadToDeg(angle)))*2;
				laser_ranges[i]=fminf(laser_ranges[i], (float)dist);
			}
		}
		else{
			for(double angle=angle2; angle < angle1; angle+=angleStep){
				cLine.getIntercept(tan(angle), current);
				double dist=hypot(current.x, current.y);
				int i=(180-(int)(RadToDeg(angle)))*2;
				laser_ranges[i]=fminf(laser_ranges[i], (float)dist);
			}
		}*/
	}

	line_laser.id++;
	this->Publish(line_laser_addr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN, &line_laser);

	cvReleaseMemStorage(&storage);

	if(showWindows){
		cvShowImage("Input", input);
		cvShowImage("Grey", greyImg);
		cvShowImage("ColorSpace", colorSpace);
		cvShowImage("Canny", edgeImg);
		cvWaitKey(10);
	}
}

Point LineDetectDriver::transformToGlobal(CvPoint& pt){
	Point p;
	double rowPitch=CAM_PITCH+(VFIELD_OF_VIEW/2.0 - ((double)pt.y*radPerPxV));
	p.y=(CAM_HEIGHT*tan(rowPitch))+LIDAR_CAM_DELTA_Y;

	double colYaw=(radPerPxH*(double)pt.x)-(HFIELD_OF_VIEW/2.0);
	p.x=(p.y*tan(colYaw))+LIDAR_CAM_DELTA_X;

	return p;
}


Driver* driver_init(ConfigFile* cf, int section) {
	return new LineDetectDriver(cf, section);
}

void driver_register(DriverTable* table) {
	table->AddDriver("amoslinedetect", driver_init);
}

extern "C" {
	int player_driver_init(DriverTable* table) {
		driver_register(table);
		return 0;
	}
}



