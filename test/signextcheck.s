# ======================================================
# SIGN EXTENSION TEST
# ======================================================

# x1 = -1          (addi with imm = -1)
# x2 = -2048       (addi with imm = -2048 → 12-bit min)
# x3 = -1          (loaded negative from memory)
# x4 = +4095       (addi with imm = 4095 → 12-bit max)
# x5 = 0xFFFFF000  (lui sign extension check)
# x6 = 0xFFFFF000  (auipc sign extension check)
# x7 = 1           (branch backward taken correctly)
# x8 = 2           (confirm branch offset sign extended)
# ======================================================

# --- I-TYPE SIGN EXTENSION ---
addi x1, x0, -1          # x1 = 0xFFFFFFFFFFFFFFFF
addi x2, x0, 2047        # x2 = 0x7FF (max +12-bit)
addi x3, x0, -2048       # x3 = 0xFFFFFFFFFFFFF800
add  x4, x1, x2          # x4 = -1 + 2047 = 2046
add  x5, x3, x2          # x5 = (-2048) + 2047 = -1

# --- S-TYPE SIGN EXTENSION ---
lui  x10, 0x1            # x10 = 0x1000
addi x11, x0, -1         # x11 = -1
sd   x11, -8(x10)        # store -1 at (0x1000 - 8)
ld   x3, -8(x10)         # x3 = -1 (confirm loaded correctly)

# --- U-TYPE SIGN EXTENSION ---
lui   x5, 0xFFFFF        # upper imm sign-extends (x5 = 0xFFFFF000)
auipc x6, 0xFFFFF        # PC-relative with negative upper bits

# --- B-TYPE SIGN EXTENSION (branch backwards) ---
addi x7, x0, 0
addi x8, x0, 0
label_loop:
addi x7, x7, 1           # increment
blt  x7, x4, label_loop  # loop while x7 < 4095 (branch backward)
addi x8, x8, 2           # x8 = 2 after loop completes

.word 0xfeedfeed
