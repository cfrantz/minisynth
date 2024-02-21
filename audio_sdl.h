#ifndef MINISYNTH_AUDIO_SDL_H
#define MINISYNTH_AUDIO_SDL_H

#include <stdint.h>
#include <SDL2/SDL.h>

#define AUDIO_BUFFER_SZ 2048
#define AUDIO_SAMPLE_RATE 32768

typedef struct AudioState {
    SDL_mutex *mutex;
    SDL_cond *cond;
    uint32_t producer;
    uint32_t consumer;
    int16_t buf[AUDIO_BUFFER_SZ];
} audio_state_t;

void audio_send(audio_state_t *state, int16_t *data, uint32_t length);

void audio_init(audio_state_t *state);

#endif // MINISYNTH_AUDIO_SDL_H
