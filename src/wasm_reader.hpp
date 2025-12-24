#pragma once
#include "wasm_instr.hpp"
#include <vector>
#include <string>

// 在文件开头添加这个结构体定义
struct FunctionResult {
    std::string name;        // 函数名
    size_t numParams;        // 参数数量
    InstrSeq instructions;   // 指令序列
};

// 從 .wasm 檔案讀取指令
std::vector<FunctionResult> readWasmFile(const std::string& filename);