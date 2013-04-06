#include <avr/io.h>
#include "synth.h"
#include "tinysynth.h"
#define nop()  __asm__ __volatile__("nop")

//                        attiny45
//      reset -+---+-  power
// LED1 pb3:3 -+   +- 2:pb2 TOUCH1
// LED2 pb4:4 -+   +- 1:pb1 TOUCH2
//            -+---+- 0:pb0 (OC0A)

#define NUM_SENSE 2   // how many sensor pins do we have?
#define SAMPLES 16    // how many times do we try and detect a touch?
#define TIMEOUT 10000 // this is pretty much just a random big number

byte touchPins[] = { PB2, PB1};     // the pins that we touch
byte ledPins[] = { PB3, PB4};       // the pins that light up

#ifdef SERIALON
#include <SoftwareSerial.h>
SoftwareSerial Serial(PB2, PB1);

void serial_init() {
    DDRB |= (1 << PB2) | (1 << PB1);
    Serial.begin(4800);

}
#endif

sense_t sensors[NUM_SENSE];

void sensors_calibrate(void) {

    for(byte i=0; i < NUM_SENSE; i++) {
        sensors[i].calibration = sampleChargeTime(sensors[i].pin, SAMPLES) + 2;        
    }

}

void sensor_init() {

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



}

void setup(void) {
    audio_init();
    audio_disable();
    synth_init();
    #ifdef SERIALON
    serial_init();
    #endif

    // enable LEDs
    DDRB |= (1 << ledPins[0]) | (1 << ledPins[1]);
    // light em' up
    PORTB |= (1 << ledPins[0]) | (1 << ledPins[1]);
    // initialize the touch-sensitive pins
    sensor_init();
    // turn LEDs off
    PORTB &=  ~((1 << ledPins[0]) | (1 << ledPins[1]));

    // set volume of synth
    synth_amplitude(0xff);
}

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

        // if we've detected a touch 
        if(n > sensors[i].calibration) { 

            // reset the trigger if we're touched
            // this is a way of kind've filtering
            sensors[i].trigger = 0; 

            // keep track of how long we've been on for new calibration
            sensors[i].timeout += 1; 

            // autocalibrate if we've been touched for awhile
            // because something has gone wrong!
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
                
                
                // IMPORTANT.
                // 
                // This is where the sounds actually gets made.
                // Send @synth_start_note a number between 0 and 255  
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


// Check a single pin a bunch of times
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
// This function times how long it takes for a pin to switch from low to high.
// That time is correlated with the pin's capacitance, making it a capacitive
// sensor. 
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
