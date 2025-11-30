#include "ir_bridge.hpp"
#include <cassert>
#include <iostream>

// ==========================================================
//  假的 dstogov/ir 型別與函式實作 (stub)
//  目的是先讓專案可以編譯 & 執行，之後再換成真正的 API。
// ==========================================================

struct IRContext {};
struct IRFunction {};
struct IRRegion {};
struct IRNode {};

// 先做一堆空的 stub，之後會用真正的 dstogov/ir 取代
IRContext* irCreateContext() {
    return new IRContext();
}

IRFunction* irCreateFunction(IRContext*) {
    return new IRFunction();
}

IRRegion* irCreateRegion(IRFunction*) {
    return new IRRegion();
}

IRNode* irCreateParamNode(IRFunction*, int) {
    return new IRNode();
}

IRNode* irCreateIntConstNode(IRFunction*, int) {
    return new IRNode();
}

IRNode* irCreateAddNode(IRFunction*) {
    return new IRNode();
}

IRNode* irCreateReturnNode(IRFunction*) {
    return new IRNode();
}

void irNodeSetOperand(IRNode*, int, IRNode*) {
    // stub: do nothing
}

// ==========================================================
//                     IRBridge 實作
// ==========================================================

IRBridge::IRBridge(IRContext* ctx)
    : Ctx(ctx)
{}

// ---- 建立 function shell ----
IRFunction* IRBridge::createFunctionShell() {
    return irCreateFunction(Ctx);
}

// ---- 建立 region ----
IRRegion* IRBridge::createRegion(IRFunction* fn) {
    return irCreateRegion(fn);
}

// ---- Phase 1：建立所有 Node（不接邊） ----
void IRBridge::createNodes(const ValueIR& values,
                           IRFunction* fn,
                           IRRegion* region,
                           std::vector<IRNode*>& nodeMap)
{
    (void)region; // 目前 region 沒用到，先避免 warning

    for (auto& v : values) {
        IRNode* n = nullptr;

        switch (v.op) {
        case Op::Param:
            n = irCreateParamNode(fn, v.paramIndex);
            break;

        case Op::Const:
            n = irCreateIntConstNode(fn, v.constValue);
            break;

        case Op::Add:
            n = irCreateAddNode(fn);
            break;

        case Op::Return:
            n = irCreateReturnNode(fn);
            break;

        default:
            std::cerr << "Unsupported opcode in IRBridge\n";
            assert(false);
        }

        nodeMap[v.id] = n;
    }
}

// ---- Phase 2：依 SSA 依賴接回 Node operand ----
void IRBridge::connectNodes(const ValueIR& values,
                            std::vector<IRNode*>& nodeMap)
{
    for (auto& v : values) {
        IRNode* n = nodeMap[v.id];

        switch (v.op) {
        case Op::Add:
            irNodeSetOperand(n, 0, nodeMap[v.lhs]);
            irNodeSetOperand(n, 1, nodeMap[v.rhs]);
            break;

        case Op::Return:
            irNodeSetOperand(n, 0, nodeMap[v.lhs]);
            break;

        default:
            // Param / Const 沒有 operand
            break;
        }
    }
}

// ---- 對外主入口 ----
IRFunction* IRBridge::build(const ValueIR& values) {
    // 建 function + region
    IRFunction* fn = createFunctionShell();
    IRRegion* region = createRegion(fn);

    // 為每個 Value 分配一格 nodeMap
    std::vector<IRNode*> nodeMap(values.size(), nullptr);

    // 第一輪先建 node，不接 operand
    createNodes(values, fn, region, nodeMap);

    // 第二輪依照 SSA 依賴接邊
    connectNodes(values, nodeMap);

    return fn; // 回傳「假的」function 物件（目前只是佔位）
}
