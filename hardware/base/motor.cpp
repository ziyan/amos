#include "motor.h"

#include "pid.h"
#include "victor.h"
#include "encoder.h"
#include "timer.h"
#include "power.h"

#define MOTOR_TICKS_PER_METER (15810.0f)
#define MOTOR_METERS_PER_TICK (1.0f/MOTOR_TICKS_PER_METER)
#define MOTOR_TTL (1.0f)
#define MOTOR_POWER_THRESHOLD (6.0f)
#define MOTOR_SPEED_MAX (1.6f)

Motor::Motor() :
    pid((pid_t){0}),
    goal(0.0f),
    acceleration(0.0f),
    target(0.0f),
    output(0.0f),
    odometry(0.0f),
    odometry_t(0.0f),
    voltage(0.0f),
    current(0.0f),
    ttl(0.0f),
    powered(false)
{
    reset();
}

Motor::~Motor()
{
}

void Motor::reset()
{
    pid = (pid_t){0};
    goal = 0.0f;
    acceleration = 0.0f;
    target = 0.0f;
    output = 0.0f;
    odometry = 0.0f;
    odometry_t = 0.0f;
    voltage = 0.0f;
    current = 0.0f;
    ttl = 0.0f;
    powered = false;
}

void Motor::setSpeed(float speed)
{
    goal = speed;
    ttl = MOTOR_TTL;
}

void Motor::setPID(float p, float i, float d)
{
    this->pid = (pid_t){p, i, d, 0.0f, 0.0f};
}

void Motor::setAcceleration(float a)
{
    acceleration = a;
}

void Motor::getOdometry(float *odometry, float *t)
{
    *odometry = this->odometry;
    *t = this->odometry_t;
    this->odometry = this->odometry_t = 0.0f;
}

float Motor::getVoltage() const
{
    return voltage;
}

float Motor::getCurrent() const
{
    return current;
}

boolean Motor::isPowered() const
{
    return powered;
}

void Motor::setup()
{
    encoder_setup();
    victor_setup();
    timer_setup();
}

void Motor::run()
{
    cli();
    if (victor_timer_overflow)
    {
        victor_timer_overflow = false;
        sei();
        update();
    }
    else
    {
        sei();
    }
}

void Motor::update()
{
    cli();
    // read and save current encoder ticks
    float speed_left = encoder_left;
    float speed_right = encoder_right;
    encoder_left = encoder_right = 0;
    // get elapsed time
    uint32_t elapsed_time = timer_elapsed_time;
    timer_elapsed_time = 0;
    sei();

    // convert into meters
    speed_left *= MOTOR_METERS_PER_TICK;
    speed_right *= MOTOR_METERS_PER_TICK;
    
    // convert into seconds
    float dt = (float)elapsed_time / 1000.0f;

    // record odometry
    left.odometry += speed_left;
    left.odometry_t += dt;
    right.odometry += speed_right;
    right.odometry_t += dt;

    if (dt <= 0.0f) return;
    
    // trapezoidal speed control
    if (left.target != left.goal)
    {
        // default to no acceleration limit
        if (left.acceleration <= 0.0f)
        {
            left.target = left.goal;
        }
        else if (left.target < left.goal)
        {
            if (left.goal - left.target > left.acceleration * dt)
                left.target += left.acceleration * dt;
            else
                left.target = left.goal;
        }
        else
        {
            if (left.target - left.goal > left.acceleration * dt)
                left.target -= left.acceleration * dt;
            else
                left.target = left.goal;
        }
    }
    
    if (right.target != right.goal)
    {
        // default to no acceleration limit
        if (right.acceleration <= 0.0f)
        {
            right.target = right.goal;
        }
        else if (right.target < right.goal)
        {
            if (right.goal - right.target > right.acceleration * dt)
                right.target += right.acceleration * dt;
            else
                right.target = right.goal;
        }
        else
        {
            if (right.target - right.goal > right.acceleration * dt)
                right.target -= right.acceleration * dt;
            else
                right.target = right.goal;
        }
    }

    if (left.powered)
    {
        if (left.pid.p == 0.0f)
        {
            // open loop
            left.output = left.target;
        }
        else
        {
            // calculate speed
            speed_left /= dt;
             // pid
            left.output += pid_compute(&left.pid, left.target - speed_left, dt);
        }
    }
    else
    {
        // when power to the motor is lost, all is reset
        left.pid.sum = 0.0f;
        left.pid.e = 0.0f;
        left.output = 0.0f;
        left.target = 0.0f;
    }

    if (right.powered)
    {
        if (right.pid.p == 0.0f)
        {
            // open loop
            right.output = right.target;
        }
        else
        {
            // calculate speed
            speed_right /= dt;
            // pid
            right.output += pid_compute(&right.pid, right.target - speed_right, dt);
        }
    }
    else
    {
        // when power to the motor is lost, all is reset
        right.pid.sum = 0.0f;
        right.pid.e = 0.0f;
        right.output = 0.0f;
        right.target = 0.0f;
    }
    
    // victor cap
    if (left.output > MOTOR_SPEED_MAX)
        left.output = MOTOR_SPEED_MAX;
    else if (left.output < -MOTOR_SPEED_MAX)
        left.output = -MOTOR_SPEED_MAX;
    if (right.output > MOTOR_SPEED_MAX)
        right.output = MOTOR_SPEED_MAX;
    else if (right.output < -MOTOR_SPEED_MAX)
        right.output = -MOTOR_SPEED_MAX;

    // output to victor
    victor_set_speed(left.output / MOTOR_SPEED_MAX * VICTOR_MAX, right.output / MOTOR_SPEED_MAX * VICTOR_MAX);

    // read power
    left.voltage = power_get_voltage_left();
    left.current = power_get_current_left();
    right.voltage = power_get_voltage_right();
    right.current = power_get_current_right();

    left.powered = left.voltage > MOTOR_POWER_THRESHOLD;
    right.powered = right.voltage > MOTOR_POWER_THRESHOLD;
    
    // motor ttl, stop motor when computer is not talking
    if (left.ttl > dt)
        left.ttl -= dt;
    else
    {
        left.ttl = 0.0f;
        left.goal = 0.0f;
    }
    if (right.ttl > dt)
        right.ttl -= dt;
    else
    {
        right.ttl = 0.0f;
        right.goal = 0.0f;
    }
}

Motor Motor::left;
Motor Motor::right;

