#pragma once
#include "Node.hpp"
#include "Trace.hpp"
#include <string>
#include <unordered_set>

namespace ir_node {

struct CallNode : Node {
    void lower(BuildContext& bc, const Value& val) const override {
        ir_ctx* ctx = bc.ctx;
        size_t i = bc.current_index;
        std::string cname = val.callee_name;
        if (!cname.empty() && cname[0] >= '0' && cname[0] <= '9') cname = "func_" + cname;
        ir_ref name_ref = ir_str(ctx, cname.c_str());
        ir_ref func_ref = ir_const_func(ctx, name_ref, IR_UNUSED);
        std::vector<ir_ref> arg_refs;
        for (int arg_id : val.operands)
            if (arg_id >= 0 && arg_id < (int)bc.value_map.size())
                arg_refs.push_back(bc.value_map[arg_id]);
        // 回傳型別原本寫死 IR_I32，對標準 libm 數學函式（回傳 double）
        // 是錯的——這是今天 deriche 這個 kernel 才第一次踩到的情況
        // （之前只有 handleF64Sqrt/Exp/Log/Sin/Cos 這幾個「單參數、內建
        // 特判」的 handler 有正確處理 double 回傳，通用的 handleCall
        // 從未被要求處理過回傳 double 的外部呼叫）。
        // 這裡先用已知的標準數學函式名稱清單做特判；未來若有更多外部
        // 函式呼叫需要非 I32 回傳型別，需要從 wasm 的函式簽章正確推導，
        // 而不是繼續擴充這個清單。
        static const std::unordered_set<std::string> kDoubleReturningFuncs = {
            "exp", "expf", "pow", "powf", "log", "logf",
            "sin", "sinf", "cos", "cosf", "sqrt", "sqrtf",
            "tan", "tanf", "atan", "atanf", "atan2", "atan2f",
            "exp2", "exp2f", "log2", "log2f", "log10", "log10f",
            "fabs", "fabsf", "floor", "floorf", "ceil", "ceilf"
        };
        ir_type ret_type = kDoubleReturningFuncs.count(cname) ? IR_DOUBLE : IR_I32;
        bc.value_map[i] = ir_CALL_N(ret_type, func_ref, (uint32_t)arg_refs.size(), arg_refs.data());
        TRACE("  v%zu = Call(%s, %zu args)\n", i, val.callee_name.c_str(), val.operands.size());
    }
};

}  // namespace ir_node
