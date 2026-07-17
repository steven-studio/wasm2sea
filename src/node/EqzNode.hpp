#pragma once
#include "Node.hpp"

namespace ir_node {

struct EqzNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) return;
        bc.value_map[i] = ir_EQ(bc.value_map[val.lhs], ir_CONST_I32(0));
    }
};

}  // namespace ir_node
