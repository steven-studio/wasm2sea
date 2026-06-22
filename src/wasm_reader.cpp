#include "wasm_to_instr_seq_converter.hpp"
#include "wasm_reader.hpp"
#include <fstream>
#include <cstdio>
#include <cstdint>
#include <vector>

// Binaryen headers
#include "wasm.h"
#include "wasm-binary.h"
#include "wasm-builder.h"
#include "ir/module-utils.h"

using namespace wasm;

// 修改返回类型：从 InstrSeq 改为 vector<FunctionResult>
std::vector<FunctionResult> readWasmFile(const std::string& filename) {
    // 讀取檔案
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename.c_str());
        return {};
    }
    
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    
    if (buffer.empty()) {
        fprintf(stderr, "Error: Empty file\n");
        return {};
    }
    
    printf("Reading WASM file: %s (%zu bytes)\n", filename.c_str(), buffer.size());
    
    // 使用 Binaryen 解析
    Module module;
    
    try {
        // 使用 WasmBinaryReader
        WasmBinaryReader reader(
            module,
            FeatureSet::All,
            buffer
        );
        reader.read();
    } catch (ParseException& e) {
        fprintf(stderr, "Error parsing WASM: %s\n", e.text.c_str());
        return {};
    }
    
    // 轉換第一個函數（假設只有一個）
    if (module.functions.empty()) {
        fprintf(stderr, "Error: No functions in module\n");
        return {};
    }
    
    printf("Module has %zu functions\n", module.functions.size());
    
    // ✅ 步骤 1: 先构建函数索引到导出名的映射
    std::map<size_t, std::string> functionExports;
    
    printf("\n=== Debug: Building function export map ===\n");
    printf("Total exports: %zu\n", module.exports.size());

    size_t exportIndex = 0;
    for (auto& exp : module.exports) {
        printf("Export[%zu]: name='%s', kind=%d\n", 
            exportIndex, exp->name.str.data(), (int)exp->kind);

        if (exp->kind == ExternalKind::Function) {
            Name internalName = *exp->getInternalName();
            for (size_t i = 0; i < module.functions.size(); i++) {
                if (module.functions[i]->name == internalName) {
                    functionExports[i] = std::string(exp->name.str);
                    break;
                }
            }
        }
    }

    printf("=== Total mapped: %zu functions ===\n\n", functionExports.size());

    // 存储所有函数的转换结果
    std::vector<FunctionResult> results;

    for (size_t i = 0; i < module.functions.size(); i++) {
        if (!module.functions[i]) {
            fprintf(stderr, "Error: Null function at index %zu\n", i);
            return {};
        }

        Function* func = module.functions[i].get();

        if (!func) {
            fprintf(stderr, "Error: Function pointer is null\n");
            return {};
        }

        // 获取函数名（这部分你也缺少了）
        std::string funcName;
        // 优先：使用导出名
        if (functionExports.find(i) != functionExports.end()) {
            funcName = functionExports[i];
        }
        // 次选：使用内部名
        else if (func->name.is()) {
            funcName = std::string(func->name.str);
        }
        // 后备：使用索引
        else {
            funcName = "func_" + std::to_string(i);
        }

        printf("Processing function [%zu]: %s\n", i, funcName.c_str());

        // 获取参数数量
        size_t numParams = func->getNumParams();
        printf("Function has %zu parameters\n", numParams);
        
        // 使用 walker 轉換
        WasmToInstrSeqConverter converter;
        converter.modulePtr = &module;
        // converter.numParams = numParams;  // ← 删除这行，Visitor 不需要
        converter.visitExpression(func->body);  // ← 改用 visitExpression

        // 构建返回结果
        InstrSeq instrSeq;
        instrSeq.push_back({WasmOp::FuncInfo, (int)numParams});  // 第一条指令存储参数数量
        instrSeq.insert(instrSeq.end(), 
                    converter.instructions.begin(), 
                    converter.instructions.end());

        // ✅ 关键：创建 FunctionResult 并添加到 results
        FunctionResult funcResult;
        funcResult.name = funcName;           // ← 保存函数名
        funcResult.numParams = numParams;     // ← 保存参数数量
        funcResult.instructions = instrSeq;   // ← 保存指令序列
        fprintf(stderr, "DEBUG: filling paramTypes, numParams=%zu\n", func->getNumParams());
        for (size_t j = 0; j < func->getNumParams(); j++) {
            wasm::Type t = func->getLocalType(j);
            fprintf(stderr, "DEBUG: param[%zu] type=%s\n", j, t.toString().c_str());
            if (t == wasm::Type::i64)      funcResult.paramTypes.push_back(ParamType::I64);
            else if (t == wasm::Type::f64) funcResult.paramTypes.push_back(ParamType::F64);
            else                            funcResult.paramTypes.push_back(ParamType::I32);
        }
        fprintf(stderr, "DEBUG: paramTypes.size()=%zu\n", funcResult.paramTypes.size());

        results.push_back(funcResult);        // ← 添加到 results！
        
        printf("  Converted %zu instructions\n", instrSeq.size());
    }
    
    printf("Successfully converted %zu functions\n", results.size());
    
    return results;
}
