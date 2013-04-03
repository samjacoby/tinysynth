
#ifndef __sinetable_h_
#define __sinetable_h_

#include <avr/pgmspace.h>
#include <stdint.h>

#define SINETABLE_SIZE 1024
#define SINETABLE_BITS 10
#define SINETABLE_RES 8
#define SINETABLE_MASK 1023

extern const PROGMEM uint8_t sinetable[1024];

#endif // __sinetable_h_

