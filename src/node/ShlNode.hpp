#pragma once
#include "Node.hpp"

namespace ir_node {

struct ShlNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        bc.value_map[i] = ir_SHL_I32(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

}  // namespace ir_node
