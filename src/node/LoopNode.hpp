#pragma once
#include "Node.hpp"
#include "Trace.hpp"
#include "ControlFlowState.hpp"

namespace ir_node {

struct LoopNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        TRACE("  v%zu = Loop - Creating LOOP_BEGIN\n", i);
        ir_ref loop_begin = ir_LOOP_BEGIN(ir_END());
        TRACE("    ir_LOOP_BEGIN = ref %d\n\n", loop_begin);
        LoopInfo info;
        info.loop_begin = loop_begin;
        info.entry_point = loop_begin;
        info.loop_value_id = (int)i;
        loop_stack.push_back(info);
        bc.value_map[i] = loop_begin;
    }
};

}  // namespace ir_node
