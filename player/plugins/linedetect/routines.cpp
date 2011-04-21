#include "routines.h"
#include <vector>
#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <libplayerc++/playerc++.h>

using namespace std;

Point::Point(){
	x=0.0;
	y=0.0;
}

Point::Point(const player_pose2d& p){
	x=p.px;
	y=p.py;
}

double Point::magnitude(const Point& pt) const {
	return hypot(x-pt.x, y-pt.y);
}

double Point::angle(const Point& pt) const {
	return atan2(pt.y-y, pt.x-x);
}

Line::Line(){
	slope=0.0;
	yInt=0.0;
}

Line::Line(double m, double intercept){
	slope=m;
	yInt=intercept;
}

Line::Line(Point pt1, Point pt2){
	slope=((double)(pt1.y-pt2.y))/((double)(pt1.x-pt2.y));
	yInt=pt2.y-(slope*pt2.x);
}

Line::Line(CvPoint* pt1, CvPoint* pt2){
	slope=((double)(pt1->y-pt2->y))/((double)(pt1->x-pt2->y));
	yInt=pt2->y - (slope*pt2->x);
}

CvPoint Line::getCvPoint(double xVal){
	CvPoint p;
	p.x=(int)xVal;
	p.y=(int)(slope*xVal+yInt);
	return p;
}

void Line::getIntercept(double slope, Point& pt){
	pt.x=this->yInt/(this->slope-slope);
	pt.y=(this->slope*pt.x)+this->yInt;
}

Point Line::getPoint(double xVal){
	Point p;
	p.x=xVal;
	p.y=slope*xVal+yInt;
	return p;
}

ForceVector::ForceVector(){
	this->x=0.0;
	this->y=0.0;
	momentumX=0.0;
	momentumY=0.0;
	momentumSqrSum=0.0;
}

void ForceVector::clearForce(){
	this->x=0.0;
	this->y=0.0;
}

double ForceVector::magnitude() const {
	return hypot(x, y);
}

double ForceVector::angle() const {
	return atan2(x, y);
}

double ForceVector::updatePosition(Point& pt, double dt){
	if(momentumSqrSum > 0.0){	//Apply friction
		double momentumMag=sqrt(momentumSqrSum);
		x-=(momentumX/momentumMag)*FRICTION_FORCE;
		y-=(momentumY/momentumMag)*FRICTION_FORCE;
	}

	double dtMass=dt/PARTICLE_MASS;
	momentumX=x*dt;
	momentumY=y*dt;
	pt.x=(momentumX*dtMass)+pt.x;
	pt.y=(momentumY*dtMass)+pt.y;
	momentumSqrSum=(momentumX*momentumX)+(momentumY*momentumY);
	return (double)momentumSqrSum/(2.0*PARTICLE_MASS);
}

void ForceVector::add(const ForceVector& other){
	this->x+=other.x;
	this->y+=other.y;
}

void ForceVector::addSq(const Point& pt1, const Point& pt2, double charge){
	double xDiff=pt1.x-pt2.x;
	double yDiff=pt1.y-pt2.y;


	double diffMag=(xDiff*xDiff)+(yDiff*yDiff);
	double coef=(charge*PARTICLE_MASS*PARTICLE_MASS)/(diffMag*sqrt(diffMag));

	if(!isnormal(coef)){
		return;
	}

	this->x+=xDiff*coef;
	this->y+=yDiff*coef;
}


void ForceVector::addLn(const Point& pt1, const Point& pt2, double charge){
	//double xDiff=pt1.x-pt2.x;
	//double yDiff=pt1.y-pt2.y;

	//double diffMag=(xDiff*xDiff)+(yDiff*yDiff);
	//double coef=(charge*PARTICLE_MASS*PARTICLE_MASS)/(diffMag*sqrt(diffMag));

	//if(!isnormal(coef)){
		return;
	//}

	//this->x+=;
	//this->y+=yDiff*coef;
}

