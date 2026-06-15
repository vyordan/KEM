#!/usr/bin/env bash
# Script de benchmark: KEM vs Python
# Uso desde la carpeta benchmarks/: bash run_benchmark.sh
# O desde cualquier lugar: bash /ruta/a/benchmarks/run_benchmark.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KEM_BIN="$SCRIPT_DIR/../build/cli/kem"
ITERATIONS=5

if [ ! -f "$KEM_BIN" ]; then
    echo "Error: compilá KEM primero:"
    echo "  cd $SCRIPT_DIR/.."
    echo "  cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release"
    echo "  cmake --build build"
    exit 1
fi

echo ""
echo "════════════════════════════════════════════"
echo "  KEM Benchmark Suite"
echo "════════════════════════════════════════════"
echo "  KEM: $($KEM_BIN --help 2>&1 | head -1 || echo 'kem')"
echo "  Iteraciones: $ITERATIONS"
echo ""

# Medir tiempo en microsegundos
measure_us() {
    local cmd="$1"
    local start end elapsed
    start=$(date +%s%N)
    eval "$cmd" > /dev/null 2>&1
    end=$(date +%s%N)
    echo $(( (end - start) / 1000 ))
}

run_avg() {
    local label="$1"
    local cmd="$2"
    local total=0
    local ok=true
    for _ in $(seq 1 $ITERATIONS); do
        local t
        t=$(measure_us "$cmd")
        if [ $? -ne 0 ]; then ok=false; break; fi
        total=$((total + t))
    done
    if $ok; then
        printf "  %-12s │ %8d µs  (avg $ITERATIONS runs)\n" \
               "$label" "$((total / ITERATIONS))"
    else
        printf "  %-12s │ N/A (error)\n" "$label"
    fi
}

echo "── Fibonacci(35) ──────────────────────────"
run_avg "KEM" "$KEM_BIN $SCRIPT_DIR/fibonacci.kem"
if command -v python3 &>/dev/null && [ -f "$SCRIPT_DIR/fibonacci.py" ]; then
    run_avg "Python 3" "python3 $SCRIPT_DIR/fibonacci.py"
fi
if command -v lua &>/dev/null && [ -f "$SCRIPT_DIR/fibonacci.lua" ]; then
    run_avg "Lua" "lua $SCRIPT_DIR/fibonacci.lua"
fi
echo ""

echo "── Tiempos internos del compilador KEM ────"
$KEM_BIN --benchmark "$SCRIPT_DIR/fibonacci.kem" 2>/dev/null || \
    $KEM_BIN --benchmark "$SCRIPT_DIR/fibonacci.kem"
echo "════════════════════════════════════════════"
