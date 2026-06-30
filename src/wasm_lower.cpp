#include "wasm_lower.hpp"
#include <unordered_map>
#include <unordered_set>
#include <functional>

// ============================================================
// LowerContext: 把所有 lowering 狀態集中在一個結構裡
// ============================================================

struct ControlFrame {
    enum Type { If, Loop, Block } type;
    int if_id = -1;
    int cond_id = -1;
    std::vector<int> then_values;
    std::vector<int> else_values;
    size_t stack_size = 0;
    int loop_start_id = -1;
    std::unordered_map<int, int> loop_phis;
    bool has_else = false;
    std::unordered_map<int, int> entry_locals;
    std::unordered_map<int, int> then_locals;
    std::unordered_map<int, int> else_locals;
    std::unordered_set<int> set_before_inner;
};

struct LowerContext {
    ValueIR values;
    std::vector<int> stack;
    std::unordered_map<int, int> localVars;
    std::unordered_map<int, int> globalVars;  // global index -> value id
    std::vector<ControlFrame> control_stack;
    const InstrSeq& code;
    const std::vector<std::string>& funcNames;
    size_t numParams = 0;

    LowerContext(const InstrSeq& code, const std::vector<std::string>& funcNames)
        : code(code), funcNames(funcNames) {}

    int newValue(Op op) {
        int id = static_cast<int>(values.size());
        Value v;
        v.id = id;
        v.op = op;
        values.push_back(v);
        return id;
    }

    int safePop() {
        if (stack.empty()) {
            int id = newValue(Op::I32Const);
            values[id].constValue = 0;
            return id;
        }
        int v = stack.back();
        stack.pop_back();
        return v;
    }

    // 建立二元運算節點，push 結果
    int emitBinary(Op op, ValueType type = ValueType::I32) {
        int rhs = safePop(), lhs = safePop();
        int id = newValue(op);
        values[id].lhs = lhs;
        values[id].rhs = rhs;
        values[id].type = type;
        stack.push_back(id);
        return id;
    }

    // 建立一元運算節點，push 結果
    int emitUnary(Op op, ValueType type = ValueType::I32) {
        int val = safePop();
        int id = newValue(op);
        values[id].lhs = val;
        values[id].type = type;
        stack.push_back(id);
        return id;
    }

    // 更新 loop phi 的 back-edge（Br / Br_if 共用）
    void updateLoopPhiBackedges(ControlFrame& target) {
        for (auto& [idx, phi_id] : target.loop_phis) {
            if (!localVars.count(idx)) continue;
            int cur = localVars[idx];
            bool already_has = false;
            for (int existing : values[phi_id].operands)
                if (existing == cur) { already_has = true; break; }
            if (!already_has)
                values[phi_id].operands.push_back(cur);
        }
    }
};

// ============================================================
// Handler 型別
// ============================================================

using HandlerFn = void(*)(LowerContext& ctx, const Instr& ins, size_t idx);

// ============================================================
// 算術 / 比較 / 位元運算（透過 emitBinary 統一處理）
// ============================================================

#define MAKE_BINARY(wasmOp, irOp, vtype) \
    static void handle_##wasmOp(LowerContext& ctx, const Instr&, size_t) { \
        ctx.emitBinary(Op::irOp, ValueType::vtype); \
    }

MAKE_BINARY(I32Add,  Add,   I32)
MAKE_BINARY(I32Sub,  Sub,   I32)
MAKE_BINARY(I32Mul,  Mul,   I32)
MAKE_BINARY(I32DivS, Div_S, I32)
MAKE_BINARY(I32DivU, Div_U, I32)
MAKE_BINARY(I32RemS, Rem_S, I32)
MAKE_BINARY(I32RemU, Rem_U, I32)
MAKE_BINARY(I32Eq,   Eq,    I32)
MAKE_BINARY(I32Ne,   Ne,    I32)
MAKE_BINARY(I32LtS,  Lt_S,  I32)
MAKE_BINARY(I32GtS,  Gt_S,  I32)
MAKE_BINARY(I32LeS,  Le_S,  I32)
MAKE_BINARY(I32GeS,  Ge_S,  I32)
MAKE_BINARY(I32LtU,  Lt_U,  I32)
MAKE_BINARY(I32GtU,  Gt_U,  I32)
MAKE_BINARY(I32LeU,  Le_U,  I32)
MAKE_BINARY(I32GeU,  Ge_U,  I32)
MAKE_BINARY(I32And,  And,   I32)
MAKE_BINARY(I32Or,   Or,    I32)
MAKE_BINARY(I32Xor,  Xor,   I32)
MAKE_BINARY(I32Shl,  Shl,   I32)
MAKE_BINARY(I32ShrS, Shr_S, I32)
MAKE_BINARY(I32ShrU, Shr_U, I32)
MAKE_BINARY(I64Add,  Add,   I64)
MAKE_BINARY(I64Sub,  Sub,   I64)
MAKE_BINARY(I64Mul,  Mul,   I64)
MAKE_BINARY(I64DivS, Div_S, I64)
MAKE_BINARY(I64RemS, Rem_S, I64)
MAKE_BINARY(F64Add,  F64Add, F64)
MAKE_BINARY(F64Sub,  F64Sub, F64)
MAKE_BINARY(F64Mul,  F64Mul, F64)
MAKE_BINARY(F64Div,  F64Div, F64)

