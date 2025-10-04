# ======================================================
# ARITHMETIC + LOGICAL TEST (no pseudo-instr)
# ======================================================

addi x1, x0, 5         # x1 = 5
addi x2, x0, 3         # x2 = 3
addi x10, x0, -2       # x10 = -2

# ---- R-TYPE ----
add x3, x1, x2         # 8
sub x4, x1, x2         # 2
and x5, x1, x2         # 1
or  x6, x1, x2         # 7
xor x7, x1, x2         # 6
sll x8, x2, x1         # 3 << 5 = 96
srl x9, x8, x2         # 96 >> 3 = 12
sra x11, x10, x2       # -2 >> 3 = -1
slt x12, x10, x1       # -2 < 5 = 1
sltu x13, x10, x1      # unsigned(-2) < 5 = 0
addw x14, x1, x2       # 8
subw x15, x1, x2       # 2

# ---- I-TYPE ----
addi x16, x1, -1       # 4
andi x17, x1, 1        # 1
ori  x18, x1, 8        # 13
xori x19, x1, 6        # 3
slli x20, x2, 2        # 3 << 2 = 12
srli x21, x20, 1       # 12 >> 1 = 6
srai x22, x10, 1       # -2 >> 1 = -1
slti x23, x10, 0       # 1
sltiu x24, x10, 0      # 0

.word 0xfeedfeed
