#!/usr/bin/env bash
# verificar_llvm_estatico.sh
#
# Corré este script DESPUÉS de que termine "ninja install" en tu
# compilación de LLVM estático, para confirmar que todo lo necesario
# está presente antes de intentar compilar KEM contra él.
#
# Uso:
#   bash verificar_llvm_estatico.sh $HOME/llvm-static-install

set -e

ROOT="$1"

if [ -z "$ROOT" ]; then
    echo "Uso: $0 <ruta-a-llvm-static-install>"
    echo "Ejemplo: $0 \$HOME/llvm-static-install"
    exit 1
fi

echo "════════════════════════════════════════════"
echo "  Verificando instalación de LLVM estático"
echo "  Ruta: $ROOT"
echo "════════════════════════════════════════════"
echo ""

fail=0

check_file() {
    if [ -f "$1" ]; then
        size=$(du -h "$1" | cut -f1)
        echo "  OK   $1  ($size)"
    else
        echo "  FALTA: $1"
        fail=1
    fi
}

echo "-- Binarios --"
check_file "$ROOT/bin/llvm-config"
echo ""

echo "-- Librerías estáticas críticas para KEM --"
check_file "$ROOT/lib/libLLVMCore.a"
check_file "$ROOT/lib/libLLVMOrcJIT.a"
check_file "$ROOT/lib/libLLVMSupport.a"
check_file "$ROOT/lib/libLLVMAnalysis.a"
check_file "$ROOT/lib/libLLVMPasses.a"
check_file "$ROOT/lib/libLLVMIRReader.a"
check_file "$ROOT/lib/libLLVMX86CodeGen.a"
check_file "$ROOT/lib/libLLVMX86Desc.a"
check_file "$ROOT/lib/libLLVMX86Info.a"
echo ""

echo "-- Headers --"
if [ -d "$ROOT/include/llvm" ]; then
    echo "  OK   $ROOT/include/llvm/"
else
    echo "  FALTA: $ROOT/include/llvm/"
    fail=1
fi
echo ""

echo "-- Versión reportada --"
if [ -x "$ROOT/bin/llvm-config" ]; then
    "$ROOT/bin/llvm-config" --version
fi
echo ""

if [ "$fail" -eq 0 ]; then
    echo "════════════════════════════════════════════"
    echo "  Todo presente. Podés compilar KEM con:"
    echo ""
    echo "  cmake -B build-static -G Ninja \\"
    echo "      -DLLVM_STATIC_ROOT=$ROOT \\"
    echo "      -DCMAKE_BUILD_TYPE=Release"
    echo "  cmake --build build-static"
    echo "════════════════════════════════════════════"
    exit 0
else
    echo "════════════════════════════════════════════"
    echo "  Faltan archivos. Revisá que ninja install"
    echo "  haya terminado sin errores."
    echo "════════════════════════════════════════════"
    exit 1
fi
