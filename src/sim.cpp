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


// printing toggle mode
static const bool DEBUG_MODE = true;  



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

int64_t signExtend(uint64_t x, int bits) {
    uint64_t m = 1ULL << (bits - 1);
    return (int64_t)((x ^ m) - m);
}

// Get raw instruction bits from memory
Instruction simFetch(uint64_t PC, MemoryStore *myMem) {
    // fetch current instruction
    uint64_t instruction;
    myMem->getMemValue(PC, instruction, WORD_SIZE);
    instruction = (uint32_t)instruction;

    printf("Fetched instruction 0x%08lx at PC=0x%lx\n", instruction, PC);

    Instruction inst;
    inst.PC = PC;
    inst.instruction = instruction;
    return inst;
}

// Determine instruction opcode, funct, reg names, and what resources to use
Instruction simDecode(Instruction inst) {
    inst = instructionTypeandBits(inst);
    
    if (inst.instruction == 0xfeedfeed) {
        inst.isHalt = true;
        return inst; // halt instruction
    }
    if (inst.instruction == 0x00000013) { // addi 
        inst.isNop = true;
        return inst; // NOP instruction
    }
    inst.isLegal = true; // assume legal unless proven otherwise

    // Needed for I/IW shift-immediate legality
    uint8_t immHi  = 0; // imm[11:5]
    uint8_t shamt6 = 0; // 6-bit shamt (RV64)
    uint8_t shamt5 = 0; // 5-bit shamt (W)

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

                // I-type shift-immediates (RV64 -> 6-bit shamt)
                if (inst.funct3 == FUNCT3_SLL) {
                    immHi  = (inst.instruction >> 25) & 0x7F; // imm[11:5]
                    shamt6 = (inst.instruction >> 20) & 0x3F; // 6-bit shamt

                    // SLLI: imm[11:5] must be 0000000; shamt in [0..63]
                    if (immHi != 0b0000000) {
                    inst.isLegal = false;
                    }

                    if (shamt6 >= 64) {
                    inst.isLegal = false; // checking the bound
                    }
                } 
                else if (inst.funct3 == FUNCT3_SRL_SRA) {
                    immHi  = (inst.instruction >> 25) & 0x7F; // imm[11:5]
                    shamt6 = (inst.instruction >> 20) & 0x3F;

                    // SRLI (0000000) or SRAI (0100000) only; shamt in [0..63]
                    if (!(immHi == 0b0000000 || immHi == 0b0100000)) {
                    inst.isLegal = false;
                    }

                    if (shamt6 >= 64) {
                    inst.isLegal = false; // checking the bound
                    }
                }      
                }
            else {
                inst.isLegal = false;
            }
            break;
        
        case OP_INTIMMW: // addiw, slliw, srliw, sraiw
            inst.doesArithLogic = true;
            inst.writesRd = true;
            inst.readsRs1 = true;
            inst.readsRs2 = false;

            if (inst.funct3 == FUNCT3_ADD_SUB || 
                inst.funct3 == FUNCT3_SLL || 
                inst.funct3 == FUNCT3_SRL_SRA) {

                // I-W shift-immediates (W -> 5-bit shamt)
                if (inst.funct3 == FUNCT3_SLL) {
                    immHi  = (inst.instruction >> 25) & 0x7F; // imm[11:5]
                    shamt5 = (inst.instruction >> 20) & 0x1F; // 5-bit shamt

                    // SLLIW: imm[11:5] must be 0000000; shamt in [0..31]
                    if (immHi != 0b0000000) {
                    inst.isLegal = false;
                    }

                    if (shamt5 >= 32) {      
                        inst.isLegal = false; // checking the bound
                    }
                } 
                else if (inst.funct3 == FUNCT3_SRL_SRA) {
                    immHi  = (inst.instruction >> 25) & 0x7F; // imm[11:5]
                    shamt5 = (inst.instruction >> 20) & 0x1F;

                    // SRLIW (0000000) or SRAIW (0100000); shamt in [0..31]
                    if (!(immHi == 0b0000000 || immHi == 0b0100000)) {
                        inst.isLegal = false;
                    }
                    if (shamt5 >= 32) {      
                        inst.isLegal = false; // checking the bound
                    }
                }
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
                // pass
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
                // R/RW use inst.funct7 directly
                if (inst.funct3 == FUNCT3_ADD_SUB || inst.funct3 == FUNCT3_SRL_SRA) {
                    // ADD/SUB and SRL/SRA allow funct7 = 0000000 or 0100000
                    if (!((inst.funct7 == FUNCT7_DEFAULT) || (inst.funct7 == FUNCT7_SUB_SRA))) { 
                        inst.isLegal = false;
                    }
                }
                    // all other R ops require funct7 == 0000000
                    else if (inst.funct7 != FUNCT7_DEFAULT) {
                        inst.isLegal = false;
                    }
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

                if (inst.funct3 == FUNCT3_ADD_SUB || inst.funct3 == FUNCT3_SRL_SRA) {
                    // ADDW/SUBW and SRLW/SRAW allow funct7 = 0000000 or 0100000
                    if (!((inst.funct7 == FUNCT7_DEFAULT) || (inst.funct7 == FUNCT7_SUB_SRA))) {
                        inst.isLegal = false;
                    }
                }
                    else if (inst.funct7 != FUNCT7_DEFAULT) { // SLLW
                        inst.isLegal = false;
                    }
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
                // pass
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
                // pass
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

    if (DEBUG_MODE) {
        std::cout << "Operand Collection:" << std::endl;
        std::cout << "rs1(x" << std::dec << inst.rs1 << ") = 0x" 
                << std::hex << inst.op1Val << std::endl;
        if (inst.readsRs2)
            std::cout << "rs2(x" << std::dec << inst.rs2 << ") = 0x" 
                    << std::hex << inst.op2Val << std::endl;
        std::cout << std::endl;
    }

    return inst;
}

