# ======================================================
# LOAD + STORE TEST (pure RV64I)
# ======================================================

# ---- Initialize registers manually ----
addi x1, x0, 10         # x1 = 10 (data value)
lui  x2, 0x0            # upper 20 bits = 0
addi x2, x2, 64         # x2 = 0x40 (some small memory address)

# ---- STORE ----
sw   x1, 0(x2)          # store word x1 -> Mem[x2]

# ---- LOAD ----
lw   x3, 0(x2)          # load word Mem[x2] -> x3

# ---- Verify with another immediate ----
addi x4, x0, 15         # x4 = 15
add  x5, x3, x4         # x5 = 25 expected

.word 0xfeedfeed
