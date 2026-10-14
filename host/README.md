# host

Host-side C experiments for stage 2: the exact-distance oracle, the
heuristic tables, two reference IDA* solvers, the correctness gates that
check them against each other, and the emitters that hand the tables to
`../target/`. Nothing here runs on Ripes.

`solver.c` starts from the baseline `../solver.c` and extends it:

* **Oracle.** `build_table` stores `depth << 4 | move` in each
  `toward_solved` byte, so the exact distance of every state comes out
  of the same BFS that records the move toward solved.
* **Heuristic tables.** During that BFS, `hp[p]` and `ho[o]` keep the
  smallest distance seen for each permutation rank and each orientation
  rank. The heuristic is `h(p, o) = max(hp[p], ho[o])`.
* **IDA\*, recursive.** `solve_recursion(p, o)` deepens a bound from
  `h(p, o)` and prunes when `g + h` exceeds it, skipping moves on the same
  face as the previous move. It returns the solution length and leaves
  the moves in `path`.
* **IDA\*, iterative.** `solve_iteration(p, o)` performs the same search
  without recursion, keeping each level's state, face, turn, and
  partially turned state in fixed 12-entry arrays. It is the blueprint
  for the assembly, which must not recurse, and it visits exactly the
  same nodes as the recursive version.
* **Node count.** `nodes` counts expanded nodes only: a node is counted
  after it passes the `g + h > bound` test, just before its children are
  tried. Nodes pruned on entry are not counted.
* **Table emitters.** `--emit-c` and `--emit-asm` print the transition
  tables and `hp`/`ho` as C arrays or as assembler data.

## Usage

```sh
make                      # build ./solver
./solver 21345671111111   # BFS solution, then IDA* solution, with distances
make rss-distance-test    # distance distribution, with peak RSS
make heuristic-test       # H1 and H2, plus hp/ho and average-h tables
make star-test            # H3: IDA* length equals BFS distance, all states
make apply-test           # every IDA* path returns its state to solved
make worst-test           # most expanded nodes over the distance-11 states
make rec-iter-test        # recursive and iterative IDA* agree, all states
make tables               # write ../target/tables.h and ../target/tables.s
```

| Target | Checks | Solver | States |
| :--- | :--- | :--- | ---: |
| `rss-distance-test` | Distance distribution matches `report.md` §4 | — | 3,674,160 |
| `heuristic-test` | H1: `hp[p] <= d` and `ho[o] <= d`; H2: tables fully populated, solved entry 0 | — | 3,674,160 |
| `star-test` | H3: IDA* returns a solution of exactly the BFS distance | iterative | 3,674,160 |
| `apply-test` | Applying each IDA* path reaches the solved state | recursive | 3,674,160 |
| `worst-test` | Largest expanded-node count, and the state that reaches it | recursive | 2,644 |
| `rec-iter-test` | Both solvers return the same length and path | both | 3,674,160 |

`star-test`, `apply-test`, and `rec-iter-test` run IDA* once or twice
per state and take minutes; the other targets finish in a few seconds.

The worst distance-11 state is rank 2,437,047, `54721631111111`, with
106,635 expanded nodes. Over all 3,674,160 states the two solvers return
the same length, the same path, and the same expanded-node count.

## Tables for the targets

`make tables` regenerates `../target/tables.h` and `../target/tables.s`
from the same `build_table()` that H1 and H2 check, so the standalone C
solver and the RV32I solver link identical data. Rerun it whenever the
tables or the heuristic change.