// determine the type of instruction R, I, S, SB, U, UJ and instruction type 
Instruction instructionTypeandBits(Instruction inst) {
        inst.opcode = inst.instruction & 0b1111111;

    if (inst.opcode == OP_RTYPE || inst.opcode == OP_RTYPEW) {
        inst.isR = true;

        inst.rd     = (inst.instruction >> 7)  & 0b11111;
        inst.funct3 = (inst.instruction >> 12) & 0b111;
        inst.rs1    = (inst.instruction >> 15) & 0b11111;
        inst.rs2    = (inst.instruction >> 20) & 0b11111;
        inst.funct7 = (inst.instruction >> 25) & 0b1111111;
        inst.imm = 0; 
    }
    else if (inst.opcode == OP_INTIMM || inst.opcode == OP_INTIMMW || 
    inst.opcode == OP_LOAD || inst.opcode == OP_JALR) {
        
        inst.isI = true;
        
        inst.funct3 = inst.instruction >> 12 & 0b111;
        inst.rd = inst.instruction >> 7 & 0b11111;
        inst.rs1 = inst.instruction >> 15 & 0b11111;
        inst.imm = inst.instruction >> 20 & 0b111111111111;
        uint64_t imm12 = (inst.instruction >> 20) & 0xFFF;
        inst.imm = signExtend(imm12, 12);
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
        inst.imm = signExtend((imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1 << 1), 13);
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

    if (DEBUG_MODE) {
        std::cout << "Decoded Instruction:" << std::endl;
        std::cout << "------------------------------------" << std::endl;
        std::cout << "PC: 0x" << std::hex << inst.PC 
                << "   Raw: 0x" << inst.instruction << std::endl;
        std::cout << "Opcode: 0x" << std::hex << inst.opcode 
                << "   funct3: 0x" << inst.funct3 
                << "   funct7: 0x" << inst.funct7 << std::endl;
        std::cout << "rd: x" << std::dec << inst.rd 
                << "   rs1: x" << inst.rs1 
                << "   rs2: x" << inst.rs2 << std::endl;
        std::cout << "imm: 0x" << std::hex << inst.imm 
                << "   Type: " 
                << (inst.isR ? "R" : inst.isI ? "I" : inst.isS ? "S" :
                    inst.isSB ? "SB" : inst.isU ? "U" : inst.isUJ ? "UJ" : "?")
                << std::endl;
        std::cout << "------------------------------------" << std::endl << std::endl;
    }


    return inst; 
}

// Resolve next PC whether +4 or branch/jump target
Instruction simNextPCResolution(Instruction inst) {

    if (inst.isSB) {
        bool takeBranch = false;
        switch (inst.funct3) {
            case FUNCT3_BEQ:  takeBranch = (inst.op1Val == inst.op2Val); break;
            case FUNCT3_BNE:  takeBranch = (inst.op1Val != inst.op2Val); break;
            case FUNCT3_BLT:  takeBranch = ((int64_t)inst.op1Val < (int64_t)inst.op2Val); break;
            case FUNCT3_BGE:  takeBranch = ((int64_t)inst.op1Val >= (int64_t)inst.op2Val); break;
            case FUNCT3_BLTU: takeBranch = (inst.op1Val < inst.op2Val); break;
            case FUNCT3_BGEU: takeBranch = (inst.op1Val >= inst.op2Val); break;
        }
        inst.nextPC = takeBranch ? inst.PC + inst.imm : inst.PC + 4;
    }

    else if (inst.opcode == OP_JAL) {
        inst.nextPC = inst.PC + inst.imm;
    }

    else if (inst.opcode == OP_JALR){
        inst.nextPC = (inst.op1Val + inst.imm) & ~1ULL;
    }
    else {
        inst.nextPC = inst.PC + 4;
    }

    return inst;
}

// Perform arithmetic/logic operations
Instruction simArithLogic(Instruction inst) {
    switch (inst.opcode) {

        // -------- I-TYPE: ALU immediates --------
        case OP_INTIMM: // addi, slli, slti, sltiu, xori, srli, srai, ori, andi
            switch (inst.funct3) {
                case FUNCT3_ADD_SUB: // addi 
                    inst.arithResult = (int64_t) inst.op1Val + inst.imm; // typecast?
                    break;
                case FUNCT3_SLL: //slli
                    inst.arithResult = inst.op1Val << (inst.imm & 0b111111);
                    break;
                case FUNCT3_SLT: //slti 
                    inst.arithResult = ((int64_t)inst.op1Val < (int64_t)inst.imm) ? 1 : 0;
                    break;
                case FUNCT3_SLTU: //sltiu 
                    inst.arithResult = (inst.op1Val < (uint64_t)inst.imm) ? 1 : 0;
                    break;
                case FUNCT3_XOR: //xori
                    inst.arithResult = inst.op1Val ^ inst.imm; 
                    break;
                case FUNCT3_SRL_SRA: 
                // further check funct7 (SRAI vs SRLI)
                    if (inst.funct7 == FUNCT7_DEFAULT){ // srli
                        inst.arithResult = (uint64_t)inst.op1Val >> (inst.imm & 0b111111);
                    }
                    else if (inst.funct7 == FUNCT7_SUB_SRA ){ // srai
                        inst.arithResult = (int64_t)inst.op1Val >> (inst.imm & 0b111111); // understand the sign nad unsigned

                    }
                    break;
                case FUNCT3_OR: //ori
                    inst.arithResult = inst.op1Val | inst.imm; 
                    break;
                case FUNCT3_AND: //andi
                    inst.arithResult = inst.op1Val & inst.imm; 
                    break;
            }
            break;

        // -------- I-TYPE W (32-bit ops) --------
        // Integer ALU immediate instructions (keep low 32-bits and sign extends) addiw, slliw, srliw, sraiw
        case OP_INTIMMW:
            switch (inst.funct3) {
                case FUNCT3_ADD_SUB: //addiw
                    inst.arithResult = (u_int32_t)(inst.op1Val + inst.imm);
                    
                    break;
                case FUNCT3_SLL: //slliw
                    inst.arithResult = (u_int32_t)(inst.op1Val << (inst.imm & 0b111111));

                    break;
                case FUNCT3_SRL_SRA: 
                    // further check funct7 (SRAIW vs SRLIW)
                    if (inst.funct7 == FUNCT7_SUB_SRA){ //sraiw
                        inst.arithResult = (int32_t)((int32_t)inst.op1Val >> (inst.imm & 0x1F));

                    }
                    else if (inst.funct7 == FUNCT7_DEFAULT ){ //srliw
                        inst.arithResult = (int32_t)((uint32_t)inst.op1Val >> (inst.imm & 0x1F));

                    }
                    break;
            }
            break;

        // -------- R-TYPE --------
        // Register to Register instructions add, sub, sll, slt, sltu, xor, srl, sra, or, and
        case OP_RTYPE:
            switch (inst.funct3) {
                case FUNCT3_ADD_SUB: 
                    // further check funct7 (ADD vs SUB)
                    if (inst.funct7 == FUNCT7_DEFAULT){ //add
                        inst.arithResult = (int64_t)inst.op1Val + (int64_t)inst.op2Val; 

                    }
                    else if (inst.funct7 == FUNCT7_SUB_SRA ){ //sub
                        inst.arithResult = (int64_t)inst.op1Val -  (int64_t)inst.op2Val; 

                    }
                    break;
                case FUNCT3_SLL: 
                    inst.arithResult = inst.op1Val << (inst.op2Val & 0b111111); 
                    break;
                case FUNCT3_SLT:
                    inst.arithResult =  ((int64_t)inst.op1Val < (int64_t)inst.op2Val) ? 1 : 0;
                    break;
                case FUNCT3_SLTU: 
                    inst.arithResult = (inst.op1Val < inst.op2Val) ? 1 : 0;
                    break;
                case FUNCT3_XOR:
                    inst.arithResult =  inst.op1Val ^ inst.op2Val;
                    break;
                case FUNCT3_SRL_SRA: 
                    // further check funct7 (SRA vs SRL)
                    if (inst.funct7 == FUNCT7_DEFAULT){ // srl
                        inst.arithResult = (uint64_t)inst.op1Val >> (inst.op2Val & 0b111111);
                    }
                    else if (inst.funct7 == FUNCT7_SUB_SRA ){ // sra
                        inst.arithResult = (int64_t)inst.op1Val >> (inst.op2Val & 0b111111);

                    }
                    break;
                case FUNCT3_OR: 
                    inst.arithResult = inst.op1Val | inst.op2Val;
                    break;
                case FUNCT3_AND: 
                    inst.arithResult = inst.op1Val & inst.op2Val;
                    break;
            }
            break;

        // -------- R-TYPE W (32-bit ops) --------
        // Register to Register instructions (keep low 32-bits and sign extends) addw, subw, sllw, srlw, sraw
        case OP_RTYPEW:
            switch (inst.funct3) {
                case FUNCT3_ADD_SUB: 
                    // further check funct7 (ADDW vs SUBW)
                    if (inst.funct7 == FUNCT7_DEFAULT){ //addw
                        inst.arithResult = (int64_t)((int32_t)inst.op1Val + (int32_t)inst.op2Val);

                    }
                    else if (inst.funct7 == FUNCT7_SUB_SRA ){ //subw
                        inst.arithResult = (int64_t)((int32_t)inst.op1Val - (int32_t)inst.op2Val);

                    }
                    break;
                case FUNCT3_SLL:  //sllw
                    inst.arithResult = (int64_t)(int32_t)((int32_t)inst.op1Val << (inst.op2Val & 0x1F));
                    break;
                case FUNCT3_SRL_SRA: 
                    // further check funct7 (SRAW vs SRLW)
                    if (inst.funct7 == FUNCT7_SUB_SRA){ // sraw
                        inst.arithResult = (int64_t)(int32_t)((int32_t)inst.op1Val >> (inst.op2Val & 0x1F));
                    }
                    else if (inst.funct7 == FUNCT7_DEFAULT){ // srlw
                        inst.arithResult =(int64_t)(int32_t)((uint32_t)inst.op1Val >> (inst.op2Val & 0x1F));
                    }
                    break;
            }
            break;

        // -------- U-TYPE --------
        case OP_LUI: // lui
            inst.arithResult = inst.imm;
            break;
        case OP_AUIPC: // auipc
            inst.arithResult = inst.imm + inst.PC;
            break;
    }
    return inst;
}



// Generate memory address for load/store instructions
Instruction simAddrGen(Instruction inst) {
    // For I-type LOADs and S-type STOREs: addr = rs1 + imm (imm is already sign-extended)
    if (inst.opcode == OP_LOAD || inst.opcode == OP_STORE) {
        inst.memAddress = inst.op1Val + inst.imm;
    }
    return inst;
}


Instruction simMemAccess(Instruction inst, MemoryStore *myMem) {
    if (inst.opcode == OP_LOAD) {
        uint64_t val = 0;

        switch (inst.funct3) {
            case FUNCT3_LB: {   // signed 8
                myMem->getMemValue(inst.memAddress, val, BYTE_SIZE);
                int8_t  v8  = (int8_t)val;
                inst.memResult = (int64_t)v8;           // sign-extend to 64
                break;
            }
            case FUNCT3_LH: {   // signed 16
                myMem->getMemValue(inst.memAddress, val, HALF_SIZE);
                int16_t v16 = (int16_t)val;
                inst.memResult = (int64_t)v16;          // sign-extend
                break;
            }
            case FUNCT3_LW: {   // signed 32
                myMem->getMemValue(inst.memAddress, val, WORD_SIZE);
                int32_t v32 = (int32_t)val;
                inst.memResult = (int64_t)v32;          // sign-extend
                break;
            }
            case FUNCT3_LD: {   // 64
                myMem->getMemValue(inst.memAddress, val, DOUBLE_SIZE);
                inst.memResult = val;                   // already 64-bit
                break;
            }
            case FUNCT3_LBU: {  // unsigned 8
                myMem->getMemValue(inst.memAddress, val, BYTE_SIZE);
                uint8_t v8 = (uint8_t)val;
                inst.memResult = (uint64_t)v8;          // zero-extend
                break;
            }
            case FUNCT3_LHU: {  // unsigned 16
                myMem->getMemValue(inst.memAddress, val, HALF_SIZE);
                uint16_t v16 = (uint16_t)val;
                inst.memResult = (uint64_t)v16;         // zero-extend
                break;
            }
            case FUNCT3_LWU: {  // unsigned 32 (RV64)
                myMem->getMemValue(inst.memAddress, val, WORD_SIZE);
                uint32_t v32 = (uint32_t)val;
                inst.memResult = (uint64_t)v32;         // zero-extend
                break;
            }
            default:
                // should have been marked illegal in decode
                break;
        }

        // Loads write rd: pass value forward via arithResult for commit
        inst.arithResult = inst.memResult;
    }
    else if (inst.opcode == OP_STORE) {
        // rs2 contains data to store (already in op2Val)
        switch (inst.funct3) {
            case FUNCT3_SB: {  // store 8
                uint8_t v8 = (uint8_t)(inst.op2Val & 0xFF);
                myMem->setMemValue(inst.memAddress, (uint64_t)v8, BYTE_SIZE);
                break;
            }
            case FUNCT3_SH: {  // store 16
                uint16_t v16 = (uint16_t)(inst.op2Val & 0xFFFF);
                myMem->setMemValue(inst.memAddress, (uint64_t)v16, HALF_SIZE);
                break;
            }
            case FUNCT3_SW: {  // store 32
                uint32_t v32 = (uint32_t)(inst.op2Val & 0xFFFFFFFFULL);
                myMem->setMemValue(inst.memAddress, (uint64_t)v32, WORD_SIZE);
                break;
            }
            case FUNCT3_SD: {  // store 64
                myMem->setMemValue(inst.memAddress, (uint64_t)inst.op2Val, DOUBLE_SIZE);
                break;
            }
            default:
                break;
        }
    }

    return inst;
}


// Write back results to registers
Instruction simCommit(Instruction inst, REGS &regData) {

    if (inst.writesRd && inst.rd != 0) {
        regData.registers[inst.rd] = inst.arithResult;
    }

    // Belt-and-suspenders: x0 is always zero.
    regData.registers[0] = 0;

    return inst;
}

// Simulate the whole instruction using functions above
Instruction simInstruction(uint64_t &PC, MemoryStore *myMem, REGS &regData) {
    Instruction inst = simFetch(PC, myMem);
    inst = simDecode(inst);
    if (inst.isHalt) {
        return inst;
    }
    
    if (inst.isNop) {
        inst.nextPC = inst.PC + 4;
        PC = inst.nextPC;
        return inst;
    }

    if (!inst.isLegal) {
        return inst;
    }
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