#define MAKE_UNARY(wasmOp, irOp, vtype) \
    static void handle_##wasmOp(LowerContext& ctx, const Instr&, size_t) { \
        ctx.emitUnary(Op::irOp, ValueType::vtype); \
    }

MAKE_UNARY(I32Eqz,       Eqz,          I32)
MAKE_UNARY(I32Clz,       Clz,          I32)
MAKE_UNARY(I32WrapI64,   I32WrapI64,   I32)
MAKE_UNARY(I64ExtendI32S,I64ExtendI32S,I64)
MAKE_UNARY(I64ExtendI32U,I64ExtendI32U,I64)
MAKE_UNARY(F64ConvertI32S,F64ConvertI32S,I32)
MAKE_UNARY(F64ConvertI32U,F64ConvertI32U,I32)
MAKE_UNARY(F64ConvertI64S,F64ConvertI64S,I32)
MAKE_UNARY(F64ConvertI64U,F64ConvertI64U,I32)
MAKE_UNARY(I32TruncF64S, I32TruncF64S, I32)
MAKE_UNARY(I32TruncF64U, I32TruncF64U, I32)
MAKE_UNARY(I64TruncF64S, I64TruncF64S, I64)
MAKE_UNARY(I64TruncF64U, I64TruncF64U, I64)
MAKE_UNARY(F64Neg, F64Neg, F64)

// ============================================================
// 常數
// ============================================================

static void handle_I32Const(LowerContext& ctx, const Instr& ins, size_t) {
    int id = ctx.newValue(Op::I32Const);
    ctx.values[id].constValue = ins.operand;
    ctx.stack.push_back(id);
}

static void handle_I64Const(LowerContext& ctx, const Instr& ins, size_t) {
    int id = ctx.newValue(Op::I64Const);
    ctx.values[id].constValue = ins.i64operand;
    ctx.values[id].type = ValueType::I64;
    ctx.stack.push_back(id);
}

static void handle_F64Const(LowerContext& ctx, const Instr& ins, size_t) {
    int id = ctx.newValue(Op::F64Const);
    ctx.values[id].type = ValueType::F64;
    ctx.values[id].fconst = ins.foperand;
    ctx.stack.push_back(id);
}

// ============================================================
// Local Get / Set / Tee
// ============================================================

static void handle_LocalGet(LowerContext& ctx, const Instr& ins, size_t) {
    auto it = ctx.localVars.find(ins.operand);
    if (it != ctx.localVars.end()) {
        ctx.stack.push_back(it->second);
        return;
    }
    int id;
    if (ins.operand < (int)ctx.numParams) {
        id = ctx.newValue(Op::Param);
        ctx.values[id].paramIndex = ins.operand;
    } else {
        id = ctx.newValue(Op::I32Const);
        ctx.values[id].constValue = 0;
    }
    ctx.localVars[ins.operand] = id;
    ctx.stack.push_back(id);
}

static void handle_LocalSet(LowerContext& ctx, const Instr& ins, size_t) {
    if (ctx.stack.empty()) return;
    int val = ctx.stack.back();
    ctx.stack.pop_back();
    ctx.localVars[ins.operand] = val;
    int set_id = ctx.newValue(Op::LocalSet);
    ctx.values[set_id].paramIndex = ins.operand;
    ctx.values[set_id].lhs = val;
}

static void handle_GlobalGet(LowerContext& ctx, const Instr& ins, size_t) {
    int id = ctx.newValue(Op::GlobalGet);
    ctx.values[id].globalIndex = ins.operand;
    ctx.globalVars[ins.operand] = id;
    ctx.stack.push_back(id);
}

