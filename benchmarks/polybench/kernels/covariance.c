typedef double DATA_TYPE;

void kernel_covariance(int m, int n, DATA_TYPE float_n,
                       DATA_TYPE *data, DATA_TYPE *cov, DATA_TYPE *mean)
    __attribute__((export_name("kernel_covariance")));

void kernel_covariance(int m, int n, DATA_TYPE float_n,
                       DATA_TYPE *data, DATA_TYPE *cov, DATA_TYPE *mean) {
    int i, j, k;
    for (j = 0; j < m; j++) {
        mean[j] = 0.0;
        for (i = 0; i < n; i++)
            mean[j] += data[i*m+j];
        mean[j] /= float_n;
    }
    for (i = 0; i < n; i++)
        for (j = 0; j < m; j++)
            data[i*m+j] -= mean[j];
    for (i = 0; i < m; i++)
        for (j = i; j < m; j++) {
            cov[i*m+j] = 0.0;
            for (k = 0; k < n; k++)
                cov[i*m+j] += data[k*m+i] * data[k*m+j];
            cov[i*m+j] /= (float_n - 1.0);
            cov[j*m+i] = cov[i*m+j];
        }
}
