#pragma once

#include <vector>
#include <string>

enum class Op {
    Param,     // 對應 local.get
    Const,     // 對應 i32.const
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

    Select,   // 新增：三元运算符

    Return
};

struct Value {
    int id;         // SSA value id
    Op op;     // 改：Op → ValueOp

    int paramIndex = -1;  // for Param
    int constValue = 0;   // for Const
    int lhs = -1;         // 對 binary op / Return 使用
    int rhs = -1;         // for binary op

    // 為了向後兼容，可以加 operands 陣列
    std::vector<int> operands;
    
    // 建構函式（可選）
    Value() = default;
};

using ValueIR = std::vector<Value>;  // 改：Value → ValueDef

inline std::string opToString(Op op) {  // 改：Op → ValueOp
    switch (op) {
    case Op::Param:  return "Param";    // 改：Op:: → ValueOp::
    case Op::Const:  return "Const";    // 改：Op:: → ValueOp::
    case Op::Add:    return "Add";      // 改：Op:: → ValueOp::
    case Op::Sub:    return "Sub";      // 改：Op:: → ValueOp::
    case Op::Mul:    return "Mul";      // 改：Op:: → ValueOp::
    case Op::Div_S:  return "Div_S";    // 新增：有號除法
    case Op::Div_U:  return "Div_U";    // 新增：無號除法
    case Op::Rem_S:  return "Rem_S";    // 新增：有號取餘
    case Op::Rem_U:  return "Rem_U";    // 新增：無號取餘
    case Op::Eq:    return "Eq";
    case Op::Ne:    return "Ne";
    case Op::Lt_S:  return "Lt_S";
    case Op::Lt_U:  return "Lt_U";
    case Op::Gt_S:  return "Gt_S";
    case Op::Gt_U:  return "Gt_U";
    case Op::Le_S:  return "Le_S";
    case Op::Le_U:  return "Le_U";
    case Op::Ge_S:  return "Ge_S";
    case Op::Ge_U:  return "Ge_U";
    case Op::Eqz:   return "Eqz";
    case Op::And:    return "And";
    case Op::Or:     return "Or";
    case Op::Xor:    return "Xor";
    case Op::Shl:    return "Shl";
    case Op::Shr_S:  return "Shr_S";
    case Op::Shr_U:  return "Shr_U";
    case Op::Rotl:   return "Rotl";
    case Op::Rotr:   return "Rotr";
    case Op::Clz:    return "Clz";
    case Op::Ctz:    return "Ctz";
    case Op::Popcnt: return "Popcnt";
    case Op::LocalGet: return "LocalGet";
    case Op::LocalSet: return "LocalSet";
    case Op::LocalTee: return "LocalTee";
    case Op::Select:   return "Select";
    case Op::Return: return "Return";
    default:         return "Unknown";
    }
    return "?";
}