#include "sqrt_test.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <test_utils.h>

#include "math_utils.h"

// arbitrary epsilon, seemed good after trial&error testing
static const double epsilon = 0.0001f;

static void test_sqrt(const char* name, float (sqrt_func)(float num));

void test_sqrt_heron(const char* prog_name)
{
  printf("%s: Testing square root implementations with max. allowed delta %f:\n", prog_name, epsilon);
  printf("Implementations are tested on interval [0.0, 10.0] with steps of 0.1f, and on interval [1000.0, 10000.0] with steps of 0.5f.\n");

  test_sqrt("sqrt_heron", &sqrt_heron);
  test_sqrt("sqrt_ieee", &sqrt_ieee);
}

static void test_sqrt(const char* name, float (*sqrt_func)(float))
{
  bool fail = false;

  // test for precision near zero
  for (float f = 0.0f; f < 10.0f; f += 0.1f)
  {
    double res = sqrt_func(f);
    double res_lib = sqrtf(f);

    if (fabs(res - res_lib) <= epsilon)
      continue;

    printf(TEST_FAILED " %s: Unexpected value - expected: %f, actual: %f\n", name, res_lib, res);
    fail = true;
  }

  // when testing with the example images, sigma was mostly in this range when brightness wasn't altered too much
  for (float f = 1000.0f; !fail && f < 10000.0f; f += 0.5f)
  {
    double res = sqrt_func(f);
    double res_lib = sqrtf(f);

    if (fabs(res - res_lib) <= epsilon)
      continue;

    printf(TEST_FAILED " %s: Unexpected value - expected: %f, actual: %f\n", name, res_lib, res);
    fail = true;
    break;
  }

  if (!fail)
    printf(TEST_PASSED " %s: Test passed\n", name);
}
