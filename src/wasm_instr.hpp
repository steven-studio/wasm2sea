#pragma once

#include <vector>

enum class WasmOp {
    LocalGet,
    LocalSet,
    I32Const,
    I32Add,
    I32Sub,
    I32Mul,
    Return,
    I32DivS,
    I32DivU,
    I32RemS,
    I32RemU,

    // 比較指令
    I32Eq,
    I32Ne,
    I32LtS,
    I32LtU,
    I32GtS,
    I32GtU,
    I32LeS,
    I32LeU,
    I32GeS,
    I32GeU,
    I32Eqz
};

struct WasmInstr {
    WasmOp op;
    int operand;  // 用於 LocalGet index, I32Const value 等
};

using InstrSeq = std::vector<WasmInstr>;