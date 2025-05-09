#pragma once

#include <stdlib.h>

#include "brightness_contrast.h"

void bc_test_implementations(const BCInput* input, const size_t width, const size_t height,
                             const uint8_t* source_img, uint8_t* result_img, const char* prog_name);