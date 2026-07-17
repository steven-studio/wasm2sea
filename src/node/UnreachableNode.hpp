#pragma once
#include "Node.hpp"
#include "Trace.hpp"

namespace ir_node {

struct UnreachableNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        ir_TRAP();
        TRACE("  v%zu = Unreachable -> ir_TRAP()\n", bc.current_index);
    }
};

}  // namespace ir_node
