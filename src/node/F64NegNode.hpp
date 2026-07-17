#pragma once
#include "Node.hpp"

namespace ir_node {

struct F64NegNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_NEG_D(bc.value_map[val.lhs]);
    }
};

}  // namespace ir_node
