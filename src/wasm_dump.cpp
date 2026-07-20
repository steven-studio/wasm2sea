#include "wasm_dump.hpp"
#include <cstdio>
#include <cstddef>

// ============================================================
// opToString: WasmOp -> 字串
//
// 用查表取代原本的巨大 switch-case。原本的 switch 遺漏了
// I32Rotl / I32Rotr 兩個 case（會靜默 fallback 到 default 印出
// "Unknown"，不會有任何編譯錯誤或警告提醒），改成查表後用
// static_assert 鎖住表格大小跟 enum WasmOp 的項目數一致：只要
// enum 之後新增/刪除項目卻忘記同步這張表，編譯就會直接失敗，
// 不會再默默印錯名字。
// ============================================================

static const char* const kWasmOpNames[] = {
    "FuncInfo",       // FuncInfo
    "LocalGet",       // LocalGet
    "LocalSet",       // LocalSet
    "LocalTee",       // LocalTee
    "I32Const",       // I32Const
    "I32Add",         // I32Add
    "I32Sub",         // I32Sub
    "I32Mul",         // I32Mul
    "I32DivS",        // I32DivS
    "I32DivU",        // I32DivU
    "I32RemS",        // I32RemS
    "I32RemU",        // I32RemU
    "I32Eq",          // I32Eq
    "I32Ne",          // I32Ne
    "I32LtS",         // I32LtS
    "I32LtU",         // I32LtU
    "I32GtS",         // I32GtS
    "I32GtU",         // I32GtU
    "I32LeS",         // I32LeS
    "I32LeU",         // I32LeU
    "I32GeS",         // I32GeS
    "I32GeU",         // I32GeU
    "I32Eqz",         // I32Eqz
    "I32And",         // I32And
    "I32Or",          // I32Or
    "I32Xor",         // I32Xor
    "I32Shl",         // I32Shl
    "I32ShrS",        // I32ShrS
    "I32ShrU",        // I32ShrU
    "I32Rotl",        // I32Rotl   ← 原本 switch 漏掉的 case，補上
    "I32Rotr",        // I32Rotr   ← 同上
    "I32Clz",         // I32Clz
    "I32Ctz",         // I32Ctz
    "I32Popcnt",      // I32Popcnt
    "Select",         // Select
    "If",             // If
    "Else",           // Else
    "End",            // End
    "Block",          // Block
    "Loop",           // Loop
    "Br",             // Br
    "Br_if",          // Br_if
    "BrTable",        // BrTable
    "Drop",           // Drop
    "GlobalGet",      // GlobalGet
    "GlobalSet",      // GlobalSet
    "Return",         // Return
    "MemorySize",     // MemorySize
    "MemoryCopy",     // MemoryCopy
    "MemoryFill",     // MemoryFill
    "F64Const",       // F64Const
    "F64Add",         // F64Add
    "F64Sub",         // F64Sub
    "F64Mul",         // F64Mul
    "F64Div",         // F64Div
    "F64Abs",         // F64Abs
    "F64Neg",         // F64Neg
    "F64Sqrt",        // F64Sqrt
    "F64Exp",         // F64Exp
    "F64Log",         // F64Log
    "F64Sin",         // F64Sin
    "F64Cos",         // F64Cos
    "F64Pow",         // F64Pow
    "F64Min",         // F64Min
    "F64Max",         // F64Max
    "F64Eq",          // F64Eq
    "F64Ne",          // F64Ne
    "F64Lt",          // F64Lt
    "F64Gt",          // F64Gt
    "F64Le",          // F64Le
    "F64Ge",          // F64Ge
    "F64ConvertI32S", // F64ConvertI32S
    "F64ConvertI32U", // F64ConvertI32U
    "I32TruncF64S",   // I32TruncF64S
    "I32TruncF64U",   // I32TruncF64U
    "F64Load",        // F64Load
    "F64Store",       // F64Store
    "I32Load",        // I32Load
    "I32Store",       // I32Store
    "I64Const",       // I64Const
    "I64Add",         // I64Add
    "I64Sub",         // I64Sub
    "I64Mul",         // I64Mul
    "I64DivS",        // I64DivS
    "I64DivU",        // I64DivU
    "I64RemS",        // I64RemS
    "I64RemU",        // I64RemU
    "I64And",         // I64And
    "I64Or",          // I64Or
    "I64Xor",         // I64Xor
    "I64Shl",         // I64Shl
    "I64ShrS",        // I64ShrS
    "I64ShrU",        // I64ShrU
    "I64Clz",         // I64Clz
    "I64Ctz",         // I64Ctz
    "I64Popcnt",      // I64Popcnt
    "I64Eqz",         // I64Eqz
    "I64Eq",          // I64Eq
    "I64Ne",          // I64Ne
    "I64LtS",         // I64LtS
    "I64LtU",         // I64LtU
    "I64GtS",         // I64GtS
    "I64GtU",         // I64GtU
    "I64LeS",         // I64LeS
    "I64LeU",         // I64LeU
    "I64GeS",         // I64GeS
    "I64GeU",         // I64GeU
    "I32WrapI64",     // I32WrapI64
    "I64ExtendI32S",  // I64ExtendI32S
    "I64ExtendI32U",  // I64ExtendI32U
    "F64ConvertI64S", // F64ConvertI64S
    "F64ConvertI64U", // F64ConvertI64U
    "I64TruncF64S",   // I64TruncF64S
    "I64TruncF64U",   // I64TruncF64U
    "I64Load",        // I64Load
    "I64Store",       // I64Store
    "Call",           // Call
    "Unreachable",    // Unreachable
    "Unsupported",    // Unsupported
};

