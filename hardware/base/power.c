#include "power.h"

#define POWER_VOLTAGE_LEFT 0
#define POWER_CURRENT_LEFT 1
#define POWER_VOLTAGE_RIGHT 2
#define POWER_CURRENT_RIGHT 3

#define POWER_VOLTAGE_CONVERSION(v) ((float)v * 0.0758)
#define POWER_CURRENT_CONVERSION(v) (v)

float power_get_voltage_left()
{
    return POWER_VOLTAGE_CONVERSION(analogRead(POWER_VOLTAGE_LEFT));
}

float power_get_current_left()
{
    return POWER_CURRENT_CONVERSION(analogRead(POWER_CURRENT_LEFT));
}

float power_get_voltage_right()
{
    return POWER_VOLTAGE_CONVERSION(analogRead(POWER_VOLTAGE_RIGHT));
}

float power_get_current_right()
{
    return POWER_CURRENT_CONVERSION(analogRead(POWER_CURRENT_RIGHT));
}