static void handle_GlobalSet(LowerContext& ctx, const Instr& ins, size_t) {
    if (ctx.stack.empty()) return;
    int val = ctx.stack.back();
    ctx.stack.pop_back();
    ctx.globalVars[ins.operand] = val;
    int set_id = ctx.newValue(Op::GlobalSet);
    ctx.values[set_id].globalIndex = ins.operand;
    ctx.values[set_id].lhs = val;
}

static void handle_LocalTee(LowerContext& ctx, const Instr& ins, size_t) {
    if (ctx.stack.empty()) return;
    ctx.localVars[ins.operand] = ctx.stack.back();
}

// ============================================================
// 控制流：If / Else / End
// ============================================================

static void handle_If(LowerContext& ctx, const Instr&, size_t) {
    if (ctx.stack.empty()) return;
    int cond = ctx.stack.back(); ctx.stack.pop_back();
    int if_id = ctx.newValue(Op::If);
    ctx.values[if_id].lhs = cond;
    ControlFrame frame;
    frame.type = ControlFrame::If;
    frame.if_id = if_id;
    frame.cond_id = cond;
    frame.stack_size = ctx.stack.size();
    frame.entry_locals = ctx.localVars;
    ctx.control_stack.push_back(std::move(frame));
}

static void handle_Else(LowerContext& ctx, const Instr&, size_t) {
    if (ctx.control_stack.empty()) return;
    auto& top = ctx.control_stack.back();
    top.then_locals = ctx.localVars;
    top.has_else = true;
    if (ctx.stack.size() > top.stack_size)
        top.then_values.push_back(ctx.stack.back()), ctx.stack.pop_back();
    ctx.localVars = top.entry_locals;
    ctx.newValue(Op::Else);
}

static void handle_End(LowerContext& ctx, const Instr&, size_t) {
    if (ctx.control_stack.empty()) return;
    if (ctx.control_stack.back().type == ControlFrame::If && !ctx.control_stack.back().has_else)
        ctx.control_stack.back().then_locals = ctx.localVars;

    ControlFrame frame = ctx.control_stack.back();
    ctx.control_stack.pop_back();

    if (frame.type == ControlFrame::Loop) {
        int end_id = ctx.newValue(Op::End);
        ctx.values[end_id].constValue = 0;
        return;
    }

    frame.else_locals = ctx.localVars;
    if (ctx.stack.size() > frame.stack_size)
        frame.else_values.push_back(ctx.stack.back()), ctx.stack.pop_back();

    int end_id = ctx.newValue(Op::End);
    ctx.values[end_id].constValue = (frame.type == ControlFrame::Block) ? 1 : 2;

    // expression-if 回傳值
    if (!frame.then_values.empty() && !frame.else_values.empty()) {
        int phi_id = ctx.newValue(Op::Phi);
        ctx.values[phi_id].local_index = -1;
        ctx.values[phi_id].operands = {frame.then_values[0], frame.else_values[0]};
        ctx.stack.push_back(phi_id);
    }

    // 合併被修改的 locals
    if (frame.has_else) {
        for (auto& [idx, then_val] : frame.then_locals) {
            auto else_it = frame.else_locals.find(idx);
            auto entry_it = frame.entry_locals.find(idx);
            int else_val = (else_it != frame.else_locals.end())
                ? else_it->second
                : (entry_it != frame.entry_locals.end() ? entry_it->second : then_val);
            if (then_val != else_val) {
                int phi_id = ctx.newValue(Op::Phi);
                ctx.values[phi_id].local_index = -1;
                ctx.values[phi_id].operands = {then_val, else_val};
                ctx.localVars[idx] = phi_id;
            }
        }
    } else {
        for (auto& [idx, then_val] : frame.then_locals) {
            auto entry_it = frame.entry_locals.find(idx);
            if (entry_it != frame.entry_locals.end() && entry_it->second != then_val) {
                int phi_id = ctx.newValue(Op::Phi);
                ctx.values[phi_id].local_index = -1;
                ctx.values[phi_id].operands = {then_val, entry_it->second};
                ctx.localVars[idx] = phi_id;
            }
        }
    }
}

// ============================================================
// 控制流：Loop / Block
// ============================================================

// 掃描 loop body 裡會被修改的 locals
struct LoopScanResult {
    std::unordered_set<int> modified_locals;
    std::unordered_set<int> read_before_write;
    std::unordered_set<int> set_before_inner;
};

