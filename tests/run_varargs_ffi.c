#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ffi.h>

extern int32_t test();

int main(int argc, char* argv[]) {
    int num_args = argc - 1;
    
    // 準備參數
    int32_t args[num_args];
    for (int i = 0; i < num_args; i++) {
        args[i] = atoi(argv[i + 1]);
    }
    
    // 準備 FFI 類型
    ffi_type *arg_types[num_args];
    void *arg_values[num_args];
    for (int i = 0; i < num_args; i++) {
        arg_types[i] = &ffi_type_sint32;
        arg_values[i] = &args[i];
    }
    
    // 準備 CIF（Call Interface）
    ffi_cif cif;
    ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, num_args,
                                     &ffi_type_sint32, arg_types);
    
    if (status != FFI_OK) {
        fprintf(stderr, "ffi_prep_cif failed\n");
        return 1;
    }
    
    // 調用函數
    int32_t result;
    ffi_call(&cif, FFI_FN(test), &result, arg_values);
    
    printf("%d\n", result);
    return 0;
}