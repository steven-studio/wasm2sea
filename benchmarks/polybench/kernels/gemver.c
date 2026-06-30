typedef double DATA_TYPE;

void kernel_gemver(int n, DATA_TYPE alpha, DATA_TYPE beta,
                   DATA_TYPE *A, DATA_TYPE *u1, DATA_TYPE *v1,
                   DATA_TYPE *u2, DATA_TYPE *v2, DATA_TYPE *w,
                   DATA_TYPE *x, DATA_TYPE *y, DATA_TYPE *z) {
    int i, j;
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            A[i*n+j] = A[i*n+j] + u1[i]*v1[j] + u2[i]*v2[j];
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            x[i] = x[i] + beta * A[j*n+i] * y[j];
    for (i = 0; i < n; i++)
        x[i] = x[i] + z[i];
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            w[i] = w[i] + alpha * A[i*n+j] * x[j];
}
