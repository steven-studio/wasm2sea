#pragma once
#include "Node.hpp"
#include "Trace.hpp"
#include <cstdio>

namespace ir_node {

struct LocalGetNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        int var_idx = val.paramIndex;
        auto it = bc.local_vars.find(var_idx);
        if (it == bc.local_vars.end()) { fprintf(stderr, "ERROR: LocalGet for untracked local_%d\n", var_idx); return; }
        ir_ref var_ref = it->second;
        ir_type t = bc.local_types.count(var_idx) ? bc.local_types[var_idx] : IR_I32;
        ir_ref loaded = (t == IR_I64) ? ir_VLOAD_I64(var_ref)
                      : (t == IR_DOUBLE) ? ir_VLOAD_D(var_ref)
                      : ir_VLOAD_I32(var_ref);
        bc.value_map[i] = loaded;
        TRACE("  v%zu = LocalGet(local_%d) -> ref %d\n\n", i, var_idx, loaded);
    }
};

}  // namespace ir_node
