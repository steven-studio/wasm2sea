#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[131072];
void kernel_atax(uintptr_t __mem, int32_t m, int32_t n,
                 int32_t A_off, int32_t x_off, int32_t y_off, int32_t tmp_off);

static void ref_atax(int m, int n, double *A, double *x, double *y, double *tmp) {
    int i, j;
    for (i = 0; i < n; i++)
        y[i] = 0;
    for (i = 0; i < m; i++) {
        tmp[i] = 0.0;
        for (j = 0; j < n; j++)
            tmp[i] = tmp[i] + A[i*n+j] * x[j];
        for (j = 0; j < n; j++)
            y[j] = y[j] + A[i*n+j] * tmp[i];
    }
}

int main() {
    int m = 4, n = 4;
    int A_off   = 0;
    int x_off   = m*n*8;
    int y_off   = x_off + n*8;
    int tmp_off = y_off + n*8;

    double *A   = (double*)(wasm_memory + A_off);
    double *x   = (double*)(wasm_memory + x_off);
    double *y   = (double*)(wasm_memory + y_off);
    double *tmp = (double*)(wasm_memory + tmp_off);

    double refA[16], refx[4], refy[4], reftmp[4];
    for (int i = 0; i < m*n; i++) { A[i] = i + 1.0; refA[i] = i + 1.0; }
    for (int i = 0; i < n;   i++) { x[i] = i + 1.0; refx[i] = i + 1.0; }
    for (int i = 0; i < n;   i++) { y[i] = 0.0;     refy[i] = 0.0; }
    for (int i = 0; i < m;   i++) { tmp[i] = 0.0;   reftmp[i] = 0.0; }

    kernel_atax((uintptr_t)wasm_memory, m, n, A_off, x_off, y_off, tmp_off);
    ref_atax(m, n, refA, refx, refy, reftmp);

    int pass = 1;
    for (int i = 0; i < n; i++) {
        if (fabs(y[i] - refy[i]) > 1e-6) {
            pass = 0;
            printf("MISMATCH at i=%d: y=%.6f ref=%.6f\n", i, y[i], refy[i]);
        }
    }

    if (pass)
        printf("PASS atax differential test\n");
    else
        printf("FAIL\n");
    return 0;
}
