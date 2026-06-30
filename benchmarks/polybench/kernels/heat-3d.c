typedef double DATA_TYPE;

void kernel_heat_3d(int tsteps, int n, DATA_TYPE *A, DATA_TYPE *B) {
    int t, i, j, k;
    for (t = 1; t <= tsteps; t++) {
        for (i = 1; i < n-1; i++)
            for (j = 1; j < n-1; j++)
                for (k = 1; k < n-1; k++)
                    B[i*n*n+j*n+k] = 0.125 * (A[(i+1)*n*n+j*n+k] - 2.0*A[i*n*n+j*n+k] + A[(i-1)*n*n+j*n+k])
                                   + 0.125 * (A[i*n*n+(j+1)*n+k] - 2.0*A[i*n*n+j*n+k] + A[i*n*n+(j-1)*n+k])
                                   + 0.125 * (A[i*n*n+j*n+(k+1)] - 2.0*A[i*n*n+j*n+k] + A[i*n*n+j*n+(k-1)])
                                   + A[i*n*n+j*n+k];
        for (i = 1; i < n-1; i++)
            for (j = 1; j < n-1; j++)
                for (k = 1; k < n-1; k++)
                    A[i*n*n+j*n+k] = 0.125 * (B[(i+1)*n*n+j*n+k] - 2.0*B[i*n*n+j*n+k] + B[(i-1)*n*n+j*n+k])
                                   + 0.125 * (B[i*n*n+(j+1)*n+k] - 2.0*B[i*n*n+j*n+k] + B[i*n*n+(j-1)*n+k])
                                   + 0.125 * (B[i*n*n+j*n+(k+1)] - 2.0*B[i*n*n+j*n+k] + B[i*n*n+j*n+(k-1)])
                                   + B[i*n*n+j*n+k];
    }
}
