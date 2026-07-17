#pragma once
#include "value_ir.hpp"
#include "wasm_reader.hpp"
#include <vector>
#include <unordered_map>

// Forward declarations for dstogov/ir types
typedef struct _ir_ctx ir_ctx;
typedef int32_t ir_ref;

struct IRFunction {
    ir_ctx* ctx;
    ir_ref entry_ref;
};

class IRBridge {
public:
    IRBridge();
    ~IRBridge();

    // 將 ValueIR 轉成 dstogov/ir 的 IRFunction
    IRFunction* build(const ValueIR& values,
                    const std::vector<ParamType>& paramTypes = {},
                    const std::unordered_map<int, int32_t>& globalInitValues = {});

    // 印出 IR graph
    void dump(IRFunction* fn);

    // 回傳 true 表示成功
    bool save(const char* path);
    ir_ctx* getCtx() { return ctx_; }

private:
    ir_ctx* ctx_;
};
