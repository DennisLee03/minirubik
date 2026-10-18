# target

Everything that runs on Ripes: the IDA* solver as RV32I assembly, and a
standalone C version of the same algorithm that serves as the
`gcc -O2 -march=rv32i` reference.

| File | Contents |
| :--- | :--- |
| `ida.S` | The hand-written RV32I solver. Preprocessed with `cpp` before Ripes sees it. |
| `aux.s` | Debug printers for `ida.S`, linked only into the `DEBUG=1` build. |
| `solver.c` | Standalone C solver. Builds for the host (argv, printf) or, under `__riscv`, as a freestanding program for Ripes. |
| `common.h` | Ranking, search, and verification shared by `solver.c` and the unit tests. |
| `test/` | Exhaustive host unit tests for ranking, the modulo lookup tables, and path verification. |
| `testcases/` | Test states for the assembly and `run.sh`, which runs them on Ripes. |
| `tables.h` | Transition tables and `hp`/`ho` as C arrays. Generated, do not edit. |
| `tables.s` | The same tables as assembler data. Generated, do not edit. |

Both table files come from `make tables` in `../host/`, which builds them
with the `build_table()` that H1 and H2 check.

## Table layout

| Label | Type | Entries | Bytes |
| :--- | :--- | ---: | ---: |
| `perm_R`, `perm_B`, `perm_D` | `.half` | 3 × 5,040 | 30,240 |
| `orient_R`, `orient_B`, `orient_D` | `.half` | 3 × 729 | 4,374 |
| `hp` | `.byte` | 5,040 | 5,040 |
| `ho` | `.byte` | 729 | 729 |
| Total | | | 40,383 |

`perm_R[r_p]` is the permutation rank after one R quarter turn, and so on
for the other faces; `perm` and `orient` also label the start of each
whole table. Giving every face row its own label lets the assembly load a
row's address with `la` instead of multiplying the face by the row
stride. The tables sit in `.data`, since Ripes may not accept `.rodata`.

## Running the C reference on Ripes

```sh
make ripes-gcc        # build solver.elf and run it on RV32_ISS with --iret
```

A program built the usual way, against picolibc with
`--specs=picolibc.specs`, retires zero instructions on Ripes: picolibc's
linker script places it at `0x80000000`, where Ripes does not start. The
reference is therefore built as a freestanding program with no C library
at all:

