#pragma once
#include "Node.hpp"
#include "Trace.hpp"
#include <cstdio>

namespace ir_node {

struct LocalSetNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        int var_idx = val.paramIndex;
        auto it = bc.local_vars.find(var_idx);
        if (it == bc.local_vars.end()) { fprintf(stderr, "ERROR: LocalSet for untracked local_%d\n", var_idx); return; }
        if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid value index %d for LocalSet\n", val.lhs); return; }
        ir_VSTORE(it->second, bc.value_map[val.lhs]);
        TRACE("  v%zu = LocalSet(local_%d, v%d)\n\n", i, var_idx, val.lhs);
    }
};

}  // namespace ir_node
