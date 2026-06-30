typedef double DATA_TYPE;

void kernel_syr2k(int n, int m, DATA_TYPE alpha, DATA_TYPE beta,
                  DATA_TYPE *C, DATA_TYPE *A, DATA_TYPE *B)
    __attribute__((export_name("kernel_syr2k")));

void kernel_syr2k(int n, int m, DATA_TYPE alpha, DATA_TYPE beta,
                  DATA_TYPE *C, DATA_TYPE *A, DATA_TYPE *B) {
    int i, j, k;
    for (i = 0; i < n; i++) {
        for (j = 0; j <= i; j++)
            C[i*n+j] *= beta;
        for (k = 0; k < m; k++)
            for (j = 0; j <= i; j++)
                C[i*n+j] += A[j*m+k]*alpha*B[i*m+k] + B[j*m+k]*alpha*A[i*m+k];
    }
}
