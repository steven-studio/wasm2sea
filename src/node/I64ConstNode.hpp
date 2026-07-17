#pragma once
#include "Node.hpp"

namespace ir_node {

struct I64ConstNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        ir_ref c = ir_CONST_I64(val.constValue);
        ir_ref copy = ir_COPY_I64(c);
        bc.value_map[i] = copy;
    }
};

}  // namespace ir_node
