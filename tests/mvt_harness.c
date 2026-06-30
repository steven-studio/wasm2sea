#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[131072];
void kernel_mvt(uintptr_t __mem, int32_t n,
                int32_t x1_off, int32_t x2_off, int32_t y1_off, int32_t y2_off, int32_t A_off);

static void ref_mvt(int n, double *x1, double *x2, double *y_1, double *y_2, double *A) {
    int i, j;
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            x1[i] = x1[i] + A[i*n+j] * y_1[j];
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            x2[i] = x2[i] + A[j*n+i] * y_2[j];
}

int main() {
    int n = 4;
    int x1_off = 0;
    int x2_off = x1_off + n*8;
    int y1_off = x2_off + n*8;
    int y2_off = y1_off + n*8;
    int A_off  = y2_off + n*8;

    double *x1  = (double*)(wasm_memory + x1_off);
    double *x2  = (double*)(wasm_memory + x2_off);
    double *y_1 = (double*)(wasm_memory + y1_off);
    double *y_2 = (double*)(wasm_memory + y2_off);
    double *A   = (double*)(wasm_memory + A_off);

    double refx1[4], refx2[4], refy1[4], refy2[4], refA[16];
    for (int i = 0; i < n;   i++) { x1[i]  = i+1.0; refx1[i] = i+1.0; }
    for (int i = 0; i < n;   i++) { x2[i]  = i+1.0; refx2[i] = i+1.0; }
    for (int i = 0; i < n;   i++) { y_1[i] = i+1.0; refy1[i] = i+1.0; }
    for (int i = 0; i < n;   i++) { y_2[i] = i+1.0; refy2[i] = i+1.0; }
    for (int i = 0; i < n*n; i++) { A[i]   = i+1.0; refA[i]  = i+1.0; }

    kernel_mvt((uintptr_t)wasm_memory, n, x1_off, x2_off, y1_off, y2_off, A_off);
    ref_mvt(n, refx1, refx2, refy1, refy2, refA);

    int pass = 1;
    for (int i = 0; i < n; i++) {
        if (fabs(x1[i] - refx1[i]) > 1e-6) {
            pass = 0;
            printf("MISMATCH x1[%d]: %.6f ref=%.6f\n", i, x1[i], refx1[i]);
        }
        if (fabs(x2[i] - refx2[i]) > 1e-6) {
            pass = 0;
            printf("MISMATCH x2[%d]: %.6f ref=%.6f\n", i, x2[i], refx2[i]);
        }
    }

    if (pass)
        printf("PASS mvt differential test\n");
    else
        printf("FAIL\n");
    return 0;
}
