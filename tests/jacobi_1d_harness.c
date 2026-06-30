#include <stdio.h>
#include <stdint.h>

static uint8_t wasm_memory[131072];
void kernel_jacobi_1d(uintptr_t __mem, int32_t tsteps, int32_t n, int32_t A_off, int32_t B_off);

static void ref_jacobi_1d(int tsteps, int n, double *A, double *B) {
    for (int t = 0; t < tsteps; t++) {
        for (int i = 1; i < n - 1; i++)
            B[i] = 0.33333 * (A[i-1] + A[i] + A[i+1]);
        for (int i = 1; i < n - 1; i++)
            A[i] = 0.33333 * (B[i-1] + B[i] + B[i+1]);
    }
}

int main() {
    int n = 8, tsteps = 2;
    // wasm_memory layout:
//
// +---------------------------------------------------------+
// |      A (n * sizeof(double))      |  B (n * sizeof(double)) |
// +---------------------------------------------------------+
// ^                                  ^
// |                                  |
// A_off = 0                          B_off = n * sizeof(double)
    int A_off = 0;
    int B_off = n * 8;
    double *A = (double*)(wasm_memory + A_off);
    double *B = (double*)(wasm_memory + B_off);

    // ref arrays
    double refA[8], refB[8];
    for (int i = 0; i < n; i++) { A[i] = i + 1.0; refA[i] = i + 1.0; }
    for (int i = 0; i < n; i++) { B[i] = 0.0;     refB[i] = 0.0; }

    kernel_jacobi_1d((uintptr_t)wasm_memory, tsteps, n, A_off, B_off);
    ref_jacobi_1d(tsteps, n, refA, refB);

    int pass = 1;
    for (int i = 0; i < n; i++) {
        if (fabs(A[i] - refA[i]) > 1e-6 || fabs(B[i] - refB[i]) > 1e-6) {
            pass = 0;
            printf("MISMATCH at i=%d: A=%.6f ref=%.6f  B=%.6f ref=%.6f\n",
                   i, A[i], refA[i], B[i], refB[i]);
        }
    }

    if (pass)
        printf("PASS jacobi1d differential test\n");
    else {
        printf("A: "); for (int i = 0; i < n; i++) printf("%.4f ", A[i]); printf("\n");
        printf("B: "); for (int i = 0; i < n; i++) printf("%.4f ", B[i]); printf("\n");
    }
    return 0;
}
