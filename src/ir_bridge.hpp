#pragma once

#include "value_ir.hpp"

// === forward declarations (避免 include 太深) ===
struct IRContext;
struct IRFunction;
struct IRRegion;
struct IRNode;

class IRBridge {
public:
    IRBridge(IRContext* ctx);

    // 將一個 ValueIR 轉成 dstogov/ir 的 IRFunction
    IRFunction* build(const ValueIR& values);

private:
    IRContext* Ctx;

    // 建 function 與 region
    IRFunction* createFunctionShell();
    IRRegion*   createRegion(IRFunction* fn);

    // 第一輪建 node (不接 operands)
    void createNodes(const ValueIR& values,
                     IRFunction* fn,
                     IRRegion* region,
                     std::vector<IRNode*>& nodeMap);

    // 第二輪接 operands (依 SSA 關係)
    void connectNodes(const ValueIR& values,
                      std::vector<IRNode*>& nodeMap);

};
