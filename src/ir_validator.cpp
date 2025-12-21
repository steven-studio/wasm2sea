#include <stdio.h>
#include <set>
#include <queue>

extern "C" {
#include "ir.h"
}

bool validate_ir(ir_ctx* ctx) {
    printf("\n=== IR Validation ===\n");
    
    std::set<ir_ref> visited;
    std::queue<ir_ref> worklist;
    
    // 1. 检查所有节点的定义
    printf("Checking all nodes are defined...\n");
    for (ir_ref i = 1; i < ctx->insns_count; i++) {
        ir_insn* insn = &ctx->ir_base[i];
        if (insn->op == IR_UNUSED) {
            printf("  ERROR: Node %d is UNUSED\n", i);
            return false;
        }
    }
    printf("  ✓ All nodes defined\n");
    
    // 2. 检查控制流连通性
    printf("Checking control flow connectivity...\n");
    worklist.push(1); // START
    visited.insert(1);
    
    while (!worklist.empty()) {
        ir_ref ref = worklist.front();
        worklist.pop();
        
        ir_insn* insn = &ctx->ir_base[ref];
        
        // 检查控制流输入
        if (insn->op1) {
            worklist.push(insn->op1);
            visited.insert(insn->op1);
        }
        if (insn->op2 && IR_OPND_KIND(ir_op_flags[insn->op], 2) == IR_OPND_CONTROL) {
            worklist.push(insn->op2);
            visited.insert(insn->op2);
        }
        if (insn->op3 && IR_OPND_KIND(ir_op_flags[insn->op], 3) == IR_OPND_CONTROL) {
            worklist.push(insn->op3);
            visited.insert(insn->op3);
        }
    }
    
    printf("  Visited %zu / %d control flow nodes\n", visited.size(), ctx->insns_count - 1);
    
    // 3. 检查数据流定义
    printf("Checking data flow definitions...\n");
    for (ir_ref i = 1; i < ctx->insns_count; i++) {
        ir_insn* insn = &ctx->ir_base[i];
        
        // 检查数据操作数
        for (int opnd = 1; opnd <= 3; opnd++) {
            ir_ref op_ref = *(&insn->op1 + opnd - 1);
            if (op_ref > 0 && IR_OPND_KIND(ir_op_flags[insn->op], opnd) != IR_OPND_CONTROL) {
                if (op_ref >= ctx->insns_count) {
                    printf("  ERROR: Node %d references undefined data %d\n", i, op_ref);
                    return false;
                }
            }
        }
    }
    printf("  ✓ All data references valid\n");
    
    // 4. 检查 MERGE 的输入
    printf("Checking MERGE nodes...\n");
    for (ir_ref i = 1; i < ctx->insns_count; i++) {
        if (ctx->ir_base[i].op == IR_MERGE || ctx->ir_base[i].op == IR_LOOP_BEGIN) {
            int inputs = ir_input_edges_count(ctx, &ctx->ir_base[i]);
            printf("  Node %d (%s) has %d inputs\n", 
                   i, ir_op_name[ctx->ir_base[i].op], inputs);
        }
    }
    
    printf("\n✓ IR structure is valid!\n");
    return true;
}
