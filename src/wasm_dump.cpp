#include "wasm_dump.hpp"
#include <cstdio>

// 将 WasmOp 转换为字符串（用于显示）
static const char* opToString(WasmOp op) {
    switch (op) {
    case WasmOp::FuncInfo:  return "FuncInfo";
    case WasmOp::LocalGet:  return "LocalGet";
    case WasmOp::LocalSet:  return "LocalSet";
    case WasmOp::LocalTee:  return "LocalTee";
    case WasmOp::I32Const:  return "I32Const";
    case WasmOp::I32Add:    return "I32Add";
    case WasmOp::I32Sub:    return "I32Sub";
    case WasmOp::I32Mul:    return "I32Mul";
    case WasmOp::I32DivS:   return "I32DivS";
    case WasmOp::I32DivU:   return "I32DivU";
    case WasmOp::I32RemS:   return "I32RemS";
    case WasmOp::I32RemU:   return "I32RemU";
    case WasmOp::I32Eq:     return "I32Eq";
    case WasmOp::I32Ne:     return "I32Ne";
    case WasmOp::I32LtS:    return "I32LtS";
    case WasmOp::I32LtU:    return "I32LtU";
    case WasmOp::I32GtS:    return "I32GtS";
    case WasmOp::I32GtU:    return "I32GtU";
    case WasmOp::I32LeS:    return "I32LeS";
    case WasmOp::I32LeU:    return "I32LeU";
    case WasmOp::I32GeS:    return "I32GeS";
    case WasmOp::I32GeU:    return "I32GeU";
    case WasmOp::I32Eqz:    return "I32Eqz";
    case WasmOp::I32And:    return "I32And";
    case WasmOp::I32Or:     return "I32Or";
    case WasmOp::I32Xor:    return "I32Xor";
    case WasmOp::I32Shl:    return "I32Shl";
    case WasmOp::I32ShrS:   return "I32ShrS";
    case WasmOp::I32ShrU:   return "I32ShrU";
    case WasmOp::I32Clz:    return "I32Clz";
    case WasmOp::I32Ctz:    return "I32Ctz";
    case WasmOp::I32Popcnt: return "I32Popcnt";
    case WasmOp::Select:    return "Select";
    case WasmOp::If:        return "If";
    case WasmOp::Else:      return "Else";
    case WasmOp::End:       return "End";
    case WasmOp::Loop:      return "Loop";
    case WasmOp::Br:        return "Br";
    case WasmOp::Br_if:     return "Br_if";
    case WasmOp::Return:    return "Return";
    default:                return "Unknown";
    }
}

// 打印单条指令
void dumpInstr(const Instr& instr, int indent) {
    // 打印缩进
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    
    // 打印指令
    switch (instr.op) {
    case WasmOp::FuncInfo:
        printf("FuncInfo(numParams=%d)\n", instr.operand);
        break;
    
    case WasmOp::LocalGet:
        printf("LocalGet(%d)\n", instr.operand);
        break;
    
    case WasmOp::LocalSet:
        printf("LocalSet(%d)\n", instr.operand);
        break;
    
    case WasmOp::LocalTee:
        printf("LocalTee(%d)\n", instr.operand);
        break;
    
    case WasmOp::I32Const:
        printf("I32Const(%d)\n", instr.operand);
        break;
    
    case WasmOp::Br:
        printf("Br(depth=%d)\n", instr.operand);
        break;
    
    case WasmOp::Br_if:
        printf("Br_if(depth=%d)\n", instr.operand);
        break;
    
    // 无操作数的指令
    case WasmOp::I32Add:
    case WasmOp::I32Sub:
    case WasmOp::I32Mul:
    case WasmOp::I32DivS:
    case WasmOp::I32DivU:
    case WasmOp::I32RemS:
    case WasmOp::I32RemU:
    case WasmOp::I32Eq:
    case WasmOp::I32Ne:
    case WasmOp::I32LtS:
    case WasmOp::I32LtU:
    case WasmOp::I32GtS:
    case WasmOp::I32GtU:
    case WasmOp::I32LeS:
    case WasmOp::I32LeU:
    case WasmOp::I32GeS:
    case WasmOp::I32GeU:
    case WasmOp::I32Eqz:
    case WasmOp::I32And:
    case WasmOp::I32Or:
    case WasmOp::I32Xor:
    case WasmOp::I32Shl:
    case WasmOp::I32ShrS:
    case WasmOp::I32ShrU:
    case WasmOp::I32Clz:
    case WasmOp::I32Ctz:
    case WasmOp::I32Popcnt:
    case WasmOp::Select:
    case WasmOp::If:
    case WasmOp::Else:
    case WasmOp::End:
    case WasmOp::Loop:
    case WasmOp::Return:
        printf("%s\n", opToString(instr.op));
        break;
    
    default:
        printf("Unknown(op=%d, operand=%d)\n", (int)instr.op, instr.operand);
        break;
    }
}

// 打印整个 InstrSeq
void dumpInstrSeq(const InstrSeq& seq) {
    printf("=== InstrSeq (total: %zu instructions) ===\n", seq.size());
    
    for (size_t i = 0; i < seq.size(); i++) {
        printf("[%3zu] ", i);
        dumpInstr(seq[i], 0);
    }
    
    printf("=== End of InstrSeq ===\n");
}
