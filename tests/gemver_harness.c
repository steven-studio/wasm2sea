#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[131072];
void kernel_gemver(uintptr_t __mem, int32_t n, double alpha, double beta,
                   int32_t A_off, int32_t u1_off, int32_t v1_off,
                   int32_t u2_off, int32_t v2_off, int32_t w_off,
                   int32_t x_off, int32_t y_off, int32_t z_off);

static void ref_gemver(int n, double alpha, double beta,
                       double *A, double *u1, double *v1,
                       double *u2, double *v2, double *w,
                       double *x, double *y, double *z) {
    int i, j;
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            A[i*n+j] = A[i*n+j] + u1[i]*v1[j] + u2[i]*v2[j];
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            x[i] = x[i] + beta * A[j*n+i] * y[j];
    for (i = 0; i < n; i++)
        x[i] = x[i] + z[i];
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            w[i] = w[i] + alpha * A[i*n+j] * x[j];
}

int main() {
    int n = 4;
    double alpha = 1.5, beta = 0.5;
    int base = 65536;  // 給 stack 留 4096 bytes
    int A_off  = base + 256;
    int u1_off = A_off  + n*n*8;
    int v1_off = u1_off + n*8;
    int u2_off = v1_off + n*8;
    int v2_off = u2_off + n*8;
    int w_off  = v2_off + n*8;
    int x_off  = w_off  + n*8;
    int y_off  = x_off  + n*8;
    int z_off  = y_off  + n*8;

    double *A  = (double*)(wasm_memory + A_off);
    double *u1 = (double*)(wasm_memory + u1_off);
    double *v1 = (double*)(wasm_memory + v1_off);
    double *u2 = (double*)(wasm_memory + u2_off);
    double *v2 = (double*)(wasm_memory + v2_off);
    double *w  = (double*)(wasm_memory + w_off);
    double *x  = (double*)(wasm_memory + x_off);
    double *y  = (double*)(wasm_memory + y_off);
    double *z  = (double*)(wasm_memory + z_off);

    double refA[16], refu1[4], refv1[4], refu2[4], refv2[4];
    double refw[4], refx[4], refy[4], refz[4];

    for (int i = 0; i < n*n; i++) { A[i]  = i+1.0; refA[i]  = i+1.0; }
    for (int i = 0; i < n;   i++) { u1[i] = i+1.0; refu1[i] = i+1.0; }
    for (int i = 0; i < n;   i++) { v1[i] = i+1.0; refv1[i] = i+1.0; }
    for (int i = 0; i < n;   i++) { u2[i] = i+1.0; refu2[i] = i+1.0; }
    for (int i = 0; i < n;   i++) { v2[i] = i+1.0; refv2[i] = i+1.0; }
    for (int i = 0; i < n;   i++) { w[i]  = 0.0;   refw[i]  = 0.0; }
    for (int i = 0; i < n;   i++) { x[i]  = i+1.0; refx[i]  = i+1.0; }
    for (int i = 0; i < n;   i++) { y[i]  = i+1.0; refy[i]  = i+1.0; }
    for (int i = 0; i < n;   i++) { z[i]  = i+1.0; refz[i]  = i+1.0; }

    kernel_gemver((uintptr_t)wasm_memory, n, alpha, beta,
                  A_off, u1_off, v1_off, u2_off, v2_off,
                  w_off, x_off, y_off, z_off);
    ref_gemver(n, alpha, beta, refA, refu1, refv1, refu2, refv2,
               refw, refx, refy, refz);

    int pass = 1;
    for (int i = 0; i < n; i++) {
        if (fabs(w[i] - refw[i]) > 1e-6) {
            pass = 0;
            printf("MISMATCH w[%d]: %.6f ref=%.6f\n", i, w[i], refw[i]);
        }
        if (fabs(x[i] - refx[i]) > 1e-6) {
            pass = 0;
            printf("MISMATCH x[%d]: %.6f ref=%.6f\n", i, x[i], refx[i]);
        }
    }

    if (pass)
        printf("PASS gemver differential test\n");
    else
        printf("FAIL\n");
    return 0;
}
