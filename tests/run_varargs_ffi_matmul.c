#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ffi.h>

extern int32_t test(void* mem, int32_t idx);

static uint8_t wasm_memory[65536];

int main(int argc, char* argv[]) {
    int32_t idx = (argc > 1) ? atoi(argv[1]) : 0;

    memset(wasm_memory, 0, sizeof(wasm_memory));

    void* mem_ptr = wasm_memory;

    ffi_type *arg_types[2];
    void *arg_values[2];
    arg_types[0] = &ffi_type_pointer;
    arg_values[0] = &mem_ptr;
    arg_types[1] = &ffi_type_sint32;
    arg_values[1] = &idx;

    ffi_cif cif;
    ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 2, &ffi_type_sint32, arg_types);

    int32_t result;
    ffi_call(&cif, FFI_FN(test), &result, arg_values);

    printf("%d\n", result);
    return 0;
}
