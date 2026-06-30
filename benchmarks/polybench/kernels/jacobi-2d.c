typedef double DATA_TYPE;

void kernel_jacobi_2d(int tsteps, int n, DATA_TYPE *A, DATA_TYPE *B) {
    int t, i, j;
    for (t = 0; t < tsteps; t++) {
        for (i = 1; i < n - 1; i++)
            for (j = 1; j < n - 1; j++)
                B[i*n+j] = 0.2 * (A[i*n+j] + A[i*n+(j-1)] + A[i*n+(1+j)] + A[(1+i)*n+j] + A[(i-1)*n+j]);
        for (i = 1; i < n - 1; i++)
            for (j = 1; j < n - 1; j++)
                A[i*n+j] = 0.2 * (B[i*n+j] + B[i*n+(j-1)] + B[i*n+(1+j)] + B[(1+i)*n+j] + B[(i-1)*n+j]);
    }
}
