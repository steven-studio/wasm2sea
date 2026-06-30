typedef double DATA_TYPE;

void kernel_trmm(int m, int n, DATA_TYPE alpha, DATA_TYPE *A, DATA_TYPE *B)
    __attribute__((export_name("kernel_trmm")));

void kernel_trmm(int m, int n, DATA_TYPE alpha, DATA_TYPE *A, DATA_TYPE *B) {
    int i, j, k;
    for (i = 0; i < m; i++)
        for (j = 0; j < n; j++) {
            for (k = i+1; k < m; k++)
                B[i*n+j] += A[k*m+i] * B[k*n+j];
            B[i*n+j] = alpha * B[i*n+j];
        }
}
