#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_covariance(uintptr_t __mem, int32_t m, int32_t n, double float_n,
                       int32_t data_off, int32_t cov_off, int32_t mean_off);

static void ref_covariance(int m, int n, double float_n,
                           double *data, double *cov, double *mean) {
    int i, j, k;
    for (j = 0; j < m; j++) {
        mean[j] = 0.0;
        for (i = 0; i < n; i++) mean[j] += data[i*m+j];
        mean[j] /= float_n;
    }
    for (i = 0; i < n; i++)
        for (j = 0; j < m; j++) data[i*m+j] -= mean[j];
    for (i = 0; i < m; i++)
        for (j = i; j < m; j++) {
            cov[i*m+j] = 0.0;
            for (k = 0; k < n; k++) cov[i*m+j] += data[k*m+i]*data[k*m+j];
            cov[i*m+j] /= (float_n - 1.0);
            cov[j*m+i] = cov[i*m+j];
        }
}

int main() {
    int m=4, n=4;
    double float_n = (double)n;
    int base = 131072;
    int data_off = base, cov_off = data_off + n*m*8, mean_off = cov_off + m*m*8;

    double *data=(double*)(wasm_memory+data_off), *cov=(double*)(wasm_memory+cov_off), *mean=(double*)(wasm_memory+mean_off);
    double refdata[16], refcov[16], refmean[4];

    for (int i=0;i<n*m;i++){data[i]=i+1.0; refdata[i]=i+1.0;}

    kernel_covariance((uintptr_t)wasm_memory, m, n, float_n, data_off, cov_off, mean_off);
    ref_covariance(m, n, float_n, refdata, refcov, refmean);

    int pass=1;
    for (int i=0;i<m*m;i++) {
        if (fabs(cov[i]-refcov[i])>1e-6){pass=0;printf("MISMATCH cov[%d]: %.6f ref=%.6f\n",i,cov[i],refcov[i]);}
    }
    if (pass) printf("PASS covariance differential test\n");
    else printf("FAIL\n");
    return 0;
}
