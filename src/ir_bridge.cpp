#include "ir_bridge.hpp"
#include "value_ir.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// 引入 dstogov/ir 的 C API
extern "C" {
#include "ir.h"
#include "ir_builder.h"
}

IRBridge::IRBridge() {
    // 分配 ir_ctx 結構
    ctx_ = (ir_ctx*)malloc(sizeof(ir_ctx));
    if (!ctx_) {
        fprintf(stderr, "Failed to allocate ir_ctx\n");
        exit(1);
    }
}

IRBridge::~IRBridge() {
    if (ctx_) {
        ir_free(ctx_);
        free(ctx_);
    }
}

IRFunction* IRBridge::build(const ValueIR& values) {
    // 重要：dstogov/ir 的巨集需要變數名為 'ctx'
    ir_ctx* ctx = ctx_;

    ir_init(ctx_, IR_FUNCTION | IR_OPT_FOLDING, 128, 128);
    
    // 開始建構 IR
    ir_START();
    ir_ref start = ctx_->ir_base[1].op2;  // START node reference
    
    // 映射 ValueID → ir_ref
    std::vector<ir_ref> value_map(values.size(), IR_UNUSED);
    uint32_t param_idx = 0;
    
    // 遍歷所有 ValueIR 指令
    for (size_t i = 0; i < values.size(); i++) {
        const Value& val = values[i];
                
        switch (val.op) {
        case Op::Param: {
            char name[32];
            snprintf(name, sizeof(name), "p%u", param_idx);
            ir_ref param = ir_PARAM(IR_I32, name, param_idx + 1);
            value_map[i] = param;
            param_idx++;
            break;
        }
        
        case Op::Add: {
            // 檢查 lhs 和 rhs 是否有效
            if (val.lhs < 0 || val.lhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i);
                break;
            }
            if (val.rhs < 0 || val.rhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i);
                break;
            }

            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_ADD_I32(lhs_ref, rhs_ref);
            break;
        }

        case Op::Sub: {
            // 檢查 lhs 和 rhs 是否有效
            if (val.lhs < 0 || val.lhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i);
                break;
            }
            if (val.rhs < 0 || val.rhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i);
                break;
            }

            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_SUB_I32(lhs_ref, rhs_ref);
            break;
        }

        case Op::Mul: {
            // 檢查 lhs 和 rhs 是否有效
            if (val.lhs < 0 || val.lhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i);
                break;
            }
            if (val.rhs < 0 || val.rhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i);
                break;
            }

            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_MUL_I32(lhs_ref, rhs_ref);
            break;
        }

        case Op::Div_S: {
            if (val.lhs < 0 || val.lhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i);
                break;
            }
            if (val.rhs < 0 || val.rhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i);
                break;
            }
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_DIV_I32(lhs_ref, rhs_ref);  // ✅ 正確
            break;
        }
        case Op::Div_U: {
            if (val.lhs < 0 || val.lhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i);
                break;
            }
            if (val.rhs < 0 || val.rhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i);
                break;
            }
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_DIV_U32(lhs_ref, rhs_ref);  // ✅ 改成 U32
            break;
        }
        case Op::Rem_S: {
            if (val.lhs < 0 || val.lhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i);
                break;
            }
            if (val.rhs < 0 || val.rhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i);
                break;
            }
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_MOD_I32(lhs_ref, rhs_ref);  // ✅ 正確
            break;
        }
        case Op::Rem_U: {
            if (val.lhs < 0 || val.lhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i);
                break;
            }
            if (val.rhs < 0 || val.rhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i);
                break;
            }
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_MOD_U32(lhs_ref, rhs_ref);  // ✅ 改成 U32
            break;
        }

        case Op::Eq: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            if (val.rhs < 0 || val.rhs >= (int)i) break;
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_EQ(lhs_ref, rhs_ref);  // 注意：沒有 _I32 後綴
            break;
        }

        case Op::Ne: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            if (val.rhs < 0 || val.rhs >= (int)i) break;
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_NE(lhs_ref, rhs_ref);
            break;
        }

        case Op::Lt_S: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            if (val.rhs < 0 || val.rhs >= (int)i) break;
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_LT(lhs_ref, rhs_ref);  // 有號小於
            break;
        }

        case Op::Lt_U: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            if (val.rhs < 0 || val.rhs >= (int)i) break;
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_ULT(lhs_ref, rhs_ref);  // 無號小於
            break;
        }

        case Op::Gt_S: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            if (val.rhs < 0 || val.rhs >= (int)i) break;
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_GT(lhs_ref, rhs_ref);
            break;
        }

        case Op::Gt_U: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            if (val.rhs < 0 || val.rhs >= (int)i) break;
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_UGT(lhs_ref, rhs_ref);
            break;
        }

        case Op::Le_S: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            if (val.rhs < 0 || val.rhs >= (int)i) break;
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_LE(lhs_ref, rhs_ref);
            break;
        }

        case Op::Le_U: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            if (val.rhs < 0 || val.rhs >= (int)i) break;
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_ULE(lhs_ref, rhs_ref);
            break;
        }

        case Op::Ge_S: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            if (val.rhs < 0 || val.rhs >= (int)i) break;
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_GE(lhs_ref, rhs_ref);
            break;
        }

        case Op::Ge_U: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            if (val.rhs < 0 || val.rhs >= (int)i) break;
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_UGE(lhs_ref, rhs_ref);
            break;
        }

        case Op::Eqz: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            
            ir_ref val_ref = value_map[val.lhs];
            ir_ref zero = ir_CONST_I32(0);
            value_map[i] = ir_EQ(val_ref, zero);  // eqz 等價於 == 0
            break;
        }
        
        case Op::Return: {            
            if (val.lhs < 0 || val.lhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid return lhs=%d\n", val.lhs);
                break;
            }
            
            ir_ref ret_val = value_map[val.lhs];
            ir_RETURN(ret_val);
            break;
        }
        
        case Op::Const: {
            value_map[i] = ir_CONST_I32(val.constValue);
            break;
        }
        
        default:
            fprintf(stderr, "Unsupported Op: %d\n", (int)val.op);
            break;
        }
    }
    
    // 包裝成 IRFunction
    auto* fn = new IRFunction();
    fn->ctx = ctx_;
    fn->entry_ref = start;
    
    return fn;
}

void IRBridge::dump(IRFunction* fn) {
    if (!fn || !fn->ctx) {
        fprintf(stderr, "Invalid IRFunction\n");
        return;
    }
    
    // 文字格式
    printf("\nIR Dump:\n");
    ir_dump(fn->ctx, stdout);
    
    // DOT 格式（可用 Graphviz 畫圖）
    printf("\n\nDOT Format (copy to file.dot and run: dot -Tpng file.dot -o graph.png):\n");
    ir_dump_dot(fn->ctx, "wasm_function", stdout);
}

bool IRBridge::save(const char* path) {
    if (!path) return false;

    FILE* f = fopen(path, "w");
    if (!f) return false;

    ir_save(ctx_, 0, f);

    fclose(f);
    return true;
}