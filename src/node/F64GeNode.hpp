#pragma once
#include "Node.hpp"

namespace ir_node {

struct F64GeNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.rhs < 0) return;
        bc.value_map[i] = ir_GE(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

}  // namespace ir_node
