# ======================================================
# BRANCH TEST (no pseudo)
# ======================================================

addi x1, x0, 5
addi x2, x0, 5
addi x3, x0, 3
addi x10, x0, 0

# BEQ taken
beq x1, x2, label_eq
addi x10, x10, 99     # skipped
label_eq:

# BNE not taken
bne x1, x2, label_ne
addi x10, x10, 1      # x10 = 1
label_ne:

# BLT taken
blt x3, x1, label_blt
addi x10, x10, 100    # skipped
label_blt:

# BGE not taken
bge x3, x1, label_bge
addi x10, x10, 2      # x10 = 3
label_bge:

.word 0xfeedfeed
