#pragma once
#include "Node.hpp"
#include "MemAddr.hpp"

namespace ir_node {

struct F64LoadNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) return;
        ir_ref ptr_ref = bc.value_map[val.lhs];
        if (ptr_ref == IR_UNUSED) return;
        ir_ref real_ptr = makeMemAddr(bc, ptr_ref, val.mem_offset);
        bc.value_map[i] = ir_LOAD_D(real_ptr);
    }
};

}  // namespace ir_node
