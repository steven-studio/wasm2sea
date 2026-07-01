typedef double DATA_TYPE;

void kernel_floyd_warshall(int n, DATA_TYPE *path)
    __attribute__((export_name("kernel_floyd_warshall")));

void kernel_floyd_warshall(int n, DATA_TYPE *path) {
    int i, j, k;
    for (k = 0; k < n; k++)
        for (i = 0; i < n; i++)
            for (j = 0; j < n; j++)
                path[i*n+j] = path[i*n+j] < path[i*n+k] + path[k*n+j] ?
                    path[i*n+j] : path[i*n+k] + path[k*n+j];
}
