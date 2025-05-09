#include "brightness_contrast.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <emmintrin.h> //SSE2
#include <smmintrin.h> //SSE4.1
#include <omp.h>

#include "math_utils.h"

const uint16_t bc_default_benchmark_runs = 5000;
const uint8_t bc_default_test_delta = 1;

const BCImplementation bc_implementation[] =
{
  { &brightness_contrast,    "Assembly SIMD"          }, // BCImplAsmSIMD
  { &brightness_contrast_V1, "C SIMD"                 }, // BCImplCSIMD
  { &brightness_contrast_V2, "Assembly SISD"          }, // BCImplAsmSISD
  { &brightness_contrast_V3, "C SISD"                 }, // BCImplCSISD
  { &brightness_contrast_V4, "C SISD Multithreaded"   }, // BCImplCSISD_MT
  { &brightness_contrast_V5, "C SISD with sqrt_heron" }, // BCImplCSISD_Heron
  { &brightness_contrast_V6, "C SISD with sqrt_ieee"  }, // BCImplCSISD_IEEE
};

static_assert((sizeof(bc_implementation) / sizeof(*bc_implementation)) == BCImplMax, "Implementation declared in enum is missing in "
                                                                                     "bc_implementation array");

static const float default_a = 0.2126f;
static const float default_b = 0.7152f;
static const float default_c = 0.0722f;

void bc_init_input(BCInput* input)
{
  input->impl = 0;
  input->benchmark_runs = 0;
  input->benchmark_csv = false;
  input->input_file = NULL;
  input->output_file = NULL;
  input->coeffs[0] = default_a;
  input->coeffs[1] = default_b;
  input->coeffs[2] = default_c;
  input->brightness = 0;
  input->contrast = 0.0f;
  input->run_tests = false;
  input->test_delta = 0;
  input->run_sqrt_tests_benchmark = false;
  input->help_printed = false;
}

static void brightness_contrast_c_sisd(const uint8_t *img, size_t width, size_t height,
                                       float coeff_a, float coeff_b, float coeff_c,
                                       int16_t brightness, float contrast,
                                       uint8_t *result, float (sqrt_func)(float f));

void bc_destroy_input(BCInput* input)
{
  free(input->input_file);
  free(input->output_file);
}

static inline float clamp_float(float min, float max, float val)
{
  const float t = val < min ? min : val;
  return t > max ? max : t;
}

void brightness_contrast_V3(const uint8_t *img, size_t width, size_t height,
                            float a, float b, float c,
                            int16_t brightness, float contrast,
                            uint8_t *result)
{
  brightness_contrast_c_sisd(img, width, height, a, b, c, brightness, contrast, result, &sqrtf);
}

void brightness_contrast_V5(const uint8_t *img, size_t width, size_t height,
                            float a, float b, float c,
                            int16_t brightness, float contrast,
                            uint8_t *result)
{
  brightness_contrast_c_sisd(img, width, height, a, b, c, brightness, contrast, result, &sqrt_heron);
}

void brightness_contrast_V6(const uint8_t *img, size_t width, size_t height,
                            float a, float b, float c,
                            int16_t brightness, float contrast,
                            uint8_t *result)
{
  brightness_contrast_c_sisd(img, width, height, a, b, c, brightness, contrast, result, &sqrt_ieee);
}

static void brightness_contrast_c_sisd(const uint8_t *img, size_t width, size_t height,
                                       float coeff_a, float coeff_b, float coeff_c,
                                       int16_t brightness, float contrast,
                                       uint8_t *result, float (sqrt_func)(float f))
{
  const size_t pixel_count = width * height;
  const float coeff_sum = coeff_a + coeff_b + coeff_c;

  coeff_a /= coeff_sum;
  coeff_b /= coeff_sum;
  coeff_c /= coeff_sum;

  float avg = 0.0f;

  size_t out_idx;
  for (out_idx = 0; out_idx < pixel_count; ++out_idx)
  {
    const float r = img[0];
    const float g = img[1];
    const float b = img[2];
    img += 3;

    const float res_val = clamp_float(0.0f, 255.0f,
      (coeff_a * r + coeff_b * g + coeff_c * b) + (float) brightness);

    result[out_idx] = (uint8_t) rintf(res_val);
    avg += res_val;
  }

  avg /= (float) pixel_count;

  float sigma = 0.0f;
  for (out_idx = 0; out_idx < pixel_count; ++out_idx)
  {
    float val = (float) result[out_idx] - avg;
    sigma += val * val;
  }

  sigma /= (float) pixel_count;
  const float div = (sigma == 0.0f && contrast == sigma) ? 0.0f : contrast / sqrt_func(sigma);
  const float adjusted_avg = ((1.0f - div) * avg);

  for (out_idx = 0; out_idx < pixel_count; ++out_idx)
    result[out_idx] = (uint8_t) rintf(clamp_float(0.0f, 255.0f,
      (div * (float) result[out_idx]) + adjusted_avg));
}

