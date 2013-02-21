#include <avr/io.h>
#include "synth.h"
#define nop()  __asm__ __volatile__("nop")

//                        attiny45
//      reset -+---+-  power
//      pb3:3 -+   +- 2:pb2
//      pb4:4 -+   +- 1:pb1
//            -+---+- 0:pb0 (OC0A)
//#define NUM_CHANNELS 3
//#define SERIALON

#define NUM_SENSE 3
#define SAMPLES 16
#define TIMEOUT 10000

byte touchPins[] = { PB1, PB2, PB4};

#ifdef SERIALON
#include <SoftwareSerial.h>
SoftwareSerial Serial(PB2, PB1);
#endif

typedef struct {
    uint8_t pin;
    uint8_t active;
    uint8_t shift;
    uint16_t calibration;
} sense_t; 

sense_t sensors[NUM_SENSE];

#ifdef SERIALON
void serial_init() {

    DDRB |= (1 << PB2) | (1 << PB1);
    Serial.begin(4800);

}
#endif

void setup(void) {
    audio_init();
    synth_init();
    #ifdef SERIALON
    serial_init();
    #endif

    // led
    DDRB |= (1 << PB3);

    for(byte i=0; i < NUM_SENSE; i++) {
            DDRB |= 1 << touchPins[i];
            sensors[i].pin = touchPins[i];
            sensors[i].active = 0; 
            sensors[i].shift = i << 3; // multiply by eight 
            sensors[i].calibration = sampleChargeTime(sensors[i].pin, SAMPLES) + 1;        
    }

    PORTB |= (1 << PB3);
    delay(60000);
    PORTB &= ~(1 << PB3);
    delay(60000);
    PORTB |= (1 << PB3);

    synth_amplitude(0xff);
}

/* not for now
typedef struct {
      uint8_t carrier_inc;
      uint8_t carrier_pos;
      uint8_t on; 
      uint8_t off; 
} channel_t;

channel_t channels[NUM_CHANNELS];
*/

byte notes[] = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33};

uint8_t notes_i = 0;
uint8_t notes_mask = 7;

void loop(void) {
    uint16_t n;
    for(int i=0; i<NUM_SENSE; i++) {
        n = sampleChargeTime(sensors[i].pin, SAMPLES);
        #ifdef SERIALON
        Serial.println(n);
        #endif
        if(n > (sensors[i].calibration)) {
            PORTB |= 1 << PB3;
            sensors[i].active = 1;
            synth_start_note(notes[notes_i + sensors[i].shift]);
        } else if(sensors[i].active) {
            PORTB &= ~(1 << PB3);
            sensors[i].active = 0;
            synth_stop_note();
            notes_i = (notes_i + 1) & notes_mask;
        }
    }
}


void audio_init(void) {
      TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00); 
      TCCR0B = (1 << CS00);
      OCR0A = 127;
      DDRB |= (1 << PB0); 
}

uint16_t sampleChargeTime(byte pin, uint8_t samples) {
    byte val, prev_val = 0;
    uint16_t sum = 0;
    for(int i=0; i<samples; i++) {
        val = chargeTime(pin);
        sum += ((prev_val * 7) + val) >> 3;
        prev_val = val;
    }
    return sum;
}


byte chargeTime(byte pin) {

    byte mask, i;
    mask = 1 << pin;

    DDRB &= ~mask; // input
    PORTB |= mask; // pull-up on

    for(i=0; i < 16; i++) {
        if(PINB & mask) break;
    }

    PORTB &= ~mask; // pull-up off
    DDRB |= mask; // discharge

    return i;
}
