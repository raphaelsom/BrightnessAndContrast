.intel_syntax noprefix

.global brightness_contrast_V2

brightness_contrast_V2:
  /*
  rdi: uint8_t*  img
  rsi: size_t    width
  rdx: size_t    height
  rcx: int16_t   brightness
  r8:  uint8_t*  result

  xmm0: float    a
  xmm1: float    b
  xmm2: float    c
  xmm3: float    contrast
   */

  push rbx
  push r12
  push r13
  push r14

  //CONST CALCULATIONS

  //xmm4 -> coeff sum
  movss xmm4, xmm0
  addss xmm4, xmm1
  addss xmm4, xmm2

  //divide abc by coeff sum
  divss xmm0, xmm4
  divss xmm1, xmm4
  divss xmm2, xmm4

  //rbx -> pixel count = width * height
  mov rbx, rsi
  mov rax, rdx
  mul rbx
  mov rbx, rax

  //xmm8 -> avg
  pxor xmm8, xmm8
  
  //r13 -> out_idx bzw counter
  mov r13, 0

  //initialize clamp values
  xorps xmm10, xmm10
  mov rax, 255
  cvtsi2ss xmm12, rax

  //cast brightness to float
  //(float) brightness
  movsx rcx, cx
  cvtsi2ss xmm15, rcx

  //copy res pointer
  mov r14, r8

//------------------------------------------------------------------------------------------------------------------

.LcolorConversionLoop:
  //0->pixel_count
  cmp r13, rbx
  je .LcolorConversionLoopEnd
  
  //get three pixels from img pointer
  movzx r10, byte ptr [rdi]
  movzx r11, byte ptr [rdi + 1]
  movzx r12, byte ptr [rdi + 2]
  add rdi, 3

  //multiply coeffs and color values:

  //xmm5 -> coeff_a * pixel_red
  cvtsi2ss xmm5, r10
  mulss xmm5, xmm0

  //xmm6 -> coeff_b * g
  cvtsi2ss xmm6, r11
  mulss xmm6, xmm1

  //xmm7 -> coeff_c * b
  cvtsi2ss xmm7, r12
  mulss xmm7, xmm2

  //xmm5 -> coeff_a * r + coeff_b * g + coeff_c * b
  addss xmm5, xmm6
  addss xmm5, xmm7

  //add brightness
  addss xmm5, xmm15

  //clamp value to 0 <= x <= 255
  maxss xmm5, xmm10
  minss xmm5, xmm12


  //add value to average
  //avg += (float) result[out_idx]
  addss xmm8, xmm5

  //result[out_idx] = val
  cvtss2si rax, xmm5
  mov byte ptr [r14], al

  //increase loop iterator and res pointer
  inc r14
  inc r13
  jmp .LcolorConversionLoop

//------------------------------------------------------------------------------------------------------------------

.LcolorConversionLoopEnd:
  //avg /= (float) pixel_count
  cvtsi2ss xmm5, rbx
  divss xmm8, xmm5

  //reset loop index and res pointer
  mov r13, 0
  mov r14, r8

  //xmm9 -> sigma
  pxor xmm9, xmm9

//------------------------------------------------------------------------------------------------------------------

.LsigmaLoop:
  /*
  in use from now on:
    xmm3 -> contrast
    xmm5 -> (float) pixel_count
    xmm8 -> avg (finished calculation)
    xmm9 -> sigma
    rbx -> pixel_count (=loop max)
    r8 -> result ptr
    r14 -> result ptr (moving)
  */
  cmp r13, rbx
  je .LsigmaLoopEnd

  //(float) result[out_idx]
  movzx rax, byte ptr [r14]
  cvtsi2ss xmm0, rax
  // -= avg
  subss xmm0, xmm8
  // square
  mulss xmm0, xmm0

  //add to sigma
  addss xmm9, xmm0

  //increase loop index
  inc r13
  inc r14
  jmp .LsigmaLoop

//------------------------------------------------------------------------------------------------------------------

.LsigmaLoopEnd:
  //reset counter for next loop
  xor r13, r13
  //sigma /= (float) pixel_count
  divss xmm9, xmm5

  //div = contrast / sqrtf(sigma), wenn nicht beide 0 sind
  //xmm9 -> sigma; xmm3 -> contrast
  pxor xmm0, xmm0
  movss xmm4, xmm9
  cmpss xmm4, xmm0, 0b00000000
  cvtss2si rax, xmm4
  cmp rax, 0
  je .Lcalc_div

  movss xmm4, xmm3
  cmpss xmm4, xmm0, 0b00000000
  cvtss2si rax, xmm4
  cmp rax, 0
  je .Lcalc_div

  movss xmm3, xmm0
  jmp .LloopWriteFinalImage

.Lcalc_div:
  //xmm9 erst sigma, jetzt sqrt(sigma)
  sqrtss xmm9, xmm9

  //xmm3 erst contrast, jetzt div
  divss xmm3, xmm9

  //xmm1 -> (1.0 - div) * avg
  mov rax, 1
  cvtsi2ss xmm1, rax
  subss xmm1, xmm3
  mulss xmm1, xmm8

  //reset res pointer
  mov r14, r8

//------------------------------------------------------------------------------------------------------------------

.LloopWriteFinalImage:
  cmp r13, rbx
  je .Lend

  //xmm0 -> result[out_idx]
  movzx rax, byte ptr [r14]
  cvtsi2ss xmm0, rax
  
  //MULTIPLY WITH DIV
  mulss xmm0, xmm3

  //ADD BOTH RESULTS
  addss xmm0, xmm1

  //xmm10 still on 0.0 from earlier clamp
  //xmm12 still on 255.0 from earlier clamp
  //CLAMP VAL 0.0 <= x <= 255.0
  maxss xmm0, xmm10
  minss xmm0, xmm12

  //write result
  cvtss2si rax, xmm0
  mov [r14], al
  
  inc r13
  inc r14
  jmp .LloopWriteFinalImage

.Lend:
  pop r14
  pop r13
  pop r12
  pop rbx
  ret
  