void brightness_contrast_V1(const uint8_t *img, size_t width, size_t height,
                               float a, float b, float c,
                               int16_t brightness, float contrast,
                               uint8_t *result)
{

  //calculate coeff_sum
  float coeff_sum = a + b + c;

  //divide each coeff by sum
  a /= coeff_sum;
  b /= coeff_sum;
  c /= coeff_sum;

  //fill all 4 slots with the respective coefficient
  __m128 coeff_a_xmm = _mm_set1_ps(a);
  __m128 coeff_b_xmm = _mm_set1_ps(b);
  __m128 coeff_c_xmm = _mm_set1_ps(c);

  __m128 clamp_min = _mm_set1_ps(0.0f);
  __m128 clamp_max = _mm_set1_ps(255.0f);

  //load brightness as int16, convert to int32 bc simd doesn't support int16->float
  __m128i brightness_int16 = _mm_set1_epi16(brightness);
  __m128i brightness_int32 = _mm_cvtepi16_epi32(brightness_int16);
  __m128 brightness_float = _mm_cvtepi32_ps(brightness_int32);

  __m128i res_sum = _mm_setzero_si128();

  const size_t pixel_count = width * height;

  __m128i shuffle_mask_r = _mm_set_epi8(-1, -1, -1, 9, -1, -1, -1, 6, -1, -1, -1, 3, -1, -1, -1, 0);
  __m128i shuffle_mask_g = _mm_set_epi8(-1, -1, -1, 10, -1, -1, -1, 7, -1, -1, -1, 4, -1, -1, -1, 1);
  __m128i shuffle_mask_b = _mm_set_epi8(-1, -1, -1, 11, -1, -1, -1, 8, -1, -1, -1, 5, -1, -1, -1, 2);

  //Color conversion loop
  //read 16, move pointer by 12
  size_t i = 0;
  for (i = 0; i*12 < pixel_count * 3 - 16; ++i)
  {
    //load 16 bytes from img; we need 4 pixels -> 4*3 = 12 and 4 unused bytes -> 16
    __m128i raw_data = _mm_loadu_si128((__m128i*) &img[i*12]);

    //extract r values: offsets 9, 6, 3, 0
    __m128i r_values = _mm_shuffle_epi8(raw_data, shuffle_mask_r);

    //extract b values: offsets 10, 7, 4, 1
    __m128i g_values = _mm_shuffle_epi8(raw_data, shuffle_mask_g);

    //extract g values: offsets 11, 8, 5, 2
    __m128i b_values = _mm_shuffle_epi8(raw_data, shuffle_mask_b);


    //convert r g b - values to float
    __m128 red = _mm_cvtepi32_ps(r_values);
    __m128 green = _mm_cvtepi32_ps(g_values);
    __m128 blue = _mm_cvtepi32_ps(b_values);

    //(coeff_a * r + coeff_b * g + coeff_c * b)
    __m128 res = _mm_add_ps(_mm_add_ps(_mm_mul_ps(red, coeff_a_xmm),_mm_mul_ps(green, coeff_b_xmm)),_mm_mul_ps(blue, coeff_c_xmm));

    //add brightness
    res = _mm_add_ps(res, brightness_float);

    //clamp 0.0 <= x <= 255.0
    res = _mm_max_ps(res, clamp_min);
    res = _mm_min_ps(res, clamp_max);

    //convert to 32-bit integers
    __m128i clamped_uint32 = _mm_cvtps_epi32(res);

    //add to res_sum, which is used to calculate average
    res_sum = _mm_add_epi32(res_sum, clamped_uint32);

    //pack 32-bit integers into 16-bit integers
    __m128i clamped_uint16 = _mm_packus_epi32(clamped_uint32, clamped_uint32);

    //pack 16-bit integers into 8-bit integers
    __m128i clamped_uint8 = _mm_packus_epi16(clamped_uint16, clamped_uint16);

    //store the lower 4 bytes (4 pixels) into the result array
    _mm_storeu_si32(&result[i * 4], clamped_uint8);
  }

  //horizontal add res_sum; res_sum until now: [sum, sum, sum, sum]
  res_sum = _mm_hadd_epi32(res_sum, res_sum);
  res_sum = _mm_hadd_epi32(res_sum, res_sum);

  float total_sum = (float) _mm_cvtsi128_si32(res_sum);

  //handle last pixels SISD
  for (size_t result_idx = 4 * i; result_idx < pixel_count; ++result_idx)
  {
    float re = img[result_idx * 3];
    float gr = img[result_idx * 3 + 1];
    float bl = img[result_idx * 3 + 2];

    const float res_val = clamp_float(0.0f, 255.0f,
      (a * re + b * gr + c * bl) + (float) brightness);

    result[result_idx] = (uint8_t) rintf(res_val);

    total_sum += res_val;
  }

  float avg = total_sum / (float) pixel_count;

  //get average in all 4 slots
  __m128 avg_m128 = _mm_set1_ps(avg);

  //sigma Loop
  __m128 sigma_sum = _mm_setzero_ps();
  for (i = 0; i <= pixel_count - 4; i += 4)
  {
    //sigma += (((float) result[out_idx] - avg) * ((float) result[out_idx] - avg));

    //load 4 bytes from res; that's 4 pixels now, since color turned into grey
    __m128i raw_data = _mm_loadu_si32((__m128i*) &result[i]);

    //convert from 8 bit to 32 bit
    __m128i pixels_32bit = _mm_cvtepu8_epi32(raw_data);

    //convert pixel values to float
    __m128 pixels = _mm_cvtepi32_ps(pixels_32bit);


    pixels = _mm_sub_ps(pixels, avg_m128);

    pixels = _mm_mul_ps(pixels, pixels);

    sigma_sum = _mm_add_ps(pixels, sigma_sum);
  }

  sigma_sum = _mm_hadd_ps(sigma_sum, sigma_sum);
  sigma_sum = _mm_hadd_ps(sigma_sum, sigma_sum);

  float sigma = _mm_cvtss_f32(sigma_sum);

  //handle last pixels
  for (; i < pixel_count; ++i)
  {
    sigma += ((float) result[i] - avg) * ((float) result[i] - avg);
  }

  sigma /= (float) pixel_count;

  float div = (sigma == 0.0f && contrast == sigma) ? 0.0f : contrast / sqrtf(sigma);

  float to_add = (1.0f - div) * avg;
  __m128 to_add_m128 = _mm_set1_ps(to_add);
  __m128 div_m128 = _mm_set1_ps(div);

  //contrast loop
  for (i = 0; i <= pixel_count-4; i += 4)
  {
    //load 4 bytes aka 4 pixels
    __m128i raw_data = _mm_loadu_si32((__m32*) &result[i]);
    //convert from 8 bit to 32 bit
    __m128i pixels_32bit = _mm_cvtepu8_epi32(raw_data);
    //convert to float
    __m128 pixels = _mm_cvtepi32_ps(pixels_32bit);

    pixels = _mm_mul_ps(pixels, div_m128);
    pixels = _mm_add_ps(pixels, to_add_m128);

    //clamp 0.0 <= x <= 255.0
    pixels = _mm_max_ps(pixels, clamp_min);
    pixels = _mm_min_ps(pixels, clamp_max);

    __m128i clamped_uint32 = _mm_cvtps_epi32(pixels);

    //pack 32-bit integers into 16-bit integers
    __m128i clamped_uint16 = _mm_packus_epi32(clamped_uint32, clamped_uint32);

    //pack 16-bit integers into 8-bit integers
    __m128i clamped_uint8 = _mm_packus_epi16(clamped_uint16, clamped_uint16);

    //store the lower 4 bytes (4 pixels) into the result array
    _mm_storeu_si32(&result[i], clamped_uint8);
  }

  //handle last pixels sisd
  const float adjusted_avg = (1.0f - div) * avg;
  for (; i < pixel_count; ++i)
  {
    result[i] = (uint8_t) rintf(clamp_float(0.0f, 255.0f,
      (div * (float) result[i]) + adjusted_avg));
  }
}

