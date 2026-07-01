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

struct BuildContext;

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

    void handleAdd(BuildContext& bc, const Value& val);
    void handleSub(BuildContext& bc, const Value& val);
    void handleMul(BuildContext& bc, const Value& val);
    void handleDivS(BuildContext& bc, const Value& val);
    void handleDivU(BuildContext& bc, const Value& val);
    void handleRemS(BuildContext& bc, const Value& val);
    void handleRemU(BuildContext& bc, const Value& val);
    void handleEq(BuildContext& bc, const Value& val);
    void handleNe(BuildContext& bc, const Value& val);
    void handleLtS(BuildContext& bc, const Value& val);
    void handleLtU(BuildContext& bc, const Value& val);
    void handleGtS(BuildContext& bc, const Value& val);
    void handleGtU(BuildContext& bc, const Value& val);
    void handleLeS(BuildContext& bc, const Value& val);
    void handleLeU(BuildContext& bc, const Value& val);
    void handleGeS(BuildContext& bc, const Value& val);
    void handleGeU(BuildContext& bc, const Value& val);
    void handleAnd(BuildContext& bc, const Value& val);
    void handleOr(BuildContext& bc, const Value& val);
    void handleXor(BuildContext& bc, const Value& val);
    void handleShl(BuildContext& bc, const Value& val);
    void handleShrS(BuildContext& bc, const Value& val);
    void handleShrU(BuildContext& bc, const Value& val);
    void handleEqz(BuildContext& bc, const Value& val);
    void handleClz(BuildContext& bc, const Value& val);
    void handleCtz(BuildContext& bc, const Value& val);
    void handlePopcnt(BuildContext& bc, const Value& val);
    void handleI32WrapI64(BuildContext& bc, const Value& val);
    void handleI64ExtendI32S(BuildContext& bc, const Value& val);
    void handleI64ExtendI32U(BuildContext& bc, const Value& val);
    void handleF64ConvertI(BuildContext& bc, const Value& val);
    void handleF64Neg(BuildContext& bc, const Value& val);
    void handleF64Sqrt(BuildContext& bc, const Value& val);
    void handleF64Eq(BuildContext& bc, const Value& val);
    void handleF64Ne(BuildContext& bc, const Value& val);
    void handleF64Lt(BuildContext& bc, const Value& val);
    void handleF64Gt(BuildContext& bc, const Value& val);
    void handleF64Le(BuildContext& bc, const Value& val);
    void handleF64Ge(BuildContext& bc, const Value& val);
    void handleI32TruncF64(BuildContext& bc, const Value& val);
    void handleI64TruncF64(BuildContext& bc, const Value& val);
    void handleI32Const(BuildContext& bc, const Value& val);
    void handleI64Const(BuildContext& bc, const Value& val);
    void handleF64Const(BuildContext& bc, const Value& val);
    void handleLocalGet(BuildContext& bc, const Value& val);
    void handleLocalSet(BuildContext& bc, const Value& val);
    void handleLocalTee(BuildContext& bc, const Value& val);
    void handleGlobalGet(BuildContext& bc, const Value& val);
    void handleGlobalSet(BuildContext& bc, const Value& val);
    void handleIf(BuildContext& bc, const Value& val);
    void handleElse(BuildContext& bc, const Value& val);
    void handleEnd(BuildContext& bc, const Value& val);
    void handleLoop(BuildContext& bc, const Value& val);
    void handleBrIf(BuildContext& bc, const Value& val);
    void handleBr(BuildContext& bc, const Value& val);
    void handlePhi(BuildContext& bc, const Value& val);
    void handleReturn(BuildContext& bc, const Value& val);
    void handleSelect(BuildContext& bc, const Value& val);
    void handleLoad(BuildContext& bc, const Value& val);
    void handleStore(BuildContext& bc, const Value& val);
    void handleF64Load(BuildContext& bc, const Value& val);
    void handleF64Store(BuildContext& bc, const Value& val);
    void handleF64Add(BuildContext& bc, const Value& val);
    void handleF64Sub(BuildContext& bc, const Value& val);
    void handleF64Mul(BuildContext& bc, const Value& val);
    void handleF64Div(BuildContext& bc, const Value& val);
    void handleCall(BuildContext& bc, const Value& val);
    void handleUnreachable(BuildContext& bc, const Value& val);
    void handleMemorySize(BuildContext& bc, const Value& val);
    void handleMemoryCopy(BuildContext& bc, const Value& val);
    void handleMemoryFill(BuildContext& bc, const Value& val);

private:
    ir_ctx* ctx_;
};
