#include "sinetable.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// Variable to hold the next note that will be generated 
static volatile uint8_t next_note;

// Variable to hold the next sample that will be output 
static volatile uint8_t next_sample;

void synth_init(void) {
    TCCR1 = (1 << CS11) | (1 << CS00);
    TIMSK = (1 << OCIE1A); 
    OCR1A = 0x0f;
}

void synth_start_note(uint8_t note) {
    next_note = note;
}

static volatile uint8_t carrier_inc;
static volatile uint8_t carrier_pos = 0;
static volatile uint8_t amplitude = 0xff;

void synth_generate(uint8_t note) {

    uint8_t cpos = 0;
    carrier_inc = note;
    carrier_pos += carrier_inc;

    cpos = carrier_pos & SINETABLE_MASK; 
    next_sample = (pgm_read_byte(&sinetable[cpos])); 

}

ISR(TIM1_COMPA_vect) {
    PORTB ^= (1 << PB3);
    synth_generate(next_note); 
    OCR0A = next_sample;
}
