#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ffi.h>

extern int64_t test();

int main(int argc, char* argv[]) {
    int num_args = argc - 1;

    double args[num_args];
    for (int i = 0; i < num_args; i++)
        args[i] = atof(argv[i + 1]);

    ffi_type *arg_types[num_args];
    void *arg_values[num_args];

    for (int i = 0; i < num_args; i++) {
        arg_types[i] = &ffi_type_double;
        arg_values[i] = &args[i];
    }

    ffi_cif cif;
    ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, num_args,
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
