#pragma once
#include "Node.hpp"

namespace ir_node {

struct DivSNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
        bc.value_map[i] = (val.type == ValueType::I64) ? ir_DIV_I64(lhs_ref, rhs_ref) : ir_DIV_I32(lhs_ref, rhs_ref);
    }
};

}  // namespace ir_node
