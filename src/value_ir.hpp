#pragma once

#include <vector>
#include <string>

enum class Op {
    Param,     // 對應 local.get
    Const,     // 對應 i32.const
    Add,       // 對應 i32.add
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
    case Op::Return: return "Return";   // 改：Op:: → ValueOp::
    }
    return "?";
}