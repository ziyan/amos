#ifndef _ENCODER_H_
#define _ENCODER_H_

#include <WProgram.h>

extern volatile int32_t encoder_left, encoder_right;

#ifdef __cplusplus
extern "C" {
#endif

void encoder_setup();

#ifdef __cplusplus
}
#endif

#endif // _ENCODER_H_

