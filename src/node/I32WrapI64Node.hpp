#pragma once
#include "Node.hpp"

namespace ir_node {

struct I32WrapI64Node : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_TRUNC_I32(bc.value_map[val.lhs]);
    }
};

}  // namespace ir_node
