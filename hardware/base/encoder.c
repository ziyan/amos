#include "encoder.h"

#define ENCODER_LEFT_A 3
#define ENCODER_LEFT_B 2
#define ENCODER_RIGHT_A 4
#define ENCODER_RIGHT_B 5

volatile int32_t encoder_left = 0, encoder_right = 0;

void encoder_setup()
{
    PCICR |= (1 << 2);
    PCMSK2 |= (1 << ENCODER_LEFT_A) | (1 << ENCODER_LEFT_B) | (1 << ENCODER_RIGHT_A) | (1 << ENCODER_RIGHT_B);
}

// Interrupt for all four encoders
ISR(PCINT2_vect)
{
    static struct {
        struct {
            uint8_t a : 1;
            uint8_t b : 1;
        } left, right;
    } previous = {0}, current = {0};

    current.left.a = (PIND >> ENCODER_LEFT_A) & 0x1;
    current.left.b = (PIND >> ENCODER_LEFT_B) & 0x1;
    current.right.a = (PIND >> ENCODER_RIGHT_A) & 0x1;
    current.right.b = (PIND >> ENCODER_RIGHT_B) & 0x1;

    if(current.left.a == 1 && previous.left.a == 0)
    {
        if (current.left.b == 0) 
            encoder_left++;
        else
            encoder_left--;
    }
    else if (current.left.a == 0 && previous.left.a == 1)
    {
        if (current.left.b == 0)
            encoder_left--;
        else
            encoder_left++;
    }

    if(current.left.b == 1 && previous.left.b == 0)
    {
        if (current.left.a == 0) 
            encoder_left--;
        else
            encoder_left++;
    }
    else if (current.left.b == 0 && previous.left.b == 1)
    {
        if (current.left.a == 0)
            encoder_left++;
        else
            encoder_left--;
    }

    if(current.right.a == 1 && previous.right.a == 0)
    {
        if (current.right.b == 0) 
            encoder_right++;
        else
            encoder_right--;
    }
    else if (current.right.a == 0 && previous.right.a == 1)
    {
        if (current.right.b == 0)
            encoder_right--;
        else
            encoder_right++;
    }

    if(current.right.b == 1 && previous.right.b == 0)
    {
        if (current.right.a == 0) 
            encoder_right--;
        else
            encoder_right++;
    }
    else if (current.right.b == 0 && previous.right.b == 1)
    {
        if (current.right.a == 0)
            encoder_right++;
        else
            encoder_right--;
    }

    previous = current;
}

