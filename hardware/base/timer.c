#include "timer.h"

volatile uint32_t timer_elapsed_time = 0;

void timer_setup()
{
    TCCR2A = (0<<WGM21) | (0<<WGM20);
    TCCR2B = (1<<CS22) | (0<<CS21) | (0<<CS20) | (0<<WGM22);
    TIMSK2 = (1<<TOIE2);
    ASSR = 0;
    TCNT2 = 6;
}

ISR(TIMER2_OVF_vect)
{
    TCNT2 = 6;
    ++timer_elapsed_time;
}

