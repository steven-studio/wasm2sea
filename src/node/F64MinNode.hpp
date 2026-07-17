#pragma once
#include "Node.hpp"

namespace ir_node {

struct F64MinNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_MIN_D(bc.value_map[val.lhs], bc.value_map[val.rhs]);
    }
};

}  // namespace ir_node
