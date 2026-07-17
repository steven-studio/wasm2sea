#pragma once
#include "Node.hpp"
#include "Trace.hpp"
#include <cstdio>

namespace ir_node {

struct AddNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) { fprintf(stderr, "ERROR: Invalid lhs=%d for value %zu\n", val.lhs, i); return; }
        if (val.rhs < 0 || val.rhs >= (int)i) { fprintf(stderr, "ERROR: Invalid rhs=%d for value %zu\n", val.rhs, i); return; }
        ir_ref lhs_ref = bc.value_map[val.lhs], rhs_ref = bc.value_map[val.rhs];
        bc.value_map[i] = (val.type == ValueType::I64) ? ir_ADD_I64(lhs_ref, rhs_ref) : ir_ADD_I32(lhs_ref, rhs_ref);
        TRACE("  v%zu = Add(v%d, v%d) type=%s\n", i, val.lhs, val.rhs, val.type == ValueType::I64 ? "I64" : "I32");
    }
};

}  // namespace ir_node
