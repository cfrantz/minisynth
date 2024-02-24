#ifndef MINISYNTH_SYNTH_H
#define MINISYNTH_SYNTH_H
#include <stdint.h>

#include "oscillator.h"
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define SYNTH_POLYPHONY 16

void synth_init(void);

void synth_set_program(uint8_t channel, uint8_t program);

void synth_note_on(uint8_t channel, uint8_t note, uint8_t velocity);

void synth_note_off(uint8_t channel, uint8_t note, uint8_t velocity);

void synth_midi(uint8_t* message);

int32_t synth_value(uint64_t tstep);

extern envelope_t envelope_preset[128];
extern osc_function_t function_preset[128];

#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // MINISYNTH_SYNTH_H
