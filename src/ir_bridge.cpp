/**
 * ir_bridge.cpp -- lowers the flat SSA-style ValueIR (produced from a WASM
 * function body) into dstogov/ir's own graph representation.
 *
 * Design note: node/ mirrors the Simple sea-of-nodes teaching compiler's
 * com.seaofnodes.simple.node package -- one class per opcode, each in its
 * own file (node/AddNode.hpp, node/ReturnNode.hpp, ...), instead of a flat
 * table of IRBridge member functions. See node/Node.hpp for the base
 * class every opcode handler implements, node/BuildContext.hpp for the
 * per-build state threaded through lower(), and node/ControlFlowState.hpp
 * for the if_stack/loop_stack bookkeeping shared by the control-flow
 * opcodes (If/Else/End/Loop/Br/Br_if/Phi/Return).
 *
 * This file itself only wires the pieces together: #include every
 * node/XxxNode.hpp below, build the Op -> Node* dispatch table, and drive
 * the main ValueIR -> ir_ctx lowering loop in build(). Every lower() body
 * is unchanged from the original single-file version -- this pass was a
 * pure file-layout change, not a behavior change.
 *
 * Everything under node/ lives in namespace ir_node so common names
 * (AddNode, CallNode, ReturnNode, ...) can't collide with unrelated types
 * elsewhere in the project -- the C++ analogue of Simple's Java package.
 */
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

#include "node/ir_internal.hpp"
#include "node/Trace.hpp"
#include "node/BuildContext.hpp"
#include "node/ControlFlowState.hpp"
#include "node/MemAddr.hpp"
#include "node/Node.hpp"

// ----- Arithmetic -----
#include "node/AddNode.hpp"
#include "node/SubNode.hpp"
#include "node/MulNode.hpp"
#include "node/DivSNode.hpp"
#include "node/DivUNode.hpp"
#include "node/RemSNode.hpp"
#include "node/RemUNode.hpp"

// ----- Comparison -----
#include "node/EqNode.hpp"
#include "node/NeNode.hpp"
#include "node/LtSNode.hpp"
#include "node/LtUNode.hpp"
#include "node/GtSNode.hpp"
#include "node/GtUNode.hpp"
#include "node/LeSNode.hpp"
#include "node/LeUNode.hpp"
#include "node/GeSNode.hpp"
#include "node/GeUNode.hpp"

// ----- Bitwise -----
#include "node/AndNode.hpp"
#include "node/OrNode.hpp"
#include "node/XorNode.hpp"
#include "node/ShlNode.hpp"
#include "node/ShrSNode.hpp"
#include "node/ShrUNode.hpp"

// ----- Unary -----
#include "node/EqzNode.hpp"
#include "node/ClzNode.hpp"
#include "node/CtzNode.hpp"
#include "node/PopcntNode.hpp"

// ----- Type Conversion -----
#include "node/I32WrapI64Node.hpp"
#include "node/I64ExtendI32SNode.hpp"
#include "node/I64ExtendI32UNode.hpp"
#include "node/F64NegNode.hpp"
#include "node/F64EqNode.hpp"
#include "node/F64NeNode.hpp"
#include "node/F64LtNode.hpp"
#include "node/F64GtNode.hpp"
#include "node/F64LeNode.hpp"
#include "node/F64GeNode.hpp"
#include "node/F64SqrtNode.hpp"
#include "node/F64ExpNode.hpp"
#include "node/F64LogNode.hpp"
#include "node/F64SinNode.hpp"
#include "node/F64CosNode.hpp"
#include "node/F64AbsNode.hpp"
#include "node/F64MinNode.hpp"
#include "node/F64MaxNode.hpp"
#include "node/F64ConvertINode.hpp"
#include "node/I32TruncF64Node.hpp"
#include "node/I64TruncF64Node.hpp"

