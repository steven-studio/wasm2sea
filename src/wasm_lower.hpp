#pragma once

#include "wasm_instr.hpp"
#include "value_ir.hpp"

// 將簡化版 WASM 指令序列 Lower 成你的 SSA IR
ValueIR lowerWasmToSsa(const InstrSeq& code);
