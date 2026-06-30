typedef double DATA_TYPE;

void kernel_lu(int n, DATA_TYPE *A)
    __attribute__((export_name("kernel_lu")));

void kernel_lu(int n, DATA_TYPE *A) {
    int i, j, k;
    for (i = 0; i < n; i++) {
        for (j = 0; j < i; j++) {
            for (k = 0; k < j; k++)
                A[i*n+j] -= A[i*n+k] * A[k*n+j];
            A[i*n+j] /= A[j*n+j];
        }
        for (j = i; j < n; j++) {
            for (k = 0; k < i; k++)
                A[i*n+j] -= A[i*n+k] * A[k*n+j];
        }
    }
}
