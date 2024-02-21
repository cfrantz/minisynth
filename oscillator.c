#include "oscillator.h"
#include "audio.h"
#include "functions.h"

// clang-format off
int16_t frequency[128] = {
//  C           D           E      F             G            A           B
    0,    0,    0,    0,    0,     0,     0,     0,     0,    0,    0,    0,
    0,    0,    0,    0,    0,     0,     0,     0,     0,    27,   29,   30,
    32,   34,   36,   38,   41,    43,    46,    48,    51,   55,   58,   61,
    65,   69,   73,   77,   82,    87,    92,    97,    103,  110,  116,  123,
    130,  138,  146,  155,  164,   174,   184,   195,   207,  220,  233,  246,
    261,  277,  293,  311,  329,   349,   369,   391,   415,  440,  466,  493,
    523,  554,  587,  622,  659,   698,   739,   783,   830,  880,  932,  987,
    1046, 1108, 1174, 1244, 1318,  1396,  1479,  1567,  1661, 1760, 1864, 1975,
    2093, 2217, 2349, 2489, 2637,  2793,  2959,  3135,  3322, 3520, 3729, 3951,
    4186, 4434, 4698, 4978, 5274,  5587,  5919,  6271,  6644, 7040, 7458, 7902,
    8372, 8869, 9397, 9956, 10548, 11175, 11839, 12543,
};
// clang-format on

int32_t osc_value(oscillator_t *osc, int32_t bend, uint64_t tstep) {
    if (osc->state == OscOff) return 0;

    uint32_t f = frequency[osc->note];
    if (bend) f = (f * bend) >> 15;
    uint32_t theta = (osc->phase / AUDIO_SAMPLE_RATE) % 1024;
    uint64_t delta = f * 1024;
    osc->phase += delta;
    int32_t value = 0;
    switch(osc->function) {
        case OscSine:
            value = sine(theta); break;
        case OscTriangle:
            value = triangle(theta); break;
        case OscSaw:
            value = saw(theta); break;
        case OscSquare:
            value = square(theta); break;
        default:
            /* bad osc function */
            ;
    }
    return value;
}
