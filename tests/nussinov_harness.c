#include <stdio.h>
#include <stdint.h>

static uint8_t wasm_memory[1048576];
void kernel_nussinov(uintptr_t __mem, int32_t n, int32_t seq_off, int32_t table_off);

#define match(b1, b2) (((b1)+(b2)) == 3 ? 1 : 0)
#define max_score(s1, s2) ((s1) >= (s2) ? (s1) : (s2))

static void ref_nussinov(int n, char *seq, int *table) {
    int i, j, k;
    for (i = n-1; i >= 0; i--) {
        for (j = i+1; j < n; j++) {
            if (j-1 >= 0) table[i*n+j] = max_score(table[i*n+j], table[i*n+j-1]);
            if (i+1 < n) table[i*n+j] = max_score(table[i*n+j], table[(i+1)*n+j]);
            if (j-1 >= 0 && i+1 < n) {
                if (i < j-1)
                    table[i*n+j] = max_score(table[i*n+j], table[(i+1)*n+j-1] + match(seq[i], seq[j]));
                else
                    table[i*n+j] = max_score(table[i*n+j], table[(i+1)*n+j-1]);
            }
            for (k = i+1; k < j; k++)
                table[i*n+j] = max_score(table[i*n+j], table[i*n+k] + table[(k+1)*n+j]);
        }
    }
}

int main() {
    int n = 6;
    int base_off = 131072;
    int seq_off = base_off, table_off = seq_off + n;

    char *seq = (char*)(wasm_memory+seq_off);
    int *table = (int*)(wasm_memory+table_off);
    char refseq[6];
    int reftable[36];

    for (int i=0;i<n;i++) { char v = (char)(1 + (i%3)); seq[i]=v; refseq[i]=v; }
    for (int i=0;i<n*n;i++) { table[i]=0; reftable[i]=0; }

    kernel_nussinov((uintptr_t)wasm_memory, n, seq_off, table_off);
    ref_nussinov(n, refseq, reftable);

    int pass=1;
    for (int i=0;i<n*n;i++) {
        if (table[i]!=reftable[i]) {pass=0; printf("MISMATCH table[%d]: %d ref=%d\n",i,table[i],reftable[i]);}
    }
    if (pass) printf("PASS nussinov differential test\n");
    else printf("FAIL\n");
    return 0;
}
