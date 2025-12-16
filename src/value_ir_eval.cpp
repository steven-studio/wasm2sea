#include "value_ir_eval.hpp"
#include <vector>

int evalValueIR(const ValueIR& ir, const std::vector<int>& params) {
    std::vector<int> vals(ir.size(), 0);
    for (auto& v : ir) {
        switch (v.op) {
        case Op::Param:
            vals[v.id] = params.at(v.paramIndex);
            break;
        case Op::Const:
            vals[v.id] = v.constValue;
            break;
        case Op::Add:
            vals[v.id] = vals[v.lhs] + vals[v.rhs];
            break;
        case Op::Return:
            return vals[v.lhs];
        default:
            break;
        }
    }
    return vals.back();
}