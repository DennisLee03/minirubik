#!/usr/bin/env bash
# Run target/ida.S on every state in testcases/{valid,invalid}.txt.
#
# For each test, the script copies ida.S, replaces the string after
# "input:" with the test state and the byte after "length:" with the
# expected distance, preprocesses it with DEBUG=0 and RENDER=0, and runs
# it on Ripes. It then checks one of two things:
#
#   rank  (default)  valid:   s3 = r_p and s4 = r_o, from the (p, o) comment
#                    invalid: exit code 1
#   exit             valid:   exit code 0 (the program's own verdict)
#                    invalid: exit code 1
#
# Usage: testcases/run.sh [rank|exit] [FILE]    (run from target/)
#   With FILE, every line of FILE is treated as a valid state and the
#   default valid.txt and invalid.txt are skipped.
# Environment: RIPES (AppImage path), PROC (default RV32_ISS).

set -u
mode=${1:-rank}
case $mode in rank|exit) ;; *) echo "usage: $0 [rank|exit] [FILE]" >&2; exit 2 ;; esac
only_file=${2:-}

here=$(cd "$(dirname "$0")/.." && pwd)
ripes=${RIPES:-$HOME/ca2026_ws/Ripes-v2.2.6-106-g5b8a616-linux-x86_64.AppImage}
proc=${PROC:-RV32_ISS}
cpp="riscv64-unknown-elf-cpp -P -x assembler-with-cpp -DDEBUG=0 -DRENDER=0"

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cp "$here/tables.s" "$here/aux.s" "$work/"

run_one() {   # $1 = state, $2 = expected distance (or empty)
    sed -e "s/^\(input:[[:space:]]*\.string[[:space:]]*\)\"[^\"]*\"/\1\"$1\"/" \
        -e "s/^\(length:[[:space:]]*\.byte[[:space:]]*\)[0-9][0-9]*/\1${2:-0}/" \
        "$here/ida.S" > "$work/t.S"
    (cd "$work" && $cpp t.S > t.s) || return 1
    timeout 120 "$ripes" --mode cli --src "$work/t.s" -t asm --proc "$proc" \
        --regs --iret --timeout 60000 2>&1 | tr -d '\000'
}

if [ -n "$only_file" ]; then
    sets="valid:$only_file"
else
    sets="valid:$here/testcases/valid.txt invalid:$here/testcases/invalid.txt"
fi

for set in $sets; do
    kind=${set%%:*}
    grep -v '^#' "${set#*:}" | while read -r state rest; do
        [ -n "$state" ] || continue
        want_po=$(printf '%s' "$rest" | grep -oP '\(p, o\) = \(\K[0-9]+, [0-9]+')
        want_d=$(printf '%s' "$rest" | grep -oP 'd = \K[0-9]+')
        out=$(run_one "$state" "$want_d")
        code=$(printf '%s\n' "$out" | grep -oP 'exited with code: \K-?[0-9]+')
        iret=$(printf '%s\n' "$out" | grep -A1 'instructions retired' | tail -1)
        if [ "$kind" = invalid ]; then
            [ "$code" = 1 ] && v=ok || v="not rejected (exit ${code:-none})"
        elif [ "$mode" = rank ]; then
            p=$(printf '%s\n' "$out" | awk '/^x19:/{print $2}')
            o=$(printf '%s\n' "$out" | awk '/^x20:/{print $2}')
            [ "$p, $o" = "$want_po" ] && v=ok || v="want ($want_po) got ($p, $o), exit ${code:-none}"
        else
            [ "$code" = 0 ] && v="ok  iret=$iret" || v="exit ${code:-none} (want 0), d=$want_d"
        fi
        printf '%-8s %-16s %s\n' "$kind" "$state" "$v"
    done
done | tee "$work/results"

failed=$(grep -cvE '  ok( |$)' "$work/results")
echo "---- $(grep -cE '  ok( |$)' "$work/results") passed, $failed failed"
[ "$failed" -eq 0 ]
