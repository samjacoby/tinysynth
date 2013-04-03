#include "sinetable.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// Variable to hold the next note that will be generated 
static volatile uint8_t next_note;

// Variable to hold the next sample that will be output 
static volatile uint8_t next_sample;

// Variable to hold the amplitude of the next sample 
static volatile uint8_t next_amplitude;

void synth_amplitude(uint8_t amplitude) {
    next_amplitude = amplitude;
}

void synth_enable(void) {
    TCCR1 = (1 << CS11) | (1 << CS00);
    TIMSK = (1 << OCIE1A); 
}

void synth_disable() {
    TCCR1 &= ~((1 << CS11) | (1 << CS00));
    TIMSK &= ~(1 << OCIE1A);
}

void synth_init(void) {
    OCR1A = 0x0f; // set PWM carrier frequency
    next_amplitude = 0xff; // max volume, plz
}

void synth_start_note(uint8_t note) {
    synth_enable();
    next_note = note;
}


static volatile uint16_t carrier_inc;
static volatile uint16_t carrier_pos = 0;
static volatile uint8_t amplitude = 0xff;

// hack to prevent increment
void synth_stop_note(void) {
    synth_disable();
    next_note = 0;
    carrier_pos = 0;
}

void synth_generate(uint8_t note) {

    uint16_t cpos = 0;
    carrier_inc = note;
    carrier_pos += carrier_inc;

    cpos = carrier_pos & SINETABLE_MASK; 
    next_sample = (pgm_read_byte(&sinetable[cpos]) * next_amplitude) >> 8;

}

ISR(TIM1_COMPA_vect) {
    //if(!synth_ready) return
    synth_generate(next_note); 
    OCR0A = next_sample;
}
