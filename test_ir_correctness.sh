#!/bin/bash

echo "=== IR Correctness Test ==="
echo

# 测试用例
TEST_VALUES=(0 1 2 3 4 5 10 100)

for val in "${TEST_VALUES[@]}"; do
    echo "Testing with input: $val"
    
    # 1. 用 wasmtime 运行原始 WASM
    expected=$(wasmtime --invoke test ../tests/test_loop_countdown.wasm $val 2>/dev/null || echo "ERROR")
    
    # 2. 用我们的 IR 解释器运行
    # (这里需要添加调用 IR 解释器的代码)
    
    echo "  Expected: $expected"
    echo "  Got:      (TODO: call IR interpreter)"
    echo
done

echo "=== Test Complete ==="
