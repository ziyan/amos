#ifndef _MOTOR_H_
#define _MOTOR_H_

#include <WProgram.h>
#include "pid.h"

class Motor
{
public:
    ~Motor();

    void reset();
    void setSpeed(float velocity);
    void setPID(float p, float i, float d);
    void setAcceleration(float a);
    void getOdometry(float *odometry, float *t);
    float getVoltage() const;
    float getCurrent() const;
    boolean isPowered() const;
private:
    // constructor is private so that you cannot use it!
    Motor();
    static void update();

    // PID information for the motor
    pid_t pid;
    // trapezoidal speed control limits acceleration (m/s^2)
    float acceleration;
    // goal is the real goal speed (m/s)
    // target is the current PID target (m/s)
    // output is the motor output (range from -1.0 to 1.0)
    float goal, target, output;
    // odometry for the motor (m)
    // this information is cleared every time computer reads it
    // odometry_t is seconds since last read
    float odometry, odometry_t;
    // power information, voltage in V, current in A
    float voltage, current;
    // motor time to live, when this value is 0,
    // motor goal is set to stop
    // this ensures that when computer is disconnected, we stop the motors
    float ttl;
    // flag to indicate whether the motor is being powered,
    // when e-stop is on, power might be lost and we need to
    // reset PID loop when that happens.
    boolean powered;

public:
    static void setup();
    static void run();
    static Motor left, right;
};

#endif // _MOTOR_H_

