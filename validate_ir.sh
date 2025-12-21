#!/bin/bash

echo "=== Manual IR Validation ==="
echo

# 测试原始 WASM
echo "Testing original WASM with wasmtime:"
for i in 0 1 2 3 4 5 10; do
    result=$(wasmtime --invoke test ../tests/test_loop_countdown.wasm $i 2>/dev/null)
    echo "  test($i) = $result"
done

echo
echo "Expected behavior:"
echo "  test(0) = 0"
echo "  test(1) = 1" 
echo "  test(2) = 1"
echo "  test(3) = 1"
echo "  test(4) = 1"
echo "  test(5) = 1"
echo "  test(10) = 1"
