#ifndef MINISYNTH_OSCILLATOR_H
#define MINISYNTH_OSCILLATOR_H
#include <stdint.h>

extern int16_t frequency[128];

typedef enum OscState {
    OscOff,
    OscOn,
    OscAttack,
    OscDecay,
    OscSustain,
    OscRelease,
} osc_state_t;

typedef enum OscFunction {
    OscSine,
    OscTriangle,
    OscSaw,
    OscSquare,
} osc_function_t;

typedef struct Envelope {
    int16_t attack;
    int16_t decay;
    int16_t sustain;
    int16_t release;
} envelope_t;

typedef struct Oscillator {
    osc_state_t state;
    osc_function_t function;
    uint64_t phase;
    uint8_t note;
    uint8_t channel;
    int16_t attack;
    int16_t decay;
    int16_t release;
} oscillator_t;

int32_t osc_value(oscillator_t* osc, int32_t bend, uint64_t tstep);

#endif  // MINISYNTH_OSCILLATOR_H
