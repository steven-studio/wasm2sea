#pragma once
#include "wasm_instr.hpp"
#include <vector>
#include <string>
#include <unordered_map>

enum class ParamType { I32, I64, F64, Other };  // ← 確保在最上面

// 在文件开头添加这个结构体定义
struct FunctionResult {
    std::string name;        // 函数名
    size_t numParams;        // 参数数量
    InstrSeq instructions;   // 指令序列
    std::vector<ParamType> paramTypes;  // 参数类型列表（可选）
    std::unordered_map<int, int32_t> globalInitValues;  // ← 新增
};

// 從 .wasm 檔案讀取指令
std::vector<FunctionResult> readWasmFile(const std::string& filename);
extern int g_wasm_global_count;

// 完整的函式名稱表，索引方式跟 wasm 原生的函式索引空間（import 在前，
// 自己定義的函式在後）完全一致，跟 getFunctionIndex() 用同一套索引。
// readWasmFile() 回傳的 std::vector<FunctionResult> 只包含「有 body」
// 的函式（略過 import），索引已經跟原生 wasm 函式索引不同步，因此
// Call 指令查詢 callee 名稱時不能用那份表，必須用這份完整表。
extern std::vector<std::string> g_all_function_names;