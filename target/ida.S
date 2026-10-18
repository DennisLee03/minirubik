#include "tables.s"

#define PROWS s0
#define OROWS s1

/* root state, set by _start after ranking */
#define ROOT_P   s3
#define ROOT_O   s4

/* IDA* search state, kept for the whole search */
#define BOUND     s5
#define DEPTH     s6      /* g */
#define HP_BASE   s7      /* address of hp */
#define HO_BASE   s8      /* address of ho */
#define HPTR      s9      /* &stk_p[g]    */
#define BPTR      s10     /* &stk_face[g] */
#define FOUND_LEN s11	

/* problem sizes */
#define CUBIES       7
#define MAX_DEPTH    11      /* diameter: no state is deeper */

/* exit codes, same as ripes_main in solver.c */
#define EXIT_OK          0
#define EXIT_BAD_INPUT   1
#define EXIT_NO_SOLUTION 2
#define EXIT_BAD_PATH    3
#define EXIT_BAD_LENGTH  4

/* Ripes ecall numbers */
#define SYS_PRINT_INT   1
#define SYS_PRINT_STR   4
#define SYS_PRINT_CHAR  11
#define SYS_EXIT        93

/* Offsets for stk_* arrays */
#define OFF_P     0
#define OFF_O     24
#define OFF_NP    48
#define OFF_NO    72
#define OFF_FACE  0
#define OFF_TURN  12
#define OFF_PATH  24

.data
# INPUT INFO
input:  .string "62417351323232"
length: .byte 0

# modulo data
sum_mod3: .byte 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2 # 15 elements
i_mod7:   .byte 0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 6    # 14 elements

# state structure
state_p: .byte 0, 0, 0, 0, 0, 0, 0
state_o: .byte 0, 0, 0, 0, 0, 0, 0

# smaller[6] for rank_perm
smaller: .zero 6

# move constant
move_face:  .byte 0, 0, 0, 1, 1, 1, 2, 2, 2
move_turns: .byte 1, 2, 3, 1, 2, 3, 1, 2, 3

#if DEBUG
msg_print_state: .string "State Representation:\n"
msg_permutation: .string "Calculate Permutation:\n"
msg_orientation: .string "Calculate Orientation:\n"
msg_p_equ:       .string "p = "
msg_o_equ:       .string "o = "
#endif

/**
 * WARNING:
 *     Due to data(.byte, .string) defined above,
 *     alignment is crucial after the above declaration.
 *     .halfword and .word need to be aligned!!!!!!
 */

.align 2 # aligned with multiple of 4 bytes
perm_rows:   .word perm_R, perm_B, perm_D
orient_rows: .word orient_R, orient_B, orient_D

# for solve_iteration to keep information
# continuous addresses, use one addr as base and offsets to access each
stk_p:    .zero 24  # 12 halfwords
stk_o:    .zero 24  # 12 halfwords
stk_np:   .zero 24  # 12 halfwords
stk_no:   .zero 24  # 12 halfwords
stk_face: .zero 12  # 12 bytes
stk_turn: .zero 12  # 12 bytes
path:     .zero 11  # 11 bytes


.text
.globl _start

_start:

  # calculate state
  call parse_state
  li t0, 1
  #if DEBUG
  mv t1, a0
  la a0, msg_print_state
  li a7, SYS_PRINT_STR
  ecall
  call print_state
  mv a0, t1
  #endif
  beq a0, t0, cal_p_o
  li a0, EXIT_BAD_INPUT
  j exit

  # calculate (p, o)
  cal_p_o:
  call rank_perm
  mv ROOT_P, a0
  #if DEBUG
  la a0, msg_p_equ
  li a7, SYS_PRINT_STR
  ecall
  mv a0, ROOT_P
  li a7, SYS_PRINT_INT
  ecall
  li a0, 10 # \n
  li a7, SYS_PRINT_CHAR
  ecall
  li a0, 10 # \n
  li a7, SYS_PRINT_CHAR
  ecall
  #endif
  
  call rank_orie
  mv ROOT_O, a0
  #if DEBUG
  la a0, msg_o_equ
  li a7, SYS_PRINT_STR
  ecall
  mv a0, ROOT_O
  li a7, SYS_PRINT_INT
  ecall
  li a0, 10 # \n
  li a7, SYS_PRINT_CHAR
  ecall
  li a0, 10 # \n
  li a7, SYS_PRINT_CHAR
  ecall
  #endif

  # now we have (p, o) in (ROOT_P, ROOT_O)
  call solve_iteration
  mv FOUND_LEN, a0
  li t0, 255
  bne FOUND_LEN, t0, verify
  li a0, 2
  j exit

  verify:
  call verify_path
  li t0, 1
  beq a0, t0, test_expected
  li a0, EXIT_BAD_PATH
  j exit

  test_expected:
  la t0, length
  lbu t0, 0(t0)
  bne t0, FOUND_LEN, exit # length != expected
  li a0, 0

  exit: 
  li a7, SYS_EXIT
  ecall 

