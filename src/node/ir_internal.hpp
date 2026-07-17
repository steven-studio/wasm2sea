#pragma once
// Single place that pulls in the third-party dstogov/ir headers. Every
// node/*.hpp file that needs ir_ctx/ir_ref/ir_type or the ir_ADD_I32-style
// builder macros includes this instead of reaching for "ir.h"/"ir_builder.h"
// directly, so the extern "C" wrapping only has to be written once.
//
// This header (and everything under node/) is an internal implementation
// detail of ir_bridge.cpp, the same way com/seaofnodes/simple/node/*.java
// is internal to the Simple compiler: nothing outside ir_bridge.cpp should
// need to include it. ir_bridge.hpp -- the public interface -- stays free
// of any dstogov/ir dependency, exactly as before this split.
extern "C" {
#include "ir.h"
#include "ir_builder.h"
}
