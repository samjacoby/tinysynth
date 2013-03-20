#ifndef __synth_h_
#define __synth_h_

void synth_init(void);
void synth_clear(void);

void synth_enable(void);
void synth_disable(void);

void synth_start_note(uint8_t note); 
void synth_stop_note(void); 
void synth_generate(uint8_t note);
void synth_amplitude(uint8_t amplitude);

#endif // __synth_h_
