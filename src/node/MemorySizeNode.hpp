#pragma once
#include "Node.hpp"

namespace ir_node {

struct MemorySizeNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        ir_ref name_ref = ir_str(ctx, "__wasm_memory_size");
        ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
        bc.value_map[i] = ir_CALL(IR_I32, func_ref);
    }
};

}  // namespace ir_node
