#!/usr/bin/env bash
#
# mayhem/build.sh — build wesnoth's fuzz harnesses and the core unit-test runner.
#
# 1) upstream's CMake build of the `wesnoth-common` static library (ENABLE_GAME=OFF: no SDL), once
#    with $SANITIZER_FLAGS $DEBUG_FLAGS (mayhem/build-fuzz) and once with normal flags for the tests
#    (mayhem/build-test). Both build dirs are gitignored (mayhem/.gitignore).
# 2) the few client/server sources the harnesses need beyond wesnoth-common (terrain translation,
#    colours, add-on validation), compiled with the same sanitizer flags into libfuzz-extra.a.
# 3) every harness in mayhem/harnesses twice: libFuzzer binary + -standalone reproducer, into
#    /mayhem/mayhem/bin (gitignored).
# 4) the unit-test runner from upstream's src/tests/{test_simple_wml,test_serialization,
#    test_version,test_lexical_cast}.cpp, run by mayhem/test.sh.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"
M="$SRC/mayhem"
BIN="$M/bin"
mkdir -p "$BIN"

# Upstream options for a core-only build. ENABLE_DISPLAY_REVISION=OFF: with ENABLE_GAME=OFF the
# generated revision.h is never made a dependency of wesnoth-common (upstream build quirk).
CMAKE_OPTS=(-DENABLE_GAME=OFF -DENABLE_SERVER=ON -DENABLE_CAMPAIGN_SERVER=OFF -DENABLE_TESTS=OFF
	-DENABLE_NLS=OFF -DENABLE_MYSQL=OFF -DHARDEN=OFF -DENABLE_DISPLAY_REVISION=OFF
	-DENABLE_DESKTOP_ENTRY=OFF -DENABLE_APPDATA_FILE=OFF -DCMAKE_BUILD_TYPE=Release)

# --- 1a) sanitized wesnoth-common. Upstream's CMakeLists overwrites CMAKE_CXX_FLAGS with its own
# warning set and appends CXX_FLAGS_USER, so the sanitizer/debug flags go through CXX_FLAGS_USER.
# -fsanitize=fuzzer-no-link: SanCov on the library and harness objects (LIB_FUZZING_ENGINE only
# instruments the TU it is linked with), unconditionally, also when SANITIZER_FLAGS is empty.
FUZZ_FLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link"
cmake -S "$SRC" -B "$M/build-fuzz" "${CMAKE_OPTS[@]}" \
	-DCXX_FLAGS_USER="$FUZZ_FLAGS" -DCMAKE_C_FLAGS="$FUZZ_FLAGS"
cmake --build "$M/build-fuzz" --target wesnoth-common -j"$MAYHEM_JOBS"

PANGO_CFLAGS="$(pkg-config --cflags pango)"
PANGO_LIBS="$(pkg-config --libs pango)"
SYS_LIBS=(-lboost_iostreams -lboost_locale -lboost_filesystem -lboost_regex -lboost_random
	-lboost_thread -lboost_program_options -lboost_coroutine -lboost_context
	-licui18n -licuuc -licudata -lssl -lcrypto -lbz2 -lz -lpthread -lm)
CXXSTD=(-std=c++17 -I"$SRC/src" -DBOOST_COROUTINES_NO_DEPRECATION_WARNING)

# --- 2) extra project sources the harnesses reach (outside libwesnoth_core), instrumented too.
# server_base.cpp is linked by both wesnothd and campaignd; among other things it supplies the
# servers' no-op deprecated_message() that config.cpp / preprocessor.cpp reference.
EXTRA_SRCS=(terrain/translation.cpp map/location.cpp color.cpp font/standard_colors.cpp
	gui/core/log.cpp addon/validation.cpp server/campaignd/addon_utils.cpp
	server/common/server_base.cpp)
OBJ="$M/build-fuzz/extra"
mkdir -p "$OBJ"
objs=()
for s in "${EXTRA_SRCS[@]}"; do
	o="$OBJ/$(echo "$s" | tr '/' '_').o"
	$CXX $FUZZ_FLAGS -O1 "${CXXSTD[@]}" -I"$M/build-fuzz/src" $PANGO_CFLAGS -c "$SRC/src/$s" -o "$o"
	objs+=("$o")
done
rm -f "$OBJ/libfuzz-extra.a"
ar rcs "$OBJ/libfuzz-extra.a" "${objs[@]}"

$CXX $FUZZ_FLAGS -c "$M/lsan_off.cc" -o "$OBJ/lsan_off.o"
$CC $FUZZ_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OBJ/standalone_main.o"

# --- 3) harnesses
for h in "$M"/harnesses/fuzz_*.cpp; do
	name="$(basename "$h" .cpp)"
	$CXX $FUZZ_FLAGS -O1 "${CXXSTD[@]}" $PANGO_CFLAGS -c "$h" -o "$OBJ/$name.o"
	link=("$OBJ/$name.o" "$OBJ/lsan_off.o"
		-Wl,--start-group "$OBJ/libfuzz-extra.a" "$M/build-fuzz/src/libwesnoth-common.a" -Wl,--end-group
		$PANGO_LIBS "${SYS_LIBS[@]}")
	$CXX $FUZZ_FLAGS $LIB_FUZZING_ENGINE "${link[@]}" -o "$BIN/$name"
	$CXX $FUZZ_FLAGS "$OBJ/standalone_main.o" "${link[@]}" -o "$BIN/$name-standalone"
done

# --- 4) unit tests, normal upstream flags (+ optional COVERAGE_FLAGS), no sanitizers.
cmake -S "$SRC" -B "$M/build-test" "${CMAKE_OPTS[@]}" \
	-DCXX_FLAGS_USER="$COVERAGE_FLAGS" -DCMAKE_C_FLAGS="$COVERAGE_FLAGS"
cmake --build "$M/build-test" --target wesnoth-common -j"$MAYHEM_JOBS"
TESTS=(test_simple_wml.cpp test_serialization.cpp test_version.cpp test_lexical_cast.cpp)
$CXX -O2 $COVERAGE_FLAGS "${CXXSTD[@]}" -c "$SRC/src/server/common/server_base.cpp" -o "$M/build-test/server_base.o"
tobjs=("$M/build-test/server_base.o")
for t in "${TESTS[@]}"; do
	o="$M/build-test/${t%.cpp}.o"
	$CXX -O2 $COVERAGE_FLAGS "${CXXSTD[@]}" -DBOOST_TEST_DYN_LINK -c "$SRC/src/tests/$t" -o "$o"
	tobjs+=("$o")
done
$CXX -O2 $COVERAGE_FLAGS "${CXXSTD[@]}" -DBOOST_TEST_DYN_LINK -c "$M/tests/test_main.cpp" -o "$M/build-test/test_main.o"
$CXX $COVERAGE_FLAGS "$M/build-test/test_main.o" "${tobjs[@]}" "$M/build-test/src/libwesnoth-common.a" \
	-lboost_unit_test_framework "${SYS_LIBS[@]}" -o "$BIN/wesnoth_core_tests"

echo "built: $(ls "$BIN" | tr '\n' ' ')"
