/**
 * ir_bridge.cpp -- lowers the flat SSA-style ValueIR (produced from a WASM
 * function body) into dstogov/ir's own graph representation.
 *
 * Design note: this file follows the same "one class per opcode" pattern
 * used by the Simple sea-of-nodes teaching compiler (see ReturnNode,
 * AddNode, etc. in com.seaofnodes.simple.node) instead of a flat table of
 * IRBridge member functions. Each Op gets its own small Node subclass
 * whose lower() method does exactly what the old handleXxx(BuildContext&,
 * const Value&) function used to do -- every body below is unchanged from
 * the original, only its home moved from "IRBridge member function picked
 * via a table of member-function pointers" to "virtual method on a
 * per-opcode class picked via a table of Node*". Semantics are identical.
 *
 * Two differences from Simple's Node, worth knowing:
 *  - Simple's Node owns graph edges (_inputs) directly; here the graph
 *    lives inside dstogov/ir's ir_ctx, addressed by integer ir_ref, so
 *    each lower() call reaches into BuildContext for it instead.
 *  - If/Loop/Br/Phi still coordinate through the same if_stack/loop_stack
 *    globals as before (unchanged) -- several Op values participate in
 *    building one control-flow region, so that state can't live on a
 *    single Node instance the way per-node data lives in Simple.
 *
 * The Node hierarchy and dispatch table live in an anonymous namespace so
 * the (very common) class names below -- IfNode, LoopNode, CallNode,
 * ReturnNode, etc. -- get internal linkage and can never collide with
 * same-named types elsewhere in the project.
 */
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
#include <unordered_set>
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

namespace {

// Base class for every ValueIR opcode handler. Mirrors Simple's Node: one
// subclass per operation, picked from a table instead of a switch/member
// dispatch. lower() writes the resulting ir_ref into
// bc.value_map[bc.current_index], same contract the old handleXxx had.
struct Node {
    virtual ~Node() = default;
    virtual void lower(BuildContext& bc, const Value& val) const = 0;
};

// ----- Arithmetic -----

struct AddNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i); return; }
        if (val.rhs < 0 || val.rhs >= (int)i) { fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i); return; }
        ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
        bc.value_map[i] = (val.type == ValueType::I64) ? ir_ADD_I64(lhs_ref, rhs_ref) : ir_ADD_I32(lhs_ref, rhs_ref);
        TRACE("  v%zu = Add(v%d, v%d) type=%s\n", i, val.lhs, val.rhs, val.type == ValueType::I64 ? "I64" : "I32");
    }
};

struct SubNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i); return; }
        if (val.rhs < 0 || val.rhs >= (int)i) { fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i); return; }
        ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
        bc.value_map[i] = (val.type == ValueType::I64) ? ir_SUB_I64(lhs_ref, rhs_ref) : ir_SUB_I32(lhs_ref, rhs_ref);
    }
};

struct MulNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i); return; }
        if (val.rhs < 0 || val.rhs >= (int)i) { fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i); return; }
        ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
        bc.value_map[i] = (val.type == ValueType::I64) ? ir_MUL_I64(lhs_ref, rhs_ref) : ir_MUL_I32(lhs_ref, rhs_ref);
    }
};

struct DivSNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
        bc.value_map[i] = (val.type == ValueType::I64) ? ir_DIV_I64(lhs_ref, rhs_ref) : ir_DIV_I32(lhs_ref, rhs_ref);
    }
};

struct DivUNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
        bc.value_map[i] = (val.type == ValueType::I64) ? ir_DIV_U64(lhs_ref, rhs_ref) : ir_DIV_U32(lhs_ref, rhs_ref);
    }
};

struct RemSNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
        bc.value_map[i] = (val.type == ValueType::I64) ? ir_MOD_I64(lhs_ref, rhs_ref) : ir_MOD_I32(lhs_ref, rhs_ref);
    }
};

struct RemUNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
        bc.value_map[i] = (val.type == ValueType::I64) ? ir_MOD_U64(lhs_ref, rhs_ref) : ir_MOD_U32(lhs_ref, rhs_ref);
    }
};

