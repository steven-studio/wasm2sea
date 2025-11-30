#pragma once

#include <vector>

enum class WasmOp {
    LocalGet,
    LocalSet,
    I32Const,
    I32Add,
    I32Sub,
    I32Mul,
    Return
};

struct WasmInstr {
    WasmOp op;
    int operand;  // 用於 LocalGet index, I32Const value 等
};

using InstrSeq = std::vector<WasmInstr>;