// node/ir_compat.hpp
//
// 隔開 dstogov/ir master 分支跟 simd 分支之間的 API 差異，讓
// F64SqrtNode.hpp 等呼叫端完全不用關心底層 API 名稱/簽名。
// 開關由 CMake 的 WASM2SEA_ENABLE_SIMD 選項控制（見 CMakeLists.txt）。
//
// 現況（2026-07-21）：
//   - master 分支：ir_str(ctx, const char*) 是函式，回傳 ir_ref，
//     做字串 intern。
//   - simd 分支：ir_str 這個名字只留下型別身分（整數索引 typedef），
//     移除了同名的相容函式；真正做字串 intern 的函式改名為
//     ir_string(ctx, const char*)，回傳型別是 ir_str（跟 ir_ref
//     底層都是同一種整數 typedef，可安全轉型）。
//
// 已驗證（vecadd4_seed / call_simple / factorial_rec 三支測資）：
//   - 非字串路徑（SLP 向量化偵測、一般 Add/Load lowering）不受影響
//   - 一般 Call 節點：wasm2sea_ir_str 正確 intern 被呼叫函式名稱，
//     ir_const_func 正確消費，整條 dstogov/ir pass pipeline
//     （def_use_lists ~ coalesce）順利跑完
//   - 自我遞迴 Call：callee 名稱正確顯示為函式自身名稱，不會
//     fallback 成錯誤的 index 或空字串
#pragma once
#include "ir_internal.hpp"

#ifdef WASM2SEA_ENABLE_SIMD

inline ir_ref wasm2sea_ir_str(ir_ctx* ctx, const char* s) {
    return (ir_ref)ir_string(ctx, s);
}

inline void wasm2sea_ir_dump_dot(ir_ctx* ctx, const char* name, FILE* f) {
    ir_dump_dot(ctx, name, nullptr, f);  // simd 分支新簽名：多一個 comments 參數
}

#else  // !WASM2SEA_ENABLE_SIMD — master 分支，原本能編過的寫法

inline ir_ref wasm2sea_ir_str(ir_ctx* ctx, const char* s) {
    return ir_str(ctx, s);
}

inline void wasm2sea_ir_dump_dot(ir_ctx* ctx, const char* name, FILE* f) {
    ir_dump_dot(ctx, name, f);
}

#endif
