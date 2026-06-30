#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[131072];
void kernel_gemm(uintptr_t __mem, int32_t ni, int32_t nj, int32_t nk,
                 double alpha, double beta,
                 int32_t C_off, int32_t A_off, int32_t B_off);

static void ref_gemm(int ni, int nj, int nk,
                     double alpha, double beta,
                     double *C, double *A, double *B) {
    int i, j, k;
    for (i = 0; i < ni; i++) {
        for (j = 0; j < nj; j++)
            C[i*nj+j] *= beta;
        for (k = 0; k < nk; k++)
            for (j = 0; j < nj; j++)
                C[i*nj+j] += alpha * A[i*nk+k] * B[k*nj+j];
    }
}

int main() {
    int ni = 4, nj = 4, nk = 4;
    double alpha = 1.5, beta = 0.5;
    int C_off = 0;
    int A_off = ni * nj * 8;
    int B_off = A_off + ni * nk * 8;

    double *C = (double*)(wasm_memory + C_off);
    double *A = (double*)(wasm_memory + A_off);
    double *B = (double*)(wasm_memory + B_off);

    double refC[16], refA[16], refB[16];
    for (int i = 0; i < ni*nj; i++) { C[i] = i + 1.0; refC[i] = i + 1.0; }
    for (int i = 0; i < ni*nk; i++) { A[i] = i + 1.0; refA[i] = i + 1.0; }
    for (int i = 0; i < nk*nj; i++) { B[i] = i + 1.0; refB[i] = i + 1.0; }

    kernel_gemm((uintptr_t)wasm_memory, ni, nj, nk, alpha, beta,
                C_off, A_off, B_off);
    ref_gemm(ni, nj, nk, alpha, beta, refC, refA, refB);

    int pass = 1;
    for (int i = 0; i < ni*nj; i++) {
        if (fabs(C[i] - refC[i]) > 1e-6) {
            pass = 0;
            printf("MISMATCH at i=%d: C=%.6f ref=%.6f\n", i, C[i], refC[i]);
        }
    }

    if (pass)
        printf("PASS gemm differential test\n");
    else
        printf("FAIL\n");
    return 0;
}
