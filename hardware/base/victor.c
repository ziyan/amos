#include "victor.h"

volatile boolean victor_timer_overflow = false;

void victor_setup()
{
    pinMode(9, OUTPUT);
    pinMode(10, OUTPUT);

    TCCR1A = (1 << COM1A1) | (0 << COM1A0) | (1 << COM1B1) | (0 << COM1B0) | (1 << WGM11) | (0 << WGM10);
    TCCR1B = (1 << WGM13)  | (1 << WGM12) | (0 << CS12) | (1 << CS11) | (0 << CS10);
    TIMSK1 = (1 << TOIE1);
    ICR1 = 40000u;
    TCNT1 = 0;
    OCR1A = 3000u;
    OCR1B = 3000u;
}

// left and right are +/- 1.0
void victor_set_speed(float left, float right)
{
    OCR1A = 3000u + left * 1000;
    OCR1B = 3000u + right * 1000;
}

ISR(TIMER1_OVF_vect)
{
    victor_timer_overflow = true;
}

