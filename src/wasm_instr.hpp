#pragma once

#include <vector>
#include <cstdint>

enum class WasmOp {
    FuncInfo,    // 新增：存储函数元信息（参数数量等）
    LocalGet,
    LocalSet,
    LocalTee,
    I32Const,
    I32Add,
    I32Sub,
    I32Mul,
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
    I32Clz, I32Ctz, I32Popcnt,

    Select,   // 新增

    // 控制流（新增）
    If,
    Else,
    End,

    // 新增
    Block,
    Loop,
    Br,
    Br_if,
    BrTable,

    Drop,
    GlobalGet,
    GlobalSet,

    Return,
    MemorySize,
    MemoryCopy,
    MemoryFill,
    // F64 算術
    F64Const,
    F64Add, F64Sub, F64Mul, F64Div,
    F64Abs, F64Neg, F64Sqrt,
    F64Min, F64Max,
    // F64 比較
    F64Eq, F64Ne, F64Lt, F64Gt, F64Le, F64Ge,
    // F64 轉換
    F64ConvertI32S, F64ConvertI32U,
    I32TruncF64S, I32TruncF64U,
    // Memory
    F64Load, F64Store,
    I32Load, I32Store,

    // i64 算術
    I64Const,
    I64Add, I64Sub, I64Mul,
    I64DivS, I64DivU,
    I64RemS, I64RemU,

    // i64 位元
    I64And, I64Or, I64Xor,
    I64Shl, I64ShrS, I64ShrU,
    I64Clz, I64Ctz, I64Popcnt,
    I64Eqz,

    // i64 比較
    I64Eq, I64Ne,
    I64LtS, I64LtU,
    I64GtS, I64GtU,
    I64LeS, I64LeU,
    I64GeS, I64GeU,

    // i64 型別轉換
    I32WrapI64,
    I64ExtendI32S, I64ExtendI32U,
    F64ConvertI64S, F64ConvertI64U,
    I64TruncF64S, I64TruncF64U,

    // i64 memory
    I64Load, I64Store,

    Call,       // operand = callee func index, foperand = num_args
    Unreachable,
    Unsupported
};

struct Instr {
    WasmOp op = WasmOp::Unsupported;
    int operand = 0;
    double foperand = 0.0;
    int64_t i64operand = 0;
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
