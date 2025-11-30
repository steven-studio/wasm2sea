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
    Op  op;

    int paramIndex = -1;  // for Param
    int constValue = 0;   // for Const
    int lhs = -1;         // 對 binary op / Return 使用
    int rhs = -1;         // for binary op
};

using ValueIR = std::vector<Value>;

inline std::string opToString(Op op) {
    switch (op) {
    case Op::Param:  return "Param";
    case Op::Const:  return "Const";
    case Op::Add:    return "Add";
    case Op::Return: return "Return";
    }
    return "?";
}
