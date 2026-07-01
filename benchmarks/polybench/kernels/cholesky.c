typedef double DATA_TYPE;
double sqrt(double);

void kernel_cholesky(int n, DATA_TYPE *A)
    __attribute__((export_name("kernel_cholesky")));

void kernel_cholesky(int n, DATA_TYPE *A) {
    int i, j, k;
    for (i = 0; i < n; i++) {
        for (j = 0; j < i; j++) {
            for (k = 0; k < j; k++)
                A[i*n+j] -= A[i*n+k] * A[j*n+k];
            A[i*n+j] /= A[j*n+j];
        }
        for (k = 0; k < i; k++)
            A[i*n+i] -= A[i*n+k] * A[i*n+k];
        A[i*n+i] = sqrt(A[i*n+i]);
    }
}
