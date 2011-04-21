#ifndef _TIMER_H_
#define _TIMER_H_

#include <WProgram.h>

extern volatile uint32_t timer_elapsed_time;

#ifdef __cplusplus
extern "C" {
#endif

void timer_setup();

#ifdef __cplusplus
}
#endif

#endif // _TIMER_H_

