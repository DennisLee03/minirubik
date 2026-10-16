
.text
print_state:
  la a2, state_p

  lbu a0, 0(a2)
  li a7, 1
  ecall

  li a0, 32
  li a7, 11
  ecall

  lbu a0, 1(a2)
  li a7, 1
  ecall

  li a0, 32
  li a7, 11
  ecall
 
  lbu a0, 2(a2)
  li a7, 1
  ecall

  li a0, 32
  li a7, 11
  ecall

  lbu a0, 3(a2)
  li a7, 1
  ecall

  li a0, 32
  li a7, 11
  ecall

  lbu a0, 4(a2)
  li a7, 1
  ecall

  li a0, 32
  li a7, 11
  ecall

  lbu a0, 5(a2)
  li a7, 1
  ecall

  li a0, 32
  li a7, 11
  ecall

  lbu a0, 6(a2)
  li a7, 1
  ecall

  # newline
  li a0, 10    # '\n'
  li a7, 11    # print char
  ecall

  la a2, state_o

  lbu a0, 0(a2)
  li a7, 1
  ecall

  li a0, 32
  li a7, 11
  ecall

  lbu a0, 1(a2)
  li a7, 1
  ecall

  li a0, 32
  li a7, 11
  ecall
 
  lbu a0, 2(a2)
  li a7, 1
  ecall

  li a0, 32
  li a7, 11
  ecall

  lbu a0, 3(a2)
  li a7, 1
  ecall

  li a0, 32
  li a7, 11
  ecall

  lbu a0, 4(a2)
  li a7, 1
  ecall

  li a0, 32
  li a7, 11
  ecall

  lbu a0, 5(a2)
  li a7, 1
  ecall

  li a0, 32
  li a7, 11
  ecall

  lbu a0, 6(a2)
  li a7, 1
  ecall

  # newline
  li a0, 10    # '\n'
  li a7, 11    # print char
  mv a0, zero

  ret