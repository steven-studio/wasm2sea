#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_correlation(uintptr_t __mem, int32_t m, int32_t n, double float_n,
                        int32_t data_off, int32_t corr_off,
                        int32_t mean_off, int32_t stddev_off);

static void ref_correlation(int m, int n, double float_n,
                            double *data, double *corr, double *mean, double *stddev) {
    int i, j, k;
    double eps = 0.1;
    for (j = 0; j < m; j++) {
        mean[j] = 0.0;
        for (i = 0; i < n; i++) mean[j] += data[i*m+j];
        mean[j] /= float_n;
    }
    for (j = 0; j < m; j++) {
        stddev[j] = 0.0;
        for (i = 0; i < n; i++) stddev[j] += (data[i*m+j]-mean[j])*(data[i*m+j]-mean[j]);
        stddev[j] /= float_n;
        stddev[j] = sqrt(stddev[j]);
        stddev[j] = stddev[j] <= eps ? 1.0 : stddev[j];
    }
    for (i = 0; i < n; i++)
        for (j = 0; j < m; j++) {
            data[i*m+j] -= mean[j];
            data[i*m+j] /= sqrt(float_n)*stddev[j];
        }
    for (i = 0; i < m-1; i++) {
        corr[i*m+i] = 1.0;
        for (j = i+1; j < m; j++) {
            corr[i*m+j] = 0.0;
            for (k = 0; k < n; k++) corr[i*m+j] += data[k*m+i]*data[k*m+j];
            corr[j*m+i] = corr[i*m+j];
        }
    }
    corr[(m-1)*m+(m-1)] = 1.0;
}

int main() {
    int m=4, n=4;
    double float_n = (double)n;
    int base = 131072;
    int data_off = base, corr_off = data_off + n*m*8;
    int mean_off = corr_off + m*m*8, stddev_off = mean_off + m*8;

    double *data=(double*)(wasm_memory+data_off), *corr=(double*)(wasm_memory+corr_off);
    double *mean=(double*)(wasm_memory+mean_off), *stddev=(double*)(wasm_memory+stddev_off);
    double refdata[16], refcorr[16], refmean[4], refstddev[4];

    for (int i=0;i<n*m;i++){double v=(i%5)+1.0; data[i]=v; refdata[i]=v;}

    kernel_correlation((uintptr_t)wasm_memory, m, n, float_n, data_off, corr_off, mean_off, stddev_off);
    ref_correlation(m, n, float_n, refdata, refcorr, refmean, refstddev);

    int pass=1;
    for (int i=0;i<m*m;i++) {
        if (fabs(corr[i]-refcorr[i])>1e-6){pass=0;printf("MISMATCH corr[%d]: %.6f ref=%.6f\n",i,corr[i],refcorr[i]);}
    }
    if (pass) printf("PASS correlation differential test\n");
    else printf("FAIL\n");
    return 0;
}
