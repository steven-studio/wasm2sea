#pragma once

#include <vector>

enum class WasmOp {
    FuncInfo,    // 新增：存储函数元信息（参数数量等）
    LocalGet,
    LocalSet,
    LocalTee,
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
    I32Eqz,

    I32And, I32Or, I32Xor,
    I32Shl, I32ShrS, I32ShrU,
    I32Rotl, I32Rotr,
    I32Clz, I32Ctz, I32Popcnt
};

struct Instr {
    WasmOp op;
    int operand;
};

struct InstrSeq {
    std::vector<Instr> instructions;
    size_t numParams = 0;  // 新增
    
    // 为了向后兼容
    void push_back(const Instr& instr) {
        instructions.push_back(instr);
    }

    // 添加 insert 方法
    void insert(std::vector<Instr>::iterator pos, 
                std::vector<Instr>::iterator first,
                std::vector<Instr>::iterator last) {
        instructions.insert(pos, first, last);
    }
    
    size_t size() const {
        return instructions.size();
    }

    bool empty() const {  // ← 确保有这个方法
        return instructions.empty();
    }
    
    auto begin() { return instructions.begin(); }
    auto end() { return instructions.end(); }
    auto begin() const { return instructions.begin(); }
    auto end() const { return instructions.end(); }
    
    Instr& operator[](size_t i) { return instructions[i]; }
    const Instr& operator[](size_t i) const { return instructions[i]; }
};
