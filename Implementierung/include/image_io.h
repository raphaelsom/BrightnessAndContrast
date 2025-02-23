#pragma once
#include <stddef.h>
#include <stdint.h>

int read_source_image(const char* input_file_name, uint8_t** source_image, size_t* width, size_t* height,
                      const char* program_name);
int write_to_res_img(const char* output_file_name, const uint8_t* res_image, const size_t width, const size_t height,
                     const char* program_name);
int alloc_image_pointer(uint8_t** image, size_t width, size_t height, unsigned int bytes_per_pixel);
