#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include "serial/serial.h"
#include "../../plugins/base/comm.h"

#define SAMPLE_PERIOD 		(6.0f)
#define MAX_SAMPLES		(2000)
#define STEP_RESP_SPEED		(1.5f)	

#define STEP	0
#define LINEAR	1

int main(int argc, char** argv) {

	if(argc != 8) {
		printf("Usage: %s [serial-port] [baudrate] [p] [i] [d] [max-accel] [test-type]\n", argv[0]);
		exit(0);
	}

	speed_t baudrate = serial_baudrate(atoi(argv[2]), B115200);	
	
	float left[MAX_SAMPLES], left_t[MAX_SAMPLES];
	float right[MAX_SAMPLES], right_t[MAX_SAMPLES];
	float input[MAX_SAMPLES];
	
	int sample = 0;
	float p = 0, i = 0, d = 0;
	float max_accel = 0;
	
	p = (float)atof(argv[3]);
	i = (float)atof(argv[4]);
	d = (float)atof(argv[5]);
	max_accel = (float)atof(argv[6]);
	printf("# P: %f I: %f D: %f Accel: %f\n", p, i, d, max_accel);

	if(p < 0) p = 0;
	if(i < 0) i = 0;
	if(d < 0) d = 0;
	if(max_accel < 0) max_accel = 0;

	int test_type = atoi(argv[7]);

	// open serial port
	int fd = serial_open(argv[1], baudrate);
	if(fd < 0) {
		printf("Unable to connect to serial port %s\n", argv[1]);
		exit(-1);
	}

	// reset the controller
	int result = comm_reset(fd);
	if(result) {
		printf("Unable to reset %d\n",result);
		exit(-1);
	}

	usleep(1000);

	// set PID constants and acceleration
	comm_set_pid(fd, p, i, d, p, i, d);
	comm_set_acceleration(fd, max_accel, max_accel);

	// try to get odometry at least once
	result = comm_get_odometry(fd, &left[sample], &left_t[sample], &right[sample], &right_t[sample]);
	if(result) {
		printf("Unable to get odometry %d\n",result);
		exit(-1);
	}
	
	/*
	     struct timeval tim;
             gettimeofday(&tim, NULL);
             double t1=tim.tv_sec+(tim.tv_usec/1000000.0);
             while(sample < 20000) { sample++; }
             gettimeofday(&tim, NULL);
             double t2=tim.tv_sec+(tim.tv_usec/1000000.0);
             printf("%.6lf seconds elapsed\n", t2-t1);
	*/
	
	// acquire odometry for up to SAMPLE_PERIOD seconds or until MAX_SAMPLES acquired
	struct timeval t;
	gettimeofday(&t, NULL);
	double start = t.tv_sec+(t.tv_usec/1000000.0);
	double elapsed = 0;
	double output = 0;
	do {
		gettimeofday(&t, NULL);
		elapsed = t.tv_sec+(t.tv_usec/1000000.0);
		elapsed = elapsed - start;
		
		switch(test_type) {
			case STEP:
				output = STEP_RESP_SPEED;
				break;
			case LINEAR:
				output = STEP_RESP_SPEED / SAMPLE_PERIOD * elapsed;
				break;
		}
		comm_set_speed(fd, output, output);
		
		comm_get_odometry(fd, &left[sample], &left_t[sample], &right[sample], &right_t[sample]);
		if(left_t[sample]) {
    			sample++;
			input[sample] = output;
		}
		
		usleep(10000);	// 1 ms delay
		
	} while(elapsed < SAMPLE_PERIOD && sample < MAX_SAMPLES);

	// stop motors
	comm_reset(fd);
	serial_close(fd);

	// output data
	// format: time left-velocity right-velocity
	int j;
	float left_time = 0;
	for(j = 0; j < sample; j++) {
		left_time += left_t[j];
		printf("%f %f %f\n", left_time, left[j]/left_t[j], input[j]);
	}
	return 0;
}
