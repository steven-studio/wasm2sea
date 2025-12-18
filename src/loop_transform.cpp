#include "value_ir.hpp"
#include <vector>
#include <unordered_map>

// 将 WebAssembly 结构化循环转换为显式的基本块和跳转
void transformLoops(ValueIR& values) {
    std::vector<Value> new_values;
    std::unordered_map<int, int> old_to_new;  // 映射旧 value id 到新 value id
    
    for (size_t i = 0; i < values.size(); i++) {
        const Value& val = values[i];
        
        if (val.op == Op::Loop) {
            // Loop 变成一个 Label 标记
            Value label_val;
            label_val.op = Op::Label;  // 需要添加这个新 Op
            label_val.label_id = (int)i;
            old_to_new[i] = new_values.size();
            new_values.push_back(label_val);
            
        } else if (val.op == Op::Br_if) {
            // Br_if 变成：条件为真时 Goto，否则继续
            // 1. 创建条件检查
            Value cond = val;
            cond.op = Op::BranchIf;  // 新的显式分支操作
            cond.target_label = val.rhs;  // 跳转目标
            old_to_new[i] = new_values.size();
            new_values.push_back(cond);
            
        } else if (val.op == Op::Phi) {
            // Phi 不需要了，已经用 VAR/VLOAD/VSTORE 处理
            // 跳过
            old_to_new[i] = -1;
            
        } else {
            // 其他指令保持不变，但更新引用
            Value new_val = val;
            if (val.lhs >= 0 && old_to_new.count(val.lhs)) {
                new_val.lhs = old_to_new[val.lhs];
            }
            if (val.rhs >= 0 && old_to_new.count(val.rhs)) {
                new_val.rhs = old_to_new[val.rhs];
            }
            old_to_new[i] = new_values.size();
            new_values.push_back(new_val);
        }
    }
    
    values = new_values;
}
