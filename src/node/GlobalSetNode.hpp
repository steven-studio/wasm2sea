#pragma once
#include "Node.hpp"
#include <cstdio>

namespace ir_node {

struct GlobalSetNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        int gidx = val.globalIndex;
        auto it = bc.global_vars.find(gidx);
        if (it == bc.global_vars.end()) {
            fprintf(stderr, "ERROR: GlobalSet for untracked wasm_global_%d\n", gidx);
            return;
        }
        if (val.lhs < 0 || val.lhs >= (int)i) return;
        ir_VSTORE(it->second, bc.value_map[val.lhs]);
    }
};

}  // namespace ir_node