// ----- Comparison -----

struct EqNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
        bc.value_map[i] = ir_EQ(lhs_ref, rhs_ref);
        TRACE("  v%zu = Eq(v%d, v%d) -> ir_EQ(ref %d, ref %d) = ref %d\n\n", i, val.lhs, val.rhs, lhs_ref, rhs_ref, bc.value_map[i]);
    }
};

struct NeNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        bc.value_map[i] = ir_NE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct LtSNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        bc.value_map[i] = ir_LT(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct LtUNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        bc.value_map[i] = ir_ULT(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct GtSNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
        bc.value_map[i] = ir_GT(lhs_ref, rhs_ref);
        TRACE("  v%zu = Gt_S(v%d, v%d) -> ir_GT(ref %d, ref %d) = ref %d\n\n", i, val.lhs, val.rhs, lhs_ref, rhs_ref, bc.value_map[i]);
    }
};

struct GtUNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        bc.value_map[i] = ir_UGT(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct LeSNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        bc.value_map[i] = ir_LE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct LeUNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        bc.value_map[i] = ir_ULE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct GeSNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        bc.value_map[i] = ir_GE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct GeUNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        bc.value_map[i] = ir_UGE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

// ----- Bitwise -----

struct AndNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        bc.value_map[i] = ir_AND_I32(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct OrNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        bc.value_map[i] = ir_OR_I32(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct XorNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        bc.value_map[i] = ir_XOR_I32(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct ShlNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        bc.value_map[i] = ir_SHL_I32(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct ShrSNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        bc.value_map[i] = ir_SAR_I32(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct ShrUNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        bc.value_map[i] = ir_SHR_I32(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

// ----- Unary -----

struct EqzNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) return;
        bc.value_map[i] = ir_EQ(bc.value_map[val.lhs], ir_CONST_I32(0));
    }
};

struct ClzNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_CTLZ_I32(bc.value_map[val.lhs]);
    }
};

struct CtzNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_CTTZ_I32(bc.value_map[val.lhs]);
    }
};

struct PopcntNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_CTPOP_I32(bc.value_map[val.lhs]);
    }
};

// ----- Type Conversion -----

struct I32WrapI64Node : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_TRUNC_I32(bc.value_map[val.lhs]);
    }
};

struct I64ExtendI32SNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_SEXT_I64(bc.value_map[val.lhs]);
    }
};

struct I64ExtendI32UNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_ZEXT_I64(bc.value_map[val.lhs]);
    }
};

struct F64NegNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_NEG_D(bc.value_map[val.lhs]);
    }
};

struct F64EqNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.rhs < 0) return;
        bc.value_map[i] = ir_EQ(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct F64NeNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.rhs < 0) return;
        bc.value_map[i] = ir_NE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct F64LtNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.rhs < 0) return;
        bc.value_map[i] = ir_LT(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct F64GtNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.rhs < 0) return;
        bc.value_map[i] = ir_GT(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct F64LeNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.rhs < 0) return;
        bc.value_map[i] = ir_LE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct F64GeNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.rhs < 0) return;
        bc.value_map[i] = ir_GE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct F64SqrtNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        ir_ref name_ref = ir_str(ctx, "sqrt");
        ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
        bc.value_map[bc.current_index] = ir_CALL_1(IR_DOUBLE, func_ref, bc.value_map[val.lhs]);
    }
};

struct F64ExpNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        ir_ref name_ref = ir_str(ctx, "exp");
        ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
        bc.value_map[bc.current_index] = ir_CALL_1(IR_DOUBLE, func_ref, bc.value_map[val.lhs]);
    }
};

struct F64LogNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        ir_ref name_ref = ir_str(ctx, "log");
        ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
        bc.value_map[bc.current_index] = ir_CALL_1(IR_DOUBLE, func_ref, bc.value_map[val.lhs]);
    }
};

