#include "math_utils.h"

// Source: https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Approximations_that_depend_on_the_floating_point_representation
float sqrt_ieee(float z)
{
  union { float f; uint32_t i; } val = {z};	/* Convert type, preserving bit pattern */
  /*
   * To justify the following code, prove that
   *
   * ((((val.i / 2^m) - b) / 2) + b) * 2^m = ((val.i - 2^m) / 2) + ((b + 1) / 2) * 2^m)
   *
   * where
   *
   * b = exponent bias
   * m = number of mantissa bits
   */
  val.i -= 1 << 23;	/* Subtract 2^m. */
  val.i >>= 1;		/* Divide by 2. */
  val.i += 1 << 29;	/* Add ((b + 1) / 2) * 2^m. */

  return val.f;		/* Interpret again as float */
}
