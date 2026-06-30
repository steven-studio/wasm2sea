#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_syr2k(uintptr_t __mem, int32_t n, int32_t m, double alpha, double beta,
                  int32_t C_off, int32_t A_off, int32_t B_off);

int main() {
    int n=4, m=4;
    double alpha=1.5, beta=0.5;
    int base=131072;
    int C_off=base, A_off=C_off+n*n*8, B_off=A_off+n*m*8;

    double *C=(double*)(wasm_memory+C_off), *A=(double*)(wasm_memory+A_off), *B=(double*)(wasm_memory+B_off);
    double refC[16], refA[16], refB[16];

    for (int i=0;i<n*n;i++){C[i]=i+1.0;refC[i]=i+1.0;}
    for (int i=0;i<n*m;i++){A[i]=i+1.0;refA[i]=i+1.0;}
    for (int i=0;i<n*m;i++){B[i]=i+1.0;refB[i]=i+1.0;}

    kernel_syr2k((uintptr_t)wasm_memory,n,m,alpha,beta,C_off,A_off,B_off);

    for (int i=0;i<n;i++) {
        for (int j=0;j<=i;j++) refC[i*n+j]*=beta;
        for (int k=0;k<m;k++)
            for (int j=0;j<=i;j++)
                refC[i*n+j]+=refA[j*m+k]*alpha*refB[i*m+k]+refB[j*m+k]*alpha*refA[i*m+k];
    }

    int pass=1;
    for (int i=0;i<n;i++) for (int j=0;j<=i;j++) {
        if (fabs(C[i*n+j]-refC[i*n+j])>1e-6){pass=0;printf("MISMATCH C[%d][%d]: %.6f ref=%.6f\n",i,j,C[i*n+j],refC[i*n+j]);}
    }
    if (pass) printf("PASS syr2k differential test\n");
    else printf("FAIL\n");
    return 0;
}
