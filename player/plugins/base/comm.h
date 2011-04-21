#ifndef AMOS_PLUGINS_BASE_COMM_H
#define AMOS_PLUGINS_BASE_COMM_H

#ifdef __cplusplus
extern "C" {
#endif

int comm_reset(int fd);
int comm_set_speed(int fd, const float left, const float right);
int comm_set_pid(int fd, const float p_left, const float i_left, const float d_left, const float p_right, const float i_right, const float d_right);
int comm_set_acceleration(int fd, const float a_left, const float a_right);
int comm_get_odometry(int fd, float *left, float *left_t, float *right, float *right_t);
int comm_get_power(int fd, float *voltage_left, float *current_left, float *voltage_right, float *current_right);

#ifdef __cplusplus
}
#endif

#endif // AMOS_PLUGINS_BASE_COMM_H

