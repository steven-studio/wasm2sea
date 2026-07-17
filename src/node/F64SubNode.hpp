#pragma once
#include "Node.hpp"

namespace ir_node {

struct F64SubNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        if (val.lhs < 0 || val.rhs < 0) return;
        ir_ref l = bc.value_map[val.lhs], r = bc.value_map[val.rhs];
        if (l == IR_UNUSED || r == IR_UNUSED) return;
        bc.value_map[bc.current_index] = ir_SUB_D(l, r);
    }
};

}  // namespace ir_node
