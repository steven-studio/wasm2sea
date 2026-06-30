#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_trisolv(uintptr_t __mem, int32_t n, int32_t L_off, int32_t x_off, int32_t b_off);

int main() {
    int n=4;
    int base=131072;
    int L_off=base, x_off=L_off+n*n*8, b_off=x_off+n*8;

    double *L=(double*)(wasm_memory+L_off), *x=(double*)(wasm_memory+x_off), *b=(double*)(wasm_memory+b_off);
    double refL[16], refx[4], refb[4];

    // diagonally dominant so L[i][i] won't be zero
    for (int i=0;i<n;i++) for (int j=0;j<n;j++) {
        double v = (i==j) ? (n+1.0) : (i+j+1.0);
        L[i*n+j]=v; refL[i*n+j]=v;
    }
    for (int i=0;i<n;i++){x[i]=0.0; refx[i]=0.0;}
    for (int i=0;i<n;i++){b[i]=i+1.0; refb[i]=i+1.0;}

    kernel_trisolv((uintptr_t)wasm_memory,n,L_off,x_off,b_off);

    for (int i=0;i<n;i++) {
        refx[i]=refb[i];
        for (int j=0;j<i;j++) refx[i]-=refL[i*n+j]*refx[j];
        refx[i]=refx[i]/refL[i*n+i];
    }

    int pass=1;
    for (int i=0;i<n;i++) {
        if (fabs(x[i]-refx[i])>1e-6){pass=0;printf("MISMATCH x[%d]: %.6f ref=%.6f\n",i,x[i],refx[i]);}
    }
    if (pass) printf("PASS trisolv differential test\n");
    else printf("FAIL\n");
    return 0;
}
