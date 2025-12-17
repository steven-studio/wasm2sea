#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: $0 <test_name> [args...]"
    echo "Example: $0 test_bitwise 12 10"
    exit 1
fi

TEST=$1
shift  # 移除第一个参数，剩下的是测试参数

set -e  # 遇到错误立即退出

echo "=== Step 1: Compile WAT to WASM ==="
cd ~/wasm2sea/tests
wat2wasm ${TEST}.wat -o ${TEST}.wasm

echo ""
echo "=== Step 2: Convert WASM to IR ==="
cd ~/wasm2sea/build
./wasm2sea ../tests/${TEST}.wasm

echo ""
echo "=== Step 3: Generate C code from IR ==="
cd ~/wasm2sea/third_party/dstogov-ir
./ir_main ../../build/out.ir --emit-c out.c

echo ""
echo "=== Step 4: Compile C code with libffi ==="
cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi.c -o a.out \
   $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread

echo ""
echo "=== Step 5: Execute test ==="
if [ $# -eq 0 ]; then
    echo "No arguments provided for test"
    ./a.out
else
    echo "Running: ./a.out $@"
    ./a.out "$@"
fi