#pragma once
#include "Node.hpp"
#include "ir_compat.hpp"

namespace ir_node {

struct MemoryFillNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        ir_ref name_ref = wasm2sea_ir_str(ctx, "__wasm_memory_fill");
        ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
        ir_CALL_3(IR_VOID, func_ref, bc.value_map[val.operands[0]], bc.value_map[val.operands[1]], bc.value_map[val.operands[2]]);
    }
};

}  // namespace ir_node
