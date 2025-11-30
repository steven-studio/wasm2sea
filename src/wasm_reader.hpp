#pragma once
#include "wasm_instr.hpp"
#include <vector>
#include <string>

// 從 .wasm 檔案讀取指令
InstrSeq readWasmFile(const std::string& filename);
