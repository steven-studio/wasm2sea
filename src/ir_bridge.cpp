// ===== 在所有 #include 之后添加这段 =====
// Enable trace output for debugging and verification
#define ENABLE_IR_TRACE 1

#if ENABLE_IR_TRACE
#define TRACE(...) fprintf(stderr, "[IR_TRACE] " __VA_ARGS__)
#else
#define TRACE(...) do {} while(0)
#endif
// ===== 结束 =====

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
#include <stack>

// 在 build() 函数中添加循环信息追踪
struct LoopInfo {
    ir_ref loop_begin;           // ← 改名：LOOP_BEGIN 節點
    ir_ref entry_point;  // 循环入口的控制流节点
    std::vector<int> phi_ids;    // ← 新增：記錄這個循環的 Phi 節點
};
std::stack<LoopInfo> loop_stack;

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

    // ===== 添加这行 =====
    TRACE("=== Starting IR Bridge Construction ===\n");
    TRACE("Total ValueIR entries: %zu\n\n", values.size());
    // ===== 结束 =====

    ir_init(ctx_, IR_FUNCTION, 128, 128);
    
    // 開始建構 IR
    ir_START();
    ir_ref start = ctx_->ir_base[1].op2;  // START node reference
    
    // 映射 ValueID → ir_ref
    std::vector<ir_ref> value_map(values.size(), IR_UNUSED);
    // 预先创建所有参数（按照 paramIndex 排序）
    std::map<int, int> param_index_to_value_id;  // paramIndex -> value id
    
    // ← 添加这个！存储循环退出时的值
    std::unordered_map<int, ir_ref> loop_exit_values;

    // 第一遍：找出所有参数及其 paramIndex
    for (size_t i = 0; i < values.size(); i++) {
        if (values[i].op == Op::Param) {
            param_index_to_value_id[values[i].paramIndex] = i;
        }
    }
    
    // 第二遍：按照 paramIndex 顺序创建 IR 参数
    TRACE("--- Phase 1: Creating Parameters ---\n");
    for (auto& [param_idx, value_id] : param_index_to_value_id) {
        char name[32];
        snprintf(name, sizeof(name), "p%d", param_idx);
        ir_ref param_ref = ir_PARAM(IR_I32, name, param_idx + 1);
        value_map[value_id] = param_ref;

        // ===== 添加这行 =====
        TRACE("v%d = Param(%d) -> ir_PARAM(\"%s\", %d) [node ref=%d]\n", 
            value_id, param_idx, name, param_idx + 1, param_ref);
        // ===== 结束 =====
    }
    
    // 为每个可能的局部变量创建 VAR
    std::set<int> local_indices;
    for (const auto& v : values) {
        // if (v.op == Op::Param || v.op == Op::LocalGet || 
        //     v.op == Op::LocalSet || v.op == Op::LocalTee) {
        //     local_indices.insert(v.paramIndex);
        // }
        // 从 Phi 节点收集局部变量索引
        if (v.op == Op::Phi && v.local_index >= 0) {
            local_indices.insert(v.local_index);
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

        // ===== 添加这段 =====
        TRACE("--- Processing v%zu ---\n", i);
        // ===== 结束 =====

        // 跳过已经处理的参数
        if (val.op == Op::Param) {
            TRACE("  Skipped (already processed)\n\n");
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

            // ===== 添加这行 =====
            TRACE("  v%zu = Add(v%d, v%d) -> ir_ADD_I32(ref %d, ref %d) = ref %d\n\n",
                i, val.lhs, val.rhs, lhs_ref, rhs_ref, value_map[i]);
            // ===== 结束 =====
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

            // ===== 添加这段 =====
            TRACE("  v%zu = Gt_S(v%d, v%d) -> ir_GT(ref %d, ref %d) = ref %d\n\n",
                i, val.lhs, val.rhs, lhs_ref, rhs_ref, value_map[i]);
            // ===== 结束 =====
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
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            
            // ✅ 直接从 value_map 获取返回值（可能是 PHI）
            ir_ref ret_val = value_map[val.lhs];
            
            TRACE("  v%zu = Return(v%d) -> ir_RETURN(ref %d)\n\n",
                i, val.lhs, ret_val);
            
            ir_RETURN(ret_val);
            break;
        }
        
        case Op::Const: {
            value_map[i] = ir_CONST_I32(val.constValue);

            // ===== 添加这行 =====
            TRACE("  v%zu = Const(%d) -> ir_CONST_I32(%d) = ref %d\n\n",
                i, val.constValue, val.constValue, value_map[i]);
            // ===== 结束 =====
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
            
            // ===== 添加这段 =====
            TRACE("  v%zu = Select(cond=v%d, false=v%d, true=v%d)\n", 
                i, cond_idx, false_idx, true_idx);
            TRACE("    Expanding to IF/MERGE/PHI pattern:\n");
            // ===== 结束 =====

            // 创建 if 节点
            ir_ref if_node = ir_IF(cond_ref);
            TRACE("      1. ir_IF(ref %d) = ref %d\n", cond_ref, if_node);

            // True 分支
            ir_IF_TRUE(if_node);
            ir_ref end_true = ir_END();
            TRACE("      2. ir_IF_TRUE -> ir_END() = ref %d\n", end_true);

            // False 分支
            ir_IF_FALSE(if_node);
            ir_ref end_false = ir_END();
            TRACE("      3. ir_IF_FALSE -> ir_END() = ref %d\n", end_false);

            // 合并（不要赋值！）
            ir_MERGE_2(end_true, end_false);  // ← 这里不赋值
            TRACE("      4. ir_MERGE_2(ref %d, ref %d)\n", end_true, end_false);

            // 创建 PHI 节点
            ir_ref result = ir_PHI_2(IR_I32, true_ref, false_ref);
            TRACE("      5. ir_PHI_2(true=ref %d, false=ref %d) = ref %d\n", 
                    true_ref, false_ref, result);
            TRACE("    => v%zu maps to ref %d\n\n", i, result);

            value_map[i] = result;
            break;
        }

        case Op::If:
        case Op::Else:
        case Op::End: {
            if (!loop_stack.empty()) {
                loop_stack.pop();
            }
            break;
        }

        case Op::Loop: {

            TRACE("  v%zu = Loop - Creating LOOP_BEGIN\n", i);
            
            // 創建 LOOP_BEGIN 節點
            ir_ref loop_begin = ir_LOOP_BEGIN(ir_END());
            ctx_->control = loop_begin;  // ← 關鍵：設置當前控制流
            
            TRACE("    ir_LOOP_BEGIN = ref %d\n", loop_begin);
            
            LoopInfo info;
            info.loop_begin = loop_begin;
            info.entry_point = loop_begin;
            loop_stack.push(info);
            
            value_map[i] = loop_begin;
            TRACE("    Pushed to loop_stack\n\n");
            break;
        }

        case Op::Phi: {
            TRACE("  v%zu = Phi\n", i);
            
            // ===== 添加：打印 operands =====
            TRACE("    Phi operands: [");
            for (size_t j = 0; j < val.operands.size(); j++) {
                TRACE("v%d", val.operands[j]);
                if (j < val.operands.size() - 1) TRACE(", ");
            }
            TRACE("]\n");
            // ===== 结束 =====
            
            // ✅ 直接创建 PHI
            if (!val.operands.empty()) {
                int op0 = val.operands[0];
                TRACE("    Phi op0 value-id = %d\n", op0);
                TRACE("    value_map[op0]   = %d\n", value_map[op0]);

                if (value_map[op0] == IR_UNUSED) {
                    TRACE("    ERROR: Phi entry is IR_UNUSED (op0=%d)\n", op0);
                }

                ir_ref entry_val = value_map[val.operands[0]];
                ir_ref phi = ir_PHI_2(IR_I32, entry_val, IR_UNUSED);
                value_map[i] = phi;
                
                // ===== 添加：打印创建结果 =====
                TRACE("    Created PHI = ref %d (entry=ref %d, backedge=UNUSED)\n", 
                    phi, entry_val);
                TRACE("    local_index = %d\n", val.local_index);
                // ===== 结束 =====
                
                if (!loop_stack.empty()) {
                    loop_stack.top().phi_ids.push_back(i);
                    TRACE("    Added to loop_stack.phi_ids\n");
                }
            }
            
            TRACE("\n");
            break;
        }

        case Op::Br_if: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            
            ir_ref cond_ref = value_map[val.lhs];
            TRACE("  v%zu = Br_if(cond=v%d)\n", i, val.lhs);
            
            if (loop_stack.empty()) {
                TRACE("    ERROR: Br_if without active loop!\n\n");
                break;
            }
            
            LoopInfo& loop_info = loop_stack.top();
            
            // 1. 更新所有循環變量（存儲 backedge 值）
            TRACE("    Updating loop variables:\n");
            for (int phi_id : loop_info.phi_ids) {
                const Value& phi_val = values[phi_id];
                if (phi_val.operands.size() >= 2) {
                    int entry_val_id = phi_val.operands[0];     // 入口值
                    int backedge_val_id = phi_val.operands[1];  // 回边值
                    
                    ir_ref phi_ref = value_map[phi_id];
                    ir_ref entry_ref = value_map[entry_val_id];
                    ir_ref backedge_ref = value_map[backedge_val_id];
                    
                    // ✅ 显式设置两个操作数
                    // ir_set_op(ctx_, phi_ref, 1, entry_ref);     // 第一个操作数
                    ir_PHI_SET_OP(phi_ref, 2, backedge_ref);  // 第二个操作数
                    
                    TRACE("      PHI ref %d: op1=ref %d, op2=ref %d\n", 
                        phi_ref, entry_ref, backedge_ref);
                }
            }
            
            // 3. 創建條件分支
            ir_ref if_node = ir_IF(cond_ref);
            TRACE("    ir_IF(ref %d) = ref %d\n", cond_ref, if_node);
            
            // 4. True 分支：回跳到 LOOP_BEGIN
            ir_IF_TRUE(if_node);
            ir_ref loop_end_ref = ir_LOOP_END();
            TRACE("    ir_LOOP_END = ref %d\n", loop_end_ref);
            
            // 4. ✅ 回填 LOOP_BEGIN 的第二個輸入（創建回邊）
            ir_MERGE_SET_OP(loop_info.loop_begin, 2, loop_end_ref);
            TRACE("    ir_MERGE_SET_OP(LOOP_BEGIN ref %d, 2, LOOP_END ref %d)\n",
                loop_info.loop_begin, loop_end_ref);
            
            // 5. False 分支：退出循環
            ir_IF_FALSE(if_node);
            TRACE("    ir_IF_FALSE -> (exit loop)\n\n");
            
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