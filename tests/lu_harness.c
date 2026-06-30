#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_lu(uintptr_t __mem, int32_t n, int32_t A_off);

static void ref_lu(int n, double *A) {
    int i, j, k;
    for (i = 0; i < n; i++) {
        for (j = 0; j < i; j++) {
            for (k = 0; k < j; k++)
                A[i*n+j] -= A[i*n+k]*A[k*n+j];
            A[i*n+j] /= A[j*n+j];
        }
        for (j = i; j < n; j++) {
            for (k = 0; k < i; k++)
                A[i*n+j] -= A[i*n+k]*A[k*n+j];
        }
    }
}

int main() {
    int n=4;
    int base=131072;
    int A_off=base;
    double *A=(double*)(wasm_memory+A_off);
    double refA[16];

    // diagonally dominant so divisions are well-conditioned
    for (int i=0;i<n;i++) for (int j=0;j<n;j++) {
        double v = (i==j) ? (n*2.0) : (1.0/(i+j+2));
        A[i*n+j]=v; refA[i*n+j]=v;
    }

    kernel_lu((uintptr_t)wasm_memory,n,A_off);
    ref_lu(n, refA);

    int pass=1;
    for (int i=0;i<n*n;i++) {
        if (fabs(A[i]-refA[i])>1e-6){pass=0;printf("MISMATCH A[%d]: %.6f ref=%.6f\n",i,A[i],refA[i]);}
    }
    if (pass) printf("PASS lu differential test\n");
    else printf("FAIL\n");
    return 0;
}
