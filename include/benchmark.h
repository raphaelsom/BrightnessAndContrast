#pragma once

#include <stddef.h>
#include <stdint.h>

#include "brightness_contrast.h"

extern const uint8_t benchmark_iterations_per_implementation;
extern const char* benchmark_csv_out_file;

void benchmark_implementation(const BCInput *input, const size_t width, const size_t height,
                              const uint8_t *source_image, uint8_t *result_image, const char* prog_name);

int benchmark_implementations_write_csv(BCInput *input, const size_t width, const size_t height,
                                        const uint8_t *source_image, uint8_t *result_image, const char* prog_name);

void benchmark_sqrt(const char* prog_name, const size_t runs);
