#pragma once

#include <vector>
#include <string>

enum class Op {
    Param,     // 對應 local.get
    I32Const,     // 對應 i32.const
    I64Const,     // 新增：對應 i64.const
    Add,       // 對應 i32.add
    Sub,       // 對應 i32.sub
    Mul,       // 對應 i32.mul
    Div_S,     // 新增：有號除法
    Div_U,     // 新增：無號除法
    Rem_S,     // 新增：有號取餘
    Rem_U,     // 新增：無號取餘

    // 比較指令（新增）
    Eq,       // ==
    Ne,       // !=
    Lt_S,     // < (signed)
    Lt_U,     // < (unsigned)
    Gt_S,     // > (signed)
    Gt_U,     // > (unsigned)
    Le_S,     // <= (signed)
    Le_U,     // <= (unsigned)
    Ge_S,     // >= (signed)
    Ge_U,     // >= (unsigned)
    Eqz,      // == 0 (特殊，只有一個操作數)

    // 位運算
    And, Or, Xor,
    Shl, Shr_S, Shr_U,
    Rotl, Rotr,
    
    // 一元位運算
    Clz, Ctz, Popcnt,

    // 局部变量操作（新增）
    LocalGet,    // 读取局部变量
    LocalSet,    // 设置局部变量（不返回值）
    LocalTee,    // 设置局部变量并返回值
    GlobalGet,   // 读取 wasm global
    GlobalSet,   // 写入 wasm global

    Select,   // 新增：三元运算符

    // 控制流（新增）
    If,           // if 开始
    Else,         // else 分支
    End,          // end 结束

    // 新增循环
    Loop,      // 循环开始
    Br,        // 无条件跳转
    Br_if,     // 条件跳转
    Phi,      // 新增：Phi 节点，用于合并循环变量

    // F64
    F64Const,
    F64Add, F64Sub, F64Mul, F64Div,
    F64Abs, F64Neg, F64Sqrt,
    F64Exp, F64Log, F64Sin, F64Cos,
    F64Min, F64Max,
    F64Pow,
    F64Eq, F64Ne, F64Lt, F64Gt, F64Le, F64Ge,
    F64ConvertI32S, F64ConvertI32U,
    I32TruncF64S, I32TruncF64U,
    // i32 ↔ i64
    I32WrapI64,
    I64ExtendI32S, I64ExtendI32U,
    // i64 ↔ f64
    F64ConvertI64S, F64ConvertI64U,
    I64TruncF64S, I64TruncF64U,
    // Memory
    Load, Store,
    F64Load, F64Store,
    Call,      // operands[0..n-1] = args, callee_index stored in lhs
    Unreachable,
    MemorySize,  // 新增：memory.size
    MemoryFill,    // 新增：memory.fill
    MemoryCopy,    // 新增：memory.copy
    Return,
    _Count   // ← 新增，必須放最後
};

enum class ValueType { I32, I64, F64, Void };

struct Value {
    int id = -1;         // SSA value id
    Op op;     // 改：Op → ValueOp
    ValueType type = ValueType::I32;  // 新增：值的类型

    int paramIndex = -1;  // for Param
    int constValue = 0;   // for I32Const
    double fconst = 0.0;   // for F64Const
    int lhs = -1;         // 對 binary op / Return 使用
    int rhs = -1;         // for binary op

    int mem_offset = 0;
    int mem_align = 0;

    // 為了向後兼容，可以加 operands 陣列
    std::vector<int> operands;
    
    int local_index = -1;  // ✅ 初始化为 -1 表示"未设置"
    int globalIndex = -1; // for GlobalGet/GlobalSet
    bool use_vload_entry = false;  // inner-only PHI 需要從 VAR VLOAD 初始值
    std::string callee_name;  // ← 加這行

    // 建構函式（可選）
    Value() = default;
};

using ValueIR = std::vector<Value>;  // 改：Value → ValueDef

