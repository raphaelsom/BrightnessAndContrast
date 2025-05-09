.intel_syntax noprefix

.global brightness_contrast

.section .rodata

mask_extrude_r_from_12_rgb: .byte 0, 0x80, 0x80, 0x80, 3, 0x80, 0x80, 0x80, 6, 0x80, 0x80, 0x80, 9, 0x80, 0x80, 0x80
mask_extrude_g_from_12_rgb: .byte 1, 0x80, 0x80, 0x80, 4, 0x80, 0x80, 0x80, 7, 0x80, 0x80, 0x80, 10, 0x80, 0x80, 0x80
mask_extrude_b_from_12_rgb: .byte 2, 0x80, 0x80, 0x80, 5, 0x80, 0x80, 0x80, 8, 0x80, 0x80, 0x80, 11, 0x80, 0x80, 0x80

// 0x80 sets byte to zero
// used to split 4 bytes in lower dword of xmm into all 4 dwords of xmm
mask_split_low_dword_into_4_dwords_0: .byte 0, 0x80, 0x80, 0x80, 1, 0x80, 0x80, 0x80, 2, 0x80, 0x80, 0x80, 3, 0x80, 0x80, 0x80
#ifdef __AVX__
// next three masks only needed if we have support for vpshufb
mask_split_low_dword_into_4_dwords_1: .byte 4, 0x80, 0x80, 0x80, 5, 0x80, 0x80, 0x80, 6, 0x80, 0x80, 0x80, 7, 0x80, 0x80, 0x80
mask_split_low_dword_into_4_dwords_2: .byte 8, 0x80, 0x80, 0x80, 9, 0x80, 0x80, 0x80, 10, 0x80, 0x80, 0x80, 11, 0x80, 0x80, 0x80
mask_split_low_dword_into_4_dwords_3: .byte 12, 0x80, 0x80, 0x80, 13, 0x80, 0x80, 0x80, 14, 0x80, 0x80, 0x80, 15, 0x80, 0x80, 0x80
#endif

mask_combine_low_byte_in_dwords_into_low_dword: .byte 0, 4, 8, 12, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15

.section .text
brightness_contrast:

/*
   Parameters:
    rdi: const uint8_t* img
    rsi: size_t width
    rdx: size_t height
    rcx: int16_t brightness
    r8: uint8_t result

    xmm0: coeff_a
    xmm1: coeff_b
    xmm2: coeff_c
    xmm3: contrast
*/

  // SETUP VARIABLES NEEDED IN ALL LOOPS
  // ===================================

  // copy r8 into r11
  mov r11, r8

  // mov contrast into xmm14
  movaps xmm14, xmm3

  // r9: pixel count (width * height). => rsi, rdx unused
  mov rax, rsi
  mul rdx
  mov r9, rax
  
  // store masks in registers for shuffling bytes
  movups xmm5, [rip + mask_extrude_r_from_12_rgb]
  movups xmm6, [rip + mask_extrude_g_from_12_rgb]
  movups xmm7, [rip + mask_extrude_b_from_12_rgb]
  movups xmm8, [rip + mask_combine_low_byte_in_dwords_into_low_dword]

  // xmm10: used for clamping to 255
  mov r10, 255
  cvtsi2ss xmm10, r10
  pshufd xmm10, xmm10, 0x0

  // xmm11: used for clamping to 0
  pxor xmm11, xmm11

  // avg
  pxor xmm12, xmm12


  // FIRST LOOP: CONVERT TO GRAYSCALE, CALCULATE AVG
  // used: xmm0-2 (coefficients), xmm5-8 masks,
  //       xmm10-11 for clamping, xmm12 avg accumulator, xmm14 contrast (needed later)
  // free: xmm3 (will be coeff. sum), xmm4 (will be brightness) 
  //       xmm9, xmm13, xmm15
  // ======================================

  // xmm3: coeff. sum (a+b+c) in all 4 dwords
  pxor xmm3, xmm3
  addss xmm3, xmm0
  addss xmm3, xmm1
  addss xmm3, xmm2
  pshufd xmm3, xmm3, 0x0
  
  // broadcast coefficients in all dwords
  pshufd xmm0, xmm0, 0x0
  pshufd xmm1, xmm1, 0x0
  pshufd xmm2, xmm2, 0x0

  // divide coefficients by coeff_sum
  divps xmm0, xmm3
  divps xmm1, xmm3
  divps xmm2, xmm3

  // convert rcx (int16_t brightness) to float in xmm4 => rcx unused
  // sign extend cx to rcx
  movsx rcx, cx
  cvtsi2ss xmm4, rcx
  pshufd xmm4, xmm4, 0x0

  // counter
  mov rax, r9
  jmp .LgrayscaleLoopCond
  .LgrayscaleLoop:
    // load rgb, rgb, rgb, rgb, rgb, r
    movups xmm9, [rdi]

    // use 12 bytes = 4 pixel of the 16 bytes read
    // => 4x rgb

