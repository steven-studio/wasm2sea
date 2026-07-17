#pragma once
#include "Node.hpp"

namespace ir_node {

struct F64SqrtNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        ir_ref name_ref = ir_str(ctx, "sqrt");
        ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
        bc.value_map[bc.current_index] = ir_CALL_1(IR_DOUBLE, func_ref, bc.value_map[val.lhs]);
    }
};

}  // namespace ir_node