struct F64SinNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        ir_ref name_ref = ir_str(ctx, "sin");
        ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
        bc.value_map[bc.current_index] = ir_CALL_1(IR_DOUBLE, func_ref, bc.value_map[val.lhs]);
    }
};

struct F64CosNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        ir_ref name_ref = ir_str(ctx, "cos");
        ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
        bc.value_map[bc.current_index] = ir_CALL_1(IR_DOUBLE, func_ref, bc.value_map[val.lhs]);
    }
};

// NOTE: defined but never wired into kDispatchTable below -- same as the
// original handleF64Abs/handleF64Min/handleF64Max, which existed in the
// old dispatch-table .cpp but were never added to kDispatchTable either.
// Preserved as-is (dead code) to keep behavior identical; worth a look if
// F64 abs/min/max ever need to actually work.
struct F64AbsNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_ABS_D(bc.value_map[val.lhs]);
    }
};

struct F64MinNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_MIN_D(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct F64MaxNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_MAX_D(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

struct F64ConvertINode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_INT2D(bc.value_map[val.lhs]);
    }
};

struct I32TruncF64Node : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_FP2I32(bc.value_map[val.lhs]);
    }
};

struct I64TruncF64Node : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_FP2I64(bc.value_map[val.lhs]);
    }
};

// ----- Constants -----

struct I32ConstNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        ir_ref c = ir_CONST_I32(val.constValue);
        ir_ref copy = ir_COPY_I32(c);
        bc.value_map[i] = copy;
        TRACE(" v%zu = Const(%d) -> ir_CONST_I32 ref %d, ir_COPY ref %d\n\n", i, val.constValue, c, copy);
    }
};

struct I64ConstNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        ir_ref c = ir_CONST_I64(val.constValue);
        ir_ref copy = ir_COPY_I64(c);
        bc.value_map[i] = copy;
    }
};

struct F64ConstNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_CONST_DOUBLE(val.fconst);
    }
};

// ----- Locals -----

struct LocalGetNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        int var_idx = val.paramIndex;
        auto it = bc.local_vars.find(var_idx);
        if (it == bc.local_vars.end()) { fprintf(stderr, "ERROR: LocalGet for untracked local_%d\n", var_idx); return; }
        ir_ref var_ref = it->second;
        ir_type t = bc.local_types.count(var_idx) ? bc.local_types[var_idx] : IR_I32;
        ir_ref loaded = (t == IR_I64) ? ir_VLOAD_I64(var_ref)
                      : (t == IR_DOUBLE) ? ir_VLOAD_D(var_ref)
                      : ir_VLOAD_I32(var_ref);
        bc.value_map[i] = loaded;
        TRACE("  v%zu = LocalGet(local_%d) -> ref %d\n\n", i, var_idx, loaded);
    }
};

struct LocalSetNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        int var_idx = val.paramIndex;
        auto it = bc.local_vars.find(var_idx);
        if (it == bc.local_vars.end()) { fprintf(stderr, "ERROR: LocalSet for untracked local_%d\n", var_idx); return; }
        if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid value index %d for LocalSet\n", val.lhs); return; }
        ir_VSTORE(it->second, bc.value_map[val.lhs]);
        TRACE("  v%zu = LocalSet(local_%d, v%d)\n\n", i, var_idx, val.lhs);
    }
};

struct GlobalGetNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
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
};

struct GlobalSetNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
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
};

struct LocalTeeNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
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
};

// ----- Control Flow -----

struct IfNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
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
};

struct ElseNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (if_stack.empty()) { fprintf(stderr, "ERROR: Else without matching If\n"); return; }
        ir_ref end_true = ir_END();
        TRACE("  v%zu = Else -> ir_END (true branch) ref %d\n\n", i, end_true);
        if_stack.top().end_true = end_true;
        if_stack.top().has_else = true;
        ir_IF_FALSE(if_stack.top().if_node);
    }
};

struct EndNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
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
};

struct LoopNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
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
};

