#pragma once
#include "Node.hpp"

namespace ir_node {

struct SelectNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.operands.size() != 3) return;
        int cond_idx = val.operands[0], true_idx = val.operands[1], false_idx = val.operands[2];
        if (cond_idx < 0 || cond_idx >= (int)i || true_idx < 0 || true_idx >= (int)i || false_idx < 0 || false_idx >= (int)i) return;
        ir_ref cond_ref = bc.value_map[cond_idx];
        ir_ref true_ref = bc.value_map[true_idx];
        ir_ref false_ref = bc.value_map[false_idx];
        ir_ref if_node = ir_IF(cond_ref);
        ir_IF_TRUE(if_node);
        ir_ref end_true = ir_END();
        ir_IF_FALSE(if_node);
        ir_ref end_false = ir_END();
        ir_MERGE_2(end_true, end_false);
        bc.value_map[i] = ir_PHI_2(IR_I32, true_ref, false_ref);
    }
};

}  // namespace ir_node