static LoopScanResult scanLoopBody(const InstrSeq& code, size_t loop_start) {
    LoopScanResult res;
    std::unordered_set<int> seen_get;
    bool entered_inner = false;
    int depth = 1;

    for (size_t j = loop_start + 1; j < code.size() && depth > 0; j++) {
        if (code[j].op == WasmOp::Loop || code[j].op == WasmOp::Block || code[j].op == WasmOp::If) {
            depth++;
            if (depth == 2) entered_inner = true;
        } else if (code[j].op == WasmOp::End) {
            depth--;
        }
        if (depth == 1 && code[j].op == WasmOp::LocalGet)
            seen_get.insert(code[j].operand);
        if (depth == 1 && code[j].op == WasmOp::LocalSet) {
            res.modified_locals.insert(code[j].operand);
            if (!entered_inner) res.set_before_inner.insert(code[j].operand);
            if (seen_get.count(code[j].operand)) res.read_before_write.insert(code[j].operand);
        }
        if (depth > 1 && code[j].op == WasmOp::LocalSet) {
            if (!res.set_before_inner.count(code[j].operand))
                res.modified_locals.insert(code[j].operand);
        }
    }
    return res;
}

static void handle_Loop(LowerContext& ctx, const Instr&, size_t idx) {
    int loop_id = ctx.newValue(Op::Loop);
        // fprintf(stderr, "[LOOP START] v%d = Loop\n", loop_id);

    ControlFrame frame;
    frame.type = ControlFrame::Loop;
    frame.loop_start_id = loop_id;
    frame.stack_size = ctx.stack.size();

    // 確保 params 在 localVars 裡
    for (int i = 0; i < (int)ctx.numParams; i++) {
        if (!ctx.localVars.count(i)) {
            int id = ctx.newValue(Op::Param);
            ctx.values[id].paramIndex = i;
            ctx.localVars[i] = id;
        }
    }

    auto scan = scanLoopBody(ctx.code, idx);

        // fprintf(stderr, "[LOOP v%d] set_before_inner = {", loop_id);
        // for (int i : scan.set_before_inner) fprintf(stderr, " %d", i);
        // fprintf(stderr, " }\n[LOOP v%d] modified_locals = {", loop_id);
        // for (int i : scan.modified_locals) fprintf(stderr, " %d", i);
        // fprintf(stderr, " }\n");

    // 初始化未知 local
    for (int i : scan.modified_locals) {
        if (!ctx.localVars.count(i)) {
            int id = ctx.newValue(Op::I32Const);
            ctx.values[id].constValue = 0;
            ctx.localVars[i] = id;
        }
    }

    // 建 loop-carried PHI
    for (int i : scan.modified_locals) {
        if (scan.set_before_inner.count(i) && !scan.read_before_write.count(i)) continue;
        if (!scan.set_before_inner.count(i) && !scan.read_before_write.count(i)) continue;

        int entry_val = ctx.localVars[i];
        int phi_id = ctx.newValue(Op::Phi);
        ctx.values[phi_id].operands.push_back(entry_val);
        ctx.values[phi_id].local_index = i;

        // 檢查外層 loop 是否需要 VLOAD
        for (int k = (int)ctx.control_stack.size() - 1; k >= 0; k--) {
            if (ctx.control_stack[k].type == ControlFrame::Loop) {
                auto& outer = ctx.control_stack[k];
                if (outer.loop_phis.count(i) == 0 &&
                    !outer.set_before_inner.count(i)) {
                    ctx.values[phi_id].use_vload_entry = true;
                }
                break;  // 只看最近一層，不繼續往外找
            }
        }
        // fprintf(stderr, "  [PHI CREATE] v%d = Phi(v%d) for local_%d%s\n",
                // phi_id, entry_val, i,
                // ctx.values[phi_id].use_vload_entry ? " [use_vload]" : "");
        frame.loop_phis[i] = phi_id;
        ctx.localVars[i] = phi_id;
    }

    frame.set_before_inner = scan.set_before_inner;
    ctx.control_stack.push_back(std::move(frame));
}

static void handle_Block(LowerContext& ctx, const Instr&, size_t) {
    ControlFrame frame;
    frame.type = ControlFrame::Block;
    frame.stack_size = ctx.stack.size();
    frame.entry_locals = ctx.localVars;
    ctx.control_stack.push_back(std::move(frame));
}

// ============================================================
// 控制流：Br_if / Br / BrTable
// ============================================================

