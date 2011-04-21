#ifndef AMOS_COMMON_MAP_DEFINE_H
#define AMOS_COMMON_MAP_DEFINE_H

#include <stdint.h>

#define MAP_CHANNEL_P				((uint32_t)0)
#define MAP_CHANNEL_P_CSPACE		((uint32_t)1)
#define MAP_CHANNEL_E_AVG			((uint32_t)2)
#define MAP_CHANNEL_E_VAR			((uint32_t)3)
#define MAP_CHANNEL_R				((uint32_t)4)
#define MAP_CHANNEL_G				((uint32_t)5)
#define MAP_CHANNEL_B				((uint32_t)6)
#define MAP_CHANNEL_DEFAULTS		{ 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }

#define MAP_P_MAX (1.0f)
#define MAP_P_MIN (0.0f)
#define MAP_P_OBSTACLE_THRESHOLD (0.75f)

#endif // AMOS_COMMON_MAP_DEFINE_H

