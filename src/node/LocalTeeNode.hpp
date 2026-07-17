#pragma once
#include "Node.hpp"
#include "Trace.hpp"
#include <cstdio>

namespace ir_node {

struct LocalTeeNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        int var_idx = val.paramIndex;
        auto it = bc.local_vars.find(var_idx);
        if (it == bc.local_vars.end()) { fprintf(stderr, "ERROR: LocalTee for untracked local_%d\n", var_idx); return; }
        if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid value index %d for LocalTee\n", val.lhs); return; }
        ir_ref value_ref = bc.value_map[val.lhs];
        ir_VSTORE(it->second, value_ref);
        bc.value_map[i] = value_ref;
        TRACE("  v%zu = LocalTee(local_%d, v%d)\n\n", i, var_idx, val.lhs);
    }
};

}  // namespace ir_node