static void handle_Br_if(LowerContext& ctx, const Instr& ins, size_t) {
    if (ctx.stack.empty()) return;
    int cond = ctx.stack.back(); ctx.stack.pop_back();
    int depth = ins.operand;
    if (depth >= (int)ctx.control_stack.size()) return;

//    fprintf(stderr, "[BR_IF] depth=%d, control_stack size=%zu, target type=%d\n",
//            ins.operand, ctx.control_stack.size(),
//            (int)ctx.control_stack[ctx.control_stack.size() - 1 - ins.operand].type);

    ControlFrame& target = ctx.control_stack[ctx.control_stack.size() - 1 - depth];

    if (target.type == ControlFrame::Loop) {
        ctx.updateLoopPhiBackedges(target);
        int id = ctx.newValue(Op::Br_if);
        ctx.values[id].lhs = cond;
        ctx.values[id].rhs = target.loop_start_id;
        ctx.values[id].constValue = 0;
    } else if (target.type == ControlFrame::Block) {
        int block_idx = (int)ctx.control_stack.size() - 1 - depth;
        int outer_loop_start_id = -1;
        for (int k = block_idx + 1; k < (int)ctx.control_stack.size(); k++) {
            if (ctx.control_stack[k].type == ControlFrame::Loop) {
                outer_loop_start_id = ctx.control_stack[k].loop_start_id;
                break;
            }
        }
        // WASM: cond true = break; dstogov/ir: IF_TRUE = continue loop
        // Negate condition so IF_TRUE = continue, IF_FALSE = exit
        int neg_id = ctx.newValue(Op::Eqz);
        ctx.values[neg_id].lhs = cond;
        int id = ctx.newValue(Op::Br_if);
        ctx.values[id].lhs = neg_id;
        ctx.values[id].rhs = outer_loop_start_id;
        ctx.values[id].constValue = 1;
    }
}

static void handle_Br(LowerContext& ctx, const Instr& ins, size_t) {
    int depth = ins.operand;
    if (depth >= (int)ctx.control_stack.size()) return;
    ControlFrame& target = ctx.control_stack[ctx.control_stack.size() - 1 - depth];

        // fprintf(stderr, "[BR_DEBUG] depth=%d, target.type=%d, loop_phis size=%zu\n",
            // depth, target.type, target.loop_phis.size());

    if (target.type == ControlFrame::Loop) {
        ctx.updateLoopPhiBackedges(target);
        int br_id = ctx.newValue(Op::Br);
        ctx.values[br_id].lhs = target.loop_start_id;
        ctx.values[br_id].rhs = target.loop_phis.empty() ? -1 : target.loop_phis.begin()->second;
    } else {
        if (!ctx.stack.empty()) target.then_values.push_back(ctx.stack.back());
        int br_id = ctx.newValue(Op::Br);
        ctx.values[br_id].lhs = -1;
        ctx.values[br_id].rhs = -1;
    }
    ctx.stack.resize(target.stack_size);
}

static void handle_BrTable(LowerContext& ctx, const Instr&, size_t) {
    if (ctx.stack.empty()) return;
    ctx.safePop();
    int br_id = ctx.newValue(Op::Br);
    ctx.values[br_id].lhs = -1;
    ctx.stack.clear();
}

// ============================================================
// 其他
// ============================================================

static void handle_Select(LowerContext& ctx, const Instr&, size_t) {
    if (ctx.stack.size() < 3) return;
    int cond      = ctx.stack.back(); ctx.stack.pop_back();
    int true_val  = ctx.stack.back(); ctx.stack.pop_back();
    int false_val = ctx.stack.back(); ctx.stack.pop_back();
    int id = ctx.newValue(Op::Select);
    ctx.values[id].operands = {cond, true_val, false_val};
    ctx.stack.push_back(id);
}

static void handle_Return(LowerContext& ctx, const Instr&, size_t) {
    if (ctx.stack.empty()) return;
    int v = ctx.stack.back(); ctx.stack.pop_back();
    int id = ctx.newValue(Op::Return);
    ctx.values[id].lhs = v;
}

static void handle_Drop(LowerContext& ctx, const Instr&, size_t) {
    if (!ctx.stack.empty()) ctx.stack.pop_back();
}