# parse_state
#   input:  a string of state
#   output: arrays in state_p and state_o
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
  # check null terminated
  lbu t1, 0(t1)
  bne t1, zero, ps_ret_0

  # tail call: valid's ret returns straight to our caller
  j valid

  ps_ret_0:
  add a0, zero, zero
  ret

# valid
#    input:  state_p, state_o
#    output: 1 for valid, 0 for invalid
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

# rank_perm
#    input:  state_p
#    output: permutation
rank_perm:
  #if DEBUG
  la a0, msg_permutation
  li a7, SYS_PRINT_STR
  ecall
  #endif
  mv s0, zero    # s0: p=0
  la s1, state_p # s1: state_p
  la s2, smaller # s2: smaller
  li t0, 0       # t0: i=0
  rp_loop_i:

    #if DEBUG
    mv a0, t0
    li a7, SYS_PRINT_INT
    ecall
    li a0, 58
    li a7, SYS_PRINT_CHAR
    ecall
    li a0, 32
    li a7, SYS_PRINT_CHAR
    ecall
    #endif

    addi t1, t0, 1          # t1: j = i + 1
    rp_loop_j:
      #if DEBUG
      mv a0, t1
      li a7, SYS_PRINT_INT
      ecall
      #endif

      # i(t0), j(t1)
      add t2, s1, t0 
      lbu t2, 0(t2)           # t2 = state_p[i]
      add t3, s1, t1
      lbu t3, 0(t3)           # t3 = state_p[j]
      bge t3, t2, rp_update_j # if(state_p[j] >= state_p[i]) no smaller[i]++
      add t2, s2, t0          # t2 = smaller + i
      lbu t3, 0(t2)           # t3 = smaller[i]
      addi t3, t3, 1          # smaller[i]++
      sb t3, 0(t2)            # smaller[i]++

      rp_update_j:
      addi t1, t1, 1        # j++
      li t2, 7              # t2 = 7 (temporary)
      blt t1, t2, rp_loop_j # check j < CUBIES

    #if DEBUG
    li a0, 62
    li a7, SYS_PRINT_CHAR
    ecall
    add t3, s2, t0
    lbu t3, 0(t3)
    mv a0, t3
    li a7, SYS_PRINT_INT
    ecall
    li a0, 10
    li a7, SYS_PRINT_CHAR
    ecall
    #endif

    addi t0, t0, 1          # i++
    li t1, 6                # t1 = 6 (temporary)
    blt t0, t1, rp_loop_i   # check i < 6(7 is useless)

  # unroll:
  # p = smaller[0]
  lbu t0, 0(s2)
  mv s0, t0

  # p = (p << 2) + (p << 1) + smaller[1]
  slli t0, s0, 2  # (p<<2)
  slli t1, s0, 1  # (p<<1)
  add s0, t0, t1  # p = (p<<2) + (p<<1)
  lbu t0, 1(s2)   # 
  add s0, s0, t0  # p += smaller[1]

  # p = (p << 2) + p + smaller[2]
  slli t0, s0, 2  # (p<<2)
  mv t1, s0       # p
  add s0, t0, t1  # p = (p << 2) + p
  lbu t0, 2(s2)   # 
  add s0, s0, t0  # p += smaller[2]

  # p = (p << 2) + smaller[3]
  slli s0, s0, 2  # (p <<= 2)
  lbu t0, 3(s2)   # 
  add s0, s0, t0  # p += smaller[3]

  # p = (p << 2) - p + smaller[4]
  slli t0, s0, 2  # (p<<2)
  mv t1, s0       # p
  sub s0, t0, t1  # p = (p << 2) - p
  lbu t0, 4(s2)   # 
  add s0, s0, t0  # p += smaller[4]

  # p = (p << 1) + smaller[5]
  slli s0, s0, 1  # (p<<1)
  lbu t0, 5(s2)   # 
  add s0, s0, t0  # p += smaller[5]

  mv a0, s0
  ret

