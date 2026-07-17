#pragma once
#include "Node.hpp"
#include "Trace.hpp"
#include "ControlFlowState.hpp"
#include <cstdio>

namespace ir_node {

struct EndNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
            // fprintf(stderr, "DEBUG: Op::End reached, if_stack.size()=%zu\n", if_stack.size());
        if (!if_stack.empty()) {
            IfInfo info = if_stack.top();
            if_stack.pop();

            if (info.has_else) {
                ir_ref end_false = ir_END();
                ir_MERGE_2(info.end_true, end_false);
            } else {
                if (info.true_branch_returns) {
                    ir_IF_FALSE(info.if_node);
                } else {
                    ir_ref end_true = ir_END();
                    ir_IF_FALSE(info.if_node);
                    ir_ref end_false = ir_END();
                    ir_MERGE_2(end_true, end_false);
                }
            }
        }

        // block end：不要 pop loop_stack
        if (val.constValue == 1) {
            TRACE("  v%zu = End(block)\n", i);
            return;
        }

        // loop end：只有這裡才 pop loop_stack
        if (val.constValue == 0) {
            TRACE("  v%zu = End(loop)\n", i);
            return;
        }
    }
};

}  // namespace ir_node
