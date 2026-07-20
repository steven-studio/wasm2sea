#pragma once
#include "Node.hpp"
#include "ir_compat.hpp"

namespace ir_node {

struct F64ExpNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        ir_ref name_ref = wasm2sea_ir_str(ctx, "exp");
        ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
        bc.value_map[bc.current_index] = ir_CALL_1(IR_DOUBLE, func_ref, bc.value_map[val.lhs]);
    }
};

}  // namespace ir_node
