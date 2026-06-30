typedef double DATA_TYPE;
double sqrt(double);

void kernel_correlation(int m, int n, DATA_TYPE float_n,
                        DATA_TYPE *data, DATA_TYPE *corr,
                        DATA_TYPE *mean, DATA_TYPE *stddev)
    __attribute__((export_name("kernel_correlation")));

void kernel_correlation(int m, int n, DATA_TYPE float_n,
                        DATA_TYPE *data, DATA_TYPE *corr,
                        DATA_TYPE *mean, DATA_TYPE *stddev) {
    int i, j, k;
    DATA_TYPE eps = 0.1;

    for (j = 0; j < m; j++) {
        mean[j] = 0.0;
        for (i = 0; i < n; i++)
            mean[j] += data[i*m+j];
        mean[j] /= float_n;
    }

    for (j = 0; j < m; j++) {
        stddev[j] = 0.0;
        for (i = 0; i < n; i++)
            stddev[j] += (data[i*m+j]-mean[j]) * (data[i*m+j]-mean[j]);
        stddev[j] /= float_n;
        stddev[j] = sqrt(stddev[j]);
        stddev[j] = stddev[j] <= eps ? 1.0 : stddev[j];
    }

    for (i = 0; i < n; i++)
        for (j = 0; j < m; j++) {
            data[i*m+j] -= mean[j];
            data[i*m+j] /= sqrt(float_n) * stddev[j];
        }

    for (i = 0; i < m-1; i++) {
        corr[i*m+i] = 1.0;
        for (j = i+1; j < m; j++) {
            corr[i*m+j] = 0.0;
            for (k = 0; k < n; k++)
                corr[i*m+j] += (data[k*m+i] * data[k*m+j]);
            corr[j*m+i] = corr[i*m+j];
        }
    }
    corr[(m-1)*m+(m-1)] = 1.0;
}
