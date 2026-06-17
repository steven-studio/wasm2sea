#!/bin/bash

WASM_DIR="benchmarks/polybench/wasm"
LOG_DIR="benchmarks/polybench/wasm2sea_log"

mkdir -p "$LOG_DIR"

for wasm in "$WASM_DIR"/*.wasm; do
    name=$(basename "$wasm" .wasm)
    echo "Running wasm2sea on $name..."
    ./build/wasm2sea "$wasm" > "$LOG_DIR/$name.log" 2>&1
    if [ $? -eq 0 ]; then
        echo "  ✓ $name"
    else
        echo "  ✗ $name FAILED"
    fi
done
