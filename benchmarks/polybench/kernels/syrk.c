typedef double DATA_TYPE;

void kernel_syrk(int n, int m, DATA_TYPE alpha, DATA_TYPE beta,
                 DATA_TYPE *C, DATA_TYPE *A)
    __attribute__((export_name("kernel_syrk")));

void kernel_syrk(int n, int m, DATA_TYPE alpha, DATA_TYPE beta,
                 DATA_TYPE *C, DATA_TYPE *A) {
    int i, j, k;
    for (i = 0; i < n; i++) {
        for (j = 0; j <= i; j++)
            C[i*n+j] *= beta;
        for (k = 0; k < m; k++)
            for (j = 0; j <= i; j++)
                C[i*n+j] += alpha * A[i*m+k] * A[j*m+k];
    }
}