void brightness_contrast_V4(const uint8_t *img, size_t width, size_t height,
                            float a, float b, float c,
                            int16_t brightness, float contrast,
                            uint8_t *result)
{
  const size_t pixel_count = width * height;
  const float coeff_sum = a + b + c;

  a /= coeff_sum;
  b /= coeff_sum;
  c /= coeff_sum;

  float avg = 0.0f;

  #pragma omp parallel for reduction (+:avg)
  for (size_t out_idx = 0; out_idx < pixel_count; ++out_idx)
  {
    const float red = img[out_idx * 3 + 0];
    const float green = img[out_idx * 3 + 1];
    const float blue = img[out_idx * 3 + 2];

    const float res_val = clamp_float(0.0f, 255.0f,
                                      (a * red + b * green + c * blue) + (float) brightness);

    result[out_idx] = (uint8_t) rintf(res_val);
    avg += res_val;
  }

  avg /= (float) pixel_count;

  float sigma = 0.0f;
  #pragma omp parallel for reduction (+:sigma)
  for (size_t out_idx = 0; out_idx < pixel_count; ++out_idx)
  {
    float val = (float) result[out_idx] - avg;
    sigma += val * val;
  }

  sigma /= (float) pixel_count;

  const float div = (sigma == 0.0f && contrast == sigma) ? 0.0f : contrast / sqrtf(sigma);
  const float adjusted_avg = ((1.0f - div) * avg);

  #pragma omp parallel for
  for (size_t out_idx = 0; out_idx < pixel_count; ++out_idx)
    result[out_idx] = (uint8_t) rintf(clamp_float(0.0f, 255.0f,
                                                 (div * (float) result[out_idx]) + adjusted_avg));
}
