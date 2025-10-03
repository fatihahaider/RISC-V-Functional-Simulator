#include "sim.h"

using namespace std;

// RV64I without csr, environment, or fence instructions

//           31          25 24 20 19 15 14    12 11          7 6      0
// R  type: | funct7       | rs2 | rs1 | funct3 | rd          | opcode |
// I  type: | imm[11:0]          | rs1 | funct3 | rd          | opcode |
// S  type: | imm[11:5]    | rs2 | rs1 | funct3 | imm[4:0]    | opcode |
// SB type: | imm[12|10:5] | rs2 | rs1 | funct3 | imm[4:1|11] | opcode |
// U  type: | imm[31:12]                        | rd          | opcode |
// UJ type: | imm[20|10:1|11|19:12]             | rd          | opcode |

// initialize memory with program binary
bool initMemory(char *programFile, MemoryStore *myMem) {
    // open instruction file
    ifstream infile;
    infile.open(programFile, ios::binary | ios::in);

    if (!infile.is_open()) {
        fprintf(stderr, "\tError open input file\n");
        return false;
    }

    // get length of the file and read instruction file into a buffer
    infile.seekg(0, ios::end);
    int length = infile.tellg();
    infile.seekg (0, ios::beg);

    char *buf = new char[length];
    infile.read(buf, length);
    infile.close();

    int memLength = length / sizeof(buf[0]);
    int i;
    for (i = 0; i < memLength; i++) {
        myMem->setMemValue(i * BYTE_SIZE, buf[i], BYTE_SIZE);
    }

    return true;
}

// dump registers and memory
void dump(MemoryStore *myMem) {

    dumpRegisterState(regData.reg);
    dumpMemoryState(myMem);
}

// TODO All functions below (except main) are incomplete.
// Only ADDI is implemented. Your task is to complete these functions.

uint64_t bitExtract(uint64_t instruction, int high, int low) {
    uint64_t width = hi - lo + 1;
    return (x >> lo) & ((width == 64) ? ~0ULL : ((1ULL << width) - 1));
}

uint64_t signExtend(uint64_t code, int num) {
    // bits is the source width (e.g., 12 for I-imm)
    uint64_t m = 1ULL << (bits - 1);
    return (x ^ m) - m;
}

// Get raw instruction bits from memory
Instruction simFetch(uint64_t PC, MemoryStore *myMem) {
    // fetch current instruction
    uint64_t instruction;
    myMem->getMemValue(PC, instruction, WORD_SIZE);
    instruction = (uint32_t)instruction;

    Instruction inst;
    inst.PC = PC;
    inst.instruction = instruction;
    return inst;
}

