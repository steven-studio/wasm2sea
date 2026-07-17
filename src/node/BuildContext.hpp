#pragma once
#include "ir_internal.hpp"
#include "value_ir.hpp"
#include "wasm_reader.hpp"  // ParamType
#include <vector>
#include <unordered_map>

namespace ir_node {

// Unchanged from the original file-scope BuildContext in ir_bridge.cpp --
// just relocated so every node/*.hpp can see it.
struct BuildContext {
    ir_ctx* ctx;
    const ValueIR& values;
    const std::vector<ParamType>& paramTypes;
    std::vector<ir_ref>& value_map;
    std::unordered_map<int, ir_ref>& local_vars;
    std::unordered_map<int, ir_type>& local_types;
    std::unordered_map<int, ir_ref>& global_vars;
    ir_ref mem_param;
    bool has_memory_ops;
    size_t current_index;
};

}  // namespace ir_node
