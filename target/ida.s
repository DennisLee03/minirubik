.data
# INPUT INFO
input:  .string "21345671111111"
length: .byte   11

# modulo data
sum_mod3: .byte 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2 # 15 elements
i_mod7:   .byte 0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 6    # 14 elements

# state structure
state_p: .byte 0, 0, 0, 0, 0, 0, 0
state_o: .byte 0, 0, 0, 0, 0, 0, 0

# newline char
nl: .byte 10

.text
.globl _start

_start:
  call parse_state
  call print_state
  exit: 
  li a7, 93
  ecall 

# 
parse_state:
  li t0, 14             # t0: loop counter
  la t1, input          # &input[0]
  la t6, i_mod7         # &i_mod7[0]
  ps_loop:

    li t2, 7             # t2 = 8 temporary 7 for check limit
    blt t2, t0, assign_7 # counter > 7 assign 7
    assign_3:
    li t3, 3
    j limit_check
    assign_7:
    li t3, 7
    
    # input[i] check(ascii '0': 48 ~ '9': 57)
    limit_check:         # limit in t3
    lbu t2, 0(t1)        # t2 = input[i]
    addi t4, t2, -49     # input[i] - '1'
    blt t4, zero, ps_ret_0  # goto return 0
    addi t4, t3, 48      # t4 = '0' + limit(t3)
    sub t4, t4, t2       # t4 = '0' + limit - input[i]
    blt t4, zero, ps_ret_0  # goto return 0
    # end of input check. t4, t3 is free after this

    # assign state
    addi t3, t2, -49     # t3 = input[i] - '1', to be assigned
    lbu t4, 0(t6)        # t4 = i_mod[i]
    li t2, 7
    blt t2, t0, assign_p # counter > 7 assign state_p
    assign_o:
    la t5, state_o       # t5 = &state_o[0]
    add t5, t5, t4       # t5 = &state_o[i]
    sb t3, 0(t5)
    j update_check
    assign_p:
    la t5, state_p       # t5 = &state_p[0]
    add t5, t5, t4       # t5 = &state_p[i]
    sb t3, 0(t5)

    update_check:
    addi t1, t1, 1     # next char
    addi t6, t6, 1     # next mod value
    addi t0, t0, -1    # counter--
    bne t0, zero, ps_loop # 0 <= counter < 14

  ps_ret_v:
  j valid               # tail call: valid's ret returns straight to our caller

  ps_ret_0:
  add a0, zero, zero
  ret

valid:
  mv a1, zero    # a1 = sum = 0
  la a2, state_p # a2 = &state_p[0]
  la a3, state_o # a3 = &state_o[0]
  mv t0, zero    # i = 0
  v_loop_1:
    add t1, a2, t0  # state_p + i
    lbu t1, 0(t1)   # t1 = state_p[i]
    addi t1, t1, -7 # state_p[i] - 7 >= 0
    bge t1, zero, v_ret_0

    add t1, a3, t0  # state_o + i
    lbu t1, 0(t1)   # t1 = state_o[i]
    addi t1, t1, -3 # state_o[i] - 3 >= 0
    bge t1, zero, v_ret_0

    mv t1, zero     # t1: j = 0
    j v_check_2     # test j < i before the first pass
    v_loop_2:
      add t2, a2, t0      # t2 = state_p + i
      lbu t2, 0(t2)       # t2 = state_p[i]
      add t3, a2, t1      # t3 = state_p + j
      lbu t3, 0(t3)       # t3 = state_p[j]
      beq t2, t3, v_ret_0 # state_p[j] == state_p[i] returns 0
      addi t1, t1, 1      # j++
    v_check_2:
      blt t1, t0, v_loop_2  # loop while j < i

    # update sum
    add t2, a3, t0         # state_o + i
    lbu t2, 0(t2)          # t2 = state_o[i]
    add a1, a1, t2         # sum += state_o[i]

    addi t0, t0, 1           # i++
    li t1, 7
    blt t0, t1, v_loop_1     # loop while i < CUBIES

  v_ret:
  # IF sum > 14, sum - 14 > 0
  addi t2, a1, -14
  blt zero, t2, v_ret_0

  la t2, sum_mod3
  add t2, t2, a1  # t2 = sum_mod3 + sum
  lbu t2, 0(t2)   # t2 = sum_mod3[sum]
  bne t2, zero, v_ret_0
  li a0, 1
  ret

  v_ret_0:
  add a0, zero, zero
  ret