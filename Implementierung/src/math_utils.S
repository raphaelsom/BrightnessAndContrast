.intel_syntax noprefix
.global sqrt_heron
.global sqrt_heron_n

.section .rodata
  onehalf: .float 0.5

/*
float sqrt_heron_n(float s, uint8_t n)
{
  float x_n = (s + 1.0f) / 2.0f;
  for (uint8_t i = 0; i < n; ++i)
    x_n = 0.5f * (x_n + s / x_n);

  return x_n;
}
*/

.section .text
// function with fixed 7 iterations, as this was good enough to pass our tests on the example images
sqrt_heron:
  /*
    xmm0: float s
  */

  // store S in xmm1 so xmm0 can be used for ret value
  // xmm0: x_n
  // calculate (s + 1) / 2 in xmm1
  movaps xmm1, xmm0
  mov rax, 0x1
  cvtsi2ss xmm2, rax
  addss xmm0, xmm2
  // xmm3 = 0.5f
  movss xmm3, [rip + onehalf]
  mulss xmm0, xmm3

  // 1)
  // xmm4 = s / x_n
#ifdef __AVX__
  vdivss xmm4, xmm1, xmm0
  // xmm4 += x_n
  addss xmm4, xmm0
  // xmm4 *= 0.5
  mulss xmm4, xmm3
#else
  movss xmm4, xmm1
  divss xmm4, xmm0
  addss xmm4, xmm0
  mulss xmm4, xmm3
#endif
  movss xmm0, xmm4

  // 2)
  // xmm4 = s / x_n
#ifdef __AVX__
  vdivss xmm4, xmm1, xmm0
  // xmm4 += x_n
  addss xmm4, xmm0
  // xmm4 *= 0.5
  mulss xmm4, xmm3
#else
  movss xmm4, xmm1
  divss xmm4, xmm0
  addss xmm4, xmm0
  mulss xmm4, xmm3
#endif
  movss xmm0, xmm4

  // 3)
  // xmm4 = s / x_n
#ifdef __AVX__
  vdivss xmm4, xmm1, xmm0
  // xmm4 += x_n
  addss xmm4, xmm0
  // xmm4 *= 0.5
  mulss xmm4, xmm3
#else
  movss xmm4, xmm1
  divss xmm4, xmm0
  addss xmm4, xmm0
  mulss xmm4, xmm3
#endif
  movss xmm0, xmm4

  // 4)
  // xmm4 = s / x_n
#ifdef __AVX__
  vdivss xmm4, xmm1, xmm0
  // xmm4 += x_n
  addss xmm4, xmm0
  // xmm4 *= 0.5
  mulss xmm4, xmm3
#else
  movss xmm4, xmm1
  divss xmm4, xmm0
  addss xmm4, xmm0
  mulss xmm4, xmm3
#endif
  movss xmm0, xmm4

  // 5)
  // xmm4 = s / x_n
#ifdef __AVX__
  vdivss xmm4, xmm1, xmm0
  // xmm4 += x_n
  addss xmm4, xmm0
  // xmm4 *= 0.5
  mulss xmm4, xmm3
#else
  movss xmm4, xmm1
  divss xmm4, xmm0
  addss xmm4, xmm0
  mulss xmm4, xmm3
#endif
  movss xmm0, xmm4

  // 6)
  // xmm4 = s / x_n
#ifdef __AVX__
  vdivss xmm4, xmm1, xmm0
  // xmm4 += x_n
  addss xmm4, xmm0
  // xmm4 *= 0.5
  mulss xmm4, xmm3
#else
  movss xmm4, xmm1
  divss xmm4, xmm0
  addss xmm4, xmm0
  mulss xmm4, xmm3
#endif
  movss xmm0, xmm4

  // 7)
  // xmm4 = s / x_n
#ifdef __AVX__
  vdivss xmm4, xmm1, xmm0
  // xmm4 += x_n
  addss xmm4, xmm0
  // xmm4 *= 0.5
  mulss xmm4, xmm3
#else
  movss xmm4, xmm1
  divss xmm4, xmm0
  addss xmm4, xmm0
  mulss xmm4, xmm3
#endif
  movss xmm0, xmm4

  ret


// function with "custom" amount of iterations, was used for testing.
sqrt_heron_n:
  /*
    xmm0: float s
    rdi: uint8_t n
  */

  // store S in xmm1 so xmm0 can be used for ret value
  // xmm0: x_n
  // calculate (s + 1) / 2 in xmm1
  movaps xmm1, xmm0
  mov rax, 0x1
  cvtsi2ss xmm2, rax
  addss xmm0, xmm2
  // xmm3 = 0.5f
  movss xmm3, [rip + onehalf]
  mulss xmm0, xmm3

  jmp .LloopCond
  .Lloop:

    // xmm4 = s / x_n
#ifdef __AVX__
    vdivss xmm4, xmm1, xmm0
    // xmm4 += x_n
    addss xmm4, xmm0
    // xmm4 *= 0.5
    mulss xmm4, xmm3
#else
    movss xmm4, xmm1
    divss xmm4, xmm0
    addss xmm4, xmm0
    mulss xmm4, xmm3
#endif
    movss xmm0, xmm4

    dec rdi
  .LloopCond:
  test rdi, rdi
  jnz .Lloop

  ret
