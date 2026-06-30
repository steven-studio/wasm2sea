#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[131072];
void kernel_jacobi_2d(uintptr_t __mem, int32_t tsteps, int32_t n, int32_t A_off, int32_t B_off);

static void ref_jacobi_2d(int tsteps, int n, double *A, double *B) {
    int t, i, j;
    for (t = 0; t < tsteps; t++) {
        for (i = 1; i < n - 1; i++)
            for (j = 1; j < n - 1; j++)
                B[i*n+j] = 0.2 * (A[i*n+j] + A[i*n+(j-1)] + A[i*n+(1+j)] + A[(1+i)*n+j] + A[(i-1)*n+j]);
        for (i = 1; i < n - 1; i++)
            for (j = 1; j < n - 1; j++)
                A[i*n+j] = 0.2 * (B[i*n+j] + B[i*n+(j-1)] + B[i*n+(1+j)] + B[(1+i)*n+j] + B[(i-1)*n+j]);
    }
}

int main() {
    int n = 6, tsteps = 2;
    int A_off = 0;
    int B_off = n * n * 8;
    double *A = (double*)(wasm_memory + A_off);
    double *B = (double*)(wasm_memory + B_off);

    double refA[36], refB[36];
    for (int i = 0; i < n*n; i++) { A[i] = (i % n) + 1.0; refA[i] = (i % n) + 1.0; }
    for (int i = 0; i < n*n; i++) { B[i] = 0.0; refB[i] = 0.0; }

    kernel_jacobi_2d((uintptr_t)wasm_memory, tsteps, n, A_off, B_off);
    ref_jacobi_2d(tsteps, n, refA, refB);

    int pass = 1;
    for (int i = 0; i < n*n; i++) {
        if (fabs(A[i] - refA[i]) > 1e-6 || fabs(B[i] - refB[i]) > 1e-6) {
            pass = 0;
            printf("MISMATCH at i=%d: A=%.6f ref=%.6f  B=%.6f ref=%.6f\n",
                   i, A[i], refA[i], B[i], refB[i]);
        }
    }

    if (pass)
        printf("PASS jacobi2d differential test\n");
    else
        printf("FAIL\n");
    return 0;
}
