#!/bin/bash

TEST=$1
shift

cd ~/wasm2sea/tests
wat2wasm ${TEST}.wat -o ${TEST}.wasm 2>/dev/null

cd ~/wasm2sea/build
./wasm2sea ../tests/${TEST}.wasm 2>/dev/null

cd ~/wasm2sea/third_party/dstogov-ir
./ir ../../build/${TEST}.ir --emit-c out.c 2>/dev/null

# 判斷是否為 i64 測試
if [[ "$TEST" == i64_* ]]; then
    cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi_i64.c -o a.out \
       $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
else
    cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi.c -o a.out \
       $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
fi

./a.out "$@"
