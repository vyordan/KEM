#!/bin/bash
# Script de benchmark: KEM vs Python vs Lua
# Uso: ./run_benchmark.sh

KEM_BIN="../build/cli/kem"
ITERATIONS=5

if [ ! -f "$KEM_BIN" ]; then
    echo "Error: compilá KEM primero con: cmake --build ../build"
    exit 1
fi

echo "════════════════════════════════════════════"
echo "  KEM Benchmark Suite — Tesis"
echo "════════════════════════════════════════════"
echo ""

run_avg() {
    local label="$1"
    local cmd="$2"
    local total=0
    for i in $(seq 1 $ITERATIONS); do
        local t=$(TIMEFORMAT='%R'; { time eval "$cmd" > /dev/null 2>&1; } 2>&1)
        local us=$(echo "$t * 1000000" | bc | cut -d. -f1)
        total=$((total + us))
    done
    local avg=$((total / ITERATIONS))
    echo "  $label: $avg µs (promedio de $ITERATIONS runs)"
}

echo "── Fibonacci(35) ──────────────────────────"
run_avg "KEM  " "$KEM_BIN fibonacci.kem"
[ -f "fibonacci.py" ] && run_avg "Python" "python3 fibonacci.py"
[ -f "fibonacci.lua" ] && run_avg "Lua  " "lua fibonacci.lua"
echo ""

echo "── Suma de arreglo(1000) ──────────────────"
run_avg "KEM  " "$KEM_BIN suma_arreglo.kem"
echo ""

echo "── Tiempos por fase del compilador ────────"
$KEM_BIN --benchmark fibonacci.kem
echo "════════════════════════════════════════════"