#ifdef __AVX__
    vpshufb xmm13, xmm9, xmm6
    vpshufb xmm15, xmm9, xmm7
    vpshufb xmm9, xmm9, xmm5
#else
    movaps xmm13, xmm9
    movaps xmm15, xmm9

    pshufb xmm9, xmm5
    pshufb xmm13, xmm6
    pshufb xmm15, xmm7
#endif

    // => xmm9 = [r, r, r, r], xmm13 = [g, g, g, g], xmm15 = [b, b, b, b]

    // convert to float
    cvtdq2ps xmm9, xmm9
    cvtdq2ps xmm13, xmm13
    cvtdq2ps xmm15, xmm15

    // multiply with coefficients
    mulps xmm9, xmm0
    mulps xmm13, xmm1
    mulps xmm15, xmm2

    // sum up into xmm15
    addps xmm9, xmm13
    addps xmm15, xmm9

    // += brightness
    addps xmm15, xmm4

    // clamp [0.0f, 255.0f]
    maxps xmm15, xmm11
    minps xmm15, xmm10
    
    // avg += val
    addps xmm12, xmm15

    // convert back to int (with rounding)
    cvtps2dq xmm15, xmm15

    pshufb xmm15, xmm8

    movd [r11], xmm15

    // processed 12 bytes => add 12 to source_image
    add rdi, 12

    // 12 bytes = 4 pixel processed
    sub rax, 4

    // 4 bytes written to result
    add r11, 4
  .LgrayscaleLoopCond:
  // as long we have at least 6 pixels remaining (= 6*3 = 18 bytes remaining),
  // we can read 16 byte blocks
  // with <= 5 pixel left (<= 15 bytes left), we can't read 16 bytes at a time
  cmp rax, 6
  jae .LgrayscaleLoop

  // horizontal sum avg (xmm12)
  movhlps xmm13, xmm12
  addps xmm12, xmm13
  pshufd xmm13, xmm12, 0b00000001
  addps xmm12, xmm13

  // process remaining pixels
  jmp .LgrayscaleLoopRestCond
  .LgrayscaleLoopRest:
    // move r,g,b into int registers first, zero extend (because unsigned)
    movzx esi, byte ptr[rdi]
    movzx edx, byte ptr[rdi + 1]
    movzx ecx, byte ptr[rdi + 2]

    // move into xmm6-8 as floats
    cvtsi2ss xmm6, esi
    cvtsi2ss xmm7, edx
    cvtsi2ss xmm8, ecx

    // multiply with coefficients
    mulss xmm6, xmm0
    mulss xmm7, xmm1
    mulss xmm8, xmm2

    // sum up in xmm6
    addss xmm6, xmm7
    addss xmm6, xmm8

    // div by coeff_sum
    divss xmm6, xmm3

    // add brightness, clamp to [0, 255]
    addss xmm6, xmm4
    maxss xmm6, xmm11
    minss xmm6, xmm10

    // avg += xmm6
    addss xmm12, xmm6

    // write to result
    // (use cvtss2si instead of cvttss2si to perform rounding)
    cvtss2si r10, xmm6
    mov byte ptr [r11], r10b

    add rdi, 3
    inc r11
    dec rax

  .LgrayscaleLoopRestCond:
  test rax, rax
  jnz .LgrayscaleLoopRest

  // avg /= (xmm9 = pixel_count) (xmm9 also used later)
  cvtsi2ss xmm9, r9
  divss xmm12, xmm9
  // copy first float in xmm12 in all 4 floats
  pshufd xmm12, xmm12, 0x0


  // SECOND LOOP: CALCULATE SIGMA
  // ======================================
  // xmm0-4, xmm13, xmm15 unused at this point

  // store masks in registers for shuffling bytes
  // (also used for third loop)
  movups xmm5, [rip + mask_split_low_dword_into_4_dwords_0]
