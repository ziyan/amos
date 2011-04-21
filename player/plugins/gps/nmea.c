#include "nmea.h"
#include "utm/utm.h"
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <libplayercommon/error.h>

#define SERIAL_TIMEOUT 1000000
#define NMEA_TIMEOUT 25

#define NMEA_GPGGA "GPGGA"
#define NMEA_GPRMC "GPRMC"

#define NMEA_START_CHAR '$'
#define NMEA_END_CHAR '\n'
#define NMEA_CHKSUM_CHAR '*'

#define NMEA_MAX_SENTENCE_LEN 128
#define NMEA_MAX_FIELD_LEN 32

char nmea_buf[NMEA_MAX_SENTENCE_LEN + 1] = {0};
size_t nmea_buf_len = 0;

int nmea_buffer(int fd)
{
	fd_set input;
	struct timeval timer;
	FD_ZERO(&input);
	FD_SET(fd, &input);
	timer.tv_sec = SERIAL_TIMEOUT / 1000000;
	timer.tv_usec = SERIAL_TIMEOUT % 1000000;
	fcntl(fd, F_SETFL, 0);

	if (select(fd + 1, &input, NULL, NULL, &timer) <= 0) return -1;
	int s = read(fd, nmea_buf + nmea_buf_len, NMEA_MAX_SENTENCE_LEN - nmea_buf_len);
	if(s < 0) return -1;
	nmea_buf_len += s;
	nmea_buf[nmea_buf_len] = '\0';
	return 0;
}

int nmea_read(int fd, char* buf, size_t len)
{
	char *ptr = 0;
	size_t length = 0;
	int checksum = 0, i = 0;

	// find start of the message
	while(!(ptr = strchr(nmea_buf, NMEA_START_CHAR)))
	{
		nmea_buf_len = 0;
		memset(nmea_buf, 0, sizeof(char) * (NMEA_MAX_SENTENCE_LEN + 1));
		if (i++ > NMEA_TIMEOUT) return -1;
		if (nmea_buffer(fd)) return -1;
	}

	// move sentence to start of the buffer
	nmea_buf_len = strlen(ptr);
	memmove(nmea_buf, ptr, strlen(ptr) + 1);

	// find end of the sentence
	while(!(ptr = strchr(nmea_buf, NMEA_END_CHAR)))
	{
		if(i++ > NMEA_TIMEOUT || nmea_buf_len >= NMEA_MAX_SENTENCE_LEN)
		{
			// couldn't get an end char and the buffer is full.
				memset(nmea_buf, 0, sizeof(char) * (NMEA_MAX_SENTENCE_LEN + 1));
			nmea_buf_len = 0;
			return -1;
		}
		if (nmea_buffer(fd)) return -1;
	}

	// length of the sentence
	length = nmea_buf_len - strlen(ptr) + 1;
	if(length - 3 > len - 1 || length <= 3) return -1;

	strncpy(buf, nmea_buf + 1, length - 3);
	buf[length - 3] = '\0';

	// move stuffs to the front
	memmove(nmea_buf, ptr + 1, strlen(ptr) - 1);
	nmea_buf_len -= length;
	nmea_buf[nmea_buf_len]='\0';

	// find checksum
	if ((ptr = strchr(buf, NMEA_CHKSUM_CHAR)) && (strlen(ptr) == 3))
	{
		// calculate checksum
		for(i = 0; i < (int)(strlen(buf) - 3); i++)
			checksum ^= buf[i];

		// strip off checksum
		*ptr = '\0';

		// compare checksum
		if (checksum != strtol(ptr + 1, NULL, 16)) return -1;
	}
	else
	{
		return -1;
	}
	return 0;
}