void ForceVector::add(const Point& pt1, const Point& pt2, double charge){
	//cout<<"ForceVector::add - pt1="<<pt1.x<<", "<<pt1.y<<" pt2="<<pt2.x<<", "<<pt2.y<<endl;
	double xDiff=pt1.x-pt2.x;
	double yDiff=pt1.y-pt2.y;


	double diffMag=(xDiff*xDiff)+(yDiff*yDiff);
	double coef=(charge*PARTICLE_MASS*PARTICLE_MASS)/(sqrt(diffMag));

	if(!isnormal(coef)){
		return;
	}

	this->x+=xDiff*coef;
	this->y+=yDiff*coef;
}

void ForceVector::addLn(const Point& pt1, const Line& line, double charge){
	Line segment0;

	if(line.slope!=0.0){
		segment0.slope=-1.0/line.slope;
	}
	else{
		segment0.slope=0.0;
	}
	segment0.yInt=pt1.y-(segment0.slope*pt1.x);

	Point pt2;
	if(line.slope==segment0.slope){ pt2.x=0.0; }
	else{ pt2.x=(segment0.yInt-line.yInt)/(line.slope-segment0.slope); }
	pt2.y=segment0.slope*pt2.x+segment0.yInt;

	addLn(pt1, pt2, charge);
}

void ForceVector::addSq(const Point &pt1, const Line &line, double charge){
	Line segment0;

	if(line.slope!=0.0){
		segment0.slope=-1.0/line.slope;
	}
	else{
		segment0.slope=0.0;
	}
	segment0.yInt=pt1.y-(segment0.slope*pt1.x);

	Point pt2;
	if(line.slope==segment0.slope){
		pt2.x=0.0;
	}
	else{
		pt2.x=(segment0.yInt-line.yInt)/(line.slope-segment0.slope);
	}
	pt2.y=segment0.slope*pt2.x+segment0.yInt;

	addSq(pt1, pt2, charge);
}

void ForceVector::add(const Point &pt1, const Line &line, double charge){
	Line segment0;

	if(line.slope!=0.0){
		segment0.slope=-1.0/line.slope;
	}
	else{
		segment0.slope=0.0;
	}
	segment0.yInt=pt1.y-(segment0.slope*pt1.x);

	Point pt2;
	if(line.slope==segment0.slope){
		pt2.x=0.0;
	}
	else{
		pt2.x=(segment0.yInt-line.yInt)/(line.slope-segment0.slope);
	}
	pt2.y=segment0.slope*pt2.x+segment0.yInt;

	add(pt1, pt2, charge);
}

void colorMaximization(IplImage* src, IplImage* dst){
	for(int row=0; row<src->height; row++){
		for(int col=0; col<src->width; col++){
			uint8_t chan0=(uint8_t)GetPixelPtrD8(src, row, col, 0);
			uint8_t chan1=(uint8_t)GetPixelPtrD8(src, row, col, 1);
			uint8_t chan2=(uint8_t)GetPixelPtrD8(src, row, col, 2);

			if(chan0 > chan1){
				if(chan0 > chan2){	//chan0 is brightest
					GetPixelPtrD8(dst, row, col, 1)=0;
					GetPixelPtrD8(dst, row, col, 2)=0;
				}
				else{	//chan2 is brightest
					GetPixelPtrD8(dst, row, col, 0)=0;
					GetPixelPtrD8(dst, row, col, 1)=0;
				}
			}
			else{
				if(chan1 > chan2){	//chan1 is brightest
					GetPixelPtrD8(dst, row, col, 0)=0;
					GetPixelPtrD8(dst, row, col, 2)=0;
				}
				else{	//chan2 is brightest
					GetPixelPtrD8(dst, row, col, 0)=0;
					GetPixelPtrD8(dst, row, col, 1)=0;
				}
			}

			//GetPixelPtrD8(distImg, row, col, 0) = (uint8_t)(dist);
		}
	}
}