inline const char* opToString(Op op) {
    static const char* const kOpNames[] = {
        "Param",           // Param
        "I32Const",        // I32Const
        "I64Const",        // I64Const        ← 原 switch 遺漏，補上
        "Add",             // Add
        "Sub",             // Sub
        "Mul",             // Mul
        "Div_S",           // Div_S
        "Div_U",           // Div_U
        "Rem_S",           // Rem_S
        "Rem_U",           // Rem_U
        "Eq",              // Eq
        "Ne",              // Ne
        "Lt_S",            // Lt_S
        "Lt_U",            // Lt_U
        "Gt_S",            // Gt_S
        "Gt_U",            // Gt_U
        "Le_S",            // Le_S
        "Le_U",            // Le_U
        "Ge_S",            // Ge_S
        "Ge_U",            // Ge_U
        "Eqz",             // Eqz
        "And",             // And
        "Or",              // Or
        "Xor",             // Xor
        "Shl",             // Shl
        "Shr_S",           // Shr_S
        "Shr_U",           // Shr_U
        "Rotl",            // Rotl
        "Rotr",            // Rotr
        "Clz",             // Clz
        "Ctz",             // Ctz
        "Popcnt",          // Popcnt
        "LocalGet",        // LocalGet
        "LocalSet",        // LocalSet
        "LocalTee",        // LocalTee
        "GlobalGet",       // GlobalGet
        "GlobalSet",       // GlobalSet
        "Select",          // Select
        "If",              // If
        "Else",            // Else
        "End",             // End
        "Loop",            // Loop
        "Br",              // Br
        "Br_if",           // Br_if
        "Phi",             // Phi
        "F64Const",        // F64Const
        "F64Add",          // F64Add
        "F64Sub",          // F64Sub
        "F64Mul",          // F64Mul
        "F64Div",          // F64Div
        "F64Abs",          // F64Abs         ← 原 switch 遺漏，補上
        "F64Neg",          // F64Neg         ← 同上
        "F64Sqrt",         // F64Sqrt        ← 同上
        "F64Exp",          // F64Exp         ← 同上
        "F64Log",          // F64Log         ← 同上
        "F64Sin",          // F64Sin         ← 同上
        "F64Cos",          // F64Cos         ← 同上
        "F64Min",          // F64Min         ← 同上
        "F64Max",          // F64Max         ← 同上
        "F64Pow",          // F64Pow         ← 同上
        "F64Eq",           // F64Eq          ← 同上
        "F64Ne",           // F64Ne          ← 同上
        "F64Lt",           // F64Lt          ← 同上
        "F64Gt",           // F64Gt          ← 同上
        "F64Le",           // F64Le          ← 同上
        "F64Ge",           // F64Ge          ← 同上
        "F64ConvertI32S",  // F64ConvertI32S
        "F64ConvertI32U",  // F64ConvertI32U
        "I32TruncF64S",    // I32TruncF64S
        "I32TruncF64U",    // I32TruncF64U
        "I32WrapI64",      // I32WrapI64
        "I64ExtendI32S",   // I64ExtendI32S
        "I64ExtendI32U",   // I64ExtendI32U
        "F64ConvertI64S",  // F64ConvertI64S
        "F64ConvertI64U",  // F64ConvertI64U
        "I64TruncF64S",    // I64TruncF64S
        "I64TruncF64U",    // I64TruncF64U
        "Load",            // Load
        "Store",           // Store
        "F64Load",         // F64Load
        "F64Store",        // F64Store
        "Call",            // Call
        "Unreachable",     // Unreachable
        "MemorySize",      // MemorySize
        "MemoryFill",      // MemoryFill
        "MemoryCopy",      // MemoryCopy
        "Return",          // Return
    };

    static_assert(sizeof(kOpNames) / sizeof(kOpNames[0])
                  == static_cast<size_t>(Op::_Count),
                  "kOpNames must be kept in sync with enum Op");

    size_t idx = static_cast<size_t>(op);
    if (idx >= static_cast<size_t>(Op::_Count)) return "Unknown";
    return kOpNames[idx];
}