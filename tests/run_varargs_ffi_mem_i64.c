#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ffi.h>

static uint8_t wasm_memory[65536];

extern int64_t test();

int main(int argc, char* argv[]) {
    int num_args = argc - 1;
    
    memset(wasm_memory, 0, sizeof(wasm_memory));
    void* mem_ptr = wasm_memory;

    int64_t args[num_args];
    for (int i = 0; i < num_args; i++)
        args[i] = atoll(argv[i + 1]);

    ffi_type *arg_types[num_args + 1];
    void *arg_values[num_args + 1];

    arg_types[0] = &ffi_type_pointer;
    arg_values[0] = &mem_ptr;

    for (int i = 0; i < num_args; i++) {
        arg_types[i + 1] = &ffi_type_sint64;
        arg_values[i + 1] = &args[i];
    }

    ffi_cif cif;
    ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, num_args + 1,
                                     &ffi_type_sint64, arg_types);
    if (status != FFI_OK) {
        fprintf(stderr, "ffi_prep_cif failed\n");
        return 1;
    }

    int64_t result;
    ffi_call(&cif, FFI_FN(test), &result, arg_values);

    printf("%ld\n", result);
    return 0;
}
