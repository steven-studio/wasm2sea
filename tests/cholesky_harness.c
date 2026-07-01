#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_cholesky(uintptr_t __mem, int32_t n, int32_t A_off);

static void ref_cholesky(int n, double *A) {
    int i, j, k;
    for (i = 0; i < n; i++) {
        for (j = 0; j < i; j++) {
            for (k = 0; k < j; k++) A[i*n+j] -= A[i*n+k]*A[j*n+k];
            A[i*n+j] /= A[j*n+j];
        }
        for (k = 0; k < i; k++) A[i*n+i] -= A[i*n+k]*A[i*n+k];
        A[i*n+i] = sqrt(A[i*n+i]);
    }
}

int main() {
    int n=4;
    int base = 131072;
    int A_off = base;
    double *A=(double*)(wasm_memory+A_off);
    double refA[16];

    // build a symmetric positive-definite matrix: A = B^T*B + n*I
    double B[16];
    for (int i=0;i<n*n;i++) B[i] = (i%3)+1.0;
    for (int i=0;i<n;i++) for (int j=0;j<n;j++) {
        double sum=0;
        for (int k=0;k<n;k++) sum += B[k*n+i]*B[k*n+j];
        if (i==j) sum += n*10.0;
        A[i*n+j]=sum; refA[i*n+j]=sum;
    }

    kernel_cholesky((uintptr_t)wasm_memory, n, A_off);
    ref_cholesky(n, refA);

    int pass=1;
    for (int i=0;i<n;i++) for (int j=0;j<=i;j++) {
        if (fabs(A[i*n+j]-refA[i*n+j])>1e-6){pass=0;printf("MISMATCH A[%d][%d]: %.6f ref=%.6f\n",i,j,A[i*n+j],refA[i*n+j]);}
    }
    if (pass) printf("PASS cholesky differential test\n");
    else printf("FAIL\n");
    return 0;
}
