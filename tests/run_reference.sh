#!/bin/bash
# =============================================================================
#  Tsyganator — render-reference regression test
# =============================================================================
#  Renders fixed scenarios through the current source tree and compares them,
#  sample for sample, against the blessed reference in tests/reference/.
#
#    ./tests/run_reference.sh              build + render + compare  (the test)
#    ./tests/run_reference.sh --selfcheck  render twice, prove determinism
#    ./tests/run_reference.sh --bless      accept current output as the new
#                                          reference (only after you have
#                                          LISTENED and agreed the change)
#
#  Uses its own build dir (build-tests/). The plugin build in build/ is never
#  touched, and TSYG_BUILD_TESTS defaults to OFF so ./BUILD.sh is unaffected.
# =============================================================================
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build-tests"
OUT="$ROOT/tests/out"
REF="$ROOT/tests/reference"
BIN="$BUILD/tsyg_render_artefacts/tsyg_render"
MODE="${1:-compare}"

# ---------------------------------------------------------------- build ----
echo "▶ Building the render harness…"
cmake -B "$BUILD" -S "$ROOT" -DTSYG_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release > "$BUILD.configure.log" 2>&1 \
  || { echo "✗ CMake configure failed — see $BUILD.configure.log"; tail -20 "$BUILD.configure.log"; exit 2; }
cmake --build "$BUILD" --target tsyg_render --config Release --parallel > "$BUILD.build.log" 2>&1 \
  || { echo "✗ Build failed — see $BUILD.build.log"; tail -30 "$BUILD.build.log"; exit 2; }

# JUCE puts console apps in slightly different places per generator
if [ ! -x "$BIN" ]; then
  BIN="$(find "$BUILD" -name tsyg_render -type f -perm +111 2>/dev/null | head -1)"
fi
[ -x "$BIN" ] || { echo "✗ tsyg_render binary not found under $BUILD"; exit 2; }
echo "  binary: $BIN"

# ------------------------------------------------------------- selfcheck ----
if [ "$MODE" = "--selfcheck" ]; then
  echo "▶ Determinism self-check (rendering twice)…"
  rm -rf "$OUT/_a" "$OUT/_b"; mkdir -p "$OUT/_a" "$OUT/_b"
  "$BIN" --render "$OUT/_a" > /dev/null
  "$BIN" --render "$OUT/_b" > /dev/null
  rc=0
  for f in "$OUT/_a"/*.f32; do
    n="$(basename "$f")"
    printf '  %-24s ' "${n%.f32}"
    "$BIN" --compare "$f" "$OUT/_b/$n" | head -1 || rc=1
  done
  rm -rf "$OUT/_a" "$OUT/_b"
  [ $rc -eq 0 ] && echo "✓ Renders are reproducible" || echo "✗ Renders are NOT reproducible"
  exit $rc
fi

# ---------------------------------------------------------------- render ----
echo "▶ Rendering scenarios…"
rm -rf "$OUT"; mkdir -p "$OUT"
"$BIN" --render "$OUT"

# ----------------------------------------------------------------- bless ----
if [ "$MODE" = "--bless" ]; then
  mkdir -p "$REF"
  cp "$OUT"/*.f32 "$OUT"/*.wav "$REF"/ 2>/dev/null || true
  ( cd "$REF" && shasum -a 256 *.f32 > CHECKSUMS.txt )
  echo "✓ Reference blessed in tests/reference/"
  cat "$REF/CHECKSUMS.txt"
  exit 0
fi

# --------------------------------------------------------------- compare ----
if [ ! -d "$REF" ] || [ -z "$(ls -A "$REF"/*.f32 2>/dev/null)" ]; then
  echo ""
  echo "✗ No reference yet. Review tests/out/*.wav, then run:"
  echo "    ./tests/run_reference.sh --bless"
  exit 3
fi

echo ""
echo "▶ Comparing against reference…"
FAIL=0
for f in "$REF"/*.f32; do
  n="$(basename "$f")"
  printf '  %-24s ' "${n%.f32}"
  if [ ! -f "$OUT/$n" ]; then echo "MISSING in output"; FAIL=1; continue; fi
  if ! "$BIN" --compare "$f" "$OUT/$n"; then FAIL=1; fi
done

echo ""
if [ $FAIL -eq 0 ]; then
  echo "✅ PASS — output is bit-identical to the reference."
else
  echo "⚠️  CHANGED — the DSP output differs from the reference."
  echo "   Intentional?  listen to tests/out/*.wav, then ./tests/run_reference.sh --bless"
  echo "   Unintentional? git diff, or roll back: git reset --hard v1.0-baseline"
fi
exit $FAIL
