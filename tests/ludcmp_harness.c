#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_ludcmp(uintptr_t __mem, int32_t n, int32_t A_off, int32_t b_off, int32_t x_off, int32_t y_off);

static void ref_ludcmp(int n, double *A, double *b, double *x, double *y) {
    int i, j, k;
    double w;
    for (i = 0; i < n; i++) {
        for (j = 0; j < i; j++) {
            w = A[i*n+j];
            for (k = 0; k < j; k++) w -= A[i*n+k]*A[k*n+j];
            A[i*n+j] = w / A[j*n+j];
        }
        for (j = i; j < n; j++) {
            w = A[i*n+j];
            for (k = 0; k < i; k++) w -= A[i*n+k]*A[k*n+j];
            A[i*n+j] = w;
        }
    }
    for (i = 0; i < n; i++) {
        w = b[i];
        for (j = 0; j < i; j++) w -= A[i*n+j]*y[j];
        y[i] = w;
    }
    for (i = n-1; i >= 0; i--) {
        w = y[i];
        for (j = i+1; j < n; j++) w -= A[i*n+j]*x[j];
        x[i] = w / A[i*n+i];
    }
}

int main() {
    int n=4;
    int base = 131072;
    int A_off=base, b_off=A_off+n*n*8, x_off=b_off+n*8, y_off=x_off+n*8;

    double *A=(double*)(wasm_memory+A_off), *b=(double*)(wasm_memory+b_off);
    double *x=(double*)(wasm_memory+x_off), *y=(double*)(wasm_memory+y_off);
    double refA[16], refb[4], refx[4], refy[4];

    // diagonally dominant to keep pivots well-conditioned
    for (int i=0;i<n;i++) for (int j=0;j<n;j++) {
        double v = (i==j) ? (n*3.0) : (1.0/(i+j+2));
        A[i*n+j]=v; refA[i*n+j]=v;
    }
    for (int i=0;i<n;i++){b[i]=i+1.0; refb[i]=i+1.0;}
    for (int i=0;i<n;i++){x[i]=0.0; refx[i]=0.0;}
    for (int i=0;i<n;i++){y[i]=0.0; refy[i]=0.0;}

    kernel_ludcmp((uintptr_t)wasm_memory, n, A_off, b_off, x_off, y_off);
    ref_ludcmp(n, refA, refb, refx, refy);

    int pass=1;
    for (int i=0;i<n;i++) {
        if (fabs(x[i]-refx[i])>1e-6){pass=0;printf("MISMATCH x[%d]: %.6f ref=%.6f\n",i,x[i],refx[i]);}
    }
    if (pass) printf("PASS ludcmp differential test\n");
    else printf("FAIL\n");
    return 0;
}
