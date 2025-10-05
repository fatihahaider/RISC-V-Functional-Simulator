# ======================================================
# UNSIGNED BRANCH TEST (BLTU / BGEU)
# ======================================================

addi x1, x0, -1         # x1 = 0xFFFFFFFFFFFFFFFF (large unsigned)
addi x2, x0, 5          # x2 = 5
addi x10, x0, 0         # result tracker

# BLTU should NOT be taken (since x1 > x2 unsigned)
bltu x1, x2, skip1
addi x10, x10, 1        # executed
skip1:

# BGEU should be taken (since x1 >= x2 unsigned)
bgeu x1, x2, skip2
addi x10, x10, 99       # skipped
skip2:

# BLTU should be taken (0 < 5)
addi x3, x0, 0
bltu x3, x2, done
addi x10, x10, 100      # skipped
done:

.word 0xfeedfeed