// Determine instruction opcode, funct, reg names, and what resources to use
Instruction simDecode(Instruction inst) {
    inst.opcode = inst.instruction & 0b1111111;
    
    // inst.rs2   = (inst.instruction >> 20) & 0b11111; 
    // inst.funct7 = (inst.instruction >> 25) & 0b1111111;
    // inst.funct3 = inst.instruction >> 12 & 0b111;
    // inst.rd = inst.instruction >> 7 & 0b11111;
    // inst.rs1 = inst.instruction >> 15 & 0b11111;
    // inst.imm = inst.instruction >> 20 & 0b111111111111;

    if (inst.instruction == 0xfeedfeed) {
        inst.isHalt = true;
        return inst; // halt instruction
    }
    if (inst.instruction == 0x00000013) { // addi 
        inst.isNop = true;
        return inst; // NOP instruction
    }
    inst.isLegal = true; // assume legal unless proven otherwise

    switch (inst.opcode) { //addi, slli, slti, sltiu, xori, srli, srai, ori, andi
        case OP_INTIMM:
            
            inst.doesArithLogic = true;
            inst.writesRd = true;
            inst.readsRs1 = true;
            inst.readsRs2 = false;

            if (inst.funct3 == FUNCT3_ADD_SUB || 
                inst.funct3 == FUNCT3_SLL || 
                inst.funct3 == FUNCT3_SLT || 
                inst.funct3 == FUNCT3_SLTU ||
                inst.funct3 == FUNCT3_XOR || 
                inst.funct3 == FUNCT3_SRL_SRA || 
                inst.funct3 == FUNCT3_OR || 
                inst.funct3 == FUNCT3_AND) {
                    //pass
            } 
            else {
                inst.isLegal = false;
            }
            break;
        
        case OP_INTIMMW: // addiw, slliw, srliw, sraiw
            inst.funct3 = inst.instruction >> 12 & 0b111;
            inst.rd = inst.instruction >> 7 & 0b11111;
            inst.rs1 = inst.instruction >> 15 & 0b11111;
            inst.imm = inst.instruction >> 20 & 0b111111111111;

            inst.doesArithLogic = true;
            inst.writesRd = true;
            inst.readsRs1 = true;
            inst.readsRs2 = false;

            if (inst.funct3 == FUNCT3_ADD_SUB || 
                inst.funct3 == FUNCT3_SLL || 
                inst.funct3 == FUNCT3_SRL_SRA) {
                    //pass
            } 
            else {
                inst.isLegal = false;
            }

            break;
        
        case OP_LOAD: // lb, lh, lw, ld, lbu, lhu, lwu
            inst.doesArithLogic = true;
            inst.writesRd = true;
            inst.readsMem = true;
            inst.readsRs1 = true;
            inst.readsRs2 = false;

            if (inst.funct3 == FUNCT3_LB || 
                inst.funct3 == FUNCT3_LH || 
                inst.funct3 == FUNCT3_LW || 
                inst.funct3 == FUNCT3_LD || 
                inst.funct3 == FUNCT3_LBU || 
                inst.funct3 == FUNCT3_LHU || 
                inst.funct3 == FUNCT3_LWU) {
                    //pass
            } 
            else {
                inst.isLegal = false;
            }
            
            break; 

        case OP_RTYPE: //add, sub, sll, slt, sltu, xor, srl, sra, or, and
            inst.doesArithLogic = true;
            inst.writesRd = true;
            inst.readsRs1 = true;
            inst.readsRs2 = true;

            if (inst.funct3 == FUNCT3_ADD_SUB || 
                inst.funct3 == FUNCT3_SLL || 
                inst.funct3 == FUNCT3_SLT || 
                inst.funct3 == FUNCT3_SLTU ||
                inst.funct3 == FUNCT3_XOR || 
                inst.funct3 == FUNCT3_SRL_SRA || 
                inst.funct3 == FUNCT3_OR || 
                inst.funct3 == FUNCT3_AND) {
                    //pass
            } 
            else {
                inst.isLegal = false;
            }
            break;

        case OP_RTYPEW: //addw, subw, sllw, srlw, sraw
            inst.doesArithLogic = true;
            inst.writesRd = true;
            inst.readsRs1 = true;
            inst.readsRs2 = true;

            if (inst.funct3 == FUNCT3_ADD_SUB || 
                inst.funct3 == FUNCT3_SLL || 
                inst.funct3 == FUNCT3_SRL_SRA ) {
                    //pass
            } 
            else {
                inst.isLegal = false;
            }
            break;

        case OP_STORE: // sb, sh, sw, sd
            inst.doesArithLogic = true;
            inst.readsRs1 = true;
            inst.readsRs2 = true;
            inst.writesMem = true;
            inst.writesRd = false;

            if (inst.funct3 == FUNCT3_SB || 
                inst.funct3 == FUNCT3_SH || 
                inst.funct3 == FUNCT3_SW ||
                inst.funct3 == FUNCT3_SD) {
                    //pass
            } 
            else {
                inst.isLegal = false;
            } 

            break;
        
        case OP_SBTYPE: // beq, bne, blt, bge, bltu, bgeu
            inst.doesArithLogic = true;
            inst.readsRs1 = true;
            inst.readsRs2 = true;
            inst.writesRd = false;

            if (inst.funct3 == FUNCT3_BEQ || 
                inst.funct3 == FUNCT3_BNE ||
                inst.funct3 == FUNCT3_BLT ||
                inst.funct3 == FUNCT3_BGE ||
                inst.funct3 == FUNCT3_BLTU ||
                inst.funct3 == FUNCT3_BGEU) {
                    //pass
            } 
            else {
                inst.isLegal = false;
            } 
            break;
        
        case OP_LUI:
            inst.doesArithLogic = true;
            inst.readsRs1 = false;
            inst.readsRs2 = false;
            inst.writesRd = true;

            break;

        case OP_AUIPC:
            inst.doesArithLogic = true;
            inst.readsRs1 = false;
            inst.readsRs2 = false;
            inst.writesRd = true;
            
            break;

        case OP_JAL:
            inst.doesArithLogic = true;
            inst.readsRs1 = false;
            inst.readsRs2 = false;
            inst.writesRd = true;
        
            break;

        case OP_JALR:
            inst.doesArithLogic = true;
            inst.readsRs1 = true;
            inst.readsRs2 = false;
            inst.writesRd = true;
            
            if (inst.funct3 == FUNCT3_JALR) {
            //pass
            }
            else { 
                inst.isLegal = false;
            }
            break;

        default:
            inst.isLegal = false;
    }
    return inst;
}

// Collect reg operands for arith or addr gen
Instruction simOperandCollection(Instruction inst, REGS regData) {
    if (inst.readsRs1){
        inst.op1Val = regData.registers[inst.rs1];
    }

    if(inst.readsRs2){
        inst.op2Val = regData.registers[inst.rs2];
    }
    return inst;
}

