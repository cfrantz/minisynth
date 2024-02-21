#include "audio_sdl.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <SDL2/SDL.h>

static void audio_callback(void* userdata, uint8_t* stream, int len) {
    audio_state_t *state = (audio_state_t *)userdata;
    int16_t *dest = (int16_t*)stream;
    len /= sizeof(int16_t);
    int i;

    //SDL_LockMutex(state->mutex);
    for(i=0; i<len && state->consumer < state->producer; ++i) {
        *dest++ = state->buf[state->consumer];
        state->buf[state->consumer++] = 0;
    }
    if (i < len) {
        fprintf(stderr, "Audio underrun: %d<%d\n", i, len);
        while(i < len) {
            *dest++ = 0; i++;
        }
    }
    //printf("c=%d p=%d\n", state->consumer, state->producer);
    if (state->producer == AUDIO_BUFFER_SZ) state->producer = 0;
    if (state->consumer == AUDIO_BUFFER_SZ) state->consumer = 0;
    //SDL_CondSignal(state->cond);
    //SDL_UnlockMutex(state->mutex);
}

void audio_send(audio_state_t *state, int16_t *data, uint32_t length) {
    while(length) {
        //SDL_LockMutex(state->mutex);
        while(state->producer == AUDIO_BUFFER_SZ) {
            //SDL_CondWait(state->cond, state->mutex);
        }
        //printf("p=%d c=%d\n", state->producer, state->consumer);
        while(length && state->producer < AUDIO_BUFFER_SZ) {
            state->buf[state->producer++] = *data++;
            length--;
        }
        //SDL_UnlockMutex(state->mutex);
    }
}

void audio_init(audio_state_t *state) {
    SDL_Init(SDL_INIT_AUDIO);

    SDL_AudioSpec want, have;
    SDL_memset(&want, 0, sizeof(want));
    want.freq = AUDIO_SAMPLE_RATE;
    want.channels = 1;
    //want.samples = AUDIO_BUFFER_SZ / 2;
    want.samples = AUDIO_BUFFER_SZ;
    want.format = AUDIO_S16;
    want.callback = audio_callback;
    want.userdata = state;

    state->mutex = SDL_CreateMutex();
    state->cond = SDL_CreateCond();
    state->producer = 0;
    state->consumer = 0;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    SDL_PauseAudioDevice(dev, 0);
}
