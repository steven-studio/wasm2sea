#include "wasm_dump.hpp"
#include <cstdio>

static const char* opToString(WasmOp op) {
    switch (op) {
    case WasmOp::FuncInfo:       return "FuncInfo";
    case WasmOp::LocalGet:       return "LocalGet";
    case WasmOp::LocalSet:       return "LocalSet";
    case WasmOp::LocalTee:       return "LocalTee";
    case WasmOp::I32Const:       return "I32Const";
    case WasmOp::I32Add:         return "I32Add";
    case WasmOp::I32Sub:         return "I32Sub";
    case WasmOp::I32Mul:         return "I32Mul";
    case WasmOp::I32DivS:        return "I32DivS";
    case WasmOp::I32DivU:        return "I32DivU";
    case WasmOp::I32RemS:        return "I32RemS";
    case WasmOp::I32RemU:        return "I32RemU";
    case WasmOp::I32Eq:          return "I32Eq";
    case WasmOp::I32Ne:          return "I32Ne";
    case WasmOp::I32LtS:         return "I32LtS";
    case WasmOp::I32LtU:         return "I32LtU";
    case WasmOp::I32GtS:         return "I32GtS";
    case WasmOp::I32GtU:         return "I32GtU";
    case WasmOp::I32LeS:         return "I32LeS";
    case WasmOp::I32LeU:         return "I32LeU";
    case WasmOp::I32GeS:         return "I32GeS";
    case WasmOp::I32GeU:         return "I32GeU";
    case WasmOp::I32Eqz:         return "I32Eqz";
    case WasmOp::I32And:         return "I32And";
    case WasmOp::I32Or:          return "I32Or";
    case WasmOp::I32Xor:         return "I32Xor";
    case WasmOp::I32Shl:         return "I32Shl";
    case WasmOp::I32ShrS:        return "I32ShrS";
    case WasmOp::I32ShrU:        return "I32ShrU";
    case WasmOp::I32Clz:         return "I32Clz";
    case WasmOp::I32Ctz:         return "I32Ctz";
    case WasmOp::I32Popcnt:      return "I32Popcnt";
    case WasmOp::Select:         return "Select";
    case WasmOp::Block:          return "Block";
    case WasmOp::If:             return "If";
    case WasmOp::Else:           return "Else";
    case WasmOp::End:            return "End";
    case WasmOp::Loop:           return "Loop";
    case WasmOp::Br:             return "Br";
    case WasmOp::Br_if:          return "Br_if";
    case WasmOp::BrTable:        return "BrTable";
    case WasmOp::Return:         return "Return";
    case WasmOp::Drop:           return "Drop";
    case WasmOp::Call:           return "Call";
    case WasmOp::Unreachable:    return "Unreachable";
    case WasmOp::GlobalGet:      return "GlobalGet";
    case WasmOp::GlobalSet:      return "GlobalSet";
    case WasmOp::I32Load:        return "I32Load";
    case WasmOp::I32Store:       return "I32Store";
    case WasmOp::I64Load:        return "I64Load";
    case WasmOp::I64Store:       return "I64Store";
    case WasmOp::F64Load:        return "F64Load";
    case WasmOp::F64Store:       return "F64Store";
    case WasmOp::I64Const:       return "I64Const";
    case WasmOp::I64Add:         return "I64Add";
    case WasmOp::I64Sub:         return "I64Sub";
    case WasmOp::I64Mul:         return "I64Mul";
    case WasmOp::I64DivS:        return "I64DivS";
    case WasmOp::I64DivU:        return "I64DivU";
    case WasmOp::I64RemS:        return "I64RemS";
    case WasmOp::I64RemU:        return "I64RemU";
    case WasmOp::I64And:         return "I64And";
    case WasmOp::I64Or:          return "I64Or";
    case WasmOp::I64Xor:         return "I64Xor";
    case WasmOp::I64Shl:         return "I64Shl";
    case WasmOp::I64ShrS:        return "I64ShrS";
    case WasmOp::I64ShrU:        return "I64ShrU";
    case WasmOp::I64Clz:         return "I64Clz";
    case WasmOp::I64Ctz:         return "I64Ctz";
    case WasmOp::I64Popcnt:      return "I64Popcnt";
    case WasmOp::I64Eqz:         return "I64Eqz";
    case WasmOp::I64Eq:          return "I64Eq";
    case WasmOp::I64Ne:          return "I64Ne";
    case WasmOp::I64LtS:         return "I64LtS";
    case WasmOp::I64LtU:         return "I64LtU";
    case WasmOp::I64GtS:         return "I64GtS";
    case WasmOp::I64GtU:         return "I64GtU";
    case WasmOp::I64LeS:         return "I64LeS";
    case WasmOp::I64LeU:         return "I64LeU";
    case WasmOp::I64GeS:         return "I64GeS";
    case WasmOp::I64GeU:         return "I64GeU";
    case WasmOp::I32WrapI64:     return "I32WrapI64";
    case WasmOp::I64ExtendI32S:  return "I64ExtendI32S";
    case WasmOp::I64ExtendI32U:  return "I64ExtendI32U";
    case WasmOp::F64Const:       return "F64Const";
    case WasmOp::F64Add:         return "F64Add";
    case WasmOp::F64Sub:         return "F64Sub";
    case WasmOp::F64Mul:         return "F64Mul";
    case WasmOp::F64Div:         return "F64Div";
    case WasmOp::F64Abs:         return "F64Abs";
    case WasmOp::F64Neg:         return "F64Neg";
    case WasmOp::F64Sqrt:        return "F64Sqrt";
    case WasmOp::F64Min:         return "F64Min";
    case WasmOp::F64Max:         return "F64Max";
    case WasmOp::F64Eq:          return "F64Eq";
    case WasmOp::F64Ne:          return "F64Ne";
    case WasmOp::F64Lt:          return "F64Lt";
    case WasmOp::F64Gt:          return "F64Gt";
    case WasmOp::F64Le:          return "F64Le";
    case WasmOp::F64Ge:          return "F64Ge";
    case WasmOp::F64ConvertI32S: return "F64ConvertI32S";
    case WasmOp::F64ConvertI32U: return "F64ConvertI32U";
    case WasmOp::F64ConvertI64S: return "F64ConvertI64S";
    case WasmOp::F64ConvertI64U: return "F64ConvertI64U";
    case WasmOp::I32TruncF64S:   return "I32TruncF64S";
    case WasmOp::I32TruncF64U:   return "I32TruncF64U";
    case WasmOp::I64TruncF64S:   return "I64TruncF64S";
    case WasmOp::I64TruncF64U:   return "I64TruncF64U";
    case WasmOp::MemorySize:     return "MemorySize";
    case WasmOp::MemoryCopy:     return "MemoryCopy";
    case WasmOp::MemoryFill:     return "MemoryFill";
    case WasmOp::Unsupported:    return "Unsupported";
    default:                     return "Unknown";
    }
}

