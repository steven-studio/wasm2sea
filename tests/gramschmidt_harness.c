#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_gramschmidt(uintptr_t __mem, int32_t m, int32_t n,
                        int32_t A_off, int32_t R_off, int32_t Q_off);

static void ref_gramschmidt(int m, int n, double *A, double *R, double *Q) {
    int i, j, k;
    double nrm;
    for (k = 0; k < n; k++) {
        nrm = 0.0;
        for (i = 0; i < m; i++) nrm += A[i*n+k]*A[i*n+k];
        R[k*n+k] = sqrt(nrm);
        for (i = 0; i < m; i++) Q[i*n+k] = A[i*n+k]/R[k*n+k];
        for (j = k+1; j < n; j++) {
            R[k*n+j] = 0.0;
            for (i = 0; i < m; i++) R[k*n+j] += Q[i*n+k]*A[i*n+j];
            for (i = 0; i < m; i++) A[i*n+j] = A[i*n+j] - Q[i*n+k]*R[k*n+j];
        }
    }
}

int main() {
    int m=4, n=4;
    int base = 131072;
    int A_off=base, R_off=A_off+m*n*8, Q_off=R_off+n*n*8;

    double *A=(double*)(wasm_memory+A_off), *R=(double*)(wasm_memory+R_off), *Q=(double*)(wasm_memory+Q_off);
    double refA[16], refR[16], refQ[16];

    for (int i=0;i<m*n;i++){double v=(i%4)+1.0; A[i]=v; refA[i]=v;}
    for (int i=0;i<n*n;i++){R[i]=0.0; refR[i]=0.0;}
    for (int i=0;i<m*n;i++){Q[i]=0.0; refQ[i]=0.0;}

    kernel_gramschmidt((uintptr_t)wasm_memory, m, n, A_off, R_off, Q_off);
    ref_gramschmidt(m, n, refA, refR, refQ);

    int pass=1;
    for (int i=0;i<m*n;i++) {
        if (fabs(Q[i]-refQ[i])>1e-6){pass=0;printf("MISMATCH Q[%d]: %.6f ref=%.6f\n",i,Q[i],refQ[i]);}
    }
    if (pass) printf("PASS gramschmidt differential test\n");
    else printf("FAIL\n");
    return 0;
}
