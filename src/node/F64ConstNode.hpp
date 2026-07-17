#pragma once
#include "Node.hpp"

namespace ir_node {

struct F64ConstNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        bc.value_map[bc.current_index] = ir_CONST_DOUBLE(val.fconst);
    }
};

}  // namespace ir_node
