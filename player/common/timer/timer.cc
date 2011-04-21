#include "timer.h"
#include <vector>
#include <cstring>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

using namespace std;

using namespace amos;

void Timer::printTimeval(struct timeval& t){
	if(t.tv_sec < 5){
		if(t.tv_usec < 1000 && t.tv_sec==0){
			cout<<TIMEVAL_TO_uS_64(t)<<"us";
			return;
		}
		cout<<TIMEVAL_TO_MS(t)<<"ms";
		return;
	}
	float secs=TIMEVAL_TO_SEC_F(t);
	cout<<secs<<"s";
}

int Timer::getNow(struct timeval &tv) {
	timerclear(&tv);
	if (gettimeofday(&tv, NULL)) {
		perror("getNow() gettimeofday()");
		return 0;
	}
	return 1;
}

// Returns (now - tv_prior) in microseconds
uint64_t Timer::getSince(struct timeval &tv_prior) {
	struct timeval tv;
	struct timeval result;
	timerclear(&tv);
	if (gettimeofday(&tv, NULL)) {
		perror("getSince() gettimeofday()");
		return 0;
	}
	timersub(&tv, &tv_prior, &result);
	return TIMEVAL_TO_uS_64(result);
}

uint64_t Timer::getSinceMs(struct timeval &tv_prior) {
	return getSince(tv_prior)/1000;
}


Timer::Timer(bool auto_start){
	reset();
	if(auto_start){
		start();
	}
	running=auto_start;
}

bool Timer::isGood(){
	return !overflow;
}

bool Timer::isRunning(){
	return running;
}

void Timer::start(){
	if(running || overflow){
		return;
	}

	running=true;
	struct rusage usageTemp;

	gettimeofday(&real_start, NULL);
	getrusage(RUSAGE_SELF, &usageTemp);
	user_start=usageTemp.ru_utime;
	sys_start=usageTemp.ru_stime;
}

void Timer::stop(){
	if(running){
		running=false;
		overflow = updateElapsed();
		if(overflow){
			cerr<<"Timer::pause() - ERROR, Elapsed time is not valid!"<<endl;
		}
	}
}

void Timer::resume(){
	if(!running && !overflow){
		start();
	}
}

void Timer::reset(){
	running=false;
	overflow=false;
	memset(&real_start, 0, sizeof(timeval));
	memset(&user_start, 0, sizeof(timeval));
	memset(&sys_start, 0, sizeof(timeval));
	memset(&real_elapsed, 0, sizeof(timeval));
	memset(&user_elapsed, 0, sizeof(timeval));
	memset(&sys_elapsed, 0, sizeof(timeval));
}

void Timer::print(){
	cout<<"real\t";
	printTimeval(real_elapsed);
	cout<<endl;
	cout<<"user\t";
	printTimeval(user_elapsed);
	cout<<endl;
	cout<<"sys\t";
	printTimeval(sys_elapsed);
	cout<<endl;
}

void Timer::printLine(){
	cout<<"[";
	printTimeval(real_elapsed);
	cout<<", ";
	printTimeval(user_elapsed);
	cout<<", ";
	printTimeval(sys_elapsed);
	cout<<"]"<<endl;
}

void Timer::toXml(ostream& stream){
	stream<<"<timer>"<<endl;
	stream<<"<time type=\"real\" sec=\""<<real_elapsed.tv_sec<<"\" "<<"usec=\""<<real_elapsed.tv_usec<<"\"/>"<<endl;
	stream<<"<time type=\"user\" sec=\""<<user_elapsed.tv_sec<<"\" "<<"usec=\""<<user_elapsed.tv_usec<<"\"/>"<<endl;
	stream<<"<time type=\"sys\" sec=\""<<sys_elapsed.tv_sec<<"\" "<<"usec=\""<<sys_elapsed.tv_usec<<"\"/>"<<endl;
	stream<<"</timer>"<<endl;
}

timeval Timer::getElapsedSys() {
	stop();
	resume();
	return sys_elapsed;
}

timeval Timer::getElapsedReal() {
	stop();
	resume();
	return real_elapsed;
}

timeval Timer::getElapsedUser() {
	stop();
	resume();
	return user_elapsed;
}

bool Timer::updateElapsed() {
	bool overflow=false;
	struct rusage usageTemp;
	struct timeval timeTemp, real_now;

	gettimeofday(&real_now, NULL);
	getrusage(RUSAGE_SELF, &usageTemp);

	//Calculate elapsed real time
	overflow |= !subTimeval(real_now, real_start, timeTemp);
	overflow |= !Timer::addTimeval(real_elapsed, timeTemp, real_elapsed);

	//Calculate elapsed user time
	overflow |= !subTimeval(usageTemp.ru_utime, user_start, timeTemp);
	overflow |= !addTimeval(user_elapsed, timeTemp, user_elapsed);

	//Calculate elapsed system time
	overflow |= !subTimeval(usageTemp.ru_stime, sys_start, timeTemp);
	overflow |= !addTimeval(sys_elapsed, timeTemp, sys_elapsed);

	return overflow;
}


bool Timer::addTimeval(timeval initial, timeval delta, timeval& result) {
	bool overflow=false;
	overflow |= !factorTimeval(initial);
	overflow |= !factorTimeval(delta);

	uint32_t usec = initial.tv_usec+delta.tv_usec;
	uint32_t sec = initial.tv_sec+delta.tv_sec;

	result.tv_sec = sec;
	result.tv_usec = usec;
	overflow |= !factorTimeval(result);

	//Check for overflows
	if(overflow || result.tv_sec < initial.tv_sec || result.tv_sec < delta.tv_sec){
		cerr<<"Timer::addTimeval() - ERROR, overflow!"<<endl;
		return false;
	}
	return true;
}

bool  Timer::subTimeval(const timeval final, const timeval initial, timeval& result) {
	result.tv_sec = final.tv_sec - initial.tv_sec;
	result.tv_usec = final.tv_usec - initial.tv_usec;

	//Check for underflow
	if((initial.tv_sec+result.tv_sec) != final.tv_sec || (initial.tv_usec+result.tv_usec) != final.tv_usec){
		cerr<<"Timer::subTimeval() - ERROR, underflow!"<<endl;
		return false;
	}
	return true;
}


bool Timer::factorTimeval(timeval& t) {
	if(t.tv_usec >= uS_IN_SEC ){
		__time_t seconds = t.tv_usec % uS_IN_SEC;
		seconds = t.tv_sec+seconds;

		//Check for overflow of seconds
		if(seconds < t.tv_sec){
			cerr<<"Timer::factorTimeval() - ERROR, overflow!"<<endl;
			return false;
		}

		t.tv_usec -= seconds*uS_IN_SEC;
		t.tv_sec += seconds;
	}
	return true;
}

int Timer::timevalToMs(timeval t){
	return (uint32_t)((t.tv_sec*1000) + (t.tv_usec/1000));
}
