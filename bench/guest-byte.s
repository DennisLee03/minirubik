.equ N, 1048576               # #bytes, modify this

.data
start: .word 0

.text
.global main

main:
  la t0, start           # t0: start memory address
  li t1, N               # t1: #elements to be stored
  li t2, 255             # t2: stored value
  loop:
    beq zero, t1, end
    sb t2, 0(t0)
    addi t0, t0, 1       # next byte
    addi t1, t1, -1      # decrement counter
    beq zero, zero, loop
  end:
    li a7, 10 # Exit = 10
    ecall