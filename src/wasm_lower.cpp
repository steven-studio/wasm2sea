#include "wasm_lower.hpp"
#include <unordered_map>

ValueIR lowerWasmToSsa(const InstrSeq& code) {
    ValueIR values;
    std::vector<int> stack;  // 存 SSA id

    // 局部变量管理：local index -> 当前的 SSA id
    std::unordered_map<int, int> localVars;
    
    // 读取函数元信息（参数数量）
    size_t numParams = 0;
    size_t start_idx = 0;
    
    if (!code.empty() && code[0].op == WasmOp::FuncInfo) {
        numParams = static_cast<size_t>(code[0].operand);
        start_idx = 1;
    }
    
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
            // 查找局部变量当前值
            auto it = localVars.find(ins.operand);
            if (it != localVars.end()) {
                // 已经被赋值过，使用当前值
                stack.push_back(it->second);
            } else {
                // 第一次访问
                if (ins.operand < (int)numParams) {
                    // 是参数
                    int id = newValue(Op::Param);
                    values[id].paramIndex = ins.operand;
                    localVars[ins.operand] = id;
                    stack.push_back(id);
                } else {
                    // 是未初始化的局部变量，默认为 0
                    int id = newValue(Op::Const);
                    values[id].constValue = 0;
                    localVars[ins.operand] = id;
                    stack.push_back(id);
                }
            }
            break;
        }

        case WasmOp::LocalSet: {
            if (stack.empty()) break;
            int val = stack.back(); 
            stack.pop_back();
            // 更新局部变量的当前值
            localVars[ins.operand] = val;
            break;
        }
        
        case WasmOp::LocalTee: {
            if (stack.empty()) break;
            int val = stack.back(); 
            // tee 不 pop，值留在栈上
            localVars[ins.operand] = val;
            // 栈上已经有值了，不需要再 push
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
        case WasmOp::I32DivS: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Div_S);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }
        case WasmOp::I32DivU: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Div_U);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }
        case WasmOp::I32RemS: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Rem_S);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }
        case WasmOp::I32RemU: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Rem_U);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }

        // 比較運算（二元，像 Add/Sub 一樣處理）
        case WasmOp::I32Eq: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Eq);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }
        case WasmOp::I32Ne: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Ne);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }
        case WasmOp::I32LtS: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Lt_S);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }

        case WasmOp::I32And: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::And);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }
        case WasmOp::I32Or: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Or);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }
        case WasmOp::I32Xor: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Xor);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }
        case WasmOp::I32Shl: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Shl);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }
        case WasmOp::I32ShrS: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Shr_S);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }
        case WasmOp::I32ShrU: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Shr_U);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            break;
        }

        // Eqz（一元運算，特殊處理）
        case WasmOp::I32Eqz: {
            int val = stack.back(); stack.pop_back();
            int id = newValue(Op::Eqz);
            values[id].lhs = val;  // 只有一個操作數
            stack.push_back(id);
            break;
        }
        case WasmOp::I32Clz: {
            int val = stack.back(); stack.pop_back();
            int id = newValue(Op::Clz);
            values[id].lhs = val;  // 只有一個操作數
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