#ifdef __AVX__
  movups xmm6, [rip + mask_split_low_dword_into_4_dwords_1]
  movups xmm7, [rip + mask_split_low_dword_into_4_dwords_2]
  movups xmm8, [rip + mask_split_low_dword_into_4_dwords_3]
#endif

  // process 16 bytes per iteration, clamp pixel count to multiple of 16
  xor rdx, rdx
  mov rax, r9
  mov rsi, 16
  div rsi
  mov rcx, r9
  sub rcx, rdx

  // xmm15 = sigma
  pxor xmm15, xmm15

  // reset result pointer
  mov r11, r8

  // idx
  mov rax, 0x0

  // result image is already 16 aligned, as the array is allocated via malloc in C code
  jmp .LsigmaLoopCond
  .LsigmaLoop:
    // load 16 byte from result image
    movaps xmm0, [r11]

    // xmm0 now contains 16 bytes, we need to process each byte
    // => split up into 4 dwords in xmm0-3
#ifdef __AVX__
    // split up xmm0 last to not lose bytes for xmm1-3
    vpshufb xmm1, xmm0, xmm6
    vpshufb xmm2, xmm0, xmm7
    vpshufb xmm3, xmm0, xmm8
    vpshufb xmm0, xmm0, xmm5
#else
    // move upper 3 dwords from xmm0 to xmm1-3
    pshufd xmm1, xmm0, 0b00000001
    pshufd xmm2, xmm0, 0b00000010
    pshufd xmm3, xmm0, 0b00000011

    // split lower 4 bytes into 4 dwords
    // (always use xmm5 to split lower dword into 4 dwords)
    pshufb xmm0, xmm5
    pshufb xmm1, xmm5
    pshufb xmm2, xmm5
    pshufb xmm3, xmm5
#endif

    // convert to float
    cvtdq2ps xmm0, xmm0
    cvtdq2ps xmm1, xmm1
    cvtdq2ps xmm2, xmm2
    cvtdq2ps xmm3, xmm3

    // subtract avg, square
    subps xmm0, xmm12
    subps xmm1, xmm12
    subps xmm2, xmm12
    subps xmm3, xmm12
    mulps xmm0, xmm0
    mulps xmm1, xmm1
    mulps xmm2, xmm2
    mulps xmm3, xmm3

    // sum up in sigma
    addps xmm0, xmm1
    addps xmm2, xmm3
    addps xmm0, xmm2
    addps xmm15, xmm0

    add rax, 16
    add r11, 16

  .LsigmaLoopCond:
  cmp rax, rcx
  jb .LsigmaLoop

  // sigma/xmm15 horizontal sum
  movhlps xmm13, xmm15
  addps xmm15, xmm13
  pshufd xmm13, xmm15, 0b00000001
  addps xmm15, xmm13

  jmp .LsigmaLoopRestCond
  .LsigmaLoopRest:

    // load byte from result image
    movzx esi, byte ptr [r11]
    cvtsi2ss xmm0, esi

    // subtract avg, square
    subss xmm0, xmm12
    mulss xmm0, xmm0

    // sum up in sigma
    addss xmm15, xmm0

    inc rax
    inc r11

  .LsigmaLoopRestCond:
  cmp rax, r9
  jb .LsigmaLoopRest

  .LsigmaDone:

  // sigma /= pixel_count
  divss xmm15, xmm9

  // THIRD LOOP: APPLY CONTRAST
  // ==========================
  // xmm0-4, xmm13 unused at this point
  // xmm12 = avg, xmm14 = contrast, xmm15 = sigma
  // xmm5-xmm8: mask_split_low_dword_into_4_dwords_0-mask_split_low_dword_into_4_dwords_3

  // calculate "div" == contrast / sqrt(sigma)
  // xmm13 will be 1 - div
  mov rdi, 1
  cvtsi2ss xmm13, rdi

  // xmm15 == sigma == 0? 
  movd rax, xmm15
  test rax, rax
  jnz .LcalcDiv

  // contrast == 0 (== sigma)?
  movd rsi, xmm14
  test rsi, rsi
  jnz .LcalcDiv

  // contrast and sigma == 0 => xmm14 = div = 0
  pxor xmm14, xmm14
  jmp .LcalcAdjustedAvg
  
  .LcalcDiv:
  sqrtss xmm15, xmm15
  // xmm14 = contrast/sigma == "div"
  divss xmm14, xmm15
  pshufd xmm14, xmm14, 0x0

  // xmm13 = (1 - div)
  subss xmm13, xmm14

  .LcalcAdjustedAvg:
  // adjust average (xmm12) to: (1 - div) * avg == xmm13 * avg
  mulss xmm12, xmm13
  pshufd xmm12, xmm12, 0x0
  // => xmm13 unused

  // load mask into xmm13
  movups xmm13, [rip + mask_combine_low_byte_in_dwords_into_low_dword]

  // for loop: compare pixel_count with 16 (simd loop)
  mov rax, 16
  jmp .LcontrastLoopCond
  .LcontrastLoop:  
    // load 16 byte from result image
    movaps xmm0, [r8]

    // xmm0 now contains 16 bytes, we need to process each byte
    // => split up into 4 dwords in xmm0-3 into 4 dwords
