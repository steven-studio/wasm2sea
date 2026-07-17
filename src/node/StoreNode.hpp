#pragma once
#include "Node.hpp"
#include "MemAddr.hpp"

namespace ir_node {

struct StoreNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        ir_ref ptr_ref = bc.value_map[val.lhs], val_ref = bc.value_map[val.rhs];
        if (ptr_ref == IR_UNUSED || val_ref == IR_UNUSED) return;
        ir_ref real_ptr = makeMemAddr(bc, ptr_ref, val.mem_offset);
        ir_STORE(real_ptr, val_ref);
    }
};

}  // namespace ir_node
