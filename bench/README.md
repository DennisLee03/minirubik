# bench

Small RV32I programs used to characterize the Ripes simulator for
stage 1 of the assignment. Both run in Ripes' CLI mode; neither is part
of the solver.

| File | Measures |
| :--- | :--- |
| `guest-byte.s` | Host memory per guest byte. Writes `N` consecutive bytes with `sb`, then exits. |
| `ips.s` | Retired instructions per second. Runs `N_TEST` passes of a `lw`/`addi`/`sw` loop over `MEM_COUNT` words, then exits. |

The parameters are `.equ` constants at the top of each file; edit them
before running.

## Usage

```sh
make memory-ratio                      # peak RSS of guest-byte.s via /usr/bin/time -v
make test_ips                          # --iret and --exectime of ips.s on RV32_ISS
make test_ips PROC_MODEL=RV32_5S       # same, on the 5-stage pipeline
make ripes-info                        # list Ripes CLI options and processor models
```

The Makefile expects the Ripes AppImage in `CA_WS` (default
`~/ca2026_ws`); override `CA_WS` or `RIPES_IMG` if it lives elsewhere.

## Method

* **Host bytes per guest byte.** Run `guest-byte.s` with `N = 0` as the
  control and with larger `N`. The ratio is
  `(RSS_N - RSS_0) * 1024 / N`, using "Maximum resident set size"
  (KiB) from `/usr/bin/time -v`. Take the maximum RSS over several runs.
* **Instructions per second.** Divide the retired instruction count by
  the model execution time (`--exectime`, in ms). Take the minimum time
  over several runs. Size `N_TEST` so a run lasts at least a few
  seconds; the pipelined models are much slower than `RV32_ISS`.

All measurements in the report used Ripes v2.2.6-106-g5b8a616.
