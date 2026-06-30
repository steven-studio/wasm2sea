typedef double DATA_TYPE;

void kernel_atax(int m, int n, DATA_TYPE *A, DATA_TYPE *x, DATA_TYPE *y, DATA_TYPE *tmp) {
    int i, j;
    for (i = 0; i < n; i++)
        y[i] = 0;
    for (i = 0; i < m; i++) {
        tmp[i] = 0.0;
        for (j = 0; j < n; j++)
            tmp[i] = tmp[i] + A[i*n+j] * x[j];
        for (j = 0; j < n; j++)
            y[j] = y[j] + A[i*n+j] * tmp[i];
    }
}
