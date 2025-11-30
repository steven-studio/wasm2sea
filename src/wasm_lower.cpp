#include "wasm_lower.hpp"

ValueIR lowerWasmToSsa(const InstrSeq& code) {
    ValueIR values;
    std::vector<int> stack;  // 存 SSA id

    auto newValue = [&](Op op) -> int {
        int id = static_cast<int>(values.size());
        Value v;
        v.id = id;
        v.op = op;
        values.push_back(v);
        return id;
    };

    for (auto& ins : code) {
        switch (ins.op) {
        case WasmOp::LocalGet: {
            int id = newValue(Op::Param);
            values[id].paramIndex = ins.arg;
            stack.push_back(id);
            break;
        }
        case WasmOp::I32Const: {
            int id = newValue(Op::Const);
            values[id].constValue = ins.arg;
            stack.push_back(id);
            break;
        }
        case WasmOp::I32Add: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Add);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }
        case WasmOp::Return: {
            int v = stack.back(); stack.pop_back();
            int id = newValue(Op::Return);
            values[id].lhs = v;  // 要回傳的值
            break;
        }
        }
    }

    return values;
}
