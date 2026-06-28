#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ffi.h>

extern int32_t test();

static uint8_t wasm_memory[65536];

int main(int argc, char* argv[]) {
    // argv[1] = n (array length)
    // argv[2..n+1] = array elements
    if (argc < 2) {
        fprintf(stderr, "Usage: %s n a0 a1 ... an-1\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);

    // 把 array 寫進 wasm_memory
    memset(wasm_memory, 0, sizeof(wasm_memory));
    for (int i = 0; i < n; i++) {
        int32_t val = atoi(argv[i + 2]);
        memcpy(&wasm_memory[i * 4], &val, sizeof(int32_t));
    }

    void* mem_ptr = wasm_memory;
    int32_t n_arg = n;

    ffi_type *arg_types[2];
    void *arg_values[2];

    arg_types[0] = &ffi_type_pointer;
    arg_values[0] = &mem_ptr;
    arg_types[1] = &ffi_type_sint32;
    arg_values[1] = &n_arg;

    ffi_cif cif;
    ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 2,
                 &ffi_type_sint32, arg_types);

    int32_t result;
    ffi_call(&cif, FFI_FN(test), &result, arg_values);

    printf("%d\n", result);
    return 0;
}
