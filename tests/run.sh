#!/bin/sh
# Conformance runner for the Invisible Ink toolchain.
#
#   tests/run.sh [path-to-ink]     (default ../src/ink)
#
# Every case pins a semantics decision that the inkOS kernel inherits from
# this same core: wrapping arithmetic, step/limit accounting, the core-ABI
# syscalls 18/19, Quill `data`, the .inkb object format, Scribe strings,
# scheduler equivalence, and the CPU guards (stack limit, block nesting).
# Exit code, stdout (exact) and parseability are all checked; stderr is
# free-form and deliberately not compared.

INK="${1:-../src/ink}"
TMP=${TMPDIR:-/tmp}/ink-tests.$$
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT
cd "$(dirname "$0")" || exit 1
case "$INK" in
    /*) ;;
    *)  INK="$PWD/${1:-../src/ink}" ;;
esac

pass=0
fail=0

check() { # name expected_file actual_file want_rc got_rc
    ok=1
    cmp -s "$2" "$3" || ok=0
    [ "$4" = "$5" ] || ok=0
    if [ "$ok" = 1 ]; then
        pass=$((pass+1))
    else
        fail=$((fail+1))
        echo "FAIL: $1 (want exit $4, got $5)"
        if ! cmp -s "$2" "$3"; then
            echo "--- diff (expected vs actual):"
            diff "$2" "$3" | head -20 | sed 's/^/    /'
        fi
    fi
}

# helper: assemble a .q program once, run the .txt afterwards
prepq() { "$INK" assemble "$1.q" -o "$TMP/$1.txt" 2>"$TMP/$1.aerr"; }

# 01: wrapping arithmetic (all previously-UB cases, now defined)
prepq 01-wrap || { fail=$((fail+1)); echo "FAIL: assembling 01-wrap.q: $(cat "$TMP/01-wrap.aerr")"; }
"$INK" run "$TMP/01-wrap.txt" </dev/null >"$TMP/a.out" 2>"$TMP/a.err"; rc=$?
check "arithmetic wrapping" 01-wrap.expected "$TMP/a.out" 0 "$rc"

# 02: pow bills steps — huge exponent trips the limit instead of hanging
prepq 02-pow-limit || { fail=$((fail+1)); echo "FAIL: assembling 02-pow-limit.q"; }
"$INK" run "$TMP/02-pow-limit.txt" </dev/null >/dev/null 2>"$TMP/b.err"; rc=$?
case $(cat "$TMP/b.err") in *"step limit"*) okmsg=1;; *) okmsg=0;; esac
if [ "$rc" = 1 ] && [ "$okmsg" = 1 ]; then
    pass=$((pass+1))
else
    fail=$((fail+1)); echo "FAIL: pow step-accounting (rc=$rc, stderr: $(cat "$TMP/b.err"))"
fi

# 03: rand with n=1 is deterministic 0
prepq 03-rand || { fail=$((fail+1)); echo "FAIL: assembling 03-rand.q"; }
"$INK" run "$TMP/03-rand.txt" </dev/null >"$TMP/c.out" 2>/dev/null; rc=$?
check "rand n=1 -> 0" 03-rand.expected "$TMP/c.out" 0 "$rc"

# 04: core syscalls 18/19
prepq 04-sysmem || { fail=$((fail+1)); echo "FAIL: assembling 04-sysmem.q"; }
"$INK" run "$TMP/04-sysmem.txt" </dev/null >"$TMP/d.out" 2>/dev/null; rc=$?
check "core mem-set/mem-copy" 04-sysmem.expected "$TMP/d.out" 0 "$rc"

# 05: Quill data + strings
prepq 05-data || { fail=$((fail+1)); echo "FAIL: assembling 05-data.q"; }
"$INK" run "$TMP/05-data.txt" </dev/null >"$TMP/e.out" 2>"$TMP/e.err"; rc=$?
check "quill data sections" 05-data.expected "$TMP/e.out" 0 "$rc"

# 06: .inkb round-trip (run / obj+run / disassemble+assemble+run)
prepq 06-roundtrip || { fail=$((fail+1)); echo "FAIL: assembling 06-roundtrip.q"; }
"$INK" run "$TMP/06-roundtrip.txt" </dev/null >"$TMP/f.out" 2>/dev/null; rc=$?
check "round-trip: text" 06-roundtrip.expected "$TMP/f.out" 0 "$rc"

"$INK" obj "$TMP/06-roundtrip.txt" -o "$TMP/f.inkb" 2>"$TMP/f.err"; orc=$?
if [ "$orc" != 0 ]; then
    fail=$((fail+1)); echo "FAIL: ink obj build (stderr: $(cat "$TMP/f.err"))"
else
    "$INK" check "$TMP/f.inkb" >/dev/null 2>&1 || { fail=$((fail+1)); echo "FAIL: ink check on .inkb"; }
    "$INK" run "$TMP/f.inkb" </dev/null >"$TMP/f2.out" 2>/dev/null; rc=$?
    check "round-trip: obj run" 06-roundtrip.expected "$TMP/f2.out" 0 "$rc"
    # trailing-bytes strictness: append junk, must load-fail cleanly
    cp "$TMP/f.inkb" "$TMP/f-junk.inkb" && printf 'XX' >> "$TMP/f-junk.inkb"
    "$INK" run "$TMP/f-junk.inkb" </dev/null >/dev/null 2>"$TMP/f3.err"; rc=$?
    case $(cat "$TMP/f3.err") in *"trailing"*) jfail=0;; *) jfail=1;; esac
    if [ "$rc" != 0 ] || [ "$jfail" != 0 ]; then
        pass=$((pass+1))
    else
        fail=$((fail+1)); echo "FAIL: trailing bytes accepted in .inkb"
    fi
    # disassemble the object -> Quill -> assemble -> run
    if "$INK" disassemble "$TMP/f.inkb" >"$TMP/f.dis.q" 2>/dev/null &&
       "$INK" assemble "$TMP/f.dis.q" -o "$TMP/f.dis.txt" 2>/dev/null &&
       "$INK" run "$TMP/f.dis.txt" </dev/null >"$TMP/f4.out" 2>/dev/null; then
        check "round-trip: disasm(obj)" 06-roundtrip.expected "$TMP/f4.out" 0 0
    else
        fail=$((fail+1)); echo "FAIL: disassemble/assemble of .inkb"
    fi
fi

# 07: Scribe (~, string globals, &address-of)
"$INK" scc 07-scribe.s -o "$TMP/g.q" 2>/dev/null && \
"$INK" assemble "$TMP/g.q" -o "$TMP/g.txt" 2>/dev/null && \
"$INK" run "$TMP/g.txt" </dev/null >"$TMP/g.out" 2>/dev/null; rc=$?
check "scribe ~, strings, &" 07-scribe.expected "$TMP/g.out" 0 "$rc"

# 08: stack-limit guard (needs --stack; runaway must fault, exit 1)
prepq 08-stack || { fail=$((fail+1)); echo "FAIL: assembling 08-stack.q"; }
"$INK" run "$TMP/08-stack.txt" --stack 16 </dev/null >/dev/null 2>"$TMP/h.err"; rc=$?
case $(cat "$TMP/h.err") in *"stack limit"*) ok=1;; *) ok=0;; esac
if [ "$rc" = 1 ] && [ "$ok" = 1 ]; then
    pass=$((pass+1))
else
    fail=$((fail+1)); echo "FAIL: stack limit (rc=$rc, stderr: $(cat "$TMP/h.err"))"
fi

# 09: unknown syscalls return -1
prepq 09-unknown-sys || { fail=$((fail+1)); echo "FAIL: assembling 09-unknown-sys.q"; }
"$INK" run "$TMP/09-unknown-sys.txt" </dev/null >"$TMP/i.out" 2>/dev/null; rc=$?
check "unknown syscalls -> -1" 09-unknown-sys.expected "$TMP/i.out" 0 "$rc"

# 10: embedded program (cover prose) runs identically
"$INK" embed 10-embed.q --text "prose goes here. rain falls. %s %d" >"$TMP/j.txt" 2>/dev/null && \
"$INK" run "$TMP/j.txt" </dev/null >"$TMP/j.out" 2>/dev/null; rc=$?
check "embedded program" 10-embed.expected "$TMP/j.out" 0 "$rc"

# 11: scheduler equivalence (text and .inkb under sched)
"$INK" assemble 11-sched.q -o "$TMP/k.txt" 2>/dev/null
"$INK" sched "$TMP/k.txt" </dev/null >"$TMP/k.out" 2>/dev/null; rc=$?
check "sched: text" 11-sched.expected "$TMP/k.out" 0 "$rc"
"$INK" obj 11-sched.q -o "$TMP/k.inkb" 2>/dev/null &&
"$INK" sched "$TMP/k.inkb" </dev/null >"$TMP/k2.out" 2>/dev/null; rc=$?
check "sched: .inkb" 11-sched.expected "$TMP/k2.out" 0 "$rc"

# 12: block-nesting guard — under the cap runs, over the cap faults cleanly
{
    printf 'word main\n'
    i=0
    while [ "$i" -lt 800 ]; do printf 'push 1 if {\n'; i=$((i+1)); done
    i=0
    while [ "$i" -lt 800 ]; do printf 'push 10 char-out\n}\n'; i=$((i+1)); done
} >"$TMP/m-deep.q"
"$INK" assemble "$TMP/m-deep.q" -o "$TMP/m-deep.txt" 2>/dev/null &&
"$INK" run "$TMP/m-deep.txt" </dev/null >"$TMP/m.out" 2>/dev/null; rc=$?
i=0
while [ "$i" -lt 800 ]; do printf '\n'; i=$((i+1)); done >"$TMP/m.expected"
check "block nesting: 800 ok" "$TMP/m.expected" "$TMP/m.out" 0 "$rc"

{
    printf 'word main\n'
    i=0
    while [ "$i" -lt 1100 ]; do printf 'push 1 if {\n'; i=$((i+1)); done
    printf 'push 10 char-out\n'
    i=0
    while [ "$i" -lt 1100 ]; do printf '}\n'; i=$((i+1)); done
} >"$TMP/m2.q"
"$INK" assemble "$TMP/m2.q" -o "$TMP/m2.txt" 2>/dev/null &&
"$INK" run "$TMP/m2.txt" </dev/null >/dev/null 2>"$TMP/m2.err"; rc=$?
case $(cat "$TMP/m2.err") in *"block nesting"*) ok=1;; *) ok=0;; esac
if [ "$rc" = 1 ] && [ "$ok" = 1 ]; then
    pass=$((pass+1))
else
    fail=$((fail+1)); echo "FAIL: block-nesting guard (rc=$rc, stderr: $(cat "$TMP/m2.err"))"
fi

# summary
echo "$pass passed, $fail failed"
[ "$fail" = 0 ]