static void handle_Call(LowerContext& ctx, const Instr& ins, size_t) {
    int num_args = (int)ins.foperand;
    int callee_idx = ins.operand;
    int id = ctx.newValue(Op::Call);
    ctx.values[id].lhs = callee_idx;
//    fprintf(stderr, "[handle_Call] callee_idx=%d, funcNames.size=%zu\n", callee_idx, ctx.funcNames.size());
//    for (size_t _i = 0; _i < ctx.funcNames.size(); _i++) fprintf(stderr, "  funcNames[%zu]=%s\n", _i, ctx.funcNames[_i].c_str());
    if (callee_idx >= 0 && callee_idx < (int)ctx.funcNames.size())
        ctx.values[id].callee_name = ctx.funcNames[callee_idx];
    std::vector<int> args(num_args);
    for (int i = num_args - 1; i >= 0; i--) args[i] = ctx.safePop();
    ctx.values[id].operands = args;
    ctx.stack.push_back(id);
}

static void handle_Unreachable(LowerContext& ctx, const Instr&, size_t) {
    ctx.newValue(Op::Unreachable);
}

static void handle_MemorySize(LowerContext& ctx, const Instr&, size_t) {
    int id = ctx.newValue(Op::MemorySize);
    ctx.stack.push_back(id);
}

static void handle_MemoryCopy(LowerContext& ctx, const Instr&, size_t) {
    int id = ctx.newValue(Op::MemoryCopy);
    ctx.values[id].operands.resize(3);
    ctx.values[id].operands[2] = ctx.safePop();
    ctx.values[id].operands[1] = ctx.safePop();
    ctx.values[id].operands[0] = ctx.safePop();
}

static void handle_MemoryFill(LowerContext& ctx, const Instr&, size_t) {
    int id = ctx.newValue(Op::MemoryFill);
    ctx.values[id].operands.resize(3);
    ctx.values[id].operands[2] = ctx.safePop();
    ctx.values[id].operands[1] = ctx.safePop();
    ctx.values[id].operands[0] = ctx.safePop();
}

static void handle_Load(LowerContext& ctx, const Instr& ins, size_t) {
    Op op = Op::Load;
    ValueType t = (ins.op == WasmOp::I64Load) ? ValueType::I64 : ValueType::I32;
    int ptr = ctx.safePop();
    int id = ctx.newValue(op);
    ctx.values[id].lhs = ptr;
    ctx.values[id].type = t;
    ctx.values[id].mem_offset = ins.operand;
    ctx.stack.push_back(id);
}

static void handle_F64Load(LowerContext& ctx, const Instr& ins, size_t) {
    int ptr = ctx.safePop();
    int id = ctx.newValue(Op::F64Load);
    ctx.values[id].lhs = ptr;
    ctx.values[id].type = ValueType::F64;
    ctx.values[id].mem_offset = ins.operand;
    ctx.stack.push_back(id);
}

static void handle_Store(LowerContext& ctx, const Instr& ins, size_t) {
    ValueType t = (ins.op == WasmOp::I64Store) ? ValueType::I64 : ValueType::I32;
    int val = ctx.safePop(), ptr = ctx.safePop();
    int id = ctx.newValue(Op::Store);
    ctx.values[id].lhs = ptr;
    ctx.values[id].rhs = val;
    ctx.values[id].type = t;
    ctx.values[id].mem_offset = ins.operand;
}

static void handle_F64Store(LowerContext& ctx, const Instr& ins, size_t) {
    int val = ctx.safePop(), ptr = ctx.safePop();
    int id = ctx.newValue(Op::F64Store);
    ctx.values[id].lhs = ptr;
    ctx.values[id].rhs = val;
    ctx.values[id].mem_offset = ins.operand;
}

static void handle_Unsupported(LowerContext& ctx, const Instr&, size_t) {
    int id = ctx.newValue(Op::I32Const);
    ctx.values[id].constValue = 0;
    ctx.stack.push_back(id);
}

// ============================================================
// Dispatch Table
// ============================================================

