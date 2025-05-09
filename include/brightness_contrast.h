#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
  BCImplAsmSIMD,
  BCImplCSIMD,
  BCImplAsmSISD,
  BCImplCSISD,
  BCImplCSISD_MT,
  BCImplCSISD_Heron,
  BCImplCSISD_IEEE,
  BCImplMax
} BCImplVersion;

typedef struct
{
  BCImplVersion impl;

  uint32_t benchmark_runs;
  bool benchmark_csv;

  char* input_file;
  char* output_file;

  float coeffs[3];

  int16_t brightness;
  float contrast;

  bool run_tests;
  uint8_t test_delta;

  bool run_sqrt_tests_benchmark;

  bool help_printed;
} BCInput;

typedef struct
{
  void (*impl)(const uint8_t *img, size_t width, size_t height, float a, float b, float c,
               int16_t brightness, float contrast, uint8_t *result);
  const char* name;
} BCImplementation;

extern const BCImplementation bc_implementation[];

extern const uint16_t bc_default_benchmark_runs;
extern const uint8_t bc_default_test_delta;

void bc_init_input(BCInput* input);
void bc_destroy_input(BCInput* input);

void brightness_contrast(const uint8_t *img, size_t width, size_t height, float a, float b, float c, int16_t brightness,
                         float contrast, uint8_t *result); // asm simd

void brightness_contrast_V1(const uint8_t *img, size_t width, size_t height, float a, float b, float c, int16_t brightness,
                            float contrast, uint8_t *result); // c simd

void brightness_contrast_V2(const uint8_t *img, size_t width, size_t height, float a, float b, float c, int16_t brightness,
                            float contrast, uint8_t *result); // asm sisd

void brightness_contrast_V3(const uint8_t *img, size_t width, size_t height, float a, float b, float c, int16_t brightness,
                            float contrast, uint8_t *result); // c sisd

void brightness_contrast_V4(const uint8_t *img, size_t width, size_t height, float a, float b, float c, int16_t brightness,
                            float contrast, uint8_t *result); // c sisd multithreaded

void brightness_contrast_V5(const uint8_t *img, size_t width, size_t height, float a, float b, float c, int16_t brightness,
                            float contrast, uint8_t *result); // c sisd with sqrt_heron

void brightness_contrast_V6(const uint8_t *img, size_t width, size_t height, float a, float b, float c, int16_t brightness,
                            float contrast, uint8_t *result); // c sisd with sqrt_quake

