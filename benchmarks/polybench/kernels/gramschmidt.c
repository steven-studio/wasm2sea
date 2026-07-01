typedef double DATA_TYPE;
double sqrt(double);

void kernel_gramschmidt(int m, int n, DATA_TYPE *A, DATA_TYPE *R, DATA_TYPE *Q)
    __attribute__((export_name("kernel_gramschmidt")));

void kernel_gramschmidt(int m, int n, DATA_TYPE *A, DATA_TYPE *R, DATA_TYPE *Q) {
    int i, j, k;
    DATA_TYPE nrm;

    for (k = 0; k < n; k++) {
        nrm = 0.0;
        for (i = 0; i < m; i++)
            nrm += A[i*n+k] * A[i*n+k];
        R[k*n+k] = sqrt(nrm);
        for (i = 0; i < m; i++)
            Q[i*n+k] = A[i*n+k] / R[k*n+k];
        for (j = k+1; j < n; j++) {
            R[k*n+j] = 0.0;
            for (i = 0; i < m; i++)
                R[k*n+j] += Q[i*n+k] * A[i*n+j];
            for (i = 0; i < m; i++)
                A[i*n+j] = A[i*n+j] - Q[i*n+k] * R[k*n+j];
        }
    }
}
