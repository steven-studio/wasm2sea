#pragma once
#include "Node.hpp"
#include "Trace.hpp"
#include "ControlFlowState.hpp"
#include <cstdio>

namespace ir_node {

struct ElseNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (if_stack.empty()) { fprintf(stderr, "ERROR: Else without matching If\n"); return; }
        ir_ref end_true = ir_END();
        TRACE("  v%zu = Else -> ir_END (true branch) ref %d\n\n", i, end_true);
        if_stack.top().end_true = end_true;
        if_stack.top().has_else = true;
        ir_IF_FALSE(if_stack.top().if_node);
    }
};

}  // namespace ir_node
