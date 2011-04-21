#ifndef _VICTOR_H_
#define _VICTOR_H_

#include <WProgram.h>

#define VICTOR_MAX (1.0f)

extern volatile boolean victor_timer_overflow;

#ifdef __cplusplus
extern "C" {
#endif


void victor_setup();
void victor_set_speed(float left, float right);

#ifdef __cplusplus
}
#endif

#endif