| Flag | Purpose |
| :--- | :--- |
| `-O2 -march=rv32i -mabi=ilp32` | The reference configuration the assignment specifies. |
| `-ffreestanding` | No hosted C library is assumed; only headers such as `<stdint.h>`. |
| `-nostdlib -nostartfiles` | Link neither the C library nor its startup code (`crt0`). |
| `-Wl,-Ttext=0x0` | Place the code at address 0, where Ripes starts. |
| `-Wl,-e,_start` | Use our own `_start` as the entry point (also `ld`'s default). |

Without `crt0`, `_start` must do its job. It is a few lines of top-level
`__asm__` in `solver.c`, because it has to run before any C code:

```c
__asm__(
    ".globl _start\n"
    "_start:\n"
    "    .option push\n"
    "    .option norelax\n"
    "    la   gp, __global_pointer$\n"  /* gp for linker relaxation */
    "    .option pop\n"
    "    li   sp, 0x7ffffff0\n"         /* Ripes' CLI starts with sp = 0 */
    "    call ripes_main\n"             /* result comes back in a0 */
    "    li   a7, 93\n"                 /* exit with code a0 */
    "    ecall\n"
    "1:  j    1b\n"
);
```

Ripes' CLI starts with both `sp` and `gp` at 0; its GUI starts `sp` at
`0x7ffffff0`. `_start` sets both explicitly so the CLI and GUI behave the
same. Setting `gp` lets linker relaxation stay on, so the gcc reference
keeps the shorter instruction sequences relaxation produces and the
code-size comparison stays fair. The `la` that sets `gp` is assembled with
relaxation off, since relaxing it would rewrite it relative to the very
`gp` it is setting. `ripes_main` must not be `static`: gcc cannot see the
call inside the `__asm__` string and would otherwise drop the function.

There is no `printf`, so the program reports through its exit code, which
Ripes prints as `Program exited with code: N`. `ripes_main` solves the
inlined `input`, verifies the path through the transition tables, and
returns:

| Code | Meaning |
| :---: | :--- |
| 0 | Solved, the path returns to solved, and its length equals `expected` |
| 1 | `input` is not a valid state |
| 2 | The search found no solution |
| 3 | The path does not return to solved |
| 4 | The path length differs from `expected` |

To solve another state, change `input` and `expected` in `solver.c`;
`expected` is the exact distance reported by `../host/solver`.

## The assembly solver

```sh
make ripes-asm       # build ida_full.s (DEBUG=0) and run it on RV32_ISS with --iret
make ripes-asm-dbg   # build ida_dbg.s (DEBUG=1), which also prints intermediate values
```

`ida.S` solves the state in `input` and checks the result against the
expected distance in `length`, set both before assembling. `_start` runs
the same pipeline as `ripes_main` in `solver.c` and exits with the same
codes (0 on success, 1–4 as in the table above):

1. `parse_state` reads the 14 digits, checks their ranges and that the
   string ends there, and tail-jumps to `valid` for the duplicate-cubie
   and orientation-sum checks.
2. `rank_perm` and `rank_orie` produce $(r_p, r_o)$, with the constant
   multiplies of the Lehmer rank unrolled into shifts and adds.
3. `solve_iteration` runs the iterative IDA* of `common.h`.
4. `verify_path` replays the path through the transition tables and checks
   that it reaches solved; `_start` then compares its length with `length`.

### Register map

| Register | Name | Contents |
| :--- | :--- | :--- |
| `s0`, `s1` | `PROWS`, `OROWS` | addresses of `perm_rows` and `orient_rows` |
| `s3`, `s4` | `ROOT_P`, `ROOT_O` | the input state's $(r_p, r_o)$ |
| `s5` | `BOUND` | current IDA* bound |
| `s6` | `DEPTH` | current depth $g$ |
| `s7`, `s8` | `HP_BASE`, `HO_BASE` | addresses of `hp` and `ho` |
| `s9` | `HPTR` | `&stk_p[g]` |
| `s10` | `BPTR` | `&stk_face[g]` |
| `s11` | `FOUND_LEN` | length returned by the search |

The search makes no calls, so `t` and `a` registers are free scratch
inside it; helpers called from `_start` use only `t` and `a` registers.

### Search data

The seven per-level arrays are declared back to back, so two pointers
reach all of them through fixed offsets (the `OFF_*` defines):

| Pointer | Array | Offset | Element |
| :--- | :--- | ---: | :--- |
| `HPTR` | `stk_p`, `stk_o`, `stk_np`, `stk_no` | 0, 24, 48, 72 | halfword |
| `BPTR` | `stk_face`, `stk_turn`, `path` | 0, 12, 24 | byte |

Descending one level adds 1 to `DEPTH`, 2 to `HPTR`, and 1 to `BPTR`;
backtracking subtracts the same. `perm_rows` and `orient_rows` hold the
address of each face's table row, so a face index selects its row with a
shift and a load. Changing the order or size of these declarations
requires updating the offsets.

### Building with cpp

Ripes' assembler supports neither `.include`, `.if`, nor `.macro`, and
treats `;` as the start of a comment. `ida.S` is therefore run through
the C preprocessor before Ripes sees it:

```sh
riscv64-unknown-elf-cpp -P -x assembler-with-cpp -DDEBUG=0 -DRENDER=0 ida.S > ida_full.s
```

`#include "tables.s"` comes first so that `perm_rows` and `orient_rows`
can name table labels already defined (Ripes rejects forward references
in `.word`). `aux.s` is included last, under `#if DEBUG`, so `_start`
stays the first instruction. `#define` works for register names and
constants; a macro that expands to several instructions does not, because
`cpp` joins them on one line and Ripes drops everything after `;`.

## Testing the assembly

```sh
make test-asm        # 20 cases: (r_p, r_o) in s3/s4 for valid states, exit 1 for invalid
make test-asm-exit   # 20 cases: the program's own verdict, exit 0 or 1
make test-asm-big    # 110 states, up to ten per distance 0-11
```

`testcases/run.sh` copies `ida.S`, substitutes each state into `input`
and its exact distance into `length`, builds with `DEBUG=0`, runs it on
Ripes, and checks the result. `valid.txt` and `invalid.txt` hold ten
states each, `by_distance.txt` holds the 110 states sampled from the BFS
oracle, and `search_expected.txt` lists the first bound, node count, and
path that the C search produces for the valid states.

## Results

On `RV32_ISS`, all 20 cases of `make test-asm-exit` and all 110 of
`make test-asm-big` pass. Against the gcc `-O2` build of `solver.c`:

| | Assembly | gcc -O2 | Change |
| :--- | ---: | ---: | ---: |
| Retired, `21345671111111` | 10,743,259 | 11,917,723 | −9.9% |
| Retired, `54721631111111` (worst) | 29,374,893 | 32,584,398 | −9.9% |
| `.text` bytes | 1,276 | 1,396 | −8.6% |

The worst case uses 59% of the $5 \times 10^7$ budget. The C search
expands 106,635 nodes on `54721631111111`; assuming the assembly, which
tries children in the same order, expands the same nodes, that is about
275 retired instructions per node against gcc's 306. The assembly does
not count its own nodes, so this assumption has not been checked
directly. The assembly's
`.text` was measured by assembling `ida_full.s` with
`riscv64-unknown-elf-as`; the gcc figure is the `.text` section of
`solver.elf`, excluding the tables in `.rodata`.