// 編譯期防呆：如果之後在 enum 裡加/刪項目卻忘記同步這張表，
// 這裡會直接編譯失敗，而不是默默印出錯的名字。
static_assert(sizeof(kWasmOpNames) / sizeof(kWasmOpNames[0])
              == static_cast<size_t>(WasmOp::_Count),
              "kWasmOpNames must be kept in sync with enum WasmOp");

static const char* opToString(WasmOp op) {
    size_t idx = static_cast<size_t>(op);
    if (idx >= static_cast<size_t>(WasmOp::_Count)) return "Unknown";
    return kWasmOpNames[idx];
}

// ============================================================
// dumpInstr / dumpInstrSeq: 逐指令印出 InstrSeq 內容
//
// 只在編譯時定義了 WASM2SEA_ENABLE_DUMP 這個 flag 才會真正輸出
// 內容；flag 沒開時提供空實作，讓這兩個符號永遠存在、永遠可以
// 被呼叫（例如 main.cpp 裡被註解掉的 dumpInstrSeq(code) 隨時可以
// 解開來用），不會因為拿掉 flag 就變成 undefined reference。
//
// 開啟方式（CMake 範例）：
//   cmake .. -DCMAKE_CXX_FLAGS="-DWASM2SEA_ENABLE_DUMP"
// 或在 CMakeLists.txt 裡用 option() + target_compile_definitions
// 掛成正式的建置選項。
// ============================================================

#ifdef WASM2SEA_ENABLE_DUMP

void dumpInstr(const Instr& instr, int indent) {
    for (int i = 0; i < indent; i++) printf("  ");

    switch (instr.op) {
    case WasmOp::FuncInfo:
        printf("FuncInfo(numParams=%d)\n", instr.operand); break;
    case WasmOp::LocalGet:
        printf("LocalGet(%d)\n", instr.operand); break;
    case WasmOp::LocalSet:
        printf("LocalSet(%d)\n", instr.operand); break;
    case WasmOp::LocalTee:
        printf("LocalTee(%d)\n", instr.operand); break;
    case WasmOp::I32Const:
        printf("I32Const(%d)\n", instr.operand); break;
    case WasmOp::I64Const:
        printf("I64Const(%lld)\n", (long long)instr.i64operand); break;
    case WasmOp::F64Const:
        printf("F64Const(%g)\n", instr.foperand); break;
    case WasmOp::Br:
        printf("Br(depth=%d)\n", instr.operand); break;
    case WasmOp::Br_if:
        printf("Br_if(depth=%d)\n", instr.operand); break;
    case WasmOp::BrTable:
        printf("BrTable(depth=%d)\n", instr.operand); break;
    case WasmOp::Block:
        printf("Block(depth=%d)\n", instr.operand); break;
    case WasmOp::Call:
        printf("Call(func=%d)\n", instr.operand); break;
    case WasmOp::GlobalGet:
        printf("GlobalGet(%d)\n", instr.operand); break;
    case WasmOp::GlobalSet:
        printf("GlobalSet(%d)\n", instr.operand); break;
    case WasmOp::I32Load:
        printf("I32Load(offset=%d)\n", instr.operand); break;
    case WasmOp::I32Store:
        printf("I32Store(offset=%d)\n", instr.operand); break;
    case WasmOp::I64Load:
        printf("I64Load(offset=%d)\n", instr.operand); break;
    case WasmOp::I64Store:
        printf("I64Store(offset=%d)\n", instr.operand); break;
    case WasmOp::F64Load:
        printf("F64Load(offset=%d)\n", instr.operand); break;
    case WasmOp::F64Store:
        printf("F64Store(offset=%d)\n", instr.operand); break;
    case WasmOp::End:
        if (instr.operand == 0)      printf("End(loop)\n");
        else if (instr.operand == 1) printf("End(block)\n");
        else                         printf("End(if)\n");
        break;
    default:
        printf("%s\n", opToString(instr.op)); break;
    }
}

void dumpInstrSeq(const InstrSeq& seq) {
    printf("=== InstrSeq (total: %zu instructions) ===\n", seq.size());
    for (size_t i = 0; i < seq.size(); i++) {
        printf("[%3zu] ", i);
        dumpInstr(seq[i], 0);
    }
    printf("=== End of InstrSeq ===\n");
}

#else

void dumpInstr(const Instr&, int) {}
void dumpInstrSeq(const InstrSeq&) {}

#endif  // WASM2SEA_ENABLE_DUMP