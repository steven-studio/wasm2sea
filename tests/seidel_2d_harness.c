#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[131072];
void kernel_seidel_2d(uintptr_t __mem, int32_t tsteps, int32_t n, int32_t A_off);

static void ref_seidel_2d(int tsteps, int n, double *A) {
    int t, i, j;
    for (t = 0; t <= tsteps - 1; t++)
        for (i = 1; i <= n - 2; i++)
            for (j = 1; j <= n - 2; j++)
                A[i*n+j] = (A[(i-1)*n+(j-1)] + A[(i-1)*n+j] + A[(i-1)*n+(j+1)]
                           + A[i*n+(j-1)]     + A[i*n+j]     + A[i*n+(j+1)]
                           + A[(i+1)*n+(j-1)] + A[(i+1)*n+j] + A[(i+1)*n+(j+1)]) / 9.0;
}

int main() {
    int n = 6, tsteps = 2;
    int A_off = 0;
    double *A = (double*)(wasm_memory + A_off);

    double refA[36];
    for (int i = 0; i < n*n; i++) { A[i] = i + 1.0; refA[i] = i + 1.0; }

    kernel_seidel_2d((uintptr_t)wasm_memory, tsteps, n, A_off);
    ref_seidel_2d(tsteps, n, refA);

    int pass = 1;
    for (int i = 0; i < n*n; i++) {
        if (fabs(A[i] - refA[i]) > 1e-6) {
            pass = 0;
            printf("MISMATCH at i=%d: A=%.6f ref=%.6f\n", i, A[i], refA[i]);
        }
    }

    if (pass)
        printf("PASS seidel_2d differential test\n");
    else
        printf("FAIL\n");
    return 0;
}
