#define ENABLE_IR_TRACE 0

#if ENABLE_IR_TRACE
        // #define TRACE(...) fprintf(stderr, "[IR_TRACE] " __VA_ARGS__)
#else
#define TRACE(...) do {} while(0)
#endif

#include "ir_bridge.hpp"
#include "value_ir.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <map>
#include <set>
#include <stack>

extern "C" {
#include "ir.h"
#include "ir_builder.h"
}

struct IfInfo {
    ir_ref if_node;
    ir_ref end_true;
    bool has_else;
    bool true_branch_returns;
};
std::stack<IfInfo> if_stack;

struct LoopInfo {
    ir_ref loop_begin;
    ir_ref entry_point;
    std::vector<int> phi_ids;
    int loop_value_id;
    ir_ref loop_exit = IR_UNUSED;
    std::vector<ir_ref> exits;
    std::unordered_map<int, ir_ref> outer_carry_phis;  // local_idx -> outer PHI ref
};
std::vector<LoopInfo> loop_stack;

struct BuildContext {
    ir_ctx* ctx;
    const ValueIR& values;
    const std::vector<ParamType>& paramTypes;
    std::vector<ir_ref>& value_map;
    std::unordered_map<int, ir_ref>& local_vars;
    std::unordered_map<int, ir_type>& local_types;
    std::unordered_map<int, ir_ref>& global_vars;
    ir_ref mem_param;
    bool has_memory_ops;
    size_t current_index;
};

IRBridge::IRBridge() {
    ctx_ = (ir_ctx*)malloc(sizeof(ir_ctx));
    if (!ctx_) { fprintf(stderr, "Failed to allocate ir_ctx\n"); exit(1); }
}

IRBridge::~IRBridge() {
    if (ctx_) { ir_free(ctx_); free(ctx_); }
}

// ----- Arithmetic -----

void IRBridge::handleAdd(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i); return; }
    if (val.rhs < 0 || val.rhs >= (int)i) { fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i); return; }
    ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
    bc.value_map[i] = (val.type == ValueType::I64) ? ir_ADD_I64(lhs_ref, rhs_ref) : ir_ADD_I32(lhs_ref, rhs_ref);
    TRACE("  v%zu = Add(v%d, v%d) type=%s\n", i, val.lhs, val.rhs, val.type == ValueType::I64 ? "I64" : "I32");
}

void IRBridge::handleSub(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i); return; }
    if (val.rhs < 0 || val.rhs >= (int)i) { fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i); return; }
    ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
    bc.value_map[i] = (val.type == ValueType::I64) ? ir_SUB_I64(lhs_ref, rhs_ref) : ir_SUB_I32(lhs_ref, rhs_ref);
}

void IRBridge::handleMul(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i); return; }
    if (val.rhs < 0 || val.rhs >= (int)i) { fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i); return; }
    ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
    bc.value_map[i] = (val.type == ValueType::I64) ? ir_MUL_I64(lhs_ref, rhs_ref) : ir_MUL_I32(lhs_ref, rhs_ref);
}

void IRBridge::handleDivS(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
    bc.value_map[i] = (val.type == ValueType::I64) ? ir_DIV_I64(lhs_ref, rhs_ref) : ir_DIV_I32(lhs_ref, rhs_ref);
}

void IRBridge::handleDivU(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
    bc.value_map[i] = (val.type == ValueType::I64) ? ir_DIV_U64(lhs_ref, rhs_ref) : ir_DIV_U32(lhs_ref, rhs_ref);
}

void IRBridge::handleRemS(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
    bc.value_map[i] = (val.type == ValueType::I64) ? ir_MOD_I64(lhs_ref, rhs_ref) : ir_MOD_I32(lhs_ref, rhs_ref);
}

void IRBridge::handleRemU(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
    bc.value_map[i] = (val.type == ValueType::I64) ? ir_MOD_U64(lhs_ref, rhs_ref) : ir_MOD_U32(lhs_ref, rhs_ref);
}

// ----- Comparison -----

void IRBridge::handleEq(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
    bc.value_map[i] = ir_EQ(lhs_ref, rhs_ref);
    TRACE("  v%zu = Eq(v%d, v%d) -> ir_EQ(ref %d, ref %d) = ref %d\n\n", i, val.lhs, val.rhs, lhs_ref, rhs_ref, bc.value_map[i]);
}

