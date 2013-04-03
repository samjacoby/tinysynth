#include <avr/io.h>
#include "synth.h"
#define nop()  __asm__ __volatile__("nop")

//                        attiny45
//      reset -+---+-  power
//      pb3:3 -+   +- 2:pb2
//      pb4:4 -+   +- 1:pb1
//            -+---+- 0:pb0 (OC0A)

//#define SERIALON

#define NUM_SENSE 2
#define SAMPLES 16
#define TIMEOUT 10000 // this is pretty much just a random big number

byte touchPins[] = { PB2, PB1};     // the pins that we touch
byte ledPins[] = { PB3, PB4};       // the pins that light up

#ifdef SERIALON
#include <SoftwareSerial.h>
SoftwareSerial Serial(PB2, PB1);
#endif

typedef struct {
    uint8_t pin;
    uint8_t led;
    uint8_t active;
    uint8_t shift;
    uint16_t calibration;
    uint16_t trigger;
    uint16_t timeout;
} sense_t; 

sense_t sensors[NUM_SENSE];

#ifdef SERIALON
void serial_init() {

    DDRB |= (1 << PB2) | (1 << PB1);
    Serial.begin(4800);

}
#endif


void sensors_calibrate(void) {

    for(byte i=0; i < NUM_SENSE; i++) {
        sensors[i].calibration = sampleChargeTime(sensors[i].pin, SAMPLES) + 2;        
    }

}

void setup(void) {
    audio_init();
    audio_disable();
    synth_init();
    #ifdef SERIALON
    serial_init();
    #endif

    // led
    DDRB |= (1 << ledPins[0]) | (1 << ledPins[1]);
    PORTB |= (1 << ledPins[0]) | (1 << ledPins[1]);

    // initialize sensors
    for(byte i=0; i < NUM_SENSE; i++) {
            DDRB |= 1 << touchPins[i];
            sensors[i].pin = touchPins[i];
            sensors[i].led = ledPins[i];
            sensors[i].active = 0; 
            sensors[i].shift = i << 3; // multiply by eight 
            sensors[i].calibration = sampleChargeTime(sensors[i].pin, SAMPLES) + 1;        
            sensors[i].trigger = 0; 
            sensors[i].timeout = 0; 
    }

    PORTB &=  ~((1 << ledPins[0]) | (1 << ledPins[1]));

    synth_amplitude(0xff);
}

/* not for now
//#define NUM_CHANNELS 3
typedef struct {
      uint8_t carrier_inc;
      uint8_t carrier_pos;
      uint8_t on; 
      uint8_t off; 
} channel_t;

channel_t channels[NUM_CHANNELS];
*/


byte notes[] = { 34, 38, 42, 46, 50, 54, 58, 62, 66, 70, 74, 78, 82, 86, 90, 94, 98, 102, 106, 110, 114, 118, 122, 126, 130 };

uint8_t notes_i = 0;
uint8_t notes_mask = 7;

void loop(void) {
    uint16_t n;

    // iterate over each sensor pin
    for(int i=0; i<NUM_SENSE; i++) {
        // check the sample time
        n = sampleChargeTime(sensors[i].pin, SAMPLES);
        #ifdef SERIALON
        Serial.println(n);
        #endif
        // if the sample time is larger than the calibration time
        if(n > sensors[i].calibration) { 
            // reset the trigger if we're actuated
            sensors[i].trigger = 0; 
            // keep track of how long we've been on
            sensors[i].timeout += 1; 

            // autocalibrate if we've been on for awhile
            if(sensors[i].timeout > TIMEOUT) {
                sensors[i].calibration = n;
                sensors[i].timeout = 0; // next iteration should do this, but better safe 
            }

            // if this the first time that the sensor is being turned on
            if(!sensors[i].active) {
                // set it on
                sensors[i].active = 1;
                // turn on the LED
                PORTB |= 1 << sensors[i].led;
                // enable audio
                audio_enable();
                // and generate note
                synth_start_note(notes[notes_i + sensors[i].shift]);
            }
        // if the sample time is smaller than the calibration time AND
        // this particular pin is active
        } else if(sensors[i].active) {
            // increment trigger
            sensors[i].trigger++;
            // if pin has been off for a certain amount of time, turn it off
            if(sensors[i].trigger > 10) {
                // set pin inactive
                sensors[i].active = 0;
                // set the trigger to 0
                sensors[i].trigger = 0;
                // set the timeout to 0
                sensors[i].timeout = 0;
                // switch off LED
                PORTB &= ~(1 << sensors[i].led);
                // close down sound
                audio_disable();
                synth_stop_note();
                notes_i = (notes_i + 1) & notes_mask;
            }
        }
    }
}


void audio_init(void) {
      TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00); 
      OCR0A = 127;
}

void audio_enable(void) {
    TCCR0B = (1 << CS00);
    DDRB |= (1 << PB0); 
}

void audio_disable(void) {
    TCCR0B = ~(1 << CS00);
    DDRB &= ~(1 << PB0); 
}

uint16_t sampleChargeTime(byte pin, uint8_t samples) {
    byte val, prev_val = 0;
    uint16_t sum = 0;
    for(int i=0; i<samples; i++) {
        val = chargeTime(pin);
        // moving average
        sum += ((prev_val * 7) + val) >> 3;
        prev_val = val;
    }
    return sum;
}

//
// Charge Pin
//
byte chargeTime(byte pin) {

    byte pinMask, i;
    pinMask = 1 << pin;

    DDRB &= ~pinMask; // input
    PORTB |= pinMask; // pull-up on

    // wait for pin to go high
    for(i=0; i < 16; i++) {
        if(PINB & pinMask) break;
    }

    PORTB &= ~pinMask; // pull-up off
    DDRB |= pinMask; // discharge (set to output);

    return i;
}