struct BrIfNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
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
        // find the target loop by loop_value_id (val.rhs)
        LoopInfo* target_loop_ptr = nullptr;
        for (int k = (int)loop_stack.size() - 1; k >= 0; k--) {
            if (loop_stack[k].loop_value_id == val.rhs) {
                target_loop_ptr = &loop_stack[k];
                break;
            }
        }
        if (!target_loop_ptr) target_loop_ptr = &loop_stack.back();
        LoopInfo& loop_info = *target_loop_ptr;
        for (int phi_id : loop_info.phi_ids) {
            const Value& phi_val = bc.values[phi_id];
            if (phi_val.operands.size() >= 2) {
                ir_ref phi_ref = bc.value_map[phi_id];
                ir_ref backedge_ref = bc.value_map[phi_val.operands[1]];
                if (phi_ref > 0 && backedge_ref > 0) {
                    // op1=control, op2=entry, op3=back-edge (IR_UNUSED placeholder)
                    // only set if op3 is still IR_UNUSED
                    ir_insn* phi_insn = &ctx->ir_base[phi_ref];
                    if (phi_insn->op3 == IR_UNUSED)
                        ir_PHI_SET_OP(phi_ref, 2, backedge_ref);
                    else
                        ir_PHI_SET_OP(phi_ref, 2, backedge_ref);
                }
            }
        }
        ir_IF_TRUE(if_node);
        ir_ref loop_end_ref = ir_LOOP_END();
        ir_MERGE_SET_OP(loop_info.loop_begin, 2, loop_end_ref);
        ir_IF_FALSE(if_node);
        TRACE("  v%zu = Br_if(cond=v%d) kind=loop_back\n", i, val.lhs);
    }
};

struct BrNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
            //
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
        if (!target_loop->exits.empty()) {
            ir_MERGE_2(loop_end_ref, target_loop->exits[0]);
        }
        loop_stack.pop_back();
        TRACE("  v%zu = Br -> LOOP_END\n\n", i);
    }
};

struct PhiNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
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
            //
                // 用 COPY_HARD 包住 outer PHI，讓 optimizer 不折疊
                ir_ref hard_copy = ir_emit2(ctx, IR_OPT(IR_COPY, IR_I32), outer_phi, IR_COPY_HARD);
                outer_loop.outer_carry_phis[local_idx] = outer_phi;
                ctx->control = inner_loop_begin;
                entry_val = outer_phi;
            }

            ir_ref inputs[2] = {entry_val, IR_UNUSED};
            ir_ref phi = ir_emit_N(ctx, IR_OPT(IR_PHI, IR_I32), 3);
            //
            ir_set_op(ctx, phi, 1, ctx->control);
            ir_set_op(ctx, phi, 2, entry_val);
            ir_set_op(ctx, phi, 3, IR_UNUSED);

            bc.value_map[i] = phi;
            if (!loop_stack.empty()) loop_stack.back().phi_ids.push_back(i);
            TRACE("  v%zu = Phi (Loop) -> ref %d\n\n", i, phi);
        } else {
            if (val.operands.size() != 2) { TRACE("    ERROR: If Phi should have exactly 2 operands\n\n"); return; }
            ir_ref op0 = bc.value_map[val.operands[0]];
            ir_ref op1 = bc.value_map[val.operands[1]];
            ir_type phi_type = (ir_type)ctx->ir_base[op0].type;
            ir_ref phi = ir_PHI_2(phi_type, op0, op1);
            bc.value_map[i] = phi;
            TRACE("  v%zu = Phi (If) -> ref %d\n\n", i, phi);
        }
    }
};

struct ReturnNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) {
            // void return
            ctx->ret_type = IR_VOID;
            ir_RETURN(IR_UNUSED);
        } else {
            ir_ref ret_val = bc.value_map[val.lhs];
            TRACE("  v%zu = Return(v%d) -> ir_RETURN(ref %d)\n\n", i, val.lhs, ret_val);
            ir_RETURN(ret_val);
        }
        if (!if_stack.empty() && !if_stack.top().has_else)
            if_stack.top().true_branch_returns = true;
    }
};

// ----- Select -----

struct SelectNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
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
};

// ----- Memory -----

