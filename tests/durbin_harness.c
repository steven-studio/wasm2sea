#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_durbin(uintptr_t __mem, int32_t n, int32_t r_off, int32_t y_off);

static void ref_durbin(int n, double *r, double *y) {
    double z[64], alpha, beta, sum;
    int i, k;
    y[0] = -r[0];
    beta = 1.0;
    alpha = -r[0];
    for (k = 1; k < n; k++) {
        beta = (1 - alpha*alpha) * beta;
        sum = 0.0;
        for (i = 0; i < k; i++) sum += r[k-i-1]*y[i];
        alpha = -(r[k] + sum)/beta;
        for (i = 0; i < k; i++) z[i] = y[i] + alpha*y[k-i-1];
        for (i = 0; i < k; i++) y[i] = z[i];
        y[k] = alpha;
    }
}

int main() {
    int n=4;
    int base=131072;
    int r_off=base, y_off=r_off+n*8;
    double *r=(double*)(wasm_memory+r_off), *y=(double*)(wasm_memory+y_off);
    double refr[4], refy[4];

    // small values to keep beta from going non-positive
    for (int i=0;i<n;i++){double v=0.1*(i+1); r[i]=v; refr[i]=v;}
    for (int i=0;i<n;i++){y[i]=0.0; refy[i]=0.0;}

    kernel_durbin((uintptr_t)wasm_memory,n,r_off,y_off);
    ref_durbin(n, refr, refy);

    int pass=1;
    for (int i=0;i<n;i++) {
        if (fabs(y[i]-refy[i])>1e-6){pass=0;printf("MISMATCH y[%d]: %.6f ref=%.6f\n",i,y[i],refy[i]);}
    }
    if (pass) printf("PASS durbin differential test\n");
    else printf("FAIL\n");
    return 0;
}
