#include "wasm_lower.hpp"
#include <unordered_map>

ValueIR lowerWasmToSsa(const InstrSeq& code) {
    ValueIR values;
    std::vector<int> stack;  // 存 SSA id
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

    // 控制流栈：记录 if/else 的信息
    struct ControlFrame {
        int if_id;           // if 指令的 value id
        int cond_id;         // 条件的 value id
        std::vector<int> then_values;  // then 分支产生的值
        std::vector<int> else_values;  // else 分支产生的值
        size_t stack_size;   // if 之前的栈大小
    };
    std::vector<ControlFrame> control_stack;

    for (size_t i = start_idx; i < code.size(); i++) {
        auto& ins = code[i];

        switch (ins.op) {

        case WasmOp::If: {
            // 弹出条件
            if (stack.empty()) break;
            int cond = stack.back();
            stack.pop_back();
            
            // 创建 If 节点
            int if_id = newValue(Op::If);
            values[if_id].lhs = cond;
            
            // 记录控制流信息
            ControlFrame frame;
            frame.if_id = if_id;
            frame.cond_id = cond;
            frame.stack_size = stack.size();
            control_stack.push_back(frame);
            break;
        }

        case WasmOp::Else: {
            if (control_stack.empty()) break;
            
            // 记录 then 分支的结果
            if (stack.size() > control_stack.back().stack_size) {
                int then_val = stack.back();
                stack.pop_back();
                control_stack.back().then_values.push_back(then_val);
            }
            
            // 创建 Else 节点
            int else_id = newValue(Op::Else);
            break;
        }

        case WasmOp::End: {
            if (control_stack.empty()) break;
            
            ControlFrame frame = control_stack.back();
            control_stack.pop_back();
            
            // 记录 else 分支的结果（如果有）
            if (stack.size() > frame.stack_size) {
                int else_val = stack.back();
                stack.pop_back();
                frame.else_values.push_back(else_val);
            }
            
            // 创建 End 节点并生成 Select/Phi
            int end_id = newValue(Op::End);
            
            // 如果 then 和 else 都有返回值，创建一个选择
            if (!frame.then_values.empty() && !frame.else_values.empty()) {
                int select_id = newValue(Op::Select);
                values[select_id].operands = {
                    frame.cond_id,
                    frame.else_values[0],   // ← 交换：else 在前
                    frame.then_values[0]    // ← 交换：then 在后
                };
                stack.push_back(select_id);
            } else if (!frame.then_values.empty()) {
                // 只有 then 分支有值
                stack.push_back(frame.then_values[0]);
            } else if (!frame.else_values.empty()) {
                // 只有 else 分支有值
                stack.push_back(frame.else_values[0]);
            }
            break;
        }


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
        case WasmOp::I32GtS: {
            if (stack.size() < 2) break;
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Gt_S);
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

        case WasmOp::Select: {
            if (stack.size() < 3) break;
            
            // 弹出的顺序和压入相反
            int cond = stack.back(); stack.pop_back();   // condition
            int true_val = stack.back(); stack.pop_back();  // true value
            int false_val = stack.back(); stack.pop_back(); // false value
            
            int id = newValue(Op::Select);
            values[id].operands = {cond, true_val, false_val};
            // 或者用现有的字段：
            // values[id].lhs = cond;
            // values[id].rhs = true_val;
            // 还需要第三个值，可能需要扩展 Value 结构
            
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