void dumpInstr(const Instr& instr, int indent) {
    for (int i = 0; i < indent; i++) printf("  ");

    switch (instr.op) {
    case WasmOp::FuncInfo:
        printf("FuncInfo(numParams=%d)\n", instr.operand); break;
    case WasmOp::LocalGet:
        printf("LocalGet(%d)\n", instr.operand); break;
    case WasmOp::LocalSet:
        printf("LocalSet(%d)\n", instr.operand); break;
    case WasmOp::LocalTee:
        printf("LocalTee(%d)\n", instr.operand); break;
    case WasmOp::I32Const:
        printf("I32Const(%d)\n", instr.operand); break;
    case WasmOp::I64Const:
        printf("I64Const(%lld)\n", (long long)instr.i64operand); break;
    case WasmOp::F64Const:
        printf("F64Const(%g)\n", instr.foperand); break;
    case WasmOp::Br:
        printf("Br(depth=%d)\n", instr.operand); break;
    case WasmOp::Br_if:
        printf("Br_if(depth=%d)\n", instr.operand); break;
    case WasmOp::BrTable:
        printf("BrTable(depth=%d)\n", instr.operand); break;
    case WasmOp::Block:
        printf("Block(depth=%d)\n", instr.operand); break;
    case WasmOp::Call:
        printf("Call(func=%d)\n", instr.operand); break;
    case WasmOp::GlobalGet:
        printf("GlobalGet(%d)\n", instr.operand); break;
    case WasmOp::GlobalSet:
        printf("GlobalSet(%d)\n", instr.operand); break;
    case WasmOp::I32Load:
        printf("I32Load(offset=%d)\n", instr.operand); break;
    case WasmOp::I32Store:
        printf("I32Store(offset=%d)\n", instr.operand); break;
    case WasmOp::I64Load:
        printf("I64Load(offset=%d)\n", instr.operand); break;
    case WasmOp::I64Store:
        printf("I64Store(offset=%d)\n", instr.operand); break;
    case WasmOp::F64Load:
        printf("F64Load(offset=%d)\n", instr.operand); break;
    case WasmOp::F64Store:
        printf("F64Store(offset=%d)\n", instr.operand); break;
    case WasmOp::End:
        if (instr.operand == 0)      printf("End(loop)\n");
        else if (instr.operand == 1) printf("End(block)\n");
        else                         printf("End(if)\n");
        break;
    default:
        printf("%s\n", opToString(instr.op)); break;
    }
}

void dumpInstrSeq(const InstrSeq& seq) {
    printf("=== InstrSeq (total: %zu instructions) ===\n", seq.size());
    for (size_t i = 0; i < seq.size(); i++) {
        printf("[%3zu] ", i);
        dumpInstr(seq[i], 0);
    }
    printf("=== End of InstrSeq ===\n");
}