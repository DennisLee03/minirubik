# host

Host-side C experiments for stage 2: the exact-distance oracle, the
heuristic tables, a reference IDA* solver, and the correctness gates that
check them against each other. Nothing here runs on Ripes.

`solver.c` starts from the baseline `../solver.c` and extends it:

* **Oracle.** `build_table` stores `depth << 4 | move` in each
  `toward_solved` byte, so the exact distance of every state comes out
  of the same BFS that records the move toward solved.
* **Heuristic tables.** During that BFS, `hp[p]` and `ho[o]` keep the
  smallest distance seen for each permutation rank and each orientation
  rank. The heuristic is `h(p, o) = max(hp[p], ho[o])`.
* **IDA\*.** `solve(p, o)` deepens a bound from `h(p, o)` and prunes when
  `g + h` exceeds it, skipping moves on the same face as the previous
  move. It returns the solution length and leaves the moves in `path`.
  This recursive version is a reference for the assembly, which must not
  recurse.
* **Node count.** `nodes` counts expanded nodes only: a call increments
  it after passing the `g + h > bound` test, just before trying its
  children. Calls pruned on entry are not counted.

## Usage

```sh
make                      # build ./solver
./solver 21345671111111   # BFS solution, then IDA* solution, with distances
make rss-distance-test    # distance distribution, with peak RSS
make heuristic-test       # H1 and H2, plus hp/ho and average-h tables
make star-test            # H3: IDA* length equals BFS distance, all states
make apply-test           # every IDA* path returns its state to solved
make worst-test           # most expanded nodes over the distance-11 states
```

| Target | Checks | States |
| :--- | :--- | ---: |
| `rss-distance-test` | Distance distribution matches `report.md` §4 | 3,674,160 |
| `heuristic-test` | H1: `hp[p] <= d` and `ho[o] <= d`; H2: tables fully populated, solved entry 0 | 3,674,160 |
| `star-test` | H3: IDA* returns a solution of exactly the BFS distance | 3,674,160 |
| `apply-test` | Applying each IDA* path reaches the solved state | 3,674,160 |
| `worst-test` | Largest expanded-node count, and the state that reaches it | 2,644 |

`star-test` and `apply-test` run IDA* once per state and take minutes;
the other targets finish in a few seconds.

The worst distance-11 state is rank 2,437,047, `54721631111111`, with
106,635 expanded nodes.
