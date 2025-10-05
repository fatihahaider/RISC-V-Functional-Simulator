# ======================================================
# 32-BIT WORD INSTRUCTION TEST (RV64W)
# ======================================================

addiw x1, x0, 1
slliw x1, x1, 5          # x1 = 0x100000000
addiw x2, x1, 5          # low 32 bits + sign extend → x2 = 5
slliw x3, x2, 2          # shift left 2 bits → x3 = 20
srliw x4, x3, 1          # shift right logical → x4 = 10
sraiw x5, x3, 2          # shift right arithmetic → x5 = 5
subw x6, x3, x5          # 20 - 5 = 15
sllw x7, x5, x2          # 5 << 5 = 160
sraw x7, x5, x2

.word 0xfeedfeed
