# ======================================================
# 32-BIT WORD INSTRUCTION TEST (RV64W)
# ======================================================

addiw x1, x0, 1          # x1 = 1
slliw x1, x1, 5          # x1 = 1 << 5 = 32
addiw x2, x1, 5          # x2 = (32 + 5) = 37
slliw x3, x2, 2          # x3 = 37 << 2 = 148
srliw x4, x3, 1          # x4 = 148 >> 1 (logical) = 74
sraiw x5, x3, 2          # x5 = 148 >> 2 (arith)   = 37
subw  x6, x3, x5         # x6 = 148 - 37 = 111
sllw  x7, x5, x2         # x7 = 37 << 37 = (shift by 5 bits mod 32) = 37 << 5 = 1184
sraw  x7, x5, x2         # x7 = 37 >> 5 (arith) = 1

.word 0xfeedfeed
