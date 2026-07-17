#pragma once
#include "BuildContext.hpp"
#include "value_ir.hpp"

// node/ is the C++ analogue of Simple's com.seaofnodes.simple.node package:
// one class per ValueIR opcode, all living under the same namespace so a
// common name (AddNode, CallNode, ReturnNode, ...) can never collide with
// an unrelated type elsewhere in the project.
namespace ir_node {

// Base class for every ValueIR opcode handler. Mirrors Simple's Node: one
// subclass per operation, picked from a table instead of a switch/member
// dispatch. lower() writes the resulting ir_ref into
// bc.value_map[bc.current_index], same contract the old handleXxx had.
//
// Difference from Simple's Node, worth knowing: Simple's Node owns graph
// edges (_inputs) directly; here the graph lives inside dstogov/ir's
// ir_ctx, addressed by integer ir_ref, so each lower() call reaches into
// BuildContext for it instead.
struct Node {
    virtual ~Node() = default;
    virtual void lower(BuildContext& bc, const Value& val) const = 0;
};

}  // namespace ir_node
