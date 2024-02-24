#ifndef MINISYNTH_FUNCTIONS_H
#define MINISYNTH_FUNCTIONS_H
#include <stdint.h>

int32_t sine(uint32_t theta);
int32_t saw(uint32_t theta);
int32_t square(uint32_t theta);
int32_t triangle(uint32_t theta);

#endif  // MINISYNTH_FUNCTIONS_H