# rank_orie:
#     input:  state_o
#     output: orientation
rank_orie:
  #if DEBUG
  la a0, msg_orientation
  li a7, SYS_PRINT_STR
  ecall
  #endif
  mv s0, zero    # s0: o=0
  la s1, state_o # s1: state_o
  lbu s0, 0(s1)  # o = state_o[0]
  li t0, 1       # t0: i=0
  ro_loop:
    # o = (o << 1) +  o /* x3 */ + state->o[i]
    slli t1, s0, 1     # t1 = o << 1
    add s0, t1, s0     # o = (o << 1) + o
    add t1, s1, t0     # t1 = state_o + i
    lbu t1, 0(t1)      # t1 = state_o[i]
    add s0, s0, t1     # o = (o << 1) + o + state_o[i]

    addi t0, t0, 1      # i++
    li t1, 6            # t1 = 6 (temporary)
    blt t0, t1, ro_loop # check i < 6

  mv a0, s0
  ret

# verify_path
#    input:  ROOT_P, ROOT_O, path, len
#    output: 1 valid, 0: invalid path 
verify_path:
  mv t0, ROOT_P       # t0: next_p=p0
  mv t1, ROOT_O       # t1: next_o=o0
  la t3, path         # t3: path
  la a1, move_face    # a1: move_face
  la a2, move_turns   # a2: move_turns
  la a3, perm_rows    # a3: perm_rows = {perm_R, perm_B, perm_D}
  la a4, orient_rows  # a4: orient_rows = {orient_R, orient_B, orient_D}
  li t2, 0            # t2: i=0
  vp_loop_i:
    bge t2, FOUND_LEN, vp_ret   # i >= len: terminate loop
    add t4, t3, t2              # t4 = path + i
    lbu t4, 0(t4)               # t4 = move = path[i]
    add t5, a1, t4              # t5 = move_face + move
    lbu t5, 0(t5)               # t5 = face = move_face[move]
    add t6, a2, t4              # t6 = move_turns + move
    lbu t6, 0(t6)               # t6 = turns = move_turns[move]

    mv t4, zero
    vp_loop_turn:
      bge t4, t6, vp_update_i
      slli a6, t5, 2     # a6 = face * 4
      add a5, a3, a6     # a5 = &permutation[face]
      lw a5, 0(a5)       # a5 = permutation[face]
      slli t0, t0, 1     # t0 = next_p * 2 bytes
      add t0, a5, t0     # t0 = &permutation[face][next_p]
      lhu t0, 0(t0)      # t0 = permutation[face][next_p]
      add a5, a4,a6     # a5 = &orientation[face]
      lw a5, 0(a5)       # a5 = orientation[face]
      slli t1, t1, 1     # t1 = next_o * 2 bytes
      add t1, a5, t1     # t1 = &orientation[face][next_o]
      lhu t1, 0(t1)      # t1 = orientation[face][next_o]
      addi t4, t4, 1     # turn++
      j vp_loop_turn

    vp_update_i:
    addi t2, t2, 1       # i++
    j vp_loop_i

  vp_ret:
  bne t0, zero, vp_ret_0
  bne t1, zero, vp_ret_0
  li a0, 1
  ret
  vp_ret_0:
  mv a0, zero
  ret

