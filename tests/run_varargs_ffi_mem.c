#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ffi.h>

extern int32_t test();

static uint8_t wasm_memory[65536];

int main(int argc, char* argv[]) {
    int num_args = argc - 1;
    
    void* mem_ptr = wasm_memory;
    int32_t args[num_args];
    for (int i = 0; i < num_args; i++)
        args[i] = atoi(argv[i + 1]);
    
    ffi_type *arg_types[num_args + 1];
    void *arg_values[num_args + 1];
    
    // 第一個參數是 wasm_memory
    arg_types[0] = &ffi_type_pointer;
    arg_values[0] = &mem_ptr;
    
    for (int i = 0; i < num_args; i++) {
        arg_types[i + 1] = &ffi_type_sint32;
        arg_values[i + 1] = &args[i];
    }
    
    ffi_cif cif;
    ffi_prep_cif(&cif, FFI_DEFAULT_ABI, num_args + 1,
                 &ffi_type_sint32, arg_types);
    
    int32_t result;
    ffi_call(&cif, FFI_FN(test), &result, arg_values);
    
    printf("%d\n", result);
    return 0;
}