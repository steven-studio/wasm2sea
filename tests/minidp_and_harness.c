#include <stdio.h>
#include <stdint.h>

static uint8_t wasm_memory[1048576];
void test(uintptr_t __mem, int32_t n, int32_t table_off);

int main() {
    int n = 5;
    int base = 131072;
    int table_off = base;
    int *table = (int*)(wasm_memory+table_off);
    int expected[25];
    for (int i=0;i<n*n;i++) table[i]=0;

    test((uintptr_t)wasm_memory, n, table_off);

    for (int i=0;i<n;i++)
        for (int j=0;j<n;j++)
            expected[i*n+j] = (i<j-1 && j<n-1)?100:200;

    int pass=1;
    for (int i=0;i<n*n;i++) {
        if (table[i]!=expected[i]) {pass=0; printf("MISMATCH table[%d]: %d ref=%d\n",i,table[i],expected[i]);}
    }
    if (pass) printf("PASS minidp_and differential test\n");
    else printf("FAIL\n");
    return 0;
}
