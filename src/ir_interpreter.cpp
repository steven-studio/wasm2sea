#include <stdio.h>
#include <unordered_map>

extern "C" {
#include "ir.h"
}

int32_t interpret_ir(ir_ctx* ctx, int32_t param0) {
    printf("\n=== Interpreting IR with param=%d ===\n", param0);
    
    std::unordered_map<ir_ref, int32_t> values;
    std::unordered_map<ir_ref, int32_t> vars;  // VAR storage
    
    ir_ref pc = 1;  // START
    int iteration = 0;
    
    while (pc > 0 && iteration < 1000) {
        ir_insn* insn = &ctx->ir_base[pc];
        
        printf("  [%d] %s", pc, ir_op_name[insn->op]);
        
        switch (insn->op) {
        case IR_START:
            pc = insn->op2;
            break;
            
        case IR_PARAM:
            values[pc] = param0;
            printf(" -> %d", param0);
            pc++;
            break;
            
        case IR_VAR:
            vars[pc] = 0;  // 初始化为 0
            pc++;
            break;
            
        case IR_VSTORE: {
            ir_ref var_ref = insn->op2;
            ir_ref val_ref = insn->op3;
            vars[var_ref] = values[val_ref];
            printf(" var[%d] = %d", var_ref, values[val_ref]);
            pc++;
            break;
        }
            
        case IR_VLOAD: {
            ir_ref var_ref = insn->op2;
            values[pc] = vars[var_ref];
            printf(" -> %d", values[pc]);
            pc++;
            break;
        }
            
        case IR_SUB:
            values[pc] = values[insn->op2] - values[insn->op3];
            printf(" %d - %d -> %d", values[insn->op2], values[insn->op3], values[pc]);
            pc++;
            break;
            
        case IR_GT:
            values[pc] = values[insn->op2] > values[insn->op3];
            printf(" %d > %d -> %d", values[insn->op2], values[insn->op3], values[pc]);
            pc++;
            break;
            
        case IR_RETURN:
            printf(" -> RESULT=%d", values[insn->op2]);
            printf("\n\n=== Result: %d (after %d iterations) ===\n", 
                   values[insn->op2], iteration);
            return values[insn->op2];
            
        default:
            printf(" (skipped)");
            pc++;
        }
        
        printf("\n");
        iteration++;
    }
    
    printf("\n=== ERROR: Too many iterations or invalid PC ===\n");
    return -1;
}
