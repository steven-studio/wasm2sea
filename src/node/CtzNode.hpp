#pragma once
#include "Node.hpp"

namespace ir_node {

struct CtzNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_CTTZ_I32(bc.value_map[val.lhs]);
    }
};

}  // namespace ir_node
