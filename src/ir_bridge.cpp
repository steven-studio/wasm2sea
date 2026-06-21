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

// 在文件開頭添加 if 追蹤結構
struct IfInfo {
    ir_ref if_node;      // ir_IF 的返回值
    ir_ref end_true;     // true 分支的 END 節點
    bool has_else;       // 是否有 else 分支
    bool true_branch_returns;  // true 分支是否有 return
};
std::stack<IfInfo> if_stack;

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

IRFunction* IRBridge::build(const ValueIR& values, const std::vector<ParamType>& paramTypes) {
    TRACE("--- paramTypes ---\n");
    for (size_t i = 0; i < paramTypes.size(); i++) {
        TRACE("  param[%zu] = %s\n", i, 
            paramTypes[i] == ParamType::I64 ? "I64" :
            paramTypes[i] == ParamType::F64 ? "F64" : "I32");
    }
    
    // 重要：dstogov/ir 的巨集需要變數名為 'ctx'
    ir_ctx* ctx = ctx_;

    // ===== 添加这行 =====
    TRACE("=== Starting IR Bridge Construction ===\n");
    TRACE("Total ValueIR entries: %zu\n\n", values.size());
    // ===== 结束 =====

    while (!if_stack.empty()) if_stack.pop();
    while (!loop_stack.empty()) loop_stack.pop();
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
        ir_type ir_type = IR_I32;  // 默认为 I32
        if (param_idx < (int)paramTypes.size()) {
            if (paramTypes[param_idx] == ParamType::I64) {
                ir_type = IR_I64;
            } else if (paramTypes[param_idx] == ParamType::F64) {
                ir_type = IR_DOUBLE;
            }
        }
        ir_ref param_ref = ir_PARAM(ir_type, name, param_idx + 1);
        value_map[value_id] = param_ref;

        // ===== 添加这行 =====
        TRACE("v%d = Param(%d) -> ir_PARAM(\"%s\", %d) [node ref=%d]\n", 
            value_id, param_idx, name, param_idx + 1, param_ref);
        // ===== 结束 =====
    }
    
    // 为每个可能的局部变量创建 VAR
    std::set<int> local_indices;
    for (const auto& v : values) {
        if (v.op == Op::Param || v.op == Op::LocalGet || 
            v.op == Op::LocalSet || v.op == Op::LocalTee) {
            local_indices.insert(v.paramIndex);
        }
        // 从 Phi 节点收集局部变量索引
        if (v.op == Op::Phi && v.local_index >= 0) {
            local_indices.insert(v.local_index);
        }
    }

    // 在建 local_vars 之前，先建 local_types map
    std::unordered_map<int, ir_type> local_types;
    for (int idx : local_indices) {
        // 預設 I32，如果 paramTypes 說是 I64 就改
        if (idx < (int)paramTypes.size() && paramTypes[idx] == ParamType::I64)
            local_types[idx] = IR_I64;
        else
            local_types[idx] = IR_I32;
    }
    
    std::unordered_map<int, ir_ref> local_vars;
    for (int idx : local_indices) {
        char name[32];
        snprintf(name, sizeof(name), "local_%d", idx);
        ir_type t = local_types.count(idx) ? local_types[idx] : IR_I32;
        ir_ref var = ir_VAR(t, name);
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
            // 找到這個 paramIndex 對應的 ir_ref
            auto it = param_index_to_value_id.find(val.paramIndex);
            if (it != param_index_to_value_id.end()) {
                // 用已經建好的 ir_ref
                value_map[i] = value_map[it->second];
            }
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
            if (val.type == ValueType::I64)
                value_map[i] = ir_ADD_I64(lhs_ref, rhs_ref);
            else
                value_map[i] = ir_ADD_I32(lhs_ref, rhs_ref);

            // ===== 添加这行 =====
            TRACE("  v%zu = Add(v%d, v%d) type=%s\n", i, val.lhs, val.rhs,
                val.type == ValueType::I64 ? "I64" : "I32");
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
            if (val.type == ValueType::I64)
                value_map[i] = ir_SUB_I64(lhs_ref, rhs_ref);
            else
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
            if (val.type == ValueType::I64)
                value_map[i] = ir_MUL_I64(lhs_ref, rhs_ref);
            else
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
            if (val.type == ValueType::I64)
                value_map[i] = ir_DIV_I64(lhs_ref, rhs_ref);
            else
                value_map[i] = ir_DIV_I32(lhs_ref, rhs_ref);
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
            if (val.type == ValueType::I64)
                value_map[i] = ir_DIV_U64(lhs_ref, rhs_ref);
            else
                value_map[i] = ir_DIV_U32(lhs_ref, rhs_ref);
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
            if (val.type == ValueType::I64)
                value_map[i] = ir_MOD_I64(lhs_ref, rhs_ref);
            else
                value_map[i] = ir_MOD_I32(lhs_ref, rhs_ref);
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
            if (val.type == ValueType::I64)
                value_map[i] = ir_MOD_U64(lhs_ref, rhs_ref);
            else
                value_map[i] = ir_MOD_U32(lhs_ref, rhs_ref);
            break;
        }

        case Op::Eq: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            if (val.rhs < 0 || val.rhs >= (int)i) break;
            
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            value_map[i] = ir_EQ(lhs_ref, rhs_ref);  // 注意：沒有 _I32 後綴
            
            // ===== 添加这行 =====
            TRACE("  v%zu = Eq(v%d, v%d) -> ir_EQ(ref %d, ref %d) = ref %d\n\n",
                i, val.lhs, val.rhs, lhs_ref, rhs_ref, value_map[i]);
            // ===== 结束 =====            
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
        
        case Op::LocalGet: {
            int var_idx = val.paramIndex;
            
            // ✅ 檢查是否有對應的 VAR
            auto it = local_vars.find(var_idx);
            if (it == local_vars.end()) {
                fprintf(stderr, "ERROR: LocalGet for untracked local_%d\n", var_idx);
                break;
            }
            
            ir_ref var_ref = it->second;
            
            // ✅ 從 VAR 加載值
            ir_type t = local_types.count(var_idx) ? local_types[var_idx] : IR_I32;
            ir_ref loaded_val = (t == IR_I64) ? ir_VLOAD_I64(var_ref) : ir_VLOAD_I32(var_ref);
            value_map[i] = loaded_val;
            
            TRACE("  v%zu = LocalGet(local_%d) -> ir_VLOAD_I32(var %d) = ref %d\n\n",
                i, var_idx, var_ref, loaded_val);
            break;
        }

        case Op::LocalSet: {
            int var_idx = val.paramIndex;
            
            // ✅ 檢查是否有對應的 VAR
            auto it = local_vars.find(var_idx);
            if (it == local_vars.end()) {
                fprintf(stderr, "ERROR: LocalSet for untracked local_%d\n", var_idx);
                break;
            }
            
            ir_ref var_ref = it->second;
            
            // ✅ 檢查要存儲的值是否有效
            if (val.lhs < 0 || val.lhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid value index %d for LocalSet\n", val.lhs);
                break;
            }
            
            ir_ref value_ref = value_map[val.lhs];
            
            // ✅ 存儲到 VAR
            ir_VSTORE(var_ref, value_ref);
            
            TRACE("  v%zu = LocalSet(local_%d, v%d) -> ir_VSTORE(var %d, ref %d)\n\n",
                i, var_idx, val.lhs, var_ref, value_ref);
            
            // ✅ LocalSet 本身不產生值，所以不設置 value_map[i]
            break;
        }

        case Op::LocalTee: {
            int var_idx = val.paramIndex;
            
            auto it = local_vars.find(var_idx);
            if (it == local_vars.end()) {
                fprintf(stderr, "ERROR: LocalTee for untracked local_%d\n", var_idx);
                break;
            }
            
            ir_ref var_ref = it->second;
            
            if (val.lhs < 0 || val.lhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid value index %d for LocalTee\n", val.lhs);
                break;
            }
            
            ir_ref value_ref = value_map[val.lhs];
            
            // ✅ 存儲到 VAR
            ir_VSTORE(var_ref, value_ref);
            
            // ✅ LocalTee 會返回存儲的值（與 LocalSet 的區別）
            value_map[i] = value_ref;
            
            TRACE("  v%zu = LocalTee(local_%d, v%d) -> ir_VSTORE + return ref %d\n\n",
                i, var_idx, val.lhs, value_ref);
            break;
        }

        case Op::Call: {
            const auto& args = val.operands;
            ir_ref name_ref = ir_str(ctx, val.callee_name.c_str());
            ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
            
            std::vector<ir_ref> arg_refs;
            for (int arg_id : args) {
                if (arg_id >= 0 && arg_id < (int)value_map.size())
                    arg_refs.push_back(value_map[arg_id]);
            }
            
            ir_ref result = ir_CALL_N(IR_I32, func_ref,
                                    (uint32_t)arg_refs.size(),
                                    arg_refs.data());
            value_map[i] = result;
            TRACE("  v%zu = Call(%s, %zu args) -> ir_CALL_N ref %d\n",
                i, val.callee_name.c_str(), args.size(), result);
            break;
        }

        case Op::Unreachable: {
            ir_TRAP();
            TRACE("  v%zu = Unreachable -> ir_TRAP()\n", i);
            break;
        }

        case Op::MemorySize: {
            ir_ref name_ref = ir_str(ctx, "__wasm_memory_size");
            ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
            ir_ref result = ir_CALL(IR_I32, func_ref);
            value_map[i] = result;
            TRACE("  v%zu = MemorySize -> ir_CALL(__wasm_memory_size)\n", i);
            break;
        }

        case Op::MemoryCopy: {
            ir_ref name_ref = ir_str(ctx, "__wasm_memory_copy");
            ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
            ir_ref dest   = value_map[val.operands[0]];
            ir_ref source = value_map[val.operands[1]];
            ir_ref size   = value_map[val.operands[2]];
            ir_CALL_3(IR_VOID, func_ref, dest, source, size);
            TRACE("  v%zu = MemoryCopy -> ir_CALL_3(__wasm_memory_copy)\n", i);
            break;
        }
        case Op::MemoryFill: {
            ir_ref name_ref = ir_str(ctx, "__wasm_memory_fill");
            ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
            ir_ref dest  = value_map[val.operands[0]];
            ir_ref value = value_map[val.operands[1]];
            ir_ref size  = value_map[val.operands[2]];
            ir_CALL_3(IR_VOID, func_ref, dest, value, size);
            TRACE("  v%zu = MemoryFill -> ir_CALL_3(__wasm_memory_fill)\n", i);
            break;
        }

        case Op::Return: {            
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            
            // ✅ 直接从 value_map 获取返回值（可能是 PHI）
            ir_ref ret_val = value_map[val.lhs];
            
            TRACE("  v%zu = Return(v%d) -> ir_RETURN(ref %d)\n\n",
                i, val.lhs, ret_val);
            
            ir_RETURN(ret_val);

            if (!if_stack.empty() && !if_stack.top().has_else) {
                if_stack.top().true_branch_returns = true;
            }
            break;
        }
        
        case Op::I32Const: {
            ir_ref c = ir_CONST_I32(val.constValue);
            TRACE(" v%zu = Const(%d) -> ir_CONST_I32(%d) = ref %d (literal)\n",
            i, val.constValue, val.constValue, c);

            ir_ref copy = ir_COPY_I32(c);
            value_map[i] = copy;

            TRACE(" -> ir_COPY_I32(ref %d) = ref %d (value)\n\n",
            c, copy);
            break;
        }

        case Op::I64Const: {
            ir_ref c = ir_CONST_I64(val.constValue);
            TRACE(" v%zu = Const(%d) -> ir_CONST_I64(%d) = ref %d (literal)\n",
            i, val.constValue, val.constValue, c);

            ir_ref copy = ir_COPY_I64(c);
            value_map[i] = copy;

            TRACE(" -> ir_COPY_I64(ref %d) = ref %d (value)\n\n",
            c, copy);
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

        case Op::If: {
            if (val.lhs < 0 || val.lhs >= (int)i) {
                fprintf(stderr, "ERROR: Invalid condition for If\n");
                break;
            }
            
            ir_ref cond_ref = value_map[val.lhs];
            TRACE("  v%zu = If(cond=v%d)\n", i, val.lhs);
            
            // 創建 IF 節點
            ir_ref if_node = ir_IF(cond_ref);
            TRACE("    ir_IF(ref %d) = ref %d\n", cond_ref, if_node);
            
            // 進入 true 分支
            ir_IF_TRUE(if_node);
            TRACE("    ir_IF_TRUE -> entering true branch\n\n");
            
            // 推入棧以追蹤
            IfInfo info;
            info.if_node = if_node;
            info.end_true = IR_UNUSED;  // 稍後在 Else 或 End 設置
            info.has_else = false;
            if_stack.push(info);
            
            break;
        }

        case Op::Else: {
            if (if_stack.empty()) {
                fprintf(stderr, "ERROR: Else without matching If\n");
                break;
            }
            
            TRACE("  v%zu = Else\n", i);
            
            // 結束 true 分支
            ir_ref end_true = ir_END();
            TRACE("    ir_END (true branch) = ref %d\n", end_true);
            
            // 保存 end_true 供後續 merge 使用
            if_stack.top().end_true = end_true;
            if_stack.top().has_else = true;
            
            // 進入 false 分支
            ir_IF_FALSE(if_stack.top().if_node);
            TRACE("    ir_IF_FALSE -> entering false branch\n\n");
            
            break;
        }

        case Op::End: {
            fprintf(stderr, "DEBUG: Op::End reached, if_stack.size()=%zu\n", if_stack.size());
            if (if_stack.empty()) {
                // 可能是 loop 的 end，這裡先忽略
                // TRACE("  v%zu = End (loop or block end)\n\n", i);
                break;
            }
            
            // TRACE("  v%zu = End (if end)\n", i);
            
            IfInfo info = if_stack.top();
            if_stack.pop();
            
            if (info.has_else) {
                // ✅ 结束 false 分支
                ir_ref end_false = ir_END();
                TRACE("    ir_END (false branch) = ref %d\n", end_false);
                
                // ✅ 创建 MERGE
                ir_MERGE_2(info.end_true, end_false);
                TRACE("    ir_MERGE_2(ref %d, ref %d)\n", 
                    info.end_true, end_false);
            } else {
                if (info.true_branch_returns) {
                    // true 分支已經 RETURN，只需要 IF_FALSE
                    ir_IF_FALSE(info.if_node);
                } else {
                    // true 分支正常結束，需要 MERGE
                    ir_ref end_true = ir_END();
                    ir_IF_FALSE(info.if_node);
                    ir_ref end_false = ir_END();
                    ir_MERGE_2(end_true, end_false);
                }
            }
            
            TRACE("\n");
            break;
        }

        case Op::Loop: {

            TRACE("  v%zu = Loop - Creating LOOP_BEGIN\n", i);
            
            // 創建 LOOP_BEGIN 節點
            ir_ref loop_begin = ir_LOOP_BEGIN(ir_END());
            
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
            
            // 打印 operands
            TRACE("    Phi operands: [");
            for (size_t j = 0; j < val.operands.size(); j++) {
                TRACE("v%d", val.operands[j]);
                if (j < val.operands.size() - 1) TRACE(", ");
            }
            TRACE("]\n");
            TRACE("    local_index = %d\n", val.local_index);
            
            if (val.operands.empty()) {
                TRACE("    ERROR: Phi with no operands\n\n");
                break;
            }
            
            if (val.local_index >= 0) {
                // ========== Loop Phi ==========
                TRACE("    Type: Loop Phi (for local_%d)\n", val.local_index);
                
                ir_ref entry_val = value_map[val.operands[0]];
                ir_ref phi = ir_PHI_2(IR_I32, entry_val, IR_UNUSED);
                value_map[i] = phi;
                
                TRACE("    Created PHI = ref %d (entry=ref %d, backedge=UNUSED)\n", 
                    phi, entry_val);
                
                if (!loop_stack.empty()) {
                    loop_stack.top().phi_ids.push_back(i);
                    TRACE("    Added to loop_stack.phi_ids\n");
                }
                
            } else {
                // ========== If Phi ==========
                TRACE("    Type: If Phi (expression value)\n");
                
                if (val.operands.size() != 2) {
                    TRACE("    ERROR: If Phi should have exactly 2 operands\n\n");
                    break;
                }
                
                // ✅ 关键：If Phi 创建时就设置两个操作数！
                ir_ref then_val = value_map[val.operands[0]];
                ir_ref else_val = value_map[val.operands[1]];
                ir_ref phi = ir_PHI_2(IR_I32, then_val, else_val);
                value_map[i] = phi;
                
                TRACE("    Created PHI = ref %d (then=ref %d, else=ref %d)\n", 
                    phi, then_val, else_val);
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
                    
                    // ✅ 加入這些檢查！
                    TRACE("      Updating PHI v%d (ir_ref %d):\n", phi_id, phi_ref);
                    TRACE("        entry   = v%d -> ir_ref %d\n", entry_val_id, entry_ref);
                    TRACE("        backedge = v%d -> ir_ref %d\n", backedge_val_id, backedge_ref);

                    // ⚠️ 關鍵檢查
                    if (backedge_ref == IR_UNUSED) {
                        TRACE("        ❌ ERROR: backedge is IR_UNUSED!\n");
                    }
                    if (backedge_ref == entry_ref) {
                        TRACE("        ⚠️  WARNING: backedge == entry (PHI won't change!)\n");
                    }

                    // ✅ 显式设置两个操作数
                    ir_PHI_SET_OP(phi_ref, 2, backedge_ref);  // 第二个操作数
                }
            }
            
            // 3. 創建條件分支
            ir_ref if_node = ir_IF(cond_ref);
            TRACE("    ir_IF(ref %d) = ref %d\n", cond_ref, if_node);
            
            // 4. True 分支：回跳到 LOOP_BEGIN
            ir_IF_TRUE(if_node);
            TRACE("    ir_IF_TRUE\n");  // ← 加這行
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

        case Op::F64Const: {
            value_map[i] = ir_CONST_DOUBLE(val.constValue);
            break;
        }

        case Op::F64Add: {
            if (val.lhs < 0 || val.rhs < 0) break;
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            if (lhs_ref == IR_UNUSED || rhs_ref == IR_UNUSED) break;
            value_map[i] = ir_ADD_D(lhs_ref, rhs_ref);
            break;
        }

        case Op::F64Sub: {
            if (val.lhs < 0 || val.rhs < 0) break;
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            if (lhs_ref == IR_UNUSED || rhs_ref == IR_UNUSED) break;
            value_map[i] = ir_SUB_D(lhs_ref, rhs_ref);
            break;
        }

        case Op::F64Mul: {
            if (val.lhs < 0 || val.rhs < 0) break;
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            if (lhs_ref == IR_UNUSED || rhs_ref == IR_UNUSED) break;
            value_map[i] = ir_MUL_D(lhs_ref, rhs_ref);
            break;
        }

        case Op::F64Div: {
            if (val.lhs < 0 || val.rhs < 0) break;
            ir_ref lhs_ref = value_map[val.lhs];
            ir_ref rhs_ref = value_map[val.rhs];
            if (lhs_ref == IR_UNUSED || rhs_ref == IR_UNUSED) break;
            value_map[i] = ir_DIV_D(lhs_ref, rhs_ref);
            break;
        }

        case Op::Load: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            ir_ref ptr_ref = value_map[val.lhs];
            if (ptr_ref == IR_UNUSED) break;
            value_map[i] = ir_LOAD_I32(ptr_ref);
            TRACE("  v%zu = Load(v%d) -> ir_LOAD_I32(ref %d) = ref %d\n\n",
                i, val.lhs, ptr_ref, value_map[i]);
            break;
        }

        case Op::Store: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            if (val.rhs < 0 || val.rhs >= (int)i) break;
            ir_ref ptr_ref = value_map[val.lhs];
            ir_ref val_ref = value_map[val.rhs];
            if (ptr_ref == IR_UNUSED || val_ref == IR_UNUSED) break;
            ir_STORE(ptr_ref, val_ref);
            TRACE("  v%zu = Store(v%d, v%d)\n\n", i, val.lhs, val.rhs);
            break;
        }

        case Op::F64Load: {
            if (val.lhs < 0 || val.lhs >= (int)i) break;
            ir_ref ptr_ref = value_map[val.lhs];
            if (ptr_ref == IR_UNUSED) break;
            value_map[i] = ir_LOAD_D(ptr_ref);
            break;
        }

        case Op::F64Store: {
            if (val.lhs < 0 || val.rhs < 0) break;
            ir_ref ptr_ref = value_map[val.lhs];
            ir_ref val_ref = value_map[val.rhs];
            if (ptr_ref == IR_UNUSED || val_ref == IR_UNUSED) break;
            ir_STORE(ptr_ref, val_ref);
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
