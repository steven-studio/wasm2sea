typedef double DATA_TYPE;

void kernel_trisolv(int n, DATA_TYPE *L, DATA_TYPE *x, DATA_TYPE *b)
    __attribute__((export_name("kernel_trisolv")));

void kernel_trisolv(int n, DATA_TYPE *L, DATA_TYPE *x, DATA_TYPE *b) {
    int i, j;
    for (i = 0; i < n; i++) {
        x[i] = b[i];
        for (j = 0; j < i; j++)
            x[i] -= L[i*n+j] * x[j];
        x[i] = x[i] / L[i*n+i];
    }
}
