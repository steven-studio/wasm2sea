#include "wasm_lower.hpp"
#include <unordered_map>
#include <functional>

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

    auto safePop = [&]() -> int {
        if (stack.empty()) {
            int id = newValue(Op::Const);
            values[id].constValue = 0;
            return id;
        }
        int v = stack.back();
        stack.pop_back();
        return v;
    };

    // 控制流栈：记录 if/else 的信息
    struct ControlFrame {
        enum Type { If, Loop, Block } type;
        int if_id;           // if 指令的 value id
        int cond_id;         // 条件的 value id
        std::vector<int> then_values;  // then 分支产生的值
        std::vector<int> else_values;  // else 分支产生的值
        size_t stack_size;   // if 之前的栈大小

        // Loop 专用
        int loop_start_id;                        // 循环开始的 value id
        std::unordered_map<int, int> loop_phis;   // local_index -> phi_value_id
        
        // ✅ 新增：If 专用 - 保存 if 开始时的局部变量
        std::unordered_map<int, int> entry_locals;  // if 入口時的局部變量
        std::unordered_map<int, int> then_locals;   // ← 新增：then 结束时
        std::unordered_map<int, int> else_locals;   // ← 新增：else 结束时
    };
    std::vector<ControlFrame> control_stack;

    auto printState = [&](const std::string& instrName, const std::string& ssaGenerated = "") {
        fprintf(stderr, "[TRACE] After %s:\n", instrName.c_str());
        
        // 打印 localVars
        fprintf(stderr, "  localVars = {");
        if (localVars.empty()) {
            fprintf(stderr, " }");
        } else {
            for (auto& [idx, vid] : localVars) {
                fprintf(stderr, " %d→v%d", idx, vid);
            }
            fprintf(stderr, " }");
        }
        fprintf(stderr, "\n");
        
        // 打印 stack
        fprintf(stderr, "  stack = [");
        for (size_t i = 0; i < stack.size(); i++) {
            fprintf(stderr, "v%d", stack[i]);
            if (i < stack.size() - 1) fprintf(stderr, ", ");
        }
        fprintf(stderr, "]\n");
        
        // 打印 SSA Generated（如果提供了）
        if (!ssaGenerated.empty()) {
            fprintf(stderr, "  SSA Generated: %s\n", ssaGenerated.c_str());
        } else {
            fprintf(stderr, "  SSA Generated: -\n");
        }
        fprintf(stderr, "\n");
    };

    // 打印初始状态
    fprintf(stderr, "=== SSA Lowering Trace ===\n");
    fprintf(stderr, "numParams = %zu\n\n", numParams);

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
            frame.type = ControlFrame::If;  // ✅ 設置類型
            frame.if_id = if_id;
            frame.cond_id = cond;
            frame.stack_size = stack.size();

            // ✅ 保存 if 開始時的 localVars
            frame.entry_locals = localVars;

            control_stack.push_back(frame);
    
            char buf[100];
            sprintf(buf, "v%d = If(v%d)", if_id, cond);
            printState("If", buf);
            break;
        }

        case WasmOp::Else: {
            if (control_stack.empty()) break;
            
            // ✅ 保存 then 分支结束时的 locals
            control_stack.back().then_locals = localVars;

            // 记录 then 分支的结果
            if (stack.size() > control_stack.back().stack_size) {
                int then_val = stack.back();
                stack.pop_back();
                control_stack.back().then_values.push_back(then_val);
            }
            
            // ✅ 恢复到 if 入口的 locals（开始 else 分支）
            localVars = control_stack.back().entry_locals;

            // 创建 Else 节点
            int else_id = newValue(Op::Else);
    
            char buf[100];
            sprintf(buf, "v%d = Else", else_id);
            printState("Else", buf);
            break;
        }

        case WasmOp::End: {
            if (control_stack.empty()) break;
            
            ControlFrame frame = control_stack.back();
            control_stack.pop_back();
            
            if (frame.type == ControlFrame::Loop) {
                
                // 只需創建 End 節點
                int end_id = newValue(Op::End);
                char buf[100];
                sprintf(buf, "v%d = End", end_id);
                printState("End", buf);
            } else {
                // ✅ 保存 else 分支结束时的 locals
                frame.else_locals = localVars;
                
                // 记录 else 分支的返回值
                if (stack.size() > frame.stack_size) {
                    int else_val = stack.back();
                    stack.pop_back();
                    frame.else_values.push_back(else_val);
                }
                
                // ❌ 删掉所有 Select 生成逻辑！
                
                // ✅ 只创建 End
                int end_id = newValue(Op::End);
                
                char buf[200];
                
                // ✅ 如果是 expression-if（有返回值），创建 Phi
                if (!frame.then_values.empty() && !frame.else_values.empty()) {
                    int phi_id = newValue(Op::Phi);
                    values[phi_id].operands = {
                        frame.then_values[0],
                        frame.else_values[0]
                    };
                    stack.push_back(phi_id);
                    
                    sprintf(buf, "v%d = End, v%d = Phi(v%d, v%d)", 
                            end_id, phi_id, 
                            frame.then_values[0], frame.else_values[0]);
                } else {
                    sprintf(buf, "v%d = End", end_id);
                }
                
                // TODO: 合并 locals 的 Phi（下一步再做）
                
                printState("End", buf);
            }
            break;
        }

        case WasmOp::LocalGet: {
            // 查找局部变量当前值
            auto it = localVars.find(ins.operand);
            if (it != localVars.end()) {
                // 已经被赋值过，使用当前值
                stack.push_back(it->second);
                // 传入 "-" 表示没有创建新的 SSA
                printState(
                    "LocalGet(" + std::to_string(ins.operand) + ")", 
                    "-"  // ← 关键：没有创建新 SSA
                );
            } else {
                // 第一次访问
                if (ins.operand < (int)numParams) {
                    // 是参数
                    int id = newValue(Op::Param);
                    values[id].paramIndex = ins.operand;
                    localVars[ins.operand] = id;
                    stack.push_back(id);
            
                    // 传入创建的 SSA
                    char buf[100];
                    sprintf(buf, "v%d = Param(%d)", id, ins.operand);
                    printState(
                        "LocalGet(" + std::to_string(ins.operand) + ")", 
                        buf  // ← 显示创建的 SSA
                    );
                } else {
                    // 是未初始化的局部变量，默认为 0
                    int id = newValue(Op::Const);
                    values[id].constValue = 0;
                    localVars[ins.operand] = id;
                    stack.push_back(id);
            
                    char buf[100];
                    sprintf(buf, "v%d = Const(0)", id);
                    printState(
                        "LocalGet(" + std::to_string(ins.operand) + ")", 
                        buf
                    );
                }
            }
            break;
        }

        case WasmOp::LocalSet: {
            if (!stack.empty()) {
                int val = stack.back();
                stack.pop_back();
                
                // ✅ 直接更新映射，不创建 LocalSet 节点
                localVars[ins.operand] = val;

                // ✅ 創建 LocalSet 節點
                int set_id = newValue(Op::LocalSet);
                values[set_id].paramIndex = ins.operand;  // 存儲變量索引
                values[set_id].lhs = val;  // 存儲要設置的值
                
                localVars[ins.operand] = val;  // 更新映射（用於優化）
                
                char buf[100];
                // sprintf(buf, "v%d = LocalSet(local_%d, v%d)", 
                //         set_id, ins.operand, val);
                sprintf(buf, "Updated local_%d = v%d (no IR node)", ins.operand, val);
                printState("LocalSet", buf);
            }
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
            printState("I32Const(" + std::to_string(ins.operand) + ")");
            break;
        }
        case WasmOp::I32Add: {
            if (stack.size() < 2) {
                fprintf(stderr, "Error : Stack underflow at I32Add \n");
                break;
            }
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Add);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);
            printState("I32Add");
            break;
        }
        case WasmOp::I32Sub: {
            int rhs = stack.back(); stack.pop_back();
            int lhs = stack.back(); stack.pop_back();
            int id = newValue(Op::Sub);
            values[id].lhs = lhs;
            values[id].rhs = rhs;
            stack.push_back(id);

            char buf[100];  // ✅ 加上這三行
            sprintf(buf, "v%d = Sub(v%d, v%d)", id, lhs, rhs);
            printState("I32Sub", buf);
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
    
            char buf[100];
            sprintf(buf, "v%d = Gt_S(v%d, v%d)", id, lhs, rhs);
            printState("I32GtS", buf);
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

        case WasmOp::Loop: {
            // 创建 Loop 节点
            int loop_id = newValue(Op::Loop);
            
            fprintf(stderr, "[LOOP START] v%d = Loop\n", loop_id);  // ✅ 添加

            ControlFrame frame;
            frame.type = ControlFrame::Loop;
            frame.loop_start_id = loop_id;
            frame.stack_size = stack.size();
            
            // 暂时不创建 Phi，等到循环结束时再根据实际情况创建
            // 只记录循环开始时的局部变量值
            for (auto& [idx, val] : localVars) {
                int phi_id = newValue(Op::Phi);
                values[phi_id].operands.push_back(val);
                values[phi_id].local_index = idx;  // ← 记录局部变量索引
                
                fprintf(stderr, "  [PHI CREATE] v%d = Phi(v%d) for local_%d\n",   // ✅ 添加
                        phi_id, val, idx);

                frame.loop_phis[idx] = phi_id;
                localVars[idx] = phi_id;
            }
            
            control_stack.push_back(frame);
            break;
        }

        case WasmOp::Br_if: {
            if (stack.empty()) break;
            int cond = stack.back();
            stack.pop_back();
            
            int depth = ins.operand;
            if (depth >= (int)control_stack.size()) break;
            
            ControlFrame& target = control_stack[control_stack.size() - 1 - depth];
            
            if (target.type == ControlFrame::Loop) {
                fprintf(stderr, "[BR_IF] Updating Phi backedges:\n");  // ← 你沒加這些！

                // 更新 Phi 的回边
                for (auto& [idx, phi_id] : target.loop_phis) {
                    if (localVars.count(idx)) {
                        int current_val = localVars[idx];
                        // 只有当前值不是 Phi 自己时才添加回边
                        if (current_val != phi_id) {  // ← 关键检查
                            values[phi_id].operands.push_back(current_val);

                            fprintf(stderr, "  local_%d: v%d += backedge(v%d) -> Phi(v%d, v%d)\n",
                                    idx, phi_id, current_val, 
                                    values[phi_id].operands[0], current_val);                            
                        } else {
                            fprintf(stderr, "  local_%d: v%d SKIP (not modified, stays Phi(v%d))\n",
                                    idx, phi_id, values[phi_id].operands[0]);
                        }
                    }
                }
                fprintf(stderr, "\n");

                int br_if_id = newValue(Op::Br_if);
                values[br_if_id].lhs = cond;
                values[br_if_id].rhs = target.loop_start_id;
            }
            break;
        }

        case WasmOp::Return: {
            if (!stack.empty()) {
                int v = stack.back(); stack.pop_back();
                int id = newValue(Op::Return);
                values[id].lhs = v;
            }
            break;
        }

        case WasmOp::I32Load:
        case WasmOp::F64Load: {
            int ptr = safePop();
            int id = newValue(Op::Load);
            values[id].lhs = ptr;
            stack.push_back(id);
            break;
        }

        case WasmOp::I32Store:
        case WasmOp::F64Store: {
            int val = safePop();
            int ptr = safePop();
            int id = newValue(Op::Store);
            values[id].lhs = ptr;
            values[id].rhs = val;
            // Store 不 push，void
            break;
        }

        case WasmOp::Unsupported: {
            // Push dummy value so stack doesn't underflow
            int id = newValue(Op::Const);
            values[id].constValue = 0;
            stack.push_back(id);
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

    // ===== 新增：清理退化的 Phi 節點 + DCE =====
    fprintf(stderr, "\n=== Cleanup Phase ===\n");
    
    // Step 1: 找出退化的 Phi 並建立替換表
    std::unordered_map<int, int> replacements;
    for (size_t i = 0; i < values.size(); i++) {
        if (values[i].op == Op::Phi && values[i].operands.size() == 1) {
            replacements[i] = values[i].operands[0];
            fprintf(stderr, "[DEGENERATE] v%d = Phi(v%d) -> will replace with v%d\n",
                    (int)i, values[i].operands[0], values[i].operands[0]);
        }
    }
    
    // Step 2: 遞歸解析替換鏈
    std::function<int(int)> resolve = [&](int id) -> int {
        auto it = replacements.find(id);
        return (it != replacements.end()) ? resolve(it->second) : id;
    };
    
    // Step 3: 應用替換到所有節點
    for (auto& v : values) {
        if (v.lhs != -1) v.lhs = resolve(v.lhs);
        if (v.rhs != -1) v.rhs = resolve(v.rhs);
        for (auto& op : v.operands) {
            op = resolve(op);
        }
    }
    
    // Step 4: 標記所有被使用的節點
    std::vector<bool> used(values.size(), false);

    // ✅ 修正：控制流節點和 Return 節點都要保留
    for (size_t i = 0; i < values.size(); i++) {
        if (values[i].op == Op::Return ||
            values[i].op == Op::Store ||
            values[i].op == Op::Loop ||
            values[i].op == Op::If ||
            values[i].op == Op::Else ||
            values[i].op == Op::End ||
            values[i].op == Op::Br_if ||
            values[i].op == Op::LocalSet ||
            values[i].op == Op::LocalGet) {
            used[i] = true;
        }
    }

    // 遞歸標記所有依賴
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < values.size(); i++) {
            if (!used[i]) continue;
            
            // 標記這個節點的所有操作數
            if (values[i].lhs != -1 && !used[values[i].lhs]) {
                used[values[i].lhs] = true;
                changed = true;
            }
            if (values[i].rhs != -1 && !used[values[i].rhs]) {
                used[values[i].rhs] = true;
                changed = true;
            }
            for (int op : values[i].operands) {
                if (!used[op]) {
                    used[op] = true;
                    changed = true;
                }
            }
        }
    }

    // Step 5: 建立新的 ID 映射 (old_id -> new_id)
    std::vector<int> id_map(values.size(), -1);
    int new_id = 0;
    for (size_t i = 0; i < values.size(); i++) {
        if (used[i]) {
            id_map[i] = new_id++;
        } else {
            fprintf(stderr, "[DEAD] v%d removed (not used)\n", (int)i);
        }
    }

    // Step 6: 重建 values 陣列
    ValueIR new_values;
    new_values.reserve(new_id);

    for (size_t i = 0; i < values.size(); i++) {
        if (!used[i]) continue;
        
        Value v = values[i];
        v.id = id_map[i];
        
        // 更新所有引用的 ID
        if (v.lhs != -1) v.lhs = id_map[v.lhs];
        if (v.rhs != -1) v.rhs = id_map[v.rhs];
        for (auto& op : v.operands) {
            op = id_map[op];
        }
        
        new_values.push_back(v);
    }

    fprintf(stderr, "[CLEANUP] Original: %zu nodes, After cleanup: %zu nodes\n\n",
            values.size(), new_values.size());

    return new_values;  // ← 返回清理後的陣列
}