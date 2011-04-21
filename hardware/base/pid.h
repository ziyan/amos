#ifndef _PID_H_
#define _PID_H_

#include <WProgram.h>

typedef struct pid {
    float p, i, d, sum, e;
} pid_t;

#ifdef __cplusplus
extern "C" {
#endif

void pid_setup();
float pid_compute(pid_t *pid, float e, float t);

#ifdef __cplusplus
}
#endif

#endif // _PID_H_

