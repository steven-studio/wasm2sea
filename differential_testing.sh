#!/bin/bash
# differential_testing.sh - 完整的 WASM vs C 差分测试

# set -e  # 遇到错误立即退出

# ========================================
# 配置
# ========================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTS_DIR="$SCRIPT_DIR/tests"
WASM2SEA="${SCRIPT_DIR}/build/wasm2sea"
IR_COMPILER="${SCRIPT_DIR}/third_party/dstogov-ir/ir_main"
NODE="node"
GCC="gcc"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# ========================================
# 辅助函数
# ========================================

print_header() {
    echo ""
    echo "========================================"
    echo "$1"
    echo "========================================"
}

print_test() {
    echo ""
    echo "----------------------------------------"
    echo "Testing: $1"
    echo "----------------------------------------"
}

# ========================================
# 主测试函数
# ========================================

test_program() {
    local test_name=$1
    local num_params=$2  # 函数参数数量 (2 或 3)
    local testdata_file="${TESTS_DIR}/testdata/${test_name}.txt"
    
    print_test "$test_name"
    
    # 检查文件是否存在
    if [ ! -f "${TESTS_DIR}/${test_name}.wat" ]; then
        echo -e "${RED}✗ WAT file not found${NC}"
        return 1
    fi
    
    if [ ! -f "$testdata_file" ]; then
        echo -e "${RED}✗ Test data file not found${NC}"
        return 1
    fi
    
    # ========================================
    # Step 1: 编译 WAT → WASM
    # ========================================
    echo "Step 1: Compiling WAT to WASM..."
    wat2wasm "${TESTS_DIR}/${test_name}.wat" -o "${TESTS_DIR}/${test_name}.wasm"
    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ WAT compilation failed${NC}"
        return 1
    fi
    echo -e "${GREEN}✓ WASM compiled${NC}"
    
    # ========================================
    # Step 2: WASM → IR → C
    # ========================================
    echo "Step 2: Generating C code via wasm2sea..."
    
    # 运行 wasm2sea
    cd "$(dirname "$WASM2SEA")"
    "$WASM2SEA" "${TESTS_DIR}/${test_name}.wasm" > /tmp/wasm2sea_output.log 2>&1
    
    # wasm2sea 会生成 main.ir (或其他名字的 .ir 文件)
    # 需要根据你的实际输出调整
    
    # 假设生成了 main.ir
    if [ ! -f "main.ir" ]; then
        echo -e "${RED}✗ IR file not generated${NC}"
        cat /tmp/wasm2sea_output.log
        return 1
    fi
    
    # 移动到测试目录
    mv main.ir "${TESTS_DIR}/${test_name}.ir"
    echo -e "${GREEN}✓ IR generated${NC}"
    
    # ========================================
    # Step 3: IR → C 代码
    # ========================================
    echo "Step 3: Generating C code from IR..."
    
    # 假设 dstogov/ir 工具用法是: ir <input.ir> -o <output.c>
    # 请根据实际情况调整
    if [ -f "$IR_COMPILER" ]; then
        "$IR_COMPILER" "${TESTS_DIR}/${test_name}.ir" --emit-c "${TESTS_DIR}/${test_name}.c"
    else
        echo -e "${YELLOW}⚠ IR compiler not found, assuming C code already generated${NC}"
        # 如果 wasm2sea 直接输出 C 代码，跳过这步
    fi
    
    if [ ! -f "${TESTS_DIR}/${test_name}.c" ]; then
        echo -e "${RED}✗ C code not generated${NC}"
        return 1
    fi
    echo -e "${GREEN}✓ C code generated${NC}"
    
    # ========================================
    # Step 4: 编译 C 代码
    # ========================================
    echo "Step 4: Compiling C code..."
    
    # 使用 libffi 创建动态参数调用的 wrapper
    cat > "${TESTS_DIR}/${test_name}_wrapper.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ffi.h>

// 包含生成的函数
EOF
    
    # 将生成的代码直接追加进来
    cat "${TESTS_DIR}/${test_name}.c" >> "${TESTS_DIR}/${test_name}_wrapper.c"
    
    # 添加 libffi wrapper main 函数
    cat >> "${TESTS_DIR}/${test_name}_wrapper.c" << 'EOF'

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <arg1> [arg2] [arg3] ...\n", argv[0]);
        return 1;
    }
    
    int num_args = argc - 1;
    
    // 准备参数
    int32_t args[num_args];
    for (int i = 0; i < num_args; i++) {
        args[i] = atoi(argv[i + 1]);
    }
    
    // 准备 FFI 类型
    ffi_type *arg_types[num_args];
    void *arg_values[num_args];
    for (int i = 0; i < num_args; i++) {
        arg_types[i] = &ffi_type_sint32;
        arg_values[i] = &args[i];
    }
    
    // 准备 CIF (Call Interface)
    ffi_cif cif;
    ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, num_args,
                                     &ffi_type_sint32, arg_types);
    
    if (status != FFI_OK) {
        fprintf(stderr, "ffi_prep_cif failed\n");
        return 1;
    }
    
    // 调用函数
    int32_t result;
    ffi_call(&cif, FFI_FN(test), &result, arg_values);
    
    printf("%d\n", result);
    return 0;
}
EOF
    
    # 编译
    $GCC -o "${TESTS_DIR}/${test_name}_test" "${TESTS_DIR}/${test_name}_wrapper.c" -std=c99 -lffi 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ C compilation failed${NC}"
        $GCC -o "${TESTS_DIR}/${test_name}_test" "${TESTS_DIR}/${test_name}_wrapper.c" -std=c99
        return 1
    fi
    echo -e "${GREEN}✓ C code compiled${NC}"
    
    # ========================================
    # Step 5: 运行测试并比对
    # ========================================
    echo "Step 5: Running differential tests..."
    
    local passed=0
    local failed=0
    local total=0
    
    while IFS= read -r line; do
        # 跳过注释和空行
        [[ "$line" =~ ^#.*$ ]] && continue
        [ -z "$line" ] && continue
        
        # 解析测试用例
        if [ "$num_params" -eq 1 ]; then
            read -r arg1 expected <<< "$line"
            args="$arg1"
        elif [ "$num_params" -eq 2 ]; then
            read -r arg1 arg2 expected <<< "$line"
            args="$arg1 $arg2"
        elif [ "$num_params" -eq 3 ]; then
            read -r arg1 arg2 arg3 expected <<< "$line"
            args="$arg1 $arg2 $arg3"
        fi
        
        # 运行 WASM
        wasm_output=$($NODE "${TESTS_DIR}/wasm.js" "${TESTS_DIR}/${test_name}.wasm" main $args 2>/dev/null)
        
        # 运行 C
        c_output=$("${TESTS_DIR}/${test_name}_test" $args 2>/dev/null)
        
        # 比对
        ((total++))
        
        if [ "$wasm_output" = "$c_output" ] && [ "$wasm_output" = "$expected" ]; then
            ((passed++))
        else
            ((failed++))
            echo -e "${RED}✗ FAIL${NC}: Input($args)"
            echo "  WASM:     $wasm_output"
            echo "  C:        $c_output"
            echo "  Expected: $expected"
        fi
        
    done < "$testdata_file"
    
    # 输出结果
    echo ""
    echo "Results:"
    echo "  Total:  $total"
    echo -e "  ${GREEN}Passed: $passed${NC}"
    if [ $failed -gt 0 ]; then
        echo -e "  ${RED}Failed: $failed${NC}"
    else
        echo -e "  ${GREEN}Failed: $failed${NC}"
    fi
    
    if [ $failed -eq 0 ]; then
        echo -e "${GREEN}✓✓✓ All tests PASSED! ✓✓✓${NC}"
        return 0
    else
        echo -e "${RED}✗✗✗ Some tests FAILED ✗✗✗${NC}"
        return 1
    fi
}

# ========================================
# 主程序
# ========================================

print_header "Differential Testing: WASM vs C"

# 测试列表 (名称:参数数量)
tests=(
    "test_add:2"
    "test_sub:2"
    "test_mul:2"
    "test_div:2"
    "test_rem:2"
    "test_max:2"
    "test_abs:2"
    "test_min:2"
    "test_clamp:3"
    "test_locals_01:1"
    "test_locals_02:1"
    "test_locals_03:1"
    "test_locals_04:1"
    "test_locals_05:1"
    "test_locals_06:1"
    "test_locals_07:1"
    "test_locals_08:1"
    "test_locals_09:2"
    "test_locals_10:1"
    "test_bitwise:2"
    "test_compare:2"
    "test_arith:3"
)

total_programs=0
passed_programs=0
failed_programs=0

for test_entry in "${tests[@]}"; do
    test_name=$(echo "$test_entry" | cut -d: -f1)
    num_params=$(echo "$test_entry" | cut -d: -f2)
    
    ((total_programs++))
    
    if test_program "$test_name" "$num_params"; then
        ((passed_programs++))
    else
        ((failed_programs++))
    fi
done

# 最终汇总
print_header "Final Summary"
echo "Programs tested: $total_programs"
echo -e "${GREEN}Programs passed: $passed_programs${NC}"
if [ $failed_programs -gt 0 ]; then
    echo -e "${RED}Programs failed: $failed_programs${NC}"
else
    echo -e "${GREEN}Programs failed: $failed_programs${NC}"
fi

if [ $failed_programs -eq 0 ]; then
    echo ""
    echo -e "${GREEN}🎉 ALL DIFFERENTIAL TESTS PASSED! 🎉${NC}"
    echo ""
    echo "You can now confidently state in your thesis:"
    echo "  'All non-loop test cases pass behavioral verification"
    echo "   with 100% accuracy through differential testing.'"
    exit 0
else
    echo ""
    echo -e "${RED}Some tests failed. Review the errors above.${NC}"
    exit 1
fi