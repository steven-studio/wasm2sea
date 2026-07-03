typedef int DATA_TYPE;

void kernel_minidp_and(int n, DATA_TYPE *table)
    __attribute__((export_name("kernel_minidp_and")));

void kernel_minidp_and(int n, DATA_TYPE *table) {
    int i, j;
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (i < j - 1 && j < n - 1) {
                table[i*n+j] = 100;
            } else {
                table[i*n+j] = 200;
            }
        }
    }
}
