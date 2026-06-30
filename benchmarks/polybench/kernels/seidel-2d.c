typedef double DATA_TYPE;

void kernel_seidel_2d(int tsteps, int n, DATA_TYPE *A) {
    int t, i, j;
    for (t = 0; t <= tsteps - 1; t++)
        for (i = 1; i <= n - 2; i++)
            for (j = 1; j <= n - 2; j++)
                A[i*n+j] = (A[(i-1)*n+(j-1)] + A[(i-1)*n+j] + A[(i-1)*n+(j+1)]
                           + A[i*n+(j-1)]     + A[i*n+j]     + A[i*n+(j+1)]
                           + A[(i+1)*n+(j-1)] + A[(i+1)*n+j] + A[(i+1)*n+(j+1)]) / 9.0;
}
