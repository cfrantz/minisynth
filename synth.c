#include "synth.h"

#include <stdint.h>
#include <stdio.h>

#include "oscillator.h"

const int32_t pitch_bend_mult[] = {
    26007, 26102, 26196, 26291, 26386, 26481, 26577, 26673, 26770, 26866, 26964,
    27061, 27159, 27257, 27356, 27455, 27554, 27654, 27754, 27854, 27955, 28056,
    28157, 28259, 28361, 28464, 28567, 28670, 28774, 28878, 28982, 29087, 29192,
    29298, 29404, 29510, 29617, 29724, 29832, 29940, 30048, 30157, 30266, 30375,
    30485, 30595, 30706, 30817, 30928, 31040, 31152, 31265, 31378, 31492, 31606,
    31720, 31835, 31950, 32065, 32181, 32298, 32415, 32532, 32649, 32768, 32886,
    33005, 33124, 33244, 33364, 33485, 33606, 33728, 33850, 33972, 34095, 34218,
    34342, 34466, 34591, 34716, 34842, 34968, 35094, 35221, 35348, 35476, 35604,
    35733, 35862, 35992, 36122, 36253, 36384, 36516, 36648, 36780, 36913, 37047,
    37181, 37315, 37450, 37586, 37722, 37858, 37995, 38132, 38270, 38409, 38548,
    38687, 38827, 38967, 39108, 39250, 39392, 39534, 39677, 39821, 39965, 40109,
    40254, 40400, 40546, 40693, 40840, 40988, 41136,
};

oscillator_t oscillator_bank[SYNTH_POLYPHONY];
envelope_t multiplier[16];
envelope_t envelope[16];
uint16_t pitch_bend[16];
osc_function_t function[16];

envelope_t envelope_preset[128];
osc_function_t function_preset[128];

void synth_init(void) {
  for (uint32_t i = 0; i < 16; ++i) {
    multiplier[i].attack = 32;
    multiplier[i].decay = 32;
    multiplier[i].release = 32;
    pitch_bend[i] = 64;
  }
}

void synth_set_program(uint8_t channel, uint8_t program) {
  envelope[channel] = envelope_preset[program];
  function[channel] = function_preset[program];
}

void synth_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
  for (uint32_t i = 0; i < SYNTH_POLYPHONY; ++i) {
    oscillator_t *osc = &oscillator_bank[i];
    if (osc->state == OscOff) {
      osc->state = OscOn;
      osc->function = function[channel];
      osc->note = note;
      osc->channel = channel;
      osc->attack = 0;
      osc->decay = 0;
      osc->release = 0;
      return;
    }
  }
}

void synth_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
  for (uint32_t i = 0; i < SYNTH_POLYPHONY; ++i) {
    oscillator_t *osc = &oscillator_bank[i];
    if (osc->state != OscOff && osc->note == note && osc->channel == channel) {
      osc->state = OscRelease;
      return;
    }
  }
}

void synth_controller(uint8_t ch, uint8_t ctrl, uint8_t val) {
  switch (ctrl) {
  case 0x0c:
    envelope[ch].attack = multiplier[ch].attack * val;
    printf("Channel %d: attack = %d (%d)\n", ch, val, envelope[ch].attack);
    break;
  case 0x0d:
    envelope[ch].decay = multiplier[ch].decay * val;
    printf("Channel %d: decay = %d (%d)\n", ch, val, envelope[ch].decay);
    break;
  case 0x0e:
    envelope[ch].sustain = val << 8;
    envelope[ch].sustain |= val & 1 ? 0xFF : 0;
    printf("Channel %d: sustain = %d (%d)\n", ch, val, envelope[ch].sustain);
    break;
  case 0x0f:
    envelope[ch].release = multiplier[ch].release * val;
    printf("Channel %d: release = %d (%d)\n", ch, val, envelope[ch].release);
    break;
  case 0x16:
    multiplier[ch].attack = val;
    printf("Channel %d: attack mult = %d\n", ch, val);
    break;
  case 0x17:
    multiplier[ch].decay = val;
    printf("Channel %d: decay mult = %d\n", ch, val);
    break;
  case 0x18:
    val = val / 32;
    function[ch] = val;
    printf("Channel %d: function = %s\n", ch,
           (val == 0)   ? "sine"
           : (val == 1) ? "triangle"
           : (val == 2) ? "saw"
           : (val == 3) ? "square"
                        : "unknown");
    break;
  case 0x19:
    multiplier[ch].release = val;
    printf("Channel %d: release mult = %d\n", ch, val);
    break;
  default:
    printf("unhandled controller: %02x %02x %02x\n", ch, ctrl, val);
  }
}

void synth_midi(uint8_t *message) {
  uint8_t cmd = message[0] & 0xF0;
  uint8_t channel = message[0] & 0x0F;
  switch (cmd) {
  case 0x80:
    synth_note_off(channel, message[1], message[2]);
    break;
  case 0x90:
    if (message[2]) {
      synth_note_on(channel, message[1], message[2]);
    } else {
      synth_note_off(channel, message[1], message[2]);
    }
    break;
  case 0xb0:
    synth_controller(channel, message[1], message[2]);
    break;
  case 0xc0:
    synth_set_program(channel, message[1]);
    break;
  case 0xe0:
    pitch_bend[channel] = message[2];
    break;
  default:
    printf("unhandled midi: %02x %02x %02x\n", message[0], message[1],
           message[2]);
  }
}

int32_t synth_value(uint64_t tstep) {
  int32_t value = 0;
  for (uint32_t i = 0; i < SYNTH_POLYPHONY; ++i) {
    oscillator_t *osc = &oscillator_bank[i];
    if (osc->state == OscOff)
      continue;

    envelope_t *env = &envelope[osc->channel];
    // TODO: pitch bend
    int32_t bend = pitch_bend_mult[pitch_bend[osc->channel]];
    ;
    int32_t v = osc_value(osc, bend, tstep);
    // Process the ADSR envelope.
    if (osc->state == OscOn)
      osc->state = OscAttack;
    if (osc->state == OscAttack) {
      if (env->attack && osc->attack < env->attack) {
        osc->attack += 1;
        v = v * osc->attack / env->attack;
      } else {
        osc->state = OscDecay;
      }
    }
    if (osc->state == OscDecay) {
      if (env->decay && osc->decay < env->decay) {
        int32_t factor = (32767 - env->sustain) * osc->decay / env->decay;
        v = (v * (32767 - factor)) >> 15;
        osc->decay += 1;
      } else {
        osc->state = OscSustain;
      }
    }
    if (osc->state == OscSustain) {
      if (env->sustain) {
        v = (v * env->sustain) >> 15;
      }
    }
    if (osc->state == OscRelease) {
      if (env->release && osc->release < env->release) {
        v = (v * env->sustain) >> 15;
        v = v * (env->release - osc->release) / env->release;
        osc->release += 1;
      } else {
        osc->state = OscOff;
        v = 0;
      }
    }
    value += v;
  }
  return value / SYNTH_POLYPHONY;
}
