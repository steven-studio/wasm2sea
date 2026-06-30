typedef double DATA_TYPE;

void kernel_mvt(int n, DATA_TYPE *x1, DATA_TYPE *x2, DATA_TYPE *y_1, DATA_TYPE *y_2, DATA_TYPE *A) {
    int i, j;
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            x1[i] = x1[i] + A[i*n+j] * y_1[j];
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            x2[i] = x2[i] + A[j*n+i] * y_2[j];
}