ir_ref makeMemAddr(BuildContext& bc, ir_ref ptr_ref, int mem_offset) {
    ir_ctx* ctx = bc.ctx;   // 關鍵：ir_builder macro 需要這個名字
    ir_ref offset_ref = ptr_ref;
    if (mem_offset != 0) {
        offset_ref = ir_ADD_I32(ptr_ref, ir_CONST_I32(mem_offset));
    }
    return ir_ADD_A(bc.mem_param, offset_ref);
}

struct LoadNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) return;
        ir_ref ptr_ref = bc.value_map[val.lhs];
        if (ptr_ref == IR_UNUSED) return;
        ir_ref real_ptr = makeMemAddr(bc, ptr_ref, val.mem_offset);
        bc.value_map[i] = (val.type == ValueType::I64) ? ir_LOAD_I64(real_ptr) : ir_LOAD_I32(real_ptr);
    }
};

struct StoreNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        ir_ref ptr_ref = bc.value_map[val.lhs], val_ref = bc.value_map[val.rhs];
        if (ptr_ref == IR_UNUSED || val_ref == IR_UNUSED) return;
        ir_ref real_ptr = makeMemAddr(bc, ptr_ref, val.mem_offset);
        ir_STORE(real_ptr, val_ref);
    }
};

struct F64LoadNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) return;
        ir_ref ptr_ref = bc.value_map[val.lhs];
        if (ptr_ref == IR_UNUSED) return;
        ir_ref real_ptr = makeMemAddr(bc, ptr_ref, val.mem_offset);
        bc.value_map[i] = ir_LOAD_D(real_ptr);
    }
};

struct F64StoreNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        if (val.lhs < 0 || val.rhs < 0) return;
        ir_ref ptr_ref = bc.value_map[val.lhs], val_ref = bc.value_map[val.rhs];
        if (ptr_ref == IR_UNUSED || val_ref == IR_UNUSED) return;
        ir_ref real_ptr = makeMemAddr(bc, ptr_ref, val.mem_offset);
        ir_STORE(real_ptr, val_ref);
    }
};

// ----- F64 Arithmetic -----

struct F64AddNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        if (val.lhs < 0 || val.rhs < 0) return;
        ir_ref l = bc.value_map[val.lhs], r = bc.value_map[val.rhs];
        if (l == IR_UNUSED || r == IR_UNUSED) return;
        bc.value_map[bc.current_index] = ir_ADD_D(l, r);
    }
};

struct F64SubNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        if (val.lhs < 0 || val.rhs < 0) return;
        ir_ref l = bc.value_map[val.lhs], r = bc.value_map[val.rhs];
        if (l == IR_UNUSED || r == IR_UNUSED) return;
        bc.value_map[bc.current_index] = ir_SUB_D(l, r);
    }
};

struct F64MulNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        if (val.lhs < 0 || val.rhs < 0) return;
        ir_ref l = bc.value_map[val.lhs], r = bc.value_map[val.rhs];
        if (l == IR_UNUSED || r == IR_UNUSED) return;
        bc.value_map[bc.current_index] = ir_MUL_D(l, r);
    }
};

struct F64DivNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        if (val.lhs < 0 || val.rhs < 0) return;
        ir_ref l = bc.value_map[val.lhs], r = bc.value_map[val.rhs];
        if (l == IR_UNUSED || r == IR_UNUSED) return;
        bc.value_map[bc.current_index] = ir_DIV_D(l, r);
    }
};

// ----- Call / Special -----

