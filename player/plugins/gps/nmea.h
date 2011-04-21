#ifndef AMOS_PLUGINS_GPS_NMEA_H
#define AMOS_PLUGINS_GPS_NMEA_H

#include <libplayerinterface/player.h>

#define NMEA_ERR_SUCCESS 0
#define NMEA_ERR_READ 1
#define NMEA_ERR_FORMAT 2

#ifdef __cplusplus
extern "C" {
#endif

extern int nmea(int fd, player_gps_data_t *gps, player_position2d_data_t *position2d);

#ifdef __cplusplus
}
#endif

#endif // AMOS_PLUGINS_GPS_NMEA_H