static const std::unordered_map<WasmOp, HandlerFn> kDispatch = {
    { WasmOp::I32Add,        handle_I32Add },
    { WasmOp::I32Sub,        handle_I32Sub },
    { WasmOp::I32Mul,        handle_I32Mul },
    { WasmOp::I32DivS,       handle_I32DivS },
    { WasmOp::I32DivU,       handle_I32DivU },
    { WasmOp::I32RemS,       handle_I32RemS },
    { WasmOp::I32RemU,       handle_I32RemU },
    { WasmOp::I32Eq,         handle_I32Eq },
    { WasmOp::I32Ne,         handle_I32Ne },
    { WasmOp::I32LtS,        handle_I32LtS },
    { WasmOp::I32GtS,        handle_I32GtS },
    { WasmOp::I32LeS,        handle_I32LeS },
    { WasmOp::I32GeS,        handle_I32GeS },
    { WasmOp::I32LtU,        handle_I32LtU },
    { WasmOp::I32GtU,        handle_I32GtU },
    { WasmOp::I32LeU,        handle_I32LeU },
    { WasmOp::I32GeU,        handle_I32GeU },
    { WasmOp::I32And,        handle_I32And },
    { WasmOp::I32Or,         handle_I32Or },
    { WasmOp::I32Xor,        handle_I32Xor },
    { WasmOp::I32Shl,        handle_I32Shl },
    { WasmOp::I32ShrS,       handle_I32ShrS },
    { WasmOp::I32ShrU,       handle_I32ShrU },
    { WasmOp::I32Eqz,        handle_I32Eqz },
    { WasmOp::I32Clz,        handle_I32Clz },
    { WasmOp::I32WrapI64,    handle_I32WrapI64 },
    { WasmOp::I64Add,        handle_I64Add },
    { WasmOp::I64Sub,        handle_I64Sub },
    { WasmOp::I64Mul,        handle_I64Mul },
    { WasmOp::I64DivS,       handle_I64DivS },
    { WasmOp::I64RemS,       handle_I64RemS },
    { WasmOp::I64ExtendI32S, handle_I64ExtendI32S },
    { WasmOp::I64ExtendI32U, handle_I64ExtendI32U },
    { WasmOp::F64Add,        handle_F64Add },
    { WasmOp::F64Sub,        handle_F64Sub },
    { WasmOp::F64Mul,        handle_F64Mul },
    { WasmOp::F64Div,        handle_F64Div },
    { WasmOp::F64Neg,         handle_F64Neg },
    { WasmOp::F64ConvertI32S,handle_F64ConvertI32S },
    { WasmOp::F64ConvertI32U,handle_F64ConvertI32U },
    { WasmOp::F64ConvertI64S,handle_F64ConvertI64S },
    { WasmOp::F64ConvertI64U,handle_F64ConvertI64U },
    { WasmOp::I32TruncF64S,  handle_I32TruncF64S },
    { WasmOp::I32TruncF64U,  handle_I32TruncF64U },
    { WasmOp::I64TruncF64S,  handle_I64TruncF64S },
    { WasmOp::I64TruncF64U,  handle_I64TruncF64U },
    { WasmOp::I32Const,      handle_I32Const },
    { WasmOp::I64Const,      handle_I64Const },
    { WasmOp::F64Const,      handle_F64Const },
    { WasmOp::GlobalGet,     handle_GlobalGet },
    { WasmOp::GlobalSet,     handle_GlobalSet },
    { WasmOp::LocalGet,      handle_LocalGet },
    { WasmOp::LocalSet,      handle_LocalSet },
    { WasmOp::LocalTee,      handle_LocalTee },
    { WasmOp::If,            handle_If },
    { WasmOp::Else,          handle_Else },
    { WasmOp::End,           handle_End },
    { WasmOp::Loop,          handle_Loop },
    { WasmOp::Block,         handle_Block },
    { WasmOp::Br_if,         handle_Br_if },
    { WasmOp::Br,            handle_Br },
    { WasmOp::BrTable,       handle_BrTable },
    { WasmOp::Select,        handle_Select },
    { WasmOp::Return,        handle_Return },
    { WasmOp::Drop,          handle_Drop },
    { WasmOp::Call,          handle_Call },
    { WasmOp::Unreachable,   handle_Unreachable },
    { WasmOp::MemorySize,    handle_MemorySize },
    { WasmOp::MemoryCopy,    handle_MemoryCopy },
    { WasmOp::MemoryFill,    handle_MemoryFill },
    { WasmOp::I32Load,       handle_Load },
    { WasmOp::I64Load,       handle_Load },
    { WasmOp::F64Load,       handle_F64Load },
    { WasmOp::I32Store,      handle_Store },
    { WasmOp::I64Store,      handle_Store },
    { WasmOp::F64Store,      handle_F64Store },
    { WasmOp::Unsupported,   handle_Unsupported },
};

// ============================================================
// Cleanup Phase: DCE + degenerate PHI removal
// ============================================================

