#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[131072];
void kernel_heat_3d(uintptr_t __mem, int32_t tsteps, int32_t n, int32_t A_off, int32_t B_off);

static void ref_heat_3d(int tsteps, int n, double *A, double *B) {
    int t, i, j, k;
    for (t = 1; t <= tsteps; t++) {
        for (i = 1; i < n-1; i++)
            for (j = 1; j < n-1; j++)
                for (k = 1; k < n-1; k++)
                    B[i*n*n+j*n+k] = 0.125 * (A[(i+1)*n*n+j*n+k] - 2.0*A[i*n*n+j*n+k] + A[(i-1)*n*n+j*n+k])
                                   + 0.125 * (A[i*n*n+(j+1)*n+k] - 2.0*A[i*n*n+j*n+k] + A[i*n*n+(j-1)*n+k])
                                   + 0.125 * (A[i*n*n+j*n+(k+1)] - 2.0*A[i*n*n+j*n+k] + A[i*n*n+j*n+(k-1)])
                                   + A[i*n*n+j*n+k];
        for (i = 1; i < n-1; i++)
            for (j = 1; j < n-1; j++)
                for (k = 1; k < n-1; k++)
                    A[i*n*n+j*n+k] = 0.125 * (B[(i+1)*n*n+j*n+k] - 2.0*B[i*n*n+j*n+k] + B[(i-1)*n*n+j*n+k])
                                   + 0.125 * (B[i*n*n+(j+1)*n+k] - 2.0*B[i*n*n+j*n+k] + B[i*n*n+(j-1)*n+k])
                                   + 0.125 * (B[i*n*n+j*n+(k+1)] - 2.0*B[i*n*n+j*n+k] + B[i*n*n+j*n+(k-1)])
                                   + B[i*n*n+j*n+k];
    }
}

int main() {
    int n = 4, tsteps = 2;
    int A_off = 0;
    int B_off = n*n*n*8;
    double *A = (double*)(wasm_memory + A_off);
    double *B = (double*)(wasm_memory + B_off);

    double refA[64], refB[64];
    for (int i = 0; i < n*n*n; i++) { A[i] = i + 1.0; refA[i] = i + 1.0; }
    for (int i = 0; i < n*n*n; i++) { B[i] = 0.0;     refB[i] = 0.0; }

    kernel_heat_3d((uintptr_t)wasm_memory, tsteps, n, A_off, B_off);
    ref_heat_3d(tsteps, n, refA, refB);

    int pass = 1;
    for (int i = 0; i < n*n*n; i++) {
        if (fabs(A[i] - refA[i]) > 1e-6 || fabs(B[i] - refB[i]) > 1e-6) {
            pass = 0;
            printf("MISMATCH at i=%d: A=%.6f ref=%.6f  B=%.6f ref=%.6f\n",
                   i, A[i], refA[i], B[i], refB[i]);
        }
    }

    if (pass)
        printf("PASS heat_3d differential test\n");
    else
        printf("FAIL\n");
    return 0;
}
