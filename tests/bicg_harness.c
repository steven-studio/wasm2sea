#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[131072];
void kernel_bicg(uintptr_t __mem, int32_t m, int32_t n,
                 int32_t A_off, int32_t s_off, int32_t q_off, int32_t p_off, int32_t r_off);

static void ref_bicg(int m, int n, double *A, double *s, double *q, double *p, double *r) {
    int i, j;
    for (i = 0; i < m; i++)
        s[i] = 0;
    for (i = 0; i < n; i++) {
        q[i] = 0.0;
        for (j = 0; j < m; j++) {
            s[j] = s[j] + r[i] * A[i*m+j];
            q[i] = q[i] + A[i*m+j] * p[j];
        }
    }
}

int main() {
    int m = 4, n = 4;
    int A_off = 0;
    int s_off = n*m*8;
    int q_off = s_off + m*8;
    int p_off = q_off + n*8;
    int r_off = p_off + m*8;

    double *A = (double*)(wasm_memory + A_off);
    double *s = (double*)(wasm_memory + s_off);
    double *q = (double*)(wasm_memory + q_off);
    double *p = (double*)(wasm_memory + p_off);
    double *r = (double*)(wasm_memory + r_off);

    double refA[16], refs[4], refq[4], refp[4], refr[4];
    for (int i = 0; i < n*m; i++) { A[i] = i+1.0; refA[i] = i+1.0; }
    for (int i = 0; i < m;   i++) { s[i] = 0.0;   refs[i] = 0.0; }
    for (int i = 0; i < n;   i++) { q[i] = 0.0;   refq[i] = 0.0; }
    for (int i = 0; i < m;   i++) { p[i] = i+1.0; refp[i] = i+1.0; }
    for (int i = 0; i < n;   i++) { r[i] = i+1.0; refr[i] = i+1.0; }

    kernel_bicg((uintptr_t)wasm_memory, m, n, A_off, s_off, q_off, p_off, r_off);
    ref_bicg(m, n, refA, refs, refq, refp, refr);

    int pass = 1;
    for (int i = 0; i < m; i++) {
        if (fabs(s[i] - refs[i]) > 1e-6) {
            pass = 0;
            printf("MISMATCH s[%d]: %.6f ref=%.6f\n", i, s[i], refs[i]);
        }
    }
    for (int i = 0; i < n; i++) {
        if (fabs(q[i] - refq[i]) > 1e-6) {
            pass = 0;
            printf("MISMATCH q[%d]: %.6f ref=%.6f\n", i, q[i], refq[i]);
        }
    }

    if (pass)
        printf("PASS bicg differential test\n");
    else
        printf("FAIL\n");
    return 0;
}
