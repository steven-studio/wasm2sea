#pragma once
#include "Node.hpp"
#include "MemAddr.hpp"

namespace ir_node {

struct LoadNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) return;
        ir_ref ptr_ref = bc.value_map[val.lhs];
        if (ptr_ref == IR_UNUSED) return;
        ir_ref real_ptr = makeMemAddr(bc, ptr_ref, val.mem_offset);
        bc.value_map[i] = (val.type == ValueType::I64) ? ir_LOAD_I64(real_ptr) : ir_LOAD_I32(real_ptr);
    }
};

}  // namespace ir_node