void IRBridge::handleNe(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    bc.value_map[i] = ir_NE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

void IRBridge::handleLtS(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    bc.value_map[i] = ir_LT(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

void IRBridge::handleLtU(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    bc.value_map[i] = ir_ULT(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

void IRBridge::handleGtS(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
    bc.value_map[i] = ir_GT(lhs_ref, rhs_ref);
    TRACE("  v%zu = Gt_S(v%d, v%d) -> ir_GT(ref %d, ref %d) = ref %d\n\n", i, val.lhs, val.rhs, lhs_ref, rhs_ref, bc.value_map[i]);
}

void IRBridge::handleGtU(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    bc.value_map[i] = ir_UGT(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

void IRBridge::handleLeS(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    bc.value_map[i] = ir_LE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

void IRBridge::handleLeU(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    bc.value_map[i] = ir_ULE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

void IRBridge::handleGeS(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    bc.value_map[i] = ir_GE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

void IRBridge::handleGeU(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    bc.value_map[i] = ir_UGE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

// ----- Bitwise -----

void IRBridge::handleAnd(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    bc.value_map[i] = ir_AND_I32(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

void IRBridge::handleOr(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    bc.value_map[i] = ir_OR_I32(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

void IRBridge::handleXor(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    bc.value_map[i] = ir_XOR_I32(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

void IRBridge::handleShl(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    bc.value_map[i] = ir_SHL_I32(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

void IRBridge::handleShrS(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    bc.value_map[i] = ir_SAR_I32(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

void IRBridge::handleShrU(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    bc.value_map[i] = ir_SHR_I32(bc.value_map[val.lhs], bc.value_map[val.rhs]);
}

// ----- Unary -----

void IRBridge::handleEqz(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i) return;
    bc.value_map[i] = ir_EQ(bc.value_map[val.lhs], ir_CONST_I32(0));
}

void IRBridge::handleClz(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    bc.value_map[bc.current_index] = ir_CTLZ_I32(bc.value_map[val.lhs]);
}

void IRBridge::handleCtz(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    bc.value_map[bc.current_index] = ir_CTTZ_I32(bc.value_map[val.lhs]);
}

void IRBridge::handlePopcnt(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    bc.value_map[bc.current_index] = ir_CTPOP_I32(bc.value_map[val.lhs]);
}

// ----- Type Conversion -----

void IRBridge::handleI32WrapI64(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    bc.value_map[bc.current_index] = ir_TRUNC_I32(bc.value_map[val.lhs]);
}

void IRBridge::handleI64ExtendI32S(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    bc.value_map[bc.current_index] = ir_SEXT_I64(bc.value_map[val.lhs]);
}

void IRBridge::handleI64ExtendI32U(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    bc.value_map[bc.current_index] = ir_ZEXT_I64(bc.value_map[val.lhs]);
}

void IRBridge::handleF64ConvertI(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    bc.value_map[bc.current_index] = ir_INT2D(bc.value_map[val.lhs]);
}

void IRBridge::handleI32TruncF64(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    bc.value_map[bc.current_index] = ir_FP2I32(bc.value_map[val.lhs]);
}

void IRBridge::handleI64TruncF64(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    bc.value_map[bc.current_index] = ir_FP2I64(bc.value_map[val.lhs]);
}

// ----- Constants -----

void IRBridge::handleI32Const(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    ir_ref c = ir_CONST_I32(val.constValue);
    ir_ref copy = ir_COPY_I32(c);
    bc.value_map[i] = copy;
    TRACE(" v%zu = Const(%d) -> ir_CONST_I32 ref %d, ir_COPY ref %d\n\n", i, val.constValue, c, copy);
}

void IRBridge::handleI64Const(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    ir_ref c = ir_CONST_I64(val.constValue);
    ir_ref copy = ir_COPY_I64(c);
    bc.value_map[i] = copy;
}

void IRBridge::handleF64Const(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    bc.value_map[bc.current_index] = ir_CONST_DOUBLE(val.fconst);
}

// ----- Locals -----

void IRBridge::handleLocalGet(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    int var_idx = val.paramIndex;
    auto it = bc.local_vars.find(var_idx);
    if (it == bc.local_vars.end()) { fprintf(stderr, "ERROR: LocalGet for untracked local_%d\n", var_idx); return; }
    ir_ref var_ref = it->second;
    ir_type t = bc.local_types.count(var_idx) ? bc.local_types[var_idx] : IR_I32;
    ir_ref loaded = (t == IR_I64) ? ir_VLOAD_I64(var_ref) : ir_VLOAD_I32(var_ref);
    bc.value_map[i] = loaded;
    TRACE("  v%zu = LocalGet(local_%d) -> ref %d\n\n", i, var_idx, loaded);
}

void IRBridge::handleLocalSet(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    int var_idx = val.paramIndex;
    auto it = bc.local_vars.find(var_idx);
    if (it == bc.local_vars.end()) { fprintf(stderr, "ERROR: LocalSet for untracked local_%d\n", var_idx); return; }
    if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid value index %d for LocalSet\n", val.lhs); return; }
    ir_VSTORE(it->second, bc.value_map[val.lhs]);
    TRACE("  v%zu = LocalSet(local_%d, v%d)\n\n", i, var_idx, val.lhs);
}

void IRBridge::handleGlobalGet(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    int gidx = val.globalIndex;
    auto it = bc.global_vars.find(gidx);
    if (it == bc.global_vars.end()) {
        fprintf(stderr, "ERROR: GlobalGet for untracked wasm_global_%d\n", gidx);
        return;
    }
    bc.value_map[i] = ir_VLOAD_I32(it->second);
}

void IRBridge::handleGlobalSet(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    int gidx = val.globalIndex;
    auto it = bc.global_vars.find(gidx);
    if (it == bc.global_vars.end()) {
        fprintf(stderr, "ERROR: GlobalSet for untracked wasm_global_%d\n", gidx);
        return;
    }
    if (val.lhs < 0 || val.lhs >= (int)i) return;
    ir_VSTORE(it->second, bc.value_map[val.lhs]);
}

void IRBridge::handleLocalTee(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    int var_idx = val.paramIndex;
    auto it = bc.local_vars.find(var_idx);
    if (it == bc.local_vars.end()) { fprintf(stderr, "ERROR: LocalTee for untracked local_%d\n", var_idx); return; }
    if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid value index %d for LocalTee\n", val.lhs); return; }
    ir_ref value_ref = bc.value_map[val.lhs];
    ir_VSTORE(it->second, value_ref);
    bc.value_map[i] = value_ref;
    TRACE("  v%zu = LocalTee(local_%d, v%d)\n\n", i, var_idx, val.lhs);
}

// ----- Control Flow -----

void IRBridge::handleIf(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid condition for If\n"); return; }
    ir_ref cond_ref = bc.value_map[val.lhs];
    ir_ref if_node = ir_IF(cond_ref);
    ir_IF_TRUE(if_node);
    TRACE("  v%zu = If(cond=v%d) -> ir_IF ref %d, ir_IF_TRUE\n\n", i, val.lhs, if_node);
    IfInfo info;
    info.if_node = if_node;
    info.end_true = IR_UNUSED;
    info.has_else = false;
    info.true_branch_returns = false;
    if_stack.push(info);
}

void IRBridge::handleElse(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (if_stack.empty()) { fprintf(stderr, "ERROR: Else without matching If\n"); return; }
    ir_ref end_true = ir_END();
    TRACE("  v%zu = Else -> ir_END (true branch) ref %d\n\n", i, end_true);
    if_stack.top().end_true = end_true;
    if_stack.top().has_else = true;
    ir_IF_FALSE(if_stack.top().if_node);
}

void IRBridge::handleEnd(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
        // fprintf(stderr, "DEBUG: Op::End reached, if_stack.size()=%zu\n", if_stack.size());
    if (!if_stack.empty()) {
        IfInfo info = if_stack.top();
        if_stack.pop();

        if (info.has_else) {
            ir_ref end_false = ir_END();
            ir_MERGE_2(info.end_true, end_false);
        } else {
            if (info.true_branch_returns) {
                ir_IF_FALSE(info.if_node);
            } else {
                ir_ref end_true = ir_END();
                ir_IF_FALSE(info.if_node);
                ir_ref end_false = ir_END();
                ir_MERGE_2(end_true, end_false);
            }
        }
    }

    // block end：不要 pop loop_stack
    if (val.constValue == 1) {
        TRACE("  v%zu = End(block)\n", i);
        return;
    }

    // loop end：只有這裡才 pop loop_stack
    if (val.constValue == 0) {
        TRACE("  v%zu = End(loop)\n", i);
        return;
    }
}

void IRBridge::handleLoop(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    TRACE("  v%zu = Loop - Creating LOOP_BEGIN\n", i);
    ir_ref loop_begin = ir_LOOP_BEGIN(ir_END());
    TRACE("    ir_LOOP_BEGIN = ref %d\n\n", loop_begin);
    LoopInfo info;
    info.loop_begin = loop_begin;
    info.entry_point = loop_begin;
    info.loop_value_id = (int)i;
    loop_stack.push_back(info);
    bc.value_map[i] = loop_begin;
}

void IRBridge::handleBrIf(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i) return;
    ir_ref cond_ref = bc.value_map[val.lhs];
    if (loop_stack.empty()) { TRACE("    ERROR: Br_if without active loop!\n\n"); return; }

    ir_ref if_node = ir_IF(cond_ref);
    
    // 找對應的 loop（用 phi_ids 對 val.rhs）
    if (val.constValue == 1) {
        // WASM: cond true = break
        // dstogov/ir: IF_FALSE = exit, IF_TRUE = loop body continues
        ir_IF_FALSE(if_node);
        ir_ref exit_end = ir_END();
        loop_stack.back().exits.push_back(exit_end);
        ir_IF_TRUE(if_node);
        ir_ref body_end = ir_END();
        ir_BEGIN(body_end);
        return;
    }

    // br_if depth=0: continue loop / backedge
    LoopInfo& loop_info = loop_stack.back();

    for (int phi_id : loop_info.phi_ids) {
        const Value& phi_val = bc.values[phi_id];
        if (phi_val.operands.size() >= 2) {
            ir_ref phi_ref = bc.value_map[phi_id];
            ir_ref backedge_ref = bc.value_map[phi_val.operands[1]];
            ir_PHI_SET_OP(phi_ref, 2, backedge_ref);
        }
    }

    ir_IF_TRUE(if_node);
    ir_ref loop_end_ref = ir_LOOP_END();
    ir_MERGE_SET_OP(loop_info.loop_begin, 2, loop_end_ref);
    ir_IF_FALSE(if_node);
    TRACE("  v%zu = Br_if(cond=v%d) kind=loop_back\n", i, val.lhs);
}

void IRBridge::handleBr(BuildContext& bc, const Value& val) {
        // fprintf(stderr, "[BR_BRIDGE] val.lhs=%d, val.rhs=%d\n", val.lhs, val.rhs);
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0) return;  // Block target

    // 找對應的 loop（用 phi_ids 對 val.rhs）
    LoopInfo* target_loop = nullptr;
    for (int k = (int)loop_stack.size() - 1; k >= 0; k--) {
        if (loop_stack[k].loop_value_id == val.lhs) {
            target_loop = &loop_stack[k];
            break;
        }
    }
        // fprintf(stderr, "[BR_FIND] val.lhs=%d, loop_stack.size=%zu, target_loop=%p\n",
        // val.lhs, loop_stack.size(), (void*)target_loop);    
    for (int k = 0; k < (int)loop_stack.size(); k++)
        // fprintf(stderr, "  loop_stack[%d].loop_value_id=%d\n", k, loop_stack[k].loop_value_id);
    if (!target_loop && !loop_stack.empty()) target_loop = &loop_stack.back();
    if (!target_loop) return;

    // 設 inner PHI back-edge
    for (int phi_id : target_loop->phi_ids) {
        const Value& phi_val = bc.values[phi_id];
        if ((int)phi_val.operands.size() >= 2) {
            ir_ref phi_ref = bc.value_map[phi_id];
            ir_ref backedge_ref = bc.value_map[phi_val.operands[1]];
        // fprintf(stderr, "[BR] Adding back-edge value v%d for local_%d to phi v%d\n",
                    // phi_val.operands[1], phi_val.local_index, phi_id);
            ir_PHI_SET_OP(phi_ref, 2, backedge_ref);
        }
    }

    // 設 outer carry PHI 的 back-edge
    if (loop_stack.size() >= 2) {
        LoopInfo& outer_loop = loop_stack[loop_stack.size() - 2];
        for (auto& [local_idx, outer_phi_ref] : outer_loop.outer_carry_phis) {
        // fprintf(stderr, "[OUTER_CARRY] Looking for local_%d in phi_ids (size=%zu)\n",
                // local_idx, target_loop->phi_ids.size());
            // 找 inner loop 裡這個 local 的最新值
            for (int phi_id : target_loop->phi_ids) {
                const Value& phi_val = bc.values[phi_id];
        // fprintf(stderr, "  phi_id=%d, local_index=%d\n", phi_id, phi_val.local_index);
                if (phi_val.local_index == local_idx && (int)phi_val.operands.size() >= 2) {
                    ir_ref backedge_ref = bc.value_map[phi_val.operands[1]];
        // fprintf(stderr, "[OUTER_CARRY] Setting outer PHI back-edge: local_%d, backedge=v%d\n",
                        // local_idx, phi_val.operands[1]);
                    ir_PHI_SET_OP(outer_phi_ref, 2, backedge_ref);
                    break;
                }
            }
        }
    }

        // fprintf(stderr, "[BR] val.rhs=%d, found loop_begin=%d, loop_exit=%d\n", 
        // val.rhs, target_loop ? target_loop->loop_begin : -1,
        // target_loop ? target_loop->loop_exit : -1);

    ir_ref loop_end_ref = ir_LOOP_END();
    ir_MERGE_SET_OP(target_loop->loop_begin, 2, loop_end_ref);
    fprintf(stderr, "[BR] exits.size=%zu\n", target_loop->exits.size());
    if (!target_loop->exits.empty()) {
        ir_MERGE_2(loop_end_ref, target_loop->exits[0]);
    }
    loop_stack.pop_back();
    TRACE("  v%zu = Br -> LOOP_END\n\n", i);
}

void IRBridge::handlePhi(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.operands.empty()) { TRACE("    ERROR: Phi with no operands\n\n"); return; }
    if (val.local_index >= 0) {
        ir_ref entry_val = bc.value_map[val.operands[0]];
        ir_ref loop_begin_ref = ctx->control;  // 記錄 LOOP_BEGIN

        // 如果 entry_val 是另一個 PHI，用 VSTORE/VLOAD 打破 PHI-PHI chain
        if (val.use_vload_entry && loop_stack.size() >= 2) {
            int local_idx = val.local_index;
            LoopInfo& outer_loop = loop_stack[loop_stack.size() - 2];
            ir_ref inner_loop_begin = ctx->control;
            // 切換到 outer loop 建 PHI
            ctx->control = outer_loop.loop_begin;
            ir_ref outer_phi = ir_PHI_2(IR_I32, ir_CONST_I32(0), IR_UNUSED);
        // fprintf(stderr, "[OUTER_PHI_BUILD] outer_phi=ref%d, ctx->control=%d\n", outer_phi, ctx->control);
            // 用 COPY_HARD 包住 outer PHI，讓 optimizer 不折疊
            ir_ref hard_copy = ir_emit2(ctx, IR_OPT(IR_COPY, IR_I32), outer_phi, IR_COPY_HARD);
            outer_loop.outer_carry_phis[local_idx] = outer_phi;
            ctx->control = inner_loop_begin;
            entry_val = outer_phi;
        }

        ir_ref inputs[2] = {entry_val, IR_UNUSED};
        ir_ref phi = ir_emit_N(ctx, IR_OPT(IR_PHI, IR_I32), 3);
        // fprintf(stderr, "[PHI_BUILD] v%zu: phi=ref%d, ctx->control=%d\n", i, phi, ctx->control);
        ir_set_op(ctx, phi, 1, ctx->control);
        ir_set_op(ctx, phi, 2, entry_val);
        ir_set_op(ctx, phi, 3, IR_UNUSED);        bc.value_map[i] = phi;
        if (!loop_stack.empty()) loop_stack.back().phi_ids.push_back(i);
        TRACE("  v%zu = Phi (Loop) -> ref %d\n\n", i, phi);
    } else {
        if (val.operands.size() != 2) { TRACE("    ERROR: If Phi should have exactly 2 operands\n\n"); return; }
        ir_ref phi = ir_PHI_2(IR_I32, bc.value_map[val.operands[0]], bc.value_map[val.operands[1]]);
        bc.value_map[i] = phi;
        TRACE("  v%zu = Phi (If) -> ref %d\n\n", i, phi);
    }
}

void IRBridge::handleReturn(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i) {
        // void return
        ctx_->ret_type = IR_VOID;
        ir_RETURN(IR_UNUSED);
    } else {
        ir_ref ret_val = bc.value_map[val.lhs];
        TRACE("  v%zu = Return(v%d) -> ir_RETURN(ref %d)\n\n", i, val.lhs, ret_val);
        ir_RETURN(ret_val);
    }
    if (!if_stack.empty() && !if_stack.top().has_else)
        if_stack.top().true_branch_returns = true;
}

// ----- Select -----

void IRBridge::handleSelect(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.operands.size() != 3) return;
    int cond_idx = val.operands[0], true_idx = val.operands[1], false_idx = val.operands[2];
    if (cond_idx < 0 || cond_idx >= (int)i || true_idx < 0 || true_idx >= (int)i || false_idx < 0 || false_idx >= (int)i) return;
    ir_ref cond_ref = bc.value_map[cond_idx];
    ir_ref true_ref = bc.value_map[true_idx];
    ir_ref false_ref = bc.value_map[false_idx];
    ir_ref if_node = ir_IF(cond_ref);
    ir_IF_TRUE(if_node);
    ir_ref end_true = ir_END();
    ir_IF_FALSE(if_node);
    ir_ref end_false = ir_END();
    ir_MERGE_2(end_true, end_false);
    bc.value_map[i] = ir_PHI_2(IR_I32, true_ref, false_ref);
}

// ----- Memory -----

void IRBridge::handleLoad(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i) return;
    ir_ref ptr_ref = bc.value_map[val.lhs];
    if (ptr_ref == IR_UNUSED) return;
    ir_ref real_ptr = ir_ADD_A(bc.mem_param, ptr_ref);
    bc.value_map[i] = (val.type == ValueType::I64) ? ir_LOAD_I64(real_ptr) : ir_LOAD_I32(real_ptr);
}

void IRBridge::handleStore(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
    ir_ref ptr_ref = bc.value_map[val.lhs], val_ref = bc.value_map[val.rhs];
    if (ptr_ref == IR_UNUSED || val_ref == IR_UNUSED) return;
    ir_STORE(ir_ADD_A(bc.mem_param, ptr_ref), val_ref);
}

void IRBridge::handleF64Load(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    if (val.lhs < 0 || val.lhs >= (int)i) return;
    ir_ref ptr_ref = bc.value_map[val.lhs];
    if (ptr_ref == IR_UNUSED) return;
    ir_ref real_ptr = ir_ADD_A(bc.mem_param, ptr_ref);
    bc.value_map[i] = ir_LOAD_D(real_ptr);
}

void IRBridge::handleF64Store(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    if (val.lhs < 0 || val.rhs < 0) return;
    ir_ref ptr_ref = bc.value_map[val.lhs], val_ref = bc.value_map[val.rhs];
    if (ptr_ref == IR_UNUSED || val_ref == IR_UNUSED) return;
    ir_ref real_ptr = ir_ADD_A(bc.mem_param, ptr_ref);
    ir_STORE(real_ptr, val_ref);
}

// ----- F64 Arithmetic -----

void IRBridge::handleF64Add(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    if (val.lhs < 0 || val.rhs < 0) return;
    ir_ref l = bc.value_map[val.lhs], r = bc.value_map[val.rhs];
    if (l == IR_UNUSED || r == IR_UNUSED) return;
    bc.value_map[bc.current_index] = ir_ADD_D(l, r);
}

void IRBridge::handleF64Sub(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    if (val.lhs < 0 || val.rhs < 0) return;
    ir_ref l = bc.value_map[val.lhs], r = bc.value_map[val.rhs];
    if (l == IR_UNUSED || r == IR_UNUSED) return;
    bc.value_map[bc.current_index] = ir_SUB_D(l, r);
}

void IRBridge::handleF64Mul(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    if (val.lhs < 0 || val.rhs < 0) return;
    ir_ref l = bc.value_map[val.lhs], r = bc.value_map[val.rhs];
    if (l == IR_UNUSED || r == IR_UNUSED) return;
    bc.value_map[bc.current_index] = ir_MUL_D(l, r);
}

void IRBridge::handleF64Div(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    if (val.lhs < 0 || val.rhs < 0) return;
    ir_ref l = bc.value_map[val.lhs], r = bc.value_map[val.rhs];
    if (l == IR_UNUSED || r == IR_UNUSED) return;
    bc.value_map[bc.current_index] = ir_DIV_D(l, r);
}

// ----- Call / Special -----

void IRBridge::handleCall(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    std::string cname = val.callee_name;
    if (!cname.empty() && cname[0] >= '0' && cname[0] <= '9') cname = "func_" + cname;
    ir_ref name_ref = ir_str(ctx, cname.c_str());
    ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
    std::vector<ir_ref> arg_refs;
    for (int arg_id : val.operands)
        if (arg_id >= 0 && arg_id < (int)bc.value_map.size())
            arg_refs.push_back(bc.value_map[arg_id]);
    bc.value_map[i] = ir_CALL_N(IR_I32, func_ref, (uint32_t)arg_refs.size(), arg_refs.data());
    TRACE("  v%zu = Call(%s, %zu args)\n", i, val.callee_name.c_str(), val.operands.size());
}

void IRBridge::handleUnreachable(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    ir_TRAP();
    TRACE("  v%zu = Unreachable -> ir_TRAP()\n", bc.current_index);
}

void IRBridge::handleMemorySize(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    size_t i = bc.current_index;
    ir_ref name_ref = ir_str(ctx, "__wasm_memory_size");
    ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
    bc.value_map[i] = ir_CALL(IR_I32, func_ref);
}

void IRBridge::handleMemoryCopy(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    ir_ref name_ref = ir_str(ctx, "__wasm_memory_copy");
    ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
    ir_CALL_3(IR_VOID, func_ref, bc.value_map[val.operands[0]], bc.value_map[val.operands[1]], bc.value_map[val.operands[2]]);
}

void IRBridge::handleMemoryFill(BuildContext& bc, const Value& val) {
    ir_ctx* ctx = bc.ctx;
    ir_ref name_ref = ir_str(ctx, "__wasm_memory_fill");
    ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
    ir_CALL_3(IR_VOID, func_ref, bc.value_map[val.operands[0]], bc.value_map[val.operands[1]], bc.value_map[val.operands[2]]);
}

// ===== Dispatch Table =====

using HandlerFn = void (IRBridge::*)(BuildContext&, const Value&);

static const std::unordered_map<Op, HandlerFn> kDispatchTable = {
    { Op::Add,            &IRBridge::handleAdd },
    { Op::Sub,            &IRBridge::handleSub },
    { Op::Mul,            &IRBridge::handleMul },
    { Op::Div_S,          &IRBridge::handleDivS },
    { Op::Div_U,          &IRBridge::handleDivU },
    { Op::Rem_S,          &IRBridge::handleRemS },
    { Op::Rem_U,          &IRBridge::handleRemU },
    { Op::Eq,             &IRBridge::handleEq },
    { Op::Ne,             &IRBridge::handleNe },
    { Op::Lt_S,           &IRBridge::handleLtS },
    { Op::Lt_U,           &IRBridge::handleLtU },
    { Op::Gt_S,           &IRBridge::handleGtS },
    { Op::Gt_U,           &IRBridge::handleGtU },
    { Op::Le_S,           &IRBridge::handleLeS },
    { Op::Le_U,           &IRBridge::handleLeU },
    { Op::Ge_S,           &IRBridge::handleGeS },
    { Op::Ge_U,           &IRBridge::handleGeU },
    { Op::And,            &IRBridge::handleAnd },
    { Op::Or,             &IRBridge::handleOr },
    { Op::Xor,            &IRBridge::handleXor },
    { Op::Shl,            &IRBridge::handleShl },
    { Op::Shr_S,          &IRBridge::handleShrS },
    { Op::Shr_U,          &IRBridge::handleShrU },
    { Op::Eqz,            &IRBridge::handleEqz },
    { Op::Clz,            &IRBridge::handleClz },
    { Op::Ctz,            &IRBridge::handleCtz },
    { Op::Popcnt,         &IRBridge::handlePopcnt },
    { Op::I32WrapI64,     &IRBridge::handleI32WrapI64 },
    { Op::I64ExtendI32S,  &IRBridge::handleI64ExtendI32S },
    { Op::I64ExtendI32U,  &IRBridge::handleI64ExtendI32U },
    { Op::F64ConvertI32S, &IRBridge::handleF64ConvertI },
    { Op::F64ConvertI64S, &IRBridge::handleF64ConvertI },
    { Op::F64ConvertI32U, &IRBridge::handleF64ConvertI },
    { Op::F64ConvertI64U, &IRBridge::handleF64ConvertI },
    { Op::I32TruncF64S,   &IRBridge::handleI32TruncF64 },
    { Op::I32TruncF64U,   &IRBridge::handleI32TruncF64 },
    { Op::I64TruncF64S,   &IRBridge::handleI64TruncF64 },
    { Op::I64TruncF64U,   &IRBridge::handleI64TruncF64 },
    { Op::I32Const,       &IRBridge::handleI32Const },
    { Op::I64Const,       &IRBridge::handleI64Const },
    { Op::F64Const,       &IRBridge::handleF64Const },
    { Op::LocalGet,       &IRBridge::handleLocalGet },
    { Op::LocalSet,       &IRBridge::handleLocalSet },
    { Op::LocalTee,       &IRBridge::handleLocalTee },
    { Op::GlobalGet,      &IRBridge::handleGlobalGet },
    { Op::GlobalSet,      &IRBridge::handleGlobalSet },
    { Op::If,             &IRBridge::handleIf },
    { Op::Else,           &IRBridge::handleElse },
    { Op::End,            &IRBridge::handleEnd },
    { Op::Loop,           &IRBridge::handleLoop },
    { Op::Br_if,          &IRBridge::handleBrIf },
    { Op::Br,             &IRBridge::handleBr },    
    { Op::Phi,            &IRBridge::handlePhi },
    { Op::Return,         &IRBridge::handleReturn },
    { Op::Select,         &IRBridge::handleSelect },
    { Op::Load,           &IRBridge::handleLoad },
    { Op::Store,          &IRBridge::handleStore },
    { Op::F64Load,        &IRBridge::handleF64Load },
    { Op::F64Store,       &IRBridge::handleF64Store },
    { Op::F64Add,         &IRBridge::handleF64Add },
    { Op::F64Sub,         &IRBridge::handleF64Sub },
    { Op::F64Mul,         &IRBridge::handleF64Mul },
    { Op::F64Div,         &IRBridge::handleF64Div },
    { Op::Call,           &IRBridge::handleCall },
    { Op::Unreachable,    &IRBridge::handleUnreachable },
    { Op::MemorySize,     &IRBridge::handleMemorySize },
    { Op::MemoryCopy,     &IRBridge::handleMemoryCopy },
    { Op::MemoryFill,     &IRBridge::handleMemoryFill },
};

// ===== build() =====

IRFunction* IRBridge::build(const ValueIR& values,
                             const std::vector<ParamType>& paramTypes,
                             void* memory_base) {
    ir_ctx* ctx = ctx_;

    TRACE("--- paramTypes ---\n");
    for (size_t i = 0; i < paramTypes.size(); i++) {
        TRACE("  param[%zu] = %s\n", i,
            paramTypes[i] == ParamType::I64 ? "I64" :
            paramTypes[i] == ParamType::F64 ? "F64" : "I32");
    }
    TRACE("=== Starting IR Bridge Construction ===\n");
    TRACE("Total ValueIR entries: %zu\n\n", values.size());

    while (!if_stack.empty()) if_stack.pop();
    while (!loop_stack.empty()) loop_stack.pop_back();
    ir_init(ctx_, IR_FUNCTION, 128, 128);
    ctx_->ret_type = IR_I32;  // will be overridden for void functions

    ir_START();
    ir_ref start = ctx_->ir_base[1].op2;

    bool has_memory_ops = false;
    for (const auto& v : values) {
        if (v.op == Op::Load || v.op == Op::Store ||
            v.op == Op::F64Load || v.op == Op::F64Store) {
            has_memory_ops = true; break;
        }
    }

    ir_ref mem_param = IR_UNUSED;
    if (has_memory_ops) mem_param = ir_PARAM(IR_ADDR, "__mem", 1);

    std::vector<ir_ref> value_map(values.size(), IR_UNUSED);
    std::map<int, int> param_index_to_value_id;

    // 第一遍：收集 param
    for (size_t i = 0; i < values.size(); i++)
        if (values[i].op == Op::Param)
            param_index_to_value_id[values[i].paramIndex] = i;

    // 第二遍：建 ir_PARAM
    TRACE("--- Phase 1: Creating Parameters ---\n");
    for (auto& [param_idx, value_id] : param_index_to_value_id) {
        char name[32];
        snprintf(name, sizeof(name), "p%d", param_idx);
        ir_type t = IR_I32;
        if (param_idx < (int)paramTypes.size()) {
            if (paramTypes[param_idx] == ParamType::I64) t = IR_I64;
            else if (paramTypes[param_idx] == ParamType::F64) t = IR_DOUBLE;
        }
        int pos = has_memory_ops ? param_idx + 2 : param_idx + 1;
        ir_ref param_ref = ir_PARAM(t, name, pos);
        value_map[value_id] = param_ref;
        TRACE("v%d = Param(%d) -> ir_PARAM(\"%s\", %d) [node ref=%d]\n",
            value_id, param_idx, name, param_idx + 1, param_ref);
    }

    // 建 local_types
    std::set<int> local_indices;
    for (const auto& v : values) {
        if (v.op == Op::Param || v.op == Op::LocalGet ||
            v.op == Op::LocalSet || v.op == Op::LocalTee)
            local_indices.insert(v.paramIndex);
        if (v.op == Op::Phi && v.local_index >= 0)
            local_indices.insert(v.local_index);
    }
    std::unordered_map<int, ir_type> local_types;
    for (int idx : local_indices)
        local_types[idx] = (idx < (int)paramTypes.size() && paramTypes[idx] == ParamType::I64)
                           ? IR_I64 : IR_I32;

    // 建 local_vars
    std::unordered_map<int, ir_ref> local_vars;
    for (int idx : local_indices) {
        char name[32];
        if (idx >= 2000) {
            // wasm global variable
            snprintf(name, sizeof(name), "wasm_global_%d", idx - 2000);
        } else {
            snprintf(name, sizeof(name), "local_%d", idx);
        }
        ir_type t = local_types.count(idx) ? local_types[idx] : IR_I32;
        ir_ref var = ir_VAR(t, name);
        local_vars[idx] = var;
        auto it = param_index_to_value_id.find(idx);
        if (it != param_index_to_value_id.end()) {
            ir_VSTORE(var, value_map[it->second]);
        } else {
            ir_ref zero = ir_CONST_I32(0);
            ir_VSTORE(var, zero);
        }
    }

    // 建 global_vars (wasm globals)
    std::unordered_map<int, ir_ref> global_vars;
    for (const auto& v : values) {
        if ((v.op == Op::GlobalGet || v.op == Op::GlobalSet) && v.globalIndex >= 0) {
            if (!global_vars.count(v.globalIndex)) {
                char name[32];
                snprintf(name, sizeof(name), "wasm_global_%d", v.globalIndex);
                ir_ref var = ir_VAR(IR_I32, name);
                global_vars[v.globalIndex] = var;
            }
        }
    }

    // 主迴圈
    for (size_t i = 0; i < values.size(); i++) {
        const Value& val = values[i];
        TRACE("--- Processing v%zu ---\n", i);

        if (val.op == Op::Param) {
            auto it = param_index_to_value_id.find(val.paramIndex);
            if (it != param_index_to_value_id.end())
                value_map[i] = value_map[it->second];
            continue;
        }

        BuildContext bc{
            ctx_, values, paramTypes,
            value_map, local_vars, local_types,
            global_vars,
            mem_param, has_memory_ops, i
        };

        auto it = kDispatchTable.find(val.op);
        if (it != kDispatchTable.end())
            (this->*it->second)(bc, val);
        else
            fprintf(stderr, "Unsupported Op: %d\n", (int)val.op);
    }

    auto* fn = new IRFunction();
    fn->ctx = ctx_;
    fn->entry_ref = start;
    return fn;
}

void IRBridge::dump(IRFunction* fn) {
        // if (!fn || !fn->ctx) { fprintf(stderr, "Invalid IRFunction\n"); return; }
        // printf("\nIR Dump:\n");
    ir_dump(fn->ctx, stdout);
        // printf("\n\nDOT Format:\n");
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