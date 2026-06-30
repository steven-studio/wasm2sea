#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_symm(uintptr_t __mem, int32_t m, int32_t n, double alpha, double beta,
                 int32_t C_off, int32_t A_off, int32_t B_off);

int main() {
    int m=4, n=4;
    double alpha=1.5, beta=0.5;
    int base=131072;
    int C_off=base, A_off=C_off+m*n*8, B_off=A_off+m*m*8;

    double *C=(double*)(wasm_memory+C_off), *A=(double*)(wasm_memory+A_off), *B=(double*)(wasm_memory+B_off);
    double refC[16], refA[16], refB[16];

    for (int i=0;i<m*n;i++){C[i]=i+1.0;refC[i]=i+1.0;}
    for (int i=0;i<m*m;i++){A[i]=i+1.0;refA[i]=i+1.0;}
    for (int i=0;i<m*n;i++){B[i]=i+1.0;refB[i]=i+1.0;}

    kernel_symm((uintptr_t)wasm_memory,m,n,alpha,beta,C_off,A_off,B_off);

    for (int i=0;i<m;i++)
        for (int j=0;j<n;j++) {
            double temp2=0;
            for (int k=0;k<i;k++) {
                refC[k*n+j]+=alpha*refB[i*n+j]*refA[i*m+k];
                temp2+=refB[k*n+j]*refA[i*m+k];
            }
            refC[i*n+j]=beta*refC[i*n+j]+alpha*refB[i*n+j]*refA[i*m+i]+alpha*temp2;
        }

    int pass=1;
    for (int i=0;i<m*n;i++) {
        if (fabs(C[i]-refC[i])>1e-6){pass=0;printf("MISMATCH C[%d]: %.6f ref=%.6f\n",i,C[i],refC[i]);}
    }
    if (pass) printf("PASS symm differential test\n");
    else printf("FAIL\n");
    return 0;
}
