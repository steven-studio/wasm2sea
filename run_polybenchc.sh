#!/bin/bash

POLYBENCH_DIR=~/PolyBenchC
WASM2SEA_DIR=$HOME/wasm2sea
BUILD_DIR=$HOME/wasm2sea/build
IR_BIN=$HOME/wasm2sea/third_party/dstogov-ir/ir
KERNELS_DIR=$HOME/wasm2sea/benchmarks/polybench/kernels

BENCHMARKS=(
  "jacobi-1d kernel_jacobi_1d"
  "jacobi-2d kernel_jacobi_2d"
  "gemm kernel_gemm"
  "seidel-2d kernel_seidel_2d"
  "heat-3d kernel_heat_3d"
  "atax kernel_atax"
  "bicg kernel_bicg"
  "mvt kernel_mvt"
  "gemver kernel_gemver"
  "gesummv kernel_gesummv"
  "2mm kernel_2mm"
  "3mm kernel_3mm"
  "trmm kernel_trmm"
  "syrk kernel_syrk"
  "syr2k kernel_syr2k"
  "symm kernel_symm"
  "trisolv kernel_trisolv"
  "durbin kernel_durbin"
  "lu kernel_lu"
  "covariance kernel_covariance"
  "correlation kernel_correlation"
  "fdtd-2d kernel_fdtd_2d"
  "adi kernel_adi"
  "ludcmp kernel_ludcmp"
  "cholesky kernel_cholesky"
  "gramschmidt kernel_gramschmidt"
  "floyd-warshall kernel_floyd_warshall"
  "nussinov kernel_nussinov"
  "deriche kernel_deriche"
)

PASS=0
FAIL=0

for entry in "${BENCHMARKS[@]}"; do
  NAME=$(echo $entry | cut -d' ' -f1)
  KERNEL=$(echo $entry | cut -d' ' -f2)

  echo "=== $NAME ==="

  # Step 1: clang → wasm
  clang --target=wasm32 \
    -O0 -nostdlib \
    -Wl,--no-entry \
    -Wl,--allow-undefined \
    -Wl,--export=$KERNEL \
    -o /tmp/${NAME}.wasm \
    ~/wasm2sea/benchmarks/polybench/kernels/${NAME}.c

  if [ $? -ne 0 ]; then
    echo "FAIL: $NAME (clang failed)"; FAIL=$((FAIL+1)); continue
  fi
  echo "  [1/4] clang → wasm OK"

  # Step 2: wasm2sea → .ir
  $BUILD_DIR/wasm2sea /tmp/${NAME}.wasm --save-ir /tmp/${NAME}.ir 2>/dev/null
  # ir 存到當前目錄，移到 /tmp
  mv $HOME/wasm2sea/${NAME}.ir /tmp/${NAME}.ir

  if [ $? -ne 0 ]; then
    echo "FAIL: $NAME (wasm2sea failed)"; FAIL=$((FAIL+1)); continue
  fi
  echo "  [2/4] wasm2sea → .ir OK"

  # Step 3: dstogov/ir → .c
  $IR_BIN /tmp/${NAME}.ir --emit-c /tmp/${NAME}_out.c 2>/dev/null

  if [ $? -ne 0 ]; then
    echo "FAIL: $NAME (ir emit-c failed)"; FAIL=$((FAIL+1)); continue
  fi
  echo "  [3/4] ir → .c OK"

  # Step 4: compile & run vs reference
  OUTC=/tmp/${NAME}_out.c
  HARNESS=$HOME/wasm2sea/tests/${NAME//-/_}_harness.c

  cp $HARNESS /tmp/${NAME}_harness.c
  sed -i "s/\b${KERNEL}\b/test/g" /tmp/${NAME}_harness.c

  cc -O2 -include stdint.h -include stdbool.h -include math.h \
    $OUTC /tmp/${NAME}_harness.c -o /tmp/${NAME}_bin -lm

  if [ $? -ne 0 ]; then
    echo "FAIL: $NAME (compile failed)"; FAIL=$((FAIL+1)); continue
  fi

  RESULT=$(/tmp/${NAME}_bin)
  if echo "$RESULT" | grep -q "PASS"; then
    echo "  [4/4] run OK"
    echo "PASS: $NAME"
    PASS=$((PASS+1))
  else
    echo "FAIL: $NAME (wrong output)"
    echo "$RESULT"
    FAIL=$((FAIL+1))
  fi

done

echo ""
echo "Results: $PASS passed, $FAIL failed"
