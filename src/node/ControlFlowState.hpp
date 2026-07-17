#pragma once
#include "ir_internal.hpp"
#include <stack>
#include <vector>
#include <unordered_map>

// Shared control-flow bookkeeping used by IfNode/ElseNode/EndNode/LoopNode/
// BrIfNode/BrNode/PhiNode/ReturnNode while lowering a run of ValueIR entries
// that together form one if/loop region. Unchanged from the original
// file-scope IfInfo/LoopInfo/if_stack/loop_stack in ir_bridge.cpp -- these
// only moved here so the node/*.hpp files that need them can include one
// header instead of each declaring their own extern.
//
// This is the C++ analogue of Simple's `public static StartNode START` --
// state that has to be reachable across several Node subclasses while a
// single function/region is being built, so it can't live on one Node
// instance the way per-node fields do in Simple's fuller Node hierarchy.
namespace ir_node {

struct IfInfo {
    ir_ref if_node;
    ir_ref end_true;
    bool has_else;
    bool true_branch_returns;
};

struct LoopInfo {
    ir_ref loop_begin;
    ir_ref entry_point;
    std::vector<int> phi_ids;
    int loop_value_id;
    ir_ref loop_exit = IR_UNUSED;
    std::vector<ir_ref> exits;
    std::unordered_map<int, ir_ref> outer_carry_phis;  // local_idx -> outer PHI ref
};

// Defined once in ir_bridge.cpp.
extern std::stack<IfInfo> if_stack;
extern std::vector<LoopInfo> loop_stack;

}  // namespace ir_node
