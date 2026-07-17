#pragma once
#include "Node.hpp"
#include "Trace.hpp"
#include "ControlFlowState.hpp"
#include <cstdio>

namespace ir_node {

struct IfNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid condition for If\n"); return; }
        ir_ref cond_ref = bc.value_map[val.lhs];
        ir_ref if_node = ir_IF(cond_ref);
        ir_IF_TRUE(if_node);
        TRACE("  v%zu = If(cond=v%d) -> ir_IF ref %d, ir_IF_TRUE\n\n", i, val.lhs, if_node);
        IfInfo info;
        info.if_node = if_node;
        info.end_true = IR_UNUSED;
        info.has_else = false;
        info.true_branch_returns = false;
        if_stack.push(info);
    }
};

}  // namespace ir_node
