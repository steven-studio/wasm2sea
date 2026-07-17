#pragma once
#include "Node.hpp"
#include <cstdio>

namespace ir_node {

struct GlobalGetNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        int gidx = val.globalIndex;
        auto it = bc.global_vars.find(gidx);
        if (it == bc.global_vars.end()) {
            fprintf(stderr, "ERROR: GlobalGet for untracked wasm_global_%d\n", gidx);
            return;
        }
        bc.value_map[i] = ir_VLOAD_I32(it->second);
    }
};

}  // namespace ir_node
