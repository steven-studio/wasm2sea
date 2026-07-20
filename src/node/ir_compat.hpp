// node/ir_compat.hpp
//
// 隔開 dstogov/ir master 分支跟 simd 分支之間的 API 差異，讓
// F64SqrtNode.hpp 等呼叫端完全不用關心底層 API 名稱/簽名。
// 開關由 CMake 的 WASM2SEA_ENABLE_SIMD 選項控制（見 CMakeLists.txt）。
//
// 現況（2026-07-21）：
//   - master 分支：ir_str(ctx, const char*) 是函式，回傳 ir_ref
//   - simd 分支：ir_str 變成一個型別（typedef int ir_str），原本
//     函式的角色改由另一個尚未確認名稱的函式取代 —— SIMD 分支目前
//     還在快速變動中，下面的 SIMD 分支實作先留佔位符，故意編譯失敗，
//     避免在還沒查清楚正確 API 前被誤用、產生看似能編但語意錯誤的
//     程式碼。
#pragma once
#include "ir_internal.hpp"

#ifdef WASM2SEA_ENABLE_SIMD

inline ir_ref wasm2sea_ir_str(ir_ctx* ctx, const char* s) {
    static_assert(sizeof(ir_ctx) == 0,
        "wasm2sea_ir_str: simd 分支上正確的字串 intern API 尚未確認，"
        "先去 third_party/dstogov-ir（simd 分支）的 ir.h 查清楚 ir_str "
        "改名後的函式簽名，再補上這裡的實作。");
    return 0;
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
