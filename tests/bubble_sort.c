#include <stdint.h>

// 排序 arr[0..n-1]
void bubble_sort(int* arr, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

// 包一層：輸入 8 個數，排完後回傳第 k 小的
int sort_and_get(int a0, int a1, int a2, int a3,
                 int a4, int a5, int a6, int a7, int k) {
    int arr[8] = {a0, a1, a2, a3, a4, a5, a6, a7};
    bubble_sort(arr, 8);
    return arr[k];
}
