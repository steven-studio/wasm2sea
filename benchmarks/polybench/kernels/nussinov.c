typedef char base;
typedef int DATA_TYPE;

#define match(b1, b2) (((b1)+(b2)) == 3 ? 1 : 0)
#define max_score(s1, s2) ((s1) >= (s2) ? (s1) : (s2))

void kernel_nussinov(int n, base *seq, DATA_TYPE *table)
    __attribute__((export_name("kernel_nussinov")));

void kernel_nussinov(int n, base *seq, DATA_TYPE *table) {
    int i, j, k;
    for (i = n-1; i >= 0; i--) {
        for (j = i+1; j < n; j++) {
            if (j-1 >= 0)
                table[i*n+j] = max_score(table[i*n+j], table[i*n+j-1]);
            if (i+1 < n)
                table[i*n+j] = max_score(table[i*n+j], table[(i+1)*n+j]);

            if (j-1 >= 0 && i+1 < n) {
                if (i < j-1)
                    table[i*n+j] = max_score(table[i*n+j], table[(i+1)*n+j-1] + match(seq[i], seq[j]));
                else
                    table[i*n+j] = max_score(table[i*n+j], table[(i+1)*n+j-1]);
            }

            for (k = i+1; k < j; k++) {
                table[i*n+j] = max_score(table[i*n+j], table[i*n+k] + table[(k+1)*n+j]);
            }
        }
    }
}
