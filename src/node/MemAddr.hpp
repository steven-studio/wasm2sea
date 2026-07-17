#pragma once
#include "ir_internal.hpp"
#include "BuildContext.hpp"

namespace ir_node {

// Shared by LoadNode/StoreNode/F64LoadNode/F64StoreNode. Unchanged from the
// original file-scope `static ir_ref makeMemAddr(...)` in ir_bridge.cpp,
// just made `inline` since it now lives in a header (harmless -- only
// ir_bridge.cpp ever includes this transitively, but `inline` keeps it
// correct even if that stops being true later).
inline ir_ref makeMemAddr(BuildContext& bc, ir_ref ptr_ref, int mem_offset) {
    ir_ctx* ctx = bc.ctx;   // 關鍵：ir_builder macro 需要這個名字
    ir_ref offset_ref = ptr_ref;
    if (mem_offset != 0) {
        offset_ref = ir_ADD_I32(ptr_ref, ir_CONST_I32(mem_offset));
    }
    return ir_ADD_A(bc.mem_param, offset_ref);
}

}  // namespace ir_node