// ----- Constants -----
#include "node/I32ConstNode.hpp"
#include "node/I64ConstNode.hpp"
#include "node/F64ConstNode.hpp"

// ----- Locals -----
#include "node/LocalGetNode.hpp"
#include "node/LocalSetNode.hpp"
#include "node/GlobalGetNode.hpp"
#include "node/GlobalSetNode.hpp"
#include "node/LocalTeeNode.hpp"

// ----- Control Flow -----
#include "node/IfNode.hpp"
#include "node/ElseNode.hpp"
#include "node/EndNode.hpp"
#include "node/LoopNode.hpp"
#include "node/BrIfNode.hpp"
#include "node/BrNode.hpp"
#include "node/PhiNode.hpp"
#include "node/ReturnNode.hpp"

// ----- Select -----
#include "node/SelectNode.hpp"

// ----- Memory -----
#include "node/LoadNode.hpp"
#include "node/StoreNode.hpp"
#include "node/F64LoadNode.hpp"
#include "node/F64StoreNode.hpp"

// ----- F64 Arithmetic -----
#include "node/F64AddNode.hpp"
#include "node/F64SubNode.hpp"
#include "node/F64MulNode.hpp"
#include "node/F64DivNode.hpp"

// ----- Call / Special -----
#include "node/CallNode.hpp"
#include "node/UnreachableNode.hpp"
#include "node/MemorySizeNode.hpp"
#include "node/MemoryCopyNode.hpp"
#include "node/MemoryFillNode.hpp"

namespace ir_node {
std::stack<IfInfo> if_stack;
std::vector<LoopInfo> loop_stack;
}  // namespace ir_node

IRBridge::IRBridge() {
    ctx_ = (ir_ctx*)malloc(sizeof(ir_ctx));
    if (!ctx_) { fprintf(stderr, "Failed to allocate ir_ctx\n"); exit(1); }
}

IRBridge::~IRBridge() {
    if (ctx_) { ir_free(ctx_); free(ctx_); }
}

