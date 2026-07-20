// src/slp_detect.hpp
#pragma once
#include "value_ir.hpp"
#include <vector>
#include <set>
#include <algorithm>

struct VecBundle {
    std::vector<int> root;       // isomorphic 的 consumer，依 offset 由小到大排序
    std::vector<int> lhsGroup;   // consumer.lhs 各自對應的 SSA id，順序跟 root 一致
    std::vector<int> rhsGroup;   // consumer.rhs 各自對應的 SSA id，順序跟 root 一致
};

// 檢查兩個instruction是否isomorphic:同op、同type,
// 如果都是Load，額外要求同一個base pointer；offset 差距由呼叫端另外判斷
// （之前這裡直接檢查「差一個 elemSize」，但要支援寬度>2 時，
// 相鄰兩者未必剛好差一個 elemSize——例如 4-wide 的第 1、3 個
// 元素差 2*elemSize——所以這裡改成只確認「同 base pointer」，
// 真正的「offset 是否構成連續序列」交給 findVectorizableGroups
// 統一收集後判斷，避免這個函式對「相鄰」有過強的假設）。
// 二元運算則遞迴檢查lhs/rhs。
inline bool isIsomorphicShape(const ValueIR& ir, int i, int j) {
    if (i < 0 || j < 0) return false;
    if (i >= (int)ir.size() || j >= (int)ir.size()) return false;

    const Value& vi = ir[i];
    const Value& vj = ir[j];
    if (vi.op != vj.op || vi.type != vj.type) return false;

    if (vi.op == Op::Load) {
        return vi.lhs == vj.lhs;   // 同一個 base pointer；offset 差距由上層判斷
    }

    static const std::set<Op> binaryLikeOps = {
        Op::Add, Op::Sub, Op::Mul, Op::Div_S, Op::Div_U, Op::Rem_S, Op::Rem_U,
        Op::Eq, Op::Ne, Op::Lt_S, Op::Lt_U, Op::Gt_S, Op::Gt_U,
        Op::Le_S, Op::Le_U, Op::Ge_S, Op::Ge_U,
        Op::And, Op::Or, Op::Xor, Op::Shl, Op::Shr_S, Op::Shr_U,
        Op::F64Add, Op::F64Sub, Op::F64Mul, Op::F64Div, Op::F64Min, Op::F64Max
    };
    if (!binaryLikeOps.count(vi.op)) return false;   // Call等opaque opcode，直接視為not isomorphic

    if (vi.lhs >= 0 && vj.lhs >= 0) {
        if (!isIsomorphicShape(ir, vi.lhs, vj.lhs)) return false;
    }
    if (vi.rhs >= 0 && vj.rhs >= 0) {
        if (!isIsomorphicShape(ir, vi.rhs, vj.rhs)) return false;
    }
    return true;
}

// 取得一個（遞迴展開到底層 Load 為止的）operand 鏈上，每一層 Load 的
// mem_offset。目前只处理最簡單的情形：lhs（或 rhs）本身就是 Load。
// 若之後要支援 lhs/rhs 本身也是巢狀二元運算（而非直接是 Load），
// 這裡需要對應擴充，目前先保守只吃「一層」的情形，跟原本
// isIsomorphic 的遞迴比對範圍一致，只是把「差距檢查」抽出來。
inline bool getLoadOffset(const ValueIR& ir, int idx, int& outOffset) {
    if (idx < 0 || idx >= (int)ir.size()) return false;
    if (ir[idx].op != Op::Load) return false;
    outOffset = ir[idx].mem_offset;
    return true;
}

// 從 candidates（一組跟 seed 同構、但彼此順序未知的 consumer id）裡，
// 依照 lhs 的 Load offset 排序，找出「offset 構成連續序列
// （0, elemSize, 2*elemSize, ...）」的最長子集，回傳排序後的 id 列表。
// 若 lhs/rhs 有任何一組 offset 不構成一致的連續序列，回傳空 vector
// （表示這組 candidates 湊不出合法的 vector bundle）。
inline std::vector<int> longestConsecutiveRun(
        const ValueIR& ir, const std::vector<int>& candidates, int elemSize) {
    struct Entry { int id; int lhsOff; int rhsOff; };
    std::vector<Entry> entries;

    for (int c : candidates) {
        int lhsOff, rhsOff;
        if (!getLoadOffset(ir, ir[c].lhs, lhsOff)) return {};
        if (!getLoadOffset(ir, ir[c].rhs, rhsOff)) return {};
        entries.push_back({c, lhsOff, rhsOff});
    }

    // 依 lhs offset 排序（rhs offset 必須跟著同步遞增，否則不是合法 pack）
    std::sort(entries.begin(), entries.end(),
              [](const Entry& a, const Entry& b) { return a.lhsOff < b.lhsOff; });

    for (size_t k = 1; k < entries.size(); ++k) {
        if (entries[k].lhsOff - entries[k - 1].lhsOff != elemSize) return {};
        if (entries[k].rhsOff - entries[k - 1].rhsOff != elemSize) return {};
    }

    std::vector<int> result;
    for (auto& e : entries) result.push_back(e.id);
    return result;
}

// 從isomorphic的二元運算consumer group當root去找，支援寬度 >= 2 的
// bundle：先蒐集跟 seed 同構的所有 candidate，再從中找出 offset
// 連續的最長子集。maxWidth 限制單一 bundle 最多打包幾個元素
// （例如鎖定 128-bit 向量、i32 elemSize=4 時通常設 4）。
inline std::vector<VecBundle> findVectorizableGroups(
        const ValueIR& ir, int elemSize = 4, int maxWidth = 4) {
    std::vector<VecBundle> bundles;
    std::vector<bool> usedConsumer(ir.size(), false);

    for (size_t i = 0; i < ir.size(); ++i) {
        if (usedConsumer[i]) continue;
        if (ir[i].lhs < 0 || ir[i].rhs < 0) continue;   // 只處理二元運算
        if (ir[i].op == Op::Load) continue;             // Load 本身不是 consumer root

        std::vector<int> candidates = {(int)i};
        for (size_t j = i + 1; j < ir.size(); ++j) {
            if (usedConsumer[j]) continue;
            if ((int)candidates.size() >= maxWidth) break;
            if (!isIsomorphicShape(ir, (int)i, (int)j)) continue;
            candidates.push_back((int)j);
        }

        if (candidates.size() < 2) continue;

        std::vector<int> run = longestConsecutiveRun(ir, candidates, elemSize);
        if (run.size() < 2) continue;   // 湊不出至少 2 個連續元素，放棄這組

        VecBundle b;
        b.root = run;
        for (int id : run) {
            b.lhsGroup.push_back(ir[id].lhs);
            b.rhsGroup.push_back(ir[id].rhs);
        }
        bundles.push_back(b);
        for (int id : run) usedConsumer[id] = true;
    }
    return bundles;
}