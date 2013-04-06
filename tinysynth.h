#ifndef __tinysynth_h__
#define __tinysynth_h__
typedef struct {
    uint8_t pin;
    uint8_t led;
    uint8_t active;
    uint8_t shift;
    uint16_t calibration;
    uint16_t trigger;
    uint16_t timeout;
} sense_t; 
#endif

