#include <avr/io.h>
#include "synth.h"
#include <SoftwareSerial.h>
#define nop()  __asm__ __volatile__("nop")

//                        attiny45
//      reset -+---+-  power
//      pb3:3 -+   +- 2:pb2
//      pb4:4 -+   +- 1:pb1
//            -+---+- 0:pb0 (OC0A)

#define NUM_CHANNELS 1
#define NUM_SENSE 1


byte touchPins[] = {PB4};

SoftwareSerial serial(PB2, PB1);

typedef struct {
    uint8_t pin;
    uint16_t calibration;
} sense_t; 

sense_t sensors[NUM_SENSE];

void serial_init() {

    DDRB |= (1 << PB2) | (1 << PB1);
    serial.begin(4800);

}

void setup(void) {
      audio_init();
      synth_init();
      serial_init();

      // led
      DDRB |= (1 << PB3);

      // pins
      for(byte i=0; i < sizeof(touchPins); i++) {
          DDRB |= 1 << touchPins[i];
      }

      for(byte i=0; i < NUM_SENSE; i++) {
            sensors[i].pin = touchPins[i];
            sensors[i].calibration = sampleChargeTime(sensors[i].pin, 4);        
      }

      PORTB |= (1 << PB3);
      delay(60000);
      PORTB &= ~(1 << PB3);
      delay(60000);
      PORTB |= (1 << PB3);
      delay(60000);
      PORTB &= ~(1 << PB3);
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

byte notes[] = {10, 20, 30};
void loop(void) {
    uint16_t n;
    for(int i=0; i<NUM_SENSE; i++) {
        n = sampleChargeTime(sensors[i].pin, 4);
        if(n > sensors[i].calibration) {
            PORTB |= 1 << PB3;
        } else {
            PORTB &= ~(1 << PB3);
        }
    }
    delay(100);
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
        serial.println(val);
        delay(1000);
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


