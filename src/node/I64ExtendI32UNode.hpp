#pragma once
#include "Node.hpp"

namespace ir_node {

struct I64ExtendI32UNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_ZEXT_I64(bc.value_map[val.lhs]);
    }
};

}  // namespace ir_node
