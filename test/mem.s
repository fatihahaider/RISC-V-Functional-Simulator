# ======================================================
# BASIC MEMORY TESTS
# ======================================================

addi x1, x0, 64           # base address
addi x2, x0, 42           # store value

# BYTE
sb  x2, 0(x1)
lb  x3, 0(x1)
lbu x4, 0(x1)

# HALFWORD
addi x5, x0, 300
sh  x5, 2(x1)
lh  x6, 2(x1)
lhu x7, 2(x1)

# WORD
addi x8, x0, 1000
sw  x8, 4(x1)
lw  x9, 4(x1)

# DOUBLEWORD (RV64)
sd  x8, 8(x1)
ld  x10, 8(x1)

.word 0xfeedfeed
