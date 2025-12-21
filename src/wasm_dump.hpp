#pragma once
#include "wasm_instr.hpp"

// 打印 InstrSeq 到 stdout
void dumpInstrSeq(const InstrSeq& seq);

// 打印单条指令（辅助函数）
void dumpInstr(const Instr& instr, int indent = 0);