// determine the type of instruction R, I, S, SB, U, UJ and instruction type 
Instruction instructionTypeandBits(Instruction inst) {
    if (inst.opcode == OP_RTYPE || inst.opcode == OP_RTYPEW) {
        inst.isR = true;

        inst.rd     = (inst.instruction >> 7)  & 0b11111;
        inst.funct3 = (inst.instruction >> 12) & 0b111;
        inst.rs1    = (inst.instruction >> 15) & 0b11111;
        inst.rs2    = (inst.instruction >> 20) & 0b11111;
        inst.funct7 = (inst.instruction >> 25) & 0b1111111;
    }
    else if (inst.opcode == OP_INTIMM || inst.opcode == OP_INTIMMW || 
    inst.opcode == OP_LOAD || inst.opcode == OP_JALR) {
        inst.isI = true;
        
        inst.funct3 = inst.instruction >> 12 & 0b111;
        inst.rd = inst.instruction >> 7 & 0b11111;
        inst.rs1 = inst.instruction >> 15 & 0b11111;
        inst.imm = inst.instruction >> 20 & 0b111111111111;
    }
    else if (inst.opcode == OP_STORE) {
        inst.isS = true;

        inst.funct3 = (inst.instruction >> 12) & 0b111;
        inst.rs1 = (inst.instruction >> 15) & 0b11111;
        inst.rs2 = (inst.instruction >> 20) & 0b11111;
        uint64_t immhi = (inst.instruction >> 25) & 0x7F;
        uint64_t immlo = (inst.instruction >> 7)  & 0x1F;
        inst.imm = signExtend((immhi << 5) | immlo, 12);
    }
    else if (inst.opcode == OP_SBTYPE) {
        inst.isSB = true;

        inst.funct3 = (inst.instruction >> 12) & 0b111;
        inst.rs1 = (inst.instruction >> 15) & 0b11111;
        inst.rs2 = (inst.instruction >> 20) & 0b11111;
        uint64_t imm12 = (inst.instruction >> 31) & 0x1;
        uint64_t imm10_5 = (inst.instruction >> 25) & 0x3F;
        uint64_t imm4_1 = (inst.instruction >> 8)  & 0xF;
        uint64_t imm11 = (inst.instruction >> 7)  & 0x1;
        inst.imm    = signExtend((imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1 << 1), 13);
    }
    else if (inst.opcode == OP_LUI || inst.opcode == OP_AUIPC) {
        inst.isU = true;

        inst.rd  = (inst.instruction >> 7)  & 0b11111;
        inst.imm = (inst.instruction & 0xFFFFF000);
    }
    else if (inst.opcode == OP_JAL) {
        inst.isUJ = true;

        inst.rd = (inst.instruction >> 7) & 0b11111;
        uint64_t imm20   = (inst.instruction >> 31) & 0x1;
        uint64_t imm10_1 = (inst.instruction >> 21) & 0x3FF;
        uint64_t imm11   = (inst.instruction >> 20) & 0x1;
        uint64_t imm19_12= (inst.instruction >> 12) & 0xFF;
        inst.imm = signExtend((imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1 << 1), 21);
    }
    return inst;
}

// Resolve next PC whether +4 or branch/jump target
Instruction simNextPCResolution(Instruction inst) {

    if(inst.isSB) { 
        
    }

    inst.nextPC = inst.PC + 4;

    return inst;
}

// Perform arithmetic/logic operations
Instruction simArithLogic(Instruction inst) {
    uint64_t imm12  = inst.instruction >> 20 & 0b111111111111;
    uint64_t sext_imm12 = (imm12 & 0x800) ? (imm12 | 0xFFFFFFFFFFFFF000) : imm12;

    inst.arithResult = inst.op1Val + sext_imm12;
      
    return inst;
}

// Generate memory address for load/store instructions
Instruction simAddrGen(Instruction inst) {
    return inst;
}

// Perform memory access for load/store instructions
Instruction simMemAccess(Instruction inst, MemoryStore *myMem) {
    return inst;
}

// Write back results to registers
Instruction simCommit(Instruction inst, REGS &regData) {

    // regData here is passed by reference, so changes will be reflected in original
    regData.registers[inst.rd] = inst.arithResult;

    return inst;
}

// Simulate the whole instruction using functions above
Instruction simInstruction(uint64_t &PC, MemoryStore *myMem, REGS &regData) {
    Instruction inst = simFetch(PC, myMem);
    inst = simDecode(inst);
    if (!inst.isLegal || inst.isHalt) return inst;
    inst = simOperandCollection(inst, regData);
    inst = simNextPCResolution(inst);
    inst = simArithLogic(inst);
    inst = simAddrGen(inst);
    inst = simMemAccess(inst, myMem);
    inst = simCommit(inst, regData);
    PC = inst.nextPC;
    return inst;
}

int main(int argc, char** argv) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <instruction_file>\n", argv[0]);
        return -1;
    }

    // initialize memory store with buffer contents
    MemoryStore *myMem = createMemoryStore();
    if (!initMemory(argv[1], myMem)) {
        fprintf(stderr, "Failed to initialize memory with program binary.\n");
        return -1;
    }

    // initialize registers and program counter
    regData.reg = {};
    PC = 0;
    bool err = false;
    
    // start simulation
    while (!err) {
        Instruction inst = simInstruction(PC, myMem, regData);
        if (inst.isHalt) {
            // Normal dump and exit
            dump(myMem);
            return 0;
        }
        if (!inst.isLegal) {
            fprintf(stderr, "Illegal instruction encountered at PC: 0x%lx\n", inst.PC);
            err = true;
        }
    }

    // dump and exit with error
    dump(myMem);
    exit(127);
    return -1;
}



