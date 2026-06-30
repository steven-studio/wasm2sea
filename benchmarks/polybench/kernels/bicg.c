typedef double DATA_TYPE;

void kernel_bicg(int m, int n, DATA_TYPE *A, DATA_TYPE *s, DATA_TYPE *q, DATA_TYPE *p, DATA_TYPE *r) {
    int i, j;
    for (i = 0; i < m; i++)
        s[i] = 0;
    for (i = 0; i < n; i++) {
        q[i] = 0.0;
        for (j = 0; j < m; j++) {
            s[j] = s[j] + r[i] * A[i*m+j];
            q[i] = q[i] + A[i*m+j] * p[j];
        }
    }
}
