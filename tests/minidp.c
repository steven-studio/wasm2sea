typedef int DATA_TYPE;

void kernel_minidp(int n, DATA_TYPE *table)
    __attribute__((export_name("kernel_minidp")));

void kernel_minidp(int n, DATA_TYPE *table) {
    int i, j;
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (i < j - 1) {
                table[i*n+j] = 100;
            } else {
                table[i*n+j] = 200;
            }
        }
    }
}
