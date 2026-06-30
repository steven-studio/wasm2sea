#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[131072];
void kernel_gesummv(uintptr_t __mem, int32_t n, double alpha, double beta,
                    int32_t A_off, int32_t B_off, int32_t tmp_off,
                    int32_t x_off, int32_t y_off);

static void ref_gesummv(int n, double alpha, double beta,
                        double *A, double *B, double *tmp, double *x, double *y) {
    int i, j;
    for (i = 0; i < n; i++) {
        tmp[i] = 0.0; y[i] = 0.0;
        for (j = 0; j < n; j++) {
            tmp[i] = A[i*n+j] * x[j] + tmp[i];
            y[i]   = B[i*n+j] * x[j] + y[i];
        }
        y[i] = alpha * tmp[i] + beta * y[i];
    }
}

int main() {
    int n = 4;
    double alpha = 1.5, beta = 0.5;
    int base   = 65536;
    int A_off   = base + 256;
    int B_off   = A_off   + n*n*8;
    int tmp_off = B_off   + n*n*8;
    int x_off   = tmp_off + n*8;
    int y_off   = x_off   + n*8;

    double *A   = (double*)(wasm_memory + A_off);
    double *B   = (double*)(wasm_memory + B_off);
    double *tmp = (double*)(wasm_memory + tmp_off);
    double *x   = (double*)(wasm_memory + x_off);
    double *y   = (double*)(wasm_memory + y_off);

    double refA[16], refB[16], reftmp[4], refx[4], refy[4];

    for (int i = 0; i < n*n; i++) { A[i] = i+1.0; refA[i] = i+1.0; }
    for (int i = 0; i < n*n; i++) { B[i] = i+1.0; refB[i] = i+1.0; }
    for (int i = 0; i < n;   i++) { x[i] = i+1.0; refx[i] = i+1.0; }

    kernel_gesummv((uintptr_t)wasm_memory, n, alpha, beta,
                   A_off, B_off, tmp_off, x_off, y_off);
    ref_gesummv(n, alpha, beta, refA, refB, reftmp, refx, refy);

    int pass = 1;
    for (int i = 0; i < n; i++) {
        if (fabs(y[i] - refy[i]) > 1e-6) {
            pass = 0;
            printf("MISMATCH y[%d]: %.6f ref=%.6f\n", i, y[i], refy[i]);
        }
    }
    if (pass) printf("PASS gesummv differential test\n");
    else       printf("FAIL\n");
    return 0;
}