void calcStats(IplImage* img, vector<CvPoint*> points, PixelStats* stats){
	if(points.size()==0){
		return;
	}

	//Calaculate average
	for(unsigned int i=0; i<points.size(); i++){
			CvPoint* pt=points.at(i);

			for(int c=0; c<img->nChannels; c++){
				stats->avgVal.val[c]+=(uint8_t)GetPixelPtrD8(img, pt->y, pt->x, c);
			}
	}

	for(int c=0; c<img->nChannels; c++){
		stats->avgVal.val[c]/=points.size();
	}

	//Calculate Variance
	for(unsigned int i=0; i<points.size(); i++){
		CvPoint* pt=points.at(i);
		//double distance=0.0;
		CvScalar distTemp=cvScalarAll(0);

		for(int c=0; c<img->nChannels; c++){
			distTemp.val[c]=((uint8_t)GetPixelPtrD8(img, pt->y, pt->x, c))-stats->avgVal.val[c];
			distTemp.val[c]*=distTemp.val[c];
			stats->variance.val[c]+=distTemp.val[c];
			//distance+=distTemp.val[c];
		}

#if CALC_COVAR
		stats->coVar[0][1]+=(((uint8_t)GetPixelPtrD8(img,pt->y,pt->x,0))/255) * (((uint8_t)GetPixelPtrD8(img,pt->y,pt->x,1))/255);

		stats->coVar[0][2]+=(((uint8_t)GetPixelPtrD8(img,pt->y,pt->x,0))/255) * (((uint8_t)GetPixelPtrD8(img,pt->y,pt->x,2))/255);

		stats->coVar[1][2]+=(((uint8_t)GetPixelPtrD8(img,pt->y,pt->x,1))/255) * (((uint8_t)GetPixelPtrD8(img,pt->y,pt->x,2))/255);

		stats->coVar[0][0]+=(((uint8_t)GetPixelPtrD8(img,pt->y,pt->x,0))/255) * (((uint8_t)GetPixelPtrD8(img,pt->y,pt->x,0))/255);

		stats->coVar[1][1]+=(((uint8_t)GetPixelPtrD8(img,pt->y,pt->x,1))/255) * (((uint8_t)GetPixelPtrD8(img,pt->y,pt->x,1))/255);

		stats->coVar[2][2]+=(((uint8_t)GetPixelPtrD8(img,pt->y,pt->x,2))/255) * (((uint8_t)GetPixelPtrD8(img,pt->y,pt->x,2))/255);
#endif
		//distance=sqrt(distance);
	}

#if CALC_COVAR
	stats->coVar[0][1]=(stats->coVar[0][1]/points.size())-(stats->avgVal.val[0]/255)*(stats->avgVal.val[1]/255);
	stats->coVar[1][0]=stats->coVar[0][1];

	stats->coVar[0][2]=(stats->coVar[0][2]/points.size())-(stats->avgVal.val[0]/255)*(stats->avgVal.val[2]/255);
	stats->coVar[2][0]=stats->coVar[0][2];

	stats->coVar[1][2]=(stats->coVar[1][2]/points.size())-(stats->avgVal.val[1]/255)*(stats->avgVal.val[2]/255);
	stats->coVar[2][1]=stats->coVar[1][2];

	stats->coVar[0][0]=(stats->coVar[0][0]/points.size())-(stats->avgVal.val[0]/255)*(stats->avgVal.val[0]/255);

	stats->coVar[1][1]=(stats->coVar[1][1]/points.size())-(stats->avgVal.val[1]/255)*(stats->avgVal.val[1]/255);

	stats->coVar[2][2]=(stats->coVar[2][2]/points.size())-(stats->avgVal.val[2]/255)*(stats->avgVal.val[2]/255);

	CvMat* invCoVarMat=cvCreateMat(3, 3, CV_32FC1);
	for(int i=0; i<3; i++){
		for(int j=0; j<3; j++){
			CvScalar v;
			v.val[0]=stats->coVar[i][j];
			cvSet2D(invCoVarMat, i, j, v);
		}
	}

	cvInvert(invCoVarMat, invCoVarMat, CV_SVD);

	for(int i=0; i<3; i++){
		for(int j=0; j<3; j++){
			CvScalar v=cvGet2D(invCoVarMat, i, j);
			stats->coVar[i][j]=v.val[0];
		}
	}

	cvReleaseMat(&invCoVarMat);
#endif

	for(int c=0; c<img->nChannels; c++){
		stats->variance.val[c]/=points.size();
		stats->stdDev.val[c]=sqrt(stats->variance.val[c])/2;
	}


}
