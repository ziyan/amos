#include "motor.h"
#include "packet.h"
#include "crc.h"

void setup()
{
    // set up motors
    Motor::setup();
    // set up serial port
    comm_setup();
    // enable interrupt
    sei();
}

void loop()
{
    // run motor tasks
    Motor::run();
    // run the comm state machine in each loop
    comm_run();
}
