#include <stdio.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <string>

#include "MemoryStore.h"
#include "RegisterInfo.h"

// --------------------------------------------------------------------------
// Reg data structure
// --------------------------------------------------------------------------

union REGS {
    RegisterInfo reg;
    uint64_t registers[REG_SIZE] {0};
};

union REGS regData;

uint64_t PC;

// --------------------------------------------------------------------------
// Decode constants
// --------------------------------------------------------------------------

// TODO Complete these enums, add more if necessary

enum OPCODES {
    // I-type opcodes
    OP_INTIMM  = 0b0010011, // Integer ALU immediate instructions addi, slli, slti, sltiu, xori, srli, srai, ori, andi
    OP_INTIMMW = 0b0011011, // Integer ALU immediate instructions (keep low 32-bits and sign extends) addiw, slliw, srliw, sraiw
    OP_LOAD = 0b0000011, // lb, lh, lw, ld, lbu, lhu, lwu

    // R-Type opcodes
    OP_RTYPE = 0b0110011, // Register to Register instructions add, sub, sll, slt, sltu, xor, srl, sra, or, and
    OP_RTYPEW = 0b0111011, // Register to Register instructions (keep low 32-bits and sign extends) addw, subw, sllw, srlw, sraw

    // S-TYPE opcodes 
    OP_STORE = 0b0100011, // sb, sh, sw, sd

    // SB-TYPE opcodes
    OP_SBTYPE = 0b1100011, // Branching instructions beq, bne, blt, bge, bltu, bgeu

    // U type instruction opcode
    OP_LUI = 0b0110111, // lui
    OP_AUIPC = 0b0010111, // auipc

    // Jump type instruction opcode
    OP_JALR = 0b1100111, // jalr
    OP_JAL = 0b1101111 //jal
};

enum FUNCT3 {
    // For integer ALU instructions
    FUNCT3_ADD = 0b000, // add
    FUNCT3_SLL = 0b001, 
    FUNCT3_SLT = 0b010,
    FUNCT3_XOR = 0b100, 
    FUNCT3_SRL_SRA = 0b101,
    FUNCT3_OR = 0b110,
    FUNCT3_AND = 0b111,

    FUNCT3_SUB = 0b000,
    FUNCT3_SLTU = 0b011,
    

    // For load instructions
    FUNCT3_LB = 0b000,
    FUNCT3_LH = 0b001,
    FUNCT3_LW = 0b010,
    FUNCT3_LD  = 0b011,
    FUNCT3_LBU = 0b100,
    FUNCT3_LHU = 0b101,
    FUNCT3_LWU = 0b110,

    // For branch instructions 
    FUNCT3_BEQ = 0b000,
    FUNCT3_BNE = 0b001,
    FUNCT3_BLT = 0b100,
    FUNCT3_BGE = 0b101,
    FUNCT3_BLTU = 0b110,
    FUNCT3_BGEU = 0b111,

     // For store instructions
    FUNCT3_SB = 0b000,
    FUNCT3_SH = 0b001,
    FUNCT3_SW = 0b010,
    FUNCT3_SD = 0b011,
};

enum RI_FUNCT7 { 
    FUNCT7_DEFAULT= 0b0000000, // sub
    FUNCT7_SUB_SRA = 0b0100000, // funcs with distint funct7 code sub, srai, sraiw, sra, subw, sraw
};

// --------------------------------------------------------------------------
// Bit-level manipulation helpers
// --------------------------------------------------------------------------

// TODO You may wish to declare some helper functions for bit extractions
// and sign extensions

// --------------------------------------------------------------------------
// Utilities
// --------------------------------------------------------------------------

// initialize memory with program binary
bool initMemory(char *programFile, MemoryStore *myMem);

// dump registers and memory
void dump(MemoryStore *myMem);

// added: extracts bits 
uint64_t bitExtract(uint64_t instruction, int high, int low);

// added: sign extends 
uint64_t signExtend(uint64_t code, int num);

// --------------------------------------------------------------------------
// Simulation functions
// --------------------------------------------------------------------------

// The simulator maintains the following struct as it simulates 
// RISC-V instructions. Feel free to add more fields if needed.
struct Instruction {
    uint64_t PC = 0;
    uint64_t instruction = 0; // raw instruction binary

    bool     isHalt = false;
    bool     isLegal = false;
    bool     isNop = false;

    bool     readsMem = false;
    bool     writesMem = false;
    bool     doesArithLogic = false;
    bool     writesRd = false;
    bool     readsRs1 = false;
    bool     readsRs2 = false;

    uint64_t opcode = 0;
    uint64_t funct3 = 0;
    uint64_t funct7 = 0;
    uint64_t rd = 0;
    uint64_t rs1 = 0;
    uint64_t rs2 = 0;

    uint64_t nextPC = 0;

    uint64_t op1Val = 0;
    uint64_t op2Val = 0;

    uint64_t arithResult = 0;
    uint64_t memAddress = 0;
    uint64_t memResult = 0;
};

// The following functions are the core of the simulator. Your task is to
// complete these functions in sim.cpp. Do not modify their signatures.
// However, feel free to declare more functions if needed.

// There is no strict rule on what each function should do, but the
// following comments give suggestions.

// Get raw instruction bits from memory
Instruction simFetch(uint64_t PC, MemoryStore *myMem);

// Determine instruction opcode, funct, reg names, and what resources to use
Instruction simDecode(Instruction inst);

// Collect reg operands for arith or addr gen
Instruction simOperandCollection(Instruction inst, REGS regData);

// Resolve next PC whether +4 or branch/jump target
Instruction simNextPCResolution(Instruction inst);

// Perform arithmetic/logic operations
Instruction simArithLogic(Instruction inst);

// Generate memory address for load/store instructions
Instruction simAddrGen(Instruction inst);

// Perform memory access for load/store instructions
Instruction simMemAccess(Instruction inst, MemoryStore *myMem);

// Write back results to registers
Instruction simCommit(Instruction inst, REGS &regData);

// Simulate the whole instruction using functions above
Instruction simInstruction(uint64_t &PC, MemoryStore *myMem, REGS &regData);