#pragma once
#include "Node.hpp"
#include "Trace.hpp"

namespace ir_node {

struct EqNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i || val.rhs < 0 || val.rhs >= (int)i) return;
        ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
        bc.value_map[i] = ir_EQ(lhs_ref, rhs_ref);
        TRACE("  v%zu = Eq(v%d, v%d) -> ir_EQ(ref %d, ref %d) = ref %d\n\n", i, val.lhs, val.rhs, lhs_ref, rhs_ref, bc.value_map[i]);
    }
};

}  // namespace ir_node
