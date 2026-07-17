#pragma once
#include "Node.hpp"
#include "Trace.hpp"
#include "ControlFlowState.hpp"

namespace ir_node {

struct PhiNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.operands.empty()) { TRACE("    ERROR: Phi with no operands\n\n"); return; }
        if (val.local_index >= 0) {
            ir_ref entry_val = bc.value_map[val.operands[0]];
            ir_ref loop_begin_ref = ctx->control;  // 記錄 LOOP_BEGIN

            // 如果 entry_val 是另一個 PHI，用 VSTORE/VLOAD 打破 PHI-PHI chain
            if (val.use_vload_entry && loop_stack.size() >= 2) {
                int local_idx = val.local_index;
                LoopInfo& outer_loop = loop_stack[loop_stack.size() - 2];
                ir_ref inner_loop_begin = ctx->control;
                // 切換到 outer loop 建 PHI
                ctx->control = outer_loop.loop_begin;
                ir_ref outer_phi = ir_PHI_2(IR_I32, ir_CONST_I32(0), IR_UNUSED);
            //
                // 用 COPY_HARD 包住 outer PHI，讓 optimizer 不折疊
                ir_ref hard_copy = ir_emit2(ctx, IR_OPT(IR_COPY, IR_I32), outer_phi, IR_COPY_HARD);
                outer_loop.outer_carry_phis[local_idx] = outer_phi;
                ctx->control = inner_loop_begin;
                entry_val = outer_phi;
            }

            ir_ref inputs[2] = {entry_val, IR_UNUSED};
            ir_ref phi = ir_emit_N(ctx, IR_OPT(IR_PHI, IR_I32), 3);
            //
            ir_set_op(ctx, phi, 1, ctx->control);
            ir_set_op(ctx, phi, 2, entry_val);
            ir_set_op(ctx, phi, 3, IR_UNUSED);

            bc.value_map[i] = phi;
            if (!loop_stack.empty()) loop_stack.back().phi_ids.push_back(i);
            TRACE("  v%zu = Phi (Loop) -> ref %d\n\n", i, phi);
        } else {
            if (val.operands.size() != 2) { TRACE("    ERROR: If Phi should have exactly 2 operands\n\n"); return; }
            ir_ref op0 = bc.value_map[val.operands[0]];
            ir_ref op1 = bc.value_map[val.operands[1]];
            ir_type phi_type = (ir_type)ctx->ir_base[op0].type;
            ir_ref phi = ir_PHI_2(phi_type, op0, op1);
            bc.value_map[i] = phi;
            TRACE("  v%zu = Phi (If) -> ref %d\n\n", i, phi);
        }
    }
};

}  // namespace ir_node
