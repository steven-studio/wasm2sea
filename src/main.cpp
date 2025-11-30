#include "wasm_instr.hpp"
#include "wasm_lower.hpp"
#include "value_ir_dump.hpp"
#include "ir_bridge.hpp"   // 新增這行

// 如果 IRContext / irCreateContext 沒有在 ir_bridge.hpp 裡 include 進來
// 你之後要用 dstogov/ir 真正的 header 取代這些 forward 宣告。
// 這裡只是避免編譯錯誤的暫時作法。
struct IRContext;
IRContext* irCreateContext();

int main() {
    // ====== Step 0: 準備「假裝」的 Wasm 指令序列 ======
    // (func (param $x i32) (param $y i32) (result i32)
    //   local.get 0
    //   local.get 1
    //   i32.add
    // )
    // 我們自己加上一個 Return pseudo-op
    InstrSeq code = {
        {WasmOp::LocalGet, 0},
        {WasmOp::LocalGet, 1},
        {WasmOp::I32Add,   0},
        {WasmOp::Return,   0}
    };

    // ====== Step 1: WASM stack → 你的 SSA IR (ValueIR) ======
    ValueIR values = lowerWasmToSsa(code);

    // 先印出來確認 lowering 是對的
    dumpValueIR(values);

    // ====== Step 2: ValueIR → dstogov/ir Node Graph ======
    IRContext* ctx = irCreateContext();   // 之後要改成實際 API

    IRBridge bridge(ctx);
    IRFunction* fn = bridge.build(values);  // 建出對應的 IRFunction

    // 這裡之後可以接：
    // - dstogov/ir 自己的 IR dump
    // - JIT / codegen
    // 現在先不做，專注在前端

    (void)fn; // 暫時避免 unused variable 警告

    return 0;
}
