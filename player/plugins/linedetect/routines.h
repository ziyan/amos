#ifndef ROUTINES_H
#define ROUTINES_H

#include <vector>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <libplayerc++/playerc++.h>

#define POINT_CHARGE 0.02
#define LINE_CHARGE  0.05
#define BACKWALL_CHARGE 0.8
#define ATTRACT_CHARGE -1.8
#define TIME_STEP	0.2
#define PARTICLE_MASS 1.0
#define FRICTION_FORCE 0.001

#define MAX_STEP_SIZE 0.2

using namespace std;

#define GetPixelPtrD8(i,r,c,chan) i->imageData[((r*i->widthStep)+(c*i->nChannels)+chan)]
#define GetPixelPtrD16(i,r,c,chan) i->imageData[((r*(i->widthStep/sizeof(uint16_t)))+(c*i->nChannels)+chan)]
#define GetPixelPtrD32(i,r,c,chan) ((float*)i->imageData)[((r*i->widthStep)/4+(c*i->nChannels)+chan)]

#define CALC_COVAR false

void colorMaximization(IplImage* src, IplImage* dst);

struct PixelStats{
	PixelStats(){
		avgVal=cvScalarAll(0);
		variance=cvScalarAll(0);
		stdDev=cvScalarAll(0);
		memset((void*)coVar, 0, 3*3*sizeof(double));
	}

	CvScalar avgVal;
	CvScalar variance;
	CvScalar stdDev;
	double coVar[3][3];
};

struct Point{
	Point();
	Point(const player_pose2d& p);
	double magnitude(const Point& pt) const;
	double angle(const Point& pt) const;

	double x;
	double y;
};

struct Line{
	Line();
	Line(Point pt1, Point pt2);
	Line(double m, double intercept);
	Line(CvPoint* pt1, CvPoint* pt2);

	//! Returns a point on the line at xVal
	CvPoint getCvPoint(double xVal);
	Point getPoint(double xVal);

	/**
	  *	Test whether a ray from the origin at the given slope
	  *	intersects this line. Updates pt to reflect the location
	  *	of the intersection.
	  */
	void getIntercept(double slope, Point& pt);

	/*Point pt1;
	Point pt2;*/
	double slope;
	double yInt;
	double angle;
};

struct ForceVector{
	ForceVector();
	void clearForce();
	//! Return particles energy
	double updatePosition(Point& pt, double dt=TIME_STEP);
	double magnitude() const;
	double angle() const;

	double momentumX;
	double momentumY;
	double momentumSqrSum;
	double x;
	double y;
	void add(const ForceVector& other);
	void addSq(const Point& pt1, const Point& pt2, double charge=POINT_CHARGE);
	void add(const Point& pt1, const Point& pt2, double charge=POINT_CHARGE);
	void addLn(const Point& pt1, const Point& pt2, double charge=POINT_CHARGE);
	void addLn(const Point& pt1, const Line& line, double charge=LINE_CHARGE);
	void addSq(const Point& pt1, const Line& line, double charge=LINE_CHARGE);
	void add(const Point& pt1, const Line& line, double charge=LINE_CHARGE);
};




void calcStats(IplImage* img, vector<CvPoint*> points, PixelStats* stats);
#endif
