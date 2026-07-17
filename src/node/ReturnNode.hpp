#pragma once
#include "Node.hpp"
#include "Trace.hpp"
#include "ControlFlowState.hpp"

namespace ir_node {

struct ReturnNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) {
            // void return
            ctx->ret_type = IR_VOID;
            ir_RETURN(IR_UNUSED);
        } else {
            ir_ref ret_val = bc.value_map[val.lhs];
            TRACE("  v%zu = Return(v%d) -> ir_RETURN(ref %d)\n\n", i, val.lhs, ret_val);
            ir_RETURN(ret_val);
        }
        if (!if_stack.empty() && !if_stack.top().has_else)
            if_stack.top().true_branch_returns = true;
    }
};

}  // namespace ir_node
