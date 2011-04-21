#include "pid.h"

#define PID_WINDUP_MAX (1.0f)

float pid_compute(pid_t *pid, float e, float t)
{
    pid->sum += e * t;
    if (pid->sum > PID_WINDUP_MAX)
        pid->sum = PID_WINDUP_MAX;
    else if (pid->sum < -PID_WINDUP_MAX)
        pid->sum = -PID_WINDUP_MAX;
    float ret = pid->p * e +
                pid->i * pid->sum +
                pid->d * (e - pid->e) / t;
    pid->e = e;
    return ret;
}
