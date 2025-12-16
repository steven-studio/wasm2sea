#include "wasm_lower.hpp"
#include <unordered_map>

ValueIR lowerWasmToSsa(const InstrSeq& code) {
    ValueIR values;
    std::vector<int> stack;  // 存 SSA id

    // param index -> SSA id
    std::unordered_map<int,int> paramId;

    auto newValue = [&](Op op) -> int {
        int id = static_cast<int>(values.size());
        Value v;
        v.id = id;
        v.op = op;
        values.push_back(v);
        return id;
    };

    auto getParam = [&](int idx) -> int {
        auto it = paramId.find(idx);
        if (it != paramId.end()) return it->second;
        int id = newValue(Op::Param);
        values[id].paramIndex = idx;
        paramId[idx] = id;
        return id;
    };

    for (auto& ins : code) {
        switch (ins.op) {
        case WasmOp::LocalGet: {
            int id = getParam(ins.operand);
            stack.push_back(id);
            break;
        }
        case WasmOp::I32Const: {
            int id = newValue(Op::Const);
            values[id].constValue = ins.operand;
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
        case WasmOp::I32Sub: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Sub);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }
        case WasmOp::I32Mul: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Mul);
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

    // --- implicit return (WASM function end) ---
    if (!stack.empty()) {
        int v = stack.back();
        // 如果你允許多個 return，這裡可再加一個旗標避免重複產生
        int id = newValue(Op::Return);
        values[id].lhs = v;
    }

    return values;
}