struct CallNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
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
        // 回傳型別原本寫死 IR_I32，對標準 libm 數學函式（回傳 double）
        // 是錯的——這是今天 deriche 這個 kernel 才第一次踩到的情況
        // （之前只有 handleF64Sqrt/Exp/Log/Sin/Cos 這幾個「單參數、內建
        // 特判」的 handler 有正確處理 double 回傳，通用的 handleCall
        // 從未被要求處理過回傳 double 的外部呼叫）。
        // 這裡先用已知的標準數學函式名稱清單做特判；未來若有更多外部
        // 函式呼叫需要非 I32 回傳型別，需要從 wasm 的函式簽章正確推導，
        // 而不是繼續擴充這個清單。
        static const std::unordered_set<std::string> kDoubleReturningFuncs = {
            "exp", "expf", "pow", "powf", "log", "logf",
            "sin", "sinf", "cos", "cosf", "sqrt", "sqrtf",
            "tan", "tanf", "atan", "atanf", "atan2", "atan2f",
            "exp2", "exp2f", "log2", "log2f", "log10", "log10f",
            "fabs", "fabsf", "floor", "floorf", "ceil", "ceilf"
        };
        ir_type ret_type = kDoubleReturningFuncs.count(cname) ? IR_DOUBLE : IR_I32;
        bc.value_map[i] = ir_CALL_N(ret_type, func_ref, (uint32_t)arg_refs.size(), arg_refs.data());
        TRACE("  v%zu = Call(%s, %zu args)\n", i, val.callee_name.c_str(), val.operands.size());
    }
};

struct UnreachableNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        ir_TRAP();
        TRACE("  v%zu = Unreachable -> ir_TRAP()\n", bc.current_index);
    }
};

struct MemorySizeNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        ir_ref name_ref = ir_str(ctx, "__wasm_memory_size");
        ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
        bc.value_map[i] = ir_CALL(IR_I32, func_ref);
    }
};

struct MemoryCopyNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        ir_ref name_ref = ir_str(ctx, "__wasm_memory_copy");
        ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
        ir_CALL_3(IR_VOID, func_ref, bc.value_map[val.operands[0]], bc.value_map[val.operands[1]], bc.value_map[val.operands[2]]);
    }
};

struct MemoryFillNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        ir_ref name_ref = ir_str(ctx, "__wasm_memory_fill");
        ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
        ir_CALL_3(IR_VOID, func_ref, bc.value_map[val.operands[0]], bc.value_map[val.operands[1]], bc.value_map[val.operands[2]]);
    }
};

