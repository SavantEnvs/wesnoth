#!/usr/bin/env bash
#
# mayhem/test.sh — RUN this repo's OWN functional test suite (already built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

# Upstream's own Boost.Test cases from src/tests/{test_simple_wml,test_serialization,test_version,
# test_lexical_cast}.cpp (the part of upstream's boost_unit_tests that needs only wesnoth-common),
# built by mayhem/build.sh with normal flags. Counts come from Boost.Test's own report, never the
# exit code: a runner that prints nothing is a failure.
RUNNER="$SRC/mayhem/bin/wesnoth_core_tests"
if [ ! -x "$RUNNER" ]; then
  echo "test runner $RUNNER missing — mayhem/build.sh did not build it" >&2
  emit_ctrf "boost-test" 0 1 0
  exit 1
fi

out="$("$RUNNER" --report_level=short --log_level=error 2>&1)"
rc=$?
echo "$out"
# "N test cases out of M passed" plus optional ", F failed" / ", S skipped"
summary="$(echo "$out" | grep -E '[0-9]+ test cases? out of [0-9]+ (passed|failed)' | head -1)"
if [ -z "$summary" ]; then
  echo "no Boost.Test summary in the runner output" >&2
  emit_ctrf "boost-test" 0 1 0
  exit 1
fi
total="$(echo "$summary" | sed -E 's/.*out of ([0-9]+).*/\1/')"
passed="$(echo "$summary" | grep -oE '[0-9]+ test cases? out of' | grep -oE '^[0-9]+')"
skipped="$(echo "$summary" | grep -oE '[0-9]+ skipped' | grep -oE '^[0-9]+' || true)"
: "${skipped:=0}"
failed=$(( total - passed - skipped ))
# A non-zero exit with no failed case counted (e.g. a crash mid-run) is still a failure.
if [ "$rc" -ne 0 ] && [ "$failed" -eq 0 ]; then failed=1; fi
emit_ctrf "boost-test" "$passed" "$failed" "$skipped"
