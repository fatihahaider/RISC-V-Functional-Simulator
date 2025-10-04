# ======================================================
# JUMP + UPPER IMMEDIATE TEST (no pseudo)
# ======================================================

# ---- LUI + AUIPC ----
lui x1, 0x12345       # x1 = 0x12345000
auipc x2, 0x10        # x2 = PC + 0x10000

# ---- JAL ----
jal x3, label1        # jump forward, x3 = PC+4
addi x4, x0, 111      # skipped
label1:
addi x5, x0, 222      # executed

# ---- JALR ----
addi x6, x0, label2   # offset to label2 (simplified)
add x6, x6, x0
jalr x7, 0(x6)        # jump to label2
addi x8, x0, 333      # skipped
label2:
addi x9, x0, 444      # executed

.word 0xfeedfeed
