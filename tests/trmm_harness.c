#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_trmm(uintptr_t __mem, int32_t m, int32_t n, double alpha,
                 int32_t A_off, int32_t B_off);

int main() {
    int m=4, n=4;
    double alpha=1.5;
    int base=131072;
    int A_off=base, B_off=A_off+m*m*8;

    double *A=(double*)(wasm_memory+A_off), *B=(double*)(wasm_memory+B_off);
    double refA[16], refB[16];

    for (int i=0;i<m*m;i++){A[i]=i+1.0;refA[i]=i+1.0;}
    for (int i=0;i<m*n;i++){B[i]=i+1.0;refB[i]=i+1.0;}

    kernel_trmm((uintptr_t)wasm_memory,m,n,alpha,A_off,B_off);

    for (int i=0;i<m;i++) for (int j=0;j<n;j++) {
        for (int k=i+1;k<m;k++) refB[i*n+j]+=refA[k*m+i]*refB[k*n+j];
        refB[i*n+j]=alpha*refB[i*n+j];
    }

    int pass=1;
    for (int i=0;i<m*n;i++) {
        if (fabs(B[i]-refB[i])>1e-6){pass=0;printf("MISMATCH B[%d]: %.6f ref=%.6f\n",i,B[i],refB[i]);}
    }
    if (pass) printf("PASS trmm differential test\n");
    else printf("FAIL\n");
    return 0;
}
