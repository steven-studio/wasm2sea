typedef double DATA_TYPE;

void kernel_gesummv(int n, DATA_TYPE alpha, DATA_TYPE beta,
                    DATA_TYPE *A, DATA_TYPE *B,
                    DATA_TYPE *tmp, DATA_TYPE *x, DATA_TYPE *y)
    __attribute__((export_name("kernel_gesummv")));

void kernel_gesummv(int n, DATA_TYPE alpha, DATA_TYPE beta,
                    DATA_TYPE *A, DATA_TYPE *B,
                    DATA_TYPE *tmp, DATA_TYPE *x, DATA_TYPE *y) {
    int i, j;
    for (i = 0; i < n; i++) {
        tmp[i] = 0.0;
        y[i]   = 0.0;
        for (j = 0; j < n; j++) {
            tmp[i] = A[i*n+j] * x[j] + tmp[i];
            y[i]   = B[i*n+j] * x[j] + y[i];
        }
        y[i] = alpha * tmp[i] + beta * y[i];
    }
}
