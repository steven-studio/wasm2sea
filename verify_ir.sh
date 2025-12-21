#!/bin/bash

echo "=== IR Verification Tool ==="
echo

# 检查参数
if [ $# -lt 1 ]; then
    echo "Usage: $0 <test_name>"
    echo "Example: $0 test_loop_countdown"
    exit 1
fi

TEST_NAME=$1
WASM_FILE="../tests/${TEST_NAME}.wasm"
BUILD_DIR="build"

# 确保在正确的目录
cd ~/wasm2sea

# 1. 编译项目
echo "Step 1: Building project..."
cd $BUILD_DIR
make > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi
echo "  ✓ Build successful"

# 2. 生成 IR 和 DOT
echo
echo "Step 2: Generating IR and DOT graph..."
./wasm2sea $WASM_FILE > output.txt 2>&1

# 提取 IR dump (只要数字开头的行)
sed -n '/^IR Dump:/,/^$/p' output.txt | grep "^[0-9-]" > ir_dump.txt

# 提取 DOT (从 digraph 到第一个单独的 })
sed -n '/^digraph wasm_function/,/^}$/p' output.txt > graph.dot

echo "  ✓ Generated graph.dot"
echo "  ✓ Generated ir_dump.txt"

# 3. 生成图片
echo
echo "Step 3: Rendering graph..."
dot -Tpng graph.dot -o graph.png 2>&1
if [ $? -eq 0 ]; then
    echo "  ✓ Generated graph.png"
else
    echo "  ✗ Failed to render graph"
    echo "DOT error:"
    dot -Tpng graph.dot 2>&1 | head -5
fi

# 4. 分析 IR 结构
echo
echo "Step 4: Analyzing IR structure..."
echo "========================================="

# 统计节点
TOTAL_NODES=$(wc -l < ir_dump.txt)
echo "Total nodes: $TOTAL_NODES"

# 检查关键节点
echo
echo "Key nodes found:"
grep "START\|RETURN\|LOOP\|IF\|MERGE\|PHI" ir_dump.txt

# 5. 验证控制流
echo
echo "========================================="
echo "Step 5: Control Flow Verification"
echo "========================================="

# 检查 START
START_COUNT=$(grep -c "START" ir_dump.txt)
echo "START nodes: $START_COUNT $([ $START_COUNT -eq 1 ] && echo '✓' || echo '✗ Should be 1')"

# 检查 RETURN
RETURN_COUNT=$(grep -c "RETURN" ir_dump.txt)
echo "RETURN nodes: $RETURN_COUNT $([ $RETURN_COUNT -eq 1 ] && echo '✓' || echo '✗ Should be 1')"

# 检查循环结构
LOOP_BEGIN=$(grep -c "LOOP_BEGIN" ir_dump.txt)
LOOP_END=$(grep -c "LOOP_END" ir_dump.txt)
MERGE_COUNT=$(grep -c "MERGE" ir_dump.txt)
END_COUNT=$(grep -c " END " ir_dump.txt)

echo "END nodes: $END_COUNT"
echo "LOOP_BEGIN nodes: $LOOP_BEGIN"
echo "LOOP_END nodes: $LOOP_END"
echo "MERGE nodes: $MERGE_COUNT"

# 检查 MERGE 节点详情
if [ $MERGE_COUNT -gt 0 ]; then
    echo
    echo "MERGE node details:"
    grep "MERGE" ir_dump.txt
fi

# 6. 验证数据流
echo
echo "========================================="
echo "Step 6: Data Flow Verification"
echo "========================================="

# 检查 VAR
VAR_COUNT=$(grep -c " VAR " ir_dump.txt)
echo "VAR nodes: $VAR_COUNT"

# 检查 PARAM
PARAM_COUNT=$(grep -c "PARAM" ir_dump.txt)
echo "PARAM nodes: $PARAM_COUNT"

# 检查 VLOAD/VSTORE 配对
VLOAD_COUNT=$(grep -c "VLOAD" ir_dump.txt)
VSTORE_COUNT=$(grep -c "VSTORE" ir_dump.txt)
echo "VLOAD nodes: $VLOAD_COUNT"
echo "VSTORE nodes: $VSTORE_COUNT"

# 检查算术操作
SUB_COUNT=$(grep -c " SUB " ir_dump.txt)
GT_COUNT=$(grep -c " GT " ir_dump.txt)
echo "SUB nodes: $SUB_COUNT"
echo "GT nodes: $GT_COUNT"

# 7. 检查控制流完整性
echo
echo "========================================="
echo "Step 7: Control Flow Integrity"
echo "========================================="

# 检查 IF 节点
IF_COUNT=$(grep -c "^[0-9].*IF " ir_dump.txt)
IF_TRUE_COUNT=$(grep -c "IF_TRUE" ir_dump.txt)
IF_FALSE_COUNT=$(grep -c "IF_FALSE" ir_dump.txt)

echo "IF nodes: $IF_COUNT"
echo "IF_TRUE nodes: $IF_TRUE_COUNT"
echo "IF_FALSE nodes: $IF_FALSE_COUNT"

if [ $IF_COUNT -gt 0 ]; then
    if [ $IF_TRUE_COUNT -eq $IF_COUNT ] && [ $IF_FALSE_COUNT -eq $IF_COUNT ]; then
        echo "  ✓ All IF nodes have TRUE and FALSE branches"
    else
        echo "  ✗ WARNING: Incomplete IF branches"
    fi
fi

# 8. 循环特定验证
echo
echo "========================================="
echo "Step 8: Loop Structure Analysis"
echo "========================================="

if [ $MERGE_COUNT -gt 0 ]; then
    echo "Analyzing MERGE node #18:"
    MERGE_LINE=$(grep "00018 MERGE" ir_dump.txt)
    echo "  $MERGE_LINE"
    
    # 提取 MERGE 的输入
    MERGE_INPUTS=$(echo "$MERGE_LINE" | awk '{print $3, $4}')
    echo "  Inputs: $MERGE_INPUTS"
    
    # 检查输入是否形成回边
    INPUT1=$(echo "$MERGE_INPUTS" | awk '{print $1}')
    INPUT2=$(echo "$MERGE_INPUTS" | awk '{print $2}')
    
    echo "  Input 1: node $INPUT1"
    echo "  Input 2: node $INPUT2"
    
    # 检查其中一个输入是否是 END (循环入口)
    grep "^$INPUT2 END" ir_dump.txt > /dev/null
    if [ $? -eq 0 ]; then
        echo "  ✓ One input is END (potential loop entry)"
    fi
    
    # 检查另一个输入是否来自 IF_TRUE
    grep "^$INPUT1 END" ir_dump.txt > /dev/null
    IS_END=$?
    
    if [ $IS_END -eq 0 ]; then
        # 找这个 END 的前驱
        PREV_NODE=$(grep "^$INPUT1 END" ir_dump.txt | awk '{print $3}')
        grep "^$PREV_NODE IF_TRUE" ir_dump.txt > /dev/null
        if [ $? -eq 0 ]; then
            echo "  ✓ Back-edge detected: IF_TRUE → END → MERGE"
        fi
    fi
else
    echo "No MERGE nodes found - not using structured loops"
fi

# 9. 可视化验证提示
echo
echo "========================================="
echo "Step 9: Visual Verification"
echo "========================================="

if [ -f graph.png ]; then
    echo "✓ Graph image generated successfully"
    echo
    echo "Please visually verify the following in graph.png:"
    echo "  1. Red arrow from IF_TRUE (node 16) back to END (node 6)?"
    echo "  2. Does MERGE (node 18) connect END (17) and END (6)?"
    echo "  3. Does control flow from MERGE continue to anywhere?"
    echo "  4. Is there a path: MERGE → ... → RETURN?"
else
    echo "✗ Graph image not generated"
fi

# 10. 生成报告
echo
echo "========================================="
echo "Step 10: Verification Summary"
echo "========================================="

ISSUES=0

# 检查必要的组件
[ $START_COUNT -ne 1 ] && echo "✗ Invalid START count" && ISSUES=$((ISSUES+1))
[ $RETURN_COUNT -ne 1 ] && echo "✗ Invalid RETURN count" && ISSUES=$((ISSUES+1))
[ $PARAM_COUNT -eq 0 ] && echo "✗ No PARAM nodes found" && ISSUES=$((ISSUES+1))
[ $IF_COUNT -gt 0 ] && [ $IF_TRUE_COUNT -ne $IF_COUNT ] && echo "✗ Incomplete IF branches" && ISSUES=$((ISSUES+1))

if [ $ISSUES -eq 0 ]; then
    echo
    echo "✓✓✓ IR structure is well-formed ✓✓✓"
    echo
else
    echo
    echo "Found $ISSUES structural issues (see above)"
    echo
fi

echo "========================================="
echo "Critical Question for Loop Validation:"
echo "========================================="
echo
echo "Does the MERGE node (18) have any control flow OUTPUT?"
echo "  - If NO → Loop is NOT properly formed (dead code)"
echo "  - If YES → Need to verify it connects back to loop body"
echo
echo "Check graph.png or ir_dump.txt to answer this question."
echo

# 11. 输出文件位置
echo "========================================="
echo "Generated Files:"
echo "========================================="
echo "  - graph.dot     : DOT source"
echo "  - graph.png     : Visual graph"
echo "  - ir_dump.txt   : IR text dump"
echo "  - output.txt    : Full compiler output"
echo