// ===== Dispatch Table =====
// Same role as the original member-function-pointer table: map each Op to
// the (stateless, singleton) Node instance that knows how to lower it.
const std::unordered_map<Op, Node*> kDispatchTable = {
    { Op::Add,            new AddNode() },
    { Op::Sub,            new SubNode() },
    { Op::Mul,            new MulNode() },
    { Op::Div_S,          new DivSNode() },
    { Op::Div_U,          new DivUNode() },
    { Op::Rem_S,          new RemSNode() },
    { Op::Rem_U,          new RemUNode() },
    { Op::Eq,             new EqNode() },
    { Op::Ne,             new NeNode() },
    { Op::Lt_S,           new LtSNode() },
    { Op::Lt_U,           new LtUNode() },
    { Op::Gt_S,           new GtSNode() },
    { Op::Gt_U,           new GtUNode() },
    { Op::Le_S,           new LeSNode() },
    { Op::Le_U,           new LeUNode() },
    { Op::Ge_S,           new GeSNode() },
    { Op::Ge_U,           new GeUNode() },
    { Op::And,            new AndNode() },
    { Op::Or,             new OrNode() },
    { Op::Xor,            new XorNode() },
    { Op::Shl,            new ShlNode() },
    { Op::Shr_S,          new ShrSNode() },
    { Op::Shr_U,          new ShrUNode() },
    { Op::Eqz,            new EqzNode() },
    { Op::Clz,            new ClzNode() },
    { Op::Ctz,            new CtzNode() },
    { Op::Popcnt,         new PopcntNode() },
    { Op::I32WrapI64,     new I32WrapI64Node() },
    { Op::I64ExtendI32S,  new I64ExtendI32SNode() },
    { Op::I64ExtendI32U,  new I64ExtendI32UNode() },
    { Op::F64Neg,         new F64NegNode() },
    { Op::F64Sqrt,        new F64SqrtNode() },
    { Op::F64Exp,         new F64ExpNode() },
    { Op::F64Log,         new F64LogNode() },
    { Op::F64Sin,         new F64SinNode() },
    { Op::F64Cos,         new F64CosNode() },
    { Op::F64Eq,          new F64EqNode() },
    { Op::F64Ne,          new F64NeNode() },
    { Op::F64Lt,          new F64LtNode() },
    { Op::F64Gt,          new F64GtNode() },
    { Op::F64Le,          new F64LeNode() },
    { Op::F64Ge,          new F64GeNode() },
    { Op::F64ConvertI32S, new F64ConvertINode() },
    { Op::F64ConvertI64S, new F64ConvertINode() },
    { Op::F64ConvertI32U, new F64ConvertINode() },
    { Op::F64ConvertI64U, new F64ConvertINode() },
    { Op::I32TruncF64S,   new I32TruncF64Node() },
    { Op::I32TruncF64U,   new I32TruncF64Node() },
    { Op::I64TruncF64S,   new I64TruncF64Node() },
    { Op::I64TruncF64U,   new I64TruncF64Node() },
    { Op::I32Const,       new I32ConstNode() },
    { Op::I64Const,       new I64ConstNode() },
    { Op::F64Const,       new F64ConstNode() },
    { Op::LocalGet,       new LocalGetNode() },
    { Op::LocalSet,       new LocalSetNode() },
    { Op::LocalTee,       new LocalTeeNode() },
    { Op::GlobalGet,      new GlobalGetNode() },
    { Op::GlobalSet,      new GlobalSetNode() },
    { Op::If,             new IfNode() },
    { Op::Else,           new ElseNode() },
    { Op::End,            new EndNode() },
    { Op::Loop,           new LoopNode() },
    { Op::Br_if,          new BrIfNode() },
    { Op::Br,             new BrNode() },
    { Op::Phi,            new PhiNode() },
    { Op::Return,         new ReturnNode() },
    { Op::Select,         new SelectNode() },
    { Op::Load,           new LoadNode() },
    { Op::Store,          new StoreNode() },
    { Op::F64Load,        new F64LoadNode() },
    { Op::F64Store,       new F64StoreNode() },
    { Op::F64Add,         new F64AddNode() },
    { Op::F64Sub,         new F64SubNode() },
    { Op::F64Mul,         new F64MulNode() },
    { Op::F64Div,         new F64DivNode() },
    { Op::Call,           new CallNode() },
    { Op::Unreachable,    new UnreachableNode() },
    { Op::MemorySize,     new MemorySizeNode() },
    { Op::MemoryCopy,     new MemoryCopyNode() },
    { Op::MemoryFill,     new MemoryFillNode() },
};

} // namespace

// 掃描整個 ValueIR，收集所有出現過的 local 變數索引：包含 wasm
// 函式參數（Op::Param）、所有被 LocalGet/LocalSet/LocalTee 用過的
// local，以及 loop-carried Phi 對應的 local_index。
static std::set<int> collectLocalIndices(const ValueIR& values) {
    std::set<int> local_indices;
    for (const auto& v : values) {
        if (v.op == Op::Param || v.op == Op::LocalGet ||
            v.op == Op::LocalSet || v.op == Op::LocalTee)
            local_indices.insert(v.paramIndex);
        if (v.op == Op::Phi && v.local_index >= 0)
            local_indices.insert(v.local_index);
    }
    return local_indices;
}

