CC ?= cc
CFLAGS ?= -O3 -std=c99 -Wall -Wextra -Wpedantic
SAMPLE_STATE := 21345671111111
SAMPLE_SOLUTION := B' R' D2 R' B R B' R D2 B R'
VECTORS := tests/solutions.txt
# One per rejection path: short, long, cubie digit low, cubie digit high,
# orientation digit low, orientation digit high, non-digit, duplicate, parity.
INVALID_STATES := 1234567111111 123456711111111 02345671111111 82345671111111 \
	12345671111110 12345671111114 1234567111111a 11345671111111 12345671111112

RV_CC      := riscv64-unknown-elf-gcc
RV_NM      := riscv64-unknown-elf-nm
RV_AS      := riscv64-unknown-elf-as -march=rv32i -mabi=ilp32
RV_SIZE    := riscv64-unknown-elf-size -A
RV_CPP     := riscv64-unknown-elf-cpp -P -x assembler-with-cpp
RV_CFLAGS  := -O2 -march=rv32i -mabi=ilp32 -std=c99 -ffreestanding
RV_LDFLAGS := -nostdlib -nostartfiles -Wl,-Ttext=0x0 -Wl,-e,_start
RIPES      ?= ~/ca2026_ws/Ripes-v2.2.6-106-g5b8a616-linux-x86_64.AppImage
PROC_MODEL ?= RV32_ISS

.PHONY: all clean test check-sym ripes-gcc ripes-asm ripes-asm-dbg test-asm test-asm-exit test-asm-big size size-asm size-gcc

all: solver solver.elf ida_full.s ida_dbg.s

# hosted program for testing purpose
solver: solver.c
	@$(CC) $(CFLAGS) $< -o $@

test: solver
	@./solver 21345671111111

# gcc-optimized ELF for target
solver.elf: solver.c common.h tables.h
	@$(RV_CC) $(RV_CFLAGS) $(RV_LDFLAGS) $< -o $@

check-sym: solver.elf
	@$(RV_NM) solver.elf

ripes-gcc: solver.elf
	@$(RIPES) --mode cli --src solver.elf -t elf --proc $(PROC_MODEL) --iret

# hand-written assembly for target
ida_full.s: ida.S tables.s aux.s
	@$(RV_CPP) -DDEBUG=0 -DRENDER=0 ida.S > $@

ripes-asm: ida_full.s
	@$(RIPES) --mode cli --src ida_full.s -t asm --proc $(PROC_MODEL) --iret

ida_dbg.s: ida.S tables.s aux.s
	@$(RV_CPP) -DDEBUG=1 -DRENDER=0 ida.S > $@

ripes-asm-dbg: ida_dbg.s
	@$(RIPES) --mode cli --src ida_dbg.s -t asm --proc $(PROC_MODEL) --iret

# run ida.S on every state in testcases/: rank checks (s3, s4),
# exit checks the program's own exit code
test-asm: ida.S tables.s aux.s
	@RIPES=$(RIPES) PROC=$(PROC_MODEL) testcases/run.sh rank

test-asm-exit: ida.S tables.s aux.s
	@RIPES=$(RIPES) PROC=$(PROC_MODEL) testcases/run.sh exit

# 110 states, up to ten per distance 0..11, sampled from the BFS oracle
test-asm-big: ida.S tables.s aux.s
	@RIPES=$(RIPES) PROC=$(PROC_MODEL) testcases/run.sh exit testcases/by_distance.txt

# Code and static-data size. The assembly is measured as GNU as assembles
# ida_full.s, without linker relaxation, which matches what Ripes'
# own assembler produces; the gcc reference is the linked solver.elf.
# "data" sums every allocated data section: .data .rodata .sdata .bss .sbss.
ida.o: ida_full.s
	@$(RV_AS) $< -o $@

size-asm: ida.o
	@$(RV_SIZE) ida.o

size-gcc: solver.elf
	@$(RV_SIZE) solver.elf

size: size-asm size-gcc

clean:
	@rm -f solver solver.elf ida_full.s ida_dbg.s ida.o