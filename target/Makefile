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
RV_CFLAGS  := -O2 -march=rv32i -mabi=ilp32 -std=c99 -ffreestanding
RV_LDFLAGS := -nostdlib -nostartfiles -Wl,-Ttext=0x0 -Wl,-e,_start
RIPES      ?= ~/ca2026_ws/Ripes-v2.2.6-106-g5b8a616-linux-x86_64.AppImage

.PHONY: all clean test

all: solver solver.elf

solver: solver.c
	@$(CC) $(CFLAGS) $< -o $@

test: solver
	@./solver 21345671111111

solver.elf: solver.c common.h tables.h
	@$(RV_CC) $(RV_CFLAGS) $(RV_LDFLAGS) $< -o $@

check-sym: solver.elf
	@$(RV_NM) solver.elf

ripes: solver.elf
	@$(RIPES) --mode cli --src solver.elf -t elf --proc RV32_ISS --iret

clean:
	@rm solver *.o solver.elf