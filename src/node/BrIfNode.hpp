#pragma once
#include "Node.hpp"
#include "Trace.hpp"
#include "ControlFlowState.hpp"

namespace ir_node {

struct BrIfNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0 || val.lhs >= (int)i) return;
        ir_ref cond_ref = bc.value_map[val.lhs];
        if (loop_stack.empty()) { TRACE("    ERROR: Br_if without active loop!\n\n"); return; }

        ir_ref if_node = ir_IF(cond_ref);

        // 找對應的 loop（用 phi_ids 對 val.rhs）
        if (val.constValue == 1) {
            // WASM: cond true = break
            // dstogov/ir: IF_FALSE = exit, IF_TRUE = loop body continues
            ir_IF_FALSE(if_node);
            ir_ref exit_end = ir_END();
            loop_stack.back().exits.push_back(exit_end);
            ir_IF_TRUE(if_node);
            ir_ref body_end = ir_END();
            ir_BEGIN(body_end);
            return;
        }

        // br_if depth=0: continue loop / backedge
        // find the target loop by loop_value_id (val.rhs)
        LoopInfo* target_loop_ptr = nullptr;
        for (int k = (int)loop_stack.size() - 1; k >= 0; k--) {
            if (loop_stack[k].loop_value_id == val.rhs) {
                target_loop_ptr = &loop_stack[k];
                break;
            }
        }
        if (!target_loop_ptr) target_loop_ptr = &loop_stack.back();
        LoopInfo& loop_info = *target_loop_ptr;
        for (int phi_id : loop_info.phi_ids) {
            const Value& phi_val = bc.values[phi_id];
            if (phi_val.operands.size() >= 2) {
                ir_ref phi_ref = bc.value_map[phi_id];
                ir_ref backedge_ref = bc.value_map[phi_val.operands[1]];
                if (phi_ref > 0 && backedge_ref > 0) {
                    // op1=control, op2=entry, op3=back-edge (IR_UNUSED placeholder)
                    // only set if op3 is still IR_UNUSED
                    ir_insn* phi_insn = &ctx->ir_base[phi_ref];
                    if (phi_insn->op3 == IR_UNUSED)
                        ir_PHI_SET_OP(phi_ref, 2, backedge_ref);
                    else
                        ir_PHI_SET_OP(phi_ref, 2, backedge_ref);
                }
            }
        }
        ir_IF_TRUE(if_node);
        ir_ref loop_end_ref = ir_LOOP_END();
        ir_MERGE_SET_OP(loop_info.loop_begin, 2, loop_end_ref);
        ir_IF_FALSE(if_node);
        TRACE("  v%zu = Br_if(cond=v%d) kind=loop_back\n", i, val.lhs);
    }
};

}  // namespace ir_node
