#include <avr/io.h>
#include "synth.h"

//                        attiny45
//      reset -+---+-  power
//      pb3:3 -+   +- 2:pb2
//      pb4:4 -+   +- 1:pb1
//            -+---+- 0:pb0 (OC0A)

void setup(void) {
    audio_init();
    synth_init();

    // led
    DDRB |= (1 << PB3);


}
void loop(void) {
    //byte time = chargeTime(2);
    for(int i=1; i < 0x0f; i++) {
    synth_start_note(i);
    delay(60000);
    }
}


void audio_init(void) {
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00); 
    TCCR0B = (1 << CS00);
    OCR0A = 127;
    DDRB |= (1 << PB0); 
}

byte chargeTime(byte pin) {

  byte mask, i;
  mask = digitalPinToBitMask(pin);

  DDRB &= ~mask; // input
  PORTB |= mask; // pull-up on

  for (i = 0; i < 16; i++) {
      if (PINB & mask) break;
    }

  PORTB &= ~mask; // pull-up off
  DDRB |= mask; // discharge

  return i;
}


