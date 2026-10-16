# target

Everything that runs on Ripes: the IDA* solver as RV32I assembly, and a
standalone C version of the same algorithm that serves as the
`gcc -O2 -march=rv32i` reference.

| File | Contents |
| :--- | :--- |
| `solver.c` | Standalone C solver. Builds for the host (argv, printf) or, under `__riscv`, as a freestanding program for Ripes. |
| `common.h` | Ranking, search, and verification shared by `solver.c` and the unit tests. |
| `test/` | Exhaustive host unit tests for ranking, the modulo lookup tables, and path verification. |
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
make ripes          # build solver.elf and run it on RV32_ISS with --iret
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

## Assembling for Ripes

Ripes does not support `.include`, so the solver source and `tables.s`
must be concatenated into one file before assembling, with the solver
first so that its `.text` comes first:

```sh
cat ida.s tables.s > ida_full.s
```
