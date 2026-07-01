typedef double DATA_TYPE;

void kernel_ludcmp(int n, DATA_TYPE *A, DATA_TYPE *b, DATA_TYPE *x, DATA_TYPE *y)
    __attribute__((export_name("kernel_ludcmp")));

void kernel_ludcmp(int n, DATA_TYPE *A, DATA_TYPE *b, DATA_TYPE *x, DATA_TYPE *y) {
    int i, j, k;
    DATA_TYPE w;

    for (i = 0; i < n; i++) {
        for (j = 0; j < i; j++) {
            w = A[i*n+j];
            for (k = 0; k < j; k++)
                w -= A[i*n+k] * A[k*n+j];
            A[i*n+j] = w / A[j*n+j];
        }
        for (j = i; j < n; j++) {
            w = A[i*n+j];
            for (k = 0; k < i; k++)
                w -= A[i*n+k] * A[k*n+j];
            A[i*n+j] = w;
        }
    }

    for (i = 0; i < n; i++) {
        w = b[i];
        for (j = 0; j < i; j++)
            w -= A[i*n+j] * y[j];
        y[i] = w;
    }

    for (i = n-1; i >= 0; i--) {
        w = y[i];
        for (j = i+1; j < n; j++)
            w -= A[i*n+j] * x[j];
        x[i] = w / A[i*n+i];
    }
}