#ifdef __AVX__
    // split up xmm0 last to not lose bytes for xmm1-3
    vpshufb xmm1, xmm0, xmm6
    vpshufb xmm2, xmm0, xmm7
    vpshufb xmm3, xmm0, xmm8
    vpshufb xmm0, xmm0, xmm5
#else
    // move upper 3 dwords from xmm0 to xmm1-3
    pshufd xmm1, xmm0, 0b00000001
    pshufd xmm2, xmm0, 0b00000010
    pshufd xmm3, xmm0, 0b00000011

    // split lower 4 bytes into 4 dwords
    // (always use xmm5 to split lower dword into 4 dwords)
    pshufb xmm0, xmm5
    pshufb xmm1, xmm5
    pshufb xmm2, xmm5
    pshufb xmm3, xmm5
#endif

    // convert to float
    cvtdq2ps xmm0, xmm0
    cvtdq2ps xmm1, xmm1
    cvtdq2ps xmm2, xmm2
    cvtdq2ps xmm3, xmm3

    // xmm12 = adjusted_avg, xmm14 = div (both in all 4 floats)

    // val *= div
    mulps xmm0, xmm14
    mulps xmm1, xmm14
    mulps xmm2, xmm14
    mulps xmm3, xmm14

    // val += adjusted_avg
    addps xmm0, xmm12
    addps xmm1, xmm12
    addps xmm2, xmm12
    addps xmm3, xmm12

    // clamp [0.0f, 255.0f]
    maxps xmm0, xmm11
    maxps xmm1, xmm11
    maxps xmm2, xmm11
    maxps xmm3, xmm11
    minps xmm0, xmm10
    minps xmm1, xmm10
    minps xmm2, xmm10
    minps xmm3, xmm10

    // convert back to int (this also performs rounding)
    cvtps2dq  xmm0, xmm0
    cvtps2dq  xmm1, xmm1
    cvtps2dq  xmm2, xmm2
    cvtps2dq  xmm3, xmm3

    // combine lowest byte of each dword in lowest dword
    pshufb xmm0, xmm13
    pshufb xmm1, xmm13
    pshufb xmm2, xmm13
    pshufb xmm3, xmm13

    movd [r8], xmm0
    movd [r8 + 4], xmm1
    movd [r8 + 8], xmm2
    movd [r8 + 12], xmm3

    add r8, 16
    sub r9, 16
  .LcontrastLoopCond:
  cmp r9, rax
  jae .LcontrastLoop

  jmp .LcontrastLoopRestCond
  .LcontrastLoopRest:
    movzx esi, byte ptr [r8]
    cvtsi2ss xmm0, esi

    // xmm12 = adjusted_avg, xmm14 = div (both in all 4 floats)
    
    // ((val * div) + adjusted_avg)
    mulss xmm0, xmm14
    addss xmm0, xmm12
    
    // clamp
    maxss xmm0, xmm11
    minss xmm0, xmm10

    cvtss2si esi, xmm0
    mov byte ptr [r8], sil

    inc r8
    dec r9
  .LcontrastLoopRestCond:
  test r9, r9
  jnz .LcontrastLoopRest

  ret
