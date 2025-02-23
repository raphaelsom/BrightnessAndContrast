#include "brightness_contrast_test.h"

#include <stdio.h>

#include "test_utils.h"

#define MULTITHREADED_TESTRUNS 750

int array_equals(int impl, const size_t size, const uint8_t *result_img, const uint8_t *curr_result,
                 const uint8_t allowed_delta, uint8_t* max_delta, size_t* differing_pixels);

void bc_test_implementations(const BCInput* input, const size_t width, const size_t height,
                             const uint8_t* source_img, uint8_t* result_img, const char* prog_name)
{
  printf("%s: Running tests with %s as reference implementation...\n", prog_name, bc_implementation[0].name);

  // run default implementation as a reference
  bc_implementation[0].impl(source_img, width, height, input->coeffs[0], input->coeffs[1], input->coeffs[2],
                            input->brightness, input->contrast, result_img);

  uint8_t* test_results[BCImplMax - 1];

  // initialize with NULL so we can always call free in END (even on error when not all elements have been malloced)
  for (int i = 0; i < BCImplMax - 1; ++i)
    test_results[i] = NULL;

  for (int impl = 1; impl < BCImplMax; ++impl)
  {
    uint8_t** curr_result = &test_results[impl - 1];
    uint8_t max_delta = 0;
    size_t differing_pixels = 0;

    *curr_result = (uint8_t*) malloc(width * height);
    if (!*curr_result)
    {
      fprintf(stderr, "%s: Test error: Not enough memory", prog_name);
      goto END;
    }

    if (impl == BCImplCSISD_MT)
    {
      bool failed = false;

      // Perform test multiple times on multithreaded impl. to find possible race condition problems
      for (uint32_t i = 0; i < MULTITHREADED_TESTRUNS; ++i)
      {
        size_t curr_diff_indices = 0;
        bc_implementation[impl].impl(source_img, width, height, input->coeffs[0], input->coeffs[1], input->coeffs[2],
                                     input->brightness, input->contrast, *curr_result);

        if (array_equals(impl, width * height, result_img, *curr_result, input->test_delta, &max_delta, &curr_diff_indices))
        {
          failed = true;
          break;
        }

        if (curr_diff_indices > differing_pixels)
          differing_pixels = curr_diff_indices;
      }

      if (!failed)
        printf(TEST_PASSED " %s (max. delta: %u, max. diff. pixels: %lu)\n",
               bc_implementation[impl].name, max_delta, differing_pixels);
    }
    else
    {
      bc_implementation[impl].impl(source_img, width, height, input->coeffs[0], input->coeffs[1], input->coeffs[2],
                                   input->brightness, input->contrast, *curr_result);

      if (array_equals(impl, width * height, result_img, *curr_result, input->test_delta, &max_delta, &differing_pixels))
        continue;

      printf(TEST_PASSED " %s (max. delta: %u, diff. pixels: %lu)\n",
             bc_implementation[impl].name, max_delta, differing_pixels);
    }
  }

END:
  for (int i = 0; i < BCImplMax - 1; ++i)
    free(test_results[i]);
}

int array_equals(int impl, const size_t size, const uint8_t *result_img, const uint8_t *curr_result,
                 const uint8_t allowed_delta, uint8_t* max_delta, size_t* differing_pixels)
{
  for (size_t i = 0; i < size; ++i)
  {
    const uint8_t delta = (uint8_t) abs((int16_t) curr_result[i] - result_img[i]);
    if (delta <= allowed_delta)
    {
      if (delta > *max_delta)
        *max_delta = delta;

      if (delta != 0)
        *differing_pixels += 1;

      continue;
    }

    printf(TEST_FAILED " %s: Value mismatch at index %lu - expected: %u, actual: %u\n",
           bc_implementation[impl].name, i, result_img[i], curr_result[i]);
    return 1;
  }

  return 0;
}
