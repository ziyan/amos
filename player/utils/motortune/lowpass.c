#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>

#define SAMPLE_PERIOD 		8.0
#define MAX_SAMPLES		2000
#define STEP_RESP_SPEED		1.5

#define ALPHA	(0.5f)

int main(int argc, char** argv) {

	FILE *fp = fopen("open-loop.dat", "r");
	
	if(fp == NULL) {
		printf("File error");
		exit(-1);
	}
	
	float prev = 0;
	float output = 0;
	
	float time = 0, left = 0, right = 0;
	while (!feof(fp)) {
		if (fscanf(fp, "%f %f %f", &time, &left, &right) != 3) {
			printf("fail\n");
			break;
		}
		output = ALPHA * left + (1.0f - ALPHA)*prev;
		prev = output;
		printf("%f %f\n", time, output);
	}
	
	fclose(fp);

	return 0;
}
