CC ?= cc
CFLAGS ?= -O3 -std=c99 -Wall -Wextra -Wpedantic
SAMPLE_STATE := 21345671111111
SAMPLE_SOLUTION := B' R' D2 R' B R B' R D2 B R'
VECTORS := tests/solutions.txt
# One per rejection path: short, long, cubie digit low, cubie digit high,
# orientation digit low, orientation digit high, non-digit, duplicate, parity.
INVALID_STATES := 1234567111111 123456711111111 02345671111111 82345671111111 \
	12345671111110 12345671111114 1234567111111a 11345671111111 12345671111112

.PHONY: all clean rss-test

all: solver

solver: solver.c
	@$(CC) $(CFLAGS) $< -o $@

rss-distance-test: solver
	@/usr/bin/time -v ./solver --distance-test

heuristic-test: solver
	@./solver --heuristic-test

star-test: solver
	@./solver --star-test

apply-test: solver
	@./solver --apply-test

worst-test: solver
	@./solver --worst-test

clean:
	@$(RM) solver