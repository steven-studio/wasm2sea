typedef double DATA_TYPE;

void kernel_gemm(int ni, int nj, int nk,
                 DATA_TYPE alpha, DATA_TYPE beta,
                 DATA_TYPE *C, DATA_TYPE *A, DATA_TYPE *B) {
    int i, j, k;
    for (i = 0; i < ni; i++) {
        for (j = 0; j < nj; j++)
            C[i*nj+j] *= beta;
        for (k = 0; k < nk; k++)
            for (j = 0; j < nj; j++)
                C[i*nj+j] += alpha * A[i*nk+k] * B[k*nj+j];
    }
}
