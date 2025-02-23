#pragma once

#include "brightness_contrast.h"

int parse_input(const int argc, char **argv, BCInput *input);
int parse_uint8(const char* str, uint8_t* param);
int parse_uint16(const char* str, uint16_t* param);
int parse_int16(const char* str, int16_t* param);
int parse_uint32(const char* str, uint32_t* param);
int parse_float(const char* str, float* param);
int parse_size_t(const char* str, size_t* param);
