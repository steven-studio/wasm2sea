typedef double DATA_TYPE;

void kernel_symm(int m, int n, DATA_TYPE alpha, DATA_TYPE beta,
                 DATA_TYPE *C, DATA_TYPE *A, DATA_TYPE *B)
    __attribute__((export_name("kernel_symm")));

void kernel_symm(int m, int n, DATA_TYPE alpha, DATA_TYPE beta,
                 DATA_TYPE *C, DATA_TYPE *A, DATA_TYPE *B) {
    int i, j, k;
    DATA_TYPE temp2;
    for (i = 0; i < m; i++)
        for (j = 0; j < n; j++) {
            temp2 = 0;
            for (k = 0; k < i; k++) {
                C[k*n+j] += alpha*B[i*n+j] * A[i*m+k];
                temp2 += B[k*n+j] * A[i*m+k];
            }
            C[i*n+j] = beta * C[i*n+j] + alpha*B[i*n+j] * A[i*m+i] + alpha * temp2;
        }
}
