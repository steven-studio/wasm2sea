#pragma once

#include <vector>

// 簡化版 WASM opcode 枚舉
enum class WasmOp {
    LocalGet,
    I32Const,
    I32Add,
    Return
};

// 一條指令：op + 一個整數參數
struct Instr {
    WasmOp op;
    int arg;   // local index 或 const value，用不到時填 0
};

// 之後如果你要從 wasm binary 解析，可以回傳 std::vector<Instr>
using InstrSeq = std::vector<Instr>;