static ValueIR cleanupValueIR(ValueIR& values) {
        // fprintf(stderr, "\n=== Cleanup Phase ===\n");

    // Step 1: 找退化 PHI
    std::unordered_map<int, int> replacements;
    for (size_t i = 0; i < values.size(); i++) {
        if (values[i].op == Op::Phi && values[i].operands.size() == 1) {
            replacements[i] = values[i].operands[0];
        // fprintf(stderr, "[DEGENERATE] v%d = Phi(v%d)\n", (int)i, values[i].operands[0]);
        }
    }

    // Step 2: 解析替換鏈
    std::function<int(int)> resolve = [&](int id) -> int {
        auto it = replacements.find(id);
        return (it != replacements.end()) ? resolve(it->second) : id;
    };

    // Step 3: 應用替換
    for (auto& v : values) {
        if (v.lhs != -1) v.lhs = resolve(v.lhs);
        if (v.rhs != -1) v.rhs = resolve(v.rhs);
        for (auto& op : v.operands) op = resolve(op);
    }

    // Step 4: 標記 live 節點
    std::vector<bool> used(values.size(), false);
    for (size_t i = 0; i < values.size(); i++) {
        switch (values[i].op) {
            case Op::Return: case Op::Store: case Op::F64Store:
            case Op::Loop: case Op::If: case Op::Else: case Op::End:
            case Op::Br_if: case Op::Br: case Op::LocalSet: case Op::LocalGet:
            case Op::GlobalGet: case Op::GlobalSet:
                used[i] = true; break;
            default: break;
        }
    }
    /* worklist-based mark: O(n) instead of O(n^2) */
    std::vector<int> worklist;
    for (size_t i = 0; i < values.size(); i++)
        if (used[i]) worklist.push_back((int)i);
    while (!worklist.empty()) {
        int idx = worklist.back(); worklist.pop_back();
        auto mark = [&](int ref) {
            if (ref >= 0 && ref < (int)values.size() && !used[ref]) {
                used[ref] = true;
                worklist.push_back(ref);
            }
        };
        mark(values[idx].lhs);
        mark(values[idx].rhs);
        for (int op : values[idx].operands) mark(op);
    }

    // Step 5: remap IDs
    std::vector<int> id_map(values.size(), -1);
    int new_id = 0;
    for (size_t i = 0; i < values.size(); i++) {
        if (used[i]) id_map[i] = new_id++;
        // else fprintf(stderr, "[DEAD] v%d removed\n", (int)i);
    }

    // Step 6: 重建
    ValueIR result;
    result.reserve(new_id);
    for (size_t i = 0; i < values.size(); i++) {
        if (!used[i]) continue;
        Value v = values[i];
        v.id = id_map[i];
        if (v.lhs != -1 && v.lhs < (int)id_map.size()) v.lhs = id_map[v.lhs];
        else if (v.lhs != -1) v.lhs = -1;
        if (v.rhs != -1 && v.rhs < (int)id_map.size()) v.rhs = id_map[v.rhs];
        else if (v.rhs != -1) v.rhs = -1;
        for (auto& op : v.operands) {
            if (op >= 0 && op < (int)id_map.size()) op = id_map[op];
            else op = -1;
        }
        result.push_back(v);
    }
        // fprintf(stderr, "[CLEANUP] %zu → %zu nodes\n\n", values.size(), result.size());
    return result;
}

// ============================================================
// 主入口
// ============================================================

ValueIR lowerWasmToSsa(const InstrSeq& code,
                        const std::vector<std::string>& funcNames) {
    LowerContext ctx(code, funcNames);

    size_t start_idx = 0;
    if (!code.empty() && code[0].op == WasmOp::FuncInfo) {
        ctx.numParams = static_cast<size_t>(code[0].operand);
        start_idx = 1;
    }
        // fprintf(stderr, "=== SSA Lowering ===\nnumParams = %zu\n\n", ctx.numParams);

    for (size_t i = start_idx; i < code.size(); i++) {
        const Instr& ins = code[i];
        auto it = kDispatch.find(ins.op);
        if (it != kDispatch.end()) {
            it->second(ctx, ins, i);
        } else if (ins.op != WasmOp::FuncInfo) {
            fprintf(stderr, "Unhandled WasmOp: %d\n", (int)ins.op);
        }
    }

    // implicit return
    if (!ctx.stack.empty()) {
        int v = ctx.stack.back();
        int id = ctx.newValue(Op::Return);
        ctx.values[id].lhs = v;
    } else {
        // void function: still need a Return node for CFG
        int id = ctx.newValue(Op::Return);
        ctx.values[id].lhs = -1;
    }

    return cleanupValueIR(ctx.values);
}