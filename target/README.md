# target

Everything that runs on Ripes: the IDA* solver as RV32I assembly, and a
standalone C version of the same algorithm that serves as the
`gcc -O2 -march=rv32i` reference.

| File | Contents |
| :--- | :--- |
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

## Assembling for Ripes

Ripes does not support `.include`, so the solver source and `tables.s`
must be concatenated into one file before assembling, with the solver
first so that its `.text` comes first:

```sh
cat ida.s tables.s > ida_full.s
```
