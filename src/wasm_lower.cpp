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
MAKE_BINARY(F64Eq,   F64Eq,  I32)
MAKE_BINARY(F64Ne,   F64Ne,  I32)
MAKE_BINARY(F64Lt,   F64Lt,  I32)
MAKE_BINARY(F64Gt,   F64Gt,  I32)
MAKE_BINARY(F64Le,   F64Le,  I32)
MAKE_BINARY(F64Ge,   F64Ge,  I32)

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
MAKE_UNARY(F64Sqrt, F64Sqrt, F64)
MAKE_UNARY(F64Exp, F64Exp, F64)
MAKE_UNARY(F64Log, F64Log, F64)
MAKE_UNARY(F64Sin, F64Sin, F64)
MAKE_UNARY(F64Cos, F64Cos, F64)
MAKE_UNARY(F64Abs, F64Abs, F64)
MAKE_UNARY(F64Pow, F64Pow, F64)

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
    } else if (target.type == ControlFrame::Block &&
               !ctx.control_stack.empty() &&
               ctx.control_stack.back().type == ControlFrame::Loop) {
        // 只有當這個 br_if 是在 Loop 內部直接發生時，才是標準的
        // wasm 迴圈退出慣用法：block { loop { ...; br_if N (exit) } }
        // 如果目前最內層不是 Loop（例如身處另一個 Block 內），這個
        // br_if 只是 block-scoped 的一般跳轉（例如三元運算式用
        // block+br_if+br 模擬 if/else），不該被誤判成迴圈退出，
        // 否則會把外層迴圈的 control flow 接錯，造成 backedge 遺失。
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
    } else if (target.type == ControlFrame::Block) {
        // block-scoped 跳轉（非迴圈退出）：目前 ir_bridge 的
        // control-flow 重建機制尚未支援這種一般化的 block+br_if
        // 值合併模式（常見於 -O0 下編譯有回傳值的三元運算式）。
        // 保守處理：不生成任何跳轉節點，避免產生錯誤的 control
        // flow（寧可讓後續值可能不正確，也不能讓外層迴圈的
        // backedge 被破壞導致無限迴圈）。
        (void)cond;
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
    { WasmOp::F64Eq,          handle_F64Eq },
    { WasmOp::F64Ne,          handle_F64Ne },
    { WasmOp::F64Lt,          handle_F64Lt },
    { WasmOp::F64Gt,          handle_F64Gt },
    { WasmOp::F64Le,          handle_F64Le },
    { WasmOp::F64Ge,          handle_F64Ge },
    { WasmOp::F64Sqrt,        handle_F64Sqrt },
    { WasmOp::F64Exp,         handle_F64Exp },
    { WasmOp::F64Log,         handle_F64Log },
    { WasmOp::F64Sin,         handle_F64Sin },
    { WasmOp::F64Cos,         handle_F64Cos },
    { WasmOp::F64Pow,         handle_F64Pow },
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
// ============================================================
// 【演進脈絡總覽】這段邏輯是怎麼走到現在這個樣子的
// ============================================================
// 這是本檔案裡改動次數最多、也最複雜的一段邏輯，經過好幾輪迭代：
//
//  1. 最早只有 rewriteTernaryBlocks：只認得「有 LocalSet 收斂」的
//     兩層 block 三元運算式（例如 floyd-warshall 的
//     `path[i][j]<sum ? path[i][j] : sum`）。
//  2. 發現 nussinov 的 `if (i<j-1) {...} else {...}`（兩邊都是
//     Store，沒有 LocalSet 收斂）完全沒被辨識，於是放寬判斷條件，
//     改用 Br(depth=1) 這個結構性訊號取代「猜 LocalSet」，統一成
//     現在的 rewriteBlockBrIf，同時涵蓋有/無 LocalSet 收斂、有/無
//     else 的情況。
//  3. 發現短路 `&&`（例如 `if (j-1>=0 && i+1<n)`）會產生多個堆疊
//     的 guard，原本的邏輯只吃第一個，於是加上 findAllGuards，把
//     堆疊的 guard 合併成單一 And 提前求值（見下方 findAllGuards
//     的完整說明，含「為什麼不用巢狀 If」的取捨理由）。
//  4. 發現「guard 處理完之後的 body」需要遞迴再檢查一次，才能抓到
//     巢狀在裡面的其他 if/else（nussinov 真正卡住的地方），於是把
//     主掃描邏輯包成可遞迴呼叫的 rewriteSpan，但刻意把遞迴範圍限縮
//     在「已知是 guard/if-else 的 body」，不去碰泛用的 Loop 遞迴
//     （見下方 rewriteSpan 的完整說明，含 dstogov/ir segfault 的
//     教訓）。
//
// 這條路上也有一次失敗的嘗試：泛用地遞迴進入任意 Loop body、並用
// 巢狀 If + 複製 else body 處理堆疊 guard，對獨立最小案例是對的，
// 但在 nussinov 的規模下讓 dstogov/ir 的 register allocator
// segfault，保留在 experiment/recursive-guard-crashes-nussinov
// 分支，未合併進 main。
// ============================================================

// Pre-pass: 把 block+br_if 模擬 if/else 的慣用法改寫成原生 If/Else/End
// ============================================================
//
// 【這個函式在解決什麼問題】
// clang -O0 對「block-scoped、非迴圈退出」的條件式，一律用
// block(+block)+eqz+br_if(0) 這個慣用法編譯，涵蓋兩種形狀：
//
//   (a) 有 else（兩層巢狀 block）：
//         Block(outer)
//           Block(inner)
//             <cond>
//             Eqz
//             Br_if(0)      ; cond 為 false 就跳到 inner block 結尾
//             <then body>
//             Br(1)         ; then 執行完，跳過 else，到 outer 結尾
//           End(inner)       ; else 分支的進入點
//           <else body>
//         End(outer)
//
//   (b) 沒有 else（單層 block）：
//         Block
//           <cond>
//           Eqz
//           Br_if(0)        ; cond 為 false 就跳到 block 結尾（跳過整段 body）
//           <body>
//         End
//
// 我們自己既有的 handle_Br_if 只認得「迴圈退出」這一種 br_if 慣用法，
// 其餘 block-scoped 的 br_if 一律保守丟棄（不產生任何跳轉節點），
// 這會讓上述兩種形狀的 then/else 分支變成「無條件都執行」，是錯的。
// 這個 pre-pass 在 wasm_lower 主迴圈開始前，先把符合這兩種形狀的
// 片段改寫成原生的 If/(Else)/End，讓既有、已經驗證過的
// handle_If/handle_Else/handle_End（含合併 locals 的 Phi 邏輯）
// 直接處理，不需要再另外教 handle_Br_if 認得這些形狀。
//
// 【怎麼判斷 then/else 的邊界】
// 邊界的判斷不依賴「then/else 結尾是不是 LocalSet 收斂到同一個
// local」（這是舊版的作法，只認得「會產生一個值」的三元運算式），
// 而是直接找 `Br(depth=1)` 這個跳轉指令本身當作 then body 的結尾——
// 這是真正結構性的訊號，不管 then/else 兩邊是「產生一個值收斂到
// 共用 local」（三元運算式）還是「純粹各自的副作用，例如直接
// Store 到記憶體」（一般 if/else），這個訊號都成立，因此這個函式
// 同時涵蓋這兩種情況。
//
// - matchEnd(openIdx)：從一個 Block/Loop/If 的開啟位置往後找，
//   用深度計數找出它對應的 End 位置。
// - findGuard(start, limit)：在 [start, limit) 範圍內、depth==1
//   （相對於這段範圍本身）尋找第一個 Eqz+Br_if(0) 組合，作為
//   guard 的條件判斷點。
//
// 【已知限制：短路 && / || 的多重堆疊 guard 尚未支援】
// 若原始碼是 `if (A && B) {...}`，clang 會產生「兩個連續、都指向
// 同一個 block 結尾的 Eqz+Br_if(0)」（A 為 false 時跳一次，B 為
// false 時再跳一次，都跳到同一個進入點）。目前這個函式的
// findGuard 只找「第一個」guard 就當作邊界，第二個 guard 會被
// 誤當成 then/else body 的一部分照抄過去，導致更內層真正巢狀在
// 裡面的 if/else（例如 nussinov 的
// `if (j-1>=0 && i+1<n) { if (i<j-1) {...} else {...} }`）沒有
// 被正確辨識、對應的跳轉語意也跟著跑掉。
//
// 曾經嘗試過遞迴版本（用巢狀 If + 複製 else body 處理多重堆疊
// guard），對獨立的最小案例（單一短路 && + if/else）驗證正確，
// 但套用到 nussinov 這種「三層巢狀 guard、疊加 470 個 wasm local」
// 的規模時，會讓 dstogov/ir 的 register allocator 直接 segfault
// （ir_compute_live_ranges, ir_ra.c:1408，NULL ival），懷疑是
// dstogov/ir 本身在這種巢狀 Phi 鏈規模下的邊界案例，根因尚未找到。
// 該版本保存在 experiment/recursive-guard-crashes-nussinov 分支，
// 未合併進 main。nussinov 因此仍是已知失敗的 benchmark。
static InstrSeq rewriteBlockBrIf(const InstrSeq& in) {
    std::vector<Instr> src(in.begin(), in.end());
    std::vector<Instr> out;
    out.reserve(src.size());

    std::function<int(size_t)> matchEnd = [&](size_t openIdx) -> int {
        int depth = 1;
        for (size_t k = openIdx + 1; k < src.size(); k++) {
            WasmOp op = src[k].op;
            if (op == WasmOp::Block || op == WasmOp::Loop || op == WasmOp::If) {
                depth++;
            } else if (op == WasmOp::End) {
                depth--;
                if (depth == 0) return (int)k;
            }
        }
        return -1;
    };

    auto findGuard = [&](int start, int limit) -> std::pair<int, int> {
        int depth = 1;
        for (int k = start; k < limit; k++) {
            WasmOp op = src[k].op;
            if (op == WasmOp::Block || op == WasmOp::Loop || op == WasmOp::If) {
                depth++;
            } else if (op == WasmOp::End) {
                depth--;
            } else if (depth == 1 && op == WasmOp::I32Eqz &&
                       k + 1 < limit && src[k + 1].op == WasmOp::Br_if &&
                       src[k + 1].operand == 0) {
                return {k, k + 1};
            }
        }
        return {-1, -1};
    };

    // 找出從 start 開始、連續堆疊在一起的多個 guard（例如短路 &&
    // 產生的「Eqz+Br_if(0)」重複出現多次，中間只夾著下一個條件的
    // 計算，沒有其他 Block/Loop/If）。回傳：合併後改寫出來的指令
    // （把每一段 guard 的原始條件都原封不動接上，中間插入 I32And
    // 合併成一個值），以及最後一個 br_if 之後、真正 body 開始的
    // 位置。
    //
    // 這裡選擇「合併成單一 And 提前求值」而不是「巢狀 If、複製 else
    // 保留短路求值順序」，是因為後者在 nussinov 這種三層巢狀、
    // 470 個 wasm local 的規模下，會讓 dstogov/ir 的 register
    // allocator segfault（ir_compute_live_ranges, ir_ra.c:1408），
    // 根因尚未找到（相關嘗試保留在
    // experiment/recursive-guard-crashes-nussinov 分支）。
    //
    // 提前求值兩個條件、而非短路求值，語意上的差異只有：當第一個
    // 條件為 false、且第二個條件「求值本身會有副作用或可能出錯」
    // 時才會出問題。對於 nussinov 這種所有 guard 都是單純的整數
    // 比較（沒有副作用、不會出錯）而言，這個差異是安全的；但這不是
    // 一個通用、對任何 && 都成立的正確轉換，未來若遇到 guard 本身
    // 有副作用的情況，需要換回巢狀 If 的處理方式。
    auto findAllGuards = [&](int start, int limit) -> std::tuple<std::vector<Instr>, int> {
        std::vector<Instr> combined;
        int pos = start;
        int lastBrIfEnd = -1;

        while (true) {
            auto guardResult = findGuard(pos, limit);
            int eqzIdx = guardResult.first, brIfIdx = guardResult.second;
            if (brIfIdx < 0) break;

            // 這段 guard 的「原始條件」是 [pos, eqzIdx)，不含 Eqz 本身
            // （這樣才是 Eqz 取反前的原始布林值）。
            for (int k = pos; k < eqzIdx; k++) combined.push_back(src[k]);

            if (!combined.empty() && lastBrIfEnd >= 0) {
                // 這不是第一個 guard，跟前一個已收集的條件用 And 合併。
                Instr andIns; andIns.op = WasmOp::I32And; andIns.operand = 0;
                combined.push_back(andIns);
            }

            lastBrIfEnd = brIfIdx + 1;
            pos = brIfIdx + 1;
        }

        return {combined, lastBrIfEnd};
    };

    // rewriteSpan：對 [start, limit) 這段範圍做一次線性掃描改寫。
    // 包成一個可遞迴呼叫的函式，這樣「guard 處理完之後的 body」跟
    // 「兩層 if/else 的 then/else body」都能遞迴地再次檢查，找出
    // 巢狀在裡面的其他 guard 或 if/else（例如 nussinov 的
    // `if (j-1>=0 && i+1<n) { if (i<j-1) {...} else {...} }`，
    // 外層 guard 處理完，還要遞迴進去才能抓到內層真正的 if/else）。
    //
    // 刻意不遞迴進入 Loop 或其他無法辨識的 Block 內容——先前的嘗試
    // 曾經泛用地遞迴進入任意 Loop body，結果讓 dstogov/ir 的
    // register allocator 在 nussinov 這種規模下 segfault
    // （ir_ra.c:1408），根因未明。這裡改成只在「已經確定是 guard
    // 或 if/else 的 body/then/else」這幾個明確、範圍受限的地方
    // 遞迴，避免重蹈覆轍。
    std::function<std::vector<Instr>(int, int)> rewriteSpan =
        [&](int start, int limit) -> std::vector<Instr> {
        std::vector<Instr> localOut;
        int i = start;
        while (i < limit) {
            bool matched = false;

            if (src[i].op == WasmOp::Block && i + 1 < limit &&
                src[i + 1].op == WasmOp::Block) {
                int outerEnd = matchEnd(i);
                int innerEnd = (outerEnd >= 0 && outerEnd <= limit) ? matchEnd(i + 1) : -1;

                if (outerEnd >= 0 && outerEnd <= limit && innerEnd >= 0 && innerEnd < outerEnd) {
                    auto [condInstrs, brIfEnd] = findAllGuards(i + 2, innerEnd);

                    if (brIfEnd >= 0) {
                        int thenBr = -1;
                        for (int k = brIfEnd; k < innerEnd; k++) {
                            if (src[k].op == WasmOp::Br && src[k].operand == 1) {
                                thenBr = k;
                            }
                        }
                        if (thenBr == innerEnd - 1) {
                            localOut.insert(localOut.end(), condInstrs.begin(), condInstrs.end());
                            Instr ifIns; ifIns.op = WasmOp::If; ifIns.operand = 0;
                            localOut.push_back(ifIns);
                            auto thenBody = rewriteSpan(brIfEnd, thenBr);
                            localOut.insert(localOut.end(), thenBody.begin(), thenBody.end());
                            Instr elseIns; elseIns.op = WasmOp::Else; elseIns.operand = 0;
                            localOut.push_back(elseIns);
                            auto elseBody = rewriteSpan(innerEnd + 1, outerEnd);
                            localOut.insert(localOut.end(), elseBody.begin(), elseBody.end());
                            Instr endIns; endIns.op = WasmOp::End; endIns.operand = 2;
                            localOut.push_back(endIns);

                            i = outerEnd + 1;
                            matched = true;
                        }
                    }
                }
            }

            if (!matched && src[i].op == WasmOp::Block) {
                int blockEnd = matchEnd(i);
                if (blockEnd >= 0 && blockEnd <= limit) {
                    auto [condInstrs, brIfEnd] = findAllGuards(i + 1, blockEnd);
                    if (brIfEnd >= 0) {
                        localOut.insert(localOut.end(), condInstrs.begin(), condInstrs.end());
                        Instr ifIns; ifIns.op = WasmOp::If; ifIns.operand = 0;
                        localOut.push_back(ifIns);
                        auto body = rewriteSpan(brIfEnd, blockEnd);
                        localOut.insert(localOut.end(), body.begin(), body.end());
                        Instr endIns; endIns.op = WasmOp::End; endIns.operand = 0;
                        localOut.push_back(endIns);

                        i = blockEnd + 1;
                        matched = true;
                    }
                }
            }

            if (matched) continue;
            localOut.push_back(src[i]);
            i++;
        }
        return localOut;
    };

    out = rewriteSpan(0, (int)src.size());

    InstrSeq result;
    result.numParams = in.numParams;
    for (auto& ins : out) result.push_back(ins);
    return result;
}

ValueIR lowerWasmToSsa(const InstrSeq& code,
                        const std::vector<std::string>& funcNames) {
    InstrSeq code2 = rewriteBlockBrIf(code);
    LowerContext ctx(code2, funcNames);

    size_t start_idx = 0;
    if (!code2.empty() && code2[0].op == WasmOp::FuncInfo) {
        ctx.numParams = static_cast<size_t>(code2[0].operand);
        start_idx = 1;
    }
        // fprintf(stderr, "=== SSA Lowering ===\nnumParams = %zu\n\n", ctx.numParams);

    for (size_t i = start_idx; i < code2.size(); i++) {
        const Instr& ins = code2[i];
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