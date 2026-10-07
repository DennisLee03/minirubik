.equ N_TEST, 1000000         # #tests modify this
.equ MEM_COUNT, 4            # #words to do lw, add, sw
.data
  start: .word 0, 0, 0, 0    # 4-word address space

.text
.globl main

main:
  li t3, N_TEST              # t3 = #test
  iter_loop:
    la t0, start             # t0 = start address of the 4-word address space
    li t1, MEM_COUNT         # t1 = #words
    mem_loop:
      lw t2, 0(t0)           # t2 = content of the address
      addi t2, t2, 1         # update memory content
      sw t2, 0(t0)           # store back to the address
      addi t0, t0, 4         # next address
      addi t1, t1, -1        # decrement 1 of the counter
      bne zero, t1, mem_loop # check counter
    addi t3, t3, -1          # finish 1 test
    bne zero, t3, iter_loop
  li a7, 10 # Exit = 10
  ecall