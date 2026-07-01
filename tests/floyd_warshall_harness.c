#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_floyd_warshall(uintptr_t __mem, int32_t n, int32_t path_off);

static void ref_floyd_warshall(int n, double *path) {
    int i, j, k;
    for (k = 0; k < n; k++)
        for (i = 0; i < n; i++)
            for (j = 0; j < n; j++)
                path[i*n+j] = path[i*n+j] < path[i*n+k]+path[k*n+j] ?
                    path[i*n+j] : path[i*n+k]+path[k*n+j];
}

int main() {
    int n=5;
    int base = 131072;
    int path_off = base;
    double *path = (double*)(wasm_memory+path_off);
    double refpath[25];

    for (int i=0;i<n;i++) for (int j=0;j<n;j++) {
        double v = (i==j) ? 0.0 : ((i+j)%3)+1.0;
        path[i*n+j]=v; refpath[i*n+j]=v;
    }

    kernel_floyd_warshall((uintptr_t)wasm_memory, n, path_off);
    ref_floyd_warshall(n, refpath);

    int pass=1;
    for (int i=0;i<n*n;i++) {
        if (fabs(path[i]-refpath[i])>1e-6){pass=0;printf("MISMATCH path[%d]: %.6f ref=%.6f\n",i,path[i],refpath[i]);}
    }
    if (pass) printf("PASS floyd-warshall differential test\n");
    else printf("FAIL\n");
    return 0;
}
