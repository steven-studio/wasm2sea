#include "ir_bridge.hpp"
#include "value_ir.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <map>
#include <set>

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
    // 预先创建所有参数（按照 paramIndex 排序）
    std::map<int, int> param_index_to_value_id;  // paramIndex -> value id
    
    // 第一遍：找出所有参数及其 paramIndex
    for (size_t i = 0; i < values.size(); i++) {
        if (values[i].op == Op::Param) {
            param_index_to_value_id[values[i].paramIndex] = i;
        }
    }
    
    // 第二遍：按照 paramIndex 顺序创建 IR 参数
    for (auto& [param_idx, value_id] : param_index_to_value_id) {
        char name[32];
        snprintf(name, sizeof(name), "p%d", param_idx);
        ir_ref param_ref = ir_PARAM(IR_I32, name, param_idx + 1);
        value_map[value_id] = param_ref;
    }
    
    // 为每个可能的局部变量创建 VAR
    std::set<int> local_indices;
    for (const auto& v : values) {
        if (v.op == Op::Param || v.op == Op::LocalGet || 
            v.op == Op::LocalSet || v.op == Op::LocalTee) {
            local_indices.insert(v.paramIndex);
        }
    }
    
    std::unordered_map<int, ir_ref> local_vars;
    for (int idx : local_indices) {
        char name[32];
        snprintf(name, sizeof(name), "local_%d", idx);
        ir_ref var = ir_VAR(IR_I32, name);
        local_vars[idx] = var;
        
        // 如果是参数，初始化 VAR
        auto it = param_index_to_value_id.find(idx);
        if (it != param_index_to_value_id.end()) {
            int value_id = it->second;
            ir_VSTORE(var, value_map[value_id]);
        }
    }

    // 遍歷所有 ValueIR 指令
    for (size_t i = 0; i < values.size(); i++) {
        const Value& val = values[i];

        // 跳过已经处理的参数
        if (val.op == Op::Param) {
            continue;
        }
                
        switch (val.op) {
        
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

        case Op::And: {
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_AND_I32(lhs_ref, rhs_ref);
            break;
        }
        case Op::Or: {
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_OR_I32(lhs_ref, rhs_ref);
            break;
        }
        case Op::Xor: {
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_XOR_I32(lhs_ref, rhs_ref);
            break;
        }
        case Op::Shl: {
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_SHL_I32(lhs_ref, rhs_ref);
            break;
        }
        case Op::Shr_S: {
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_SAR_I32(lhs_ref, rhs_ref);  // SAR = Shift Arithmetic Right
            break;
        }
        case Op::Shr_U: {
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_SHR_I32(lhs_ref, rhs_ref);  // SHR = Shift Right (logical)
            break;
        }

        case Op::Eqz: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            
            ir_ref val_ref = value_map[val.lhs];
            ir_ref zero = ir_CONST_I32(0);
            value_map[i] = ir_EQ(val_ref, zero);  // eqz 等價於 == 0
            break;
        }

        case Op::Clz: {
            ir_ref val_ref = value_map[val.lhs];
            value_map[i] = ir_CTLZ_I32(val_ref);  // Count Leading Zeros
            break;
        }
        case Op::Ctz: {
            ir_ref val_ref = value_map[val.lhs];
            value_map[i] = ir_CTTZ_I32(val_ref);  // Count Trailing Zeros
            break;
        }
        case Op::Popcnt: {
            ir_ref val_ref = value_map[val.lhs];
            value_map[i] = ir_CTPOP_I32(val_ref);  // Count Population (1 bits)
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

        case Op::Select: {
            if (val.operands.size() != 3) break;
            
            int cond_idx = val.operands[0];
            int true_idx = val.operands[1];
            int false_idx = val.operands[2];
            
            // 边界检查
            if (cond_idx < 0 || cond_idx >= (int)i) break;
            if (true_idx < 0 || true_idx >= (int)i) break;
            if (false_idx < 0 || false_idx >= (int)i) break;
            
            ir_ref cond_ref = value_map[cond_idx];
            ir_ref true_ref = value_map[true_idx];
            ir_ref false_ref = value_map[false_idx];
            
            // 创建 if 节点
            ir_ref if_node = ir_IF(cond_ref);
            
            // True 分支
            ir_IF_TRUE(if_node);
            ir_ref end_true = ir_END();
            
            // False 分支
            ir_IF_FALSE(if_node);
            ir_ref end_false = ir_END();
            
            // 合并（不要赋值！）
            ir_MERGE_2(end_true, end_false);  // ← 这里不赋值
            
            // 创建 PHI 节点
            ir_ref result = ir_PHI_2(IR_I32, false_ref, true_ref);
            
            value_map[i] = result;
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