# solve_iteration
#    input:  ROOT_P, ROOT_O
#    output: #moves
solve_iteration:
  
  #if DEBUG
  li a0, 40             # (
  li a7, SYS_PRINT_CHAR
  ecall
  mv a0, ROOT_P         # (p
  li a7, SYS_PRINT_INT
  ecall
  li a0, 44             # (p,
  li a7, SYS_PRINT_CHAR
  ecall
  mv a0, ROOT_O         # (p,o
  li a7, SYS_PRINT_INT
  ecall
  li a0, 41             # (p,o)
  li a7, SYS_PRINT_CHAR
  ecall
  li a0, 10
  li a7, SYS_PRINT_CHAR
  ecall
  #endif

  # check solved
  bne zero, ROOT_P, si_get_bound # p0 == 0 ?
  bne zero, ROOT_O, si_get_bound # o0 == 0 ?
  li a0, 0
  ret

  # get bound
  si_get_bound:
  la HP_BASE, hp
  add t0, HP_BASE, ROOT_P  # t0 = hp + p0
  lbu t0, 0(t0)            # t0 = hp[p0]
  la HO_BASE, ho
  add t1, HO_BASE, ROOT_O  # t1 = ho + o0
  lbu t1, 0(t1)            # t1 = ho[o0]

  #if DEBUG
  li a0, 40             # (
  li a7, SYS_PRINT_CHAR
  ecall
  mv a0, t0             # (hp[p0]
  li a7, SYS_PRINT_INT
  ecall
  li a0, 44             # (hp[p0],
  li a7, SYS_PRINT_CHAR
  ecall
  mv a0, t1             # (hp[p0],ho[o0]
  li a7, SYS_PRINT_INT
  ecall
  li a0, 41             # (hp[p0],ho[o0])
  li a7, SYS_PRINT_CHAR
  ecall
  li a0, 10
  li a7, SYS_PRINT_CHAR
  ecall
  #endif

  mv BOUND, t0
  bge t0, t1, si_loop_bound
  mv BOUND, t1
  si_loop_bound:
    li t0, 11
    blt t0, BOUND, si_ret_err

    #if DEBUG
    li a0, 66
    li a7, SYS_PRINT_CHAR
    ecall
    mv a0, BOUND
    li a7, SYS_PRINT_INT
    ecall
    li a0, 10
    li a7, SYS_PRINT_CHAR
    ecall
    #endif

    # perm & orie transition tables setup
    la PROWS, perm_rows
    la OROWS, orient_rows

    # DFS root
    # stk_* arrays setup
    la HPTR, stk_p          # base address for halfword arrays (stk_{p, o, np, no})
    la BPTR, stk_face       # base address for byte arrays (stk_{face, turn}, path)
    li DEPTH, 0             # g=0
    sh ROOT_P, OFF_P(HPTR)  # p[0] = p0
    sh ROOT_O, OFF_O(HPTR)  # o[0] = o0
    sb zero, OFF_FACE(BPTR) # face[0] = 0
    sb zero, OFF_TURN(BPTR) # turn[0] = 0
    sh ROOT_P, OFF_NP(HPTR) # next_p[0] = p0
    sh ROOT_O, OFF_NO(HPTR) # next_o[0] = o0
    
    si_while_1:
      # explore nodes at level g:
      si_loop_face:
        lbu t1, OFF_FACE(BPTR)          # t1 = face[g]
        li t2, 3                        # t2 = 3
        bge t1, t2, si_loop_face_break  # face[g] >= 3: break

        // if(g > 0 && face[g] == face[g-1]): skip this face
        bge zero, DEPTH, si_loop_turn   // g <= 0: not continue 
        addi t2, BPTR, -1               // t2 = base of &face[g-1]
        lbu t2, OFF_FACE(t2)            // t2 = face[g-1]
        beq t1, t2, si_loop_face_update // face[g] == face[g-1]: continue

        // for(turn)
        si_loop_turn:
          # check loop boundary
          lbu t3, OFF_TURN(BPTR)          # t3 = turn[g], end this loop
          li t2, 3
          bge t3, t2, si_loop_face_update # turn[g] >= 3

          # 1. turn by transition tables
          slli t0, t1, 2          # t0 = face[g] * 4
          # next_p[g] = permutation[face[g]][next_p[g]]
          lhu a1, OFF_NP(HPTR)    # a1 = next_p[g]
          add a2, PROWS, t0       # a2 = &perm[face[g]]
          lw a2, 0(a2)            # a2 = perm[face[g]]
          slli t4, a1, 1          # t4 = next_p[g] * 2
          add t4, a2, t4          # t4 = &perm[face[g]][next_p[g]]
          lhu t4, 0(t4)           # t4 = perm[face[g]][next_p[g]] 
          sh t4, OFF_NP(HPTR)     # next_p[g] = the table value
          # next_o[g] = orientation[face[g]][next_o[g]]
          lhu a3, OFF_NO(HPTR)    # a3 = next_o[g]
          add a2, OROWS, t0       # a2 = &orie[face[g]]
          lw a2, 0(a2)            # a2 = orie[face[g]]
          slli t5, a3, 1          # t5 = next_o[g] * 2
          add t5, a2, t5          # t5 = &orie[face[g]][next_o[g]]
          lhu t5, 0(t5)           # t5 = orie[face[g]][next_o[g]] 
          sh t5, OFF_NO(HPTR)     # next_o[g] = the table value
          # INFO: ====== t4: next_p[g], t5: next_o[g] ======
          # INFO: ====== a1: next_p + g, a3: next_o + g ======

          # update path[g]: t1=face[g], t3=turn[g]
          slli t0, t1, 1        # t0 = (face[g] << 1)
          add t0, t0, t1        # t0 = (face[g] << 1) + face[g]
          add t0, t0, t3        # t0 = (face[g] << 1) + face[g] + turn[g]
          sb t0, OFF_PATH(BPTR) # path[g] = move
          addi t3, t3, 1        # turn[g]++
          sb t3, OFF_TURN(BPTR) # turn[g]++

          # 2. check solved
          bne t4, zero, si_prune  # check next_p[g] != 0
          bne t5, zero, si_prune  # check next_o[g] != 0
          mv a0, DEPTH            # a0 = g
          addi a0, a0, 1          # a0 = g+1
          ret

          # 3. prune
          // if((g+1) + h(next_p[g], next_o[g]) > bound) continue;
          si_prune:
          add t0, HP_BASE, t4    # t0 = hp + next_p[g]
          lbu t0, 0(t0)          # t0 = hp[next_p[g]]
          add t2, HO_BASE, t5    # t2 = ho + next_o[g]
          lbu t2, 0(t2)          # t2 = ho[next_o[g]]
          bge t0, t2, si_skip_mv # t0 >= t2: keep t0's value 
          mv t0, t2
          si_skip_mv:                 # t0 = h(p,o)
          addi t0, t0, 1
          add t0, t0, DEPTH           # t0 = g + 1 + h(p,o)
          blt BOUND, t0, si_loop_turn # bound < estimate: continue

          // 4. update
          addi DEPTH, DEPTH, 1 # g++
          addi HPTR, HPTR, 2
          addi BPTR, BPTR, 1
          sh t4, OFF_P(HPTR)      # p[g] = next_p[g-1] 
          sh t5, OFF_O(HPTR)      # o[g] = next_o[g-1]
          sb zero, OFF_FACE(BPTR) # face[g] = 0
          sb zero, OFF_TURN(BPTR) # turn[g] = 0
          sh t4, OFF_NP(HPTR)     # next_p[g] = p[g]
          sh t5, OFF_NO(HPTR)     # next_o[g] = o[g]
          j si_while_1

        // update stacks
        si_loop_face_update:
        addi t1, t1, 1          //
        sb t1, OFF_FACE(BPTR)   // 1. face[g]++
        sb zero, OFF_TURN(BPTR) // 2. turn[g] = 0
        lhu t2, OFF_P(HPTR)     // 3. t2 = p[g]
        sh t2, OFF_NP(HPTR)     //    next_p[g] = p[g]
        lhu t2, OFF_O(HPTR)     // 4. t2 = o[g]
        sh t2, OFF_NO(HPTR)     //    next_o[g] = o[g]
        j si_loop_face

      si_loop_face_break:

      /* if (g == 0) break while_1; */
      beq DEPTH, zero, si_loop_bound_continue

      /* go back one level */
      addi DEPTH, DEPTH, -1
      addi HPTR, HPTR, -2
      addi BPTR, BPTR, -1

      si_while_continue:
      j si_while_1

    si_loop_bound_continue:
    addi BOUND, BOUND, 1
    j si_loop_bound

  si_ret_err:
  li a0, 255
  ret

#if DEBUG
#include "aux.s"
#endif