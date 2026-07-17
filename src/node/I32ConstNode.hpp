#pragma once
#include "Node.hpp"
#include "Trace.hpp"

namespace ir_node {

struct I32ConstNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        ir_ref c = ir_CONST_I32(val.constValue);
        ir_ref copy = ir_COPY_I32(c);
        bc.value_map[i] = copy;
        TRACE(" v%zu = Const(%d) -> ir_CONST_I32 ref %d, ir_COPY ref %d\n\n", i, val.constValue, c, copy);
    }
};

}  // namespace ir_node
