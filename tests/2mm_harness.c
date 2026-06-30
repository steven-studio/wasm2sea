#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_2mm(uintptr_t __mem, int32_t ni, int32_t nj, int32_t nk, int32_t nl,
                double alpha, double beta,
                int32_t tmp_off, int32_t A_off, int32_t B_off,
                int32_t C_off, int32_t D_off);

int main() {
    int ni=4, nj=4, nk=4, nl=4;
    double alpha=1.5, beta=0.5;
    int base = 65536;
    int tmp_off = base + 256;
    int A_off   = tmp_off + ni*nj*8;
    int B_off   = A_off   + ni*nk*8;
    int C_off   = B_off   + nk*nj*8;
    int D_off   = C_off   + nj*nl*8;

    double *tmp = (double*)(wasm_memory + tmp_off);
    double *A   = (double*)(wasm_memory + A_off);
    double *B   = (double*)(wasm_memory + B_off);
    double *C   = (double*)(wasm_memory + C_off);
    double *D   = (double*)(wasm_memory + D_off);

    double reftmp[16], refA[16], refB[16], refC[16], refD[16];

    for (int i = 0; i < ni*nj; i++) { tmp[i]=0; reftmp[i]=0; }
    for (int i = 0; i < ni*nk; i++) { A[i]=i+1.0; refA[i]=i+1.0; }
    for (int i = 0; i < nk*nj; i++) { B[i]=i+1.0; refB[i]=i+1.0; }
    for (int i = 0; i < nj*nl; i++) { C[i]=i+1.0; refC[i]=i+1.0; }
    for (int i = 0; i < ni*nl; i++) { D[i]=i+1.0; refD[i]=i+1.0; }

    kernel_2mm((uintptr_t)wasm_memory, ni, nj, nk, nl, alpha, beta,
               tmp_off, A_off, B_off, C_off, D_off);

    // ref
    for (int i=0;i<ni;i++) for (int j=0;j<nj;j++) {
        reftmp[i*nj+j]=0;
        for (int k=0;k<nk;k++) reftmp[i*nj+j]+=alpha*refA[i*nk+k]*refB[k*nj+j];
    }
    for (int i=0;i<ni;i++) for (int j=0;j<nl;j++) {
        refD[i*nl+j]*=beta;
        for (int k=0;k<nj;k++) refD[i*nl+j]+=reftmp[i*nj+k]*refC[k*nl+j];
    }

    int pass=1;
    for (int i=0;i<ni*nl;i++) {
        if (fabs(D[i]-refD[i])>1e-6) {
            pass=0;
            printf("MISMATCH D[%d]: %.6f ref=%.6f\n",i,D[i],refD[i]);
        }
    }
    if (pass) printf("PASS 2mm differential test\n");
    else       printf("FAIL\n");
    return 0;
}
