#pragma once
#include "Node.hpp"
#include "Trace.hpp"
#include "ControlFlowState.hpp"
#include <cstdio>

namespace ir_node {

struct BrNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
            //
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        if (val.lhs < 0) return;  // Block target

        // 找對應的 loop（用 phi_ids 對 val.rhs）
        LoopInfo* target_loop = nullptr;
        for (int k = (int)loop_stack.size() - 1; k >= 0; k--) {
            if (loop_stack[k].loop_value_id == val.lhs) {
                target_loop = &loop_stack[k];
                break;
            }
        }
            // fprintf(stderr, "[BR_FIND] val.lhs=%d, loop_stack.size=%zu, target_loop=%p\n",
            // val.lhs, loop_stack.size(), (void*)target_loop);
        for (int k = 0; k < (int)loop_stack.size(); k++)
            // fprintf(stderr, "  loop_stack[%d].loop_value_id=%d\n", k, loop_stack[k].loop_value_id);
        if (!target_loop && !loop_stack.empty()) target_loop = &loop_stack.back();
        if (!target_loop) return;

        // 設 inner PHI back-edge
        for (int phi_id : target_loop->phi_ids) {
            const Value& phi_val = bc.values[phi_id];
            if ((int)phi_val.operands.size() >= 2) {
                ir_ref phi_ref = bc.value_map[phi_id];
                ir_ref backedge_ref = bc.value_map[phi_val.operands[1]];
                ir_PHI_SET_OP(phi_ref, 2, backedge_ref);
            }
        }

        // 設 outer carry PHI 的 back-edge
        if (loop_stack.size() >= 2) {
            LoopInfo& outer_loop = loop_stack[loop_stack.size() - 2];
            for (auto& [local_idx, outer_phi_ref] : outer_loop.outer_carry_phis) {
            // fprintf(stderr, "[OUTER_CARRY] Looking for local_%d in phi_ids (size=%zu)\n",
                    // local_idx, target_loop->phi_ids.size());
                // 找 inner loop 裡這個 local 的最新值
                for (int phi_id : target_loop->phi_ids) {
                    const Value& phi_val = bc.values[phi_id];
            // fprintf(stderr, "  phi_id=%d, local_index=%d\n", phi_id, phi_val.local_index);
                    if (phi_val.local_index == local_idx && (int)phi_val.operands.size() >= 2) {
                        ir_ref backedge_ref = bc.value_map[phi_val.operands[1]];
            // fprintf(stderr, "[OUTER_CARRY] Setting outer PHI back-edge: local_%d, backedge=v%d\n",
                            // local_idx, phi_val.operands[1]);
                        ir_PHI_SET_OP(outer_phi_ref, 2, backedge_ref);
                        break;
                    }
                }
            }
        }

            // fprintf(stderr, "[BR] val.rhs=%d, found loop_begin=%d, loop_exit=%d\n",
            // val.rhs, target_loop ? target_loop->loop_begin : -1,
            // target_loop ? target_loop->loop_exit : -1);

        ir_ref loop_end_ref = ir_LOOP_END();
        ir_MERGE_SET_OP(target_loop->loop_begin, 2, loop_end_ref);
        if (!target_loop->exits.empty()) {
            ir_MERGE_2(loop_end_ref, target_loop->exits[0]);
        }
        loop_stack.pop_back();
        TRACE("  v%zu = Br -> LOOP_END\n\n", i);
    }
};

}  // namespace ir_node