const char *nmea_next_field(const char *ptr, char* field, size_t len)
{
	assert(ptr);
	const char* end = 0;
	size_t length = 0;

	// try to find next , separator
	if (!(end = strchr(ptr, ',')))
	{
		// could not find ',' this field must be whatever is left in the buffer
		length = strlen(ptr);
	}
	else
	{
		// found ','
		length = strlen(ptr) - strlen(end);
	}

	// truncate if field buffer is not large enough
	if (length > len - 1) length = len - 1;

	// copy field content
	strncpy(field, ptr, length);
	field[length] = '\0';

	// if we cannot find ',', we've reached the end
	if (!end) return 0;

	// next field starts at one after the ','
	return end + 1;
}

int nmea_parse_gpgga(char *buf, player_gps_data_t *gps, player_position2d_data_t *position2d)
{
	const char* ptr = buf;
	char field[NMEA_MAX_FIELD_LEN + 1] = {0};
	char tmp[4] = {0};
	double degrees = 0.0, minutes = 0.0;
	double lat = 0.0, lon = 0.0;

	assert(buf && gps && position2d);

	// 1. time of day
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	if (strlen(field) == 9 && field[6] == '.')
	{
		gps->time_usec = 10000 * atoi(field + 7);
	}

	// 2. latitude
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	if (strlen(field) >= 2)
	{
		strncpy(tmp, field, 2);
		tmp[2] = '\0';
		degrees = atoi(tmp);
		minutes = atof(field + 2);
		lat = ((degrees * 60.0) + minutes) * 60.0 / 3600.0;
	}

	// 3. north or south
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	if (field[0] == 'S') lat = -lat;
	gps->latitude = (int32_t)rint(lat * 1e7);

	// 4. longitude
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	if (strlen(field) >= 3)
	{
		strncpy(tmp, field, 3);
		tmp[3] = '\0';
		degrees = atoi(tmp);
		minutes = atof(field + 3);
		lon = ((degrees * 60.0) + minutes) * 60.0 / 3600.0;
	}

	// 5. east or west
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	if (field[0] == 'W') lon = -lon;
	gps->longitude = (int32_t)rint(lon * 1e7);

	// 6. quality
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	gps->quality = atoi(field);

	// 7. number of sats in view
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	gps->num_sats = atoi(field);

	// 8. HDOP
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	gps->hdop = (uint16_t)rint(atof(field) * 10.0);

	// 9. altitude
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	gps->altitude = (int32_t)rint(atof(field) * 1000.0);

	// 10. 'M'
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;

	// 11. height of geoid above WGS84 ellipsoid
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;

	// 12. 'M'
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;

	// 13. DGPS age
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;

	// 14. DGPS base station ID
	if ((ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;

	utm(lat, lon, &gps->utm_e, &gps->utm_n);
	position2d->pos.px = gps->utm_e;
	position2d->pos.py = gps->utm_n;

	return 0;
}

int nmea_parse_gprmc(char *buf, player_gps_data_t *gps, player_position2d_data_t *position2d)
{
	const char* ptr = buf;
	char field[NMEA_MAX_FIELD_LEN + 1] = {0};
	char tmp[4] = {0};
	struct tm tms = {0};
	//double degrees = 0.0, minutes = 0.0;
	//double lat = 0.0, lon = 0.0;

	assert(buf && gps && position2d);

	// 1. utc
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	if (strlen(field) < 6) return -1;
	strncpy(tmp, field, 2);
	tmp[2] = '\0';
	tms.tm_hour = atoi(tmp);
	strncpy(tmp, field + 2, 2);
	tmp[2] = '\0';
	tms.tm_min = atoi(tmp);
	strncpy(tmp, field + 4, 2);
	tmp[2] = '\0';
	tms.tm_sec = atoi(tmp);

	// 2. Status
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;

	// 3. latitude
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	/*if (strlen(field) < 2) return -1;
	strncpy(tmp, field, 2);
	tmp[2] = '\0';
	degrees = atoi(tmp);
	minutes = atof(field + 2);
	lat = ((degrees * 60.0) + minutes) * 60.0 / 3600.0;
	*/

	// 4. north or south
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	//if (field[0] == 'S') lat = -lat;
	//gps->latitude = (int32_t)rint(lat * 1e7);

	// 5. longitude
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	/*if (strlen(field) < 3) return -1;
	strncpy(tmp, field, 3);
	tmp[3] = '\0';
	degrees = atoi(tmp);
	minutes = atof(field + 3);
	lon = ((degrees * 60.0) + minutes) * 60.0 / 3600.0;
	*/

	// 6. east or west
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	//if(field[0] == 'W') lon = -lon;
	//gps->longitude = (int32_t)rint(lon * 1e7);

	// 7. ground speed in knots
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	position2d->vel.px = 1852.0 * atof(field);

	// 8. track made heading
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	position2d->pos.pa = (-atof(field) + 90.0) / 180.0 * M_PI;
	while (position2d->pos.pa >= M_PI) position2d->pos.pa -= M_PI + M_PI;
	while (position2d->pos.pa < -M_PI) position2d->pos.pa += M_PI + M_PI;

	// 9. date
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	if (strlen(field) != 6) return -1;
	strncpy(tmp, field, 2);
	tmp[2] = '\0';
	tms.tm_mday = atoi(tmp);
	strncpy(tmp, field + 2, 2);
	tmp[2] = '\0';
	tms.tm_mon = atoi(tmp) - 1;
	strncpy(tmp, field + 4, 2);
	tmp[2] = '\0';
	tms.tm_year = 100 + atoi(tmp);
	// year starts from 1900, in the year of 2100, you need to change this to 200 instead of 100

	// 10. magnetic variation
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	position2d->vel.pa = DTOR(atof(field));

	// 11. direction of magnetic variation
	if (!(ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;
	if (field[0] == 'E') position2d->vel.pa = -position2d->vel.pa;
	while (position2d->vel.pa >= M_PI) position2d->vel.pa -= M_PI + M_PI;
	while (position2d->vel.pa < -M_PI) position2d->vel.pa += M_PI + M_PI;

	// 12. mode indication
	if ((ptr = nmea_next_field(ptr, field, NMEA_MAX_FIELD_LEN + 1))) return -1;

	gps->time_sec = (uint32_t)mktime(&tms);

	//utm(lat, lon, &gps->utm_e, &gps->utm_n);
	//position2d->pos.px = gps->utm_e;
	//position2d->pos.py = gps->utm_n;

	return 0;
}

int nmea(int fd, player_gps_data_t *gps, player_position2d_data_t *position2d)
{
	char buf[NMEA_MAX_SENTENCE_LEN + 1] = {0};
	player_gps_data_t gps_data;
	player_position2d_data_t position2d_data;

	assert(fd >= 0 && gps && position2d);
	gps_data = *gps;
	position2d_data = *position2d;

	if (nmea_read(fd, buf, NMEA_MAX_SENTENCE_LEN + 1)) return -NMEA_ERR_READ;
	PLAYER_MSG1(9, "gps: %s", buf);

	if (strlen(buf) < 6) return -NMEA_ERR_FORMAT;
	if (strncmp(buf, NMEA_GPGGA, strlen(NMEA_GPGGA)) == 0 && buf[strlen(NMEA_GPGGA)] == ',')
	{
		if (nmea_parse_gpgga(buf + strlen(NMEA_GPGGA) + 1, &gps_data, &position2d_data)) return -NMEA_ERR_FORMAT;
	}
	else if (strncmp(buf, NMEA_GPRMC, strlen(NMEA_GPRMC)) == 0 && buf[strlen(NMEA_GPRMC)] == ',')
	{
		if (nmea_parse_gprmc(buf + strlen(NMEA_GPRMC) + 1, &gps_data, &position2d_data)) return -NMEA_ERR_FORMAT;
	}
	else
	{
		return -NMEA_ERR_SUCCESS;
	}

	*gps = gps_data;
	*position2d = position2d_data;
	return -NMEA_ERR_SUCCESS;
}

