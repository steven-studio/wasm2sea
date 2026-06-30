#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_3mm(uintptr_t __mem, int32_t ni, int32_t nj, int32_t nk, int32_t nl, int32_t nm,
                int32_t E_off, int32_t A_off, int32_t B_off,
                int32_t F_off, int32_t C_off, int32_t D_off, int32_t G_off);

int main() {
    int ni=4,nj=4,nk=4,nl=4,nm=4;
    int base=131072;
    int E_off=base, A_off=E_off+ni*nj*8, B_off=A_off+ni*nk*8;
    int F_off=B_off+nk*nj*8, C_off=F_off+nj*nl*8, D_off=C_off+nj*nm*8;
    int G_off=D_off+nm*nl*8;

    double *E=(double*)(wasm_memory+E_off), *A=(double*)(wasm_memory+A_off);
    double *B=(double*)(wasm_memory+B_off), *F=(double*)(wasm_memory+F_off);
    double *C=(double*)(wasm_memory+C_off), *D=(double*)(wasm_memory+D_off);
    double *G=(double*)(wasm_memory+G_off);
    double refE[16],refA[16],refB[16],refF[16],refC[16],refD[16],refG[16];

    for (int i=0;i<ni*nk;i++){A[i]=i+1.0;refA[i]=i+1.0;}
    for (int i=0;i<nk*nj;i++){B[i]=i+1.0;refB[i]=i+1.0;}
    for (int i=0;i<nj*nm;i++){C[i]=i+1.0;refC[i]=i+1.0;}
    for (int i=0;i<nm*nl;i++){D[i]=i+1.0;refD[i]=i+1.0;}

    kernel_3mm((uintptr_t)wasm_memory,ni,nj,nk,nl,nm,
               E_off,A_off,B_off,F_off,C_off,D_off,G_off);

    for (int i=0;i<ni;i++) for (int j=0;j<nj;j++) {
        refE[i*nj+j]=0; for (int k=0;k<nk;k++) refE[i*nj+j]+=refA[i*nk+k]*refB[k*nj+j];
    }
    for (int i=0;i<nj;i++) for (int j=0;j<nl;j++) {
        refF[i*nl+j]=0; for (int k=0;k<nm;k++) refF[i*nl+j]+=refC[i*nm+k]*refD[k*nl+j];
    }
    for (int i=0;i<ni;i++) for (int j=0;j<nl;j++) {
        refG[i*nl+j]=0; for (int k=0;k<nj;k++) refG[i*nl+j]+=refE[i*nj+k]*refF[k*nl+j];
    }

    int pass=1;
    for (int i=0;i<ni*nl;i++) {
        if (fabs(G[i]-refG[i])>1e-6){pass=0;printf("MISMATCH G[%d]: %.6f ref=%.6f\n",i,G[i],refG[i]);}
    }
    if (pass) printf("PASS 3mm differential test\n");
    else printf("FAIL\n");
    return 0;
}
