// src/slp_detect.hpp
#pragma once
#include "value_ir.hpp"
#include <vector>
#include <set>

struct VecBundle {
    std::vector<int> root;       // isomorphic 的 consumer pair，例如 {v6, v10}
    std::vector<int> lhsGroup;   // consumer.lhs 各自對應的 SSA id，例如 {v4, v8}
    std::vector<int> rhsGroup;   // consumer.rhs 各自對應的 SSA id，例如 {v5, v9}
};

// 檢查兩個instruction是否isomorphic:同op、同type,
// 如果都是Load，額外要求同一個base pointer、offset相差elemSize
// （這就是之前isConsecutiveLoadPair的邏輯，內嵌進來取代外部另外檢查）。
// 二元運算則遞迴檢查lhs/rhs。
inline bool isIsomorphic(const ValueIR& ir, int i, int j, int elemSize) {
    // 必修：防止crash
    if (i < 0 || j < 0) return false;
    if (i >= (int)ir.size() || j >= (int)ir.size()) return false;

    const Value& vi = ir[i];
    const Value& vj = ir[j];
    if (vi.op != vj.op || vi.type != vj.type) return false;

    if (vi.op == Op::Load) {
        if (vi.lhs != vj.lhs) return false;   // 必須是同一個base pointer
        int32_t diff = vj.mem_offset - vi.mem_offset;
        return diff == elemSize || diff == -elemSize;  // offset 差一個 element，雙向都接受
    }

    // 之後該修：只對「lhs/rhs真的代表SSA operand」的opcode才遞迴，
    // 其餘（Call的callee_idx、MemorySize等）視為leaf、不遞迴比對，
    // 保守地當作not isomorphic，避免把callee_idx誤判成operand
    // 而產生語意錯誤的false positive（現在只是防crash，還沒防這個）
    static const std::set<Op> binaryLikeOps = {
        Op::Add, Op::Sub, Op::Mul, Op::Div_S, Op::Div_U, Op::Rem_S, Op::Rem_U,
        Op::Eq, Op::Ne, Op::Lt_S, Op::Lt_U, Op::Gt_S, Op::Gt_U,
        Op::Le_S, Op::Le_U, Op::Ge_S, Op::Ge_U,
        Op::And, Op::Or, Op::Xor, Op::Shl, Op::Shr_S, Op::Shr_U,
        Op::F64Add, Op::F64Sub, Op::F64Mul, Op::F64Div, Op::F64Min, Op::F64Max
    };
    if (!binaryLikeOps.count(vi.op)) return false;   // Call等opaque opcode，直接視為not isomorphic

    if (vi.lhs >= 0 && vj.lhs >= 0) {
        if (!isIsomorphic(ir, vi.lhs, vj.lhs, elemSize)) return false;
    }
    if (vi.rhs >= 0 && vj.rhs >= 0) {
        if (!isIsomorphic(ir, vi.rhs, vj.rhs, elemSize)) return false;
    }
    return true;
}

// 從isomorphic的二元運算consumer pair當root去找，lhs/rhs兩邊的
// packing資訊都記錄成同一個bundle底下的sibling，不會有一邊被另一邊擠掉。
inline std::vector<VecBundle> findVectorizableGroups(const ValueIR& ir, int elemSize = 4) {
    std::vector<VecBundle> bundles;
    std::vector<bool> usedConsumer(ir.size(), false);

    for (size_t i = 0; i < ir.size(); ++i) {
        if (usedConsumer[i]) continue;
        if (ir[i].lhs < 0 || ir[i].rhs < 0) continue;   // 只處理二元運算

        for (size_t j = i + 1; j < ir.size(); ++j) {
            if (usedConsumer[j]) continue;
            if (ir[i].op != ir[j].op || ir[i].type != ir[j].type) continue;
            if (ir[j].lhs < 0 || ir[j].rhs < 0) continue;

            bool lhsPackable = isIsomorphic(ir, ir[i].lhs, ir[j].lhs, elemSize);
            bool rhsPackable = isIsomorphic(ir, ir[i].rhs, ir[j].rhs, elemSize);
            if (!lhsPackable || !rhsPackable) continue;

            VecBundle b;
            b.root     = {(int)i, (int)j};
            b.lhsGroup = {ir[i].lhs, ir[j].lhs};
            b.rhsGroup = {ir[i].rhs, ir[j].rhs};
            bundles.push_back(b);
            usedConsumer[i] = usedConsumer[j] = true;
            break;
        }
    }
    return bundles;
}