typedef double DATA_TYPE;

void kernel_2mm(int ni, int nj, int nk, int nl,
                DATA_TYPE alpha, DATA_TYPE beta,
                DATA_TYPE *tmp, DATA_TYPE *A, DATA_TYPE *B,
                DATA_TYPE *C, DATA_TYPE *D)
    __attribute__((export_name("kernel_2mm")));

void kernel_2mm(int ni, int nj, int nk, int nl,
                DATA_TYPE alpha, DATA_TYPE beta,
                DATA_TYPE *tmp, DATA_TYPE *A, DATA_TYPE *B,
                DATA_TYPE *C, DATA_TYPE *D) {
    int i, j, k;
    for (i = 0; i < ni; i++)
        for (j = 0; j < nj; j++) {
            tmp[i*nj+j] = 0.0;
            for (k = 0; k < nk; k++)
                tmp[i*nj+j] += alpha * A[i*nk+k] * B[k*nj+j];
        }
    for (i = 0; i < ni; i++)
        for (j = 0; j < nl; j++) {
            D[i*nl+j] *= beta;
            for (k = 0; k < nj; k++)
                D[i*nl+j] += tmp[i*nj+k] * C[k*nl+j];
        }
}
