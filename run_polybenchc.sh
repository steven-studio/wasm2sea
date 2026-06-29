#!/bin/bash

POLYBENCH_DIR=~/PolyBenchC
WASM2SEA_DIR=$HOME/wasm2sea
BUILD_DIR=$HOME/wasm2sea/build
IR_BIN=$HOME/wasm2sea/third_party/dstogov-ir/ir
KERNELS_DIR=$HOME/wasm2sea/benchmarks/polybench/kernels

BENCHMARKS=(
  "stencils/jacobi-1d jacobi-1d kernel_jacobi_1d"
)

PASS=0
FAIL=0

for entry in "${BENCHMARKS[@]}"; do
  SUBDIR=$(echo $entry | cut -d' ' -f1)
  NAME=$(echo $entry | cut -d' ' -f2)
  KERNEL=$(echo $entry | cut -d' ' -f3)

  echo "=== $NAME ==="

  # Step 1: clang → wasm
  clang --target=wasm32 \
    -O0 -nostdlib \
    -Wl,--no-entry \
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

  # 自動偵測並補上 local_* 宣告
  UNDECL=$(grep -oP '\blocal_\d+\b' $OUTC | sort -u | tr '\n' ',' | sed 's/,$//')
  if [ -n "$UNDECL" ]; then
    sed -i "1s/^/int32_t ${UNDECL};\n/" $OUTC
  fi

  # 把 harness 的函數名換掉
  cp $HOME/wasm2sea/tests/jacobi_harness.c /tmp/${NAME}_harness.c
  sed -i 's/\bkernel_jacobi_1d\b/test/g' /tmp/${NAME}_harness.c

  cc -O2 -include stdint.h -include stdbool.h -include math.h \
    $OUTC /tmp/${NAME}_harness.c -o /tmp/${NAME}_bin -lm

  if [ $? -ne 0 ]; then
    echo "FAIL: $NAME (compile failed)"; FAIL=$((FAIL+1)); continue
  fi

  RESULT=$(/tmp/${NAME}_bin)
  if echo "$RESULT" | grep -q "PASS jacobi1d differential test"; then
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