// 幫每個 local 推算它的 ir_type。函式參數直接用 paramTypes 裡宣告的
// 型別；其餘 local 沒有型別宣告，掃描所有對它的 LocalSet/LocalTee
// 賦值，用實際賦值來源的型別決定（而非一律假設 I32）。
static std::unordered_map<int, ir_type> buildLocalTypes(
        const ValueIR& values,
        const std::vector<ParamType>& paramTypes,
        const std::set<int>& local_indices) {
    std::unordered_map<int, ir_type> local_types;
    for (int idx : local_indices) {
        if (idx < (int)paramTypes.size() && paramTypes[idx] == ParamType::I64) {
            local_types[idx] = IR_I64;
        } else if (idx < (int)paramTypes.size() && paramTypes[idx] == ParamType::F64) {
            local_types[idx] = IR_DOUBLE;
        } else {
            // 非 param local：掃描所有 LocalSet/LocalTee 對這個 idx 的賦值，
            // 用實際賦值來源的型別決定 local 的型別（而非一律假設 I32）。
            ir_type inferred = IR_I32;
            for (const auto& v : values) {
                if ((v.op == Op::LocalSet || v.op == Op::LocalTee) &&
                    v.paramIndex == idx && v.lhs >= 0 && v.lhs < (int)values.size()) {
                    if (values[v.lhs].type == ValueType::F64) { inferred = IR_DOUBLE; break; }
                    if (values[v.lhs].type == ValueType::I64) { inferred = IR_I64; break; }
                }
            }
            local_types[idx] = inferred;
        }
    }
    return local_types;
}

// 掃描整個 ValueIR，找出每個 wasm 函式參數（Op::Param）對應的
// value id：param_idx -> value_id。用來在建立 ir_PARAM 節點、以及
// 後續判斷某個 local 是否為參數時查找。
static std::map<int, int> collectParamIndexToValueId(const ValueIR& values) {
    std::map<int, int> param_index_to_value_id;
    for (size_t i = 0; i < values.size(); i++)
        if (values[i].op == Op::Param)
            param_index_to_value_id[values[i].paramIndex] = i;
    return param_index_to_value_id;
}

// 掃描整個 ValueIR，找出哪些 local_idx 有被 LocalGet/LocalTee 讀取過。
// 用來判斷後面要不要幫這個 local 建立 ir_VAR（純 write-only、
// 從沒被讀過的 local 不需要建，見下方 local_vars 的建立）。
static std::unordered_set<int> collectReadLocals(const ValueIR& values) {
    std::unordered_set<int> read_locals;
    for (const auto& v : values) {
        if (v.op == Op::LocalGet || v.op == Op::LocalTee) {
            read_locals.insert(v.local_index);
        }
    }
    return read_locals;
}

// 掃描整個 ValueIR，判斷這個函式裡有沒有任何記憶體讀寫指令
// （Load/Store/F64Load/F64Store）。只有需要記憶體操作的函式才需要
// 額外的 __mem 參數（見下方 mem_param 的建立）。
static bool detectMemoryOps(const ValueIR& values) {
    for (const auto& v : values) {
        if (v.op == Op::Load || v.op == Op::Store ||
            v.op == Op::F64Load || v.op == Op::F64Store) {
            return true;
        }
    }
    return false;
}

// ===== build() =====

IRFunction* IRBridge::build(const ValueIR& values,
                             const std::vector<ParamType>& paramTypes,
                             const std::unordered_map<int, int32_t>& globalInitValues) {
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

    bool has_memory_ops = detectMemoryOps(values);

    ir_ref mem_param = IR_UNUSED;
    if (has_memory_ops) mem_param = ir_PARAM(IR_ADDR, "__mem", 1);

    std::vector<ir_ref> value_map(values.size(), IR_UNUSED);
    std::map<int, int> param_index_to_value_id;

    // 第一遍：收集 param
    param_index_to_value_id = collectParamIndexToValueId(values);

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

    std::set<int> local_indices = collectLocalIndices(values);
    std::unordered_map<int, ir_type> local_types = buildLocalTypes(values, paramTypes, local_indices);

    std::unordered_set<int> read_locals = collectReadLocals(values);

    // 建 local_vars
    std::unordered_map<int, ir_ref> local_vars;
    for (int idx : local_indices) {
        // param 一定要建（有被 VSTORE 初始化）
        bool is_param = param_index_to_value_id.count(idx) > 0;
        bool is_read  = read_locals.count(idx) > 0;
        if (!is_param && !is_read) continue;  // 純 write-only local，跳過

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
                ir_VSTORE(var, ir_CONST_I32(
                    globalInitValues.count(v.globalIndex)
                    ? globalInitValues.at(v.globalIndex) : 0));
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
            it->second->lower(bc, val);
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