// ===== Dispatch Table =====
// Same role as the original member-function-pointer table: map each Op to
// the (stateless, singleton) Node instance that knows how to lower it.
namespace {
const std::unordered_map<Op, ir_node::Node*> kDispatchTable = {
    { Op::Add,            new ir_node::AddNode() },
    { Op::Sub,            new ir_node::SubNode() },
    { Op::Mul,            new ir_node::MulNode() },
    { Op::Div_S,          new ir_node::DivSNode() },
    { Op::Div_U,          new ir_node::DivUNode() },
    { Op::Rem_S,          new ir_node::RemSNode() },
    { Op::Rem_U,          new ir_node::RemUNode() },
    { Op::Eq,             new ir_node::EqNode() },
    { Op::Ne,             new ir_node::NeNode() },
    { Op::Lt_S,           new ir_node::LtSNode() },
    { Op::Lt_U,           new ir_node::LtUNode() },
    { Op::Gt_S,           new ir_node::GtSNode() },
    { Op::Gt_U,           new ir_node::GtUNode() },
    { Op::Le_S,           new ir_node::LeSNode() },
    { Op::Le_U,           new ir_node::LeUNode() },
    { Op::Ge_S,           new ir_node::GeSNode() },
    { Op::Ge_U,           new ir_node::GeUNode() },
    { Op::And,            new ir_node::AndNode() },
    { Op::Or,             new ir_node::OrNode() },
    { Op::Xor,            new ir_node::XorNode() },
    { Op::Shl,            new ir_node::ShlNode() },
    { Op::Shr_S,          new ir_node::ShrSNode() },
    { Op::Shr_U,          new ir_node::ShrUNode() },
    { Op::Eqz,            new ir_node::EqzNode() },
    { Op::Clz,            new ir_node::ClzNode() },
    { Op::Ctz,            new ir_node::CtzNode() },
    { Op::Popcnt,         new ir_node::PopcntNode() },
    { Op::I32WrapI64,     new ir_node::I32WrapI64Node() },
    { Op::I64ExtendI32S,  new ir_node::I64ExtendI32SNode() },
    { Op::I64ExtendI32U,  new ir_node::I64ExtendI32UNode() },
    { Op::F64Neg,         new ir_node::F64NegNode() },
    { Op::F64Sqrt,        new ir_node::F64SqrtNode() },
    { Op::F64Exp,         new ir_node::F64ExpNode() },
    { Op::F64Log,         new ir_node::F64LogNode() },
    { Op::F64Sin,         new ir_node::F64SinNode() },
    { Op::F64Cos,         new ir_node::F64CosNode() },
    { Op::F64Eq,          new ir_node::F64EqNode() },
    { Op::F64Ne,          new ir_node::F64NeNode() },
    { Op::F64Lt,          new ir_node::F64LtNode() },
    { Op::F64Gt,          new ir_node::F64GtNode() },
    { Op::F64Le,          new ir_node::F64LeNode() },
    { Op::F64Ge,          new ir_node::F64GeNode() },
    { Op::F64ConvertI32S, new ir_node::F64ConvertINode() },
    { Op::F64ConvertI64S, new ir_node::F64ConvertINode() },
    { Op::F64ConvertI32U, new ir_node::F64ConvertINode() },
    { Op::F64ConvertI64U, new ir_node::F64ConvertINode() },
    { Op::I32TruncF64S,   new ir_node::I32TruncF64Node() },
    { Op::I32TruncF64U,   new ir_node::I32TruncF64Node() },
    { Op::I64TruncF64S,   new ir_node::I64TruncF64Node() },
    { Op::I64TruncF64U,   new ir_node::I64TruncF64Node() },
    { Op::I32Const,       new ir_node::I32ConstNode() },
    { Op::I64Const,       new ir_node::I64ConstNode() },
    { Op::F64Const,       new ir_node::F64ConstNode() },
    { Op::LocalGet,       new ir_node::LocalGetNode() },
    { Op::LocalSet,       new ir_node::LocalSetNode() },
    { Op::LocalTee,       new ir_node::LocalTeeNode() },
    { Op::GlobalGet,      new ir_node::GlobalGetNode() },
    { Op::GlobalSet,      new ir_node::GlobalSetNode() },
    { Op::If,             new ir_node::IfNode() },
    { Op::Else,           new ir_node::ElseNode() },
    { Op::End,            new ir_node::EndNode() },
    { Op::Loop,           new ir_node::LoopNode() },
    { Op::Br_if,          new ir_node::BrIfNode() },
    { Op::Br,             new ir_node::BrNode() },
    { Op::Phi,            new ir_node::PhiNode() },
    { Op::Return,         new ir_node::ReturnNode() },
    { Op::Select,         new ir_node::SelectNode() },
    { Op::Load,           new ir_node::LoadNode() },
    { Op::Store,          new ir_node::StoreNode() },
    { Op::F64Load,        new ir_node::F64LoadNode() },
    { Op::F64Store,       new ir_node::F64StoreNode() },
    { Op::F64Add,         new ir_node::F64AddNode() },
    { Op::F64Sub,         new ir_node::F64SubNode() },
    { Op::F64Mul,         new ir_node::F64MulNode() },
    { Op::F64Div,         new ir_node::F64DivNode() },
    { Op::Call,           new ir_node::CallNode() },
    { Op::Unreachable,    new ir_node::UnreachableNode() },
    { Op::MemorySize,     new ir_node::MemorySizeNode() },
    { Op::MemoryCopy,     new ir_node::MemoryCopyNode() },
    { Op::MemoryFill,     new ir_node::MemoryFillNode() },
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

    while (!ir_node::if_stack.empty()) ir_node::if_stack.pop();
    while (!ir_node::loop_stack.empty()) ir_node::loop_stack.pop_back();
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

        ir_node::BuildContext bc{
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
