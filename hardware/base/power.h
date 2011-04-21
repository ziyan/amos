#ifndef _POWER_H_
#define _POWER_H_

#include <WProgram.h>


#ifdef __cplusplus
extern "C" {
#endif

float power_get_voltage_left();
float power_get_current_left();
float power_get_voltage_right();
float power_get_current_right();

#ifdef __cplusplus
}
#endif

#endif // _POWER_H_

