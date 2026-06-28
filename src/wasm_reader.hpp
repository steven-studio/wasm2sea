#pragma once
#include "wasm_instr.hpp"
#include <vector>
#include <string>

enum class ParamType { I32, I64, F64, Other };  // ← 確保在最上面

// 在文件开头添加这个结构体定义
struct FunctionResult {
    std::string name;        // 函数名
    size_t numParams;        // 参数数量
    InstrSeq instructions;   // 指令序列
    std::vector<ParamType> paramTypes;  // 参数类型列表（可选）
};

// 從 .wasm 檔案讀取指令
std::vector<FunctionResult> readWasmFile(const std::string& filename);
extern int g_wasm_global_count;