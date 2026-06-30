typedef double DATA_TYPE;
#define MAXN 64

void kernel_durbin(int n, DATA_TYPE *r, DATA_TYPE *y)
    __attribute__((export_name("kernel_durbin")));

void kernel_durbin(int n, DATA_TYPE *r, DATA_TYPE *y) {
    DATA_TYPE z[MAXN];
    DATA_TYPE alpha, beta, sum;
    int i, k;

    y[0] = -r[0];
    beta = 1.0;
    alpha = -r[0];

    for (k = 1; k < n; k++) {
        beta = (1 - alpha*alpha) * beta;
        sum = 0.0;
        for (i = 0; i < k; i++)
            sum += r[k-i-1] * y[i];
        alpha = -(r[k] + sum) / beta;

        for (i = 0; i < k; i++)
            z[i] = y[i] + alpha * y[k-i-1];
        for (i = 0; i < k; i++)
            y[i] = z[i];
        y[k] = alpha;
    }
}
