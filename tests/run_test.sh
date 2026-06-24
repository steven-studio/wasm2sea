#!/bin/bash

TEST=$1
shift

cd ~/wasm2sea/tests
wat2wasm ${TEST}.wat -o ${TEST}.wasm 2>/dev/null

cd ~/wasm2sea/build
./wasm2sea ../tests/${TEST}.wasm 2>/dev/null

cd ~/wasm2sea/third_party/dstogov-ir
./ir ../../build/${TEST}.ir --emit-c out.c 2>/dev/null

# 判斷是否需要 memory 參數
if [[ "$TEST" == *store* ]] || [[ "$TEST" == *load* ]]; then
    if [[ "$TEST" == i64_* ]]; then
        cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi_mem_i64.c -o a.out \
           $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
    else
        cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi_mem.c -o a.out \
           $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
    fi
elif [[ "$TEST" == i64_* ]]; then
    cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi_i64.c -o a.out \
       $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
elif [[ "$TEST" == i32_wrap_* ]]; then
    cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi_i64_ret_i32.c -o a.out \
       $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
elif [[ "$TEST" == i32_trunc_* ]]; then
    cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi_f64_ret_i32.c -o a.out \
       $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
elif [[ "$TEST" == i64_trunc_* ]]; then
    cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi_f64_ret_i64.c -o a.out \
       $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
elif [[ "$TEST" == f64_convert_i32_* ]]; then
    cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi_f64.c -o a.out \
       $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
elif [[ "$TEST" == f64_convert_i64_* ]]; then
    cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi_i64_ret_f64.c -o a.out \
       $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
elif [[ "$TEST" == matmul* ]]; then
    cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi_matmul.c -o a.out \
       $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
elif [[ "$TEST" == *nested_loop* ]] || [[ "$TEST" == matmul* ]]; then
    cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi_mem.c -o a.out \
       $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
elif [[ "$TEST" == bubble_sort* ]] || [[ "$TEST" == *sort* ]]; then
    cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi_sort.c -o a.out \
       $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
else
    cc -O2 -include stdint.h -include stdbool.h out.c run_varargs_ffi.c -o a.out \
       $(pkg-config --cflags --libs libffi) -lm -ldl -lpthread 2>/dev/null
fi

./a.out "$@"
