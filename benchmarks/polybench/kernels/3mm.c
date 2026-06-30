typedef double DATA_TYPE;

void kernel_3mm(int ni, int nj, int nk, int nl, int nm,
                DATA_TYPE *E, DATA_TYPE *A, DATA_TYPE *B,
                DATA_TYPE *F, DATA_TYPE *C, DATA_TYPE *D, DATA_TYPE *G)
    __attribute__((export_name("kernel_3mm")));

void kernel_3mm(int ni, int nj, int nk, int nl, int nm,
                DATA_TYPE *E, DATA_TYPE *A, DATA_TYPE *B,
                DATA_TYPE *F, DATA_TYPE *C, DATA_TYPE *D, DATA_TYPE *G) {
    int i, j, k;
    for (i = 0; i < ni; i++)
        for (j = 0; j < nj; j++) {
            E[i*nj+j] = 0.0;
            for (k = 0; k < nk; k++)
                E[i*nj+j] += A[i*nk+k] * B[k*nj+j];
        }
    for (i = 0; i < nj; i++)
        for (j = 0; j < nl; j++) {
            F[i*nl+j] = 0.0;
            for (k = 0; k < nm; k++)
                F[i*nl+j] += C[i*nm+k] * D[k*nl+j];
        }
    for (i = 0; i < ni; i++)
        for (j = 0; j < nl; j++) {
            G[i*nl+j] = 0.0;
            for (k = 0; k < nj; k++)
                G[i*nl+j] += E[i*nj+k] * F[k*nl+j];
        }
}
